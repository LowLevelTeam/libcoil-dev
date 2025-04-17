/**
 * @file coil.hpp
 * @brief Main header for the COIL library
 */

#pragma once
#include "coil/types.hpp"
#include <string>
#include <string_view>

namespace coil {

/**
 * @brief Library version information
 */
struct Version {
  int major;           // Major version
  int minor;           // Minor version
  int patch;           // Patch version
  std::string string;  // Version as string
  std::string build;   // Build information
};

/**
 * @brief Library configuration information
 */
struct Configuration {
  bool debug_enabled;     // Whether debug is enabled
  bool asserts_enabled;   // Whether asserts are enabled
};

/**
 * @brief COIL library singleton
 */
class Library {
public:
  /**
   * @brief Get the library instance
   */
  static Library& instance() {
    static Library instance;
    return instance;
  }
  
  /**
   * @brief Initialize the library
   * Call this before using any other functions
   */
  void initialize();
  
  /**
   * @brief Shutdown the library
   * Call this when done using the library
   */
  void shutdown();
  
  /**
   * @brief Get the library version
   */
  const Version& getVersion() const { return m_version; }
  
  /**
   * @brief Get the library configuration
   */
  const Configuration& getConfiguration() const { return m_config; }
  
  /**
   * @brief Check if library is initialized
   */
  bool isInitialized() const { return m_initialized; }
  
private:
  // Private constructor for singleton
  Library();
  
  // No copy or move
  Library(const Library&) = delete;
  Library& operator=(const Library&) = delete;
  Library(Library&&) = delete;
  Library& operator=(Library&&) = delete;
  
  Version m_version;
  Configuration m_config;
  bool m_initialized = false;
};

// Convenience functions for global library instance
inline void initialize() {
  Library::instance().initialize();
}

inline void shutdown() {
  Library::instance().shutdown();
}

inline const Version& getVersion() {
  return Library::instance().getVersion();
}

inline const Configuration& getConfiguration() {
  return Library::instance().getConfiguration();
}

} // namespace coil