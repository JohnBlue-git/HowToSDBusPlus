#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

class CalculatorService {
  public:
    CalculatorService(boost::asio::io_context& io) : 
        conn_(std::make_shared<sdbusplus::asio::connection>(io, sdbusplus::bus::new_system())),
        objServer_(conn_) 
    {
        conn_->request_name(serviceName_);
        setupInterface();
    }

  private:
    void setupInterface() {
        // Use the initialization lambda style
        calculatorIface_ = objServer_.add_unique_interface(
            objectPath_, interfaceName_,
            [this](sdbusplus::asio::dbus_interface& i) {
                // --- Properties ---
                
                // LastResult: Read/Write
                i.register_property_rw<int64_t>(
                    "LastResult", sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newVal, auto& /*currVal*/) {
                        lastResult_ = newVal;
                        return true;
                    },
                    [this](const auto& /*currVal*/) { return lastResult_; });

                // Status: Read-Only Constant
                i.register_property_r<std::string>(
                    "Status", sdbusplus::vtable::property_::const_,
                    [this](const auto& /*currVal*/) { return status_; });

                // Owner: Read/Write
                i.register_property_rw<std::string>(
                    "Owner", sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newVal, auto& /*currVal*/) {
                        if (newVal == "forbidden") {
                            throw sdbusplus::exception::SdBusError(EPERM, "PermissionDenied");
                        }
                        owner_ = newVal;
                        return true;
                    },
                    [this](const auto& /*currVal*/) { return owner_; });

                // --- Methods ---

                i.register_method("Multiply", [this](int64_t x, int64_t y) {
                    lastResult_ = x * y;
                    return lastResult_;
                });

                i.register_method("Divide", [this](int64_t x, int64_t y) {
                    if (y == 0) {
                        throw sdbusplus::exception::SdBusError(EDOM, "DivisionByZero");
                    }
                    lastResult_ = x / y;
                    return lastResult_;
                });

                i.register_method("Clear", [this, &i]() {
                    int64_t oldVal = lastResult_;
                    lastResult_ = 0;
                    
                    // Emit signal
                    auto s = i.new_signal("Cleared");
                    s.append(oldVal);
                    s.signal_send();
                });
            });
    }

    // Connection & Server
    std::shared_ptr<sdbusplus::asio::connection> conn_;
    sdbusplus::asio::object_server objServer_;
    std::unique_ptr<sdbusplus::asio::dbus_interface> calculatorIface_;

    // DBus Metadata
    const char* serviceName_ = "xyz.openbmc_project.Calculator";
    const char* objectPath_ = "/calculator";
    const char* interfaceName_ = "xyz.openbmc_project.Calculator";

    // Internal State
    int64_t lastResult_ = 0;
    std::string status_ = "xyz.openbmc_project.Calculator.State.Success";
    std::string owner_ = "None";
};

int main() {
    try {
        boost::asio::io_context io;
        CalculatorService calc(io);
        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}