# benchmark-examples

The source layout separates implementation style from test and contract data:

- `source/non-async/`: regular, non-coroutine implementation
- `source/async/`: Asio and native sdbusplus coroutine implementations
- `source/crtp/`: CRTP implementations and the shared C++ Concept contract
- `source/async-crtp/`: asynchronous CRTP implementations, including the YAML-generated version
- `include/`: shared constants used by handwritten implementations
- `yaml/`: D-Bus YAML contract and generation Meson files
- `test/`: benchmark runner and its design notes

This example uses one D-Bus contract and demonstrates six implementation styles:

- `boost::asio::io_context` + regular class
- `boost::asio::io_context` + CRTP
- `sdbusplus::async::context` + regular class
- `sdbusplus::async::context` + CRTP
- `sdbusplus::async::context` + CRTP (generated from YAML by `sdbus++`)
- `sdbusplus::async::context` + CRTP (generated from YAML, with an awaited timer)

---

## 1) Usage (including D-Bus service/object/interface layout)

### 1.1 Build & install

Run from repository root:

```bash
meson setup build --wipe
meson compile -C build
sudo meson install -C build
```

`meson.build` installs:

- Executables:
	- `boost_asio_caculator`
	- `boost_asio_crtp_caculator`
	- `sdbusplus_async_caculator`
	- `sdbusplus_async_crtp_caculator`
	- `sdbusplus_async_sleep_crtp_caculator`
	- `yaml_generated_crtp_caculator`
	- `yaml_generated_sleep_crtp_caculator`
- D-Bus policy: `my-calculator.conf` to `/etc/dbus-1/system.d`

If you just installed the policy, reload dbus:

```bash
sudo systemctl reload dbus
```

### 1.2 Start the service

Run one of the eight binaries (all request the same service name):

```bash
sudo ./build/benchmark-examples/boost_asio_caculator
# or
sudo ./build/benchmark-examples/boost_asio_crtp_caculator
# or
sudo ./build/benchmark-examples/sdbusplus_async_caculator
# or
sudo ./build/benchmark-examples/sdbusplus_async_crtp_caculator
# or
sudo ./build/benchmark-examples/sdbusplus_async_sleep_crtp_caculator
# or
sudo ./build/benchmark-examples/yaml_generated_sleep_crtp_caculator
# or
sudo ./build/benchmark-examples/yaml_generated_crtp_caculator
```

### 1.2.1 YAML-generated CRTP target

`yaml_generated_crtp_caculator` and `yaml_generated_sleep_crtp_caculator` are
generated and built from:

- `xyz/openbmc_project/Calculator.interface.yaml`
- `xyz/openbmc_project/Calculator.events.yaml`

Meson runs `sdbus++-gen-meson` to produce generated headers/sources (`common.hpp`,
`event.hpp`, `aserver.hpp`, ...), then compiles the YAML CRTP source
against them. The sleep target uses the same implementation with `Multiply`
awaiting a 10 ms timer.

### 1.3 D-Bus interface model

- Service: `xyz.openbmc_project.Calculator`
- Interface: `xyz.openbmc_project.Calculator`

Object paths depend on implementation:

- Single-object variants: `/xyz/openbmc_project/calculator`
	- `boost_asio_caculator`
	- `sdbusplus_async_caculator`
- Multi-object (CRTP) variants:
	- `/xyz/openbmc_project/calculator/decimal`
	- `/xyz/openbmc_project/calculator/binary`
	- `/xyz/openbmc_project/calculator/heximal`

Methods (from `Calculator.interface.yaml` + implementation):

- `Multiply(x:int64, y:int64) -> z:int64`
- `Divide(x:int64, y:int64) -> z:int64` (`y=0` throws an error)
- `Express() -> z:string`
- `Clear()` (emits `Cleared` signal)

Properties:

- `LastResult : int64` (rw)
- `Status : string` (ro)
- `Base : string` (ro)
- `Owner : string` (rw)

Signal:

- `Cleared(oldValue:int64)`

### 1.4 busctl quick test commands

Check object tree first:

```bash
busctl tree xyz.openbmc_project.Calculator
```

Example using the single-object variant:

```bash
OBJ=/xyz/openbmc_project/calculator
SVC=xyz.openbmc_project.Calculator
IFACE=xyz.openbmc_project.Calculator

busctl introspect "$SVC" "$OBJ"
busctl call "$SVC" "$OBJ" "$IFACE" Multiply xx 6 7
busctl call "$SVC" "$OBJ" "$IFACE" Divide xx 20 5
busctl call "$SVC" "$OBJ" "$IFACE" Express
busctl get-property "$SVC" "$OBJ" "$IFACE" LastResult
busctl call "$SVC" "$OBJ" org.freedesktop.DBus.Properties Set ssv \
	"$IFACE" Owner s root
busctl call "$SVC" "$OBJ" "$IFACE" Clear
```

Monitor signal:

```bash
busctl monitor --match="type='signal',sender='xyz.openbmc_project.Calculator',interface='xyz.openbmc_project.Calculator',member='Cleared'"
```

For CRTP multi-object variants, switch `OBJ` to one of:

- `/xyz/openbmc_project/calculator/decimal`
- `/xyz/openbmc_project/calculator/binary`
- `/xyz/openbmc_project/calculator/heximal`

---

## 2) Structure (purpose of each file)

- `meson.build`
	- Declares eight executables
	- Installs `my-calculator.conf` into the D-Bus policy directory

- `my-calculator.conf`
	- Allows owning and sending to `xyz.openbmc_project.Calculator`
	- On system bus, requests are often denied without this policy

- `Calculator.interface.yaml`
	- Interface contract (methods / properties / signals / enums)
	- Used as the main D-Bus contract document in this project

- `Calculator.events.yaml`
	- Error and event descriptions (`DivisionByZero`, `PermissionDenied`, `Cleared`)
	- Complements the interface YAML with behavior semantics

- `calculator_enum.hpp`
	- Central constants for service/interface/object paths/error names
	- Avoids duplicated hard-coded strings across `.cpp` files

- `source/crtp/calculator_contract.hpp`
	- C++ Concept defining the async CRTP method contract
	- Used by the hand-written async CRTP services before interface setup

- `boost_asio_caculator.cpp`
	- `boost::asio::io_context` + `sdbusplus::asio`
	- Single object path (root)

- `boost_asio_crtp_caculator.cpp`
	- `boost::asio::io_context` + CRTP with synchronous CPU-only handlers
	- Creates three objects: decimal/binary/heximal

- `sdbusplus_async_caculator.cpp`
	- `sdbusplus::async::context` + coroutine `task<>`
	- Single object path (root)

- `sdbusplus_async_crtp_caculator.cpp`
	- `sdbusplus::async::context` + CRTP with direct synchronous handlers
	- Three objects: decimal/binary/heximal

- `async-crtp/sdbusplus_async_sleep_crtp_caculator.cpp`
	- `sdbusplus::async::context` + CRTP + `co_await sleep_for`
	- Delays `Multiply` asynchronously by 10 ms to demonstrate suspension
	- Keeps the event loop available for other D-Bus requests while waiting

- `yaml_generated_sleep_crtp_caculator.cpp`
	- `sdbusplus::async::context` + CRTP based on generated `aserver.hpp`
	- Uses generated `common.hpp` / `event.hpp` / `aserver.hpp`
	- Three objects: decimal/binary/heximal
	- Adds a 10 ms `co_await sleep_for` to successful `Multiply` and `Divide`
	  calls for an apples-to-apples async suspension comparison

- `source/crtp/yaml_generated_crtp_caculator.cpp`
	- Uses the same generated YAML CRTP implementation without the timer wait
	- Provides the fair CPU-only counterpart to the sleep target

- `xyz/openbmc_project/Calculator.interface.yaml`
	- YAML contract used by `sdbus++` code generation for this target

- `xyz/openbmc_project/Calculator.events.yaml`
	- YAML error/event definitions used by `sdbus++` code generation

---

## 3) `boost::asio::io_context` vs `sdbusplus::async::context`

- `boost::asio::io_context`
	- General-purpose event loop (network/timer/signal integration)
	- Connected to D-Bus in this project via `sdbusplus::asio::connection`
	- Async style is callback / `yield_context` oriented

- `sdbusplus::async::context`
	- Async runtime tailored for sdbusplus
	- This CRTP example keeps CPU-only methods synchronous to avoid an unnecessary
	  coroutine scheduling hop
	- Use `sdbusplus::async::task<T>` when a method actually awaits I/O

Practical rule of thumb:

- If your system already uses many Asio components, `io_context` is easier to integrate
- If your focus is coroutine-first D-Bus service code, `sdbusplus::async::context` is usually cleaner

---

## 4) `virtual override` vs CRTP

Both are polymorphism techniques, but they resolve function calls at different times.

The examples below show the general design difference. This repository uses
CRTP for the calculator service glue; it does not add a virtual calculator base
class. The D-Bus vtable or generated `aserver.hpp` dispatches into the selected
derived implementation.

- `virtual override`: runtime polymorphism (vtable dispatch)
- CRTP: compile-time polymorphism (template instantiation + static dispatch)

### 4.1 `virtual override` (runtime dispatch)

Typical shape:

```cpp
struct CalculatorApi {
		virtual ~CalculatorApi() = default;
		virtual int64_t multiply(int64_t x, int64_t y) = 0;
		virtual std::string express() const = 0;
};

class DecimalCalculator : public CalculatorApi {
	public:
		int64_t multiply(int64_t x, int64_t y) override {
				lastResult_ = x * y;
				return lastResult_;
		}

		std::string express() const override {
				return std::to_string(lastResult_);
		}

	private:
		int64_t lastResult_ = 0;
};
```

Call site behavior:

```cpp
void serve(CalculatorApi& calc) {
		auto z = calc.multiply(6, 7); // resolved at runtime via vtable
		(void)z;
}
```

Pros:

- Very intuitive OOP model
- Swappable implementations behind a base reference/pointer

Trade-off:

- Runtime indirection (vtable)
- Interface mismatch is found when implementing/compiling derived classes, but not as rich as contract-style template constraints

### 4.2 CRTP (compile-time dispatch)

Typical shape:

```cpp
template <typename Derived>
class CalculatorBase {
	public:
		int64_t multiply(int64_t x, int64_t y) {
				return derived().multiplyImpl(x, y); // resolved at compile time
		}

		std::string express() const {
				return derived().expressImpl();
		}

	private:
		Derived& derived() { return static_cast<Derived&>(*this); }
		const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

class BinaryCalculator : public CalculatorBase<BinaryCalculator> {
	public:
		int64_t multiplyImpl(int64_t x, int64_t y) {
				lastResult_ = x * y;
				return lastResult_;
		}

		std::string expressImpl() const {
				return "0b" + toBinary(lastResult_);
		}

	private:
		int64_t lastResult_ = 0;
		static std::string toBinary(int64_t v);
};
```

Contract checking in this repository:

```cpp
#include "calculator_contract.hpp"

template <typename Derived>
class SdbusplusAsyncCalculatorService {
	void checkContract() const {
		static_assert(CalculatorContract<Derived>);
	}
};

// CalculatorResult accepts either Value or sdbusplus::async::task<Value>.
// This supports synchronous CPU-only CRTP handlers and coroutine handlers.
```

The shared contract in `source/crtp/calculator_contract.hpp` checks all four
calculator operations:

```cpp
calculator.multiply(x, y)  -> int64_t or task<int64_t>
calculator.divide(x, y)    -> int64_t or task<int64_t>
constCalculator.express()  -> std::string or task<std::string>
calculator.clear()         -> void or task<void>
```

Each CRTP constructor calls `checkContract()` before `setupInterface()`, so a
derived class that violates the method contract fails at compile time before
the D-Bus interface is registered.

Pros:

- No virtual dispatch cost
- Strong compile-time contract checking (`requires` / `static_assert`)
- Great for sharing infrastructure while specializing behavior

Trade-off:

- More template complexity
- Can increase compile time and error-message complexity

### 4.3 How this repo applies them

- Non-CRTP files (`boost_asio_caculator.cpp`, `sdbusplus_async_caculator.cpp`)
	- One service class with all logic in one place
	- Easier to read when behavior variants are not required

- CRTP files (`boost_asio_crtp_caculator.cpp`, `sdbusplus_async_crtp_caculator.cpp`,
  and the generated YAML CRTP targets)
	- One reusable base for D-Bus glue
	- Derived classes only customize behavior (especially `Express()` for decimal/binary/heximal)
	- Better fit for "same interface, multiple formatting strategies"
	- The YAML pair uses the same generated contract; only the sleep target awaits
	  a timer in its method implementation

### 4.4 Dispatch difference at a glance

```cpp
// Virtual: runtime dispatch
CalculatorApi* p = new DecimalCalculator();
auto a = p->express();

// CRTP: compile-time dispatch
BinaryCalculator b;
auto c = b.express();
```

In short:

- Prefer `virtual override` when runtime substitution is the primary goal
- Prefer CRTP when the implementation is selected at compile time and you want
	strong compile-time contracts
- For this benchmark, do not interpret CRTP as automatically faster: D-Bus
	marshalling, coroutine scheduling, and timer waits dominate the measured path

---

## 5) Comprehensive comparison between all executables

This section benchmarks all `benchmark-examples` executables with the same D-Bus
contract and compares:

- Handling speed (`Ops/s`, `Avg ms/op`)
- Memory usage (`Peak RSS`, `Peak HWM`)

### 5.1 Build in optimized mode (`O2` / `O3`)

From repository root, enable the benchmark group and choose its optimization
mode via the Meson option `benchmark-examples-opt-mode`:

```bash
meson setup build --wipe \
	-Dbenchmark-examples=enabled \
	-Dbenchmark-examples-opt-mode=O2
meson compile -C build
```

or:

```bash
meson setup build --wipe \
	-Dbenchmark-examples=enabled \
	-Dbenchmark-examples-opt-mode=O3
meson compile -C build
```

> Current choices: `default`, `O2`, `O3`.

### 5.2 Pytest benchmark workflow (fixture style)

Benchmark file: `benchmark_compare.py`

- Uses `pytest` parameterization to iterate all executables
- Uses fixture to start one service process per test case and stop it in teardown
- Runs sequentially (do **not** use `pytest-xdist` parallel mode)
- Because all binaries share the same service/interface name,
  fixture lifecycle avoids cross-case interference

### 5.3 Run benchmark

Install pytest if needed:

```bash
apt update
apt install -y python3-pip
python3 -m pip install -U pytest
```

Run from repository root:

```bash
pytest -s benchmark-examples/test/benchmark_compare.py

python3 -m pytest -s benchmark-examples/test/benchmark_compare.py
```

### 5.4 Optional runtime controls (environment variables)

You can tune benchmark behavior without editing code:

- `MYCALC_BUILD_DIR` (default: `build/benchmark-examples`)
- `MYCALC_ITERATIONS` (default: `300`)
- `MYCALC_WARMUP` (default: `40`)
- `MYCALC_STARTUP_TIMEOUT` (default: `8.0`)
- `MYCALC_BUS` (`system` or `user`, default: `system`)
- `MYCALC_USE_SUDO` (`1/true/yes` to enable `sudo -n` startup)
- `MYCALC_ONLY` (comma-separated executable names)

Example (only run two executables with custom loop count):

```bash
MYCALC_ITERATIONS=1000 \
MYCALC_ONLY=boost_asio_caculator,sdbusplus_async_caculator \
pytest -s benchmark-examples/test/benchmark_compare.py
```

### 5.5 Output interpretation

At session end, pytest prints a summary table and rankings:

- Speed ranking: high `Ops/s` is better
- Memory ranking: low `Peak HWM` is better

For fair comparison:

- Keep CPU governor/system load stable
- Run each mode (`O2` / `O3`) multiple times
- Compare trends, not just a single run

### 5.6 Latest validation results

Terms:
- **Ops/s**  
  Operations per second (throughput). **Higher is better**.
- **Avg ms/op**  
  Average milliseconds per operation (latency). **Lower is better**.  
  Roughly inverse to Ops/s:  
  $$\text{Avg ms/op} \approx \frac{1000}{\text{Ops/s}}$$
- **Peak RSS (KiB)**  
  Peak resident memory usage (physical RAM used), in KiB.  
  **Lower is generally better**.
- **Peak HWM (KiB)**  
  High-water mark of resident memory during process lifetime (highest RSS ever reached), in KiB.
The latest Docker build completed successfully with the following validation
command:

```console
docker run --rm \
	-v "$(pwd):/workspace" \
	-w /workspace \
	johnbluedocker/sdbusplus-dev:latest \
	meson compile -C build-docker
```

Result: **54/54 build steps passed**, including:

- all basic examples;
- all synchronous and asynchronous benchmark examples;
- both `sdbusplus_async_sleep_crtp_caculator` and `yaml_generated_sleep_crtp_caculator`;
- YAML code generation and all `generated-via-yaml-examples` executables.

The latest runtime benchmark was executed inside
`johnbluedocker/sdbusplus-dev:latest` with an isolated session-configured bus
exposed through `DBUS_SYSTEM_BUS_ADDRESS`. It used `MYCALC_ITERATIONS=5`,
`MYCALC_WARMUP=1`, and all eight targets. All tests passed in 0.66 seconds:

```text
Executable                                Ops/s    Avg ms/op  Peak RSS(KiB)  Peak HWM(KiB)
----------------------------------------------------------------------------
non_async_caculator                      215.86       4.6325           5100           5100
boost_asio_caculator                     269.93       3.7047           5356           5356
boost_asio_crtp_caculator                249.99       4.0001           5324           5324
sdbusplus_async_caculator                302.03       3.3110           5180           5180
sdbusplus_async_crtp_caculator           235.71       4.2425           5368           5368
sdbusplus_async_sleep_crtp_caculator      65.68      15.2263           5300           5300
yaml_generated_crtp_caculator            281.78       3.5489           5356           5356
yaml_generated_sleep_crtp_caculator       66.52      15.0328           5496           5496
```

Speed ranking for this run was:

```text
sdbusplus_async_caculator > yaml_generated_crtp_caculator > boost_asio_caculator
> boost_asio_crtp_caculator > sdbusplus_async_crtp_caculator
> non_async_caculator > yaml_generated_sleep_crtp_caculator
> sdbusplus_async_sleep_crtp_caculator
```

The sleep-based async targets are intentionally not fair CPU-only competitors:
their successful `Multiply` and `Divide` methods await a 10 ms timer. Their
measured latency includes that wait plus D-Bus overhead and demonstrates
coroutine suspension rather than arithmetic throughput. These are short-sample
development results, not production performance numbers; repeat with larger
iteration counts and a stable host when comparing small differences.

### Why the async sleep target is slower

The `co_await` operation does not make one request complete faster. It
intentionally suspends successful `Multiply` and `Divide` calls for 10 ms:

```text
measured latency ~= 10 ms timer wait
					+ D-Bus round trip
					+ busctl process startup
					+ coroutine scheduling
```

The benchmark sends requests sequentially and waits for each `busctl` command
to finish before sending the next one. Therefore every request includes the
full timer delay. This measures single-request latency, not the concurrency
benefit of asynchronous execution.

The benefit of `co_await` appears when multiple requests or other D-Bus work
can run while one request is waiting for I/O or a timer. For example, ten
concurrent 10 ms requests can complete in roughly one timer interval plus
transport overhead, while ten blocking requests would wait for approximately
ten intervals. A concurrent client benchmark is required to measure that
behavior fairly.

For this reason, interpret the targets separately:

- zero-wait targets compare CPU-only D-Bus dispatch and arithmetic overhead;
- `sdbusplus_async_sleep_crtp_caculator` and `yaml_generated_sleep_crtp_caculator`
	demonstrate coroutine suspension;
- their lower Ops/s is expected and does not indicate that CRTP itself is slow.

When runtime D-Bus is available, run the zero-wait implementations together:

```bash
MYCALC_ONLY=non_async_caculator,boost_asio_caculator,boost_asio_crtp_caculator,sdbusplus_async_caculator,sdbusplus_async_crtp_caculator,yaml_generated_crtp_caculator \
pytest -s benchmark-examples/test/benchmark_compare.py
```

Run the timer-based async examples separately because each successful `Multiply`
and `Divide` call intentionally waits 10 ms:

```bash
MYCALC_ONLY=sdbusplus_async_sleep_crtp_caculator,yaml_generated_sleep_crtp_caculator \
pytest -s benchmark-examples/test/benchmark_compare.py
```

