#include "coil/coil.hpp"

namespace coil {

// Library version - compile-time constants for performance
static constexpr int VERSION_MAJOR = 1;
static constexpr int VERSION_MINOR = 0;
static constexpr int VERSION_PATCH = 0;
static constexpr const char* VERSION_STRING = "1.0.0";

Version getVersion() {
    // Static storage is more efficient than new allocations every time
    // All version values are compile-time constants
    static const Version version = {
        VERSION_MAJOR,
        VERSION_MINOR,
        VERSION_PATCH,
        VERSION_STRING
    };
    return version;
}

} // namespace coil