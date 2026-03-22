[Setup]
AppName=Leyline Virtual Audio Device
AppVersion=1.0
DefaultDirName={autopf}\LeylineAudio
DefaultGroupName=Leyline Audio
OutputDir=userdocs:InnoSetup
OutputBaseFilename=LeylineAudio_Setup
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "..\driver\bin\Release\leyline.sys"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\driver\bin\Release\leyline.inf"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\driver\bin\Release\leyline.cat"; DestDir: "{app}\driver"; Flags: ignoreversion
Source: "..\asio\Release\LeylineASIO.dll"; DestDir: "{app}\asio"; Flags: ignoreversion regserver 64bit

[Run]
Filename: "{sys}\pnputil.exe"; Parameters: "/add-driver ""{app}\driver\leyline.inf"" /install"; Flags: runhidden
