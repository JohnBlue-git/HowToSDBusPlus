
# --- Build Executable ---
executable(
    'boost_asio_caculator',
    'boost_asio_caculator.cpp',
    dependencies: sdbusplus_dep,
    install: true
)
executable(
    'boost_asio_crtp_caculator',
    'boost_asio_crtp_caculator.cpp',
    dependencies: sdbusplus_dep,
    install: true
)
executable(
    'sdbusplus_async_caculator',
    'sdbusplus_async_caculator.cpp',
    dependencies: sdbusplus_dep,
    install: true
)
executable(
    'sdbusplus_async_crtp_caculator',
    'sdbusplus_async_crtp_caculator.cpp',
    dependencies: sdbusplus_dep,
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