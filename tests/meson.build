cmocka_dep = dependency('cmocka', required: false)

if not cmocka_dep.found()
  message('cmocka not found, falling back to subproject')
  cmocka_proj = subproject('cmocka', required: false)
  if cmocka_proj.found()
    cmocka_dep = cmocka_proj.get_variable('cmocka_dep')
  else
    warning('cmocka not available, tests will not be built')
  endif
endif

if cmocka_dep.found()
  test_src = [
    'test_arena.c',
    'test_err.c',
    'test_instr.c',
    'test_obj.c',
    'test_coil.c',
    'test_integration.c',
  ]

  test_deps = [
    coil_dep,
    cmocka_dep,
  ]

  # Version defines
  version_defines = [
    '-DCOIL_VERSION_MAJOR=' + version_major.to_string(),
    '-DCOIL_VERSION_MINOR=' + version_minor.to_string(),
    '-DCOIL_VERSION_PATCH=' + version_patch.to_string(),
  ]

  foreach src : test_src
    test_name = src.split('.')[0]
    test_exe = executable(test_name,
      src,
      include_directories: inc_dir,
      dependencies: test_deps,
      c_args: version_defines,
    )
    
    test(test_name, test_exe)
  endforeach
endif