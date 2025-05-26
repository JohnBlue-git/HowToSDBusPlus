## How to use
```bash
cd simple_dbuscall
./simple_dbuscall
./simple_dbuscall_timeout
```

## Equivalent dbus command
```bash
busctl call -j \
org.freedesktop.DBus \
/org/freedesktop/DBus \
org.freedesktop.DBus.Properties \
Get ss \
"org.freedesktop.DBus" "Interfaces"
```