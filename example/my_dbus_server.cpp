
/*

g++ -o my_dbus_server my_dbus_server.cpp -lsdbusplus -pthread --std=c++20

./my_dbus_server

dbus-send --print-reply --dest=com.example.MyDBusObject /com/example/MyDBusObject com.example.MyDBusInterface.GetStatus


*/


#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <iostream>
#include <string>

class MyDBusObject
{
public:
    MyDBusObject(sdbusplus::bus::bus& bus, const char* path) :
        m_bus(bus),
        m_object(path),
        m_interface("com.example.MyDBusInterface"),
        m_property("Status", "Initial status")
    {
        // Register the object with D-Bus
        m_object.initialize(m_bus, m_interface.c_str());

        // Set up the property and method
        m_object.register_property("Status", m_property);
        m_object.register_method("GetStatus", &MyDBusObject::getStatus);
    }

    // Method to get the status
    std::string getStatus() {
        return m_property;
    }

    // Property setter
    void setStatus(const std::string& status) {
        m_property = status;
        m_object.set_property("Status", m_property);
    }

private:
    sdbusplus::bus::bus& m_bus;
    sdbusplus::server::object::object<> m_object;
    std::string m_interface;
    std::string m_property;
};

int main()
{
    // Create a D-Bus connection
    auto bus = sdbusplus::bus::new_system();

    // Create the object
    MyDBusObject myObject(bus, "/com/example/MyDBusObject");

    // Request the name on the D-Bus
    bus.request_name("com.example.MyDBusObject");

    // Main loop to process incoming D-Bus messages
    while (true) {
        bus.process_discard(); // Process any incoming messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep to prevent busy waiting
    }

    return 0;
}

