#include <iostream>
#include <sstream>
#include <tuple>
#include <concepts>
#include <type_traits>
#include <variant>
#include <sdbusplus/async.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/exception.hpp>
#include "calculator_enum.hpp"

template <typename Derived>
class SdbusplusAsyncCalculatorService {
  public:
    SdbusplusAsyncCalculatorService(sdbusplus::async::context& ctx,
                                    const char* objectPath,
                                    const std::string& baseState) :
        ctx_(ctx),
        objectPath_(objectPath),
        interfaceName_(CalculatorEnum::interface),
        lastResult_(0),
        base_(baseState),
        status_(CalculatorEnum::State::success),
        owner_("root")
    {
        checkContract();
        setupInterface();
    }

  protected:
    void setupInterface()
    {
        static const sdbusplus::vtable_t vtable[] = {
            sdbusplus::vtable::start(),

            sdbusplus::vtable::property(
                "LastResult", "x",
                get_property<&SdbusplusAsyncCalculatorService::lastResult_>,
                set_property<&SdbusplusAsyncCalculatorService::lastResult_>,
                sdbusplus::vtable::property_::emits_change),

            sdbusplus::vtable::property(
                "Status", "s",
                get_property<&SdbusplusAsyncCalculatorService::status_>,
                sdbusplus::vtable::property_::const_),

            sdbusplus::vtable::property(
                "Base", "s",
                get_property<&SdbusplusAsyncCalculatorService::base_>,
                sdbusplus::vtable::property_::const_),

            sdbusplus::vtable::property(
                "Owner", "s",
                get_property<&SdbusplusAsyncCalculatorService::owner_>,
                set_property<&SdbusplusAsyncCalculatorService::owner_>,
                sdbusplus::vtable::property_::emits_change),

            sdbusplus::vtable::method(
                "Multiply", "xx", "x",
                handle_method<multiply_t, &Derived::multiply>),

            sdbusplus::vtable::method(
                "Divide", "xx", "x",
                handle_method<divide_t, &Derived::divide>),

            sdbusplus::vtable::method(
                "Express", "", "s",
                handle_method<express_t, &Derived::express>),

            sdbusplus::vtable::method(
                "Clear", "", "",
                handle_method<clear_t, &Derived::clear>),

            sdbusplus::vtable::signal("Cleared", "x"),

            sdbusplus::vtable::end()};

        interface_ = std::make_unique<sdbusplus::server::interface::interface>(
            ctx_.get_bus(), objectPath_, interfaceName_, vtable, this);
    }

    // --- Define Method Traits ---

    struct multiply_t
    {
        using value_types = std::tuple<int64_t, int64_t>;
        using return_type = int64_t;
    };

    struct divide_t
    {
        using value_types = std::tuple<int64_t, int64_t>;
        using return_type = int64_t;
    };

    struct express_t
    {
        using value_types = std::tuple<>;
        using return_type = std::string;
    };

    struct clear_t
    {
        using value_types = std::tuple<>;
        using return_type = void;
    };

    // --- CRTP Helpers ---

    inline Derived& derived()
    {
        return static_cast<Derived&>(*this);
    }

    inline const Derived& derived() const
    {
        return static_cast<const Derived&>(*this);
    }

    inline void checkContract() const
    {
        static_assert(
            requires(Derived& d, const Derived& cd, int64_t x, int64_t y) {
            {
                d.lastResult_
            } -> std::same_as<int64_t&>;
            {
                d.base_
            } -> std::same_as<std::string&>;
            {
                d.status_
            } -> std::same_as<std::string&>;
            {
                d.owner_
            } -> std::same_as<std::string&>;

            {
                d.multiply(x, y)
            } -> std::same_as<sdbusplus::async::task<int64_t>>;
            {
                d.divide(x, y)
            } -> std::same_as<sdbusplus::async::task<int64_t>>;
            {
                cd.express()
            } -> std::same_as<sdbusplus::async::task<std::string>>;
            {
                d.clear()
            } -> std::same_as<sdbusplus::async::task<void>>;
        }, "CRTP contract mismatch: required properties or methods are missing or have incorrect signatures.");
    }

    // --- Templated Handlers ---

    template <auto PtrToMember>
    static int get_property(sd_bus*, const char*, const char*, const char*,
                            sd_bus_message* reply, void* userdata,
                            sd_bus_error*)
    {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto m = sdbusplus::message_t(reply);
        m.append(self->*PtrToMember);
        return 1;
    }

    template <auto PtrToMember>
    static int set_property(sd_bus*, const char*, const char*, const char*,
                            sd_bus_message* value, void* userdata,
                            sd_bus_error* ret_error)
    {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto m = sdbusplus::message_t(value);
        using PropertyType =
            std::remove_reference_t<decltype(self->*PtrToMember)>;

        try
        {
            std::variant<PropertyType> val;
            m.read(val);
            self->*PtrToMember = std::get<PropertyType>(val);
        }
        catch (const std::exception& e)
        {
            return sd_bus_error_set(ret_error, CalculatorEnum::Error::generic,
                                    e.what());
        }
        return 1;
    }

    template <typename T, auto MemberFunc>
    static int handle_method(sd_bus_message* m, void* userdata,
                             sd_bus_error* ret_error)
    {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto* derivedSelf = static_cast<Derived*>(self);
        auto msg = sdbusplus::message_t(m);

        try
        {
            typename T::value_types args;
            if constexpr (std::tuple_size_v<typename T::value_types> > 0)
            {
                args = unpack_args<typename T::value_types>(
                    msg,
                    std::make_index_sequence<std::tuple_size_v<typename T::value_types>>{});
            }

            self->ctx_.spawn(
                [](Derived* s,
                   sdbusplus::message_t m_inner,
                   typename T::value_types a) -> sdbusplus::async::task<> {
                    if constexpr (std::is_void_v<typename T::return_type>)
                    {
                        co_await std::apply(
                            [s](auto&&... params) {
                                return (s->*MemberFunc)(
                                    std::forward<decltype(params)>(params)...);
                            },
                            a);

                        auto reply = m_inner.new_method_return();
                        reply.method_return();
                    }
                    else
                    {
                        auto result = co_await std::apply(
                            [s](auto&&... params) {
                                return (s->*MemberFunc)(
                                    std::forward<decltype(params)>(params)...);
                            },
                            a);

                        auto reply = m_inner.new_method_return();
                        reply.append(result);
                        reply.method_return();
                    }
                    co_return;
                }(derivedSelf, std::move(msg), std::move(args)));
        }
        catch (const std::exception& e)
        {
            return sd_bus_error_set(ret_error, CalculatorEnum::Error::generic,
                                    e.what());
        }
        return 1;
    }

    template <typename Tuple, std::size_t... I>
    static Tuple unpack_args(sdbusplus::message_t& msg, std::index_sequence<I...>)
    {
        return msg.unpack<std::tuple_element_t<I, Tuple>...>();
    }

    // --- Logic Functions ---

    sdbusplus::async::task<int64_t> multiply(int64_t x, int64_t y)
    {
        lastResult_ = x * y;
        co_return lastResult_;
    }

    sdbusplus::async::task<int64_t> divide(int64_t x, int64_t y)
    {
        if (y == 0)
        {
            throw sdbusplus::exception::SdBusError(
                EDOM, CalculatorEnum::Error::divisionByZero);
        }
        lastResult_ = x / y;
        co_return lastResult_;
    }

    sdbusplus::async::task<std::string> express() const
    {
        co_return std::to_string(lastResult_);
    }

    sdbusplus::async::task<void> clear()
    {
        if (!owner_.empty() && owner_ != "root")
        {
            throw sdbusplus::exception::SdBusError(
                EACCES, CalculatorEnum::Error::permissionDenied);
        }

        int64_t oldVal = lastResult_;
        lastResult_ = 0;

        auto s = interface_->new_signal("Cleared");
        s.append(oldVal);
        s.signal_send();

        co_return;
    }

    // --- Variables ---

    sdbusplus::async::context& ctx_;
    std::unique_ptr<sdbusplus::server::interface::interface> interface_;

    const char* objectPath_;
    const char* interfaceName_;

    int64_t lastResult_;
    std::string base_;
    std::string status_;
    std::string owner_;
};

class SdbusplusAsyncCalculatorServiceDecimal :
    public SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceDecimal> {
  public:
    using Base =
        SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceDecimal>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceDecimal(sdbusplus::async::context& ctx) :
        Base(ctx, CalculatorEnum::ObjectPath::decimal, CalculatorEnum::NumberBase::decimal)
    {}

  protected:
    sdbusplus::async::task<std::string> express() const
    {
        co_return std::to_string(lastResult_);
    }
};

class SdbusplusAsyncCalculatorServiceBinary :
    public SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceBinary> {
  public:
    using Base =
        SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceBinary>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceBinary(sdbusplus::async::context& ctx) :
        Base(ctx, CalculatorEnum::ObjectPath::binary, CalculatorEnum::NumberBase::binary)
    {}

  protected:
    sdbusplus::async::task<std::string> express() const
    {
        int64_t value = lastResult_;
        if (value == 0)
        {
            co_return "0b0";
        }

        std::string binary = "0b";
        bool negative = value < 0;
        uint64_t absValue = negative ? -value : value;

        std::string bits;
        while (absValue > 0)
        {
            bits = (absValue & 1 ? '1' : '0') + bits;
            absValue >>= 1;
        }

        if (negative)
        {
            co_return "-" + binary + bits;
        }
        co_return binary + bits;
    }
};

class SdbusplusAsyncCalculatorServiceHeximal :
    public SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceHeximal> {
  public:
    using Base =
        SdbusplusAsyncCalculatorService<SdbusplusAsyncCalculatorServiceHeximal>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceHeximal(sdbusplus::async::context& ctx) :
        Base(ctx, CalculatorEnum::ObjectPath::heximal, CalculatorEnum::NumberBase::heximal)
    {}

  protected:
    sdbusplus::async::task<std::string> express() const
    {
        int64_t value = lastResult_;
        std::ostringstream oss;
        oss << "0x" << std::hex << std::uppercase << value;
        co_return oss.str();
    }
};

int main()
{
    try
    {
        sdbusplus::async::context ctx;
        ctx.get_bus().request_name(CalculatorEnum::service);

        std::cout << "Starting Decimal Calculator..." << std::endl;
        SdbusplusAsyncCalculatorServiceDecimal calcDecimal(ctx);

        std::cout << "Starting Binary Calculator..." << std::endl;
        SdbusplusAsyncCalculatorServiceBinary calcBinary(ctx);

        std::cout << "Starting Heximal Calculator..." << std::endl;
        SdbusplusAsyncCalculatorServiceHeximal calcHeximal(ctx);

        std::cout << "Calculator services running. Press Ctrl+C to stop."
                  << std::endl;
        ctx.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}