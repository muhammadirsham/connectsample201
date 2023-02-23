

if "%VSCMD_TEST%" NEQ "" goto :test
if "%VSCMD_ARG_CLEAN_ENV%" NEQ "" goto :clean_env

@REM ------------------------------------------------------------------------
:start

@REM Set VSSDK150INSTALL environment variable to $VSInstallRoot\VSSDK.
@REM This is still being used in legacy scenarios; don't delete
@REM This will be dissappearing in future version of Visual Studio.
set "VSSDK150INSTALL=%VSINSTALLDIR%VSSDK"

@REM Set VSSDKINSTALL environment variable to $VSInstallRoot\VSSDK.
set "VSSDKINSTALL=%VSINSTALLDIR%VSSDK"

goto :end

:test

set __VSCMD_TEST_FailCount=0

setlocal

@REM Check for vsct.exe is installed under the VSSDK directory.
if NOT EXIST "%VSINSTALLDIR%VSSDK\VisualStudioIntegration\Tools\Bin\vsct.exe" (
    @echo [ERROR:%~nx0] 'VSSDK\VisualStudioIntegration\Tools\Bin\vsct.exe' does not exist
    set /A __VSCMD_TEST_FailCount=__VSCMD_TEST_FailCount+1
)

endlocal & set __VSCMD_TEST_FailCount=%__VSCMD_TEST_FailCount%

@REM return value other than 0 if tests failed.
if "%__VSCMD_TEST_FailCount%" NEQ "0" (
   set __VSCMD_Test_FailCount=
   exit /B 1
)

set __VSCMD_Test_FailCount=
exit /B 0

:clean_env

set VSSDK150INSTALL=
set VSSDKINSTALL=

goto :end
:end

exit /B 0