#ifndef ADDRESS_PARSER_H
#define ADDRESS_PARSER_H

#include "address_components.h"
#include <string>
#include <memory>

namespace nav {
namespace geocoding {

/**
 * @brief Address parsing service using libpostal
 * Thread-safe, singleton pattern
 * 
 * @note libpostal must be installed separately:
 *       - Linux: sudo apt-get install libpostal-dev
 *       - Windows: Download from https://github.com/openvenues/libpostal
 *       - Build with -DUSE_LIBPOSTAL=ON to enable libpostal support
 */
class AddressParser {
public:
    static AddressParser& getInstance();
    
    // Delete copy/move constructors (singleton)
    AddressParser(const AddressParser&) = delete;
    AddressParser& operator=(const AddressParser&) = delete;
    
    /**
     * @brief Parse freeform address text into components
     * @param address_text Raw input (e.g., "123 Main St Apt 4B SF CA 94102")
     * @param country_hint Country code hint for better accuracy ("US", "CA", "MX")
     * @return Parsed address components
     * 
     * @note If libpostal is not available, uses simple regex-based parser
     */
    AddressComponents parse(const std::string& address_text,
                           const std::string& country_hint = "US");
    
    /**
     * @brief Check if parser is initialized and ready
     */
    bool isReady() const { return m_initialized; }
    
    /**
     * @brief Check if libpostal library is available
     */
    bool hasLibpostalSupport() const;
    
private:
    AddressParser();
    ~AddressParser();
    
    bool m_initialized;
    bool m_use_libpostal;
    
    // Fallback parser when libpostal is not available
    AddressComponents parseWithRegex(const std::string& address_text,
                                     const std::string& country_hint);
};

} // namespace geocoding
} // namespace nav

#endif // ADDRESS_PARSER_H
