@chcp 65001 >nul
:: 清理旧的构建缓存
if exist build (
    rmdir /s /q build
)

:: 清理旧的输出目录
if exist output (
    rmdir /s /q output
)

:: 创建构建目录
mkdir build
cd build

:: 配置 CMake - 使用 MinGW Makefiles 和 Qt6
:: 请根据实际情况修改 Qt 安装路径
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/Qt6.11.0/mingw_64" -DCMAKE_BUILD_TYPE=Release

:: 编译 Release 版本
cmake --build . --config Release

:: 显示输出目录信息
echo.
echo ==========================================
echo 编译完成！
echo 可执行文件位置：
echo   - 构建目录：build\bin\AutoDownloadSubtitle.exe
echo   - 发布目录：output\AutoDownloadSubtitle.exe
echo ==========================================
echo.

pause