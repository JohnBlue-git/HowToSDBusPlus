This guide explains the `sdbusplus` calculator example and its YAML-generated
server, asynchronous CRTP server, and asynchronous client bindings.

## 0. Build and Run

Run these commands from the repository root. The generated examples are
controlled by the `generated-via-yaml-examples` Meson group option:

```bash
meson setup build --wipe \
    -Dgenerated-via-yaml-examples=enabled
meson compile -C build
```

To build only the generated examples and benchmark examples, disable the basic
group:

```bash
meson setup build --wipe \
    -Dbasic-examples=disabled \
    -Dbenchmark-examples=enabled \
    -Dgenerated-via-yaml-examples=enabled
meson compile -C build
```

The same build works in the provided Docker image:

```console
docker run --rm \
    -v "$(pwd):/workspace" \
    -w /workspace \
    johnbluedocker/sdbusplus-dev:latest \
    bash -lc '
        meson setup build-docker --wipe \
            -Dgenerated-via-yaml-examples=enabled
        meson compile -C build-docker
    '
```

The build produces:

- `calculator-server`: synchronous generated server using virtual overrides.
- `calculator-aserver`: asynchronous generated server using CRTP and
  `sdbusplus::async::task` handlers.
- `calculator-client`: asynchronous generated client proxy.

After installing the policy with `meson install`, start D-Bus before running
the server/client pair. See [DBUS_USAGE.md](../docs/DBUS_USAGE.md) for common
bus commands and [INSTALL_SDBUSPLUS.md](../docs/INSTALL_SDBUSPLUS.md) for host
dependencies.

## 1. Source Structure & Meson Build Flow

The project separates **Definition** (YAML), **Implementation** (CPP), and **Build Logic** (Meson).

### Project Source Tree

```text
generated-via-yaml-examples/
├── meson.build                   # Build definitions
├── yaml/                         # Interface definitions
│   └── net/poettering/
│       ├── Calculator.interface.yaml
│       └── Calculator.events.yaml
├── calculator-server.cpp         # Main server implementation
├── calculator-aserver.cpp        # Asio-based implementation
└── calculator-client.cpp         # Client/Proxy implementation

```

1. **`calculator-server.cpp`**: Synchronous. Simple, but blocks the bus during processing.
2. **`calculator-aserver.cpp`**: Asynchronous. Uses
    `sdbusplus::async::context` and coroutine tasks to stay responsive during
    long tasks.
3. **`calculator-client.cpp`**: Uses the generated **Client Proxy** to call the server as if it were a local C++ object.

### The Meson/Ninja Build Flow

1. **Find Tooling**: Meson locates `sdbus++` and the local
    `sdbus++-gen-meson` helper.
2. **Run Generator**: Meson triggers the generator to parse the YAML and
    generate:
* `server.hpp`: Synchronous server interfaces.
* `aserver.hpp`: Asynchronous server interfaces and CRTP dispatch support.
* `client.hpp`: Proxy classes for callers.
* `event.hpp`: Exception and event classes from `events.yaml`.


3. **Compile**: The C++ compiler takes your `.cpp` files, includes the generated `.hpp` files, and creates the binary.

### Build Folder Structure (The Result)

The `gen/` directory (inside your build folder) contains the generated artifacts:

```text
build/
├── generated-via-yaml-examples/gen/net/poettering/Calculator/
│   ├── server.hpp / .cpp     <-- Included by calculator-server.cpp
│   ├── client.hpp / .cpp     <-- Included by calculator-client.cpp
│   ├── event.hpp / .cpp      <-- Error/Signal handling
│   └── common.hpp / .cpp     <-- Shared enums/types
└── calculator-server         <-- Final Executable

```

common.hpp
```cpp
namespace sdbusplus::common::net::poettering
{

struct Calculator
{
    static constexpr auto interface = "net.poettering.Calculator";
    ...
}
```

event.hpp
```cpp
namespace sdbusplus::error::net::poettering::Calculator
{

struct DivisionByZero final :
    public sdbusplus::exception::generated_event<DivisionByZero>
{
    ...
};

}
```

server.hpp
```cpp
namespace sdbusplus::aserver::net::poettering
{

class Calculator :
    public sdbusplus::common::net::poettering::Calculator
{
    public:
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *         - Move operations due to 'this' being registered as the
         *           'context' with sdbus.
         *     Allowed:
         *         - Destructor.
         */
         ...
};

}
```

aserver.hpp
```cpp
namespace sdbusplus::aserver::net::poettering
{

namespace details
{
// forward declaration
template <typename Instance, typename Server>
class Calculator;
} // namespace details

template <typename Instance, typename Server = void>
struct Calculator :
    public std::conditional_t<
        std::is_void_v<Server>,
        sdbusplus::async::server_t<Instance, details::Calculator>,
        details::Calculator<Instance, Server>>
{
    template <typename... Args>
    Calculator(Args&&... args) :
        std::conditional_t<
            std::is_void_v<Server>,
            sdbusplus::async::server_t<Instance, details::Calculator>,
            details::Calculator<Instance, Server>>(std::forward<Args>(args)...)
    {}
};

namespace details
{

namespace server_details = sdbusplus::async::server::details;

template <typename Instance, typename Server>
class Calculator :
    public sdbusplus::common::net::poettering::Calculator,
    protected server_details::server_context_friend
{
    ...
};
}

}
```

---

## 2. The Role of YAML Files

### `Calculator.interface.yaml` (The "What")

Defines the **Methods** (actions) and **Properties** (data).

* **YAML Snippet**:
```yaml
methods:
  - name: Multiply
    arguments:
      - name: X
        type: x  # int64
      - name: Y
        type: x
    returns:
      - name: Result
        type: x

```


* **Relationship**: Generates a `virtual int64_t multiply(int64_t x, int64_t y) = 0;` inside `server.hpp`.

### `Calculator.events.yaml` (The "Oops")

Defines **Errors** (Exceptions) and **Signals**.

* **YAML Snippet**:
```yaml
events:
  - name: Cleared
    en:
      message: "The calculator is cleared."

```

* **Usage**: Generates `sdbusplus::error::...::Cleared`. You can `throw Cleared();` in C++ to send a D-Bus error.

---

## 3. How Generated Interfaces are Bound

The generated bindings connect C++ functions to the D-Bus bus. The synchronous
server uses virtual overrides, while the asynchronous server uses CRTP
(`Curiously Recurring Template Pattern`) dispatch.

### The Synchronous Developer Implementation

In `calculator-server.cpp`, you bind your logic by inheriting from a template-generated class.

```cpp
// The synchronous generated server uses a virtual interface.
class Calculator : public sdbusplus::server::object_t<
                          sdbusplus::net::poettering::server::Calculator> 
{
    public:
        // Constructor passes the bus and object path up to object_t
        Calculator(sdbusplus::bus_t& bus, const char* path) :
            sdbusplus::server::object_t<
                sdbusplus::net::poettering::server::Calculator>(bus, path) {}

        // Override the generated pure virtual functions
        int64_t add(int64_t x, int64_t y) override {
            return x + y;
        }
};

```

**What `object_t` does automatically:**

* **Registers Object Path**: e.g., `/net/poettering/calculator`.
* **Registers Interface**: e.g., `net.poettering.Calculator`.
* **Introspection**: Handles XML data so tools like `busctl` can "see" your methods.
* **Message Routing**: Automatically maps incoming D-Bus messages to your `add` or `multiply` overrides.

---

In an asynchronous server (like `calculator-aserver.cpp`), the goal is to prevent the D-Bus service from "hanging" while performing a calculation. Instead of blocking the entire thread, the server uses **Boost.ASIO** to handle requests non-blockingly.

---

## 4. How the Async Server Works (`sdbusplus::async`)

The modern async server uses the **Execution Context** pattern. Instead of inheriting from `sdbusplus::server::object_t`, it often uses a context-aware approach where methods return a "Task."

### The Power of `co_await`

In a `task<>` based server, you don't "return" a value immediately. Instead, the function is a coroutine. When you hit a slow operation (like waiting for a timer or another D-Bus call), you `co_await` it. This suspends the function, frees up the CPU for other D-Bus tasks, and resumes automatically when the data is ready.

### Code Comparison: Sync vs. Modern Async

**Synchronous (`calculator-server.cpp`)**

```cpp
int64_t add(int64_t x, int64_t y) override {
    return x + y; // Straightforward, but blocks the thread.
}

```

**Modern Async (`calculator-aserver.cpp`)**

```cpp
// Returns a Task instead of a raw value
sdbusplus::async::task<int64_t> add(int64_t x, int64_t y) override {
    // If we needed to wait for something:
    // co_await some_slow_io(); 
    
    co_return x + y; // 'co_return' handles the D-Bus reply asynchronously
}

```

### Why use the Async Server?

* **Parallelism**: You can handle multiple `Multiply` or `Add` requests simultaneously without multiple threads.
* **Responsiveness**: Tools like `busctl` or the OpenBMC Web UI won't "timeout" while waiting for a slow backend sensor reading.
* **Integration**: It integrates perfectly with other OpenBMC daemons that are already using `boost::asio`.

---

## 5. Comparison: With YAML vs. Without YAML

### Implementation Effort

| Feature | Manual (`sd-bus`) | `sdbusplus` (YAML-generated bindings) |
| --- | --- | --- |
| **Interface** | Vtable arrays in C code. | High-level YAML (declarative). |
| **Type Safety** | **Low**: Manual pack/unpack. | **High**: Generated native C++ types. |
| **Introspection** | Manual XML/Vtable strings. | **Automatic**: Handled by `object_t`. |
| **Boilerplate** | Massive: 70% is D-Bus "glue." | Minimal: You write the "business logic." |
| **Client Side** | Manually construct messages. | Proxy classes generated for you. |

### Code Example: Implementing "Add"

**Without YAML (Manual `sd-bus`)**

```cpp
int handle_add(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    int64_t x, y;
    sd_bus_message_read(m, "xx", &x, &y); // Manual parsing
    int64_t result = x + y;
    return sd_bus_reply_method_return(m, "x", result); // Manual reply
}

```

**With YAML (`sdbusplus`)**

```cpp
int64_t add(int64_t x, int64_t y) override {
    return x + y; // Type-safe C++, no message parsing
}

```
