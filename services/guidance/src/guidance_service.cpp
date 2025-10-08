#include "guidance_service.h"
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef __QNX__
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#endif

namespace nav {

GuidanceService::GuidanceService()
    : is_running_(false)
    , should_stop_(false)
    , has_active_route_(false)
    , current_heading_(0.0)
    , has_current_position_(false)
    , last_guidance_update_(0)
    , ipc_channel_(-1)
    , positioning_channel_(-1)
    , routing_channel_(-1)
    , last_reroute_request_time_(0)
{
    route_monitor_ = std::make_unique<RouteMonitoring>();
    instruction_generator_ = std::make_unique<TurnByTurnGenerator>();
    can_interface_ = std::make_unique<CanInterface>();
}

GuidanceService::~GuidanceService() {
    stop();
    
#ifdef __QNX__
    if (ipc_channel_ >= 0) {
        ChannelDestroy(ipc_channel_);
    }
#endif
}

bool GuidanceService::initialize() {
    // Initialize CAN interface for instrument cluster communication
    if (!can_interface_->initialize("can0")) {
        std::cerr << "Warning: Failed to initialize CAN interface" << std::endl;
        // Continue without CAN - not critical for basic functionality
    }
    
#ifdef __QNX__
    // Create IPC channel
    ipc_channel_ = ChannelCreate(0);
    if (ipc_channel_ < 0) {
        std::cerr << "Failed to create IPC channel for GuidanceService" << std::endl;
        return false;
    }
#endif
    
    std::cout << "GuidanceService initialized" << std::endl;
    return true;
}

bool GuidanceService::start() {
    if (is_running_) {
        return true;
    }
    
    should_stop_ = false;
    
    try {
        // Start threads
        ipc_thread_ = std::thread(&GuidanceService::ipcServerThread, this);
        guidance_thread_ = std::thread(&GuidanceService::guidanceProcessorThread, this);
        
        is_running_ = true;
        
        std::cout << "GuidanceService started successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start GuidanceService: " << e.what() << std::endl;
        return false;
    }
}

void GuidanceService::stop() {
    if (!is_running_) {
        return;
    }
    
    should_stop_ = true;
    guidance_condition_.notify_all();
    
    // Wait for threads to finish
    if (ipc_thread_.joinable()) {
        ipc_thread_.join();
    }
    if (guidance_thread_.joinable()) {
        guidance_thread_.join();
    }
    
    is_running_ = false;
    
    std::cout << "GuidanceService stopped" << std::endl;
}

void GuidanceService::setActiveRoute(const Route& route) {
    std::lock_guard<std::mutex> lock(route_mutex_);
    active_route_ = route;
    has_active_route_ = true;
    
    route_monitor_->setRoute(route);
    
    std::cout << "Active route set with " << route.node_count << " waypoints" << std::endl;
    
    // Wake up guidance thread
    guidance_condition_.notify_one();
}

void GuidanceService::clearActiveRoute() {
    std::lock_guard<std::mutex> lock(route_mutex_);
    has_active_route_ = false;
    route_monitor_->reset();
    
    std::cout << "Active route cleared" << std::endl;
}

GuidanceInstruction GuidanceService::getCurrentInstruction() const {
    std::lock_guard<std::mutex> lock(guidance_mutex_);
    return current_instruction_;
}

bool GuidanceService::subscribeToPositioning(int positioning_channel) {
    positioning_channel_ = positioning_channel;
    
#ifdef __QNX__
    // Send subscription message to positioning service
    SubscriptionMsg sub_msg;
    sub_msg.header.type = MessageType::SUBSCRIBE_POSITION;
    sub_msg.subscriber_pid = getpid();
    sub_msg.update_interval_ms = 200; // 5 Hz updates
    
    NavMessage reply;
    int status = MsgSend(positioning_channel_, &sub_msg, sizeof(sub_msg),
                        &reply, sizeof(reply));
    
    if (status == -1) {
        std::cerr << "Failed to subscribe to positioning service" << std::endl;
        return false;
    }
    
    std::cout << "Subscribed to positioning service" << std::endl;
    return true;
#else
    std::cout << "Subscribed to positioning service (simulation mode)" << std::endl;
    return true;
#endif
}

void GuidanceService::setRoutingServiceChannel(int routing_channel) {
    routing_channel_ = routing_channel;
    std::cout << "Connected to routing service channel: " << routing_channel << std::endl;
}

void GuidanceService::ipcServerThread() {
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
                std::cerr << "GuidanceService MsgReceive error: " << strerror(errno) << std::endl;
            }
        }
    }
#else
    // Simulation mode - simulate position updates
    while (!should_stop_) {
        if (has_active_route_) {
            // Simulate moving along the route
            static int sim_segment = 0;
            std::lock_guard<std::mutex> lock(route_mutex_);
            
            if (sim_segment < active_route_.node_count - 1) {
                // Create fake position update
                PositionUpdateMsg pos_msg;
                pos_msg.current_position = Point(21.0 + sim_segment * 0.001, 
                                               105.8 + sim_segment * 0.001);
                pos_msg.heading_degrees = 45.0;
                pos_msg.speed_kmh = 50.0;
                pos_msg.gps_valid = true;
                pos_msg.timestamp_ms = NavUtils::getCurrentTimestampMs();
                
                handlePositionUpdate(pos_msg);
                
                sim_segment++;
                if (sim_segment >= active_route_.node_count) {
                    sim_segment = 0;
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
#endif
}

void GuidanceService::guidanceProcessorThread() {
    while (!should_stop_) {
        std::unique_lock<std::mutex> lock(guidance_mutex_);
        
        // Wait for guidance updates or route changes
        guidance_condition_.wait(lock, [this]() {
            return should_stop_ || has_current_position_;
        });
        
        if (should_stop_) {
            break;
        }
        
        if (has_current_position_ && has_active_route_) {
            Point position;
            double heading;
            
            {
                std::lock_guard<std::mutex> pos_lock(position_mutex_);
                position = current_position_;
                heading = current_heading_;
            }
            
            lock.unlock();
            updateGuidance(position, heading);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(GUIDANCE_UPDATE_INTERVAL_MS));
    }
}

void GuidanceService::handleIpcMessage(const NavMessage& message, int reply_channel) {
    switch (message.header.type) {
        case MessageType::POSITION_UPDATE:
            handlePositionUpdate(message.position_update);
            break;
            
        case MessageType::SET_ROUTE:
            handleSetRoute(message.set_route, reply_channel);
            break;
            
        case MessageType::REQUEST_GUIDANCE: {
            GuidanceUpdateMsg guidance_msg;
            {
                std::lock_guard<std::mutex> lock(guidance_mutex_);
                guidance_msg.instruction = current_instruction_;
            }
            
            if (has_active_route_) {
                guidance_msg.distance_to_destination_meters = route_monitor_->getRemainingDistance();
                guidance_msg.time_to_destination_seconds = route_monitor_->getEstimatedTimeToDestination();
                guidance_msg.route_active = true;
            } else {
                guidance_msg.distance_to_destination_meters = 0.0;
                guidance_msg.time_to_destination_seconds = 0.0;
                guidance_msg.route_active = false;
            }
            
#ifdef __QNX__
            MsgReply(reply_channel, EOK, &guidance_msg, sizeof(guidance_msg));
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

void GuidanceService::handlePositionUpdate(const PositionUpdateMsg& position_msg) {
    {
        std::lock_guard<std::mutex> lock(position_mutex_);
        current_position_ = position_msg.current_position;
        current_heading_ = position_msg.heading_degrees;
        has_current_position_ = true;
    }
    
    // Wake up guidance processor
    guidance_condition_.notify_one();
}

void GuidanceService::handleSetRoute(const SetRouteMsg& route_msg, int reply_channel) {
    setActiveRoute(route_msg.route);
    
    // Send acknowledgment
    NavMessage reply;
    reply.header.type = MessageType::SERVICE_READY;
    reply.header.error_code = static_cast<int32_t>(NavError::SUCCESS);
    
#ifdef __QNX__
    MsgReply(reply_channel, EOK, &reply, sizeof(reply.header));
#endif
}

void GuidanceService::updateGuidance(const Point& current_position, double heading) {
    if (!has_active_route_) {
        return;
    }
    
    // Update route monitoring
    bool position_updated = route_monitor_->updatePosition(current_position, heading);
    
    if (!position_updated) {
        return; // No significant change
    }
    
    // Check if off route
    if (route_monitor_->isOffRoute()) {
        uint64_t current_time = NavUtils::getCurrentTimestampMs();
        
        // Request reroute if cooldown period has passed
        if (current_time - last_reroute_request_time_ > REROUTE_REQUEST_COOLDOWN_MS) {
            Point destination;
            {
                std::lock_guard<std::mutex> lock(route_mutex_);
                if (active_route_.node_count > 0) {
                    // For simplicity, use the last route point as destination
                    // In reality, you'd need to look up the node position
                    destination = current_position; // Placeholder
                }
            }
            
            requestReroute(current_position, destination);
            last_reroute_request_time_ = current_time;
        }
        
        return;
    }
    
    // Generate next instruction
    Route route_copy;
    {
        std::lock_guard<std::mutex> lock(route_mutex_);
        route_copy = active_route_;
    }
    
    int current_segment = route_monitor_->getCurrentSegmentIndex();
    GuidanceInstruction new_instruction = instruction_generator_->generateNextInstruction(
        route_copy, current_segment, current_position);
    
    // Update current instruction
    {
        std::lock_guard<std::mutex> lock(guidance_mutex_);
        current_instruction_ = new_instruction;
        last_guidance_update_ = NavUtils::getCurrentTimestampMs();
    }
    
    // Send to instrument cluster
    sendToInstrumentCluster(new_instruction);
    
    // Broadcast to subscribers
    broadcastGuidanceUpdate();
    
    std::cout << "Guidance updated: " << new_instruction.instruction_text << std::endl;
}

void GuidanceService::broadcastGuidanceUpdate() {
    GuidanceUpdateMsg guidance_msg;
    
    {
        std::lock_guard<std::mutex> lock(guidance_mutex_);
        guidance_msg.instruction = current_instruction_;
    }
    
    if (has_active_route_) {
        guidance_msg.distance_to_destination_meters = route_monitor_->getRemainingDistance();
        guidance_msg.time_to_destination_seconds = route_monitor_->getEstimatedTimeToDestination();
        guidance_msg.current_route_node = route_monitor_->getCurrentSegmentIndex();
        guidance_msg.route_active = true;
    } else {
        guidance_msg.distance_to_destination_meters = 0.0;
        guidance_msg.time_to_destination_seconds = 0.0;
        guidance_msg.current_route_node = 0;
        guidance_msg.route_active = false;
    }
    
    // Send to all subscribers
    std::lock_guard<std::mutex> lock(subscribers_mutex_);
    for (uint32_t subscriber_pid : subscribers_) {
#ifdef __QNX__
        // Send IPC message to subscriber
        // Implementation would use MsgSend
#else
        std::cout << "Broadcasting guidance to PID " << subscriber_pid << std::endl;
#endif
    }
}

void GuidanceService::requestReroute(const Point& current_position, const Point& destination) {
    if (routing_channel_ < 0) {
        std::cerr << "Cannot request reroute: no routing service connection" << std::endl;
        return;
    }
    
    std::cout << "Requesting reroute from current position to destination" << std::endl;
    
#ifdef __QNX__
    RouteRequestMsg reroute_msg;
    reroute_msg.start_point = current_position;
    reroute_msg.end_point = destination;
    reroute_msg.route_preferences = 0; // Fastest route
    
    RouteResponseMsg response;
    int status = MsgSend(routing_channel_, &reroute_msg, sizeof(reroute_msg),
                        &response, sizeof(response));
    
    if (status != -1 && response.header.error_code == static_cast<int32_t>(NavError::SUCCESS)) {
        // Update route with new calculation
        setActiveRoute(response.route);
        std::cout << "Reroute successful" << std::endl;
    } else {
        std::cerr << "Reroute failed" << std::endl;
    }
#endif
}

void GuidanceService::sendToInstrumentCluster(const GuidanceInstruction& instruction) {
    if (can_interface_->isConnected()) {
        can_interface_->sendGuidanceData(instruction);
    }
}

} // namespace nav