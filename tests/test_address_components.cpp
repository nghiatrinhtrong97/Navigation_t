#include <gtest/gtest.h>
#include "../hmi/services/geocoding/include/address_components.h"
#include <string>

using namespace nav::geocoding;

// ============================================================================
// Test Fixture for AddressComponents
// ============================================================================

class AddressComponentsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common test data
    }
};

// ============================================================================
// Test Helper Functions
// ============================================================================

TEST(HelperFunctionsTest, QualityToString) {
    EXPECT_EQ(qualityToString(GeocodingQuality::ROOFTOP), "ROOFTOP");
    EXPECT_EQ(qualityToString(GeocodingQuality::RANGE_INTERPOLATED), "RANGE_INTERPOLATED");
    EXPECT_EQ(qualityToString(GeocodingQuality::GEOMETRIC_CENTER), "GEOMETRIC_CENTER");
    EXPECT_EQ(qualityToString(GeocodingQuality::APPROXIMATE), "APPROXIMATE");
    EXPECT_EQ(qualityToString(GeocodingQuality::UNKNOWN), "UNKNOWN");
}

TEST(HelperFunctionsTest, QualityToAccuracyMeters) {
    EXPECT_DOUBLE_EQ(qualityToAccuracyMeters(GeocodingQuality::ROOFTOP), 5.0);
    EXPECT_DOUBLE_EQ(qualityToAccuracyMeters(GeocodingQuality::RANGE_INTERPOLATED), 50.0);
    EXPECT_DOUBLE_EQ(qualityToAccuracyMeters(GeocodingQuality::GEOMETRIC_CENTER), 250.0);
    EXPECT_DOUBLE_EQ(qualityToAccuracyMeters(GeocodingQuality::APPROXIMATE), 5000.0);
    EXPECT_DOUBLE_EQ(qualityToAccuracyMeters(GeocodingQuality::UNKNOWN), 10000.0);
}

// ============================================================================
// Test AddressComponents::isGeocodable()
// ============================================================================

TEST_F(AddressComponentsTest, IsGeocodable_StreetAddress) {
    AddressComponents addr;
    addr.house_number = "123";
    addr.street_name = "Main Street";
    addr.city = "Springfield";
    
    EXPECT_TRUE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_StreetAddressWithPostal) {
    AddressComponents addr;
    addr.house_number = "456";
    addr.street_name = "Oak Avenue";
    addr.postal_code = "62701";
    
    EXPECT_TRUE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_Intersection) {
    AddressComponents addr;
    addr.is_intersection = true;
    addr.intersection_street1 = "Main Street";
    addr.intersection_street2 = "1st Avenue";
    addr.city = "Downtown";
    
    EXPECT_TRUE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_POBox) {
    AddressComponents addr;
    addr.is_po_box = true;
    addr.po_box = "PO Box 123";
    addr.city = "Springfield";
    addr.state = "IL";
    addr.postal_code = "62701";
    
    EXPECT_TRUE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_PostalCodeOnly) {
    AddressComponents addr;
    addr.postal_code = "62701";
    addr.city = "Springfield";
    
    EXPECT_TRUE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_Incomplete) {
    AddressComponents addr;
    addr.house_number = "123";
    // Missing street name, city, and postal code
    
    EXPECT_FALSE(addr.isGeocodable());
}

TEST_F(AddressComponentsTest, IsGeocodable_Empty) {
    AddressComponents addr;
    
    EXPECT_FALSE(addr.isGeocodable());
}

// ============================================================================
// Test AddressComponents::toFormattedString()
// ============================================================================

TEST_F(AddressComponentsTest, ToFormattedString_USPS_Standard) {
    AddressComponents addr;
    addr.house_number = "123";
    addr.street_name = "Main";
    addr.street_type = "St";
    addr.city = "Springfield";
    addr.state_code = "IL";
    addr.postal_code = "62701";
    
    std::string formatted = addr.toFormattedString("USPS");
    
    // USPS format: "123 Main St\nSpringfield, IL 62701"
    EXPECT_NE(formatted.find("123 Main St"), std::string::npos);
    EXPECT_NE(formatted.find("Springfield, IL 62701"), std::string::npos);
}

TEST_F(AddressComponentsTest, ToFormattedString_USPS_WithUnit) {
    AddressComponents addr;
    addr.house_number = "456";
    addr.street_name = "Oak";
    addr.street_type = "Ave";
    addr.unit_type = "Apt";
    addr.unit_number = "2B";
    addr.city = "Chicago";
    addr.state_code = "IL";
    addr.postal_code = "60601";
    
    std::string formatted = addr.toFormattedString("USPS");
    
    EXPECT_NE(formatted.find("456 Oak Ave Apt 2B"), std::string::npos);
}

TEST_F(AddressComponentsTest, ToFormattedString_Display_Full) {
    AddressComponents addr;
    addr.house_number = "789";
    addr.street_prefix = "N";
    addr.street_name = "Michigan";
    addr.street_type = "Ave";
    addr.unit_type = "Suite";
    addr.unit_number = "500";
    addr.city = "Chicago";
    addr.state = "Illinois";
    addr.postal_code = "60611";
    
    std::string formatted = addr.toFormattedString("display");
    
    EXPECT_NE(formatted.find("789 N Michigan Ave"), std::string::npos);
    EXPECT_NE(formatted.find("Suite 500"), std::string::npos);
    EXPECT_NE(formatted.find("Chicago"), std::string::npos);
    EXPECT_NE(formatted.find("Illinois"), std::string::npos);
}

TEST_F(AddressComponentsTest, ToFormattedString_Intersection) {
    AddressComponents addr;
    addr.is_intersection = true;
    addr.intersection_street1 = "Broadway";
    addr.intersection_street2 = "42nd St";
    addr.city = "New York";
    addr.state_code = "NY";
    addr.postal_code = "10036";
    
    std::string formatted = addr.toFormattedString("display");
    
    EXPECT_NE(formatted.find("Broadway & 42nd St"), std::string::npos);
    EXPECT_NE(formatted.find("New York"), std::string::npos);
}

TEST_F(AddressComponentsTest, ToFormattedString_POBox) {
    AddressComponents addr;
    addr.is_po_box = true;
    addr.po_box = "PO Box 999";
    addr.city = "Springfield";
    addr.state_code = "IL";
    addr.postal_code = "62701";
    
    std::string formatted = addr.toFormattedString("USPS");
    
    EXPECT_NE(formatted.find("PO Box 999"), std::string::npos);
}

// ============================================================================
// Test AddressComponents::toMap()
// ============================================================================

TEST_F(AddressComponentsTest, ToMap_AllFields) {
    AddressComponents addr;
    addr.house_number = "123";
    addr.street_name = "Main St";
    addr.city = "Springfield";
    addr.state = "IL";
    addr.postal_code = "62701";
    addr.country_code = "US";
    
    auto map = addr.toMap();
    
    EXPECT_EQ(map["house_number"], "123");
    EXPECT_EQ(map["street_name"], "Main St");
    EXPECT_EQ(map["city"], "Springfield");
    EXPECT_EQ(map["state"], "IL");
    EXPECT_EQ(map["postal_code"], "62701");
    EXPECT_EQ(map["country_code"], "US");
    EXPECT_EQ(map["is_intersection"], "false");
    EXPECT_EQ(map["is_po_box"], "false");
}

TEST_F(AddressComponentsTest, ToMap_OptionalFieldsEmpty) {
    AddressComponents addr;
    addr.house_number = "123";
    addr.street_name = "Main St";
    // unit_type, unit_number, etc. are empty
    
    auto map = addr.toMap();
    
    EXPECT_EQ(map.count("house_number"), 1);
    EXPECT_EQ(map.count("unit_type"), 0); // Should not exist
    EXPECT_EQ(map.count("unit_number"), 0);
}

// ============================================================================
// Test AddressComponents::fromBasicFields()
// ============================================================================

TEST_F(AddressComponentsTest, FromBasicFields_AllProvided) {
    auto addr = AddressComponents::fromBasicFields(
        "123", "Main Street", "Springfield", "Illinois", "62701"
    );
    
    EXPECT_EQ(*addr.house_number, "123");
    EXPECT_EQ(*addr.street_name, "Main Street");
    EXPECT_EQ(*addr.city, "Springfield");
    EXPECT_EQ(*addr.state, "Illinois");
    EXPECT_EQ(*addr.postal_code, "62701");
    EXPECT_EQ(*addr.country_code, "US");
    EXPECT_FALSE(addr.is_intersection);
    EXPECT_FALSE(addr.is_po_box);
}

TEST_F(AddressComponentsTest, FromBasicFields_Partial) {
    auto addr = AddressComponents::fromBasicFields(
        "", "Oak Street", "Chicago", "", "60601"
    );
    
    EXPECT_FALSE(addr.house_number.has_value());
    EXPECT_EQ(*addr.street_name, "Oak Street");
    EXPECT_EQ(*addr.city, "Chicago");
    EXPECT_FALSE(addr.state.has_value());
    EXPECT_EQ(*addr.postal_code, "60601");
}

TEST_F(AddressComponentsTest, FromBasicFields_AllEmpty) {
    auto addr = AddressComponents::fromBasicFields("", "", "", "", "");
    
    EXPECT_FALSE(addr.house_number.has_value());
    EXPECT_FALSE(addr.street_name.has_value());
    EXPECT_FALSE(addr.city.has_value());
    EXPECT_FALSE(addr.state.has_value());
    EXPECT_FALSE(addr.postal_code.has_value());
}

// ============================================================================
// Test GeocodingCandidate
// ============================================================================

TEST(GeocodingCandidateTest, DefaultConstructor) {
    GeocodingCandidate candidate;
    
    EXPECT_DOUBLE_EQ(candidate.latitude, 0.0);
    EXPECT_DOUBLE_EQ(candidate.longitude, 0.0);
    EXPECT_DOUBLE_EQ(candidate.altitude, 0.0);
    EXPECT_DOUBLE_EQ(candidate.confidence_score, 0.0);
    EXPECT_EQ(candidate.quality, GeocodingQuality::UNKNOWN);
}

TEST(GeocodingCandidateTest, FullyPopulated) {
    GeocodingCandidate candidate;
    candidate.latitude = 39.7817;
    candidate.longitude = -89.6501;
    candidate.confidence_score = 0.95;
    candidate.quality = GeocodingQuality::ROOFTOP;
    candidate.accuracy_meters = 5.0;
    candidate.match_type = "exact";
    
    EXPECT_DOUBLE_EQ(candidate.latitude, 39.7817);
    EXPECT_DOUBLE_EQ(candidate.longitude, -89.6501);
    EXPECT_DOUBLE_EQ(candidate.confidence_score, 0.95);
    EXPECT_EQ(candidate.quality, GeocodingQuality::ROOFTOP);
    EXPECT_DOUBLE_EQ(candidate.accuracy_meters, 5.0);
    EXPECT_EQ(candidate.match_type, "exact");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(AddressComponentsTest, EdgeCase_VeryLongStreetName) {
    AddressComponents addr;
    addr.house_number = "1";
    addr.street_name = std::string(500, 'A'); // 500 character street name
    addr.city = "Test City";
    
    EXPECT_TRUE(addr.isGeocodable());
    std::string formatted = addr.toFormattedString("display");
    EXPECT_GT(formatted.length(), 500);
}

TEST_F(AddressComponentsTest, EdgeCase_SpecialCharacters) {
    AddressComponents addr;
    addr.house_number = "123";
    addr.street_name = "O'Malley's Café";
    addr.city = "Saint-André";
    addr.postal_code = "H3Z-2Y7";
    
    EXPECT_TRUE(addr.isGeocodable());
    auto map = addr.toMap();
    EXPECT_EQ(map["street_name"], "O'Malley's Café");
}

TEST_F(AddressComponentsTest, EdgeCase_PostalCodeWithSuffix) {
    AddressComponents addr;
    addr.house_number = "1600";
    addr.street_name = "Pennsylvania Ave NW";
    addr.city = "Washington";
    addr.state_code = "DC";
    addr.postal_code = "20500";
    addr.postal_code_suffix = "0003";
    
    std::string formatted = addr.toFormattedString("USPS");
    EXPECT_NE(formatted.find("20500-0003"), std::string::npos);
}

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
