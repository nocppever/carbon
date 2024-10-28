@echo off
"C:\OpenSSL\bin\openssl.exe" req -x509 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.crt
pause