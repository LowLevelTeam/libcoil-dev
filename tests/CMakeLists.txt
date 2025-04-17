# Test configuration
set(TEST_SOURCES
  test_coil.cpp
  test_err.cpp
  test_stream.cpp
  test_obj.cpp
  test_instr.cpp
)

# Build the test executable
add_executable(coil_tests ${TEST_SOURCES})
target_link_libraries(coil_tests PRIVATE coil Catch2::Catch2WithMain)

# Register tests with CTest
include(Catch)
catch_discover_tests(coil_tests)