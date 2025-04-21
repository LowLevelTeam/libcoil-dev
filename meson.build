project('coil', 'c',
  version : '0.1.0',
  default_options : ['warning_level=3', 'c_std=c11'],
  license : 'Unlicense',
  meson_version : '>=0.53.0'
)

# Project description
project_description = 'Computer Oriented Intermediate Language'

# Compiler flags
cc = meson.get_compiler('c')

# Include directories
inc_dir = include_directories('include')

# Source files
coil_sources = [
  'csrc/arena.c',
  'csrc/err.c',
  'csrc/obj.c',
  'csrc/instr.c',
]

# Headers
coil_headers = [
  'include/ccoil/arena.h',
  'include/ccoil/err.h',
  'include/ccoil/instr.h',
  'include/ccoil/obj.h',
]

# Build options
build_tests = get_option('build_tests')
enable_exceptions = get_option('enable_exceptions')
enable_rtti = get_option('enable_rtti')

# Define version numbers
version_array = meson.project_version().split('.')
version_major = version_array[0].to_int()
version_minor = version_array[1].to_int()
version_patch = version_array[2].to_int()

# Version defines
version_defines = [
  '-DCOIL_VERSION_MAJOR=' + version_major.to_string(),
  '-DCOIL_VERSION_MINOR=' + version_minor.to_string(),
  '-DCOIL_VERSION_PATCH=' + version_patch.to_string(),
]

# Create the library
coil_lib = library('coil',
  sources: coil_sources,
  include_directories: inc_dir,
  c_args: version_defines,
  install: true,
)

# Dependency for other projects
coil_dep = declare_dependency(
  include_directories: inc_dir,
  link_with: coil_lib,
)

# Install headers
install_headers(coil_headers, subdir: 'ccoil')

# Build tests if enabled
if build_tests
  subdir('tests')
endif

# PKG config
pkg = import('pkgconfig')
pkg.generate(
  coil_lib,
  description: project_description,
  name: 'libcoil',
  filebase: 'coil',
  version: meson.project_version(),
)

# Summary
summary = [
  '',
  'COIL build configuration:',
  '  Version:          @0@'.format(meson.project_version()),
  '  Build tests:      @0@'.format(build_tests),
  '',
]
message('\n'.join(summary))