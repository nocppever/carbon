#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <windows.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include "error.h"

#define HASH_SIZE 32  // SHA256 hash size
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

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* sha256 = EVP_sha256();
    unsigned int hash_len;

    EVP_DigestInit_ex(ctx, sha256, NULL);

    unsigned char buffer[4096];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        EVP_DigestUpdate(ctx, buffer, bytes);
    }

    EVP_DigestFinal_ex(ctx, hash, &hash_len);
    EVP_MD_CTX_free(ctx);
    fclose(file);
}

static inline void scan_directory(const char* base_path, const char* rel_path, 
                                FileList* list, size_t base_len) {
    char full_path[MAX_PATH];
    WIN32_FIND_DATA find_data;
    HANDLE find_handle;
    char search_path[MAX_PATH];

    // Construct the full path for searching
    snprintf(full_path, sizeof(full_path), "%s\\%s", base_path, rel_path);
    snprintf(search_path, sizeof(search_path), "%s\\*", full_path);

    // Start finding files
    find_handle = FindFirstFile(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) return;

    do {
        // Skip "." and ".." directories
        if (strcmp(find_data.cFileName, ".") == 0 || 
            strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        // Construct relative and full paths for the current file
        char current_rel_path[MAX_PATH];
        char current_full_path[MAX_PATH];
        
        if (strlen(rel_path) == 0) {
            snprintf(current_rel_path, sizeof(current_rel_path), "%s", 
                    find_data.cFileName);
        } else {
            snprintf(current_rel_path, sizeof(current_rel_path), "%s\\%s", 
                    rel_path, find_data.cFileName);
        }
        
        snprintf(current_full_path, sizeof(current_full_path), "%s\\%s", 
                base_path, current_rel_path);

        if (list->count >= MAX_FILES) break;

        FileMetadata* meta = &list->files[list->count];
        strncpy(meta->path, current_rel_path, sizeof(meta->path) - 1);
        meta->path[sizeof(meta->path) - 1] = '\0';

        meta->is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        if (meta->is_directory) {
            meta->hash[0] = 0;
            meta->size = 0;
            meta->modified_time = 0;
            list->count++;
            
            // Recursively scan subdirectories
            scan_directory(base_path, current_rel_path, list, base_len);
        } else {
            meta->size = (find_data.nFileSizeHigh * (MAXDWORD + 1)) + 
                        find_data.nFileSizeLow;
            
            ULARGE_INTEGER ull;
            ull.LowPart = find_data.ftLastWriteTime.dwLowDateTime;
            ull.HighPart = find_data.ftLastWriteTime.dwHighDateTime;
            meta->modified_time = (ull.QuadPart - 116444736000000000ULL) / 10000000ULL;
            
            calculate_file_hash(current_full_path, meta->hash);
            list->count++;
        }
    } while (FindNextFile(find_handle, &find_data));

    FindClose(find_handle);
}

static inline FileList* get_file_list(const char* path) {
    FileList* list = (FileList*)malloc(sizeof(FileList));
    if (!list) return NULL;

    list->count = 0;
    scan_directory(path, "", list, strlen(path));
    return list;
}

#endif