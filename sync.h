#ifndef SYNC_H
#define SYNC_H

#include "common.h"

typedef struct {
    int action;
    int is_directory;
    char path[MAX_PATH];
} SyncAction;

void execute_sync_action(const char* base_path, SyncAction* action, SSL* ssl, int is_server);
void periodic_sync(Config* config, SSL* ssl, int is_server);
int mkdirp(const char* path);

#endif // SYNC_H