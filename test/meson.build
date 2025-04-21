test_sources = [
  'test_framework.c',
  'test_arena.c',
  'test_error.c',
  'test_instr.c',
  'test_obj.c',
  'test_coil.c',
  'test_main.c',
]

test_exe = executable(
  'coil_tests',
  test_sources,
  include_directories: [inc_dir, '.'],
  dependencies: [coil_dep],
  c_args: ['-Wall', '-Wextra', '-Werror'],
  install: false
)

test('coil_tests', test_exe)

# Add individual test targets for convenience
test('arena_tests', test_exe, args: ['arena'])
test('error_tests', test_exe, args: ['error'])
test('instr_tests', test_exe, args: ['instr'])
test('obj_tests', test_exe, args: ['obj'])
test('coil_tests', test_exe, args: ['coil'])