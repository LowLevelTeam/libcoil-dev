#include "coil/coil.hpp"

namespace coil {

// Library version
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_STRING = "1.0.0";

Version getVersion() {
    Version version;
    version.major = VERSION_MAJOR;
    version.minor = VERSION_MINOR;
    version.patch = VERSION_PATCH;
    version.string = VERSION_STRING;
    return version;
}

} // namespace coil