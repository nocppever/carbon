#include <openssl/ssl.h>
#include <openssl/err.h>
#include "sync.h"
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

int mkdirp(const char* path) {
    char temp[MAX_PATH];
    char* p = NULL;
    size_t len;

    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    if (temp[len - 1] == '\\') {
        temp[len - 1] = '\0';
    }

    for (p = temp + 1; *p; p++) {
        if (*p == '\\') {
            *p = '\0';
            if (mkdir(temp) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '\\';
        }
    }

    if (mkdir(temp) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

void execute_sync_action(const char* base_path, SyncAction* action, SSL* ssl, int is_server) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s\\%s", base_path, action->path);

    switch (action->action) {
        case 1: // Create
            if (action->is_directory) {
                mkdirp(full_path);
            } else {
                char dir_path[MAX_PATH];
                strncpy(dir_path, full_path, sizeof(dir_path) - 1);
                char* last_slash = strrchr(dir_path, '\\');
                if (last_slash) {
                    *last_slash = '\0';
                    mkdirp(dir_path);
                }

                if (is_server) {
                    // Server sends file
                    FILE* file = fopen(full_path, "rb");
                    if (file) {
                        char buffer[4096];
                        size_t bytes;
                        uint64_t file_size;
                        
                        // Get file size
                        fseek(file, 0, SEEK_END);
                        file_size = ftell(file);
                        fseek(file, 0, SEEK_SET);
                        
                        // Send file size first
                        SSL_write(ssl, &file_size, sizeof(file_size));
                        
                        // Send file content
                        while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                            SSL_write(ssl, buffer, (int)bytes);
                        }
                        fclose(file);
                    }
                } else {
                    // Client receives file
                    FILE* file = fopen(full_path, "wb");
                    if (file) {
                        char buffer[4096];
                        int bytes;
                        uint64_t file_size, received = 0;
                        
                        // Receive file size first
                        SSL_read(ssl, &file_size, sizeof(file_size));
                        
                        // Receive file content
                        while (received < file_size && 
                               (bytes = SSL_read(ssl, buffer, 
                                sizeof(buffer))) > 0) {
                            fwrite(buffer, 1, bytes, file);
                            received += bytes;
                        }
                        fclose(file);
                    }
                }
            }
            break;

        case 2: // Update
            if (!action->is_directory) {
                // Handle update same as create
                action->action = 1;
                execute_sync_action(base_path, action, ssl, is_server);
            }
            break;
    }
}

void periodic_sync(Config* config, SSL* ssl, int is_server) {
    // Implementation of periodic_sync
    // This function should periodically sync files between client and server

    // For simplicity, let's assume we have a list of actions to perform
    SyncAction actions[] = {
        {1, 0, "file1.txt"}, // Create file1.txt
        {1, 1, "dir1"},      // Create directory dir1
        {2, 0, "file2.txt"}  // Update file2.txt
    };
    size_t num_actions = sizeof(actions) / sizeof(actions[0]);

    for (size_t i = 0; i < num_actions; ++i) {
        execute_sync_action(".", &actions[i], ssl, is_server);
    }

    // Sleep for the sync interval
    Sleep(config->sync_interval * 1000);
}