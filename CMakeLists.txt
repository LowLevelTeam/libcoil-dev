cmake_minimum_required(VERSION 3.14)
project(coil 
  VERSION 0.1.0
  DESCRIPTION "Computer Oriented Intermediate Language"
  LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(COIL_BUILD_TESTS "Build COIL tests" ON)
option(COIL_ENABLE_EXCEPTIONS "Enable C++ exceptions" ON)
option(COIL_ENABLE_RTTI "Enable C++ runtime type information" ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Conditionally disable exceptions and RTTI
if(NOT COIL_ENABLE_EXCEPTIONS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

if(NOT COIL_ENABLE_RTTI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

# Source files
set(COIL_SOURCES
  src/coil.cpp
  src/instr.cpp
  src/obj.cpp
)

# Headers
set(COIL_HEADERS
  include/coil/coil.hpp
  include/coil/err.hpp
  include/coil/instr.hpp
  include/coil/obj.hpp
  include/coil/stream.hpp
  include/coil/types.hpp
)

# Create the library
add_library(coil ${COIL_SOURCES} ${COIL_HEADERS})
target_include_directories(coil
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Version definitions
target_compile_definitions(coil PRIVATE 
  COIL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  COIL_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  COIL_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Tests
if(COIL_BUILD_TESTS)
  include(FetchContent)
  
  # Fetch Catch2 for testing
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.1.0 # Update to the latest version
  )
  FetchContent_MakeAvailable(Catch2)
  
  # Enable testing
  enable_testing()
  
  # Add tests subdirectory
  add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

# Install targets
install(TARGETS coil
  EXPORT coil-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT coil-targets
  FILE coil-targets.cmake
  NAMESPACE coil::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/coil
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/coil-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/coil-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/coil
)

# Create and install version file
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/coil-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/coil-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/coil-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/coil
)

# Package configuration
set(CPACK_PACKAGE_NAME "libcoil-dev")
set(CPACK_PACKAGE_VENDOR "COIL Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Computer Oriented Intermediate Language")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)