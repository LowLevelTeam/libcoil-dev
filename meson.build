project('coil', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
  meson_version : '>= 0.56.0'
)

# Include directories
incdir = include_directories('include')

# Library sources
coil_src = [
  'src/coil.cpp',
  'src/log.cpp',
  'src/err.cpp',
  'src/stream.cpp'
]

# Build the library with optimization flags
compiler = meson.get_compiler('cpp')
add_project_arguments('-fno-exceptions', '-fno-rtti', language : 'cpp')

# Build the library
coil_lib = library('coil',
  coil_src,
  include_directories : incdir,
  install : true
)

# Create dependency for test code
coil_dep = declare_dependency(
  include_directories : incdir,
  link_with : coil_lib
)

# Generate pkg-config file
pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries : coil_lib,
  version : '0.1.0',
  name : 'coil',
  filebase : 'coil',
  description : 'Zero-cost library for reading and writing data streams'
)

# Tests could be added in a subdirectory if needed
# subdir('tests')