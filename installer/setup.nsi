; OBS Scene Switcher Installer
; Copyright (C) 2025 ksmksks
; SPDX-License-Identifier: GPL-2.0-or-later

!define PRODUCT_NAME "OBS Scene Switcher"
!define PRODUCT_PUBLISHER "ksmksks"
!define PRODUCT_WEB_SITE "https://twitch.tv/ksmksks"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Version is passed from CMake via /DPRODUCT_VERSION=x.x.x
!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.8.0"
!endif

; Modern UI
!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"
!include "LogicLib.nsh"

; Name and file
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "obs-scene-switcher-${PRODUCT_VERSION}-installer.exe"

; Installation directory for uninstaller management only
InstallDir "$PROGRAMFILES64\obs-scene-switcher"

; Request administrator privileges
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Custom page to detect OBS installation
!define MUI_PAGE_CUSTOMFUNCTION_PRE DetectOBSInstallation

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Language
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Japanese"

; Variables
Var OBS_INSTALL_DIR

; Function to detect OBS Studio installation
Function DetectOBSInstallation
  ; Try multiple detection methods
  
  ; Method 1: Check registry (64-bit)
  ${If} ${RunningX64}
    SetRegView 64
    ReadRegStr $0 HKLM "Software\OBS Studio" ""
    ${If} $0 != ""
      ${If} ${FileExists} "$0\bin\64bit\obs64.exe"
        StrCpy $OBS_INSTALL_DIR "$0"
        DetailPrint "Found OBS Studio (64-bit registry): $OBS_INSTALL_DIR"
        Return
      ${EndIf}
    ${EndIf}
  ${EndIf}
  
  ; Method 2: Check common installation paths
  ${If} ${FileExists} "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe"
    StrCpy $OBS_INSTALL_DIR "$PROGRAMFILES\obs-studio"
    DetailPrint "Found OBS Studio (Program Files): $OBS_INSTALL_DIR"
    Return
  ${EndIf}
  
  ${If} ${FileExists} "$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe"
    StrCpy $OBS_INSTALL_DIR "$PROGRAMFILES64\obs-studio"
    DetailPrint "Found OBS Studio (Program Files x64): $OBS_INSTALL_DIR"
    Return
  ${EndIf}
  
  ; Method 3: Check user's local AppData
  ${If} ${FileExists} "$LOCALAPPDATA\obs-studio\bin\64bit\obs64.exe"
    StrCpy $OBS_INSTALL_DIR "$LOCALAPPDATA\obs-studio"
    DetailPrint "Found OBS Studio (Local AppData): $OBS_INSTALL_DIR"
    Return
  ${EndIf}
  
  ; Method 4: Search in registry uninstall entries
  StrCpy $1 0
  loop:
    EnumRegKey $2 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall" $1
    ${If} $2 == ""
      Goto done
    ${EndIf}
    
    ReadRegStr $3 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$2" "DisplayName"
    ${If} $3 == "OBS Studio"
      ReadRegStr $4 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\$2" "InstallLocation"
      ${If} $4 != ""
        ${If} ${FileExists} "$4\bin\64bit\obs64.exe"
          StrCpy $OBS_INSTALL_DIR "$4"
          DetailPrint "Found OBS Studio (Uninstall registry): $OBS_INSTALL_DIR"
          Return
        ${EndIf}
      ${EndIf}
    ${EndIf}
    
    IntOp $1 $1 + 1
    Goto loop
  done:
  
  ; If not found, show warning
  MessageBox MB_ICONEXCLAMATION|MB_OK "OBS Studio installation not found.$\n$\nPlease ensure OBS Studio is installed before continuing."
  Abort
FunctionEnd

; Installer Sections
Section "Install" SecInstall
  ; Verify OBS Studio exists
  ${IfNot} ${FileExists} "$OBS_INSTALL_DIR\bin\64bit\obs64.exe"
    MessageBox MB_ICONSTOP "OBS Studio not found in:$\n$OBS_INSTALL_DIR$\n$\nPlease install OBS Studio first."
    Abort
  ${EndIf}
  
  ; Install plugin files to OBS Studio directory
  SetOutPath "$OBS_INSTALL_DIR\obs-plugins\64bit"
  File "files\obs-studio\obs-plugins\64bit\obs-scene-switcher.dll"
  
  SetOutPath "$OBS_INSTALL_DIR\data\obs-plugins\obs-scene-switcher"
  File /r "files\obs-studio\data\obs-plugins\obs-scene-switcher\*.*"
  
  ; Create management directory for uninstaller
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
  ; Save OBS installation path for uninstaller
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "OBSInstallDir" "$OBS_INSTALL_DIR"
  
  ; Write registry keys for Add/Remove Programs
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" 1
  
  ; Calculate installed size
  !insertmacro GetSize
  ${GetSize} "$OBS_INSTALL_DIR\obs-plugins\64bit" "/S=0K" $0 $1 $2
  ${GetSize} "$OBS_INSTALL_DIR\data\obs-plugins\obs-scene-switcher" "/S=0K" $3 $4 $5
  IntOp $0 $0 + $3
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
  
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}
  
  ; Success message
  MessageBox MB_ICONINFORMATION "Installation completed successfully!$\n$\nOBS Scene Switcher has been installed to:$\n$OBS_INSTALL_DIR"
SectionEnd

; Uninstaller Section
Section "Uninstall"
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  
  ; Read OBS installation path
  ReadRegStr $OBS_INSTALL_DIR ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "OBSInstallDir"
  
  ${If} $OBS_INSTALL_DIR == ""
    ; Fallback to default path if registry key not found
    StrCpy $OBS_INSTALL_DIR "$PROGRAMFILES\obs-studio"
  ${EndIf}
  
  ; Remove plugin files from OBS Studio directory
  Delete "$OBS_INSTALL_DIR\obs-plugins\64bit\obs-scene-switcher.dll"
  RMDir /r "$OBS_INSTALL_DIR\data\obs-plugins\obs-scene-switcher"
  
  ; Remove uninstaller and management directory
  Delete "$INSTDIR\uninstall.exe"
  RMDir "$INSTDIR"
  
  ; Remove registry keys (both 64-bit and 32-bit views)
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  
  ; Also check WOW6432Node for 32-bit registry
  SetRegView 32
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
  
  ${If} ${RunningX64}
    SetRegView lastused
  ${EndIf}
  
  ; Success message
  MessageBox MB_ICONINFORMATION "OBS Scene Switcher has been successfully uninstalled."
SectionEnd
