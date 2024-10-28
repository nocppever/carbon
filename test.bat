@echo off
echo Testing file sync server...

:: Test config file creation
if exist config.ini del config.ini
echo Testing config creation...
server.exe -test_config
if errorlevel 1 goto error

:: Test SSL certificate generation
if exist server.crt del server.crt
if exist server.key del server.key
echo Testing SSL certificate generation...
server.exe -generate_cert
if errorlevel 1 goto error

:: Test server startup
echo Testing server startup...
start /b server.exe
timeout /t 2 >nul
tasklist | find "server.exe" >nul
if errorlevel 1 goto error

:: Cleanup
taskkill /f /im server.exe >nul 2>&1

echo All tests passed!
goto end

:error
echo Test failed!
pause
exit /b 1

:end
pause
exit /b 0