# Main test executable
simple_test = executable('simple_test',
  'simple_test.cpp',
  dependencies : [libcoil_dep])

# Binary format tests
binary_format_test = executable('binary_format_test',
  'binary_format_test.cpp',
  dependencies : [libcoil_dep])

# Type system tests
type_system_test = executable('type_system_test',
  'type_system_test.cpp',
  dependencies : [libcoil_dep])

# Instruction set tests
instruction_set_test = executable('instruction_set_test',
  'instruction_set_test.cpp',
  dependencies : [libcoil_dep])

# Variable system tests
variable_system_test = executable('variable_system_test',
  'variable_system_test.cpp',
  dependencies : [libcoil_dep])

# Error system tests
error_system_test = executable('error_system_test',
  'error_system_test.cpp',
  dependencies : [libcoil_dep])

# Validation tests
validation_test = executable('validation_test',
  'validation_test.cpp',
  dependencies : [libcoil_dep])

# Integration tests
integration_test = executable('integration_test',
  'integration_test.cpp',
  dependencies : [libcoil_dep])

# Register all tests
test('simple_test', simple_test)
test('binary_format_test', binary_format_test)
test('type_system_test', type_system_test)
test('instruction_set_test', instruction_set_test)
test('variable_system_test', variable_system_test)
test('error_system_test', error_system_test)
test('validation_test', validation_test)
test('integration_test', integration_test)

# Examples are built but not automatically tested
# subdir('examples')