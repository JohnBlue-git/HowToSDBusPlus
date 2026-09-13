
benchmark_examples_opt_mode = get_option('benchmark-examples-opt-mode')
benchmark_examples_override_options = []

if benchmark_examples_opt_mode == 'O2'
    benchmark_examples_override_options += ['optimization=2']
elif benchmark_examples_opt_mode == 'O3'
    benchmark_examples_override_options += ['optimization=3']
endif

# --- Build Executable ---
executable(
    'non_async_caculator',
    'source/non-async/non_async_caculator.cpp',
    include_directories: include_directories('include'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)
executable(
    'boost_asio_caculator',
    'source/async/boost_asio_caculator.cpp',
    include_directories: include_directories('include'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)
executable(
    'boost_asio_crtp_caculator',
    'source/crtp/boost_asio_crtp_caculator.cpp',
    include_directories: include_directories('include'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)
executable(
    'sdbusplus_async_caculator',
    'source/async/sdbusplus_async_caculator.cpp',
    include_directories: include_directories('include'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)
executable(
    'sdbusplus_async_crtp_caculator',
    'source/crtp/sdbusplus_async_crtp_caculator.cpp',
    include_directories: include_directories('include'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)

executable(
    'sdbusplus_async_sleep_crtp_caculator',
    'source/async-crtp/sdbusplus_async_sleep_crtp_caculator.cpp',
    include_directories: include_directories('include', 'source/crtp'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)

# --- Build Yaml Generated Executable ---
sdbusplusplus_prog = find_program('sdbus++', native: true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)
yaml_generated_sources = []
subdir('yaml/xyz')
executable(
    'yaml_generated_crtp_caculator',
    'source/crtp/yaml_generated_crtp_caculator.cpp',
    yaml_generated_sources,
    include_directories: include_directories('include', 'yaml', 'source/crtp'),
    dependencies: sdbusplus_dep,
    cpp_args: ['-Wno-subobject-linkage'],
    override_options: benchmark_examples_override_options,
    install: true
)
executable(
    'yaml_generated_sleep_crtp_caculator',
    'source/async-crtp/yaml_generated_sleep_crtp_caculator.cpp',
    yaml_generated_sources,
    include_directories: include_directories('include', 'yaml', 'source/crtp'),
    dependencies: sdbusplus_dep,
    override_options: benchmark_examples_override_options,
    install: true
)

# --- Install Configuration ---
# Define the installation directory for D-Bus system policies
#dbus_dep = dependency('dbus-1')
#dbus_policy_dir = dbus_dep.get_variable(pkgconfig: 'sysconfdir') / 'dbus-1' / 'system.d'

# If pkgconfig isn't available or you want to hardcode the standard path:
dbus_policy_dir = '/etc/dbus-1/system.d'

install_data(
    'my-calculator.conf',
    install_dir: dbus_policy_dir
)