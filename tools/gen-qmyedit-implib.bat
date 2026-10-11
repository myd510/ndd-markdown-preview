@echo off
REM ===================================================================
REM  Generate the qmyedit_qt5 import library (.lib) from an installed
REM  Notepad-- (its binary distribution ships qmyedit_qt5.dll but no .lib,
REM  so we build one from the DLL export table using dumpbin + lib).
REM
REM  Usage:
REM      set NDD_INSTALL_DIR=<Notepad-- install dir>
REM      tools\gen-qmyedit-implib.bat
REM
REM  Environment variables (nothing is hard-coded in this script):
REM      NDD_INSTALL_DIR  (required) dir containing qmyedit_qt5.dll
REM      OUT_DIR          output dir, default: <repo>\libs
REM
REM  NOTE: ASCII-only on purpose (cmd.exe reads .bat in OEM codepage).
REM ===================================================================
setlocal

set "PROJ=%~dp0.."
if "%OUT_DIR%"=="" set "OUT_DIR=%PROJ%\libs"

if "%NDD_INSTALL_DIR%"=="" (
    echo [ERROR] NDD_INSTALL_DIR is not set. Point it at your Notepad-- install
    echo         directory, e.g.
    echo         set NDD_INSTALL_DIR=^<Notepad--^>
    exit /b 1
)
if not exist "%NDD_INSTALL_DIR%\qmyedit_qt5.dll" (
    echo [ERROR] qmyedit_qt5.dll not found in "%NDD_INSTALL_DIR%"
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0gen-qmyedit-implib.ps1" -NddDir "%NDD_INSTALL_DIR%" -OutDir "%OUT_DIR%"
if errorlevel 1 exit /b 1
endlocal
