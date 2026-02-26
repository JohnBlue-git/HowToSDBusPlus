
my_calculator_opt_mode = get_option('my-calculator-opt-mode')
my_calculator_override_options = []

if my_calculator_opt_mode == 'O2'
    my_calculator_override_options += ['optimization=2']
elif my_calculator_opt_mode == 'O3'
    my_calculator_override_options += ['optimization=3']
endif

# --- Build Executable ---
executable(
    'non_async_caculator',
    'non_async_caculator.cpp',
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
    install: true
)
executable(
    'boost_asio_caculator',
    'boost_asio_caculator.cpp',
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
    install: true
)
executable(
    'boost_asio_crtp_caculator',
    'boost_asio_crtp_caculator.cpp',
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
    install: true
)
executable(
    'sdbusplus_async_caculator',
    'sdbusplus_async_caculator.cpp',
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
    install: true
)
executable(
    'sdbusplus_async_crtp_caculator',
    'sdbusplus_async_crtp_caculator.cpp',
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
    install: true
)

# --- Build Yaml Generated Executable ---
sdbusplusplus_prog = find_program('sdbus++', native: true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)
yaml_generated_sources = []
subdir('xyz')
executable(
    'yaml_generated_caculator',
    'yaml_generated_caculator.cpp',
    yaml_generated_sources,
    include_directories: include_directories('.'),
    dependencies: sdbusplus_dep,
    override_options: my_calculator_override_options,
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