#include <stdio.h>
#include <stdlib.h>
#include "port_utils.h"
#include "system_utils.h"
#include "error.h"

void print_port_status(uint16_t port) {
    PortInfo info = check_port(port);
    printf("\nPort %d Status:\n", port);
    printf("Available: %s\n", info.is_available ? "Yes" : "No");
    
    if (!info.is_available) {
        printf("In use by: %s (PID: %lu)\n", 
               info.process_name, info.process_id);
    }
    
    // Test local connectivity
    BOOL local_connect = test_port_connectivity("127.0.0.1", port, 1000);
    printf("Local connectivity: %s\n", local_connect ? "Success" : "Failed");
    
    // Get system info for additional context
    SystemInfo sys_info = get_system_info();
    if (!info.is_available && !sys_info.is_admin) {
        printf("Note: Administrative privileges may be required to use this port\n");
    }
}

void scan_system_ports(void) {
    size_t count;
    PortScanResult* results = scan_common_ports(&count);
    
    if (!results) {
        printf("Failed to scan ports\n");
        return;
    }
    
    printf("\nCommon Ports Status:\n");
    printf("%-6s %-15s %s\n", "Port", "Service", "Status");
    printf("----------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        printf("%-6d %-15s %s\n",
               results[i].port,
               results[i].service_name,
               results[i].is_available ? "Available" : "In Use");
    }
    
    free(results);
}

void test_port_range(uint16_t start_port, uint16_t end_port) {
    printf("\nScanning port range %d-%d:\n", start_port, end_port);
    printf("First 10 available ports:\n");
    
    int found = 0;
    for (uint16_t port = start_port; port <= end_port && found < 10; port++) {
        PortInfo info = check_port(port);
        if (info.is_available) {
            printf("Port %d is available\n", port);
            found++;
        }
    }
}

int main(int argc, char* argv[]) {
    printf("Port Testing Utility\n");
    printf("===================\n");
    
    // Initialize Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }
    
    // Get system information
    SystemInfo sys_info = get_system_info();
    printf("System Information:\n");
    printf("Computer Name: %s\n", sys_info.computer_name);
    printf("User Name: %s\n", sys_info.username);
    printf("Admin Privileges: %s\n", sys_info.is_admin ? "Yes" : "No");
    printf("Firewall Enabled: %s\n", 
           check_firewall_enabled() ? "Yes" : "No");
    
    while (1) {
        printf("\nOptions:\n");
        printf("1. Test specific port\n");
        printf("2. Scan common ports\n");
        printf("3. Find available ports in range\n");
        printf("4. Exit\n");
        printf("Choose option: ");
        
        int choice;
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: {
                uint16_t port;
                printf("Enter port number: ");
                scanf("%hu", &port);
                print_port_status(port);
                break;
            }
            case 2:
                scan_system_ports();
                break;
            case 3: {
                uint16_t start_port, end_port;
                printf("Enter start port: ");
                scanf("%hu", &start_port);
                printf("Enter end port: ");
                scanf("%hu", &end_port);
                
                if (start_port > end_port) {
                    uint16_t temp = start_port;
                    start_port = end_port;
                    end_port = temp;
                }
                
                test_port_range(start_port, end_port);
                break;
            }
            case 4:
                WSACleanup();
                return 0;
            default:
                printf("Invalid option\n");
        }
    }
    
    return 0;
}