; SnowDesktop Installer Script
; Inno Setup 6 - Apple HIG Style Installer

#define MyAppName "Desktop Organizer"
#define MyAppNameShort "DesktopOrganizer"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Desktop Organizer"
#define MyAppExeName "SnowDesktop.exe"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppNameShort}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=..\dist
OutputBaseFilename=DesktopOrganizer-{#MyAppVersion}-Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
; Apple HIG style: minimal, clean installer
SetupIconFile=..\assets\icon\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
; Require admin for Program Files install
PrivilegesRequired=admin
; Min Windows 10
MinVersion=10.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "startupicon"; Description: "Start with Windows"; GroupDescription: "Startup:"

[Files]
; Main executable
Source: "..\.build\Release\SnowDesktop.exe"; DestDir: "{app}"; Flags: ignoreversion
; Taskbar hook DLL
Source: "..\.build\Release\SnowDesktopTaskbarHook.dll"; DestDir: "{app}"; Flags: ignoreversion
; Workshop manager
Source: "..\.build\Release\SnowDesktopWorkshopManager.exe"; DestDir: "{app}"; Flags: ignoreversion
; Widget engine
Source: "..\.build\Release\snowwidget.exe"; DestDir: "{app}"; Flags: ignoreversion
; Widget Lua files
Source: "..\widgets\*"; DestDir: "{app}\widgets"; Flags: ignoreversion recursesubdirs createallsubdirs
; Assets (fonts, icons)
Source: "..\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
; Language files
Source: "..\lang\*"; DestDir: "{app}\lang"; Flags: ignoreversion recursesubdirs createallsubdirs
; License
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Start with Windows (optional)
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "DesktopOrganizer"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: startupicon; Flags: uninsdeletevalue

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\widgets"
Type: filesandordirs; Name: "{app}\assets"
Type: filesandordirs; Name: "{app}\lang"

[Code]
// Apple HIG style: check if already running before install
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  // Check if SnowDesktop is running
  if FindWindowByClassName('SnowDesktopOverlay') <> 0 then
  begin
    if MsgBox('Desktop Organizer is currently running. It needs to be closed before installation.' + #13#10 + 'Close it now?', mbConfirmation, MB_YESNO) = IDYES then
    begin
      // Try graceful shutdown
      Exec('taskkill', '/IM SnowDesktop.exe /F', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
      Sleep(1000);
    end
    else
      Result := False;
  end;
end;
