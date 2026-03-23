@echo off
@chcp 65001 >nul
setlocal enabledelayedexpansion

echo ============================================
echo AutoDownloadSubtitle - 快速构建脚本
echo ============================================
echo.

cd /d "%~dp0"

REM 创建构建目录
if not exist "build" mkdir build
cd build

REM 配置 CMake（快速构建模式）
echo [1/2] 配置 CMake (快速构建模式)...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DQUICK_BUILD=ON ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/mingw_64" ^
    -DCMAKE_C_COMPILER="C:/Qt/Tools/mingw1310_64/bin/gcc.exe" ^
    -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe" ^
    -DCMAKE_RC_COMPILER="C:/Qt/Tools/mingw1310_64/bin/windres.exe" ^
    >nul 2>&1

if errorlevel 1 (
    echo [错误] CMake 配置失败！
    cd ..
    exit /b 1
)

REM 编译
echo [2/2] 编译中...
mingw32-make -j8

if errorlevel 1 (
    echo.
    echo [错误] 编译失败！
    cd ..
    exit /b 1
)

cd ..

REM 复制 exe 到 output 目录
echo [3/3] Copying executable to output directory...
if not exist "output" mkdir output
copy /Y "build\AutoDownloadSubtitle.exe" "output\AutoDownloadSubtitle.exe"

echo.
echo ============================================
echo 构建完成！
echo ============================================
echo.
echo 可执行文件位置：output\AutoDownloadSubtitle.exe
echo.
echo 注意：此版本只复制了 exe 文件，没有复制 DLL
echo       需要在有 Qt 环境的情况下运行
echo       如需完整部署版本（包含 DLL），请运行 build.bat
echo.

pause