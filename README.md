# VulkanLibrary

基于 Vulkan 的 C++ 图形引擎库及示例工程，支持 **Visual Studio 2022 (MSVC)** 与 **MinGW-w64 (GCC)** 两种编译配置。

## 工程结构

```
VulkanLibrary/
├── CMakeLists.txt          # 根 CMake 配置
├── CMakePresets.json       # 双工具链 Preset 定义
├── Dependence/             # 第三方依赖（glm、glfw、stb 等）
├── vkEngine/               # Vulkan 引擎静态库
├── vkEngineDemo/           # 示例可执行程序
└── .vscode/                # Cursor / VS Code 编辑器配置
```

## 环境要求

| 依赖 | 说明 |
|------|------|
| **CMake** | ≥ 3.16 |
| **Vulkan SDK** | 需设置环境变量 `VULKAN_SDK` |
| **Visual Studio 2022** | 含「使用 C++ 的桌面开发」工作负载（MSVC 配置） |
| **MinGW-w64** | 默认路径 `E:/mingw64`，`bin` 目录已加入用户 PATH（MinGW 配置） |

> MinGW 路径在 `CMakePresets.json` 中写死为 `E:/mingw64`。若安装位置不同，请修改 Preset 中的 `CMAKE_C_COMPILER`、`CMAKE_CXX_COMPILER`、`CMAKE_MAKE_PROGRAM`。

## 编译配置一览

| 配置 | 工具链 | 生成器 | 构建目录 |
|------|--------|--------|----------|
| **vs2022** | MSVC (Visual Studio 2022) | Visual Studio 17 2022 | `build-vs/` |
| **mingw-debug** | GCC 16.x (MinGW-w64) | MinGW Makefiles | `build-mingw/` |
| **mingw-release** | GCC 16.x (MinGW-w64) | MinGW Makefiles | `build-mingw/` |

---

## 命令行编译

在项目根目录执行以下命令。

### 查看可用 Preset

```bash
cmake --list-presets=configure
cmake --list-presets=build
```

### Visual Studio 2022 (MSVC)

**配置：**

```bash
cmake --preset vs2022
```

**编译 Debug：**

```bash
cmake --build --preset vs2022-debug
```

**编译 Release：**

```bash
cmake --build --preset vs2022-release
```

可执行文件输出路径（示例）：

```
build-vs/vkEngineDemo/Debug/vkEngineDemo.exe
build-vs/vkEngineDemo/Release/vkEngineDemo.exe
```

### MinGW-w64 (GCC)

**配置 Debug：**

```bash
cmake --preset mingw-debug
```

**配置 Release：**

```bash
cmake --preset mingw-release
```

> Debug 与 Release 共用 `build-mingw/` 目录，切换构建类型需重新执行对应的 configure preset。

**编译 Debug：**

```bash
cmake --build --preset mingw-debug
```

**编译 Release：**

```bash
cmake --build --preset mingw-release
```

可执行文件输出路径（示例）：

```
build-mingw/vkEngineDemo/vkEngineDemo.exe
```

### 编译指定目标

仅编译引擎库或示例程序：

```bash
# MSVC
cmake --build build-vs --config Debug --target vkEngine
cmake --build build-vs --config Debug --target vkEngineDemo

# MinGW
cmake --build build-mingw --target vkEngine
cmake --build build-mingw --target vkEngineDemo
```

### 清理构建

```bash
# 删除对应构建目录后重新配置
rmdir /s /q build-vs
rmdir /s /q build-mingw
```

---

## Cursor / VS Code 编译

1. 安装推荐扩展：**C/C++**、**CMake Tools**
2. 打开项目根目录 `VulkanLibrary`
3. 点击状态栏 **Configure Preset**，选择：
   - `Visual Studio 2022 x64` — MSVC
   - `MinGW GCC x64 (Debug)` / `(Release)` — MinGW
4. 执行 **CMake: Configure**，再 **CMake: Build**

### 代码跳转（IntelliSense）

`Ctrl+Shift+P` → **C/C++: Select a Configuration...**，选择与当前 Preset 匹配的配置：

| CMake Preset | C/C++ 配置 |
|--------------|------------|
| vs2022 | **MSVC** |
| mingw-debug / mingw-release | **MinGW** |

Configure 成功后会生成 `compile_commands.json`，供 clangd 实现符号跳转。

---

## 注意事项

- **Vulkan SDK**：两种配置均依赖 `VULKAN_SDK` 环境变量，请确保已安装并在终端 / 编辑器中可用。
- **MinGW 第三方库**：当前 `Dependence/lib` 中的 GLFW 以及 Vulkan SDK 的 volk 为 **MSVC 格式（`.lib`）**。MinGW 配置可用于代码分析与部分编译流程；若需 MinGW 完整链接通过，需另行提供 MinGW 兼容库或从源码编译依赖。
- **着色器编译**：`vkEngineDemo` 构建时会调用 `glslc` 编译 SPIR-V 着色器，需确保 Vulkan SDK 的 `Bin` 目录在 PATH 中。
