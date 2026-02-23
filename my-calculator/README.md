# my-calculator

This example uses one D-Bus contract and demonstrates four implementation styles:

- `boost::asio::io_context` + regular class
- `boost::asio::io_context` + CRTP
- `sdbusplus::async::context` + regular class
- `sdbusplus::async::context` + CRTP

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
- D-Bus policy: `my-calculator.conf` to `/etc/dbus-1/system.d`

If you just installed the policy, reload dbus:

```bash
sudo systemctl reload dbus
```

### 1.2 Start the service

Run one of the four binaries (all request the same service name):

```bash
sudo ./build/my-calculator/boost_asio_caculator
# or
sudo ./build/my-calculator/boost_asio_crtp_caculator
# or
sudo ./build/my-calculator/sdbusplus_async_caculator
# or
sudo ./build/my-calculator/sdbusplus_async_crtp_caculator
```

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
	- Declares four executables
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

- `boost_asio_caculator.cpp`
	- `boost::asio::io_context` + `sdbusplus::asio`
	- Single object path (root)

- `boost_asio_crtp_caculator.cpp`
	- `boost::asio::io_context` + CRTP
	- Creates three objects: decimal/binary/heximal

- `sdbusplus_async_caculator.cpp`
	- `sdbusplus::async::context` + coroutine `task<>`
	- Single object path (root)

- `sdbusplus_async_crtp_caculator.cpp`
	- `sdbusplus::async::context` + CRTP + coroutine `task<>`
	- Three objects: decimal/binary/heximal

---

## 3) `boost::asio::io_context` vs `sdbusplus::async::context`

- `boost::asio::io_context`
	- General-purpose event loop (network/timer/signal integration)
	- Connected to D-Bus in this project via `sdbusplus::asio::connection`
	- Async style is callback / `yield_context` oriented

- `sdbusplus::async::context`
	- Async runtime tailored for sdbusplus
	- Method handlers can return `sdbusplus::async::task<T>` directly
	- More coroutine / `co_await` native style for D-Bus handlers

Practical rule of thumb:

- If your system already uses many Asio components, `io_context` is easier to integrate
- If your focus is coroutine-first D-Bus service code, `sdbusplus::async::context` is usually cleaner

---

## 4) `virtual override` vs CRTP

Both are polymorphism techniques, but they resolve function calls at different times.

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

Contract checking (same idea used in this repo):

```cpp
template <typename D>
concept CalculatorContract = requires(D d, const D cd, int64_t x, int64_t y) {
		{ d.multiplyImpl(x, y) } -> std::same_as<int64_t>;
		{ cd.expressImpl() } -> std::same_as<std::string>;
};

static_assert(CalculatorContract<BinaryCalculator>);
```

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

- CRTP files (`boost_asio_crtp_caculator.cpp`, `sdbusplus_async_crtp_caculator.cpp`)
	- One reusable base for D-Bus glue
	- Derived classes only customize behavior (especially `Express()` for decimal/binary/heximal)
	- Better fit for "same interface, multiple formatting strategies"

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
- Prefer CRTP when you want zero-overhead static polymorphism and strong compile-time contracts

