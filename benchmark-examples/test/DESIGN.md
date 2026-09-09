# Benchmark Design

`benchmark_compare.py` measures request handling through the system or user
D-Bus, rather than calling calculator functions directly. Each executable is
started as a separate process, discovered with `busctl introspect`, exercised
with the same `Multiply` workload, and stopped before the next executable is
started.

## Why the test is sequential

All implementations use the same service name and interface. Running them in
parallel would make service ownership and object discovery nondeterministic.
The parameterized pytest cases therefore share one fixture lifecycle and
explicitly reject pytest-xdist workers.

## Measurements

- Throughput and average latency are measured around only the D-Bus calls.
- Warmup calls are excluded from the timed interval.
- A background thread samples `/proc/<pid>/status` for peak RSS and HWM.
- The same object path is configured for each executable in `TARGETS`.

The results are comparative, not a microbenchmark of arithmetic. They include
message marshalling, dispatch, any implementation-specific scheduling, and
process memory overhead. The CRTP calculator handlers currently execute the
CPU-only arithmetic path synchronously; async scheduling should be introduced
only for methods that actually await I/O.

The `sdbusplus_async_sleep_crtp_caculator` target is intentionally different:
its `Multiply` method awaits a 10 ms timer. It demonstrates coroutine
suspension and should be benchmarked separately from the zero-wait targets.

## Fairness and limits

Run the same build mode and bus mode for every target. Keep CPU frequency and
system load stable, and repeat runs before drawing conclusions. The benchmark
does not currently measure `Divide`, `Express`, `Clear`, property access, or
error paths, so those behaviors still need focused functional tests.