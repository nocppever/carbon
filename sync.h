#ifndef SYNC_H
#define SYNC_H

#include "checksum.h"
#include "ssl.h"
#include "error.h"

typedef struct {
    char path[MAX_PATH];
    int action;  // 1: create, 2: update, 3: delete
    int is_directory;
} SyncAction;

typedef struct {
    SyncAction* actions;
    size_t count;
    size_t capacity;
} SyncPlan;

static inline SyncPlan* create_sync_plan(FileList* source, FileList* dest) {
    SyncPlan* plan = (SyncPlan*)malloc(sizeof(SyncPlan));
    if (!plan) return NULL;

    plan->capacity = source->count + dest->count;
    plan->actions = (SyncAction*)malloc(sizeof(SyncAction) * plan->capacity);
    if (!plan->actions) {
        free(plan);
        return NULL;
    }
    plan->count = 0;

    // First, mark files to create/update
    for (size_t i = 0; i < source->count; i++) {
        int found = 0;
        for (size_t j = 0; j < dest->count; j++) {
            if (strcmp(source->files[i].path, dest->files[j].path) == 0) {
                found = 1;
                if (memcmp(source->files[i].hash, dest->files[j].hash, HASH_SIZE) != 0) {
                    SyncAction* action = &plan->actions[plan->count++];
                    strncpy(action->path, source->files[i].path, MAX_PATH - 1);
                    action->action = 2;
                    action->is_directory = source->files[i].is_directory;
                }
                break;
            }
        }
        if (!found) {
            SyncAction* action = &plan->actions[plan->count++];
            strncpy(action->path, source->files[i].path, MAX_PATH - 1);
            action->action = 1;
            action->is_directory = source->files[i].is_directory;
        }
    }

    // Then, mark files to delete
    for (size_t i = 0; i < dest->count; i++) {
        int found = 0;
        for (size_t j = 0; j < source->count; j++) {
            if (strcmp(dest->files[i].path, source->files[j].path) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            SyncAction* action = &plan->actions[plan->count++];
            strncpy(action->path, dest->files[i].path, MAX_PATH - 1);
            action->action = 3;
            action->is_directory = dest->files[i].is_directory;
        }
    }

    return plan;
}

static inline void execute_sync_action(const char* base_path, SyncAction* action, 
                                     SSL* ssl, int is_server) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, action->path);

    switch (action->action) {
        case 1: // Create
            if (action->is_directory) {
                mkdir(full_path, 0755);
            } else {
                // Receive/Send file content
                if (is_server) {
                    FILE* file = fopen(full_path, "rb");
                    if (file) {
                        char buffer[4096];
                        size_t bytes;
                        while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                            SSL_write(ssl, buffer, bytes);
                        }
                        fclose(file);
                    }
                } else {
                    FILE* file = fopen(full_path, "wb");
                    if (file) {
                        char buffer[4096];
                        int bytes;
                        while ((bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
                            fwrite(buffer, 1, bytes, file);
                        }
                        fclose(file);
                    }
                }
            }
            break;

        case 2: // Update
            if (!action->is_directory) {
                char dir_path[MAX_PATH];
                strncpy(dir_path, full_path, sizeof(dir_path));
                char* last_slash = strrchr(dir_path, '/');
                if (last_slash) {
                    *last_slash = '\0';
                    mkdir(dir_path, 0755);
                }
                execute_sync_action(base_path, action, ssl, is_server);  // Reuse create logic
            }
            break;

        case 3: // Delete
            if (action->is_directory) {
                rmdir(full_path);
            } else {
                unlink(full_path);
            }
            break;
    }
}

static inline void periodic_sync(Config* config, SSL* ssl, int is_server) {
    while (1) {
        FileList* source_list = get_file_list(is_server ? 
            config->parent_file_path : config->destination_paths[0]);
        
        if (is_server) {
            SSL_write(ssl, source_list, sizeof(FileList));
        } else {
            FileList received_list;
            SSL_read(ssl, &received_list, sizeof(FileList));
            
            SyncPlan* plan = create_sync_plan(&received_list, source_list);
            
            for (size_t i = 0; i < plan->count; i++) {
                execute_sync_action(config->destination_paths[0], 
                                  &plan->actions[i], ssl, is_server);
            }
            
            free(plan->actions);
            free(plan);
        }
        
        free(source_list);
        sleep(config->sync_interval);
    }
}

#endif