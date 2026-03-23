cmake_minimum_required(VERSION 3.16)
project(AutoDownloadSubtitle VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Windows 特定设置
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# 查找 Qt6 组件 (版本 6.11.0)
find_package(Qt6 6.11.0 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Gui
)

# 源文件列表
set(SOURCES
    src/main.cpp
    src/core/AssrtAPI.cpp
    src/core/SubtitleManager.cpp
    src/ui/MainWindow.cpp
)

# 头文件列表
set(HEADERS
    src/core/AssrtAPI.h
    src/core/SubtitleManager.h
    src/core/HistoryManager.h
    src/core/ConfigManager.h
    src/models/VideoFile.h
    src/models/SubtitleInfo.h
    src/models/AppConfig.h
    src/utils/NameParser.h
    src/utils/FileUtils.h
    src/utils/Logger.h
    src/ui/MainWindow.h
    src/ui/LogWidget.h
)

# UI 文件
set(UI_FILES
    src/ui/MainWindow.ui
)

# 资源文件
set(RESOURCE_FILES
    resources/app.qrc
)

# Windows 资源文件（用于设置图标和版本信息）
if(WIN32)
    set(WIN_RESOURCE_FILES resources/app.rc)
else()
    set(WIN_RESOURCE_FILES "")
endif()

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
    ${WIN_RESOURCE_FILES}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

# 链接 Qt6 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Gui
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX-)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 发布配置
option(QUICK_BUILD "快速构建模式：只编译，不部署 DLL" OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Windows 部署工具
    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Qt6*_qtbase]/bin)
        if(WINDEPLOYQT_EXECUTABLE AND NOT QUICK_BUILD)
            # 创建发布目录
            set(DEPLOY_DIR ${CMAKE_SOURCE_DIR}/output)
            
            # 安装主程序到输出目录
            install(TARGETS ${PROJECT_NAME}
                RUNTIME DESTINATION .
            )
            
            # 部署 Qt 依赖到输出目录
            # 注意：如果 output 目录中的程序正在运行，部署会跳过
            # 用户可以手动关闭程序后重新运行构建
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Deploying to output directory..."
                COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJECT_NAME}> ${DEPLOY_DIR}/ || echo "Warning: Could not copy executable. The program may be running."
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --release --dir ${DEPLOY_DIR} ${DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}> || echo "Warning: windeployqt failed."
            )
        else()
            # 快速构建模式：只复制到 build 目录
            if(WINDEPLOYQT_EXECUTABLE)
                message(STATUS "Quick Build Mode: Skipping deployment, executable will be in build/bin/")
            endif()
        endif()
    endif()
endif()

# 安装规则（用于 cmake --install）
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION resources FILES_MATCHING PATTERN "*")

# 打印配置信息
message(STATUS "===========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt Version: ${Qt6Core_VERSION}")
message(STATUS "===========================================")
