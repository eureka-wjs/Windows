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

# 查找 Qt 组件
find_package(Qt5 REQUIRED COMPONENTS 
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
    src/core/HistoryManager.cpp
    src/core/ConfigManager.cpp
    src/models/VideoFile.cpp
    src/models/SubtitleInfo.cpp
    src/models/AppConfig.cpp
    src/utils/NameParser.cpp
    src/utils/FileUtils.cpp
    src/utils/Logger.cpp
    src/ui/MainWindow.cpp
    src/ui/LogWidget.cpp
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

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Gui
)

# 编译选项
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX-)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# 发布配置
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Windows 部署工具
    if(WIN32)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Qt5*_qtbase]/bin)
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --release $<TARGET_FILE:${PROJECT_NAME}>
                COMMENT "Deploying Qt libraries..."
            )
        endif()
    endif()
endif()

# 安装规则
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "===========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt Version: ${Qt5Core_VERSION}")
message(STATUS "===========================================")
