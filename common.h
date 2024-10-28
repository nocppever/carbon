#ifndef COMMON_H
#define COMMON_H

// Prevent multiple definitions
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Required Windows headers in correct order
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <stdint.h>

#include "error.h"
#include "config.h"  // Include config.h to ensure Config is defined

// Forward declarations for OpenSSL
typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;


// Common definitions
#define MAX_PATH 260
#define MAX_COMPUTERS 10
#define DEFAULT_PORT 8080

// Client data structure
typedef struct {
    SSL* ssl;
    Config* config;
    SOCKET client_socket;
} ClientData;

// System check structure
typedef struct {
    BOOL is_admin;
    BOOL firewall_enabled;
    BOOL port_blocked;
    char username[256];
    char computer_name[256];
} SystemCheck;

// Function declarations for server.c
ErrorCode check_system_requirements(SystemCheck* sys_check);
ErrorCode configure_ssl_context(SSL_CTX* ssl_ctx, const char* cert_file, const char* key_file);
void handle_client(ClientData* data);
void start_server(void);
void accept_connections(SOCKET server_socket, SSL_CTX* ssl_ctx);
void periodic_sync(Config* config, SSL* ssl, int is_server);

#endif // COMMON_H