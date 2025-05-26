## How to use
```bash
# to monitor
dbus-monitor --session "type='signal',interface='com.example.Demo'"
dbus-monitor --system "type='signal',interface='com.example.Demo'"

# run
./emit_signal
sudo ./emit_signal
```
