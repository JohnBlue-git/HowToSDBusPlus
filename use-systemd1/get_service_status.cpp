#include <sdbusplus/bus.hpp>

#include <iostream>
#include <vector>
#include <string>

enum UnitStructFields
{
    UNIT_NAME,
    UNIT_DESC,
    UNIT_LOAD_STATE,
    UNIT_ACTIVE_STATE,
    UNIT_SUB_STATE,
    UNIT_DEVICE,
    UNIT_OBJ_PATH,
    UNIT_ALWAYS_0,
    UNIT_ALWAYS_EMPTY,
    UNIT_ALWAYS_ROOT_PATH
};

using UnitStruct = std::tuple<
    std::string,
    std::string,
    std::string,
    std::string,
    std::string,
    std::string,
    sdbusplus::message::object_path,
    uint32_t,
    std::string,
    sdbusplus::message::object_path
>;

bool getServiceStatus(const std::vector<std::string>& serviceNames)
{
    auto bus = sdbusplus::bus::new_default();

    std::string service = "org.freedesktop.systemd1";
    std::string path = "/org/freedesktop/systemd1";
    std::string interface = "org.freedesktop.systemd1.Manager";
    std::string method = "ListUnitsByNames";
    auto msg = bus.new_method_call(service.c_str(), path.c_str(), interface.c_str(), method.c_str());

    // Append the service names to the message
    msg.append(serviceNames);

    try {
        auto reply = bus.call(msg);

        std::vector<UnitStruct> units;
        reply.read(units);

        for (const UnitStruct& unit : units)
        {
            const std::string& serviceName = std::get<UNIT_NAME>(unit);
            const std::string& activeState = std::get<UNIT_ACTIVE_STATE>(unit);

            if (activeState.compare("active") == 0) {
                std::cout << serviceName << " is active" << std::endl;
                return true;
            } else if (activeState.compare("inactive") == 0) {
                std::cout << serviceName << " is inactive" << std::endl;
                return false;
            } else if (activeState.compare("failed") == 0) {
                std::cout << serviceName << " has failed" << std::endl;
                return false;
            } else {
                std::cout << serviceName << " status is unknown" << std::endl;
            }
        }
        std::cout << "Unknown situation" << std::endl;

    } catch (const sdbusplus::exception::SdBusError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return false;
}

int main() {
    std::vector<std::string> serviceNames = {"ssh.service"};
    getServiceStatus(serviceNames);
    return 0;
}