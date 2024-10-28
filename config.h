#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_PATH 256
#define MAX_COMPUTERS 10
#define DEFAULT_SYNC_INTERVAL 300  // 5 minutes in seconds

typedef struct {
    char server_ip[16];
    uint16_t server_port;
    char parent_file_path[MAX_PATH];
    char destination_paths[MAX_COMPUTERS][MAX_PATH];
    char computer_names[MAX_COMPUTERS][50];
    uint8_t num_computers;
    uint32_t sync_interval;  // in seconds
} Config;

static inline Config* read_config(const char* filename) {
    Config* config = (Config*)malloc(sizeof(Config));
    if (!config) {
        return NULL;
    }
    memset(config, 0, sizeof(Config));
    
    FILE* file = fopen(filename, "r");
    
    if (!file) {
        printf("Config file not found. Creating default config...\n");
        config->server_port = 8080;
        strcpy(config->server_ip, "127.0.0.1");
        strcpy(config->parent_file_path, "./parent_file");
        config->num_computers = 0;
        config->sync_interval = DEFAULT_SYNC_INTERVAL;
        
        // Create default config file
        file = fopen(filename, "w");
        if (!file) {
            free(config);
            return NULL;
        }
        fprintf(file, "server_ip=127.0.0.1\n");
        fprintf(file, "server_port=8080\n");
        fprintf(file, "parent_file_path=./parent_file\n");
        fprintf(file, "sync_interval=300\n");
        fclose(file);
        return config;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[50], value[256];
        if (sscanf(line, "%49[^=]=%255s", key, value) == 2) {
            if (strcmp(key, "server_ip") == 0) {
                strncpy(config->server_ip, value, sizeof(config->server_ip) - 1);
            } else if (strcmp(key, "server_port") == 0) {
                config->server_port = (uint16_t)atoi(value);
            } else if (strcmp(key, "parent_file_path") == 0) {
                strncpy(config->parent_file_path, value, sizeof(config->parent_file_path) - 1);
            } else if (strcmp(key, "sync_interval") == 0) {
                config->sync_interval = (uint32_t)atoi(value);
            }
        }
    }
    
    fclose(file);
    return config;
}

static inline void add_computer(Config* config, const char* name, const char* path) {
    if (!config || !name || !path) return;
    
    if (config->num_computers < MAX_COMPUTERS) {
        strncpy(config->computer_names[config->num_computers], name, 
                sizeof(config->computer_names[0]) - 1);
        strncpy(config->destination_paths[config->num_computers], path,
                sizeof(config->destination_paths[0]) - 1);
        config->num_computers++;
        
        // Update config file
        FILE* file = fopen("config.ini", "a");
        if (file) {
            fprintf(file, "computer_%d_name=%s\n", config->num_computers, name);
            fprintf(file, "computer_%d_path=%s\n", config->num_computers, path);
            fclose(file);
        }
    }
}

#endif