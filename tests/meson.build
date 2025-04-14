# Test configuration
test_sources = [
  'test_version.cpp',
  'test_log.cpp',
  'test_err.cpp',
  'test_stream.cpp',
  'test_obj.cpp',
]

# Include catch2-with-main
catch2_main_dep = dependency('catch2-with-main', required : false)
if not catch2_main_dep.found()
  # Fallback: try to use catch2 as a subproject with main
  catch2_proj = subproject('catch2', required : false)
  if catch2_proj.found()
    catch2_main_dep = catch2_proj.get_variable('catch2_with_main_dep')
  else
    # Second fallback: try to use catch2-with-main from system
    catch2_main_dep = declare_dependency(
      dependencies: [dependency('catch2-with-main', required : true)]
    )
  endif
endif

# Build the test executable
test_exe = executable('coil_tests',
  test_sources,
  dependencies : [coil_dep, catch2_main_dep],
  include_directories : incdir
)

# Register tests
test('COIL Library Tests', test_exe, protocol: 'tap')