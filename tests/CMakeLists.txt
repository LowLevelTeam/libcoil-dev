# Tests for libcoil-dev
include_directories(${CMAKE_SOURCE_DIR}/include)

# Simple test
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE libcoil-dev)

# Add to CTest
add_test(NAME simple_test COMMAND simple_test)

# Future tests to implement
# TODO: Add unit tests for each component
# - Binary format tests
# - Type system tests 
# - Instruction set tests
# - Variable system tests
# - Error system tests
# - Validation tests

# Create separate test executables for each component
# add_executable(binary_format_test binary_format_test.cpp)
# target_link_libraries(binary_format_test PRIVATE libcoil-dev)
# add_test(NAME binary_format_test COMMAND binary_format_test)

# add_executable(type_system_test type_system_test.cpp)
# target_link_libraries(type_system_test PRIVATE libcoil-dev)
# add_test(NAME type_system_test COMMAND type_system_test)

# add_executable(instruction_set_test instruction_set_test.cpp)
# target_link_libraries(instruction_set_test PRIVATE libcoil-dev)
# add_test(NAME instruction_set_test COMMAND instruction_set_test)

# add_executable(variable_system_test variable_system_test.cpp)
# target_link_libraries(variable_system_test PRIVATE libcoil-dev)
# add_test(NAME variable_system_test COMMAND variable_system_test)