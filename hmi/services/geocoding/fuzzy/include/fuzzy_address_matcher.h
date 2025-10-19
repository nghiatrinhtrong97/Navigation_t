#pragma once
#include "string_matcher.h"
#include <vector>
#include <memory>

// Forward declaration
namespace nav {
namespace geocoding {
    struct AddressComponents;
}
}

namespace geocoding {
namespace fuzzy {

/**
 * @brief Component-wise matching scores for address fuzzy matching
 */
struct ComponentMatchScore {
    double street_number_score = 0.0;
    double street_name_score = 0.0;
    double city_score = 0.0;
    double district_score = 0.0;
    double overall_score = 0.0;
    bool is_phonetic_match = false;
};

/**
 * @brief Configuration for fuzzy address matching
 */
struct FuzzyMatchConfig {
    // Weights for different components (should sum to 1.0)
    double street_number_weight = 0.20;
    double street_name_weight = 0.40;
    double city_weight = 0.25;
    double district_weight = 0.15;

    // Minimum score thresholds
    double min_overall_score = 0.70;      // Accept matches above 70%
    double min_street_name_score = 0.60;  // Street name is critical

    // Edit distance tolerance
    int max_edit_distance = 3;

    // Enable phonetic matching
    bool use_phonetic_matching = true;
};

/**
 * @brief Fuzzy address matching engine
 * Performs component-wise fuzzy matching with configurable weights
 */
class FuzzyAddressMatcher {
public:
    explicit FuzzyAddressMatcher(const FuzzyMatchConfig& config = FuzzyMatchConfig());

    /**
     * @brief Calculate fuzzy match score between query and candidate address
     * @param query Query address components
     * @param candidate Candidate address components
     * @return Component-wise match scores
     */
    ComponentMatchScore calculateMatch(const nav::geocoding::AddressComponents& query,
                                      const nav::geocoding::AddressComponents& candidate);

    /**
     * @brief Find best matches from a list of candidates
     * @param query Query address
     * @param candidates List of candidate addresses
     * @param max_results Maximum number of results to return
     * @return Sorted list of (address, score) pairs
     */
    std::vector<std::pair<nav::geocoding::AddressComponents, ComponentMatchScore>>
        findBestMatches(const nav::geocoding::AddressComponents& query,
                       const std::vector<nav::geocoding::AddressComponents>& candidates,
                       size_t max_results = 5);

    /**
     * @brief Check if match score meets acceptance criteria
     * @param score Match score to evaluate
     * @return True if match is acceptable
     */
    bool isAcceptableMatch(const ComponentMatchScore& score) const;

    /**
     * @brief Update matching configuration
     * @param config New configuration
     */
    void setConfig(const FuzzyMatchConfig& config) { m_config = config; }

    /**
     * @brief Get current configuration
     * @return Current matching configuration
     */
    const FuzzyMatchConfig& getConfig() const { return m_config; }

private:
    FuzzyMatchConfig m_config;

    // Component matching helpers
    double matchStreetNumber(const std::string& query, const std::string& candidate);
    double matchStreetName(const std::string& query, const std::string& candidate);
    double matchCity(const std::string& query, const std::string& candidate);
    double matchDistrict(const std::string& query, const std::string& candidate);

    // Calculate weighted overall score
    double calculateOverallScore(const ComponentMatchScore& scores);
};

}} // namespace geocoding::fuzzy
