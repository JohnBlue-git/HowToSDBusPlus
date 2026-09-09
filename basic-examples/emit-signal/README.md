## How to use
```bash
# to monitor
dbus-monitor --session "type='signal',interface='com.example.Demo'"
dbus-monitor --system "type='signal',interface='com.example.Demo'"

# run receiver
./receive_signal
sudo ./receive_signal

# run sender
./emit_signal
sudo ./emit_signal
```
