#include "routing_service.h"
#include <iostream>
#include <signal.h>

using namespace nav;

// Global variable for signal handling
static RoutingService* g_service = nullptr;

void signalHandler(int signal) {
    if (g_service && signal == SIGINT) {
        std::cout << "\nReceived interrupt signal, shutting down..." << std::endl;
        g_service->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Starting Routing Service..." << std::endl;
    
    // Parse command line arguments
    int map_service_channel = -1;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--map-channel" && i + 1 < argc) {
            map_service_channel = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --map-channel <id>  Map service IPC channel ID\n";
            std::cout << "  --help, -h          Show this help message\n";
            return 0;
        }
    }
    
    // Create and initialize service
    RoutingService service;
    g_service = &service;
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize service
    if (!service.initialize()) {
        std::cerr << "Failed to initialize routing service" << std::endl;
        return 1;
    }
    
    // Set map service channel if provided
    if (map_service_channel >= 0) {
        service.setMapServiceChannel(map_service_channel);
        std::cout << "Connected to map service channel: " << map_service_channel << std::endl;
    }
    
    // Start service
    if (!service.start()) {
        std::cerr << "Failed to start routing service" << std::endl;
        return 1;
    }
    
    std::cout << "Routing service started successfully" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Keep running until stopped
    while (service.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Print status information
        std::cout << "Routing service running..." << std::endl;
    }
    
    std::cout << "Routing service stopped" << std::endl;
    return 0;
}