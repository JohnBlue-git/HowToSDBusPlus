
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
    dependencies: dependency('boost', required: false),
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

if not get_option('simple-dbuscall').disabled()
  subdir('simple-dbuscall')
endif

if not get_option('use-systemd1').disabled()
  subdir('use-systemd1')
endif

if not get_option('emit-signal').disabled()
  subdir('emit-signal')
endif

if not get_option('list-users').disabled()
  subdir('list-users')
endif

if not get_option('coroutine-example').disabled()
  subdir('coroutine-example')
endif



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

if not get_option('asio-example').disabled()
  subdir('asio-example')
endif

if not get_option('register-property').disabled()
  subdir('register-property')
endif

if not get_option('get-all-properties').disabled()
  subdir('get-all-properties')
endif


# build (async) example with gen ...

if not get_option('calculator').disabled()
  subdir('calculator')
endif
