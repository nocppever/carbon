#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <openssl/sha.h>
#include <dirent.h>
#include <sys/stat.h>
#include "error.h"

#define HASH_SIZE SHA256_DIGEST_LENGTH
#define MAX_FILES 1000

typedef struct {
    char path[MAX_PATH];
    unsigned char hash[HASH_SIZE];
    time_t modified_time;
    size_t size;
    int is_directory;
} FileMetadata;

typedef struct {
    FileMetadata files[MAX_FILES];
    size_t count;
} FileList;

static inline void calculate_file_hash(const char* path, unsigned char* hash) {
    FILE* file = fopen(path, "rb");
    if (!file) return;

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    unsigned char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&sha256, buffer, bytes);
    }

    SHA256_Final(hash, &sha256);
    fclose(file);
}

static inline void scan_directory(const char* base_path, const char* rel_path, 
                                FileList* list, size_t base_len) {
    char full_path[MAX_PATH];
    struct dirent* entry;
    struct stat statbuf;
    DIR* dir;

    snprintf(full_path, sizeof(full_path), "%s/%s", base_path, rel_path);
    dir = opendir(full_path);
    if (!dir) return;

    while ((entry = readdir(dir)) != NULL && list->count < MAX_FILES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        char file_path[MAX_PATH];
        char rel_file_path[MAX_PATH];
        
        snprintf(file_path, sizeof(file_path), "%s/%s", full_path, entry->d_name);
        snprintf(rel_file_path, sizeof(rel_file_path), "%s/%s", 
                rel_path[0] == '\0' ? "" : rel_path, entry->d_name);

        if (stat(file_path, &statbuf) == 0) {
            FileMetadata* meta = &list->files[list->count];
            strncpy(meta->path, rel_file_path, sizeof(meta->path) - 1);
            meta->modified_time = statbuf.st_mtime;
            meta->size = statbuf.st_size;
            meta->is_directory = S_ISDIR(statbuf.st_mode);

            if (meta->is_directory) {
                meta->hash[0] = 0;
                scan_directory(base_path, rel_file_path, list, base_len);
            } else {
                calculate_file_hash(file_path, meta->hash);
            }
            
            list->count++;
        }
    }

    closedir(dir);
}

static inline FileList* get_file_list(const char* path) {
    FileList* list = (FileList*)malloc(sizeof(FileList));
    if (!list) return NULL;

    list->count = 0;
    scan_directory(path, "", list, strlen(path));
    return list;
}

#endif