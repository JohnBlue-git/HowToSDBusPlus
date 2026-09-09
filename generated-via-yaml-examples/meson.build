
sdbusplusplus_prog = find_program('sdbus++', native: true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)

should_generate_cpp = true
should_generate_markdown = false
should_generate_registry = false

generated_sources = []
generated_others = []
yaml_selected_subdirs = ['net']
subdir('gen')

#custom_target(
#    'example_md',
#    command: 'true',
#    output: 'example_md',
#    capture: true,
#    depends: generated_others,
#    build_by_default: true)

executable(
    'calculator-server',
    'calculator-server.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)

executable(
    'calculator-aserver',
    'calculator-aserver.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)

executable(
    'calculator-client',
    'calculator-client.cpp',
    generated_sources,
    implicit_include_directories: false,
    include_directories: include_directories('gen'),
    dependencies: sdbusplus_dep,
)



# Define the installation directory for D-Bus system policies
#dbus_dep = dependency('dbus-1')
#dbus_policy_dir = dbus_dep.get_variable(pkgconfig: 'sysconfdir') / 'dbus-1' / 'system.d'

# If pkgconfig isn't available or you want to hardcode the standard path:
dbus_policy_dir = '/etc/dbus-1/system.d'

install_data(
    'calculator.conf',
    install_dir: dbus_policy_dir
)
