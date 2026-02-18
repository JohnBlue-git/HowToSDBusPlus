
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




