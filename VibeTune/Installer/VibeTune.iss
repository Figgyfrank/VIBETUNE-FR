; VibeTune installer (Inno Setup 6). Built automatically by the GitHub workflow.
#define MyAppName "VibeTune"
#define MyAppVersion "1.0.0"
#define BuildDir "..\build\VibeTune_artefacts\Release"

[Setup]
AppId={{6F1B7E52-3C9A-4D0B-9A55-7A2E0D8B41C3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=VibeTune
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=VibeTune-Setup
SetupIconFile=..\Assets\setup.ico
UninstallDisplayIcon={app}\vibetune.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut (standalone app)"; GroupDescription: "Shortcuts:"; Flags: checkedonce

[Files]
; VST3 plugin -> the folder Ableton scans
Source: "{#BuildDir}\VST3\VibeTune.vst3\*"; DestDir: "{commoncf64}\VST3\VibeTune.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; Standalone app + icon
Source: "{#BuildDir}\Standalone\VibeTune.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Assets\vibetune.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autodesktop}\VibeTune"; Filename: "{app}\VibeTune.exe"; IconFilename: "{app}\vibetune.ico"; Tasks: desktopicon
Name: "{autoprograms}\VibeTune"; Filename: "{app}\VibeTune.exe"; IconFilename: "{app}\vibetune.ico"
Name: "{autoprograms}\Uninstall VibeTune"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\VibeTune.exe"; Description: "Launch VibeTune standalone"; Flags: nowait postinstall skipifsilent unchecked

[Messages]
FinishedLabel=VibeTune is installed.%n%nIn Ableton Live: Preferences > Plug-ins > turn on "Use VST3 System Folders", press Rescan, then find VibeTune under Plug-ins > Audio Effects.
