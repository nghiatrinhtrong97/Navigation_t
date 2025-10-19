#include "fuzzy_address_matcher.h"
#include "address_components.h"  // For AddressComponents definition
#include <algorithm>
#include <numeric>

namespace geocoding {
namespace fuzzy {

FuzzyAddressMatcher::FuzzyAddressMatcher(const FuzzyMatchConfig& config)
    : m_config(config) {
}

ComponentMatchScore FuzzyAddressMatcher::calculateMatch(
    const nav::geocoding::AddressComponents& query,
    const nav::geocoding::AddressComponents& candidate) {
    
    ComponentMatchScore score;
    
    // Match each component
    score.street_number_score = matchStreetNumber(
        query.house_number.value_or(""), 
        candidate.house_number.value_or(""));
    score.street_name_score = matchStreetName(
        query.street_name.value_or(""), 
        candidate.street_name.value_or(""));
    score.city_score = matchCity(
        query.city.value_or(""), 
        candidate.city.value_or(""));
    score.district_score = matchDistrict(
        query.sublocality.value_or(""), 
        candidate.sublocality.value_or(""));
    
    // Check phonetic matching
    score.is_phonetic_match = StringMatcher::isPhoneticMatch(
        query.street_name.value_or(""), 
        candidate.street_name.value_or(""));
    
    // Calculate weighted overall score
    score.overall_score = calculateOverallScore(score);
    
    return score;
}

std::vector<std::pair<nav::geocoding::AddressComponents, ComponentMatchScore>>
FuzzyAddressMatcher::findBestMatches(
    const nav::geocoding::AddressComponents& query,
    const std::vector<nav::geocoding::AddressComponents>& candidates,
    size_t max_results) {
    
    std::vector<std::pair<nav::geocoding::AddressComponents, ComponentMatchScore>> matches;
    matches.reserve(candidates.size());
    
    // Calculate match scores for all candidates
    for (const auto& candidate : candidates) {
        ComponentMatchScore score = calculateMatch(query, candidate);
        
        // Only include acceptable matches
        if (isAcceptableMatch(score)) {
            matches.emplace_back(candidate, score);
        }
    }
    
    // Sort by overall score (descending)
    std::sort(matches.begin(), matches.end(),
        [](const auto& a, const auto& b) {
            return a.second.overall_score > b.second.overall_score;
        });
    
    // Return top N results
    if (matches.size() > max_results) {
        matches.resize(max_results);
    }
    
    return matches;
}

bool FuzzyAddressMatcher::isAcceptableMatch(const ComponentMatchScore& score) const {
    return score.overall_score >= m_config.min_overall_score &&
           score.street_name_score >= m_config.min_street_name_score;
}

double FuzzyAddressMatcher::matchStreetNumber(const std::string& query, const std::string& candidate) {
    if (query.empty() && candidate.empty()) {
        return 1.0;
    }
    if (query.empty() || candidate.empty()) {
        return 0.5;  // Partial credit for missing number
    }
    
    std::string norm_query = StringMatcher::normalize(query);
    std::string norm_candidate = StringMatcher::normalize(candidate);
    
    // Exact match
    if (norm_query == norm_candidate) {
        return 1.0;
    }
    
    // Try to parse as numbers
    try {
        int num_query = std::stoi(norm_query);
        int num_candidate = std::stoi(norm_candidate);
        
        // Close numbers (within 2)
        int diff = std::abs(num_query - num_candidate);
        if (diff == 0) return 1.0;
        if (diff == 1) return 0.9;
        if (diff == 2) return 0.8;
        return 0.6;  // Different numbers
    } catch (...) {
        // Not numbers, use fuzzy string matching
        return StringMatcher::similarityScore(norm_query, norm_candidate);
    }
}

double FuzzyAddressMatcher::matchStreetName(const std::string& query, const std::string& candidate) {
    if (query.empty() || candidate.empty()) {
        return 0.0;
    }
    
    std::string norm_query = StringMatcher::normalize(query);
    std::string norm_candidate = StringMatcher::normalize(candidate);
    
    // Exact match
    if (norm_query == norm_candidate) {
        return 1.0;
    }
    
    // Phonetic match (e.g., "Street" vs "St")
    if (m_config.use_phonetic_matching && 
        StringMatcher::isPhoneticMatch(norm_query, norm_candidate)) {
        return 0.95;
    }
    
    // Levenshtein distance
    int distance = StringMatcher::levenshteinDistance(norm_query, norm_candidate);
    if (distance > m_config.max_edit_distance) {
        return 0.0;  // Too different
    }
    
    // Use Jaro-Winkler for street names (better for short strings)
    double jaro_winkler = StringMatcher::jaroWinklerSimilarity(norm_query, norm_candidate);
    
    // Use regular similarity as backup
    double levenshtein = StringMatcher::similarityScore(norm_query, norm_candidate);
    
    // Take the best score
    return std::max(jaro_winkler, levenshtein);
}

double FuzzyAddressMatcher::matchCity(const std::string& query, const std::string& candidate) {
    if (query.empty() && candidate.empty()) {
        return 1.0;
    }
    if (query.empty() || candidate.empty()) {
        return 0.3;  // Partial credit
    }
    
    std::string norm_query = StringMatcher::normalize(query);
    std::string norm_candidate = StringMatcher::normalize(candidate);
    
    // Exact match
    if (norm_query == norm_candidate) {
        return 1.0;
    }
    
    // Substring match (e.g., "Ho Chi Minh" contains "Minh")
    if (norm_candidate.find(norm_query) != std::string::npos ||
        norm_query.find(norm_candidate) != std::string::npos) {
        return 0.85;
    }
    
    // Fuzzy match with tolerance
    double similarity = StringMatcher::jaroWinklerSimilarity(norm_query, norm_candidate);
    return similarity;
}

double FuzzyAddressMatcher::matchDistrict(const std::string& query, const std::string& candidate) {
    if (query.empty() && candidate.empty()) {
        return 1.0;
    }
    if (query.empty() || candidate.empty()) {
        return 0.4;  // Partial credit
    }
    
    std::string norm_query = StringMatcher::normalize(query);
    std::string norm_candidate = StringMatcher::normalize(candidate);
    
    // Exact match
    if (norm_query == norm_candidate) {
        return 1.0;
    }
    
    // Extract district numbers (e.g., "District 1" -> "1")
    auto extractNumber = [](const std::string& str) -> std::string {
        std::string result;
        for (char c : str) {
            if (std::isdigit(c)) {
                result += c;
            }
        }
        return result;
    };
    
    std::string query_num = extractNumber(norm_query);
    std::string candidate_num = extractNumber(norm_candidate);
    
    if (!query_num.empty() && query_num == candidate_num) {
        return 0.95;  // Same district number
    }
    
    // Fuzzy match
    double similarity = StringMatcher::jaroWinklerSimilarity(norm_query, norm_candidate);
    return similarity;
}

double FuzzyAddressMatcher::calculateOverallScore(const ComponentMatchScore& scores) {
    return scores.street_number_score * m_config.street_number_weight +
           scores.street_name_score * m_config.street_name_weight +
           scores.city_score * m_config.city_weight +
           scores.district_score * m_config.district_weight;
}

}} // namespace geocoding::fuzzy
