#include "string_matcher.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace geocoding {
namespace fuzzy {

int StringMatcher::levenshteinDistance(const std::string& s1, const std::string& s2) {
    const size_t len1 = s1.size();
    const size_t len2 = s2.size();
    
    // Create distance matrix
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));
    
    // Initialize first column and row
    for (size_t i = 0; i <= len1; ++i) {
        dp[i][0] = static_cast<int>(i);
    }
    for (size_t j = 0; j <= len2; ++j) {
        dp[0][j] = static_cast<int>(j);
    }
    
    // Fill the matrix
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (std::tolower(s1[i-1]) == std::tolower(s2[j-1])) ? 0 : 1;
            
            dp[i][j] = std::min({
                dp[i-1][j] + 1,      // deletion
                dp[i][j-1] + 1,      // insertion
                dp[i-1][j-1] + cost  // substitution
            });
        }
    }
    
    return dp[len1][len2];
}

double StringMatcher::similarityScore(const std::string& s1, const std::string& s2) {
    if (s1.empty() && s2.empty()) {
        return 1.0;
    }
    if (s1.empty() || s2.empty()) {
        return 0.0;
    }
    
    int distance = levenshteinDistance(s1, s2);
    int max_len = static_cast<int>(std::max(s1.length(), s2.length()));
    
    // Normalize to 0.0 - 1.0 range
    return 1.0 - (static_cast<double>(distance) / max_len);
}

double StringMatcher::jaroSimilarity(const std::string& s1, const std::string& s2) {
    if (s1.empty() && s2.empty()) {
        return 1.0;
    }
    if (s1.empty() || s2.empty()) {
        return 0.0;
    }
    
    size_t len1 = s1.length();
    size_t len2 = s2.length();
    
    // Maximum allowed distance for matching
    int max_dist = static_cast<int>(std::max(len1, len2) / 2) - 1;
    max_dist = std::max(max_dist, 0);
    
    // Arrays to track matched characters
    std::vector<bool> s1_matches(len1, false);
    std::vector<bool> s2_matches(len2, false);
    
    int matches = 0;
    int transpositions = 0;
    
    // Find matches
    for (size_t i = 0; i < len1; ++i) {
        int start = std::max(0, static_cast<int>(i) - max_dist);
        int end = std::min(static_cast<int>(i) + max_dist + 1, static_cast<int>(len2));
        
        for (int j = start; j < end; ++j) {
            if (s2_matches[j] || std::tolower(s1[i]) != std::tolower(s2[j])) {
                continue;
            }
            s1_matches[i] = true;
            s2_matches[j] = true;
            ++matches;
            break;
        }
    }
    
    if (matches == 0) {
        return 0.0;
    }
    
    // Count transpositions
    int k = 0;
    for (size_t i = 0; i < len1; ++i) {
        if (!s1_matches[i]) continue;
        
        while (!s2_matches[k]) ++k;
        
        if (std::tolower(s1[i]) != std::tolower(s2[k])) {
            ++transpositions;
        }
        ++k;
    }
    
    double m = static_cast<double>(matches);
    return (m / len1 + m / len2 + (m - transpositions / 2.0) / m) / 3.0;
}

double StringMatcher::jaroWinklerSimilarity(const std::string& s1, const std::string& s2) {
    double jaro = jaroSimilarity(s1, s2);
    
    // Jaro-Winkler adds bonus for common prefix
    int prefix_len = commonPrefixLength(s1, s2, 4);
    
    const double prefix_scale = 0.1;
    return jaro + (prefix_len * prefix_scale * (1.0 - jaro));
}

int StringMatcher::commonPrefixLength(const std::string& s1, const std::string& s2, int maxLength) {
    int prefix = 0;
    int min_len = std::min({static_cast<int>(s1.length()), 
                            static_cast<int>(s2.length()), 
                            maxLength});
    
    for (int i = 0; i < min_len; ++i) {
        if (std::tolower(s1[i]) == std::tolower(s2[i])) {
            ++prefix;
        } else {
            break;
        }
    }
    
    return prefix;
}

std::string StringMatcher::normalize(const std::string& str) {
    std::string result;
    result.reserve(str.length());
    
    // Convert to lowercase and remove extra spaces
    bool last_was_space = true;
    for (char c : str) {
        if (std::isspace(c)) {
            if (!last_was_space && !result.empty()) {
                result += ' ';
                last_was_space = true;
            }
        } else {
            result += std::tolower(c);
            last_was_space = false;
        }
    }
    
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

std::string StringMatcher::getPhoneticCode(const std::string& str) {
    if (str.empty()) {
        return "";
    }
    
    std::string normalized = normalize(str);
    std::string code;
    code.reserve(4);
    
    // First character
    code += std::toupper(normalized[0]);
    
    // Map consonants to digits (Soundex-like)
    auto getCode = [](char c) -> char {
        switch (std::tolower(c)) {
            case 'b': case 'f': case 'p': case 'v': return '1';
            case 'c': case 'g': case 'j': case 'k': case 'q': case 's': case 'x': case 'z': return '2';
            case 'd': case 't': return '3';
            case 'l': return '4';
            case 'm': case 'n': return '5';
            case 'r': return '6';
            default: return '0';
        }
    };
    
    char last_code = getCode(normalized[0]);
    for (size_t i = 1; i < normalized.length() && code.length() < 4; ++i) {
        char curr_code = getCode(normalized[i]);
        if (curr_code != '0' && curr_code != last_code) {
            code += curr_code;
            last_code = curr_code;
        }
    }
    
    // Pad with zeros
    while (code.length() < 4) {
        code += '0';
    }
    
    return code;
}

bool StringMatcher::isPhoneticMatch(const std::string& s1, const std::string& s2) {
    // Check for common abbreviations
    std::string norm1 = normalize(s1);
    std::string norm2 = normalize(s2);
    
    // Common street abbreviations
    static const std::vector<std::pair<std::string, std::string>> abbreviations = {
        {"street", "st"},
        {"avenue", "ave"},
        {"road", "rd"},
        {"boulevard", "blvd"},
        {"drive", "dr"},
        {"lane", "ln"},
        {"court", "ct"},
        {"place", "pl"},
        {"apartment", "apt"},
        {"building", "bldg"},
        {"north", "n"},
        {"south", "s"},
        {"east", "e"},
        {"west", "w"}
    };
    
    for (const auto& [full, abbr] : abbreviations) {
        if ((norm1 == full && norm2 == abbr) || (norm1 == abbr && norm2 == full)) {
            return true;
        }
    }
    
    // Phonetic code comparison
    std::string code1 = getPhoneticCode(s1);
    std::string code2 = getPhoneticCode(s2);
    
    return code1 == code2;
}

}} // namespace geocoding::fuzzy
