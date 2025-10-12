#include "nav_utils.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

#ifdef __QNX__
#include <unistd.h>
#include <sys/socket.h>
#include <sys/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#else
// For Windows and other systems (simulation/testing)
#include <iostream>
#ifdef _WIN32
#include <io.h>
// Windows doesn't have these Unix functions, we'll use simulation mode
#else
#include <unistd.h>
#endif
#endif

namespace nav {

CanInterface::CanInterface() : can_socket_(-1), is_initialized_(false) {
}

CanInterface::~CanInterface() {
    close();
}

bool CanInterface::initialize(const std::string& can_device) {
#if defined(__QNX__) || defined(__linux__)
    // Create CAN socket
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) {
        return false;
    }
    
    // Get interface index
    struct ifreq ifr;
    strcpy(ifr.ifr_name, can_device.c_str());
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        ::close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    
    // Bind socket to CAN interface
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(can_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(can_socket_, F_GETFL, 0);
    fcntl(can_socket_, F_SETFL, flags | O_NONBLOCK);
    
    is_initialized_ = true;
    return true;
#else
    // Simulation mode for Windows and other systems
    std::cout << "CAN Interface initialized in simulation mode on " << can_device << std::endl;
    can_socket_ = 1; // Fake socket
    is_initialized_ = true;
    return true;
#endif
}

void CanInterface::close() {
    if (can_socket_ >= 0) {
#if defined(__QNX__) || defined(__linux__)
        ::close(can_socket_);
#elif defined(_WIN32)
        // Windows simulation mode - just reset the socket
        // In real Windows CAN implementation, you'd close the actual handle here
#endif
        can_socket_ = -1;
    }
    is_initialized_ = false;
}

bool CanInterface::readVehicleData(VehicleData& vehicle_data) {
    if (!is_initialized_) {
        return false;
    }
    
#if defined(__QNX__) || defined(__linux__)
    struct can_frame frame;
    ssize_t nbytes = read(can_socket_, &frame, sizeof(frame));
    
    if (nbytes < 0) {
        return false; // No data available or error
    }
    
    if (nbytes == sizeof(frame)) {
        // Parse CAN message based on ID
        switch (frame.can_id) {
            case VEHICLE_SPEED_MSG_ID:
                // Parse speed data (implementation depends on vehicle protocol)
                if (frame.can_dlc >= 2) {
                    uint16_t speed_raw = (frame.data[0] << 8) | frame.data[1];
                    vehicle_data.speed_kmh = speed_raw * 0.1; // Example scaling
                }
                break;
                
            case YAW_RATE_MSG_ID:
                // Parse yaw rate data
                if (frame.can_dlc >= 2) {
                    int16_t yaw_raw = (frame.data[0] << 8) | frame.data[1];
                    vehicle_data.yaw_rate = yaw_raw * 0.1; // Example scaling
                }
                break;
                
            default:
                // Unknown message
                break;
        }
        
        vehicle_data.timestamp_ms = NavUtils::getCurrentTimestampMs();
        return true;
    }
#else
    // Simulation mode - generate fake data
    static uint64_t last_update = 0;
    uint64_t now = NavUtils::getCurrentTimestampMs();
    
    if (now - last_update > 50) { // Update every 50ms
        vehicle_data.speed_kmh = 50.0 + (rand() % 20 - 10); // 40-60 km/h
        vehicle_data.yaw_rate = (rand() % 100 - 50) * 0.1;  // -5 to +5 deg/s
        vehicle_data.timestamp_ms = now;
        last_update = now;
        return true;
    }
#endif
    
    return false;
}

bool CanInterface::sendGuidanceData(const GuidanceInstruction& instruction) {
    if (!is_initialized_) {
        return false;
    }
    
#if defined(__QNX__) || defined(__linux__)
    struct can_frame frame;
    frame.can_id = GUIDANCE_MSG_ID;
    frame.can_dlc = 8;
    
    // Pack guidance data into CAN frame
    frame.data[0] = static_cast<uint8_t>(instruction.turn_type);
    frame.data[1] = static_cast<uint8_t>(instruction.distance_to_turn_meters >> 8);
    frame.data[2] = static_cast<uint8_t>(instruction.distance_to_turn_meters & 0xFF);
    frame.data[3] = static_cast<uint8_t>(instruction.target_node_id >> 24);
    frame.data[4] = static_cast<uint8_t>(instruction.target_node_id >> 16);
    frame.data[5] = static_cast<uint8_t>(instruction.target_node_id >> 8);
    frame.data[6] = static_cast<uint8_t>(instruction.target_node_id & 0xFF);
    frame.data[7] = 0; // Reserved
    
    ssize_t nbytes = write(can_socket_, &frame, sizeof(frame));
    return nbytes == sizeof(frame);
#else
    // Simulation mode
    std::cout << "CAN TX: " << NavUtils::turnTypeToString(instruction.turn_type) 
              << " in " << instruction.distance_to_turn_meters << "m" << std::endl;
    return true;
#endif
}

} // namespace nav