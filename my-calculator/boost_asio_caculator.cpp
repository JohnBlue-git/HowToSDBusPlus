#include <iostream>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include "calculator_enum.hpp"

class BoostAsioCalculatorService {
  public:
    BoostAsioCalculatorService(boost::asio::io_context& io) : 
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
                    [this](const auto& newVal, auto& currVal) {
                        return this->set_property<&BoostAsioCalculatorService::lastResult_>(newVal, currVal);
                    },
                    [this](const auto& currVal) {
                        return this->get_property<&BoostAsioCalculatorService::lastResult_>(currVal);
                    });

                i.register_property_r<std::string>(
                    "Status", sdbusplus::vtable::property_::const_,
                    [this](const auto& currVal) {
                        return this->get_property<&BoostAsioCalculatorService::status_>(currVal);
                    });

                i.register_property_r<std::string>(
                    "Base", sdbusplus::vtable::property_::const_,
                    [this](const auto& currVal) {
                        return this->get_property<&BoostAsioCalculatorService::base_>(currVal);
                    });

                i.register_property_rw<std::string>(
                    "Owner", sdbusplus::vtable::property_::emits_change,
                    [this](const auto& newVal, auto& currVal) {
                        // Logic: Only root or the current owner can change ownership
                        // For simplicity in this example, we just set it:
                        return this->set_property<&BoostAsioCalculatorService::owner_>(newVal, currVal);
                    },
                    [this](const auto& currVal) {
                        return this->get_property<&BoostAsioCalculatorService::owner_>(currVal);
                    });

                // Methods using yield_context (The sdbusplus-native async way)
                
                i.register_method("Multiply",
                    [this](boost::asio::yield_context yield, int64_t x, int64_t y) {
                        // 1.
                        // Simulate a 2-second cloud calculation
                        // boost::asio::steady_timer timer(conn_->get_io_context());
                        // timer.expires_after(std::chrono::seconds(2));
                        // This uses 'yield' to suspend!
                        // timer.async_wait(yield);
                        // 2.
                        // This suspends THIS lambda, but let's other DBus requests through
                        // double taxRate = sdbusplus::asio::getProperty<double>(
                        //     *conn_, destService, destPath, destIface, "Value", yield[ec]);
                        // lastResult_ = ... taxRate;

                        return this->handle_multiply(yield, x, y);
                    });

                i.register_method("Divide",
                    [this](boost::asio::yield_context yield, int64_t x, int64_t y) {
                        return this->handle_divide(yield, x, y);
                    });

                i.register_method("Express",
                    [this, &i](boost::asio::yield_context yield) {
                        return this->handle_express(yield);
                    });

                i.register_method("Clear",
                    [this, &i](boost::asio::yield_context yield) {
                        // Clear
                        int64_t oldValue = this->lastResult_;
                        this->handle_clear(yield);

                        // Signal
                        auto s = i.new_signal("Cleared");
                        s.append(oldValue);
                        s.signal_send();
                    });
            });
    }

    // --- Logic Functions ---

    template <auto PtrToMember>
    bool set_property(const auto& newVal, auto& /*currVal*/) {
        this->*PtrToMember = newVal;
        return true;
    }

    template <auto PtrToMember>
    const auto& get_property(const auto& /*currVal*/) const {
        return this->*PtrToMember;
    }

    int64_t handle_multiply(boost::asio::yield_context /*yield*/, int64_t x, int64_t y) {
        lastResult_ = x * y;
        return lastResult_;
    }

    int64_t handle_divide(boost::asio::yield_context /*yield*/, int64_t x, int64_t y) {
        if (y == 0) {
            throw sdbusplus::exception::SdBusError(EDOM, CalculatorEnum::Error::divisionByZero);
        }
        lastResult_ = x / y;
        return lastResult_;
    }

    std::string handle_express(boost::asio::yield_context /*yield*/) const {
        return std::to_string(lastResult_);
    }

    void handle_clear(boost::asio::yield_context /*yield*/) {
        // Note: In a real system, you'd map 'caller' to a UID.
        // For this example, if owner_ is set and doesn't match, we deny.
        if (!owner_.empty() && owner_ != "root") { 
            throw sdbusplus::exception::SdBusError(EACCES, CalculatorEnum::Error::permissionDenied);
        }

        // Clear
        lastResult_ = 0;
    }

    // --- Variables ---

    std::shared_ptr<sdbusplus::asio::connection> conn_;
    sdbusplus::asio::object_server objServer_;
    std::unique_ptr<sdbusplus::asio::dbus_interface> calculatorIface_;

    const char* serviceName_ = CalculatorEnum::service;
    const char* objectPath_ = CalculatorEnum::ObjectPath::root;
    const char* interfaceName_ = CalculatorEnum::interface;

    int64_t lastResult_ = 0;
    std::string status_ = CalculatorEnum::State::success;
    std::string base_ = CalculatorEnum::NumberBase::decimal;
    std::string owner_ = "root";
};

int main() {
    try {
        boost::asio::io_context io;
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);

        BoostAsioCalculatorService calc(io);
        signals.async_wait(
            [&io](const boost::system::error_code&, const int&) {
            io.stop();
        });
        io.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}