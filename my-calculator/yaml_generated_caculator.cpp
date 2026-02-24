#include <iostream>
#include <sstream>
#include <utility>
#include <sdbusplus/async.hpp>
#include <sdbusplus/exception.hpp>
#include <xyz/openbmc_project/Calculator/common.hpp>
#include <xyz/openbmc_project/Calculator/event.hpp>
#include <xyz/openbmc_project/Calculator/aserver.hpp>

template <typename Derived>
class YamlGeneratedCalculatorService :
    public sdbusplus::aserver::xyz::openbmc_project::Calculator<Derived>
{
  public:
    using GeneratedBase = sdbusplus::aserver::xyz::openbmc_project::Calculator<Derived>;
    using State = typename GeneratedBase::State;

    YamlGeneratedCalculatorService(sdbusplus::async::context& ctx,
                                   const char* objectPath) :
        GeneratedBase(ctx, objectPath)
    {
        this->status(State::Success);
        this->owner("root");
    }

    bool set_property(typename GeneratedBase::owner_t, std::string owner)
    {
        // Logic: Only root or the current owner can change ownership
        // For simplicity in this example, we just set it:
        const bool changed = (owner_ != owner);
        owner_ = std::move(owner);
        return changed;
    }

  public:

    auto method_call(typename GeneratedBase::multiply_t, auto x, auto y)
    {
        auto result = x * y;
        this->last_result(result);
        this->status(State::Success);
        return result;
    }

    auto method_call(typename GeneratedBase::divide_t, auto x, auto y)
        -> sdbusplus::async::task<typename GeneratedBase::divide_t::return_type>
    {
        using DivisionByZero = sdbusplus::error::xyz::openbmc_project::Calculator::DivisionByZero;

        if (y == 0)
        {
            this->status(State::Error);
            throw DivisionByZero();
        }

        auto result = x / y;
        this->last_result(result);
        this->status(State::Success);
        co_return result;
    }

    auto method_call(typename GeneratedBase::clear_t)
        -> sdbusplus::async::task<>
    {
        if (owner_ != "root")
        {
            throw sdbusplus::error::xyz::openbmc_project::Calculator::PermissionDenied();
        }

        const auto oldValue = this->last_result();
        this->last_result(0);
        this->status(State::Success);
        this->cleared(oldValue);
        co_return;
    }

    auto method_call(typename GeneratedBase::express_t) const
        -> sdbusplus::async::task<typename GeneratedBase::express_t::return_type>
    {
        const auto value = this->last_result();
        co_return static_cast<const Derived&>(*this).format(value);
    }

  protected:
    std::string owner_ = "root";
};

class SdbusplusAsyncCalculatorServiceDecimal :
    public YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceDecimal>
{
  public:
    using Base = YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceDecimal>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceDecimal(sdbusplus::async::context& ctx) :
        Base(ctx, "/xyz/openbmc_project/calculator/decimal") {        
    }

  private:
    std::string format(int64_t value) const
    {
        return std::to_string(value);
    }
};

class SdbusplusAsyncCalculatorServiceBinary :
    public YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceBinary>
{
  public:
    using Base = YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceBinary>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceBinary(sdbusplus::async::context& ctx) :
        Base(ctx, "/xyz/openbmc_project/calculator/binary") {
    }

  private:
    std::string format(int64_t value) const
    {
        if (value == 0)
        {
            return "0b0";
        }

        const bool negative = value < 0;
        uint64_t absValue = negative ? -value : value;
        std::string bits;

        while (absValue > 0)
        {
            bits = (absValue & 1 ? '1' : '0') + bits;
            absValue >>= 1;
        }

        return negative ? "-0b" + bits : "0b" + bits;
    }
};

class SdbusplusAsyncCalculatorServiceHeximal :
    public YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceHeximal>
{
  public:
    using Base = YamlGeneratedCalculatorService<SdbusplusAsyncCalculatorServiceHeximal>;
    friend Base;

    explicit SdbusplusAsyncCalculatorServiceHeximal(sdbusplus::async::context& ctx) :
        Base(ctx, "/xyz/openbmc_project/calculator/heximal") {
    }

  private:
    std::string format(int64_t value) const
    {
        std::ostringstream oss;
        if (value < 0)
        {
            oss << "-0x" << std::hex << std::uppercase << -value;
        }
        else
        {
            oss << "0x" << std::hex << std::uppercase << value;
        }
        return oss.str();
    }
};

int main()
{
    try
    {
        sdbusplus::async::context ctx;
        sdbusplus::server::manager_t manager{ctx, "/xyz/openbmc_project/calculator"};

        SdbusplusAsyncCalculatorServiceDecimal decimal(ctx);
        SdbusplusAsyncCalculatorServiceBinary binary(ctx);
        SdbusplusAsyncCalculatorServiceHeximal heximal(ctx);

        ctx.spawn([](sdbusplus::async::context& context)
                      -> sdbusplus::async::task<> {
            context.request_name(sdbusplus::common::xyz::openbmc_project::Calculator::default_service);
            co_return;
        }(ctx));

        std::cout << "yaml_generated_caculator service is running" << std::endl;
        ctx.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}