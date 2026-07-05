; Inno Setup script — Detail Forge (Windows VST3 installer)
; Compile:  "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" packaging\DetailForge.iss
;   optionally override version/source:
;   ISCC.exe /DMyAppVersion=0.1.0 /DSourceVst3="..\build\...\Detail Forge.vst3" packaging\DetailForge.iss
; Output: packaging\output\DetailForge-<version>-Windows-x64.exe

#define MyAppName "Detail Forge"
#define MyAppPublisher "Fayella"
#ifndef MyAppVersion
  #define MyAppVersion "0.0.1"
#endif
; VST3 on Windows is a bundle FOLDER; install the whole tree.
#ifndef SourceVst3
  #define SourceVst3 "..\build\DetailForge_artefacts\Release\VST3\Detail Forge.vst3"
#endif

[Setup]
AppId={{082A6F10-B12E-4841-A8D8-9870A7B3F0C9}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/fayella/detail-forge
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=output
OutputBaseFilename=DetailForge-{#MyAppVersion}-Windows-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName} (VST3)
; --- optional code signing (uncomment + configure a SignTool in the IDE or via /Ssigntool=...) ---
; SignTool=signtool
; SignedUninstaller=yes

[Types]
Name: "full"; Description: "Detail Forge VST3"

[Components]
Name: "vst3"; Description: "VST3 plug-in (installs to the shared VST3 folder)"; Types: full; Flags: fixed

[Files]
; Install the entire "Detail Forge.vst3" bundle into the standard shared VST3 folder.
Source: "{#SourceVst3}\*"; DestDir: "{commoncf64}\VST3\Detail Forge.vst3"; \
  Components: vst3; Flags: recursesubdirs createallsubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Detail Forge.vst3"

[Messages]
WelcomeLabel2=This will install [name/ver] to your shared VST3 folder:%n%n{commoncf64}\VST3\Detail Forge.vst3%n%nClose your DAW before continuing.
