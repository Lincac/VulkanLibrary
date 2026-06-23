# matVkEngine

Vulkan **光栅渲染引擎**，核心目标是 **自由可配置管线**：用户用 **C++ struct** 描述 Pass 链、Pass 间资源、材质与 Shader，引擎编译为 Vulkan 对象并按序调度。**Forward、Deferred 都不是写死在引擎里的固定路径**，只是两种（或多种）管线 preset 配置。

**当前范围：**

- **光栅 Pass 链可配置**：Draw Pass（场景网格）、Fullscreen Pass（全屏三角/quad）可组合；通过 Pass 输入/输出衔接，可搭 Forward，也可搭 Deferred（GBuffer → Lighting）。
- **C++ struct 配置**：不做 JSON；日后 JSON 仅填充同一套 struct。
- **离屏验证**：Color/Depth attachment → readback PNG；不做 Swapchain。

**暂不做：** 光追、JSON SceneLoader、窗口呈现、可视化管线编辑器。

---

## 术语约定

| 记号 | 含义 |
|------|------|
| 渲染管线（Render Pipeline） | 有序 Pass 列表 + Pass 间资源声明；Forward / Deferred 均为其实例 |
| Pass | 一次 `VkRenderPass` 录制单元，含 attachment 与执行方式 |
| Draw Pass | 遍历场景 Entity，按 `passName` / `drawGroup` 筛选并 `vkCmdDraw` |
| Fullscreen Pass | 全屏 draw，用于 Deferred lighting、后处理等 |
| 管线资源（Pipeline Resource） | Pass 间传递的离屏 Image（如 GBuffer 的 albedo/normal） |
| 材质（Material） | shader、descriptor、渲染状态；绑定到特定 Pass |
| 场景（Scene） | Entity、Camera、Light；与管线配置分离 |

---

## 设计原则

### 引擎不提供「固定 Forward / 固定 Deferred 类」

| 错误做法 | 本工程做法 |
|----------|------------|
| `ForwardRenderer` 内写死单 Pass draw | `RasterRenderer` 只执行 `RenderPipelineConfig` 中的 Pass 序 |
| `DeferredRenderer` 内写死 GBuffer + Light | Demo 用 preset 描述 GBuffer Pass + Fullscreen Pass |
| 换渲染方案改 C++ Renderer 源码 | 换 `RenderPipelineConfig` + 对应 Material / Shader |

### 用户可配置项

| 类别 | 内容 | 配置 |
|------|------|------|
| Pass 链 | Pass 数量、顺序、类型（Draw / Fullscreen） | `RenderPipelineConfig` |
| Attachment | 每 Pass 的 color 数量与 format、depth、clear | `PassConfig` |
| Pass 间资源 | 上一 Pass 输出 → 下一 Pass shader 输入 | `PassResourceConfig` |
| 材质 / Shader | 每 Pass 用哪套 vert/frag、binding、渲染状态 | `MaterialConfig`（含 `targetPass`） |
| 场景 | mesh、transform、材质引用、相机、光 | `SceneConfig` |
| 输出 | 分辨率、最终 PNG 取自哪个 Pass 的哪个 attachment | `RenderConfig` |

### 引擎固定项

- 顶点布局：`Vertex { pos, normal }`（`matVkEngineTypes.h`）
- Shader 入口：`main`
- 呈现：离屏 + PNG
- Pass 执行器类型：仅 **Draw** 与 **Fullscreen** 两种（可扩展，但不写死 Forward/Deferred 逻辑）

---

## Forward vs Deferred：同一套机制

### Forward preset（单 Draw Pass）

```
Pass "forward"
  attachments: color0 + depth
  type: Draw
  drawGroups: Opaque, Transparent
  materials: forward lit / unlit（targetPass = "forward"）
  → 输出 color0 → PNG
```

### Deferred preset（GBuffer Draw + Lighting Fullscreen）

```
Pass "gbuffer"
  attachments: color0=albedo, color1=normal, color2=material, depth
  type: Draw
  drawGroups: Opaque
  materials: gbuffer write（targetPass = "gbuffer"）

Pass "deferred_light"
  attachments: color0=hdr
  type: Fullscreen
  inputs: gbuffer.albedo, gbuffer.normal, gbuffer.depth, ...（Pipeline Resource 引用）
  material: deferred_light.frag（targetPass = "deferred_light"）

  → 输出 hdr → PNG
```

**切换 Forward ↔ Deferred = 换 `RenderPipelineConfig` + 换 Material/Shader preset，不改 `RasterRenderer`。**

---

## 数据流

```
RenderPipelineConfig    MaterialConfig[]    SceneConfig
         │                     │                  │
         └──────────┬──────────┴──────────────────┘
                    ▼
         VkEngineRenderPipeline::build()
           · 分配/别名 Pipeline Resource（Pass 间 Image）
           · 为每 Pass 创建 RenderPass + Framebuffer
           · 注册 Material → PipelineCache
                    ▼
         VkEngineRasterRenderer::render(scene, pipeline)
           for (pass : pipeline.passesInOrder)
               if DrawPass      → draw 匹配 targetPass 的 entities
               if FullscreenPass → draw 全屏 + 绑定 inputs
                    ▼
         pipeline.getOutputImage() → saveToPng()
```

---

## 目标目录结构

**`✓` = 已有，`+` = 待新增，`—` = 暂不做**

```
matVkEngine/
├── matVkEngine/
│   ├── config/
│   │   ├── matVkEngineConfig.h                 ✓
│   │   ├── matVkEngineRenderConfig.h           + 分辨率、最终输出 Pass/attachment
│   │   ├── matVkEngineRenderPipelineConfig.h   + Pass 链、Pass 类型、资源 IO
│   │   ├── matVkEnginePassConfig.h             + 单 Pass attachment / clear / inputs
│   │   ├── matVkEngineMaterialConfig.h         + shader、binding、state、targetPass
│   │   └── matVkEngineSceneConfig.h            + Entity / Camera / Light
│   │
│   ├── core/                                     ✓
│   ├── device/                                   ✓（FrameSync + 可选）
│   ├── platform/                                 — 暂不做
│   │
│   ├── resource/
│   │   ├── matVkEngineBuffer/Image/Texture       ✓
│   │   ├── matVkEngineGpuMesh.*                  +
│   │   └── func/                                 ✓
│   │
│   ├── shader/                                   ✓
│   │
│   ├── pipeline/                                 + 可配置管线核心
│   │   ├── matVkEngineRenderPass.*               + 由 PassConfig 创建（支持多 color attachment）
│   │   ├── matVkEngineFramebuffer.*
│   │   ├── matVkEngineGraphicsPipeline.*
│   │   ├── matVkEnginePipelineCache.*
│   │   ├── matVkEnginePipelineResource.*         + Pass 间 Image 注册表与 lifetime
│   │   └── matVkEngineRenderPipeline.*           + build / 查询 Pass 输出
│   │
│   ├── rt/                                       — 暂不做
│   │
│   ├── scene/
│   │   ├── matVkEngineTransform/Camera/Light     +
│   │   ├── matVkEngineMaterial.*                 +
│   │   ├── matVkEngineEntity/Scene.*             +
│   │   └── matVkEngineSceneLoader.*              — 暂不做
│   │
│   ├── renderer/
│   │   ├── matVkEngineRenderer.h                 + render / release 接口
│   │   ├── matVkEngineRasterRenderer.*           + 通用 Pass 链调度（非 Forward 专用）
│   │   └── matVkEngineFullscreenDraw.*           + 全屏 Pass 辅助（可选独立小模块）
│   │
│   └── common/
│       ├── matVkEngineCommon/Mesh                ✓
│       ├── matVkEngineTypes.h                    + Vertex、SceneUBO 等
│       └── matVkEngineFileUtil.h                 + PNG
│
└── matVkEngineDemo/
    ├── app/
    │   ├── DemoApp.*
    │   └── DemoPresets.*                         + makeForwardPipeline() / makeDeferredPipeline()
    ├── shaders/
    │   ├── forward/                              + phong.vert/frag
    │   ├── deferred/                             + gbuffer.vert/frag, lighting.frag
    │   └── compiled/
    ├── models/                                   ✓
    └── output/                                   + forward.png, deferred.png
```

---

## 配置 struct 要点

### `PassConfig`

```cpp
enum class PassType { Draw, Fullscreen };

struct PassAttachmentConfig {
    std::string name;           // 管线内资源名，如 "albedo", "hdr"
    VkFormat format;
};

struct PassConfig {
    std::string name;           // "forward" / "gbuffer" / "deferred_light"
    PassType type = PassType::Draw;
    std::vector<PassAttachmentConfig> colors;
    std::optional<PassAttachmentConfig> depth;
    ClearValueConfig clear{};
    std::vector<DrawGroup> drawGroups;              // Draw Pass 用
    std::vector<std::string> inputResources;        // Fullscreen / 后续 Pass 引用的上游资源名
};
```

### `RenderPipelineConfig`

```cpp
struct RenderPipelineConfig {
    std::string name;
    std::vector<PassConfig> passes;
    std::string outputPass;     // 最终 PNG 取自哪个 Pass
    std::string outputColor;    // 该 Pass 的哪个 color attachment 名
};
```

### `MaterialConfig`

```cpp
struct MaterialConfig {
    std::string name;
    std::string targetPass;     // 本材质在哪个 Pass 使用
    std::string vertSpv;
    std::string fragSpv;
    DrawGroup drawGroup = DrawGroup::Opaque;
    std::vector<BindingConfig> bindings;
    RasterStateConfig state{};
};
```

`SceneConfig` 持有 `RenderPipelineConfig`、entities（每个 entity 引用 `MaterialConfig`）、camera、lights。

---

## 配置示例：Forward preset

```cpp
RenderPipelineConfig makeForwardPipeline() {
    RenderPipelineConfig cfg{};
    cfg.name = "forward";
    cfg.passes = {{
        .name = "forward",
        .type = PassType::Draw,
        .colors = {{.name = "color", .format = VK_FORMAT_R8G8B8A8_UNORM}},
        .depth = {{.name = "depth", .format = VK_FORMAT_D32_SFLOAT}},
        .drawGroups = {DrawGroup::Opaque, DrawGroup::Transparent},
    }};
    cfg.outputPass = "forward";
    cfg.outputColor = "color";
    return cfg;
}
```

## 配置示例：Deferred preset

```cpp
RenderPipelineConfig makeDeferredPipeline() {
    RenderPipelineConfig cfg{};
    cfg.name = "deferred";
    cfg.passes = {
        {
            .name = "gbuffer",
            .type = PassType::Draw,
            .colors = {
                {.name = "albedo", .format = VK_FORMAT_R8G8B8A8_UNORM},
                {.name = "normal", .format = VK_FORMAT_R16G16B16A16_SFLOAT},
            },
            .depth = {{.name = "depth", .format = VK_FORMAT_D32_SFLOAT}},
            .drawGroups = {DrawGroup::Opaque},
        },
        {
            .name = "deferred_light",
            .type = PassType::Fullscreen,
            .colors = {{.name = "hdr", .format = VK_FORMAT_R8G8B8A8_UNORM}},
            .inputResources = {"gbuffer.albedo", "gbuffer.normal", "gbuffer.depth"},
        },
    };
    cfg.outputPass = "deferred_light";
    cfg.outputColor = "hdr";
    return cfg;
}
```

Material 示例：`gbuffer_write` 的 `targetPass = "gbuffer"`；`deferred_light` 的 `targetPass = "deferred_light"`，bindings 引用 GBuffer 纹理。

---

## 离屏呈现

- 所有 Pass attachment 由 `VkEnginePipelineResource` 统一分配（尺寸 = `RenderConfig` 宽高）。
- Pass 间 **barrier + layout 转换** 在 `RasterRenderer` 按 Pass 边界自动插入（`SHADER_READ_ONLY` / `COLOR_ATTACHMENT`）。
- 最终 `outputPass.outputColor` → `TRANSFER_SRC` → `saveToPng()`。

---

## 各目录职责

| 目录 | 职责 |
|------|------|
| `config/` | RenderPipeline / Pass / Material / Scene struct |
| `pipeline/` | Pass 编译、Pipeline Resource 表、GraphicsPipeline 缓存 |
| `scene/` | 场景数据；Material 按 `targetPass` 分组 |
| `renderer/` | **`RasterRenderer`：通用 Pass 链执行器** |
| 其余 | 与先前相同 |

---

## 依赖方向

```
config ──► core ──► device ──► resource / shader
                                    │
                          ┌─────────┴─────────┐
                          ▼                   ▼
              pipeline (RenderPipeline)    scene
                          │                   │
                          └────► RasterRenderer
                                      │
                                 离屏 PNG
```

- `RasterRenderer` **不出现** Forward/Deferred 分支；只认 `PassType` 与 `RenderPipelineConfig`
- Forward / Deferred 逻辑完全在 **preset + shader + material** 侧

---

## 建议实现顺序

```
阶段 1 — 基础
  ✓ shader/*、resource/Buffer, Image, Texture
  + Types、GpuMesh
  + PassConfig、RenderPipelineConfig、MaterialConfig、SceneConfig、RenderConfig

阶段 2 — 可配置管线框架（先 Forward preset 验证）
  + PipelineResource、RenderPass（多 RT）、Framebuffer
  + GraphicsPipeline、PipelineCache、RenderPipeline::build
  + RasterRenderer（仅 Draw Pass）
  → Demo：makeForwardPipeline() → output/forward.png

阶段 3 — Pass 间资源 + Fullscreen Pass（打开 Deferred）
  + Pass 输入资源绑定、Pass 间 barrier
  + FullscreenDraw、RasterRenderer 支持 PassType::Fullscreen
  → Demo：makeDeferredPipeline() → output/deferred.png

阶段 4 —  polish
  + 多 Draw Pass（opaque → transparent 等）
  + Demo 切换 preset 即可对比 Forward / Deferred

— 将来：JSON loader、Swapchain、RT
```

---

## Demo 目标形态

```cpp
RenderConfig render = DemoPresets::makeRenderConfig();

// 二选一，或命令行切换 preset 名
RenderPipelineConfig pipeline = DemoPresets::makeForwardPipeline();
// RenderPipelineConfig pipeline = DemoPresets::makeDeferredPipeline();

SceneConfig scene = DemoPresets::makeScene(pipeline);  // materials 与 pipeline 匹配

DemoApp app;
app.init(render);
app.loadPipeline(pipeline);
app.loadScene(scene);
app.render();       // RasterRenderer
app.saveOutput(render.outputPath);
```

---

## 验收标准

| 项 | 标准 |
|----|------|
| Forward preset | `makeForwardPipeline()` 离屏 PNG 正确 |
| Deferred preset | `makeDeferredPipeline()` 离屏 PNG 正确 |
| 换 Pass 链 | 改 `RenderPipelineConfig`，**不改 RasterRenderer 源码** |
| 换材质 / Pass | Material 的 `targetPass` 决定在哪个 Pass draw |
| Pipeline 缓存 | 相同 Material + Pass attachment 格式不重复建 `VkPipeline` |
| RT / JSON / 窗口 | 不在当前范围 |

---

## 与现有代码的迁移点

| 现有 | 迁移到 |
|------|--------|
| 若已有 `ForwardPipeline*` / `ForwardRenderer` 命名 | 重命名为 `RenderPipeline` / `RasterRenderer` |
| Demo STORAGE buffer/image | Draw Pass：`VERTEX_BUFFER` + Pass attachment |
| `Vertex` | `matVkEngineTypes.h` |
| `shader/` | Material + PipelineCache 底层 |

---

## 命名对照（避免回退成固定 Forward）

| 避免 | 推荐 |
|------|------|
| `ForwardPipelineConfig` | `RenderPipelineConfig` |
| `VkEngineForwardPipeline` | `VkEngineRenderPipeline` |
| `ForwardRenderer` | `VkEngineRasterRenderer` |
| `ForwardUBO` | `SceneUBO` / `FrameUBO` |
| Demo 目录仅 `shaders/forward/` | 增加 `shaders/deferred/`，Forward 只是其中一套 shader |
