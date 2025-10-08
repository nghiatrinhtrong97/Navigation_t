#include "positioning_service.h"
#include <iostream>
#include <signal.h>

using namespace nav;

// Global variable for signal handling
static PositioningService* g_service = nullptr;

void signalHandler(int signal) {
    if (g_service && signal == SIGINT) {
        std::cout << "\nReceived interrupt signal, shutting down..." << std::endl;
        g_service->stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Starting Positioning Service..." << std::endl;
    
    // Parse command line arguments
    std::string gps_device = "/dev/ser1";
    std::string can_device = "can0";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--gps" && i + 1 < argc) {
            gps_device = argv[++i];
        } else if (arg == "--can" && i + 1 < argc) {
            can_device = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --gps <device>  GPS serial device (default: /dev/ser1)\n";
            std::cout << "  --can <device>  CAN interface device (default: can0)\n";
            std::cout << "  --help, -h      Show this help message\n";
            return 0;
        }
    }
    
    // Create and initialize service
    PositioningService service;
    g_service = &service;
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    
    // Initialize service
    if (!service.initialize(gps_device, can_device)) {
        std::cerr << "Failed to initialize positioning service" << std::endl;
        return 1;
    }
    
    // Start service
    if (!service.start()) {
        std::cerr << "Failed to start positioning service" << std::endl;
        return 1;
    }
    
    std::cout << "Positioning service started successfully" << std::endl;
    std::cout << "GPS device: " << gps_device << std::endl;
    std::cout << "CAN device: " << can_device << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Keep running until stopped
    while (service.isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Print status information
        Point current_pos = service.getCurrentPosition();
        GpsData gps_data = service.getCurrentGpsData();
        VehicleData vehicle_data = service.getCurrentVehicleData();
        
        std::cout << "Position: (" << std::fixed << std::setprecision(6) 
                  << current_pos.latitude << ", " << current_pos.longitude << ") ";
        std::cout << "GPS Valid: " << (gps_data.valid ? "Yes" : "No") << " ";
        std::cout << "Speed: " << std::setprecision(1) << vehicle_data.speed_kmh << " km/h";
        std::cout << std::endl;
    }
    
    std::cout << "Positioning service stopped" << std::endl;
    return 0;
}