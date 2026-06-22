## 目标目录结构

根目录保持 `matVkEngine/`（解决方案）不变，在现有分层上扩展。**`✓` = 已有，`+` = 待新增**。

```
matVkEngine/                              # 仓库根
├── matVkEngine.sln
├── .clang-format
│
├── matVkEngine/                          # 静态库（引擎本体）
│   ├── matVkEngine.vcxproj
│   ├── matVkEngine.vcxproj.filters
│   │
│   ├── config/                           # 配置与类型定义
│   │   ├── matVkEngineConfig.h           ✓ Instance / 设备扩展
│   │   ├── matVkEngineRenderConfig.h     + 分辨率、帧数、后端枚举、输出路径
│   │   └── matVkEngineSceneConfig.h      + 场景序列化 struct（Entity/Camera/Light）
│   │
│   ├── core/                             # Vulkan 实例
│   │   ├── matVkEngineContext.h          ✓
│   │   └── matVkEngineContext.cpp        ✓
│   │
│   ├── device/                           # 设备与同步
│   │   ├── matVkEnginePhysicalDevice.h   ✓
│   │   ├── matVkEnginePhysicalDevice.cpp ✓
│   │   ├── matVkEngineLogicalDevice.h    ✓
│   │   ├── matVkEngineLogicalDevice.cpp  ✓
│   │   ├── matVkEngineCmdPool.h          ✓
│   │   ├── matVkEngineCmdPool.cpp        ✓
│   │   ├── matVkEngineFrameSync.h        + Semaphore / Fence / 多帧 in-flight
│   │   └── matVkEngineFrameSync.cpp      +
│   │
│   ├── platform/                         # 窗口与呈现（可后做）
│   │   ├── matVkEngineSurface.h          + GLFW/VkSurfaceKHR 封装
│   │   ├── matVkEngineSurface.cpp        +
│   │   ├── matVkEngineSwapchain.h        + Swapchain + image views
│   │   └── matVkEngineSwapchain.cpp      +
│   │
│   ├── resource/                         # GPU 资源
│   │   ├── matVkEngineBuffer.h           ✓
│   │   ├── matVkEngineBuffer.cpp         ✓
│   │   ├── matVkEngineImage.h            ✓
│   │   ├── matVkEngineImage.cpp          ✓
│   │   ├── matVkEngineTexture.h          + 2D 贴图 / HDR 加载
│   │   ├── matVkEngineTexture.cpp        +
│   │   ├── matVkEngineGpuMesh.h          + CPU Mesh → GPU buffer + draw 参数
│   │   └── matVkEngineGpuMesh.cpp        +
│   │
│   ├── shader/                           # Shader 与描述符
│   │   ├── matVkEngineShaderModule.h     + 加载 .spv
│   │   ├── matVkEngineShaderModule.cpp   +
│   │   ├── matVkEngineDescriptorSetLayout.h +
│   │   ├── matVkEngineDescriptorSetLayout.cpp +
│   │   ├── matVkEngineDescriptorPool.h   +
│   │   ├── matVkEngineDescriptorPool.cpp +
│   │   ├── matVkEngineDescriptorSet.h    + 绑定 buffer / image / sampler / TLAS
│   │   └── matVkEngineDescriptorSet.cpp  +
│   │
│   ├── pipeline/                         # 管线（Forward 光栅）
│   │   ├── matVkEngineRenderPass.h       + RenderPass + depth
│   │   ├── matVkEngineRenderPass.cpp     +
│   │   ├── matVkEngineFramebuffer.h      +
│   │   ├── matVkEngineFramebuffer.cpp    +
│   │   ├── matVkEngineGraphicsPipeline.h + 前向渲染 Graphics Pipeline
│   │   └── matVkEngineGraphicsPipeline.cpp +
│   │
│   ├── rt/                               # 光追专用
│   │   ├── matVkEngineAccelerationStructure.h  + BLAS / TLAS
│   │   ├── matVkEngineAccelerationStructure.cpp +
│   │   ├── matVkEngineRayTracingPipeline.h     + RTPSO + SBT
│   │   ├── matVkEngineRayTracingPipeline.cpp   +
│   │   ├── matVkEngineRTDescriptor.h           + 光追 binding 集合
│   │   ├── matVkEngineRTDescriptor.cpp         +
│   │   └── matVkEngineRTHelp.h                 + RT 相关 struct / 工具（可 header-only）
│   │
│   ├── scene/                            # 场景组合（用户可配置单元）
│   │   ├── matVkEngineTransform.h        + model 矩阵
│   │   ├── matVkEngineCamera.h           + view / proj
│   │   ├── matVkEngineLight.h            + 方向光等
│   │   ├── matVkEngineMaterial.h         + shader 参数引用
│   │   ├── matVkEngineEntity.h           + transform + mesh + material
│   │   ├── matVkEngineScene.h            + entities + camera + lights
│   │   ├── matVkEngineScene.cpp          + build GPU 资源
│   │   └── matVkEngineSceneLoader.h      + 可选：JSON 解析（后做）
│   │
│   ├── renderer/                         # 渲染调度
│   │   ├── matVkEngineRenderer.h         + 抽象接口 create / render / resize / release
│   │   ├── matVkEngineForwardRenderer.h  + 前向光栅实现
│   │   ├── matVkEngineForwardRenderer.cpp+
│   │   ├── matVkEngineRayTracingRenderer.h +
│   │   ├── matVkEngineRayTracingRenderer.cpp +
│   │   └── matVkEngineRendererFactory.h  + 按 RenderBackend 创建
│   │
│   └── common/                           # 通用工具
│       ├── matVkEngineCommon.h           ✓ layout barrier / findMemoryType
│       ├── matVkEngineCommon.cpp         ✓
│       ├── matVkEngineMesh.h             ✓ CPU OBJ/STL
│       ├── matVkEngineMesh.cpp           ✓
│       ├── matVkEngineTypes.h            + Vertex、GPU UBO struct 等公共类型
│       └── matVkEngineFileUtil.h         + 可选：读 spv、存 PNG、路径解析
│
└── matVkEngineDemo/                      # 可执行 Demo
    ├── matVkEngineDemo.vcxproj
    ├── matVkEngineDemo.vcxproj.filters
    ├── main.cpp                          入口：解析参数 → 加载场景 → 选后端 → 渲染
    │
    ├── app/                              # Demo 应用层（薄封装，不放进引擎库）
    │   ├── DemoApp.h                     + 初始化 engine、跑一帧/循环
    │   └── DemoApp.cpp
    │
    ├── scenes/                           # 场景配置（用户改这里拼场景）
    │   ├── forward_default.json          + Forward 测试场景
    │   └── rt_default.json               + 光追测试场景
    │
    ├── shaders/                          # GLSL 源码（编译产物放 compiled/）
    │   ├── forward/
    │   │   ├── forward.vert
    │   │   └── forward.frag
    │   └── rt/
    │       ├── raygen.rgen
    │       ├── miss.rmiss
    │       └── closesthit.rchit
    │
    ├── shaders/compiled/                 # glslangValidator / spirv 输出（运行时加载）
    │   ├── forward/
    │   │   ├── forward.vert.spv
    │   │   └── forward.frag.spv
    │   └── rt/
    │       ├── raygen.spv
    │       ├── miss.spv
    │       └── closesthit.spv
    │
    ├── models/                           ✓ 已有 OBJ
    │   ├── bunny.obj
    │   ├── dragon.obj
    │   ├── quad.obj
    │   ├── scene.obj
    │   └── sphere.obj
    │
    ├── hdr/                              + 光追环境贴图
    │   └── env.hdr
    │
    └── output/                           + 离屏渲染输出（gitignore）
        ├── forward.png
        └── rt.png
```

---

## 各目录职责（一句话）

| 目录 | 职责 |
|------|------|
| `config/` | 引擎、渲染、场景三层配置 struct |
| `core/` | `VkInstance` |
| `device/` | 物理/逻辑设备、命令池、帧同步 |
| `platform/` | Surface / Swapchain（有窗口时再实现） |
| `resource/` | Buffer、Image、Texture、GpuMesh |
| `shader/` | ShaderModule + Descriptor 全家桶 |
| `pipeline/` | Forward 专用：RenderPass、Framebuffer、GraphicsPipeline |
| `rt/` | 光追专用：AS、RT Pipeline、RT Descriptor |
| `scene/` | Transform / Camera / Light / Material / Entity / Scene |
| `renderer/` | 对外统一渲染接口 + Forward/RT 两个实现 |
| `common/` | 工具、CPU Mesh、公共 GPU struct |

---

## 依赖方向（实现时不要反向 include）

```
config ──► core ──► device ──► resource / shader
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
               pipeline          rt            scene
                    │               │               │
                    └───────► renderer ◄────────────┘
                                    │
                              platform（可选）
```

- `scene/` 只依赖 `resource/`、`common/`，**不依赖** `renderer/`、`pipeline/`、`rt/`
- `renderer/` 依赖 `scene/` + 对应后端（Forward 用 `pipeline/`，RT 用 `rt/`）
- `matVkEngineDemo` 只依赖 `renderer/` 和 `config/`，不写 Vulkan 细节

---

## 建议实现顺序（对应建目录）

```
1. common/matVkEngineTypes.h
   resource/VkEngineGpuMesh
   shader/VkEngineShaderModule + Descriptor*
   config/matVkEngineRenderConfig.h

2. pipeline/* + scene/* + renderer/Forward*
   → Demo: scenes/forward_default.json + shaders/forward/

3. rt/* + resource/VkEngineTexture
   → Demo: scenes/rt_default.json + shaders/rt/ + hdr/

4. platform/*（需要窗口 present 时）
5. scene/matVkEngineSceneLoader.h（JSON 解析，可最后用 nlohmann/json 或手写）
```

---

## Demo `main.cpp` 目标形态

```cpp
// 伪代码，仅说明调用关系
DemoApp app;
app.init(argc, argv);              // --backend=forward|rt --scene=scenes/xxx.json
app.loadScene("scenes/forward_default.json");
app.render();                      // 内部 RendererFactory → ForwardRenderer / RayTracingRenderer
app.saveOutput("output/forward.png");
```

---

## 与现有代码的迁移点

| 现有 | 迁移到 |
|------|--------|
| `main.cpp` 里手写 buffer upload | `VkEngineGpuMesh` |
| `vkEngine/src/rt/*` | `matVkEngine/rt/*`（改名 + 统一 mat 风格） |
| `VkEngineMesh` | 留在 `common/`，GPU 侧交给 `VkEngineGpuMesh` |
| `Vertex` struct | 抽到 `common/matVkEngineTypes.h` |

按这个结构建目录和 `.vcxproj` 条目即可；先做 **阶段 1 + Forward 一条链**，光追目录可以空着，等 Forward 跑通再填 `rt/`。