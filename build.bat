@echo off
REM =============================================================================
REM BUILD.BAT - One-Click Build Script
REM =============================================================================
REM Just double-click this file to compile ALL your C++ into a DLL!
REM =============================================================================

echo.
echo ========================================
echo   CPPBridge Auto-Builder
echo ========================================
echo.

REM Find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "tokens=*" %%i in ('"%VSWHERE%" -latest -property installationPath') do set VSDIR=%%i
)

if not defined VSDIR (
    echo ERROR: Visual Studio not found!
    echo Please install Visual Studio with C++ tools.
    pause
    exit /b 1
)

REM Setup compiler environment
call "%VSDIR%\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul 2>&1

REM Find all .cpp files in src folder
echo Finding C++ files...
set SOURCES=
for /R "%~dp0src" %%f in (*.cpp) do (
    echo   Found: %%~nxf
    set SOURCES=!SOURCES! "%%f"
)

REM Also check root for any .cpp files
for %%f in ("%~dp0*.cpp") do (
    echo   Found: %%~nxf
    set SOURCES=!SOURCES! "%%f"
)

REM Enable delayed expansion for variable
setlocal enabledelayedexpansion

REM Compile
echo.
echo Compiling...
set ALL_SOURCES=
for /R "%~dp0src" %%f in (*.cpp) do set ALL_SOURCES=!ALL_SOURCES! "%%f"
for %%f in ("%~dp0*.cpp") do set ALL_SOURCES=!ALL_SOURCES! "%%f"

if "!ALL_SOURCES!"=="" (
    echo No .cpp files found! Create some in the 'src' folder.
    pause
    exit /b 1
)

cl /LD /EHsc /O2 /I"%~dp0" !ALL_SOURCES! /Fe:"%~dp0backend.dll" /link /DLL

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo   SUCCESS! Created: backend.dll
    echo ========================================
    echo.
    echo Your C++ is now ready for JavaScript!
    
    REM Cleanup temp files
    del *.obj *.exp *.lib 2>nul
) else (
    echo.
    echo ========================================
    echo   BUILD FAILED - Check errors above
    echo ========================================
)

echo.
pause
