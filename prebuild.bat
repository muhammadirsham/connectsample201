@echo off

:Build
call "%~dp0tools\packman\python.bat" "%~dp0tools\repoman\build.py" --generateprojects-only --platform-target windows-x86_64 %*
if errorlevel 1 ( goto Error )

call "%~dp0tools\packman\python" "%~dp0tools\repoman\filecopy.py" --platform-target windows-x86_64 "%~dp0tools\buildscripts\pre_build_copies.json"
if errorlevel 1 ( goto Error )

:Success
exit /b 0

:Error
exit /b %errorlevel%
