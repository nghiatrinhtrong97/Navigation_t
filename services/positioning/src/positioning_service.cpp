#include "positioning_service.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <algorithm>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#endif

namespace nav {

PositioningService::PositioningService()
    : is_running_(false)
    , should_stop_(false)
    , using_dead_reckoning_(false)
    , last_valid_gps_time_(0)
    , gps_fd_(-1)
    , ipc_channel_(-1)
{
    can_interface_ = std::make_unique<CanInterface>();
    dead_reckoning_ = std::make_unique<DeadReckoning>();
    map_matching_ = std::make_unique<MapMatching>();
}

PositioningService::~PositioningService() {
    stop();
    
    if (gps_fd_ >= 0) {
        close(gps_fd_);
    }
    
#ifdef __QNX__
    if (ipc_channel_ >= 0) {
        ChannelDestroy(ipc_channel_);
    }
#endif
}

bool PositioningService::initialize(const std::string& gps_device, const std::string& can_device) {
    gps_device_path_ = gps_device;
    
    // Initialize CAN interface
    if (!can_interface_->initialize(can_device)) {
        std::cerr << "Failed to initialize CAN interface" << std::endl;
        return false;
    }
    
    // Open GPS serial port
    gps_fd_ = open(gps_device.c_str(), O_RDONLY | O_NOCTTY);
    if (gps_fd_ < 0) {
        std::cerr << "Failed to open GPS device: " << gps_device << std::endl;
        return false;
    }
    
    // Configure serial port for GPS (9600 baud, 8N1)
    struct termios tty;
    if (tcgetattr(gps_fd_, &tty) != 0) {
        std::cerr << "Error getting GPS serial attributes" << std::endl;
        close(gps_fd_);
        gps_fd_ = -1;
        return false;
    }
    
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr(gps_fd_, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting GPS serial attributes" << std::endl;
        close(gps_fd_);
        gps_fd_ = -1;
        return false;
    }
    
#ifdef __QNX__
    // Create IPC channel
    ipc_channel_ = ChannelCreate(0);
    if (ipc_channel_ < 0) {
        std::cerr << "Failed to create IPC channel" << std::endl;
        return false;
    }
#endif
    
    return true;
}

bool PositioningService::start() {
    if (is_running_) {
        return true;
    }
    
    should_stop_ = false;
    
    try {
        // Start threads
        gps_thread_ = std::thread(&PositioningService::gpsReaderThread, this);
        can_thread_ = std::thread(&PositioningService::canReaderThread, this);
        processor_thread_ = std::thread(&PositioningService::positionProcessorThread, this);
        ipc_thread_ = std::thread(&PositioningService::ipcServerThread, this);
        
        is_running_ = true;
        
        std::cout << "PositioningService started successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start PositioningService: " << e.what() << std::endl;
        return false;
    }
}

void PositioningService::stop() {
    if (!is_running_) {
        return;
    }
    
    should_stop_ = true;
    
    // Wait for threads to finish
    if (gps_thread_.joinable()) {
        gps_thread_.join();
    }
    if (can_thread_.joinable()) {
        can_thread_.join();
    }
    if (processor_thread_.joinable()) {
        processor_thread_.join();
    }
    if (ipc_thread_.joinable()) {
        ipc_thread_.join();
    }
    
    is_running_ = false;
    
    std::cout << "PositioningService stopped" << std::endl;
}

void PositioningService::gpsReaderThread() {
    char buffer[512];
    std::string line_buffer;
    
    while (!should_stop_) {
        if (gps_fd_ < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        ssize_t bytes_read = read(gps_fd_, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            line_buffer += buffer;
            
            // Process complete NMEA sentences
            size_t pos = 0;
            while ((pos = line_buffer.find('\n')) != std::string::npos) {
                std::string sentence = line_buffer.substr(0, pos);
                line_buffer.erase(0, pos + 1);
                
                // Remove carriage return if present
                if (!sentence.empty() && sentence.back() == '\r') {
                    sentence.pop_back();
                }
                
                // Parse NMEA sentence
                GpsData gps_data;
                if (NmeaParser::parseNmeaSentence(sentence, gps_data)) {
                    std::lock_guard<std::mutex> lock(gps_mutex_);
                    current_gps_data_ = gps_data;
                    
                    if (gps_data.valid) {
                        last_valid_gps_time_ = gps_data.timestamp_ms;
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void PositioningService::canReaderThread() {
    while (!should_stop_) {
        VehicleData vehicle_data;
        if (can_interface_->readVehicleData(vehicle_data)) {
            std::lock_guard<std::mutex> lock(vehicle_mutex_);
            current_vehicle_data_ = vehicle_data;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void PositioningService::positionProcessorThread() {
    while (!should_stop_) {
        updatePosition();
        std::this_thread::sleep_for(std::chrono::milliseconds(POSITION_UPDATE_INTERVAL_MS));
    }
}

void PositioningService::updatePosition() {
    Point new_position;
    bool gps_valid = false;
    uint64_t current_time = NavUtils::getCurrentTimestampMs();
    
    // Get current GPS data
    {
        std::lock_guard<std::mutex> lock(gps_mutex_);
        if (current_gps_data_.valid && 
            (current_time - current_gps_data_.timestamp_ms) < GPS_TIMEOUT_MS) {
            gps_valid = true;
            new_position = current_gps_data_.position;
            
            // Apply map matching if GPS is valid
            new_position = map_matching_->matchToMap(
                new_position, 
                current_gps_data_.course_degrees, 
                current_gps_data_.speed_kmh
            );
            
            using_dead_reckoning_ = false;
        }
    }
    
    // If GPS is not valid, use dead reckoning
    if (!gps_valid && (current_time - last_valid_gps_time_) < GPS_TIMEOUT_MS * 2) {
        VehicleData vehicle_data;
        {
            std::lock_guard<std::mutex> lock(vehicle_mutex_);
            vehicle_data = current_vehicle_data_;
        }
        
        if (!using_dead_reckoning_) {
            // Initialize dead reckoning with last known position
            std::lock_guard<std::mutex> lock(position_mutex_);
            dead_reckoning_->initialize(current_position_, current_gps_data_.course_degrees);
            using_dead_reckoning_ = true;
        }
        
        new_position = dead_reckoning_->updatePosition(vehicle_data);
    }
    
    // Update current position
    {
        std::lock_guard<std::mutex> lock(position_mutex_);
        current_position_ = new_position;
    }
    
    // Broadcast position update
    broadcastPositionUpdate();
}

void PositioningService::broadcastPositionUpdate() {
    PositionUpdateMsg msg;
    
    {
        std::lock_guard<std::mutex> lock(position_mutex_);
        msg.current_position = current_position_;
        msg.positioning_mode = using_dead_reckoning_ ? 1 : 0;
        msg.timestamp_ms = NavUtils::getCurrentTimestampMs();
    }
    
    {
        std::lock_guard<std::mutex> lock(gps_mutex_);
        msg.speed_kmh = current_gps_data_.speed_kmh;
        msg.heading_degrees = current_gps_data_.course_degrees;
        msg.gps_valid = current_gps_data_.valid;
    }
    
    // Send to all subscribers
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    for (uint32_t subscriber_pid : subscribers_) {
        // Send IPC message to subscriber
#ifdef __QNX__
        // Implementation would use MsgSend to send message to subscriber
        // This is simplified for demonstration
#else
        std::cout << "Broadcasting position to PID " << subscriber_pid 
                  << ": (" << msg.current_position.latitude 
                  << ", " << msg.current_position.longitude << ")" << std::endl;
#endif
    }
}

void PositioningService::ipcServerThread() {
#ifdef __QNX__
    struct _msg_info info;
    NavMessage msg;
    
    while (!should_stop_) {
        int rcvid = MsgReceive(ipc_channel_, &msg, sizeof(msg), &info);
        if (rcvid > 0) {
            handleIpcMessage(msg, rcvid);
        } else if (rcvid == 0) {
            // Pulse received
        } else {
            // Error
            if (errno != EINTR) {
                std::cerr << "MsgReceive error: " << strerror(errno) << std::endl;
            }
        }
    }
#else
    // Simulation mode
    while (!should_stop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
}

void PositioningService::handleIpcMessage(const NavMessage& message, int reply_channel) {
    switch (message.header.type) {
        case MessageType::SUBSCRIBE_POSITION: {
            uint32_t subscriber_pid = message.subscription.subscriber_pid;
            subscribeToPositionUpdates(subscriber_pid, message.subscription.update_interval_ms);
            
            // Send acknowledgment
            NavMessage reply;
            reply.header.type = MessageType::SERVICE_READY;
            reply.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
            
#ifdef __QNX__
            MsgReply(reply_channel, EOK, &reply, sizeof(reply.header));
#endif
            break;
        }
        
        case MessageType::UNSUBSCRIBE_POSITION: {
            uint32_t subscriber_pid = message.subscription.subscriber_pid;
            unsubscribeFromPositionUpdates(subscriber_pid);
            
            // Send acknowledgment
            NavMessage reply;
            reply.header.type = MessageType::SERVICE_READY;
            reply.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
            
#ifdef __QNX__
            MsgReply(reply_channel, EOK, &reply, sizeof(reply.header));
#endif
            break;
        }
        
        default: {
            // Unknown message type
            ErrorResponseMsg error_reply;
            error_reply.error_code = NavError::INVALID_PARAMETER;
            strcpy(error_reply.error_description, "Unknown message type");
            
#ifdef __QNX__
            MsgReply(reply_channel, EINVAL, &error_reply, sizeof(error_reply));
#endif
            break;
        }
    }
}

Point PositioningService::getCurrentPosition() const {
    std::lock_guard<std::mutex> lock(position_mutex_);
    return current_position_;
}

GpsData PositioningService::getCurrentGpsData() const {
    std::lock_guard<std::mutex> lock(gps_mutex_);
    return current_gps_data_;
}

VehicleData PositioningService::getCurrentVehicleData() const {
    std::lock_guard<std::mutex> lock(vehicle_mutex_);
    return current_vehicle_data_;
}

bool PositioningService::subscribeToPositionUpdates(uint32_t subscriber_pid, uint32_t update_interval_ms) {
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    
    // Check if already subscribed
    auto it = std::find(subscribers_.begin(), subscribers_.end(), subscriber_pid);
    if (it == subscribers_.end()) {
        subscribers_.push_back(subscriber_pid);
        return true;
    }
    
    return false; // Already subscribed
}

void PositioningService::unsubscribeFromPositionUpdates(uint32_t subscriber_pid) {
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    
    auto it = std::find(subscribers_.begin(), subscribers_.end(), subscriber_pid);
    if (it != subscribers_.end()) {
        subscribers_.erase(it);
    }
}

} // namespace nav