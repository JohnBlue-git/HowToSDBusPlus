
## How to build

Pe-install
[build-from-source-sdbusplus.md](build-from-source-sdbusplus.md)

The [sdbusplus] library builds on top of the [sd-bus] library to create a modern C++ API for D-Bus. The library attempts to be as lightweight as possible, usually compiling to exactly the sd-bus API calls that would have been necessary, while also providing compile-time type-safety and memory leak protection afforded by modern C++ practices.

[sdbusplus]: https://github.com/openbmc/sdbusplus
[sd-bus]: http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html

\
To build examples
```console
meson setup build
cd build && ninja
```

## Examples and How to use

## Start dbus if it is not yet started
```bash
suudo systemctl start dbus
sudo service dbus start
```

### simple-dbuscall
It is an exmple to create a dbus call (with timeout)
```bash
cd simple_dbuscall
./simple_dbuscall
./simple_dbuscall_timeout
```

### Still organizing ...
- asio-example
- calculator
- coroutine-example
- get-all-properties
- list-users
- register-property


