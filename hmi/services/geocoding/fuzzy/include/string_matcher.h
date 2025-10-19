#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace geocoding {
namespace fuzzy {

/**
 * @brief String similarity matching utilities
 * Implements Levenshtein distance, Jaro-Winkler, and phonetic matching
 */
class StringMatcher {
public:
    StringMatcher() = default;

    /**
     * @brief Calculate Levenshtein edit distance between two strings
     * @param s1 First string
     * @param s2 Second string
     * @return Edit distance (number of single-character edits needed)
     */
    static int levenshteinDistance(const std::string& s1, const std::string& s2);

    /**
     * @brief Calculate normalized similarity score (0.0 - 1.0)
     * @param s1 First string
     * @param s2 Second string
     * @return Similarity score where 1.0 = exact match
     */
    static double similarityScore(const std::string& s1, const std::string& s2);

    /**
     * @brief Jaro-Winkler similarity (good for short strings like names)
     * @param s1 First string
     * @param s2 Second string
     * @return Similarity score (0.0 - 1.0)
     */
    static double jaroWinklerSimilarity(const std::string& s1, const std::string& s2);

    /**
     * @brief Check if strings are phonetically similar (e.g., "Street" vs "St")
     * @param s1 First string
     * @param s2 Second string
     * @return True if phonetically similar
     */
    static bool isPhoneticMatch(const std::string& s1, const std::string& s2);

    /**
     * @brief Normalize string for comparison (lowercase, trim, etc.)
     * @param str Input string
     * @return Normalized string
     */
    static std::string normalize(const std::string& str);

    /**
     * @brief Get phonetic representation (Soundex-like)
     * @param str Input string
     * @return Phonetic code
     */
    static std::string getPhoneticCode(const std::string& str);

private:
    // Helper: Jaro similarity
    static double jaroSimilarity(const std::string& s1, const std::string& s2);

    // Helper: Common prefix length
    static int commonPrefixLength(const std::string& s1, const std::string& s2, int maxLength = 4);
};

}} // namespace geocoding::fuzzy
