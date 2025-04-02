project('libcoil-dev', 'cpp',
  version : '1.0.0',
  default_options : ['cpp_std=c++17',
                     'warning_level=3'])

# Source files
libcoil_sources = [
  'src/binary_format.cpp',
  'src/type_system.cpp',
  'src/instruction_set.cpp',
  'src/variable_system.cpp',
  'src/error_codes.cpp',
  'src/utils/binary_utils.cpp',
  'src/utils/validation.cpp'
]

# Include directories
inc = include_directories('include')

# Main library
libcoil = library('coil-dev',
  libcoil_sources,
  include_directories : inc,
  install : true)

# Install headers
install_subdir('include/coil', install_dir : 'include')

# Dependency for other projects
libcoil_dep = declare_dependency(
  include_directories : inc,
  link_with : libcoil)

# Tests
if get_option('build_tests')
  subdir('tests')
endif

# Generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name : 'libcoil-dev',
  description : 'COIL Toolchain Development Library',
  version : meson.project_version(),
  libraries : libcoil)

meson.override_dependency('libcoil-dev', libcoil_dep)