cmake_minimum_required(VERSION 3.15)
project(libcoil-dev VERSION 1.0.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warning flags
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Library sources
set(LIBCOIL_SOURCES
    src/binary_format.cpp
    src/type_system.cpp
    src/instruction_set.cpp
    src/variable_system.cpp
    src/error_codes.cpp
    src/utils/binary_utils.cpp
    src/utils/validation.cpp
)

# Create library
add_library(libcoil-dev ${LIBCOIL_SOURCES})
target_include_directories(libcoil-dev 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set output name without "lib" prefix on Unix
set_target_properties(libcoil-dev PROPERTIES 
    OUTPUT_NAME "coil-dev"
)

# Install library
install(TARGETS libcoil-dev
    EXPORT libcoil-dev-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/
    DESTINATION include
)

# Configure and install CMake package files
install(EXPORT libcoil-dev-targets
    FILE libcoil-dev-targets.cmake
    NAMESPACE libcoil::
    DESTINATION lib/cmake/libcoil-dev
)

# Enable testing if requested
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()