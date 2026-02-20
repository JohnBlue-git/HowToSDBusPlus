
## The C++ Service Implementation

This code uses the `sdbusplus::asio` or standard `sdbusplus` object server patterns to register the interface described in your YAML.

```cpp
#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/object_server.hpp>

// Constants based on YAML
const char* SERVICE_NAME = "xyz.openbmc_project.Calculator";
const char* OBJECT_PATH = "/xyz/openbmc_project/calculator";
const char* INTERFACE_NAME = "xyz.openbmc_project.Calculator";

int main() {
    // 1. Setup Connection
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io, sdbusplus::bus::new_system());
    conn->request_name(SERVICE_NAME);

    // 2. Create the Object Server
    auto server = sdbusplus::asio::object_server(conn);
    
    // 3. Register the Interface
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface = 
        server.add_unique_interface(OBJECT_PATH, INTERFACE_NAME);

    // --- Properties ---

    // Property: LastResult (Read/Write)
    static int64_t lastResult = 0;
    iface->register_property_rw<int64_t>(
        "LastResult", 
        sdbusplus::vtable::property_::emits_change,
        [](const int64_t& newVal, int64_t& currVal) {
            currVal = newVal;
            return 1; // Success
        },
        [](const int64_t& currVal) {
            return lastResult;
        }
    );

    // Property: Status (Constant Enum)
    // In sdbusplus, enums are often passed as strings over the wire
    iface->register_property_r<std::string>(
        "Status",
        sdbusplus::vtable::property_::const_,
        [](const std::string& currVal) {
            return "xyz.openbmc_project.Calculator.State.Success";
        }
    );

    // Property: Owner (Read/Write with logic)
    static std::string owner = "None";
    iface->register_property_rw<std::string>(
        "Owner",
        sdbusplus::vtable::property_::emits_change,
        [](const std::string& newVal, std::string& currVal) {
            if (newVal == "root") { // Example logic for PermissionDenied
                currVal = newVal;
                return 1;
            }
            // In a real scenario, you'd throw the error defined in your events.yaml
            return 0; 
        },
        [](const std::string& currVal) {
            return owner;
        }
    );

    // --- Methods ---

    // Method: Multiply(x, y) -> z
    iface->register_method("Multiply", [](const int64_t x, const int64_t y) {
        lastResult = x * y;
        return lastResult;
    });

    // Method: Divide(x, y) -> z
    iface->register_method("Divide", [](const int64_t x, const int64_t y) {
        if (y == 0) {
            throw sdbusplus::xyz::openbmc_project::Calculator::Error::DivisionByZero();
        }
        lastResult = x / y;
        return lastResult;
    });

    // Method: Clear
    iface->register_method("Clear", [&iface]() {
        int64_t oldVal = lastResult;
        lastResult = 0;
        
        // --- Signal Emission ---
        // Emit the 'Cleared' signal defined in YAML
        auto s = iface->new_signal("Cleared");
        s.append(oldVal);
        s.signal_send();
    });

    // 4. Finalize and Run
    iface->initialize();
    io.run();

    return 0;
}

```

---

## Key Components Explained

### 1. `add_unique_interface`

This creates the instance of your interface at the specified object path. Unlike static bindings, this doesn't require you to inherit from a generated class; you build the "vtable" (the list of methods/properties) manually.

### 2. `register_property_rw`

This maps your internal C++ variables to DBus properties.

* The **Setter** (first lambda) handles incoming `Set` requests.
* The **Getter** (second lambda) handles `Get` requests.
* **Flags:** `sdbusplus::vtable::property_::emits_change` ensures that `PropertiesChanged` signals are sent automatically when the value is updated.

### 3. `register_method`

This connects a DBus method name directly to a C++ lambda or function.

* **Input arguments** are automatically unpacked from the DBus message.
* **Return values** are automatically packed into the DBus reply.

### 4. Handling Errors (from `events.yaml`)

In `sdbusplus`, if you have generated the error headers from your `events.yaml`, you can throw them as exceptions:

```cpp
throw sdbusplus::xyz::openbmc_project::Calculator::Error::DivisionByZero();

```

This will result in a formal DBus error being sent back to the caller instead of a standard return.

### stackful VS stackless Coroutine

While sdbusplus supports C++20 coroutines, the current register_method implementation in many versions of sdbusplus expects you to use boost::asio::yield_context (stackful) for automatic async handling, or it requires a specific template helper for stackless (co_return) coroutines that your current environment might not be implicitly mapping.

To fix this and stick to the C++20/C++23 standard while satisfying the template constraints of sdbusplus, we will use the yield_context approach. It is functionally identical for your needs but has built-in traits that sdbusplus understands for mapping types to DBus.

---

## How to Compile

You will need to link against `sdbusplus` and `systemd`. If you are using a BitBake recipe or a standard Linux environment:

```bash
g++ -std=c++20 main.cpp -lsdbusplus -lsystemd -lboost_system -o calculator_service

```
