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
  'src/arena.c',
  'src/err.c',
  'src/obj.c',
  'src/instr.c',
  'src/coil.c',
]

# Headers
coil_headers = [
  'include/coil/arena.h',
  'include/coil/err.h',
  'include/coil/instr.h',
  'include/coil/obj.h',
  'include/coil/coil.h',
]

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

# PKG config
pkg = import('pkgconfig')
pkg.generate(
  coil_lib,
  description: project_description,
  name: 'libcoil',
  filebase: 'coil',
  version: meson.project_version(),
)

# Build tests
subdir('test')