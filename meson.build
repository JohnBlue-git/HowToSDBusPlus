
project('my-sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++23',
      'warning_level=3',
      'werror=true',
    ],
    version: '1.0.0',
    meson_version: '>=1.1.1',
)

libsystemd_pkg = dependency('libsystemd')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
sdbusplus_pkg = dependency('sdbusplus', required: true)

python = import('python')
python_bin = python.find_installation('python3', modules:['inflection', 'yaml', 'mako'])

if not python_bin.found()
  error('No valid python3 installation found')
endif

boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

boost_dep = declare_dependency(
    dependencies: dependency('boost', modules: ['coroutine', 'context']),
    compile_args: boost_compile_args)

root_inc = include_directories('/usr/include', '/usr/local/include')
sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    dependencies: [
        boost_dep,
        libsystemd_pkg,
        nlohmann_json_dep,
        sdbusplus_pkg,
    ],
)



# build tools

subdir('tools')

# build example ...


# check async ...

has_asio = meson.get_compiler('cpp').has_header_symbol(
  'boost/asio.hpp',
  'boost::asio::io_context',
  required: false)
asio_dep = sdbusplus_dep
if not has_asio
  asio_dep = disabler()
endif

assert(
    #not get_option('examples').allowed() or has_asio,
    has_asio,
    'Boost is required when ... are enabled'
)

# build async example ...

# build YAML-generated examples

if not get_option('generated-via-yaml-examples').disabled()
  subdir('generated-via-yaml-examples')
endif

# build basic examples

if not get_option('basic-examples').disabled()
  subdir('basic-examples')
endif

# build benchmark examples

if not get_option('benchmark-examples').disabled()
  subdir('benchmark-examples')
endif
