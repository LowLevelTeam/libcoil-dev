test_sources = [
  'test_log.cpp',
  'test_err.cpp',
  'test_mem.cpp',
  'test_args.cpp',
  'test_stream.cpp',
  'test_thread.cpp',
  'test_coil.cpp'
]

foreach test_source : test_sources
  test_name = test_source.split('.')[0]
  test_exe = executable(
    test_name,
    test_source,
    dependencies : [coil_dep, gtest_dep, gmock_dep],
    include_directories : incdir
  )
  test(test_name, test_exe)
endforeach