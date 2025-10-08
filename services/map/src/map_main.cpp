#include "map_service.h"
#include <iostream>
#include <signal.h>

using namespace nav;

// Global variable for signal handling
static MapService* g_service = nullptr;

void signalHandler(int signal) {
    if (g_service && signal == SIGINT) {
        std::cout << "\nReceived interrupt signal, shutting down..." << std::endl;
        g_service->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Starting Map Service..." << std::endl;
    
    // Parse command line arguments
    std::string map_data_path = "/opt/nav/data/map.data";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) {
            map_data_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --data <path>   Map data file path (default: /opt/nav/data/map.data)\n";
            std::cout << "  --help, -h      Show this help message\n";
            return 0;
        }
    }
    
    // Create and initialize service
    MapService service;
    g_service = &service;
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize service
    if (!service.initialize(map_data_path)) {
        std::cerr << "Failed to initialize map service" << std::endl;
        return 1;
    }
    
    // Start service
    if (!service.start()) {
        std::cerr << "Failed to start map service" << std::endl;
        return 1;
    }
    
    std::cout << "Map service started successfully" << std::endl;
    std::cout << "Map data: " << map_data_path << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Keep running until stopped
    while (service.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Print status information
        std::cout << "Map service running - Cache size: " 
                  << "tiles loaded" << std::endl;
    }
    
    std::cout << "Map service stopped" << std::endl;
    return 0;
}