; Inno Setup 安装脚本
; AutoDownloadSubtitle 安装包配置

#define MyAppName "AutoDownloadSubtitle"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "AutoDownloadSubtitle"
#define MyAppExeName "AutoDownloadSubtitle.exe"

[Setup]
; 基本设置
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=AutoDownloadSubtitle_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
SetupIconFile=compiler:DefaultWizardSetupIcon.ico

; 权限设置
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; 语言设置
LanguageDetectionMethod=locale
ShowLanguageDialog=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Default.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; 主程序
Source: "output\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "output\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "output\*.pdb"; DestDir: "{app}"; Flags: ignoreversion

; Qt 平台插件
Source: "output\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs

; Qt 样式插件
Source: "output\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs

; Qt 图片插件
Source: "output\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs

; Qt 翻译文件
Source: "output\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion

; Qt 图标引擎插件
Source: "output\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs

; Qt 网络信息插件
Source: "output\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs

; Qt TLS 插件
Source: "output\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs

; Qt 通用触摸插件
Source: "output\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs

; 图标文件（如果存在）
Source: "resources\icons\icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
Type: filesandordirs; Name: "{userappdata}\{#MyAppName}"

[Code]
// 安装前检查
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  // 可以在这里添加自定义检查逻辑
end;

// 安装完成后提示
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // 可以在这里添加安装后操作
  end;
end;
