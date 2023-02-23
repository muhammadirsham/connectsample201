:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: This script searches for an installation of Microsoft Visual Studio and writes the results to ./deps/host-deps.package.xml
:: It will search for VS 2019.
:: If you wish to provide a substring of the installation's "display name" vswhere should find it
:: Example VS display names:
::  Visual Studio Build Tools 2019
::  Visual Studio Community 2019
::  Visual Studio Professional 2019
::  Visual Studio Professional 2017
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo off

set FIRST_VS_VERSION="2019"

echo All installed Visual Studio versions:
call "%~dp0tools\packman\python.bat" "%~dp0tools\repoman\findwindowsbuildtools.py" --list-all
echo.

call "%~dp0tools\packman\python.bat" "%~dp0tools\repoman\findwindowsbuildtools.py" --visual-studio-version %FIRST_VS_VERSION% --host-deps-path "%~dp0deps\host-deps.packman.xml"
if errorlevel 1 ( 
    echo.
    echo -----------------------------------------------------------------------------------
    echo - No Visual Studio %FIRST_VS_VERSION% installation detected.  Please view README.md for help. -
    echo -----------------------------------------------------------------------------------
    echo.
)

:End