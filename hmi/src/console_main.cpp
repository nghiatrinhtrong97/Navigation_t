#include "nav_types.h"
#include "nav_messages.h"
#include "nav_utils.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#endif

using namespace nav;

class ConsoleNavigationApp {
public:
    ConsoleNavigationApp() 
        : positioning_channel_(-1)
        , routing_channel_(-1)
        , guidance_channel_(-1)
        , map_channel_(-1)
    {
    }
    
    bool initialize() {
        std::cout << "=== Navigation Console Application ===" << std::endl;
        std::cout << "Initializing navigation services..." << std::endl;
        
#ifdef __QNX__
        // Connect to services (in a real system, you'd discover these)
        positioning_channel_ = 1; // Placeholder
        routing_channel_ = 2;
        guidance_channel_ = 3;
        map_channel_ = 4;
#endif
        
        return true;
    }
    
    void run() {
        std::cout << "Navigation system ready!" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  route <lat1> <lon1> <lat2> <lon2> - Calculate route" << std::endl;
        std::cout << "  position - Get current position" << std::endl;
        std::cout << "  guidance - Get current guidance" << std::endl;
        std::cout << "  quit - Exit application" << std::endl;
        
        std::string command;
        while (std::cout << "\nnav> " && std::getline(std::cin, command)) {
            if (command == "quit" || command == "exit") {
                break;
            }
            
            processCommand(command);
        }
    }
    
private:
    void processCommand(const std::string& command) {
        if (command.empty()) return;
        
        if (command == "position") {
            getCurrentPosition();
        } else if (command == "guidance") {
            getCurrentGuidance();
        } else if (command.substr(0, 5) == "route") {
            parseRouteCommand(command);
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
    
    void getCurrentPosition() {
        std::cout << "Getting current position..." << std::endl;
        
        // Simulate position data
        Point current_pos(21.028511, 105.804817); // Hanoi, Vietnam
        std::cout << "Current position: " << std::fixed << std::setprecision(6)
                  << current_pos.latitude << ", " << current_pos.longitude << std::endl;
    }
    
    void getCurrentGuidance() {
        std::cout << "Getting current guidance..." << std::endl;
        
        // Simulate guidance instruction
        std::cout << "Turn right in 200 meters" << std::endl;
        std::cout << "Distance to destination: 2.5 km" << std::endl;
        std::cout << "Estimated time: 5 minutes" << std::endl;
    }
    
    void parseRouteCommand(const std::string& command) {
        // Simple parsing: route lat1 lon1 lat2 lon2
        std::istringstream iss(command);
        std::string cmd;
        double lat1, lon1, lat2, lon2;
        
        if (iss >> cmd >> lat1 >> lon1 >> lat2 >> lon2) {
            calculateRoute(Point(lat1, lon1), Point(lat2, lon2));
        } else {
            std::cout << "Usage: route <lat1> <lon1> <lat2> <lon2>" << std::endl;
            std::cout << "Example: route 21.028 105.804 21.035 105.820" << std::endl;
        }
    }
    
    void calculateRoute(const Point& start, const Point& end) {
        std::cout << "Calculating route from (" << std::fixed << std::setprecision(6)
                  << start.latitude << ", " << start.longitude << ") to ("
                  << end.latitude << ", " << end.longitude << ")" << std::endl;
        
        // Simulate route calculation
        std::cout << "Route calculation in progress..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        double distance = NavUtils::haversineDistance(start, end);
        double time = (distance / 1000.0) / 50.0 * 3600.0; // Assuming 50 km/h
        
        std::cout << "Route found!" << std::endl;
        std::cout << "Distance: " << NavUtils::formatDistance(distance) << std::endl;
        std::cout << "Estimated time: " << NavUtils::formatTime(time) << std::endl;
        std::cout << "Route activated for guidance" << std::endl;
    }
    
    int positioning_channel_;
    int routing_channel_;
    int guidance_channel_;
    int map_channel_;
};

int main(int argc, char* argv[]) {
    ConsoleNavigationApp app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize navigation application" << std::endl;
        return 1;
    }
    
    app.run();
    
    std::cout << "Navigation application terminated" << std::endl;
    return 0;
}