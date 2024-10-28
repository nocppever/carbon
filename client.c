#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "error.h"
#include "ssl.h"
#include "checksum.h"
#include "sync.h"

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int signum) {
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    // Initialize
    Config* config = read_config("config.ini");
    if (!config) {
        log_error(ERROR_CONFIG, "Failed to read config");
        return ERROR_CONFIG;
    }
    
    // Initialize SSL
    init_openssl();
    SSL_CTX* ssl_ctx = create_ssl_context(0);  // 0 for client mode
    if (!ssl_ctx) {
        log_error(ERROR_SSL, "Failed to create SSL context");
        free(config);
        return ERROR_SSL;
    }
    
    // Set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("Client started, connecting to %s:%d\n", config->server_ip, config->server_port);
    
    while (keep_running) {
        SSL* ssl = NULL;
        int sock = -1;
        
        // Connect to server
        ErrorCode error = connect_to_server(config, ssl_ctx, &ssl, &sock);
        if (error == ERROR_NONE) {
            // First time connection setup
            if (config->num_computers == 0) {
                char computer_name[50];
                char dest_path[MAX_PATH];
                
                printf("Enter computer name: ");
                fgets(computer_name, sizeof(computer_name), stdin);
                computer_name[strcspn(computer_name, "\n")] = 0;
                
                printf("Enter destination path: ");
                fgets(dest_path, sizeof(dest_path), stdin);
                dest_path[strcspn(dest_path, "\n")] = 0;
                
                SSL_write(ssl, computer_name, strlen(computer_name) + 1);
                add_computer(config, computer_name, dest_path);
            }
            
            // Periodic sync
            printf("Starting file synchronization...\n");
            periodic_sync(config, ssl, 0);  // 0 indicates client mode
            
            // Cleanup connection
            SSL_free(ssl);
            close(sock);
        }
        
        printf("Connection lost or sync completed. Waiting %d seconds before next sync...\n", 
               config->sync_interval);
        sleep(config->sync_interval);
    }
    
    // Cleanup
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    free(config);
    
    return 0;
}