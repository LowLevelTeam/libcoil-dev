project('coil', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++17',
    'buildtype=debugoptimized',
  ],
  meson_version : '>= 0.56.0'
)

# Dependencies
thread_dep = dependency('threads')
gtest_dep = dependency('gtest', fallback : ['gtest', 'gtest_dep'])
gmock_dep = dependency('gmock', fallback : ['gtest', 'gmock_dep'])

# Include directories
incdir = include_directories('include')

# Library sources
coil_src = [
  'src/coil.cpp',
  'src/log.cpp',
  'src/err.cpp',
  'src/mem.cpp',
  'src/args.cpp',
  'src/stream.cpp',
  'src/thread.cpp'
]

# Build the library
coil_lib = library('coil',
  coil_src,
  include_directories : incdir,
  dependencies : [thread_dep],
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
  description : 'Library for reading and writing COIL formats'
)

# Subdirectories
subdir('tests')