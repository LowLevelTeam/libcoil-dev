#include "coil/log.hpp"
#include <ctime>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <unistd.h>

namespace coil {

Logger::Logger(const std::string& prefix, FILE* stream, LogLevel level, bool useColors)
    : stream_(stream)
    , level_(level)
    , coloredOutput_(useColors && (stream ? isatty(fileno(stream)) : false))
    , prefix_(prefix.empty() ? "COIL" : prefix) {
}

} // namespace coil