@ECHO OFF
setlocal
pushd %~dp0

if "%PM_INSTALL_PATH%"=="" set PM_INSTALL_PATH=%~dp0..\packman\

call %PM_INSTALL_PATH%\packman pull deps.packman.xml -p windows-x86_64
if %errorlevel% neq 0 ( exit /b %errorlevel% )

set PYTHONPATH=%PM_jsonref_PATH%\python;%PM_repo_man_PATH%;%PM_repo_format_PATH%;%PM_MODULE_DIR%;%PYTHONPATH%
%PM_PYTHON% -s -S -u structuredlog.py %*
if %errorlevel% neq 0 ( exit /b %errorlevel% )

exit /b 0

