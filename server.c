#include "common.h"
#include "config.h"
#include "ssl.h"
#include "sync.h"
#include "monitor.h"
#include "error.h"
#include "server_init.h"

static Config* config = NULL;

void handle_client(ClientData* data) {
    char computer_name[50];
    int bytes_read = SSL_read(data->ssl, computer_name, sizeof(computer_name));
    if (bytes_read > 0) {
        computer_name[bytes_read] = '\0';
        log_message("Connected to %s", computer_name);
        periodic_sync(data->config, data->ssl, 1);  // 1 indicates server mode
    }
    SSL_free(data->ssl);
    closesocket(data->client_socket);
    free(data);
}

void start_server() {
    SOCKET server_socket;
    struct sockaddr_in server_addr;
    SSL_CTX* ssl_ctx = create_ssl_context(1);  // Pass 1 for server mode
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        log_error(ERROR_NETWORK, "Failed to create socket");
        return;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config->server_port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        log_error(ERROR_NETWORK, "Failed to bind socket");
        closesocket(server_socket);
        return;
    }
    
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        log_error(ERROR_NETWORK, "Failed to listen on socket");
        closesocket(server_socket);
        return;
    }
    
    set_monitor_status("Server listening on port %d", config->server_port);
    log_message("Server started successfully on port %d", config->server_port);
    
    accept_connections(server_socket, ssl_ctx);
}

void accept_connections(SOCKET server_socket, SSL_CTX* ssl_ctx) {
    while (1) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            log_error(ERROR_NETWORK, "Failed to accept connection");
            continue;
        }
        
        ClientData* client_data = (ClientData*)malloc(sizeof(ClientData));
        if (!client_data) {
            log_error(ERROR_MEMORY, "Failed to allocate memory for client data");
            closesocket(client_socket);
            continue;
        }
        
        client_data->ssl = SSL_new(ssl_ctx);
        if (!client_data->ssl) {
            log_error(ERROR_SSL, "Failed to create SSL object");
            free(client_data);
            closesocket(client_socket);
            continue;
        }
        
        SSL_set_fd(client_data->ssl, client_socket);
        if (SSL_accept(client_data->ssl) <= 0) {
            log_error(ERROR_SSL, "Failed to accept SSL connection");
            SSL_free(client_data->ssl);
            free(client_data);
            closesocket(client_socket);
            continue;
        }
        
        client_data->client_socket = client_socket;
        client_data->config = config;
        
        handle_client(client_data);
    }
}

int main() {
    config = read_config("config.ini");
    if (!config) {
        log_error(ERROR_CONFIG, "Failed to read config file");
        return 1;
    }
    
    SystemCheck sys_check = {0};
    if (check_system_requirements(&sys_check) != ERROR_NONE) {
        log_error(ERROR_CONFIG, "System requirements not met");
        return 1;
    }
    
    SSL_CTX* ssl_ctx = create_ssl_context(1);  // Pass 1 for server mode
    if (!ssl_ctx) {
        log_error(ERROR_SSL, "Failed to create SSL context");
        return 1;
    }
    
    if (configure_ssl_context(ssl_ctx, "server.crt", "server.key") != ERROR_NONE) {
        log_error(ERROR_SSL, "Failed to configure SSL context");
        return 1;
    }
    
    start_server();
    
    SSL_CTX_free(ssl_ctx);
    free(config);
    return 0;
}