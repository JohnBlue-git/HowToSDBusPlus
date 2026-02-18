
## How to build

Pe-install
[build-from-source-sdbusplus.md](build-from-source-sdbusplus.md)

Another quick choice is to use docker image created via [Dockerfile](Dockerfile)
```console
# run
docker run -it johnbluedocker/sdbusplus-dev

# Mounts your current local directory into /workspace inside the container
docker run -it \
  -v $(pwd):/workspace \
  johnbluedocker/sdbusplus-dev \
  /bin/bash

# exit
exit
Crtl + D
```

The [sdbusplus] library builds on top of the [sd-bus] library to create a modern C++ API for D-Bus. The library attempts to be as lightweight as possible, usually compiling to exactly the sd-bus API calls that would have been necessary, while also providing compile-time type-safety and memory leak protection afforded by modern C++ practices.

[sdbusplus]: https://github.com/openbmc/sdbusplus
[sd-bus]: http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html

\
To build examples
```console
meson setup build --wipe
cd build
ninja -j2
```


## Here are common dbus commands to check whether the proprams work as expected

### busctl
```bash
# to tree object running
busctl tree org.freedesktop.DBus

# to introspect interfaces. methods, signals, properties
busctl introspect org.freedesktop.DBus /org/freedesktop/DBus

# to call
busctl call -j \
org.freedesktop.DBus \
/org/freedesktop/DBus \
org.freedesktop.DBus.Properties \
Get ss \
"org.freedesktop.DBus" "Interfaces"
{
        "type" : "v",
        "data" : [
                {
                        "type" : "as",
                        "data" : [
                                "org.freedesktop.DBus.Monitoring",
                                "org.freedesktop.DBus.Debug.Stats"
                        ]
                }
        ]
}

# to get property
busctl get-property -j \
org.freedesktop.DBus \
/org/freedesktop/DBus \
org.freedesktop.DBus \
Interfaces
{
        "type" : "as",
        "data" : [
                "org.freedesktop.DBus.Monitoring",
                "org.freedesktop.DBus.Debug.Stats"
        ]
}

# to emit signal
busctl emit \
  <com.example.SignalTest> \
  </com/example/SignalTest> \
  <com.example.SignalTest> \
  <...> \
  s "Hello from busctl"

# to set property then signal will emerge
sudo busctl call \
<com.example.Service> \
</com/example/Object> \
org.freedesktop.DBus.Properties \
Set ssv \
"<com.example.Interface>" "<ExampleProperty>" "s" "<new value>"

# to call something then signal will emerge
# (this will cause your client to temporarily own a name, triggering the signal)
busctl call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ListNames
# to send then signal will emerge
#dbus-send --session --dest=org.freedesktop.DBus --type=method_call \
#  --print-reply /org/freedesktop/DBus org.freedesktop.DBus.ListNames

# to monitor signal
busctl monitor \
  --match="type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'"
```

### dbus-send
```bash
# no tree method provided

# introspect
gdbus introspect --system \
  --dest org.freedesktop.DBus \
  --object-path /org/freedesktop/DBus

# to call
dbus-send --system \
  --dest=org.freedesktop.DBus \
  --print-reply \
  --type=method_call \
  /org/freedesktop/DBus \
  org.freedesktop.DBus.Properties.Get \
  string:"org.freedesktop.DBus" \
  string:"Interfaces"

# to get property
dbus-send --system \
  --dest=org.freedesktop.DBus \
  --print-reply \
  --type=method_call \
  /org/freedesktop/DBus \
  org.freedesktop.DBus.Properties.Get \
  string:"org.freedesktop.DBus" \
  string:"Interfaces"

# no signal method
```

## Examples and How to use

## Start dbus if it is not yet started
```bash
# normally
sudo systemctl start dbus
# or
sudo service dbus start
```

### Example: [simple-dbuscall](simple-dbuscall/README.md)

### Example: [use-systemd1](use-systemd1/README.md)

### Example: [emit-signal](emit-signal/README.md)

### Still organizing ...
- asio-example
- calculator
- coroutine-example
- get-all-properties
- list-users
- register-property
