project('my-sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++23',
      'warning_level=3',
      'werror=false', 
    ],
    version: '1.0.0',
    meson_version: '>=1.1.1',
)

# --- Dependencies ---
# These are the core system libraries needed for sdbusplus to function
libsystemd_pkg = dependency('libsystemd')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
sdbusplus_pkg = dependency('sdbusplus', required: true)
boost_dep = dependency('boost', required: true)

# --- Boost Configuration ---
# Since you're likely using sdbusplus::asio, these flags are standard
boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

# --- Internal Dependency Wrapper ---
# This bundles everything together for the executable
sdbusplus_dep = declare_dependency(
    dependencies: [
        boost_dep,
        libsystemd_pkg,
        nlohmann_json_dep,
        sdbusplus_pkg,
    ],
    compile_args: boost_compile_args
)

# --- Build Executable ---
# Only compiles main.cpp; no YAML or generated headers included.
executable(
    'calculator-server',
    'main.cpp',
    dependencies: sdbusplus_dep,
    install: true
)