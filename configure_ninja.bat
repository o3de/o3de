@echo off
echo === Setting up Visual Studio environment ===

REM Path to your Visual Studio 2022 installation (adjust if needed)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

REM Ninja ships with the "C++ CMake tools for Windows" VS component, and vcvars64
REM puts it on PATH. If this reports nothing, install that component from the VS
REM Installer - Individual Components - C++ CMake tools for Windows.
echo.
echo === Checking for ninja ===
where ninja
if errorlevel 1 (
    echo.
    echo ERROR: ninja is not on PATH. Install "C++ CMake tools for Windows" in the VS Installer.
    pause
    exit /b 1
)

echo.
echo === Configuring the ENGINE for Ninja ===

REM -S . is the engine root, not a project. Everything else matches the project script.
REM
REM LY_PROJECTS is the one addition: it makes the engine build compile your project's
REM enabled gems too, which is what you want if you intend to launch this Editor on
REM test_001. Drop it to build a bare engine with no gems.
cmake -G "Ninja Multi-Config" -S . -B build/windows -DLY_3RDPARTY_PATH=C:/Users/medin/.o3de/3rdParty 

if errorlevel 1 (
    echo.
    echo Configure FAILED. See the error above.
    pause
    exit /b 1
)

echo.
echo === Configure complete ===
echo Build files are in build\windows
pause
