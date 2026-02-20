#include <iostream>
#include <boost/asio/spawn.hpp> 
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
        calculatorIface_ = objServer_.add_unique_interface(
            objectPath_, interfaceName_,
            [this](sdbusplus::asio::dbus_interface& i) {
                
                // Properties (Synchronous lambdas are preferred for performance)
                i.register_property_rw<int64_t>(
                    "LastResult", sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newVal, auto& /*currVal*/) {
                        lastResult_ = newVal;
                        return true;
                    },
                    [this](const auto& /*currVal*/) { return lastResult_; });

                i.register_property_r<std::string>(
                    "Status", sdbusplus::vtable::property_::const_,
                    [this](const auto& /*currVal*/) { return status_; });

                // Methods using yield_context (The sdbusplus-native async way)
                // This satisfies the "No dbus type conversion" error.
                
                i.register_method("Multiply", 
                    [this](boost::asio::yield_context /*yield*/, int64_t x, int64_t y) {
                        lastResult_ = x * y;
                        return lastResult_; // Returns int64_t directly
                    });

                i.register_method("Divide", 
                    [this](boost::asio::yield_context /*yield*/, int64_t x, int64_t y) {
                        if (y == 0) {
                            throw sdbusplus::exception::SdBusError(EDOM, "DivisionByZero");
                        }
                        lastResult_ = x / y;
                        return lastResult_;
                    });

                i.register_method("Clear", 
                    [this, &i](boost::asio::yield_context /*yield*/) {
                        int64_t oldVal = lastResult_;
                        lastResult_ = 0;
                        
                        auto s = i.new_signal("Cleared");
                        s.append(oldVal);
                        s.signal_send();
                    });
            });
    }

    std::shared_ptr<sdbusplus::asio::connection> conn_;
    sdbusplus::asio::object_server objServer_;
    std::unique_ptr<sdbusplus::asio::dbus_interface> calculatorIface_;

    const char* serviceName_ = "xyz.openbmc_project.Calculator";
    const char* objectPath_ = "/calculator";
    const char* interfaceName_ = "xyz.openbmc_project.Calculator";

    int64_t lastResult_ = 0;
    std::string status_ = "xyz.openbmc_project.Calculator.State.Success";
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