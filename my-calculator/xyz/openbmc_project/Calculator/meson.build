sdbusplusplus_depfiles = files(
    '../Calculator.interface.yaml',
    '../Calculator.events.yaml',
)

yaml_generated_sources += custom_target(
    'my_calculator_yaml_generated_cpp',
    input: sdbusplusplus_depfiles,
    output: [
        'event.cpp',
        'event.hpp',
        'common.hpp',
        'server.hpp',
        'server.cpp',
        'aserver.hpp',
        'client.hpp',
    ],
    depend_files: sdbusplusplus_depfiles,
    command: [
        sdbuspp_gen_meson_prog,
        '--command', 'cpp',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../',
        'xyz/openbmc_project/Calculator',
    ],
)
