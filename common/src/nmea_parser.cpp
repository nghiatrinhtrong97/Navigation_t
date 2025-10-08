#include "nav_utils.h"
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace nav {

bool NmeaParser::parseNmeaSentence(const std::string& sentence, GpsData& gps_data) {
    if (sentence.empty() || sentence[0] != '$') {
        return false;
    }
    
    // Validate checksum
    if (!validateChecksum(sentence)) {
        return false;
    }
    
    // Extract sentence type
    size_t comma_pos = sentence.find(',');
    if (comma_pos == std::string::npos) {
        return false;
    }
    
    std::string sentence_type = sentence.substr(1, comma_pos - 1);
    
    if (sentence_type == "GPRMC") {
        return parseGPRMC(sentence, gps_data);
    } else if (sentence_type == "GPGGA") {
        return parseGPGGA(sentence, gps_data);
    }
    
    return false;
}

bool NmeaParser::parseGPRMC(const std::string& sentence, GpsData& gps_data) {
    // $GPRMC,time,status,lat,lat_dir,lon,lon_dir,speed,course,date,mag_var,mag_var_dir,checksum
    auto fields = splitString(sentence, ',');
    if (fields.size() < 12) {
        return false;
    }
    
    // Check status
    if (fields[2] != "A") {
        gps_data.valid = false;
        return false;
    }
    
    gps_data.valid = true;
    
    // Parse latitude
    if (!fields[3].empty() && !fields[4].empty()) {
        gps_data.position.latitude = parseCoordinate(fields[3], fields[4][0]);
    }
    
    // Parse longitude
    if (!fields[5].empty() && !fields[6].empty()) {
        gps_data.position.longitude = parseCoordinate(fields[5], fields[6][0]);
    }
    
    // Parse speed (knots to km/h)
    if (!fields[7].empty()) {
        double speed_knots = std::stod(fields[7]);
        gps_data.speed_kmh = speed_knots * 1.852;
    }
    
    // Parse course
    if (!fields[8].empty()) {
        gps_data.course_degrees = std::stod(fields[8]);
    }
    
    gps_data.timestamp_ms = NavUtils::getCurrentTimestampMs();
    
    return true;
}

bool NmeaParser::parseGPGGA(const std::string& sentence, GpsData& gps_data) {
    // $GPGGA,time,lat,lat_dir,lon,lon_dir,quality,satellites,hdop,altitude,alt_unit,geoid_height,geoid_unit,dgps_time,dgps_id,checksum
    auto fields = splitString(sentence, ',');
    if (fields.size() < 15) {
        return false;
    }
    
    // Check quality
    if (fields[6].empty() || fields[6] == "0") {
        gps_data.valid = false;
        return false;
    }
    
    gps_data.valid = true;
    
    // Parse latitude
    if (!fields[2].empty() && !fields[3].empty()) {
        gps_data.position.latitude = parseCoordinate(fields[2], fields[3][0]);
    }
    
    // Parse longitude
    if (!fields[4].empty() && !fields[5].empty()) {
        gps_data.position.longitude = parseCoordinate(fields[4], fields[5][0]);
    }
    
    // Parse satellites
    if (!fields[7].empty()) {
        gps_data.satellites_used = static_cast<uint8_t>(std::stoi(fields[7]));
    }
    
    // Parse HDOP
    if (!fields[8].empty()) {
        gps_data.hdop = std::stod(fields[8]);
    }
    
    // Parse altitude
    if (!fields[9].empty()) {
        gps_data.position.altitude = std::stod(fields[9]);
    }
    
    gps_data.timestamp_ms = NavUtils::getCurrentTimestampMs();
    
    return true;
}

std::vector<std::string> NmeaParser::splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

double NmeaParser::parseCoordinate(const std::string& coord_str, char direction) {
    if (coord_str.empty()) {
        return 0.0;
    }
    
    // Format: DDMM.MMMMM (latitude) or DDDMM.MMMMM (longitude)
    double coord = std::stod(coord_str);
    
    // Extract degrees and minutes
    int degrees = static_cast<int>(coord / 100);
    double minutes = coord - (degrees * 100);
    
    // Convert to decimal degrees
    double decimal_degrees = degrees + (minutes / 60.0);
    
    // Apply direction
    if (direction == 'S' || direction == 'W') {
        decimal_degrees = -decimal_degrees;
    }
    
    return decimal_degrees;
}

uint8_t NmeaParser::calculateChecksum(const std::string& sentence) {
    uint8_t checksum = 0;
    
    // Start after '$' and end before '*'
    size_t start = 1;
    size_t end = sentence.find('*');
    if (end == std::string::npos) {
        end = sentence.length();
    }
    
    for (size_t i = start; i < end; ++i) {
        checksum ^= static_cast<uint8_t>(sentence[i]);
    }
    
    return checksum;
}

bool NmeaParser::validateChecksum(const std::string& sentence) {
    size_t star_pos = sentence.find('*');
    if (star_pos == std::string::npos || star_pos + 3 > sentence.length()) {
        return false; // No checksum present
    }
    
    // Extract checksum from sentence
    std::string checksum_str = sentence.substr(star_pos + 1, 2);
    uint8_t expected_checksum = static_cast<uint8_t>(std::stoi(checksum_str, nullptr, 16));
    
    // Calculate actual checksum
    uint8_t actual_checksum = calculateChecksum(sentence);
    
    return actual_checksum == expected_checksum;
}

} // namespace nav