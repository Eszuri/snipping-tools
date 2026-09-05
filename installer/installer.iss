#define MyAppName "Snipping Tools"
#define MyAppVersion "1.1"
#define MyAppPublisher "Snipping Tools"
#define MyAppExeName "NativeSnippingTool.exe"

[Setup]
AppId={{D3F4A5E6-7B8C-9D0E-1F2A-3B4C5D6E7F80}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppMutex=NativeSnippingTool_App_Mutex_v1
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
UsePreviousAppDir=yes
UsePreviousGroup=yes
UsePreviousTasks=yes
CloseApplications=yes
RestartApplications=no
CloseApplicationsFilter=*.exe,NativeSnippingTool.exe
OutputDir=..\dist
OutputBaseFilename=SnippingTools-Setup-v{#MyAppVersion}
SetupIconFile=..\resources\app.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion restartreplace
Source: "..\resources\app.ico"; DestDir: "{app}"; Flags: ignoreversion restartreplace
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion restartreplace isreadme
Source: "..\assets\icons\*"; DestDir: "{app}\assets\icons"; Flags: ignoreversion restartreplace

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\app.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Code]
// Gracefully terminate existing running instance of the app before updating
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  Exec('taskkill.exe', '/F /IM NativeSnippingTool.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(200);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  Exec('taskkill.exe', '/F /IM NativeSnippingTool.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(200);
end;
