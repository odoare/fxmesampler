; FxmeSampler VST3 Installer for Windows
; Usage (one call per kit):
;   makensis -DAPPNAME="Black Widow Drums" -DSLUGNAME=BlackWidowDrums "-DPLUGFILE=Black Widow Drums.vst3" -DVERSION=1.0.0 installer.nsi
;   makensis -DAPPNAME=CenturyDrums       -DSLUGNAME=CenturyDrums    -DPLUGFILE=CenturyDrums.vst3        -DVERSION=1.0.0 installer.nsi
; ${PLUGFILE} must be present in the same directory as this script.

!define PUBLISHER  "Fx-Mechanics"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SLUGNAME}"

Name            "${APPNAME} ${VERSION}"
OutFile         "${SLUGNAME}-Windows-Setup.exe"
InstallDir      "$PROGRAMFILES64\${PUBLISHER}\${APPNAME}"
RequestExecutionLevel admin

Page instfiles

Section "VST3 Plugin"
    SetOutPath "$COMMONFILES64\VST3"
    File /r "${PLUGFILE}"

    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"     "${APPNAME}"
    WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"       "${PUBLISHER}"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"  "${VERSION}"
    WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"        1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"        1
SectionEnd

Section "Uninstall"
    RMDir /r "$COMMONFILES64\VST3\${PLUGFILE}"
    Delete   "$INSTDIR\Uninstall.exe"
    RMDir    "$INSTDIR"
    RMDir    "$PROGRAMFILES64\${PUBLISHER}"
    DeleteRegKey HKLM "${UNINST_KEY}"
SectionEnd
