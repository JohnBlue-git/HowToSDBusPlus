
generated_sources = []
generated_others = []
subdir('gen')
subdir('gen/net')

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




