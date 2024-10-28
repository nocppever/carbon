@echo off
setlocal EnableDelayedExpansion

:: Set paths
set MSYS2_PATH=C:\msys64
set MINGW64_PATH=C:\msys64\mingw64
set OPENSSL_PATH=C:\msys64\mingw64

:: Add MinGW and OpenSSL to PATH
set PATH=%MINGW64_PATH%\bin;%PATH%

:: Create build directory
if not exist "build" mkdir build

:: Compiler and linker flags
set CFLAGS=-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0601 -Wall -O2
set INCLUDES=-I. -I"%MINGW64_PATH%\include"
set LIBS=-L"%MINGW64_PATH%\lib"

echo Fixing header files...

echo Creating monitor.h...
(
echo #ifndef MONITOR_H
echo #define MONITOR_H
echo.
echo #include "common.h"
echo.
echo void init_logging(void);
echo void set_monitor_status(const char* format, ...);
echo void run_monitor(void);
echo.
echo // Define log_message only in monitor.c
echo #ifndef MONITOR_C
echo extern void log_message(const char* format, ...);
echo #endif
echo.
echo #endif
) > monitor.h

echo Creating error.h...
(
echo #ifndef ERROR_H
echo #define ERROR_H
echo.
echo #include "common.h"
echo.
echo typedef enum {
echo     ERROR_NONE = 0,
echo     ERROR_CONFIG = 1,
echo     ERROR_NETWORK = 2,
echo     ERROR_SSL = 3,
echo     ERROR_THREAD = 4,
echo     ERROR_FILE = 5,
echo     ERROR_MEMORY = 6,
echo     ERROR_SYNC = 7
echo } ErrorCode;
echo.
echo void log_error(ErrorCode code, const char* message);
echo.
echo // Use log_message from monitor.h
echo #include "monitor.h"
echo.
echo #endif
) > error.h

echo Compiling...

:: Add MONITOR_C definition when compiling monitor.c
gcc %CFLAGS% %INCLUDES% -DMONITOR_C -c monitor.c -o build\monitor.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c error.c -o build\error.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c config.c -o build\config.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c system_utils.c -o build\system_utils.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c ssl.c -o build\ssl.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c sync.c -o build\sync.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c server.c -o build\server.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c server_init.c -o build\server_init.o
if errorlevel 1 goto error

gcc %CFLAGS% %INCLUDES% -c firewall_defs.c -o build\firewall_defs.o
if errorlevel 1 goto error

echo Linking...
gcc -o build\server.exe build\*.o ^
   %LIBS% -lws2_32 -lssl -lcrypto -liphlpapi -lpsapi -lole32 -loleaut32 -luserenv
if errorlevel 1 goto error

echo Build successful!
goto end

:error
echo Build failed!
pause
exit /b 1

:end
pause