#include <catch2/catch_all.hpp>
#include "coil/coil.hpp"

TEST_CASE("Version information is correct", "[version]") {
    coil::Version version = coil::getVersion();
    
    SECTION("Major version is valid") {
        REQUIRE(version.major == 1);
    }
    
    SECTION("Minor version is valid") {
        REQUIRE(version.minor == 0);
    }
    
    SECTION("Patch version is valid") {
        REQUIRE(version.patch == 0);
    }
    
    SECTION("Version string is correctly formatted") {
        REQUIRE(strcmp(version.string, "1.0.0") == 0);
        
        // Verify that version string matches the numeric components
        char expectedVersion[32];
        sprintf(expectedVersion, "%d.%d.%d", version.major, version.minor, version.patch);
        REQUIRE(strcmp(version.string, expectedVersion) == 0);
    }
}