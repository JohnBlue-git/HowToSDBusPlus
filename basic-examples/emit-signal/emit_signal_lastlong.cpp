/*
Here are possible example to last long a service while regest signal onto the interface

But have to use sdbus++ and ..., it is hard to develop

Also, we cannot "call" or "invoke" a signal because signals are one-way asynchronous notifications, not methods.
"Signals" are emitted by services (servers), not called by clients.
"busctl emit" lets you emit a signal manually on the bus (as a client), but it does not call or trigger an existing signal on a server interface.
"busctl emit" lets you simulate emitting a signal on the bus manually.
*/

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/interface.hpp>

#include <iostream>

int main() {
    auto bus = sdbusplus::bus::new_default();

    // Create server interface on object path /com/example/Demo/Object
    sdbusplus::server::interface::interface iface(
        bus,
        "/com/example/Demo/Object",
        "com.example.Demo.Interface",
        nullptr, nullptr);

    // Define a signal emitter function (lambda)
    auto emit_hello_signal = [&iface]() {
        // Emit a signal named "HelloSignal" with a string argument
        iface.emit_signal("HelloSignal", std::string("Hello from sdbusplus"));
    };

    std::cout << "Emitting signal..." << std::endl;
    emit_hello_signal();

    // Wait so introspection can be done, and process events
    while (true) {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
