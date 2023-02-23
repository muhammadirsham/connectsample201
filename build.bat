@echo off
@setlocal

:: making sure the errorlevel is 0
if errorlevel 1 ( cd > nul )

:: loop through args looking for -c. this will clean and exit.
:loop
if "%1" == "-c" ( goto Clean )
if "%1" == "--clean" ( goto Clean )
if "%1" == "-x" ( goto Rebuild )
if "%1" == "--rebuild" ( goto Rebuild )
if "%1" == "--installer" ( goto Installer )
if "%1" == "--package" (goto Package )
shift
if "%1" neq "" ( goto loop )

goto Continue


:Clean
call "%~dp0tools\packman\python" "%~dp0tools\repoman\clean.py"
goto Finished


:Rebuild
call "%~dp0tools\packman\python" "%~dp0tools\repoman\clean.py"
if errorlevel 1 ( goto Finished )


:Continue
call "%~dp0prebuild.bat" %*
if errorlevel 1 ( goto Finished )

call "%~dp0tools\packman\python" "%~dp0tools\repoman\build.py" --platform-target windows-x86_64 %*
if errorlevel 1 ( goto Finished )

goto Finished

:Package
call "%~dp0tools\packman\python" "%~dp0tools\repoman\package.py"
if errorlevel 1 ( goto Finished )
echo ##teamcity[publishArtifacts '_unsignedpackages/*']
goto Finished

:Installer
call "%~dp0tools\packman\python" "%~dp0tools\repoman\installer.py" %*
if errorlevel 1 ( goto Finished )

:Finished
pause 
exit /b %errorlevel%
