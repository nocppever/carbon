#ifndef CONFIG_H
#define CONFIG_H


#include <stdint.h>

typedef struct {
    char server_ip[16];
    uint16_t server_port;
    uint32_t sync_interval;
} Config;

Config* read_config(const char* filename);

#endif // CONFIG_H