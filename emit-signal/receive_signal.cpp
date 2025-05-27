#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <iostream>
#include <string>

int main() {
    // Connect to the session bus
    auto bus = sdbusplus::bus::new_default();

    // Match rule
    const std::string matchRule =
        "type='signal',"
        "interface='com.example.Demo',"
        "member='HelloSignal',"
        "path='/com/example/Demo'";
    // Match
    sdbusplus::bus::match::match helloMatch(
        bus,
        matchRule,
        [](sdbusplus::message::message& msg) {
            std::string message;
            msg.read(message);
            std::cout << "Received signal: " << message << std::endl;
            });

    std::cout << "Listening for HelloSignal..." << std::endl;

    // Run the event loop
    while (true) {
        bus.process_discard();
        bus.wait();
    }

    return 0;
}
