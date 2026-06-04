#!/usr/bin/env bash
# 将本目录下的 RT 着色器编译为 SPIR-V，输出到 vkEngineDemo 可执行文件旁的 shaders 目录。
# 用法:
#   ./compile.sh
#   ./compile.sh /path/to/build/vkEngineDemo/shaders

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
OUTPUT_DIR="${1:-}"

find_glslc() {
  if command -v glslc >/dev/null 2>&1; then
    command -v glslc
    return
  fi
  if [[ -n "${VULKAN_SDK:-}" && -x "${VULKAN_SDK}/bin/glslc" ]]; then
    echo "${VULKAN_SDK}/bin/glslc"
    return
  fi
  echo "找不到 glslc。请安装 Vulkan SDK 并加入 PATH，或设置 VULKAN_SDK。" >&2
  exit 1
}

find_demo_shaders_dir() {
  local build_root="${REPO_ROOT}/build"
  local exe
  if [[ ! -d "${build_root}" ]]; then
    return 1
  fi
  exe="$(find "${build_root}" -name 'vkEngineDemo' -type f -perm -111 2>/dev/null | head -n 1 || true)"
  if [[ -z "${exe}" ]]; then
    return 1
  fi
  dirname "${exe}")/shaders
}

if [[ -z "${OUTPUT_DIR}" ]]; then
  OUTPUT_DIR="$(find_demo_shaders_dir || true)"
fi
if [[ -z "${OUTPUT_DIR}" ]]; then
  echo "未找到 vkEngineDemo 输出目录。请先构建 demo，或传入输出路径，例如:" >&2
  echo "  ./compile.sh ${REPO_ROOT}/build/vkEngineDemo/shaders" >&2
  exit 1
fi

GLSLC="$(find_glslc)"
mkdir -p "${OUTPUT_DIR}"

compile_one() {
  local stage="$1"
  local src="$2"
  local out="$3"
  echo "glslc ${src} -> ${OUTPUT_DIR}/${out}"
  "${GLSLC}" --target-spv=spv1.4 -I "${SCRIPT_DIR}" -fshader-stage="${stage}" \
    "${SCRIPT_DIR}/${src}" -o "${OUTPUT_DIR}/${out}"
}

compile_one rgen  raygen.rgen      raygen.spv
compile_one rmiss miss.rmiss       miss.spv
compile_one rchit closesthit.rchit closesthit.spv

echo "完成。SPIR-V 已写入: ${OUTPUT_DIR}"
