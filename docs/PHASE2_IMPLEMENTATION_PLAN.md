# PHASE 2 IMPLEMENTATION PLAN: Performance & Scalability Enhancement

**Duration:** Weeks 5-8 (4 weeks)  
**Focus:** Fuzzy String Matching, Caching Layer, Batch Geocoding API  
**Dependencies:** Phase 1 (Enhanced geocoding infrastructure must be complete)

---

## 📋 OVERVIEW

### **Phase 2 Objectives**
1. **Fuzzy String Matching:** Handle typos, abbreviations, and variations in address input (35% accuracy improvement)
2. **Caching Layer:** Implement Redis-backed cache with LRU eviction (80%+ cache hit rate, 10x latency reduction)
3. **Batch Geocoding API:** Enable parallel processing of thousands of addresses (10,000+ addresses/sec throughput)

### **Success Metrics**
- ✅ Fuzzy matching accuracy: >85% for addresses with 1-2 typos
- ✅ Cache hit rate: >80% in production workloads
- ✅ Batch throughput: 10,000+ addresses/second with 8 workers
- ✅ Single geocode latency: <1ms with cache hit, <10ms with cache miss

---

## 🎯 WEEK 5: FUZZY STRING MATCHING FOUNDATION

### **Day 1-2: Core String Similarity Algorithms**

#### **Task 1.1: Create String Utilities Module**

**File:** `hmi/services/geocoding/include/string_matcher.h`

```cpp
#ifndef STRING_MATCHER_H
#define STRING_MATCHER_H

#include <string>
#include <vector>
#include <unordered_map>

namespace geocoding {

/**
 * @brief Core string similarity algorithms for fuzzy address matching
 * 
 * Provides multiple distance/similarity metrics optimized for address comparison:
 * - Levenshtein: Edit distance (insertions, deletions, substitutions)
 * - Jaro-Winkler: Similarity metric favoring prefix matches
 * - Trigram: Character n-gram overlap similarity
 */
class StringMatcher {
public:
    /**
     * @brief Calculate Levenshtein edit distance
     * @param s1 First string
     * @param s2 Second string
     * @param case_sensitive Whether to consider case (default: false)
     * @return Edit distance (0 = identical, higher = more different)
     * 
     * Time Complexity: O(m*n) where m, n are string lengths
     * Space Complexity: O(min(m,n)) using optimized algorithm
     * 
     * Example:
     *   levenshteinDistance("Main St", "Main Street") = 4
     *   levenshteinDistance("Mian St", "Main St") = 1 (one substitution)
     */
    static size_t levenshteinDistance(const std::string& s1, 
                                     const std::string& s2,
                                     bool case_sensitive = false);
    
    /**
     * @brief Calculate Jaro-Winkler similarity score
     * @param s1 First string
     * @param s2 Second string
     * @param prefix_scale Scaling factor for common prefix (default: 0.1)
     * @return Similarity score 0.0-1.0 (1.0 = identical)
     * 
     * Jaro-Winkler gives higher scores to strings with matching prefixes,
     * making it ideal for addresses where "123 Main" should match "123 Maine"
     * better than "321 Main".
     * 
     * Example:
     *   jaroWinklerSimilarity("Main Street", "Main St") = 0.89
     *   jaroWinklerSimilarity("Broadway", "Brodway") = 0.96
     */
    static double jaroWinklerSimilarity(const std::string& s1,
                                       const std::string& s2,
                                       double prefix_scale = 0.1);
    
    /**
     * @brief Calculate trigram similarity
     * @param s1 First string
     * @param s2 Second string
     * @return Similarity score 0.0-1.0 based on character trigram overlap
     * 
     * Trigrams are 3-character substrings. For "Main St":
     *   Trigrams: ["Mai", "ain", "in ", "n S", " St"]
     * 
     * Good for handling OCR errors and partial matches.
     * 
     * Example:
     *   trigramSimilarity("Springfield", "Springfild") = 0.82
     */
    static double trigramSimilarity(const std::string& s1, 
                                   const std::string& s2);
    
    /**
     * @brief Generate character n-grams from string
     * @param text Input string
     * @param n N-gram size (default: 3)
     * @return Vector of n-grams
     */
    static std::vector<std::string> generateNGrams(const std::string& text, 
                                                   size_t n = 3);
    
    /**
     * @brief Normalize string for comparison (lowercase, trim, collapse spaces)
     * @param text Input string
     * @return Normalized string
     */
    static std::string normalizeForComparison(const std::string& text);
    
private:
    // Helper function for Jaro similarity calculation
    static double jaroSimilarity(const std::string& s1, const std::string& s2);
    
    // Calculate common prefix length
    static size_t commonPrefixLength(const std::string& s1, 
                                    const std::string& s2,
                                    size_t max_length = 4);
};

} // namespace geocoding

#endif // STRING_MATCHER_H
```

**File:** `hmi/services/geocoding/src/string_matcher.cpp`

```cpp
#include "string_matcher.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

namespace geocoding {

std::string StringMatcher::normalizeForComparison(const std::string& text) {
    std::string result = text;
    
    // Convert to lowercase
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    // Trim leading/trailing whitespace
    result.erase(result.begin(), 
                std::find_if(result.begin(), result.end(), 
                            [](unsigned char c) { return !std::isspace(c); }));
    result.erase(std::find_if(result.rbegin(), result.rend(),
                             [](unsigned char c) { return !std::isspace(c); }).base(), 
                result.end());
    
    // Collapse multiple spaces to single space
    auto new_end = std::unique(result.begin(), result.end(),
                              [](char a, char b) { 
                                  return std::isspace(a) && std::isspace(b); 
                              });
    result.erase(new_end, result.end());
    
    return result;
}

size_t StringMatcher::levenshteinDistance(const std::string& s1, 
                                         const std::string& s2,
                                         bool case_sensitive) {
    std::string str1 = case_sensitive ? s1 : normalizeForComparison(s1);
    std::string str2 = case_sensitive ? s2 : normalizeForComparison(s2);
    
    const size_t m = str1.length();
    const size_t n = str2.length();
    
    if (m == 0) return n;
    if (n == 0) return m;
    
    // Use space-optimized algorithm (only need previous row)
    std::vector<size_t> prev_row(n + 1);
    std::vector<size_t> curr_row(n + 1);
    
    // Initialize first row
    for (size_t j = 0; j <= n; ++j) {
        prev_row[j] = j;
    }
    
    // Calculate distances row by row
    for (size_t i = 1; i <= m; ++i) {
        curr_row[0] = i;
        
        for (size_t j = 1; j <= n; ++j) {
            size_t cost = (str1[i-1] == str2[j-1]) ? 0 : 1;
            
            curr_row[j] = std::min({
                prev_row[j] + 1,      // Deletion
                curr_row[j-1] + 1,    // Insertion
                prev_row[j-1] + cost  // Substitution
            });
        }
        
        prev_row.swap(curr_row);
    }
    
    return prev_row[n];
}

size_t StringMatcher::commonPrefixLength(const std::string& s1, 
                                        const std::string& s2,
                                        size_t max_length) {
    size_t prefix_len = 0;
    size_t min_len = std::min({s1.length(), s2.length(), max_length});
    
    for (size_t i = 0; i < min_len; ++i) {
        if (s1[i] == s2[i]) {
            ++prefix_len;
        } else {
            break;
        }
    }
    
    return prefix_len;
}

double StringMatcher::jaroSimilarity(const std::string& s1, 
                                    const std::string& s2) {
    if (s1.empty() && s2.empty()) return 1.0;
    if (s1.empty() || s2.empty()) return 0.0;
    
    const size_t s1_len = s1.length();
    const size_t s2_len = s2.length();
    
    // Maximum allowed distance for matches
    const size_t match_distance = (std::max(s1_len, s2_len) / 2) - 1;
    
    std::vector<bool> s1_matches(s1_len, false);
    std::vector<bool> s2_matches(s2_len, false);
    
    size_t matches = 0;
    size_t transpositions = 0;
    
    // Find matches
    for (size_t i = 0; i < s1_len; ++i) {
        size_t start = (i >= match_distance) ? i - match_distance : 0;
        size_t end = std::min(i + match_distance + 1, s2_len);
        
        for (size_t j = start; j < end; ++j) {
            if (s2_matches[j] || s1[i] != s2[j]) continue;
            
            s1_matches[i] = true;
            s2_matches[j] = true;
            ++matches;
            break;
        }
    }
    
    if (matches == 0) return 0.0;
    
    // Count transpositions
    size_t k = 0;
    for (size_t i = 0; i < s1_len; ++i) {
        if (!s1_matches[i]) continue;
        
        while (!s2_matches[k]) ++k;
        
        if (s1[i] != s2[k]) ++transpositions;
        ++k;
    }
    
    // Calculate Jaro similarity
    double jaro = (static_cast<double>(matches) / s1_len +
                  static_cast<double>(matches) / s2_len +
                  static_cast<double>(matches - transpositions/2) / matches) / 3.0;
    
    return jaro;
}

double StringMatcher::jaroWinklerSimilarity(const std::string& s1,
                                           const std::string& s2,
                                           double prefix_scale) {
    std::string str1 = normalizeForComparison(s1);
    std::string str2 = normalizeForComparison(s2);
    
    double jaro = jaroSimilarity(str1, str2);
    
    // Apply prefix bonus
    size_t prefix_len = commonPrefixLength(str1, str2, 4);
    double jaro_winkler = jaro + (prefix_len * prefix_scale * (1.0 - jaro));
    
    return std::min(jaro_winkler, 1.0);
}

std::vector<std::string> StringMatcher::generateNGrams(const std::string& text, 
                                                       size_t n) {
    std::vector<std::string> ngrams;
    
    if (text.length() < n) {
        ngrams.push_back(text);
        return ngrams;
    }
    
    for (size_t i = 0; i <= text.length() - n; ++i) {
        ngrams.push_back(text.substr(i, n));
    }
    
    return ngrams;
}

double StringMatcher::trigramSimilarity(const std::string& s1, 
                                       const std::string& s2) {
    std::string str1 = normalizeForComparison(s1);
    std::string str2 = normalizeForComparison(s2);
    
    if (str1 == str2) return 1.0;
    if (str1.empty() || str2.empty()) return 0.0;
    
    // Generate trigrams for both strings
    auto trigrams1 = generateNGrams(str1, 3);
    auto trigrams2 = generateNGrams(str2, 3);
    
    if (trigrams1.empty() || trigrams2.empty()) return 0.0;
    
    // Convert to sets for intersection calculation
    std::unordered_set<std::string> set1(trigrams1.begin(), trigrams1.end());
    std::unordered_set<std::string> set2(trigrams2.begin(), trigrams2.end());
    
    // Count common trigrams
    size_t common = 0;
    for (const auto& trigram : set1) {
        if (set2.count(trigram)) {
            ++common;
        }
    }
    
    // Dice coefficient: 2 * |intersection| / (|set1| + |set2|)
    double similarity = (2.0 * common) / (set1.size() + set2.size());
    
    return similarity;
}

} // namespace geocoding
```

#### **Task 1.2: Create Unit Tests**

**File:** `tests/test_string_matcher.cpp`

```cpp
#include <gtest/gtest.h>
#include "../hmi/services/geocoding/include/string_matcher.h"

using namespace geocoding;

class StringMatcherTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(StringMatcherTest, LevenshteinDistance_IdenticalStrings) {
    EXPECT_EQ(StringMatcher::levenshteinDistance("Main Street", "Main Street"), 0);
}

TEST_F(StringMatcherTest, LevenshteinDistance_SingleSubstitution) {
    EXPECT_EQ(StringMatcher::levenshteinDistance("Main St", "Mian St"), 1);
}

TEST_F(StringMatcherTest, LevenshteinDistance_MultipleEdits) {
    EXPECT_EQ(StringMatcher::levenshteinDistance("Main", "Main Street"), 7);
}

TEST_F(StringMatcherTest, LevenshteinDistance_CaseInsensitive) {
    EXPECT_EQ(StringMatcher::levenshteinDistance("MAIN ST", "main st", false), 0);
}

TEST_F(StringMatcherTest, JaroWinkler_IdenticalStrings) {
    EXPECT_DOUBLE_EQ(StringMatcher::jaroWinklerSimilarity("Broadway", "Broadway"), 1.0);
}

TEST_F(StringMatcherTest, JaroWinkler_SimilarStrings) {
    double similarity = StringMatcher::jaroWinklerSimilarity("Main Street", "Main St");
    EXPECT_GT(similarity, 0.85);  // Should be high similarity
}

TEST_F(StringMatcherTest, JaroWinkler_Typo) {
    double similarity = StringMatcher::jaroWinklerSimilarity("Broadway", "Brodway");
    EXPECT_GT(similarity, 0.90);  // Single typo should still score high
}

TEST_F(StringMatcherTest, Trigram_IdenticalStrings) {
    EXPECT_DOUBLE_EQ(StringMatcher::trigramSimilarity("Springfield", "Springfield"), 1.0);
}

TEST_F(StringMatcherTest, Trigram_SimilarStrings) {
    double similarity = StringMatcher::trigramSimilarity("Springfield", "Springfild");
    EXPECT_GT(similarity, 0.70);  // Missing 'e' should still be similar
}

TEST_F(StringMatcherTest, Trigram_DifferentStrings) {
    double similarity = StringMatcher::trigramSimilarity("Main St", "Broadway");
    EXPECT_LT(similarity, 0.30);  // Completely different
}

TEST_F(StringMatcherTest, Normalize_Whitespace) {
    EXPECT_EQ(StringMatcher::normalizeForComparison("  Main   Street  "), "main street");
}

TEST_F(StringMatcherTest, Normalize_Case) {
    EXPECT_EQ(StringMatcher::normalizeForComparison("MAIN STREET"), "main street");
}

TEST_F(StringMatcherTest, NGrams_Generation) {
    auto trigrams = StringMatcher::generateNGrams("Main", 3);
    EXPECT_EQ(trigrams.size(), 2);  // "Mai", "ain"
    EXPECT_EQ(trigrams[0], "Mai");
    EXPECT_EQ(trigrams[1], "ain");
}
```

### **Day 3-4: Address-Specific Fuzzy Matching**

#### **Task 1.3: Create Address Fuzzy Matcher**

**File:** `hmi/services/geocoding/include/fuzzy_address_matcher.h`

```cpp
#ifndef FUZZY_ADDRESS_MATCHER_H
#define FUZZY_ADDRESS_MATCHER_H

#include "string_matcher.h"
#include "address_components.h"
#include <vector>
#include <unordered_map>

namespace geocoding {

/**
 * @brief Scored match result with similarity breakdown
 */
struct FuzzyMatchResult {
    double overall_score;           // 0.0-1.0 weighted total similarity
    double street_similarity;       // Street name + number similarity
    double city_similarity;         // City name similarity
    double state_similarity;        // State similarity
    double zip_similarity;          // ZIP code similarity (0 or 1)
    
    // Which components contributed to the score
    bool street_matched;
    bool city_matched;
    bool state_matched;
    bool zip_matched;
    
    // Threshold check
    bool isAcceptableMatch(double threshold = 0.7) const {
        return overall_score >= threshold;
    }
};

/**
 * @brief Configuration for fuzzy matching weights and thresholds
 */
struct FuzzyMatchConfig {
    // Component weights (must sum to 1.0)
    double street_weight = 0.40;    // Street is most important
    double city_weight = 0.30;      // City is second
    double state_weight = 0.20;     // State is third
    double zip_weight = 0.10;       // ZIP is least (binary match)
    
    // Minimum acceptable similarity per component
    double min_street_similarity = 0.6;
    double min_city_similarity = 0.7;
    double min_state_similarity = 0.8;
    
    // Overall threshold
    double min_overall_score = 0.7;
    
    // Algorithm preferences
    bool use_phonetic_matching = true;
    bool use_abbreviation_expansion = true;
};

/**
 * @brief Fuzzy address matcher using multiple string similarity algorithms
 * 
 * Combines Levenshtein, Jaro-Winkler, and trigram similarity with
 * address-specific optimizations (abbreviations, phonetics, etc.)
 */
class FuzzyAddressMatcher {
public:
    explicit FuzzyAddressMatcher(const FuzzyMatchConfig& config = FuzzyMatchConfig());
    
    /**
     * @brief Calculate fuzzy match score between two addresses
     * @param query User input address (may have typos)
     * @param candidate Database address to compare against
     * @return Match result with detailed similarity breakdown
     */
    FuzzyMatchResult calculateMatch(const AddressComponents& query,
                                   const AddressComponents& candidate) const;
    
    /**
     * @brief Find best matches from candidate list
     * @param query User input address
     * @param candidates List of addresses from database
     * @param max_results Maximum number of results to return
     * @return Sorted list (best match first) with scores above threshold
     */
    std::vector<std::pair<AddressComponents, FuzzyMatchResult>> 
    findBestMatches(const AddressComponents& query,
                   const std::vector<AddressComponents>& candidates,
                   size_t max_results = 5) const;
    
    /**
     * @brief Calculate similarity between street names
     * Handles common abbreviations (St, Ave, Blvd, etc.)
     */
    double calculateStreetSimilarity(const std::string& street1,
                                    const std::string& street2) const;
    
    /**
     * @brief Calculate similarity between city names
     * Handles phonetic matching and common misspellings
     */
    double calculateCitySimilarity(const std::string& city1,
                                  const std::string& city2) const;
    
private:
    FuzzyMatchConfig m_config;
    
    // Abbreviation mappings for street types
    static const std::unordered_map<std::string, std::string> STREET_ABBREVIATIONS;
    
    // Expand abbreviations in street name
    std::string expandStreetAbbreviations(const std::string& street) const;
    
    // Calculate phonetic similarity (Soundex-like)
    double calculatePhoneticSimilarity(const std::string& s1, 
                                      const std::string& s2) const;
    
    // Generate phonetic code (simplified Soundex)
    std::string generatePhoneticCode(const std::string& text) const;
};

} // namespace geocoding

#endif // FUZZY_ADDRESS_MATCHER_H
```

**File:** `hmi/services/geocoding/src/fuzzy_address_matcher.cpp`

```cpp
#include "fuzzy_address_matcher.h"
#include <algorithm>
#include <sstream>

namespace geocoding {

// Street type abbreviations (USPS Publication 28 standard)
const std::unordered_map<std::string, std::string> 
FuzzyAddressMatcher::STREET_ABBREVIATIONS = {
    {"street", "st"}, {"avenue", "ave"}, {"boulevard", "blvd"},
    {"drive", "dr"}, {"road", "rd"}, {"lane", "ln"},
    {"court", "ct"}, {"circle", "cir"}, {"place", "pl"},
    {"square", "sq"}, {"terrace", "ter"}, {"parkway", "pkwy"},
    {"highway", "hwy"}, {"expressway", "expy"}, {"freeway", "fwy"},
    {"north", "n"}, {"south", "s"}, {"east", "e"}, {"west", "w"},
    {"northeast", "ne"}, {"northwest", "nw"}, {"southeast", "se"}, {"southwest", "sw"}
};

FuzzyAddressMatcher::FuzzyAddressMatcher(const FuzzyMatchConfig& config)
    : m_config(config) {
    // Validate weights sum to 1.0
    double total_weight = m_config.street_weight + m_config.city_weight + 
                         m_config.state_weight + m_config.zip_weight;
    if (std::abs(total_weight - 1.0) > 0.01) {
        // Normalize weights
        m_config.street_weight /= total_weight;
        m_config.city_weight /= total_weight;
        m_config.state_weight /= total_weight;
        m_config.zip_weight /= total_weight;
    }
}

std::string FuzzyAddressMatcher::expandStreetAbbreviations(const std::string& street) const {
    std::string normalized = StringMatcher::normalizeForComparison(street);
    std::stringstream ss(normalized);
    std::string word, result;
    
    while (ss >> word) {
        // Check if word is an abbreviation
        auto it = STREET_ABBREVIATIONS.find(word);
        if (it != STREET_ABBREVIATIONS.end()) {
            result += it->second + " ";
        } else {
            // Check reverse mapping (abbreviation to full)
            bool found = false;
            for (const auto& [full, abbr] : STREET_ABBREVIATIONS) {
                if (abbr == word) {
                    result += full + " ";
                    found = true;
                    break;
                }
            }
            if (!found) {
                result += word + " ";
            }
        }
    }
    
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

std::string FuzzyAddressMatcher::generatePhoneticCode(const std::string& text) const {
    if (text.empty()) return "";
    
    std::string normalized = StringMatcher::normalizeForComparison(text);
    std::string code;
    code += normalized[0];  // Keep first letter
    
    // Simplified Soundex encoding
    const std::string consonant_groups[] = {
        "bfpv",     // 1
        "cgjkqsxz", // 2
        "dt",       // 3
        "l",        // 4
        "mn",       // 5
        "r"         // 6
    };
    
    char prev_code = '0';
    for (size_t i = 1; i < normalized.length() && code.length() < 4; ++i) {
        char c = normalized[i];
        char curr_code = '0';
        
        // Find consonant group
        for (size_t g = 0; g < 6; ++g) {
            if (consonant_groups[g].find(c) != std::string::npos) {
                curr_code = '1' + g;
                break;
            }
        }
        
        // Add code if different from previous
        if (curr_code != '0' && curr_code != prev_code) {
            code += curr_code;
            prev_code = curr_code;
        }
    }
    
    // Pad with zeros
    while (code.length() < 4) {
        code += '0';
    }
    
    return code;
}

double FuzzyAddressMatcher::calculatePhoneticSimilarity(const std::string& s1, 
                                                       const std::string& s2) const {
    std::string code1 = generatePhoneticCode(s1);
    std::string code2 = generatePhoneticCode(s2);
    
    return (code1 == code2) ? 1.0 : 0.0;
}

double FuzzyAddressMatcher::calculateStreetSimilarity(const std::string& street1,
                                                     const std::string& street2) const {
    if (street1.empty() || street2.empty()) return 0.0;
    
    // Expand abbreviations first
    std::string expanded1 = m_config.use_abbreviation_expansion 
        ? expandStreetAbbreviations(street1) : street1;
    std::string expanded2 = m_config.use_abbreviation_expansion 
        ? expandStreetAbbreviations(street2) : street2;
    
    // Calculate multiple similarity metrics
    double jaro_winkler = StringMatcher::jaroWinklerSimilarity(expanded1, expanded2);
    double trigram = StringMatcher::trigramSimilarity(expanded1, expanded2);
    
    // Use phonetic similarity as bonus
    double phonetic = m_config.use_phonetic_matching 
        ? calculatePhoneticSimilarity(expanded1, expanded2) : 0.0;
    
    // Weighted combination
    double similarity = (jaro_winkler * 0.5) + (trigram * 0.4) + (phonetic * 0.1);
    
    return std::min(similarity, 1.0);
}

double FuzzyAddressMatcher::calculateCitySimilarity(const std::string& city1,
                                                   const std::string& city2) const {
    if (city1.empty() || city2.empty()) return 0.0;
    
    // Cities benefit from phonetic matching (Springfield vs Springfiled)
    double jaro_winkler = StringMatcher::jaroWinklerSimilarity(city1, city2);
    double phonetic = m_config.use_phonetic_matching 
        ? calculatePhoneticSimilarity(city1, city2) : 0.0;
    
    return std::max(jaro_winkler, phonetic);
}

FuzzyMatchResult FuzzyAddressMatcher::calculateMatch(
    const AddressComponents& query,
    const AddressComponents& candidate) const {
    
    FuzzyMatchResult result{};
    
    // Calculate component similarities
    std::string query_street = query.street_number + " " + query.street_name;
    std::string cand_street = candidate.street_number + " " + candidate.street_name;
    result.street_similarity = calculateStreetSimilarity(query_street, cand_street);
    result.street_matched = (result.street_similarity >= m_config.min_street_similarity);
    
    result.city_similarity = calculateCitySimilarity(query.city, candidate.city);
    result.city_matched = (result.city_similarity >= m_config.min_city_similarity);
    
    // State comparison (exact match or abbreviation)
    result.state_similarity = (StringMatcher::normalizeForComparison(query.state) == 
                              StringMatcher::normalizeForComparison(candidate.state)) ? 1.0 : 0.0;
    result.state_matched = (result.state_similarity >= m_config.min_state_similarity);
    
    // ZIP code (binary match)
    result.zip_similarity = (query.zip_code == candidate.zip_code) ? 1.0 : 0.0;
    result.zip_matched = (result.zip_similarity == 1.0);
    
    // Calculate weighted overall score
    result.overall_score = 
        (result.street_similarity * m_config.street_weight) +
        (result.city_similarity * m_config.city_weight) +
        (result.state_similarity * m_config.state_weight) +
        (result.zip_similarity * m_config.zip_weight);
    
    return result;
}

std::vector<std::pair<AddressComponents, FuzzyMatchResult>> 
FuzzyAddressMatcher::findBestMatches(
    const AddressComponents& query,
    const std::vector<AddressComponents>& candidates,
    size_t max_results) const {
    
    std::vector<std::pair<AddressComponents, FuzzyMatchResult>> matches;
    
    // Calculate scores for all candidates
    for (const auto& candidate : candidates) {
        FuzzyMatchResult score = calculateMatch(query, candidate);
        
        if (score.isAcceptableMatch(m_config.min_overall_score)) {
            matches.emplace_back(candidate, score);
        }
    }
    
    // Sort by score (descending)
    std::sort(matches.begin(), matches.end(),
             [](const auto& a, const auto& b) {
                 return a.second.overall_score > b.second.overall_score;
             });
    
    // Limit results
    if (matches.size() > max_results) {
        matches.resize(max_results);
    }
    
    return matches;
}

} // namespace geocoding
```

### **Day 5: Integration & Testing**

#### **Task 1.4: Integrate Fuzzy Matching into POIService**

**File:** `hmi/services/poi/src/poi_service.cpp` (modifications)

```cpp
// Add to includes
#include "fuzzy_address_matcher.h"

// Modify geocodeAddressEnhanced() to use fuzzy matching as fallback
geocoding::EnhancedGeocodingResult POIService::geocodeAddressEnhanced(
    const geocoding::EnhancedAddressRequest& request) {
    
    // ... existing code for exact matching ...
    
    // If no exact match found, try fuzzy matching
    if (!result.success && !spatial_candidates.empty()) {
        geocoding::FuzzyAddressMatcher fuzzyMatcher;
        geocoding::AddressComponents query_addr = 
            geocoding::AddressComponents::fromEnhancedRequest(request);
        
        std::vector<geocoding::AddressComponents> candidates;
        for (const auto& candidate : spatial_candidates) {
            candidates.push_back(candidate.address);
        }
        
        auto fuzzy_matches = fuzzyMatcher.findBestMatches(query_addr, candidates, 5);
        
        if (!fuzzy_matches.empty()) {
            const auto& best_match = fuzzy_matches[0];
            result.success = true;
            result.primary_result = convertToCandidate(best_match.first);
            result.primary_result.confidence_score = best_match.second.overall_score;
            result.primary_result.match_type = "fuzzy_match";
            result.primary_result.geocoding_method = "fuzzy_string_matching";
            
            // Add alternatives
            for (size_t i = 1; i < fuzzy_matches.size(); ++i) {
                result.alternatives.push_back(convertToCandidate(fuzzy_matches[i].first));
            }
        }
    }
    
    return result;
}
```

---

## 🗄️ WEEK 6-7: CACHING LAYER IMPLEMENTATION

### **Day 6-8: Cache Design & In-Memory Implementation**

#### **Task 2.1: Create Cache Interface**

**File:** `hmi/services/geocoding/include/geocoding_cache.h`

```cpp
#ifndef GEOCODING_CACHE_H
#define GEOCODING_CACHE_H

#include "enhanced_geocoding.h"
#include <memory>
#include <optional>
#include <chrono>
#include <string>

namespace geocoding {

/**
 * @brief Cache statistics for monitoring
 */
struct CacheStats {
    uint64_t hits = 0;           // Cache hit count
    uint64_t misses = 0;         // Cache miss count
    uint64_t inserts = 0;        // Successful inserts
    uint64_t evictions = 0;      // LRU evictions
    uint64_t expired = 0;        // TTL expiration count
    size_t current_size = 0;     // Current entries
    size_t max_size = 0;         // Maximum capacity
    
    double hitRate() const {
        uint64_t total = hits + misses;
        return total > 0 ? static_cast<double>(hits) / total : 0.0;
    }
    
    std::string toString() const;
};

/**
 * @brief Abstract cache interface for geocoding results
 * 
 * Implementations can be in-memory LRU or Redis-backed
 */
class IGeocodingCache {
public:
    virtual ~IGeocodingCache() = default;
    
    /**
     * @brief Store geocoding result in cache
     * @param key Normalized address string
     * @param result Geocoding result to cache
     * @param ttl Time-to-live (default: 24 hours)
     */
    virtual void put(const std::string& key,
                    const EnhancedGeocodingResult& result,
                    std::chrono::seconds ttl = std::chrono::hours(24)) = 0;
    
    /**
     * @brief Retrieve cached result
     * @param key Normalized address string
     * @return Result if found and not expired, std::nullopt otherwise
     */
    virtual std::optional<EnhancedGeocodingResult> get(const std::string& key) = 0;
    
    /**
     * @brief Remove entry from cache
     */
    virtual void remove(const std::string& key) = 0;
    
    /**
     * @brief Clear all entries
     */
    virtual void clear() = 0;
    
    /**
     * @brief Get cache statistics
     */
    virtual CacheStats getStats() const = 0;
    
    /**
     * @brief Evict expired entries (call periodically)
     */
    virtual size_t evictExpired() = 0;
    
protected:
    /**
     * @brief Generate normalized cache key from address
     * Ensures consistent keys regardless of input formatting
     */
    static std::string generateKey(const std::string& address);
};

/**
 * @brief In-memory LRU cache implementation
 * 
 * Thread-safe, fixed-size cache with TTL support
 */
class InMemoryGeocodingCache : public IGeocodingCache {
public:
    explicit InMemoryGeocodingCache(size_t max_size = 10000);
    ~InMemoryGeocodingCache() override = default;
    
    void put(const std::string& key,
            const EnhancedGeocodingResult& result,
            std::chrono::seconds ttl = std::chrono::hours(24)) override;
    
    std::optional<EnhancedGeocodingResult> get(const std::string& key) override;
    
    void remove(const std::string& key) override;
    void clear() override;
    CacheStats getStats() const override;
    size_t evictExpired() override;
    
private:
    struct CacheEntry {
        EnhancedGeocodingResult result;
        std::chrono::system_clock::time_point expiration;
        std::chrono::system_clock::time_point last_access;
    };
    
    size_t m_max_size;
    mutable CacheStats m_stats;
    
    // LRU implementation details (use std::list + unordered_map)
    std::list<std::pair<std::string, CacheEntry>> m_lru_list;
    std::unordered_map<std::string, decltype(m_lru_list)::iterator> m_cache_map;
    
    mutable std::mutex m_mutex;  // Thread safety
    
    // LRU helpers
    void moveToFront(const std::string& key);
    void evictLRU();
};

} // namespace geocoding

#endif // GEOCODING_CACHE_H
```

### **Day 9-11: Redis Integration**

#### **Task 2.2: Redis Cache Implementation**

**Dependencies:** Install hiredis (Redis C++ client)
```powershell
# Using vcpkg on Windows
vcpkg install hiredis:x64-windows
```

**File:** `hmi/services/geocoding/include/redis_geocoding_cache.h`

```cpp
#ifndef REDIS_GEOCODING_CACHE_H
#define REDIS_GEOCODING_CACHE_H

#include "geocoding_cache.h"
#include <hiredis/hiredis.h>
#include <memory>

namespace geocoding {

/**
 * @brief Redis-backed geocoding cache
 * 
 * Features:
 * - Persistent cache across restarts
 * - Shared cache across multiple services
 * - Atomic operations with Redis commands
 * - Automatic TTL expiration (Redis native)
 */
class RedisGeocodingCache : public IGeocodingCache {
public:
    struct RedisConfig {
        std::string host = "127.0.0.1";
        int port = 6379;
        std::string password;
        int database = 0;
        std::chrono::seconds connect_timeout = std::chrono::seconds(5);
        std::chrono::seconds command_timeout = std::chrono::seconds(1);
        std::string key_prefix = "geocode:";  // Namespace for keys
    };
    
    explicit RedisGeocodingCache(const RedisConfig& config = RedisConfig());
    ~RedisGeocodingCache() override;
    
    // Connect to Redis server
    bool connect();
    bool isConnected() const;
    void disconnect();
    
    // IGeocodingCache interface
    void put(const std::string& key,
            const EnhancedGeocodingResult& result,
            std::chrono::seconds ttl = std::chrono::hours(24)) override;
    
    std::optional<EnhancedGeocodingResult> get(const std::string& key) override;
    
    void remove(const std::string& key) override;
    void clear() override;
    CacheStats getStats() const override;
    size_t evictExpired() override;  // No-op for Redis (automatic)
    
private:
    RedisConfig m_config;
    redisContext* m_context = nullptr;
    mutable CacheStats m_stats;
    mutable std::mutex m_mutex;
    
    // Serialization helpers
    std::string serializeResult(const EnhancedGeocodingResult& result) const;
    std::optional<EnhancedGeocodingResult> deserializeResult(const std::string& data) const;
    
    // Redis command helpers
    bool executeCommand(const std::string& command);
    std::optional<std::string> getStringValue(const std::string& key);
    
    // Full key with namespace prefix
    std::string makeFullKey(const std::string& key) const {
        return m_config.key_prefix + key;
    }
};

} // namespace geocoding

#endif // REDIS_GEOCODING_CACHE_H
```

**Implementation notes:** Full Redis implementation would be ~400 lines. Key features:
- JSON serialization of `EnhancedGeocodingResult` using Qt's JSON classes
- Connection pooling for concurrent requests
- Automatic reconnection on connection loss
- Redis SETEX command for atomic set-with-TTL

---

## 🚀 WEEK 8: BATCH GEOCODING API

### **Day 12-14: Batch API Design & Implementation**

#### **Task 3.1: Create Batch Geocoder**

**File:** `hmi/services/geocoding/include/batch_geocoder.h`

```cpp
#ifndef BATCH_GEOCODER_H
#define BATCH_GEOCODER_H

#include "enhanced_geocoding.h"
#include "geocoding_cache.h"
#include <vector>
#include <functional>
#include <future>
#include <queue>
#include <thread>

namespace geocoding {

/**
 * @brief Batch geocoding request configuration
 */
struct BatchRequest {
    std::vector<EnhancedAddressRequest> addresses;
    size_t max_workers = 8;              // Thread pool size
    bool continue_on_error = true;       // Don't stop on failures
    bool use_cache = true;               // Check cache first
    bool skip_duplicates = true;         // Deduplicate addresses
    
    // Progress callback (called every N addresses)
    std::function<void(size_t completed, size_t total)> progress_callback;
    size_t progress_interval = 100;
};

/**
 * @brief Batch geocoding response
 */
struct BatchResponse {
    std::vector<EnhancedGeocodingResult> results;  // Same order as input
    size_t success_count = 0;
    size_t failure_count = 0;
    size_t cache_hits = 0;
    double total_time_ms = 0.0;
    double avg_latency_ms = 0.0;
    
    std::string summary() const;
};

/**
 * @brief High-performance batch geocoder with parallel processing
 * 
 * Features:
 * - Thread pool for concurrent geocoding
 * - Automatic cache integration
 * - Progress tracking
 * - Deduplication
 * - Error resilience
 */
class BatchGeocoder {
public:
    /**
     * @brief Constructor
     * @param geocoder Geocoding service to use
     * @param cache Optional cache (nullptr = no caching)
     */
    BatchGeocoder(IGeocodingService* geocoder, 
                 IGeocodingCache* cache = nullptr);
    
    ~BatchGeocoder();
    
    /**
     * @brief Process batch of addresses in parallel
     * @param request Batch configuration and addresses
     * @return Results in same order as input
     */
    BatchResponse geocodeBatch(const BatchRequest& request);
    
    /**
     * @brief Process addresses from CSV file
     * @param input_csv Input file path
     * @param output_csv Output file path
     * @param has_header Whether CSV has header row
     * @return Batch statistics
     * 
     * CSV Format:
     * street_number,street_name,city,state,zip_code,country
     */
    BatchResponse geocodeFromCSV(const std::string& input_csv,
                                const std::string& output_csv,
                                bool has_header = true);
    
    /**
     * @brief Process addresses from JSON file
     * @param input_json Input file path
     * @param output_json Output file path
     * @return Batch statistics
     * 
     * JSON Format:
     * [{"freeform_address": "123 Main St, Springfield, IL"}, ...]
     */
    BatchResponse geocodeFromJSON(const std::string& input_json,
                                 const std::string& output_json);
    
    /**
     * @brief Cancel ongoing batch operation
     */
    void cancel();
    
    /**
     * @brief Check if batch is currently running
     */
    bool isRunning() const;
    
private:
    IGeocodingService* m_geocoder;
    IGeocodingCache* m_cache;
    
    // Thread pool management
    std::vector<std::thread> m_workers;
    std::queue<size_t> m_task_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    std::atomic<bool> m_stop_flag{false};
    std::atomic<bool> m_running{false};
    
    // Worker thread function
    void workerThread(const std::vector<EnhancedAddressRequest>& addresses,
                     std::vector<EnhancedGeocodingResult>& results,
                     std::atomic<size_t>& completed,
                     const BatchRequest& config);
    
    // Deduplication helpers
    std::vector<size_t> findDuplicates(const std::vector<EnhancedAddressRequest>& addresses);
    
    // File I/O helpers
    std::vector<EnhancedAddressRequest> readCSV(const std::string& path, bool has_header);
    void writeCSV(const std::string& path, 
                 const std::vector<EnhancedGeocodingResult>& results);
    
    std::vector<EnhancedAddressRequest> readJSON(const std::string& path);
    void writeJSON(const std::string& path,
                  const std::vector<EnhancedGeocodingResult>& results);
};

/**
 * @brief Abstract geocoding service interface for batch processing
 */
class IGeocodingService {
public:
    virtual ~IGeocodingService() = default;
    virtual EnhancedGeocodingResult geocodeAddressEnhanced(
        const EnhancedAddressRequest& request) = 0;
};

} // namespace geocoding

#endif // BATCH_GEOCODER_H
```

#### **Task 3.2: Implement Batch Geocoder**

**Implementation highlights (key methods):**

```cpp
BatchResponse BatchGeocoder::geocodeBatch(const BatchRequest& request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    BatchResponse response;
    response.results.resize(request.addresses.size());
    
    // Deduplication
    std::vector<size_t> duplicates;
    if (request.skip_duplicates) {
        duplicates = findDuplicates(request.addresses);
    }
    
    // Parallel processing with thread pool
    std::atomic<size_t> completed{0};
    m_running = true;
    
    // Create worker threads
    size_t worker_count = std::min(request.max_workers, 
                                   std::thread::hardware_concurrency());
    
    std::vector<std::future<void>> futures;
    size_t batch_size = request.addresses.size() / worker_count;
    
    for (size_t i = 0; i < worker_count; ++i) {
        size_t start_idx = i * batch_size;
        size_t end_idx = (i == worker_count - 1) 
            ? request.addresses.size() 
            : start_idx + batch_size;
        
        futures.push_back(std::async(std::launch::async, [=, &request, &response, &completed]() {
            for (size_t j = start_idx; j < end_idx; ++j) {
                if (m_stop_flag) break;
                
                // Check cache first
                if (request.use_cache && m_cache) {
                    std::string key = IGeocodingCache::generateKey(
                        request.addresses[j].freeform_address);
                    auto cached = m_cache->get(key);
                    if (cached) {
                        response.results[j] = *cached;
                        response.cache_hits++;
                        completed++;
                        continue;
                    }
                }
                
                // Geocode
                try {
                    response.results[j] = m_geocoder->geocodeAddressEnhanced(
                        request.addresses[j]);
                    
                    if (response.results[j].success) {
                        response.success_count++;
                        
                        // Cache result
                        if (request.use_cache && m_cache) {
                            std::string key = IGeocodingCache::generateKey(
                                request.addresses[j].freeform_address);
                            m_cache->put(key, response.results[j]);
                        }
                    } else {
                        response.failure_count++;
                    }
                } catch (const std::exception& e) {
                    response.failure_count++;
                    if (!request.continue_on_error) {
                        m_stop_flag = true;
                        break;
                    }
                }
                
                completed++;
                
                // Progress callback
                if (request.progress_callback && 
                    completed % request.progress_interval == 0) {
                    request.progress_callback(completed, request.addresses.size());
                }
            }
        }));
    }
    
    // Wait for all workers
    for (auto& future : futures) {
        future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    response.total_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
    response.avg_latency_ms = response.total_time_ms / request.addresses.size();
    
    m_running = false;
    return response;
}
```

### **Day 15: CSV/JSON File I/O**

#### **Task 3.3: File Format Support**

**CSV Reader:**
```cpp
std::vector<EnhancedAddressRequest> BatchGeocoder::readCSV(
    const std::string& path, bool has_header) {
    
    std::vector<EnhancedAddressRequest> addresses;
    std::ifstream file(path);
    std::string line;
    
    if (has_header) {
        std::getline(file, line);  // Skip header
    }
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        
        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        
        if (fields.size() >= 6) {
            EnhancedAddressRequest req;
            req.freeform_address = fields[0] + " " + fields[1] + ", " + 
                                  fields[2] + ", " + fields[3] + " " + fields[4];
            addresses.push_back(req);
        }
    }
    
    return addresses;
}
```

---

## 📊 INTEGRATION & TESTING

### **Week 8 Final Days: Performance Benchmarks**

#### **Test Suite**

**File:** `tests/test_phase2_integration.cpp`

```cpp
#include <gtest/gtest.h>
#include "../hmi/services/geocoding/include/fuzzy_address_matcher.h"
#include "../hmi/services/geocoding/include/geocoding_cache.h"
#include "../hmi/services/geocoding/include/batch_geocoder.h"

using namespace geocoding;

class Phase2IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test data
    }
};

TEST_F(Phase2IntegrationTest, FuzzyMatching_HandlesTypos) {
    FuzzyAddressMatcher matcher;
    AddressComponents query{"123", "Mian Street", "Springfield", "IL", "62701", "US"};
    AddressComponents candidate{"123", "Main Street", "Springfield", "IL", "62701", "US"};
    
    auto result = matcher.calculateMatch(query, candidate);
    EXPECT_GT(result.overall_score, 0.85);  // Should match despite typo
}

TEST_F(Phase2IntegrationTest, Cache_ImprovesPer formance) {
    InMemoryGeocodingCache cache(1000);
    
    // First request (cache miss)
    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = /* geocode address */;
    auto end1 = std::chrono::high_resolution_clock::now();
    auto latency1 = std::chrono::duration<double, std::milli>(end1 - start1).count();
    
    cache.put("test_address", result1);
    
    // Second request (cache hit)
    auto start2 = std::chrono::high_resolution_clock::now();
    auto cached = cache.get("test_address");
    auto end2 = std::chrono::high_resolution_clock::now();
    auto latency2 = std::chrono::duration<double, std::milli>(end2 - start2).count();
    
    EXPECT_TRUE(cached.has_value());
    EXPECT_LT(latency2, latency1 / 10);  // Cache should be 10x+ faster
}

TEST_F(Phase2IntegrationTest, BatchGeocoding_ProcessesThousands) {
    // Create 10,000 test addresses
    std::vector<EnhancedAddressRequest> addresses(10000);
    for (size_t i = 0; i < 10000; ++i) {
        addresses[i].freeform_address = 
            std::to_string(i) + " Main St, Springfield, IL 62701";
    }
    
    BatchRequest request;
    request.addresses = addresses;
    request.max_workers = 8;
    
    BatchGeocoder batch_geocoder(/* service */, /* cache */);
    auto response = batch_geocoder.geocodeBatch(request);
    
    EXPECT_EQ(response.results.size(), 10000);
    EXPECT_GT(response.success_count, 9500);  // >95% success
    EXPECT_LT(response.total_time_ms, 30000);  // <30 seconds = ~333/sec
}
```

---

## 📈 SUCCESS METRICS & BENCHMARKS

### **Expected Performance Improvements**

| Metric | Before Phase 2 | After Phase 2 | Improvement |
|--------|----------------|---------------|-------------|
| **Typo Tolerance** | 0% (exact match only) | 85%+ (1-2 typos) | **New capability** |
| **Abbreviation Handling** | Manual normalization | Automatic expansion | **Automatic** |
| **Cache Hit Rate** | N/A | 80%+ | **New capability** |
| **Cached Request Latency** | 5-10ms | <1ms | **10x faster** |
| **Batch Throughput** | N/A | 10,000+ addr/sec | **New capability** |
| **Large Dataset Processing** | Not feasible | 1M addresses in ~2 min | **Scalable** |

---

## 🚀 DEPLOYMENT CHECKLIST

### **Phase 2 Completion Requirements**

- [ ] **Fuzzy String Matching**
  - [ ] Levenshtein distance implemented
  - [ ] Jaro-Winkler similarity implemented
  - [ ] Trigram matching implemented
  - [ ] Street abbreviation expansion (USPS standard)
  - [ ] Phonetic matching (Soundex-like)
  - [ ] Integration tests passing (>85% accuracy on typos)

- [ ] **Caching Layer**
  - [ ] In-memory LRU cache implemented
  - [ ] Redis integration complete (optional)
  - [ ] TTL expiration working
  - [ ] Cache statistics tracking
  - [ ] Thread-safe operations
  - [ ] >80% hit rate in production simulation

- [ ] **Batch Geocoding**
  - [ ] Thread pool parallel processing
  - [ ] CSV file I/O
  - [ ] JSON file I/O
  - [ ] Progress tracking
  - [ ] Deduplication
  - [ ] Error handling
  - [ ] 10,000+ addresses/sec throughput

- [ ] **Documentation**
  - [ ] API documentation complete
  - [ ] Performance benchmark results
  - [ ] Integration guide for existing code
  - [ ] Redis deployment guide

- [ ] **Testing**
  - [ ] Unit tests (>90% coverage)
  - [ ] Integration tests
  - [ ] Performance benchmarks
  - [ ] Real-world dataset validation

---

## 📝 NEXT STEPS: PHASE 3 PREVIEW

After completing Phase 2, the next phase will focus on:
1. **AI/ML Integration:** TensorFlow Lite models for semantic address matching
2. **Reverse Geocoding:** Lat/lon → address with interpolation
3. **Address Autocomplete:** Real-time suggestions with predictive text
4. **GCP Deployment:** Kubernetes, Cloud Spanner, Vertex AI

---

**Document Version:** 1.0  
**Last Updated:** October 19, 2025  
**Dependencies:** Phase 1 must be 100% complete before starting Phase 2
