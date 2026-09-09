#pragma once

class CalculatorEnum {
  public:
    static constexpr const char* service = "xyz.openbmc_project.Calculator";
    static constexpr const char* interface = "xyz.openbmc_project.Calculator";

    class ObjectPath {
      public:
        static constexpr const char* root = "/xyz/openbmc_project/calculator";
        static constexpr const char* decimal = "/xyz/openbmc_project/calculator/decimal";
        static constexpr const char* binary = "/xyz/openbmc_project/calculator/binary";
        static constexpr const char* heximal = "/xyz/openbmc_project/calculator/heximal";
    };

    class State {
      public:
        static constexpr const char* success = "xyz.openbmc_project.Calculator.State.Success";
        static constexpr const char* failure = "xyz.openbmc_project.Calculator.State.Failure";
    };

    class NumberBase {
      public:
        static constexpr const char* decimal = "xyz.openbmc_project.Calculator.Base.Decimal";
        static constexpr const char* binary = "xyz.openbmc_project.Calculator.Base.Binary";
        static constexpr const char* heximal = "xyz.openbmc_project.Calculator.Base.Heximal";
    };

    class Error {
      public:
        static constexpr const char* divisionByZero = "xyz.openbmc_project.Calculator.DivisionByZero";
        static constexpr const char* permissionDenied = "xyz.openbmc_project.Calculator.PermissionDenied";
        static constexpr const char* generic = "xyz.openbmc_project.Calculator.Error";
    };
};