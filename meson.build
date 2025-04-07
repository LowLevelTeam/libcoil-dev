project('libcoil', 'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
    'werror=true',
  ],
  license: 'MIT',
)

# Dependencies
thread_dep = dependency('threads')

# Include directories
inc_dir = include_directories('include')

# Source files
sources = [
  'src/types/type_system.cpp',
  'src/types/fixed_width_types.cpp',
  'src/types/vector_types.cpp',
  'src/types/matrix_types.cpp',
  'src/types/complex_types.cpp',
  'src/types/composite_types.cpp',
  
  'src/binary/binary_format.cpp',
  'src/binary/instruction_encoder.cpp',
  'src/binary/instruction_decoder.cpp',
  'src/binary/binary_reader.cpp',
  'src/binary/binary_writer.cpp',
  
  'src/isa/universal_instructions.cpp',
  'src/isa/extended_instructions.cpp',
  'src/isa/compiler_instructions.cpp',
  'src/isa/instruction_registry.cpp',
  
  'src/object/object_format.cpp',
  'src/object/section.cpp',
  'src/object/symbol.cpp',
  'src/object/relocation.cpp',
  'src/object/object_builder.cpp',
  'src/object/object_reader.cpp',
  
  'src/linker/linker.cpp',
  'src/linker/section_merger.cpp',
  'src/linker/symbol_resolver.cpp',
  'src/linker/relocation_processor.cpp',
  
  'src/util/endian.cpp',
  'src/util/error_handler.cpp',
  'src/util/logger.cpp',
]

# Core library
libcoil = library('coil',
  sources,
  include_directories: inc_dir,
  dependencies: [thread_dep],
  install: true,
  soversion: '0',
  version: meson.project_version(),
)

# Library dependency for internal use
libcoil_dep = declare_dependency(
  include_directories: inc_dir,
  link_with: libcoil,
)

# Tests
if get_option('build_tests')
  gtest_dep = dependency('gtest', main: true, required: false)
  gmock_dep = dependency('gmock', required: false)
  
  if gtest_dep.found() and gmock_dep.found()
    test_sources = [
      'tests/types/type_system_test.cpp',
      'tests/binary/binary_format_test.cpp',
      'tests/isa/instruction_test.cpp',
      'tests/object/object_format_test.cpp',
      'tests/linker/linker_test.cpp',
    ]
    
    test_exe = executable('coil_tests',
      test_sources,
      dependencies: [libcoil_dep, gtest_dep, gmock_dep],
    )
    
    test('coil unit tests', test_exe)
  else
    warning('GTest/GMock not found, tests will not be built')
  endif
endif

# Examples
if get_option('build_examples')
  executable('simple_example',
    'examples/simple_example.cpp',
    dependencies: libcoil_dep,
  )
  
  executable('object_reader',
    'examples/object_reader.cpp',
    dependencies: libcoil_dep,
  )
endif

# Installation
install_headers(
  'include/coil/binary_format.h',
  'include/coil/instruction.h',
  'include/coil/object_file.h',
  'include/coil/type_system.h',
  'include/coil/linker.h',
  subdir: 'coil',
)

# Generate pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  name: 'libcoil',
  description: 'Library for COIL (Computer Oriented Intermediate Language)',
  version: meson.project_version(),
  libraries: libcoil,
)

# Meson options
option('build_tests', type: 'boolean', value: true, description: 'Build unit tests')
option('build_examples', type: 'boolean', value: true, description: 'Build examples')