/*

g++ -o ../greeting_service greeting_service.cpp -lsdbusplus -lpthread

sudo service dbus --full-restart

./greeting_service

busctl --system com.example.GreetingService /com/example/Greeting com.example.Greeting Get

busctl --system com.example.GreetingService /com/example/Greeting com.example.Greeting Set s Yes

*/

#include <iostream>
#include <string>
#include <thread>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

class GreetingService {
private:
// bus
    sdbusplus::bus::bus& bus;
// service & object path
    std::string serviceName;
    std::string objectPath;
// interface
    std::string interfaceGreeting;

private:
		std::string greeting;

public:
    GreetingService(sdbusplus::bus::bus& bus, const std::string& path)
        :
        bus(bus),
        serviceName("com.example.GreetingService"),
        objectPath(path),        
        interfaceGreeting("com.example.Greeting")
        // interfaceOther() ...
    {
        // Register the interface with D-Bus
        bus.request_name(this->serviceName);

        // Register the object path
        bus.register_object(path, *this);

				// Register property and method
				this->registerGreeting();
				
        // Setup the property with a setter and getter
        //property = bus.add_property<std::string>(
        //    interfaceName, "Name", name,
        //    [this](const std::string& new_name) { 
        //        name = new_name; 
        //        std::cout << "Name changed to: " << name << std::endl;
        //    });
        
        // Setup the method
        //method = bus.add_method(interfaceName, "GetGreeting", 
        //    [this]() { return "Hello " + name; });
        // Reset the method to unregister it
        //method.reset();
        
        /* Reasons to Store Registered Methods
				Lifetime Management:
					Storing the method as a member variable can help manage its lifetime and ensure that the method remains registered as long as the object exists.
					If you don’t store it and the variable goes out of scope, the method could become unregistered unintentionally.
				Access:
					If you need to access the method from multiple member functions of the class
					(for example, to unregister it conditionally), storing it as a member variable makes this easier.
				Code Clarity:
					It can improve code clarity, making it evident which methods are associated with which objects.
				Error Handling:
					If your application needs to handle errors or changes dynamically
					, having the registered method stored allows for easier updates or re-registration.
				*/
    }

private:
		// interface Greeting
		void registerGreeting() {

				//
				// Property
				//
				
				// Value
		    bus.add_property<std::string>(
			      this->interfaceGreeting, "Value", this->greeting,
			      [this](const std::string& newGreeting) { });
			      
			  //
			  // Method
			  //
			  
			  // Get
        bus.add_method(
		        this->interfaceGreeting, "Get",
		            [this]() {
				            return greeting;
				        });
	      
	      // Set
	      bus.add_method(
		        this->interfaceGreeting, "Set",
		            [this](const std::string& newGreeting) {
				            return name = newGreeting;
				        });
		}
		
    // D-Bus signal to notify clients of name changes
    void emitNameChangedSignal() {
        // Implementation to emit signal
    }
};

int main() {

    // Create a D-Bus bus
#if defined(DBUS_SYSTEM)
    // Create a system bus connection
    auto bus = sdbusplus::bus::new_system();
#elif defined(DBUS_SYSTEM)
    // Create a session bus connection
    auto bus = sdbusplus::bus::new_session();
#else
		// Default
    auto bus = sdbusplus::bus::new_default();
#endif

    // Create the GreetingService object
    GreetingService service(bus, "/com/example/Greeting");


    // Start the D-Bus event loop
    while (true) {
        bus.process();  // This blocks until there are events to process
    }

    // Start the D-Bus event loop
    //while (true) {
    //    bus.process_discard();
    //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //}
    // use bus.process_discard() if you want to skip unhandled messages
    // , but be careful as this may discard important messages.

    return 0;
}
