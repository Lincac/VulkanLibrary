@echo off

echo Creating build directory...
mkdir build
cd build

echo Running CMake...
cmake -G "Visual Studio 17 2022" -A x64 -T v141 -DCMAKE_INSTALL_PREFIX=build -DCMAKE_SYSTEM_VERSION=10.0.20348.0 -DCMAKE_CONFIGURATION_TYPES="Release;Debug" ..

echo Building project...
@REM msbuild ALL_BUILD.vcxproj /p:Configuration=Release
cmake --build . --config Release

echo Installing project...
@REM msbuild INSTALL.vcxproj /p:Configuration=Release

cmake --install . --config Release

pause