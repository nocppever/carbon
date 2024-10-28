#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.h"
#include "error.h"
#include "ssl.h"
#include "thread_pool.h"
#include "checksum.h"
#include "sync.h"

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int signum) {
    keep_running = 0;
}

typedef struct {
    int client_socket;
    SSL* ssl;
    Config* config;
} ClientData;

static void handle_client(void* arg) {
    ClientData* data = (ClientData*)arg;
    char computer_name[50];
    
    // Initial handshake
    SSL_read(data->ssl, computer_name, sizeof(computer_name));
    
    // Periodic sync
    periodic_sync(data->config, data->ssl, 1);  // 1 indicates server mode
    
    // Cleanup
    SSL_free(data->ssl);
    close(data->client_socket);
    free(data);
}

static ClientData* accept_client(SSL_CTX* ssl_ctx, Config* config) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    static int server_fd = -1;
    
    // Create socket if it doesn't exist
    if (server_fd == -1) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            log_error(ERROR_NETWORK, "Socket creation failed");
            return NULL;
        }
        
        // Set socket options
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            log_error(ERROR_NETWORK, "setsockopt failed");
            close(server_fd);
            return NULL;
        }
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(config->server_port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            log_error(ERROR_NETWORK, "Bind failed");
            close(server_fd);
            return NULL;
        }
        
        if (listen(server_fd, 5) < 0) {
            log_error(ERROR_NETWORK, "Listen failed");
            close(server_fd);
            return NULL;
        }
    }
    
    // Accept client connection
    int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        log_error(ERROR_NETWORK, "Accept failed");
        return NULL;
    }
    
    // Allocate client data
    ClientData* client_data = (ClientData*)malloc(sizeof(ClientData));
    if (!client_data) {
        log_error(ERROR_MEMORY, "Client data allocation failed");
        close(client_socket);
        return NULL;
    }
    
    // Setup SSL
    client_data->ssl = SSL_new(ssl_ctx);
    if (!client_data->ssl) {
        log_error(ERROR_SSL, "SSL creation failed");
        free(client_data);
        close(client_socket);
        return NULL;
    }
    
    SSL_set_fd(client_data->ssl, client_socket);
    if (SSL_accept(client_data->ssl) <= 0) {
        log_error(ERROR_SSL, "SSL accept failed");
        SSL_free(client_data->ssl);
        free(client_data);
        close(client_socket);
        return NULL;
    }
    
    client_data->client_socket = client_socket;
    client_data->config = config;
    
    return client_data;
}

int main(int argc, char* argv[]) {
    // Initialize everything
    Config* config = read_config("config.ini");
    if (!config) {
        log_error(ERROR_CONFIG, "Failed to read config");
        return ERROR_CONFIG;
    }
    
    // Initialize SSL
    init_openssl();
    SSL_CTX* ssl_ctx = create_ssl_context(1);  // 1 for server mode
    if (!ssl_ctx) {
        log_error(ERROR_SSL, "Failed to create SSL context");
        free(config);
        return ERROR_SSL;
    }
    
    // Configure SSL
    if (configure_ssl_context(ssl_ctx, "server.crt", "server.key") != ERROR_NONE) {
        SSL_CTX_free(ssl_ctx);
        free(config);
        return ERROR_SSL;
    }
    
    // Initialize thread pool
    ThreadPool thread_pool;
    ErrorCode error = thread_pool_init(&thread_pool, 5);
    if (error != ERROR_NONE) {
        log_error(error, "Failed to initialize thread pool");
        SSL_CTX_free(ssl_ctx);
        free(config);
        return error;
    }
    
    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("Server started on port %d\n", config->server_port);
    
    // Main server loop
    while (keep_running) {
        ClientData* client_data = accept_client(ssl_ctx, config);
        if (client_data) {
            if (thread_pool_add_task(&thread_pool, handle_client, client_data) != ERROR_NONE) {
                SSL_free(client_data->ssl);
                close(client_data->client_socket);
                free(client_data);
            }
        }
        usleep(1000);  // Small sleep to prevent CPU spinning
    }
    
    // Cleanup
    thread_pool_destroy(&thread_pool);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    free(config);
    
    return 0;
}