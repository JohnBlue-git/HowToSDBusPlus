#include <chrono>
#include <concepts>
#include <iostream>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <sdbusplus/async.hpp>
#include <sdbusplus/async/timer.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/vtable.hpp>

#include "calculator_enum.hpp"

template <typename Derived>
class AsyncCrptCalculatorService
{
  public:
    AsyncCrptCalculatorService(sdbusplus::async::context& ctx,
                                const char* objectPath,
                                const char* baseState) :
        ctx_(ctx), objectPath_(objectPath), base_(baseState),
        status_(CalculatorEnum::State::success), owner_("root")
    {
        checkContract();
        setupInterface();
    }

  protected:
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

    void setupInterface()
    {
        static const sdbusplus::vtable_t vtable[] = {
            sdbusplus::vtable::start(),
            sdbusplus::vtable::property(
                "LastResult", "x", get_property<&AsyncCrptCalculatorService::lastResult_>,
                set_property<&AsyncCrptCalculatorService::lastResult_>,
                sdbusplus::vtable::property_::emits_change),
            sdbusplus::vtable::property(
                "Status", "s", get_property<&AsyncCrptCalculatorService::status_>,
                sdbusplus::vtable::property_::const_),
            sdbusplus::vtable::property(
                "Base", "s", get_property<&AsyncCrptCalculatorService::base_>,
                sdbusplus::vtable::property_::const_),
            sdbusplus::vtable::property(
                "Owner", "s", get_property<&AsyncCrptCalculatorService::owner_>,
                set_property<&AsyncCrptCalculatorService::owner_>,
                sdbusplus::vtable::property_::emits_change),
            sdbusplus::vtable::method(
                "Multiply", "xx", "x", handle_method<multiply_t, &Derived::multiply>),
            sdbusplus::vtable::method(
                "Divide", "xx", "x", handle_method<divide_t, &Derived::divide>),
            sdbusplus::vtable::method(
                "Express", "", "s", handle_method<express_t, &Derived::express>),
            sdbusplus::vtable::method(
                "Clear", "", "", handle_method<clear_t, &Derived::clear>),
            sdbusplus::vtable::signal("Cleared", "x"),
            sdbusplus::vtable::end()};

        interface_ = std::make_unique<sdbusplus::server::interface::interface>(
            ctx_.get_bus(), objectPath_, CalculatorEnum::interface, vtable, this);
    }

    void checkContract() const
    {
        static_assert(requires(Derived& d, const Derived& cd, int64_t x, int64_t y) {
            { d.multiply(x, y) } -> std::same_as<sdbusplus::async::task<int64_t>>;
            { d.divide(x, y) } -> std::same_as<sdbusplus::async::task<int64_t>>;
            { cd.express() } -> std::same_as<sdbusplus::async::task<std::string>>;
            { d.clear() } -> std::same_as<sdbusplus::async::task<void>>;
        });
    }

    template <auto Member>
    static int get_property(sd_bus*, const char*, const char*, const char*,
                            sd_bus_message* reply, void* userdata, sd_bus_error*)
    {
        auto* self = static_cast<AsyncCrptCalculatorService*>(userdata);
        auto message = sdbusplus::message_t(reply);
        message.append(self->*Member);
        return 1;
    }

    template <auto Member>
    static int set_property(sd_bus*, const char*, const char*, const char*,
                            sd_bus_message* value, void* userdata,
                            sd_bus_error* error)
    {
        auto* self = static_cast<AsyncCrptCalculatorService*>(userdata);
        auto message = sdbusplus::message_t(value);
        using Property = std::remove_reference_t<decltype(self->*Member)>;
        try
        {
            std::variant<Property> property;
            message.read(property);
            self->*Member = std::get<Property>(property);
        }
        catch (const std::exception& exception)
        {
            return sd_bus_error_set(error, CalculatorEnum::Error::generic,
                                    exception.what());
        }
        return 1;
    }

    template <typename Trait, auto Member>
    static int handle_method(sd_bus_message* rawMessage, void* userdata,
                             sd_bus_error* error)
    {
        auto* self = static_cast<AsyncCrptCalculatorService*>(userdata);
        auto* derived = static_cast<Derived*>(self);
        auto message = sdbusplus::message_t(rawMessage);
        try
        {
            typename Trait::value_types args;
            if constexpr (std::tuple_size_v<typename Trait::value_types> != 0)
            {
                args = message.unpack<std::tuple_element_t<0, typename Trait::value_types>,
                                       std::tuple_element_t<1, typename Trait::value_types>>();
            }
            self->ctx_.spawn(dispatch<Trait, Member>(derived, std::move(message),
                                                      std::move(args)));
        }
        catch (const std::exception& exception)
        {
            return sd_bus_error_set(error, CalculatorEnum::Error::generic,
                                    exception.what());
        }
        return 1;
    }

    template <typename Trait, auto Member>
    static sdbusplus::async::task<> dispatch(
        Derived* derived, sdbusplus::message_t message,
        typename Trait::value_types args)
    {
        if constexpr (std::is_void_v<typename Trait::return_type>)
        {
            co_await std::apply(
                [derived](auto&&... values) {
                    return (derived->*Member)(std::forward<decltype(values)>(values)...);
                },
                args);
            auto reply = message.new_method_return();
            reply.method_return();
        }
        else
        {
            auto result = co_await std::apply(
                [derived](auto&&... values) {
                    return (derived->*Member)(std::forward<decltype(values)>(values)...);
                },
                args);
            auto reply = message.new_method_return();
            reply.append(result);
            reply.method_return();
        }
        co_return;
    }

    sdbusplus::async::context& ctx_;
    std::unique_ptr<sdbusplus::server::interface::interface> interface_;
    const char* objectPath_;
    int64_t lastResult_ = 0;
    std::string base_;
    std::string status_;
    std::string owner_;
};

class AsyncSleepDecimal : public AsyncCrptCalculatorService<AsyncSleepDecimal>
{
  public:
    using Base = AsyncCrptCalculatorService<AsyncSleepDecimal>;
    friend Base;
    explicit AsyncSleepDecimal(sdbusplus::async::context& ctx) :
        Base(ctx, CalculatorEnum::ObjectPath::decimal, CalculatorEnum::NumberBase::decimal)
    {}

    sdbusplus::async::task<int64_t> multiply(int64_t x, int64_t y)
    {
        co_await sdbusplus::async::sleep_for(ctx_, std::chrono::milliseconds(10));
        lastResult_ = x * y;
        status_ = CalculatorEnum::State::success;
        co_return lastResult_;
    }

    sdbusplus::async::task<int64_t> divide(int64_t x, int64_t y)
    {
        if (y == 0)
        {
            status_ = CalculatorEnum::State::failure;
            throw sdbusplus::exception::SdBusError(
                EDOM, CalculatorEnum::Error::divisionByZero);
        }
        lastResult_ = x / y;
        status_ = CalculatorEnum::State::success;
        co_return lastResult_;
    }

    sdbusplus::async::task<std::string> express() const
    {
        co_return std::to_string(lastResult_);
    }

    sdbusplus::async::task<void> clear()
    {
        if (owner_ != "root")
        {
            throw sdbusplus::exception::SdBusError(
                EACCES, CalculatorEnum::Error::permissionDenied);
        }
        const auto oldValue = lastResult_;
        lastResult_ = 0;
        status_ = CalculatorEnum::State::success;
        auto signal = interface_->new_signal("Cleared");
        signal.append(oldValue);
        signal.signal_send();
        co_return;
    }
};

int main()
{
    try
    {
        sdbusplus::async::context ctx;
        ctx.get_bus().request_name(CalculatorEnum::service);
        AsyncSleepDecimal calculator(ctx);
        std::cout << "Async CRTP calculator is running" << std::endl;
        ctx.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Fatal Error: " << exception.what() << std::endl;
        return 1;
    }
    return 0;
}
