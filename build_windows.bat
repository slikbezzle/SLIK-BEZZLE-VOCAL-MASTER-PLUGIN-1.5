@echo off
REM Builds SLIK BEZZLE VOCAL MASTER VST3 on Windows.
REM Needs: Visual Studio (2022 or 2026, "Desktop development with C++"), CMake, Git.
REM Optional: build_windows.bat C:\path\to\JUCE   (otherwise JUCE is downloaded)
setlocal
if not "%~1"=="" (set JUCEARG=-DJUCE_DIR="%~1") else (set JUCEARG=)
if exist build rmdir /s /q build
REM No -G given: CMake picks the newest Visual Studio it finds.
cmake -B build -A x64 %JUCEARG% || goto :fail
cmake --build build --config Release --target SlikBezzleVocalMaster_VST3 || goto :fail
echo.
echo Done. Your plugin is at:
echo   build\SlikBezzleVocalMaster_artefacts\Release\VST3\SLIK BEZZLE VOCAL MASTER.vst3
echo Copy that folder to C:\Program Files\Common Files\VST3\
pause
exit /b 0
:fail
echo Build failed. Copy the error text above and send it to me.
pause
exit /b 1
