#include "guidance_service.h"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace nav {

TurnByTurnGenerator::TurnByTurnGenerator() {
}

void TurnByTurnGenerator::setMapData(const std::vector<MapNode>& nodes, 
                                    const std::vector<MapEdge>& edges) {
    node_lookup_.clear();
    edge_lookup_.clear();
    
    // Build node lookup
    for (const auto& node : nodes) {
        node_lookup_[node.id] = node;
    }
    
    // Build edge lookup
    for (const auto& edge : edges) {
        edge_lookup_[edge.from_node].push_back(edge);
    }
}

GuidanceInstruction TurnByTurnGenerator::generateNextInstruction(const Route& route, 
                                                                int current_segment_index,
                                                                const Point& current_position) {
    GuidanceInstruction instruction;
    
    if (current_segment_index >= route.node_count - 1) {
        // Reached destination
        instruction.turn_type = TurnType::DESTINATION_REACHED;
        instruction.distance_to_turn_meters = 0.0;
        instruction.target_node_id = route.nodes[route.node_count - 1];
        strcpy(instruction.instruction_text, "You have reached your destination");
        return instruction;
    }
    
    // Find upcoming maneuver
    int maneuver_index;
    TurnType turn_type;
    
    if (getUpcomingManeuver(route, current_segment_index, maneuver_index, turn_type)) {
        instruction.turn_type = turn_type;
        instruction.target_node_id = route.nodes[maneuver_index];
        
        // Calculate distance to maneuver
        // For simplification, using direct distance
        // In reality, this would sum distances along route segments
        if (maneuver_index < route.node_count) {
            // Dummy position calculation
            Point maneuver_point(21.0 + maneuver_index * 0.001, 105.8 + maneuver_index * 0.001);
            instruction.distance_to_turn_meters = NavUtils::haversineDistance(
                current_position, maneuver_point);
        } else {
            instruction.distance_to_turn_meters = 0.0;
        }
        
        // Generate instruction text
        std::string text = generateInstructionText(instruction);
        strncpy(instruction.instruction_text, text.c_str(), 
                sizeof(instruction.instruction_text) - 1);
        instruction.instruction_text[sizeof(instruction.instruction_text) - 1] = '\0';
        
    } else {
        // Continue straight
        instruction.turn_type = TurnType::STRAIGHT;
        instruction.distance_to_turn_meters = 0.0;
        instruction.target_node_id = route.nodes[current_segment_index + 1];
        strcpy(instruction.instruction_text, "Continue straight");
    }
    
    return instruction;
}

std::string TurnByTurnGenerator::generateInstructionText(const GuidanceInstruction& instruction) const {
    std::ostringstream oss;
    
    std::string turn_text = NavUtils::turnTypeToString(instruction.turn_type);
    
    if (instruction.turn_type == TurnType::DESTINATION_REACHED) {
        return turn_text;
    }
    
    if (instruction.distance_to_turn_meters <= IMMEDIATE_INSTRUCTION_DISTANCE) {
        oss << turn_text << " now";
    } else if (instruction.distance_to_turn_meters <= NEAR_INSTRUCTION_DISTANCE) {
        oss << turn_text << " in " << static_cast<int>(instruction.distance_to_turn_meters) << " meters";
    } else if (instruction.distance_to_turn_meters <= FAR_INSTRUCTION_DISTANCE) {
        oss << turn_text << " in " << static_cast<int>(instruction.distance_to_turn_meters) << " meters";
    } else {
        // Far ahead - give general direction
        oss << "Continue for " << NavUtils::formatDistance(instruction.distance_to_turn_meters);
    }
    
    return oss.str();
}

TurnType TurnByTurnGenerator::determineTurnType(const Point& from, const Point& via, const Point& to) const {
    double bearing1 = NavUtils::calculateBearing(from, via);
    double bearing2 = NavUtils::calculateBearing(via, to);
    
    double bearing_change = calculateBearingChange(bearing1, bearing2);
    
    // Classify turn based on bearing change
    if (std::abs(bearing_change) < 15.0) {
        return TurnType::STRAIGHT;
    } else if (bearing_change >= 15.0 && bearing_change < 45.0) {
        return TurnType::SLIGHT_RIGHT;
    } else if (bearing_change >= 45.0 && bearing_change < 135.0) {
        return TurnType::RIGHT;
    } else if (bearing_change >= 135.0) {
        return TurnType::SHARP_RIGHT;
    } else if (bearing_change <= -15.0 && bearing_change > -45.0) {
        return TurnType::SLIGHT_LEFT;
    } else if (bearing_change <= -45.0 && bearing_change > -135.0) {
        return TurnType::LEFT;
    } else if (bearing_change <= -135.0) {
        return TurnType::SHARP_LEFT;
    }
    
    return TurnType::STRAIGHT;
}

double TurnByTurnGenerator::calculateBearingChange(double from_bearing, double to_bearing) const {
    double change = to_bearing - from_bearing;
    
    // Normalize to [-180, 180]
    while (change > 180.0) change -= 360.0;
    while (change < -180.0) change += 360.0;
    
    return change;
}

bool TurnByTurnGenerator::getUpcomingManeuver(const Route& route, int current_index, 
                                             int& maneuver_index, TurnType& turn_type) const {
    // Look ahead for significant direction changes
    for (int i = current_index + 1; i < route.node_count - 2; ++i) {
        // Create dummy points for calculation
        Point from(21.0 + (i-1) * 0.001, 105.8 + (i-1) * 0.001);
        Point via(21.0 + i * 0.001, 105.8 + i * 0.001);
        Point to(21.0 + (i+1) * 0.001, 105.8 + (i+1) * 0.001);
        
        TurnType detected_turn = determineTurnType(from, via, to);
        
        if (detected_turn != TurnType::STRAIGHT) {
            maneuver_index = i;
            turn_type = detected_turn;
            return true;
        }
    }
    
    return false;
}

} // namespace nav