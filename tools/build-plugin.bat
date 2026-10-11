@echo off
REM ===================================================================
REM  Notepad-- Markdown Preview plugin - one-click Windows build
REM ===================================================================
REM  Usage:
REM      set QT_DIR=<Qt kit dir>
REM      set NDD_PLUGIN_DIR=<Notepad-- plugin dir>
REM      tools\build-plugin.bat
REM
REM  Environment variables (nothing is hard-coded in this script):
REM      QT_DIR          (required) Qt kit dir, e.g. <Qt>\5.15.2\msvc2019_64
REM      NDD_PLUGIN_DIR  (required unless NDD_ROOT is set) output dir
REM      QSCINT_LIB      import lib, default: <repo>\libs\qmyedit_qt5.lib
REM      NDD_ROOT        Notepad-- SOURCE root (optional, source mode)
REM      NDD_INSTALL_DIR Notepad-- install dir (used by helper scripts)
REM      TOOLSET         MSVC toolset, e.g. v141
REM
REM  NOTE: ASCII-only on purpose (cmd.exe reads .bat in OEM codepage).
REM ===================================================================
setlocal

set "PROJ=%~dp0.."
if "%QSCINT_LIB%"=="" set "QSCINT_LIB=%PROJ%\libs\qmyedit_qt5.lib"

REM ---------- locate the MSVC environment ----------
where cl >nul 2>&1
if not errorlevel 1 goto :msvc_ready

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto :no_msvc
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH goto :no_msvc
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 goto :no_msvc

:msvc_ready

REM ---------- validate inputs ----------
if "%QT_DIR%"=="" (
    echo [ERROR] QT_DIR is not set. Point it at your Qt kit directory, e.g.
    echo         set QT_DIR=^<Qt^>\5.15.2\msvc2019_64
    exit /b 1
)
if not exist "%QT_DIR%\lib\cmake\Qt5Core" (
    echo [ERROR] QT_DIR is not a valid Qt kit dir: "%QT_DIR%"
    exit /b 1
)
if "%NDD_PLUGIN_DIR%"=="" if not "%NDD_ROOT%"=="" set "NDD_PLUGIN_DIR=%NDD_ROOT%\x64\Release\plugin"
if "%NDD_PLUGIN_DIR%"=="" (
    echo [ERROR] NDD_PLUGIN_DIR is not set. Point it at the plugin folder of your
    echo         Notepad-- installation, e.g.
    echo         set NDD_PLUGIN_DIR=^<Notepad--^>\plugin
    exit /b 1
)

REM ---------- ensure the qmyedit_qt5 import library exists ----------
if not exist "%QSCINT_LIB%" if not exist "%NDD_ROOT%\qscint" (
    echo [INFO] import library missing, generating from the installed Notepad-- ...
    call "%~dp0gen-qmyedit-implib.bat"
    if errorlevel 1 ( echo [ERROR] failed to generate the import library & exit /b 1 )
)

set "TOOLSET_ARG="
if not "%TOOLSET%"=="" set "TOOLSET_ARG=-T %TOOLSET%"

echo ===================================================================
echo  Qt kit         : %QT_DIR%
echo  qmyedit lib    : %QSCINT_LIB%
echo  plugin output  : %NDD_PLUGIN_DIR%
echo  NDD_ROOT(src)  : %NDD_ROOT%
echo  toolset        : %TOOLSET_ARG%
echo ===================================================================

echo.
echo [1/4] CMake configure ...
cmake -S "%PROJ%" -B "%PROJ%\build" ^
      -G "Visual Studio 17 2022" -A x64 %TOOLSET_ARG% ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%/lib/cmake" ^
      -DQSCINT_LIB="%QSCINT_LIB%" ^
      -DNDD_PLUGIN_DIR="%NDD_PLUGIN_DIR%" ^
      -DNDD_ROOT="%NDD_ROOT%"
if errorlevel 1 ( echo [ERROR] CMake configure failed & exit /b 1 )

echo [2/4] build Release ...
cmake --build "%PROJ%\build" --config Release
if errorlevel 1 ( echo [ERROR] build failed & exit /b 1 )

echo [3/4] install ...
if exist "%NDD_PLUGIN_DIR%" (
    copy /y "%PROJ%\build\Release\ndd-markdown-preview.dll" "%NDD_PLUGIN_DIR%\" >nul
    if errorlevel 1 ( echo [ERROR] copy failed & exit /b 1 )
    echo         -^> %NDD_PLUGIN_DIR%\ndd-markdown-preview.dll
) else (
    echo [WARN] target dir not found: "%NDD_PLUGIN_DIR%"
)

echo [4/4] done.
echo.
echo ===================================================================
echo  Build OK
echo    %PROJ%\build\Release\ndd-markdown-preview.dll
echo.
echo  Copy the dll into the "plugin" folder of your Notepad-- and restart it.
echo  NOTE: the plugin needs the QtWebEngine runtime. If Notepad-- does not
echo        ship it, deploy it first with Qt's windeployqt, otherwise the
echo        preview panel stays blank.
echo ===================================================================
endlocal
exit /b 0

:no_msvc
echo [ERROR] MSVC environment not found.
echo         Run this script from a "Developer Command Prompt for VS", or
echo         install Visual Studio with the "Desktop development with C++"
echo         workload.
exit /b 1
