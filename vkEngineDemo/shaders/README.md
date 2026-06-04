# vkEngineDemo 着色器

路径追踪 RT 着色器源文件；运行 demo 时需加载同目录下的 `.spv`（相对可执行文件路径 `shaders/*.spv`）。

## 编译到 demo 输出目录

构建 demo 后，在**本目录**执行脚本，会将 `raygen.rgen`、`miss.rmiss`、`closesthit.rchit` 编译到可执行文件旁的 `shaders/`（自动在 `build/` 下查找 `vkEngineDemo` / `vkEngineDemo.exe`）：

**Windows（PowerShell 或双击 bat）：**

```powershell
cd F:\repo\VulkanLibrary\vkEngineDemo\shaders
.\compile.ps1
# 或
.\compile.bat
```

默认编译到 **Release** 配置目录（`build\vkEngineDemo\Release\shaders`）。若只有 Debug 构建，脚本会自动回退到 Debug。

```powershell
.\compile.ps1 -Configuration Debug
.\compile.ps1 -OutputDir "F:\repo\VulkanLibrary\build\vkEngineDemo\Release\shaders"
```

注意：仅修改脚本里 `throw` 提示中的路径**不会**改变实际输出目录；须使用 `-Configuration`、`-OutputDir`，或改 `Find-DemoShadersOutputDir` 的逻辑。

**Linux / macOS：**

```bash
cd vkEngineDemo/shaders
chmod +x compile.sh
./compile.sh
# 或
./compile.sh /path/to/build/vkEngineDemo/shaders
```

需要已安装 [Vulkan SDK](https://vulkan.lunarg.com/) 中的 `glslc`（或在 `PATH` / `VULKAN_SDK` 中可用）。

## 手动编译（单文件）

```text
cd F:\repo\VulkanLibrary\vkEngineDemo\shaders

glslc --target-spv=spv1.4 -I . -fshader-stage=rgen  raygen.rgen      -o raygen.spv
glslc --target-spv=spv1.4 -I . -fshader-stage=rmiss miss.rmiss       -o miss.spv
glslc --target-spv=spv1.4 -I . -fshader-stage=rchit closesthit.rchit -o closesthit.spv
```

将生成的 `.spv` 放到 demo 可执行文件同级的 `shaders/` 目录即可。

## CMake

`vkEngineDemo` 在 **POST_BUILD** 时也会调用 `glslc` 并复制到输出目录，与上述脚本参数一致。改着色器后既可重新构建 demo，也可只运行本目录脚本加快迭代。
