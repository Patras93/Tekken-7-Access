@echo off
setlocal
cd /d "%~dp0"

where cl >nul 2>nul
if errorlevel 1 (
  echo Nie znaleziono cl.exe.
  echo Uruchom ten plik z "x64 Native Tools Command Prompt for VS".
  exit /b 1
)

cl /nologo /std:c++17 /EHsc /O2 /W4 T7OnlinePracticeProbe.cpp /link bcrypt.lib user32.lib /out:T7OnlinePracticeProbe.exe
if errorlevel 1 (
  echo.
  echo BUILD FAILED
  exit /b 1
)

echo.
echo BUILD OK: T7OnlinePracticeProbe.exe
exit /b 0
