#include "guidance_service.h"
#include <iostream>
#include <signal.h>

using namespace nav;

// Global variable for signal handling
static GuidanceService* g_service = nullptr;

void signalHandler(int signal) {
    if (g_service && signal == SIGINT) {
        std::cout << "\nReceived interrupt signal, shutting down..." << std::endl;
        g_service->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Starting Guidance Service..." << std::endl;
    
    // Parse command line arguments
    int positioning_channel = -1;
    int routing_channel = -1;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--positioning-channel" && i + 1 < argc) {
            positioning_channel = std::stoi(argv[++i]);
        } else if (arg == "--routing-channel" && i + 1 < argc) {
            routing_channel = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --positioning-channel <id>  Positioning service IPC channel ID\n";
            std::cout << "  --routing-channel <id>      Routing service IPC channel ID\n";
            std::cout << "  --help, -h                  Show this help message\n";
            return 0;
        }
    }
    
    // Create and initialize service
    GuidanceService service;
    g_service = &service;
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize service
    if (!service.initialize()) {
        std::cerr << "Failed to initialize guidance service" << std::endl;
        return 1;
    }
    
    // Connect to other services
    if (positioning_channel >= 0) {
        if (service.subscribeToPositioning(positioning_channel)) {
            std::cout << "Connected to positioning service channel: " << positioning_channel << std::endl;
        }
    }
    
    if (routing_channel >= 0) {
        service.setRoutingServiceChannel(routing_channel);
        std::cout << "Connected to routing service channel: " << routing_channel << std::endl;
    }
    
    // Start service
    if (!service.start()) {
        std::cerr << "Failed to start guidance service" << std::endl;
        return 1;
    }
    
    std::cout << "Guidance service started successfully" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Simulate setting a route for testing
    if (true) { // Enable simulation
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        Route test_route;
        test_route.node_count = 5;
        test_route.nodes[0] = 1001;
        test_route.nodes[1] = 1002;
        test_route.nodes[2] = 1003;
        test_route.nodes[3] = 1004;
        test_route.nodes[4] = 1005;
        test_route.total_distance_meters = 2000.0;
        test_route.estimated_time_seconds = 240.0;
        
        service.setActiveRoute(test_route);
        std::cout << "Test route activated" << std::endl;
    }
    
    // Keep running until stopped
    while (service.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Print current guidance
        if (service.hasActiveRoute()) {
            GuidanceInstruction current = service.getCurrentInstruction();
            std::cout << "Current guidance: " << current.instruction_text << std::endl;
        }
    }
    
    std::cout << "Guidance service stopped" << std::endl;
    return 0;
}