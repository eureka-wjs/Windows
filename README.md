# AutoDownloadSubtitle - 射手网字幕自动下载工具 (Windows 桌面版)

一个基于射手网（伪）API 的字幕自动下载 Windows 桌面应用程序，采用 Qt/C++ 开发，提供友好的图形界面和完整的字幕下载功能。

## 功能特性

### 核心功能

- 🔍 **多策略字幕搜索** - 5 层搜索策略，提高匹配成功率
  - 策略 1: 完整文件名 + is_file=1（利用 API 的文件名解析和哈希匹配）
  - 策略 2: 优化提取的名称 + is_file=0（移除干扰标签后的标准名称）
  - 策略 3: 剧集标准名称（仅保留剧名 + SXXEXX 季集信息）
  - 策略 4: 搜索变体（移除 Dub、破折号格式、点号格式等）
  - 策略 5: 简化名称（移除括号内容）

- ⚡ **智能重试机制** - 优化重试策略，区分 API 配额和下载配额
  - API 配额不足：等待 60 秒重试
  - 下载配额不足：等待 120 秒重试
  - 服务器错误：自动重试

- 💾 **断点续传** - 记录已处理文件，避免重复下载
  - 支持文件名匹配（快速模式）
  - 支持文件哈希匹配（精确模式）

- 📊 **批量处理** - 支持递归扫描文件夹，批量下载字幕
  - 可配置文件间延迟
  - 实时进度显示
  - 统计报告生成

### 界面功能

- 📁 **拖放操作** - 支持拖动文件夹到界面指定工作目录
- 🔑 **Token 管理** - API Token 一次输入，自动保存
- 📈 **进度显示** - 实时进度条和统计信息
- 📝 **日志输出** - 分级日志显示（INFO/SUCCESS/WARNING/ERROR/DEBUG）

## 系统要求

- **操作系统**: Windows 10/11
- **开发环境**: 
  - Qt 5.15+
  - CMake 3.16+
  - C++17 兼容编译器 (MSVC 2019+)
- **打包工具**: Inno Setup 6+

## 编译说明

### 1. 安装依赖

1. 安装 Qt (推荐 5.15 或 6.x)
   - 下载地址：https://www.qt.io/download
   - 选择 MinGW 或 MSVC 版本

2. 安装 CMake
   - 下载地址：https://cmake.org/download/

3. 安装 Inno Setup (用于打包)
   - 下载地址：https://jrsoftware.org/isdl.php

### 2. 配置环境变量

```cmd
:: 添加 Qt 到 PATH (根据实际安装路径修改)
set PATH=C:\Qt\5.15.2\msvc2019_64\bin;%PATH%

:: 添加 CMake 到 PATH
set PATH=C:\Program Files\CMake\bin;%PATH%
```

### 3. 编译项目

```cmd
:: 进入项目目录
cd Windows

:: 创建构建目录
mkdir build
cd build

:: 配置 CMake (64 位)
cmake .. -G "Visual Studio 16 2019" -A x64

:: 或者使用 MinGW
:: cmake .. -G "MinGW Makefiles"

:: 编译 Release 版本
cmake --build . --config Release
```

### 4. 部署 Qt 运行时

编译完成后，使用 windeployqt 工具部署 Qt 依赖：

```cmd
:: 进入 release 目录
cd build/release

:: 部署 Qt 库
windeployqt AutoDownloadSubtitle.exe --release
```

### 5. 创建安装包

```cmd
:: 进入 installer 目录
cd Windows/installer

:: 使用 Inno Setup 编译脚本
iscc setup.iss
```

安装包将生成在 `installer/Output/AutoDownloadSubtitle_Setup.exe`

## 使用方法

### 首次使用

1. 获取 API Token
   - 访问 https://assrt.net/user/register.xml 注册账号
   - 登录 https://assrt.net/usercp.php
   - 查看 API Token（32 位随机字符）

2. 运行程序，在界面中输入 API Token 并保存

3. 拖放包含视频文件的文件夹到工作区域，或点击"浏览"按钮选择目录

4. 点击"开始下载"按钮

### 界面说明

| 区域 | 功能 |
|------|------|
| API Token | 输入并保存射手网 API Token |
| 工作目录 | 显示当前工作目录，支持拖放 |
| 拖放区域 | 拖动文件夹到此处设置工作目录 |
| 进度条 | 显示当前处理进度 |
| 统计信息 | 显示总文件数、成功/失败/跳过数量 |
| 日志区域 | 显示处理日志，支持分级颜色 |

### 日志级别

| 级别 | 图标 | 说明 |
|------|------|------|
| INFO | ℹ️ | 一般信息 |
| SUCCESS | ✅ | 成功信息 |
| WARNING | ⚠️ | 警告信息 |
| ERROR | ❌ | 错误信息 |
| DEBUG | 🔍 | 调试信息 |

## 项目结构

```
Windows/
├── CMakeLists.txt              # CMake 配置
├── README.md                   # 本文件
├── plans/
│   └── architecture.md         # 架构设计文档
├── src/
│   ├── main.cpp                # 程序入口
│   ├── core/
│   │   ├── AssrtAPI.h/.cpp     # API 客户端
│   │   ├── SubtitleManager.h/.cpp # 字幕管理器
│   │   ├── HistoryManager.h    # 历史记录管理
│   │   └── ConfigManager.h     # 配置管理
│   ├── models/
│   │   ├── VideoFile.h         # 视频文件模型
│   │   ├── SubtitleInfo.h      # 字幕信息模型
│   │   └── AppConfig.h         # 应用配置模型
│   ├── utils/
│   │   ├── NameParser.h        # 名称解析工具
│   │   ├── FileUtils.h         # 文件工具
│   │   └── Logger.h            # 日志工具
│   └── ui/
│       ├── MainWindow.h/.cpp   # 主窗口
│       ├── MainWindow.ui       # UI 布局
│       └── LogWidget.h         # 日志控件
├── resources/
│   ├── app.qrc                 # Qt 资源文件
│   └── icons/                  # 图标目录
└── installer/
    └── setup.iss               # Inno Setup 脚本
```

## 配置文件

### 配置存储位置

- Windows: `HKEY_CURRENT_USER\Software\AutoDownloadSubtitle\AutoDownloadSubtitle`

### 配置项

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| ApiKey | - | 射手网 API Token |
| WorkingDirectory | - | 工作目录 |
| UseHashMatching | false | 是否使用文件哈希匹配 |
| Languages | ["chi", "zh", "zh-tw"] | 目标语言 |
| VideoExtensions | [".mp4", ".mkv", ...] | 视频扩展名 |
| MinFileSize | 10MB | 最小文件大小 |
| MaxRetries | 2 | 最大重试次数 |
| Timeout | 30s | 请求超时 |

## 日志文件

| 文件 | 位置 | 说明 |
|------|------|------|
| subtitle_download.log | %APPDATA%/AutoDownloadSubtitle/ | 下载日志 |
| subtitle_processed.json | %APPDATA%/AutoDownloadSubtitle/ | 已处理记录 |
| subtitle_failed.json | %APPDATA%/AutoDownloadSubtitle/ | 失败记录 |

## 常见问题

### Token 无效

请确保 Token 是 32 位有效字符，可以从 [射手网 API 文档](https://secure.assrt.net/api/doc) 获取。

### 配额用尽

当出现"配额已用尽"提示时，程序会自动等待后重试。这是 API 的限制，无法绕过。

### 匹配失败

如果字幕匹配失败，可以尝试：
1. 修改视频文件名为标准格式
2. 检查是否已下载过该字幕
3. 查看日志了解详细原因

## 相关资源

- [射手网 API 文档](https://secure.assrt.net/api/doc)
- [API 端点](https://api.assrt.net/v1/)
- [Qt 文档](https://doc.qt.io/)
- [CMake 文档](https://cmake.org/documentation/)

## 许可证

本项目仅供学习交流使用。

## 版本历史

### v1.0.0
- 初始版本
- 完整实现 Python 版本的所有功能
- 提供 Windows 桌面界面
- 支持安装包打包
