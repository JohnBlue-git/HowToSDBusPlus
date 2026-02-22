#include <iostream>
#include <tuple>
#include <variant>
#include <sdbusplus/async.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/exception.hpp>

class SdbusplusAsyncCalculatorService {
  public:

    explicit SdbusplusAsyncCalculatorService(sdbusplus::async::context& ctx) :
        ctx_(ctx)
    {
        ctx_.get_bus().request_name(serviceName_);
        setupInterface();
    }

    void setupInterface() {
        static const sdbusplus::vtable_t vtable[] = {
            sdbusplus::vtable::start(),

            sdbusplus::vtable::property("LastResult", "x",
                                        get_property<&SdbusplusAsyncCalculatorService::lastResult_>,
                                        set_property<&SdbusplusAsyncCalculatorService::lastResult_>,
                                        sdbusplus::vtable::property_::emits_change),
            sdbusplus::vtable::property("Status", "s", 
                                        get_property<&SdbusplusAsyncCalculatorService::status_>,
                                        sdbusplus::vtable::property_::emits_change),
            sdbusplus::vtable::property("Owner", "s", 
                                        get_property<&SdbusplusAsyncCalculatorService::owner_>,
                                        set_property<&SdbusplusAsyncCalculatorService::owner_>,
                                        sdbusplus::vtable::property_::emits_change),

            sdbusplus::vtable::method("Multiply", "xx", "x",
                handle_method<multiply_t, &SdbusplusAsyncCalculatorService::multiply>),
            sdbusplus::vtable::method("Divide", "xx", "x",
                handle_method<divide_t, &SdbusplusAsyncCalculatorService::divide>),
            sdbusplus::vtable::method("Clear", "", "",
                handle_method<clear_t, &SdbusplusAsyncCalculatorService::clear>),

            sdbusplus::vtable::signal("Cleared", "x"),

            sdbusplus::vtable::end()
        };

        interface_ = std::make_unique<sdbusplus::server::interface::interface>(
            ctx_.get_bus(), objectPath_, interfaceName_, vtable, this);
    }

    // --- Define Method Traits ---

    struct multiply_t {
        using value_types = std::tuple<int64_t, int64_t>;
        using return_type = int64_t;
    };

    struct divide_t {
        using value_types = std::tuple<int64_t, int64_t>;
        using return_type = int64_t;
    };

    struct clear_t {
        using value_types = std::tuple<>;
        using return_type = void;
    };

  private:

    // --- Templated Handlers ---

    template <auto PtrToMember>
    static int get_property(sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
                           const char* /*property*/, sd_bus_message* reply,
                           void* userdata, sd_bus_error* /*ret_error*/) {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto m = sdbusplus::message_t(reply);
        m.append(self->*PtrToMember);
        return 1;
    }

    template <auto PtrToMember>
    static int set_property(sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
                           const char* /*property*/, sd_bus_message* value,
                           void* userdata, sd_bus_error* ret_error) {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto m = sdbusplus::message_t(value);
        using PropertyType = std::remove_reference_t<decltype(self->*PtrToMember)>;

        try {
            // Use 'variant' to read
            std::variant<PropertyType> val;
            m.read(val); 
            self->*PtrToMember = std::get<PropertyType>(val);
        }
        catch (const std::exception& e) {
            return sd_bus_error_set(ret_error, "xyz.openbmc_project.Calculator.Error", e.what());
        }
        return 1;
    }

    template <typename T, auto MemberFunc>
    static int handle_method(sd_bus_message* m, void* userdata, sd_bus_error* ret_error) {
        auto* self = static_cast<SdbusplusAsyncCalculatorService*>(userdata);
        auto msg = sdbusplus::message_t(m);

        try {
            // Use 'read' with the tuple
            typename T::value_types args;
            // ONLY call read if there are actually arguments to read
            if constexpr (std::tuple_size_v<typename T::value_types> > 0) {
                msg.read(args); 
            }

            // By using ctx_.spawn, you tell the event loop:
            // "Take this coroutine and run it whenever you have a moment.
            // I'm returning control to you now so you can keep processing other messages."
            self->ctx_.spawn(
                [](SdbusplusAsyncCalculatorService* s, sdbusplus::message_t m_inner, typename T::value_types a) 
                -> sdbusplus::async::task<> {
                    // works with void return type
                    if constexpr (std::is_void_v<typename T::return_type>) {
                        // std::apply works with empty tuples if the lambda accepts zero args
                        co_await std::apply(
                            [s](auto&&... params) { 
                                return (s->*MemberFunc)(std::forward<decltype(params)>(params)...); 
                            }, a);
                        
                        auto reply = m_inner.new_method_return();
                        reply.method_return();
                    }
                    // works with non void return type
                    else {
                        // std::apply works with empty tuples if the lambda accepts zero args
                        auto result = co_await std::apply(
                            [s](auto&&... params) { 
                                return (s->*MemberFunc)(std::forward<decltype(params)>(params)...); 
                            }, a);
                        
                        auto reply = m_inner.new_method_return();
                        reply.append(result);
                        reply.method_return();
                    }
                    co_return;
                }(self, std::move(msg), std::move(args))
            );
        }
        catch (const std::exception& e) {
            return sd_bus_error_set(ret_error, "xyz.openbmc_project.Calculator.Error", e.what());
        }
        return 1;
    }

    // --- Logic Functions ---

    sdbusplus::async::task<int64_t> multiply(int64_t x, int64_t y) {
        lastResult_ = x * y;
        co_return lastResult_;
    }

    sdbusplus::async::task<int64_t> divide(int64_t x, int64_t y) {
        if (y == 0) throw sdbusplus::exception::SdBusError(EDOM, "DivByZero");
        lastResult_ = x / y;
        co_return lastResult_;
    }

    sdbusplus::async::task<void> clear() {
        // ermission Check
        if (!owner_.empty() && owner_ != "root") { 
            throw sdbusplus::exception::SdBusError(EACCES, "PermissionDenied");
        }

        // Clear logic
        int64_t oldVal = lastResult_;
        lastResult_ = 0;
        
        // Emit Signal "Cleared" manually
        auto s = interface_->new_signal("Cleared");
        s.append(oldVal);
        s.signal_send();

        co_return;
    }

    // --- Variables ---

    sdbusplus::async::context& ctx_;
    std::unique_ptr<sdbusplus::server::interface::interface> interface_;

    const char* serviceName_ = "xyz.openbmc_project.Calculator";
    const char* objectPath_ = "/xyz/openbmc_project/calculator";
    const char* interfaceName_ = "xyz.openbmc_project.Calculator";

    int64_t lastResult_ = 0;
    std::string status_ = "xyz.openbmc_project.Calculator.State.Success";
    std::string owner_ = "root";
};

int main() {
    sdbusplus::async::context ctx;
    // No need boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    // When you call ctx.run(), it internally handles termination signals.

    SdbusplusAsyncCalculatorService calc(ctx);
    ctx.run();

    return 0;
}