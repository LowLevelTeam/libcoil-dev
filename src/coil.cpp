/**
 * @file coil.cpp
 * @brief Main implementation file for the COIL library
 */

#include "coil/coil.hpp"
#include "coil/err.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace coil {

Library::Library() {
  // Set up version information
  m_version.major = 0;
  m_version.minor = 1;
  m_version.patch = 0;
  m_version.string = "COIL 0.1.0";
  
  // Get build timestamp
  std::stringstream ss;
  ss << __DATE__ << " " << __TIME__;
  m_version.build = ss.str();
  
  // Set up configuration
#ifdef NDEBUG
  m_config.debug_enabled = false;
  m_config.asserts_enabled = false;
#else
  m_config.debug_enabled = true;
  m_config.asserts_enabled = true;
#endif
}

void Library::initialize() {
  if (m_initialized) {
    return;
  }
  
  // Setup default logger
  Logger::setCallback([](ErrorLevel level, const std::string& message, const ErrorPosition* position) {
    std::stringstream ss;
    ss << "COIL " << (level == ErrorLevel::Info ? "Info" : 
                     level == ErrorLevel::Warning ? "Warning" : 
                     level == ErrorLevel::Error ? "Error" : "Fatal")
       << ": ";
    
    if (position) {
      if (position->line > 0) {
        ss << position->file << ":" << position->line << ": ";
      } else {
        ss << position->file << ":" << position->index << ": ";
      }
    }
    
    ss << message;
    
    if (level == ErrorLevel::Error || level == ErrorLevel::Fatal) {
      std::cerr << ss.str() << std::endl;
      
      // Don't abort on Error, just Fatal
      if (level == ErrorLevel::Fatal) {
        std::cerr << "Fatal error: aborting" << std::endl;
        std::abort();
      }
    } else {
      std::cout << ss.str() << std::endl;
    }
  });
  
  // Report initialization
  Logger::info("COIL Library " + m_version.string + " initialized");
  
  m_initialized = true;
}

void Library::shutdown() {
  if (!m_initialized) {
    return;
  }
  
  // Report shutdown
  Logger::info("COIL Library " + m_version.string + " shutdown");
  
  m_initialized = false;
}

} // namespace coil