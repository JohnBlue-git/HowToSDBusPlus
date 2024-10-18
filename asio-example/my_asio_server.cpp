#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>

#include <chrono>
#include <ctime>
#include <iostream>
#include <variant>

using variant = std::variant<int, std::string>;

int foo(int test)
{
    std::cout << "foo(" << test << ") -> " << (test + 1) << "\n";
    return ++test;
}

// called from coroutine context, can make yielding dbus calls
int fooYield(boost::asio::yield_context yield,
             std::shared_ptr<sdbusplus::asio::connection> conn, int test)
{
    // fetch the real value from testFunction
    boost::system::error_code ec;
    std::cout << "fooYield(yield, " << test << ")...\n";
    int testCount = conn->yield_method_call<int>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestFunction", test);
    if (ec || testCount != (test + 1))
    {
        std::cout << "call to foo failed: ec = " << ec << '\n';
        return -1;
    }
    std::cout << "yielding call to foo OK! (-> " << testCount << ")\n";
    return testCount;
}

int methodWithMessage(sdbusplus::message_t& /*m*/, int test)
{
    std::cout << "methodWithMessage(m, " << test << ") -> " << (test + 1)
              << "\n";
    return ++test;
}

int voidBar(void)
{
    std::cout << "voidBar() -> 42\n";
    return 42;
}

auto ipmiInterface(boost::asio::yield_context /*yield*/, uint8_t netFn,
                   uint8_t lun, uint8_t cmd, std::vector<uint8_t>& /*data*/,
                   const std::map<std::string, variant>& /*options*/)
{
    std::vector<uint8_t> reply = {1, 2, 3, 4};
    uint8_t cc = 0;
    std::cerr << "ipmiInterface:execute(" << int(netFn) << int(cmd) << ")\n";
    return std::make_tuple(uint8_t(netFn + 1), lun, cmd, cc, reply);
}

int server()
{
    // setup connection to dbus
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // test object server
    conn->request_name("xyz.openbmc_project.asio-test");
    auto server = sdbusplus::asio::object_server(conn);
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/xyz/openbmc_project/test",
                             "xyz.openbmc_project.test");
    // test generic properties
    iface->register_property("int", 33,
                             sdbusplus::asio::PropertyPermission::readWrite);
    std::vector<std::string> myStringVec = {"some", "test", "data"};
    std::vector<std::string> myStringVec2 = {"more", "test", "data"};

    iface->register_property("myStringVec", myStringVec,
                             sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("myStringVec2", myStringVec2);

    // test properties with specialized callbacks
    iface->register_property("lessThan50", 23,
                             // custom set
                             [](const int& req, int& propertyValue) {
                                 if (req >= 50)
                                 {
                                     return false;
                                 }
                                 propertyValue = req;
                                 return true;
                             });
    iface->register_property(
        "TrailTime", std::string("foo"),
        // custom set
        [](const std::string& req, std::string& propertyValue) {
            propertyValue = req;
            return true;
        },
        // custom get
        [](const std::string& property) {
            auto now = std::chrono::system_clock::now();
            auto timePoint = std::chrono::system_clock::to_time_t(now);
            return property + std::ctime(&timePoint);
        });

    // test method creation
    iface->register_method("TestMethod", [](const int32_t& callCount) {
        return std::make_tuple(callCount,
                               "success: " + std::to_string(callCount));
    });

    iface->register_method("TestFunction", foo);

    // fooYield has boost::asio::yield_context as first argument
    // so will be executed in coroutine context if called
    iface->register_method("TestYieldFunction",
                           [conn](boost::asio::yield_context yield, int val) {
                               return fooYield(yield, conn, val);
                           });

    iface->register_method("TestMethodWithMessage", methodWithMessage);

    iface->register_method("VoidFunctionReturnsInt", voidBar);

    iface->register_method("execute", ipmiInterface);

    iface->initialize();

    io.run();

    return 0;
}


int main()
{
        return server();
}