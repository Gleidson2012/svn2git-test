; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Icecast2 Win32
AppVerName=Icecast2 Alpha 2
AppPublisherURL=http://www.icecast.org
AppSupportURL=http://www.icecast.org
AppUpdatesURL=http://www.icecast.org
DefaultDirName={pf}\Icecast2 Win32
DefaultGroupName=Icecast2 Win32
AllowNoIcons=yes
LicenseFile=..\COPYING
InfoAfterFile=..\README
OutputDir=.
OutputBaseFilename=Icecast2_win32_2.0_alpha2_setup
WizardImageFile=icecast2logo2.bmp
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "Release\Icecast2.exe"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "..\doc\icecast2.chm"; DestDir: "{app}\doc"; CopyMode: alwaysoverwrite
Source: "..\web\status.xsl"; DestDir: "{app}\web"; CopyMode: alwaysoverwrite
Source: "..\web\status2.xsl"; DestDir: "{app}\web"; CopyMode: alwaysoverwrite
Source: "..\..\pthreads\pthreadVSE.dll"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "icecast.xml"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "..\..\iconv\lib\iconv.dll"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "..\..\libxslt\lib\libxslt.dll"; DestDir: "{app}"; CopyMode: alwaysoverwrite
Source: "..\..\libxml2\lib\libxml2.dll"; DestDir: "{app}"; CopyMode: alwaysoverwrite

[Icons]

Name: "{group}\Icecast2 Win32"; Filename: "{app}\Icecast2.exe"
Name: "{userdesktop}\Icecast2 Win32"; Filename: "{app}\Icecast2.exe"; MinVersion: 4,4; Tasks: desktopicon;WorkingDir: "{app}";

[Run]

