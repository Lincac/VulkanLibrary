# Compile RT shaders in this folder to SPIR-V next to vkEngineDemo.exe (shaders/).
# Usage:
#   .\compile.ps1
#   .\compile.ps1 -Configuration Release
#   .\compile.ps1 -OutputDir "F:\repo\VulkanLibrary\build\vkEngineDemo\Release\shaders"

param(
    [string]$OutputDir = "",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel", "")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

$ShaderDir = $PSScriptRoot
$RepoRoot = (Resolve-Path (Join-Path $ShaderDir "..\..")).Path

function Get-GlslcPath {
    $cmd = Get-Command glslc -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    if ($env:VULKAN_SDK) {
        $sdkGlslc = Join-Path $env:VULKAN_SDK "Bin\glslc.exe"
        if (Test-Path $sdkGlslc) {
            return $sdkGlslc
        }
    }
    throw "glslc not found. Install Vulkan SDK and add Bin to PATH, or set VULKAN_SDK."
}

function Find-DemoShadersOutputDir {
    param([string]$PreferredConfiguration)

    $buildRoot = Join-Path $RepoRoot "build-vs"
    if (-not (Test-Path $buildRoot)) {
        return $null
    }

    if (-not [string]::IsNullOrWhiteSpace($PreferredConfiguration)) {
        $preferredExe = Join-Path $buildRoot "vkEngineDemo\$PreferredConfiguration\vkEngineDemo.exe"
        if (Test-Path $preferredExe) {
            return (Join-Path (Split-Path $preferredExe -Parent) "shaders")
        }
    }

    foreach ($cfg in @("Release", "Debug", "RelWithDebInfo", "MinSizeRel")) {
        $exe = Join-Path $buildRoot "vkEngineDemo\$cfg\vkEngineDemo.exe"
        if (Test-Path $exe) {
            return (Join-Path (Split-Path $exe -Parent) "shaders")
        }
    }

    $exeHit = Get-ChildItem -Path $buildRoot -Filter "vkEngineDemo.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($exeHit) {
        return (Join-Path $exeHit.DirectoryName "shaders")
    }
    return $null
}

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Find-DemoShadersOutputDir -PreferredConfiguration $Configuration
}
if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $hint = Join-Path $RepoRoot "build-vs\vkEngineDemo\Release\shaders"
    throw "Demo output not found. Build vkEngineDemo first, or pass -OutputDir e.g.: $hint"
}

$glslc = Get-GlslcPath
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$shaders = @(
    @{ Stage = "rgen";  Source = "raygen.rgen";      Out = "raygen.spv" },
    @{ Stage = "rmiss"; Source = "miss.rmiss";       Out = "miss.spv" },
    @{ Stage = "rchit"; Source = "closesthit.rchit"; Out = "closesthit.spv" }
)

$commonArgs = @("--target-spv=spv1.4", "-I", $ShaderDir)

foreach ($item in $shaders) {
    $src = Join-Path $ShaderDir $item.Source
    $dst = Join-Path $OutputDir $item.Out
    if (-not (Test-Path $src)) {
        throw "Missing source file: $src"
    }
    Write-Host "glslc $($item.Source) -> $dst"
    & $glslc @commonArgs "-fshader-stage=$($item.Stage)" $src "-o" $dst
    if ($LASTEXITCODE -ne 0) {
        throw "Compile failed: $($item.Source) (exit $LASTEXITCODE)"
    }
}

Write-Host "Done. SPIR-V written to: $OutputDir"
