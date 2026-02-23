#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/asio/spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include "calculator_enum.hpp"

// CRTP Base Template Class
template<typename Derived>
class BoostAsioCalculatorService {
  public:
    BoostAsioCalculatorService(std::shared_ptr<sdbusplus::asio::connection> conn, const char* objectPath,
                   const std::string& baseState) :
    conn_(std::move(conn)),
        objServer_(conn_),
        objectPath_(objectPath),
        interfaceName_(CalculatorEnum::interface),
        lastResult_(0),
        base_(baseState),
        status_(CalculatorEnum::State::success),
        owner_("root")
    {
        setupInterface();
    }

  protected:
    inline Derived& derived() { return static_cast<Derived&>(*this); }
    inline const Derived& derived() const { return static_cast<const Derived&>(*this); }

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
                        return this->derived().handle_multiply(yield, x, y);
                    });

                i.register_method("Divide",
                    [this](boost::asio::yield_context yield, int64_t x, int64_t y) {
                        return this->derived().handle_divide(yield, x, y);
                    });

                i.register_method("Express",
                    [this](boost::asio::yield_context yield) {
                        return this->derived().handle_express(yield);
                    });

                i.register_method("Clear",
                    [this, &i](boost::asio::yield_context yield) {
                        // Clear
                        int64_t oldValue = this->lastResult_;
                        this->derived().handle_clear(yield);

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

    const char* objectPath_;
    const char* interfaceName_;

    int64_t lastResult_;
    std::string base_;
    std::string status_;
    std::string owner_;
};

// Decimal Calculator (Base 10)
class BoostAsioCalculatorServiceDecimal : public BoostAsioCalculatorService<BoostAsioCalculatorServiceDecimal> {
  public:
    using Base = BoostAsioCalculatorService<BoostAsioCalculatorServiceDecimal>;
    friend Base;
    
    BoostAsioCalculatorServiceDecimal(std::shared_ptr<sdbusplus::asio::connection> conn) : 
        Base(std::move(conn), CalculatorEnum::ObjectPath::decimal,
             CalculatorEnum::NumberBase::decimal) {
    }

  protected:

    // --- Logic Functions ---

    std::string handle_express(boost::asio::yield_context /*yield*/) const {
        return std::to_string(lastResult_);
    }
};

// Binary Calculator (Base 2)
class BoostAsioCalculatorServiceBinary : public BoostAsioCalculatorService<BoostAsioCalculatorServiceBinary> {
  public:
    using Base = BoostAsioCalculatorService<BoostAsioCalculatorServiceBinary>;
    friend Base;
    
    BoostAsioCalculatorServiceBinary(std::shared_ptr<sdbusplus::asio::connection> conn) : 
        Base(std::move(conn), CalculatorEnum::ObjectPath::binary,
             CalculatorEnum::NumberBase::binary) {
    }

  protected:

    // --- Logic Functions ---

    std::string handle_express(boost::asio::yield_context /*yield*/) const {
        int64_t value = lastResult_;
        if (value == 0) return "0b0";
        
        std::string binary = "0b";
        bool negative = value < 0;
        uint64_t absValue = negative ? -value : value;
        
        std::string bits;
        while (absValue > 0) {
            bits = (absValue & 1 ? '1' : '0') + bits;
            absValue >>= 1;
        }
        
        return negative ? "-" + binary + bits : binary + bits;
    }
};

// Hexadecimal Calculator (Base 16)
class BoostAsioCalculatorServiceHeximal : public BoostAsioCalculatorService<BoostAsioCalculatorServiceHeximal> {
  public:
    using Base = BoostAsioCalculatorService<BoostAsioCalculatorServiceHeximal>;
    friend Base;
    
    BoostAsioCalculatorServiceHeximal(std::shared_ptr<sdbusplus::asio::connection> conn) : 
        Base(std::move(conn), CalculatorEnum::ObjectPath::heximal,
             CalculatorEnum::NumberBase::heximal) {
    }

  protected:

    // --- Logic Functions ---

    std::string handle_express(boost::asio::yield_context /*yield*/) const {
        int64_t value = lastResult_;
        std::ostringstream oss;
        oss << "0x" << std::hex << std::uppercase << value;
        return oss.str();
    }
};

int main() {
    try {
        boost::asio::io_context io;
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        auto conn = std::make_shared<sdbusplus::asio::connection>(io, sdbusplus::bus::new_system());
        conn->request_name(CalculatorEnum::service);

        std::cout << "Starting Decimal Calculator..." << std::endl;
        BoostAsioCalculatorServiceDecimal calcDecimal(conn);
        
        std::cout << "Starting Binary Calculator..." << std::endl;
        BoostAsioCalculatorServiceBinary calcBinary(conn);
        
        std::cout << "Starting Heximal Calculator..." << std::endl;
        BoostAsioCalculatorServiceHeximal calcHeximal(conn);
        
        std::cout << "Calculator services running. Press Ctrl+C to stop." << std::endl;
        signals.async_wait(
            [&io](const boost::system::error_code&, const int&) {
            std::cout << "Shutting down..." << std::endl;
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
