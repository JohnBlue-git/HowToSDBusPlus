# D-Bus Usage

This document collects common commands for inspecting and exercising D-Bus
services in the examples. Replace the service, object path, and interface with
the values used by the example you are running.

## Start Or Reload D-Bus

```bash
sudo systemctl start dbus
sudo systemctl reload dbus
```

On systems without systemd service management:

```bash
sudo service dbus start
sudo service dbus reload
```

## busctl

List the object tree:

```bash
busctl tree org.freedesktop.DBus
```

Inspect an interface:

```bash
busctl introspect org.freedesktop.DBus /org/freedesktop/DBus
```

Call a method:

```bash
busctl call \
  org.freedesktop.DBus \
  /org/freedesktop/DBus \
  org.freedesktop.DBus \
  ListNames
```

Read a property:

```bash
busctl get-property -j \
  org.freedesktop.DBus \
  /org/freedesktop/DBus \
  org.freedesktop.DBus \
  Interfaces
```

Set a property:

```bash
sudo busctl call \
  <com.example.Service> \
  </com/example/Object> \
  org.freedesktop.DBus.Properties \
  Set ssv \
  "<com.example.Interface>" "<ExampleProperty>" "s" "<new value>"
```

Monitor signals:

```bash
busctl monitor \
  --match="type='signal',interface='org.freedesktop.DBus',member='NameOwnerChanged'"
```

Emit a signal manually:

```bash
busctl emit \
  <com.example.SignalTest> \
  </com/example/SignalTest> \
  <com.example.SignalTest> \
  <SignalMember> \
  s "Hello from busctl"
```

## gdbus

Introspect a system-bus object:

```bash
gdbus introspect --system \
  --dest org.freedesktop.DBus \
  --object-path /org/freedesktop/DBus
```

## dbus-send

Call the standard `Properties.Get` method:

```bash
dbus-send --system \
  --dest=org.freedesktop.DBus \
  --print-reply \
  --type=method_call \
  /org/freedesktop/DBus \
  org.freedesktop.DBus.Properties.Get \
  string:"org.freedesktop.DBus" \
  string:"Interfaces"
```

## Calculator Example

The calculator services use:

```bash
SERVICE=xyz.openbmc_project.Calculator
INTERFACE=xyz.openbmc_project.Calculator
OBJECT=/xyz/openbmc_project/calculator/decimal
```

Start one of the calculator services first, then inspect and call it:

```bash
busctl introspect "$SERVICE" "$OBJECT"
busctl call "$SERVICE" "$OBJECT" "$INTERFACE" Multiply xx 6 7
busctl call "$SERVICE" "$OBJECT" "$INTERFACE" Divide xx 20 5
busctl call "$SERVICE" "$OBJECT" "$INTERFACE" Express
busctl get-property "$SERVICE" "$OBJECT" "$INTERFACE" LastResult
busctl call "$SERVICE" "$OBJECT" "$INTERFACE" Clear
```

CRTP and YAML-generated calculator services also expose these object paths:

```text
/xyz/openbmc_project/calculator/decimal
/xyz/openbmc_project/calculator/binary
/xyz/openbmc_project/calculator/heximal
```
