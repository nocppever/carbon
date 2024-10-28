#include "common.h"
#include "config.h"

Config* read_config(const char* filename) {
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
        config->sync_interval = 300;  // 5 minutes
        
        // Create default config file
        file = fopen(filename, "w");
        if (!file) {
            free(config);
            return NULL;
        }
        fprintf(file, "server_ip=127.0.0.1\n");
        fprintf(file, "server_port=8080\n");
        fprintf(file, "sync_interval=300\n");
        fclose(file);
        return config;
    }
    
    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file)) {
        char key[50], value[MAX_PATH];
        if (sscanf(line, "%49[^=]=%[^\n]", key, value) == 2) {
            // Remove any trailing whitespace
            char* end = value + strlen(value) - 1;
            while (end > value && isspace(*end)) *end-- = '\0';
            
            if (strcmp(key, "server_ip") == 0) {
                strncpy_s(config->server_ip, sizeof(config->server_ip), value, sizeof(config->server_ip) - 1);
            } else if (strcmp(key, "server_port") == 0) {
                config->server_port = (uint16_t)atoi(value);
            } else if (strcmp(key, "sync_interval") == 0) {
                config->sync_interval = (uint32_t)atoi(value);
            }
        }
    }
    
    fclose(file);
    return config;
}