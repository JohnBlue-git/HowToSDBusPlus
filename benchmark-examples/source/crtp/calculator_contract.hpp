#pragma once

#include <concepts>
#include <cstdint>
#include <string>

#include <sdbusplus/async.hpp>

template <typename Result, typename Value>
concept CalculatorResult = std::same_as<Result, Value> ||
                           std::same_as<Result, sdbusplus::async::task<Value>>;

template <typename Derived>
concept CalculatorContract = requires(Derived& calculator,
                                      const Derived& constCalculator,
                                      int64_t x, int64_t y) {
    {
        calculator.multiply(x, y)
    } -> CalculatorResult<int64_t>;
    {
        calculator.divide(x, y)
    } -> CalculatorResult<int64_t>;
    {
        constCalculator.express()
    } -> CalculatorResult<std::string>;
    {
        calculator.clear()
    } -> CalculatorResult<void>;
};