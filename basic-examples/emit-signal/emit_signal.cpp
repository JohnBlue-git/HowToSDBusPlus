#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <unistd.h>
#include <iostream>

int main() {
    // Connect to the system bus
    // auto bus = sdbusplus::bus::new_system();
    auto bus = sdbusplus::bus::new_default(); // or sdbusplus::bus::new_session();

    const char* object_path = "/com/example/Demo";
    const char* interface_name = "com.example.Demo";

    // Request a well-known name
    //bus.request_name("com.example.Demo");
    /*
    On session bus, your program can’t usually own well-known names without extra config.
    If you want to avoid the name ownership step or it fails:
    Do not request a well-known name on the session bus, or
    Use anonymous connection (don’t call request_name())
    */

    // Create a signal message
    auto signal = bus.new_signal(object_path, interface_name, "HelloSignal");

    // Append parameters (in this case, a single string)
    signal.append(std::string("Hello from sdbusplus!"));

    // Send the signal
    signal.signal_send();

    std::cout << "Signal sent.\n";
    return 0;
}
