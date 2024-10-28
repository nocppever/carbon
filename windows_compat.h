#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#include "common.h"
#include <direct.h>
#include <io.h>

// Socket type definition
typedef SOCKET socket_t;
#define INVALID_SOCKET_FD INVALID_SOCKET
#define SOCKET_ERROR_FD SOCKET_ERROR

// Path separator
#define PATH_SEPARATOR "\\"

// Function mappings
#define sleep(x) Sleep((x)*1000)
#define mkdir(path, mode) _mkdir(path)
#define unlink(path) _unlink(path)
#define close_socket(x) closesocket(x)

// Initialize Winsock
static inline int init_winsock(void) {
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

// Cleanup Winsock
static inline void cleanup_winsock(void) {
    WSACleanup();
}

#endif