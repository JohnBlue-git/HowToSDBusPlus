#include <sdbusplus/bus.hpp>
#include <iostream>

int main() {
    auto bus = sdbusplus::bus::new_default();
    std::string busName = "org.freedesktop.DBus";
    std::string objectPath = "/org/freedesktop/DBus";
    std::string interfaceName = "org.freedesktop.DBus";
    std::string propertyName = "Interfaces";

    // Create a method call to the Get method of the org.freedesktop.DBus.Properties interface
    auto method = bus.new_method_call(
        busName.c_str(), 
        objectPath.c_str(), 
        "org.freedesktop.DBus.Properties", 
        "Get");

    // Append the interface name and property name to the method call
    method.append(interfaceName);
    method.append(propertyName);

    try {
        // Call the method with timeout and read the response
        auto reply = bus.call(method);
        std::variant<std::vector<std::string>> propertyValue;
        reply.read(propertyValue);

        // Print the retrieved property
        auto value = std::get<std::vector<std::string>>(propertyValue);
        for (const auto& iface : value) {
            std::cout << iface << std::endl;
        }
    } catch (const sdbusplus::exception::SdBusError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
