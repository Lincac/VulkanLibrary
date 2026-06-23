# matVkEngine

Vulkan 渲染引擎，核心目标是 **Forward 光栅管线可自定义**：用户通过配置文件描述 Pass 链、材质与 Shader，引擎编译并调度为 Vulkan 对象，无需修改 C++ 渲染代码即可切换前向渲染方案。

光追（Ray Tracing）作为独立后端保留，不参与 Forward 管线自定义体系。

---

## 术语约定

| 记号 | 含义 |
|------|------|
| Forward 管线 | 光栅化前向渲染路径（非 Deferred、非 RT） |
| Pass | 一次 RenderPass 录制：若干 attachment + 一批 draw |
| 管线配置 | 描述 Pass 顺序、attachment、混合/深度等状态的 JSON / struct |
| 材质（Material） | 引用 vert/frag shader、descriptor binding、渲染状态；Entity 通过材质决定如何绘制 |
| 场景配置 | Entity / Camera / Light 与所用管线配置、材质路径 |

---

## 设计目标：Forward 管线可自定义

### 用户可配置项

| 类别 | 可配置内容 | 配置入口 |
|------|------------|----------|
| Pass 链 | Pass 数量、顺序、每 Pass 的 color/depth attachment、clear 值 | `pipelines/*.json` |
| Shader | 每材质独立的 vert/frag `.spv` 路径 | `materials/*.json` |
| Descriptor | binding 槽位与 UBO / 贴图 / StorageBuffer 映射 | `materials/*.json` |
| 渲染状态 | cull mode、depth test/write、blend、polygon mode | `materials/*.json` |
| 场景 | mesh 路径、transform、引用哪份材质、相机与光源 | `scenes/*.json` |
| 输出 | 分辨率、离屏/窗口、输出 PNG 路径 | `RenderConfig` |

### 引擎固定项（不在此版本自定义）

- 顶点布局：统一 `Vertex { pos, normal }`（见 `matVkEngineTypes.h`）
- Shader 入口点名：`main`
- 后端类型：Forward 与 RT 二选一，Forward Pass 链与 RT 不混用
- 完整 Render Graph 编辑器、Shader Graph 节点编辑：不在范围内

### 数据流

```
pipelines/forward_*.json     materials/*.json     scenes/*.json
         │                          │                    │
         └──────────┬───────────────┴────────────────────┘
                    ▼
           VkEngineForwardPipeline（解析 + 编译）
                    │
      ┌─────────────┼─────────────┐
      ▼             ▼             ▼
 RenderPass[]  PipelineCache  DescriptorLayouts
      │             │             │
      └─────────────┴─────────────┘
                    ▼
         VkEngineForwardRenderer::render(scene)
                    │
                    ▼
              CommandBuffer（按 Pass 顺序 draw）
```

---

## 目标目录结构

根目录保持 `matVkEngine/`（解决方案）不变。**`✓` = 已有，`+` = 待新增**。

```
matVkEngine/
├── matVkEngine.sln
├── .clang-format
├── README.md
│
├── matVkEngine/                          # 静态库（引擎本体）
│   ├── matVkEngine.vcxproj
│   ├── matVkEngine.vcxproj.filters
│   │
│   ├── config/
│   │   ├── matVkEngineConfig.h               ✓ Instance / 设备扩展
│   │   ├── matVkEngineRenderConfig.h         + 分辨率、帧数、后端枚举、输出路径
│   │   ├── matVkEngineForwardPipelineConfig.h + Pass 链、attachment 描述 struct
│   │   ├── matVkEngineMaterialConfig.h       + shader 路径、binding、渲染状态 struct
│   │   └── matVkEngineSceneConfig.h          + Entity / Camera / Light struct
│   │
│   ├── core/
│   │   ├── matVkEngineContext.h              ✓
│   │   └── matVkEngineContext.cpp            ✓
│   │
│   ├── device/
│   │   ├── matVkEnginePhysicalDevice.h       ✓
│   │   ├── matVkEnginePhysicalDevice.cpp     ✓
│   │   ├── matVkEngineLogicalDevice.h        ✓
│   │   ├── matVkEngineLogicalDevice.cpp      ✓
│   │   ├── matVkEngineCmdPool.h              ✓
│   │   ├── matVkEngineCmdPool.cpp            ✓
│   │   ├── matVkEngineFrameSync.h            + Semaphore / Fence / 多帧 in-flight
│   │   └── matVkEngineFrameSync.cpp          +
│   │
│   ├── platform/                             # 窗口与呈现（可后做）
│   │   ├── matVkEngineSurface.h              +
│   │   ├── matVkEngineSurface.cpp            +
│   │   ├── matVkEngineSwapchain.h            +
│   │   └── matVkEngineSwapchain.cpp          +
│   │
│   ├── resource/
│   │   ├── matVkEngineBuffer.h               ✓
│   │   ├── matVkEngineBuffer.cpp               ✓
│   │   ├── matVkEngineImage.h                  ✓
│   │   ├── matVkEngineImage.cpp                ✓
│   │   ├── matVkEngineTexture.h                ✓ 2D 贴图 / HDR
│   │   ├── matVkEngineTexture.cpp              ✓
│   │   ├── matVkEngineGpuMesh.h                + CPU Mesh → GPU buffer
│   │   ├── matVkEngineGpuMesh.cpp              +
│   │   └── func/                               ✓ stb
│   │
│   ├── shader/
│   │   ├── matVkEngineShaderModule.h           ✓ 加载 .spv
│   │   ├── matVkEngineShaderModule.cpp         ✓
│   │   ├── matVkEngineDescriptorSetLayout.h    ✓
│   │   ├── matVkEngineDescriptorSetLayout.cpp  ✓
│   │   ├── matVkEngineDescriptorPool.h         ✓
│   │   ├── matVkEngineDescriptorPool.cpp       ✓
│   │   ├── matVkEngineDescriptorSet.h          ✓ 绑定 buffer / image / sampler
│   │   ├── matVkEngineDescriptorSet.cpp        ✓
│   │   ├── matVkEngineSampler.h                + VkSampler 封装
│   │   └── matVkEngineSampler.cpp              +
│   │
│   ├── pipeline/                             # Forward 可自定义管线核心
│   │   ├── matVkEngineRenderPass.h             + 由 PassDesc 创建 RenderPass
│   │   ├── matVkEngineRenderPass.cpp           +
│   │   ├── matVkEngineFramebuffer.h            + 绑定 Pass 的 attachment
│   │   ├── matVkEngineFramebuffer.cpp          +
│   │   ├── matVkEngineGraphicsPipeline.h       + 由 MaterialConfig + RenderPass 创建
│   │   ├── matVkEngineGraphicsPipeline.cpp     +
│   │   ├── matVkEnginePipelineCache.h          + (shader, state, pass) → Pipeline 缓存
│   │   ├── matVkEnginePipelineCache.cpp        +
│   │   ├── matVkEngineForwardPipeline.h        + 解析管线配置、持有 Pass 链
│   │   └── matVkEngineForwardPipeline.cpp      +
│   │
│   ├── rt/                                   # 光追后端（固定流程，不可自定义 Pass 链）
│   │   ├── matVkEngineAccelerationStructure.h  +
│   │   ├── matVkEngineAccelerationStructure.cpp+
│   │   ├── matVkEngineRayTracingPipeline.h     +
│   │   ├── matVkEngineRayTracingPipeline.cpp   +
│   │   ├── matVkEngineRTDescriptor.h           +
│   │   ├── matVkEngineRTDescriptor.cpp         +
│   │   └── matVkEngineRTHelp.h                 +
│   │
│   ├── scene/
│   │   ├── matVkEngineTransform.h              +
│   │   ├── matVkEngineCamera.h                 +
│   │   ├── matVkEngineLight.h                  +
│   │   ├── matVkEngineMaterial.h               + 运行时材质：config + GPU descriptor
│   │   ├── matVkEngineMaterial.cpp             +
│   │   ├── matVkEngineEntity.h                 + transform + mesh + materialRef
│   │   ├── matVkEngineScene.h                  + entities + camera + lights + pipelineRef
│   │   ├── matVkEngineScene.cpp                + build GPU 资源
│   │   └── matVkEngineSceneLoader.h            + JSON → SceneConfig / MaterialConfig
│   │
│   ├── renderer/
│   │   ├── matVkEngineRenderer.h               + 抽象接口
│   │   ├── matVkEngineForwardRenderer.h        + 按 ForwardPipeline 的 Pass 链调度 draw
│   │   ├── matVkEngineForwardRenderer.cpp      +
│   │   ├── matVkEngineRayTracingRenderer.h     +
│   │   ├── matVkEngineRayTracingRenderer.cpp   +
│   │   └── matVkEngineRendererFactory.h        + 按 RenderBackend 创建
│   │
│   └── common/
│       ├── matVkEngineCommon.h                 ✓
│       ├── matVkEngineCommon.cpp               ✓
│       ├── matVkEngineMesh.h                   ✓ CPU OBJ/STL
│       ├── matVkEngineMesh.cpp                 ✓
│       ├── matVkEngineTypes.h                  + Vertex、ForwardUBO 等
│       └── matVkEngineFileUtil.h               + 读 JSON / spv、存 PNG
│
└── matVkEngineDemo/
    ├── matVkEngineDemo.vcxproj
    ├── main.cpp                                --pipeline --scene --backend
    │
    ├── app/
    │   ├── DemoApp.h                           +
    │   └── DemoApp.cpp                         +
    │
    ├── pipelines/                              # Forward 管线配置（用户自定义入口 ①）
    │   ├── forward_single_pass.json            # 单 Pass：color + depth
    │   └── forward_opaque_transparent.json     # 双 Pass：opaque → transparent
    │
    ├── materials/                              # 材质配置（用户自定义入口 ②）
    │   ├── phong.json
    │   ├── unlit_color.json
    │   └── textured.json
    │
    ├── scenes/                                 # 场景配置（用户自定义入口 ③）
    │   ├── forward_default.json
    │   └── rt_default.json
    │
    ├── shaders/
    │   ├── forward/
    │   │   ├── phong.vert / phong.frag
    │   │   ├── unlit.vert / unlit.frag
    │   │   └── textured.vert / textured.frag
    │   └── rt/
    │       ├── raygen.rgen / miss.rmiss / closesthit.rchit
    │   └── compiled/                           # .spv 输出
    │
    ├── models/                                 ✓
    ├── hdr/                                    +
    └── output/                                 +
```

---

## 配置文件示例

### `pipelines/forward_single_pass.json`

```json
{
  "name": "forward_single_pass",
  "passes": [
    {
      "name": "main",
      "colorFormat": "RGBA8_UNORM",
      "depthFormat": "D32_SFLOAT",
      "clearColor": [0.1, 0.1, 0.12, 1.0],
      "clearDepth": 1.0,
      "drawGroups": ["opaque", "transparent"]
    }
  ]
}
```

### `materials/phong.json`

```json
{
  "name": "phong",
  "vertSpv": "shaders/compiled/forward/phong.vert.spv",
  "fragSpv": "shaders/compiled/forward/phong.frag.spv",
  "drawGroup": "opaque",
  "bindings": [
    { "binding": 0, "type": "UBO", "stages": ["vertex", "fragment"] }
  ],
  "state": {
    "cullMode": "back",
    "depthTest": true,
    "depthWrite": true,
    "blend": false
  },
  "params": {
    "albedoColor": [0.8, 0.2, 0.2]
  }
}
```

### `scenes/forward_default.json`

```json
{
  "pipeline": "pipelines/forward_single_pass.json",
  "camera": { "eye": [0, 1, 3], "target": [0, 0, 0], "fov": 45 },
  "lights": [{ "direction": [0.3, -1, 0.2], "color": [1, 1, 1], "intensity": 1.0 }],
  "entities": [
    { "mesh": "models/bunny.obj", "material": "materials/phong.json", "translation": [0, 0, 0] },
    { "mesh": "models/quad.obj", "material": "materials/unlit_color.json", "translation": [0, -1, 0] }
  ]
}
```

---

## 各目录职责

| 目录 | 职责 |
|------|------|
| `config/` | 引擎、渲染、**Forward 管线**、材质、场景各层配置 struct |
| `core/` | `VkInstance` |
| `device/` | 物理/逻辑设备、命令池、帧同步 |
| `platform/` | Surface / Swapchain（可选） |
| `resource/` | Buffer、Image、Texture、GpuMesh |
| `shader/` | ShaderModule、Descriptor、Sampler |
| `pipeline/` | **Forward 可自定义管线**：Pass 编译、GraphicsPipeline、PipelineCache |
| `rt/` | 光追固定后端 |
| `scene/` | Material / Entity / Scene；引用管线与材质配置 |
| `renderer/` | ForwardRenderer 执行 Pass 链；RTRenderer 独立 |
| `common/` | 工具、CPU Mesh、公共 GPU struct |

---

## 依赖方向

```
config ──► core ──► device ──► resource / shader
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
               pipeline          rt            scene
           (ForwardPipeline)                  (Material/Scene)
                    │               │               │
                    └───────► renderer ◄────────────┘
                                    │
                              platform（可选）
```

- `pipeline/` 依赖 `config/`、`shader/`、`resource/`，**不依赖** `scene/`
- `scene/` 依赖 `config/`、`resource/`、`common/`，**不依赖** `renderer/`
- `renderer/ForwardRenderer` 依赖 `pipeline/VkEngineForwardPipeline` + `scene/VkEngineScene`
- `matVkEngineDemo` 只依赖 `renderer/` 与配置加载，不写 Vulkan 细节

---

## Forward 渲染一帧的逻辑

```
1. 加载 pipeline JSON → VkEngineForwardPipeline::build()
      → 为每个 Pass 创建 RenderPass + Framebuffer
2. 加载 scene JSON → VkEngineScene::build()
      → GpuMesh、Material（shader + descriptor + pipeline cache 查/建）
3. ForwardRenderer::render(scene, pipeline)
      for (pass : pipeline.passes)
          begin RenderPass(pass)
          for (drawGroup : pass.drawGroups)
              for (entity : scene.entitiesInGroup(drawGroup))
                  bind Material.pipeline + descriptor
                  push/update UBO（MVP、light、material.params）
                  draw entity.gpuMesh
          end RenderPass
4. 离屏：transition → copy → output.png
```

---

## 建议实现顺序

```
阶段 1 — 基础（部分已完成）
  ✓ shader/*
  ✓ resource/Buffer, Image, Texture
  + common/matVkEngineTypes.h
  + resource/VkEngineGpuMesh
  + shader/VkEngineSampler
  + config/matVkEngineRenderConfig.h
  + config/matVkEngineMaterialConfig.h
  + config/matVkEngineForwardPipelineConfig.h

阶段 2 — Forward 可自定义管线
  + pipeline/RenderPass, Framebuffer, GraphicsPipeline
  + pipeline/PipelineCache
  + pipeline/VkEngineForwardPipeline（解析配置 + build）
  + scene/Material, Entity, Scene
  + renderer/ForwardRenderer
  → Demo: pipelines/ + materials/ + scenes/ + 至少 2 种 shader（phong / unlit）

阶段 3 — 配置加载与多 Pass
  + scene/SceneLoader（JSON）
  + pipelines/forward_opaque_transparent.json（验证双 Pass）
  + common/FileUtil

阶段 4 — 光追后端（独立，不参与 Forward 自定义）
  + rt/* + renderer/RayTracingRenderer
  → Demo: scenes/rt_default.json

阶段 5 — 窗口呈现（可选）
  + platform/*
```

---

## Demo `main.cpp` 目标形态

```cpp
DemoApp app;
app.init(argc, argv);
// --backend=forward --pipeline=pipelines/forward_single_pass.json --scene=scenes/forward_default.json
app.loadForwardPipeline("pipelines/forward_single_pass.json");
app.loadScene("scenes/forward_default.json");
app.render();
app.saveOutput("output/forward.png");
```

切换为双 Pass 管线时，**只改 `--pipeline` 指向的 JSON**，场景与材质不用改 C++。

---

## 与现有代码的迁移点

| 现有 | 迁移到 |
|------|--------|
| `main.cpp` 手写 buffer upload | `VkEngineGpuMesh` |
| `VkEngineMesh` | 留 `common/`，GPU 侧交 `VkEngineGpuMesh` |
| `Vertex` | `common/matVkEngineTypes.h` |
| `shader/` 已完成部分 | 作为 Material / PipelineCache 的底层 |
| `vkEngine/src/rt/*` | `matVkEngine/rt/*`（光追后端，与 Forward 自定义无关） |

---

## 验收标准

| 项 | 标准 |
|----|------|
| 换材质 shader | 改 `materials/*.json` 的 spv 路径，画面变化 |
| 换 Pass 链 | 改 `pipelines/*.json`，如单 Pass ↔ opaque+transparent 双 Pass，无需改 Renderer 源码 |
| 换场景 | 改 `scenes/*.json` 的 entity / camera / light |
| Pipeline 缓存 | 相同 Material 不重复创建 `VkPipeline` |
| RT 后端 | `--backend=rt` 走固定 RT 路径，不使用 Forward Pass 配置 |
