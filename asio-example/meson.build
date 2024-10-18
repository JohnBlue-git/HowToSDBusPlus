executable(
    'asio-example',
    'asio-example.cpp',
    dependencies: [
        asio_dep,
        dependency(
            'boost',
            modules: ['coroutine', 'context'],
            disabler: true,
            required: false,
        ),
    ],
)

# my executable

executable(
    'my_asio_server',
    'my_asio_server.cpp',
    dependencies: [
        asio_dep,
        dependency(
            'boost',
            modules: ['coroutine', 'context'],
            disabler: true,
            required: false,
        ),
    ],
)