# HowToSDBusPlus

This repository is a practical collection of C++ examples for
[sdbusplus](https://github.com/openbmc/sdbusplus), the modern C++ layer on top
of systemd's sd-bus API. The examples progress from small D-Bus calls to
generated interfaces, asynchronous services, CRTP implementations, and
benchmark comparisons.

## Project Structure

```text
.
├── basic-examples/          # Small, focused D-Bus examples
├── benchmark-examples/      # Calculator implementations and benchmarks
│   ├── source/              # Asio, coroutine, CRTP, and YAML-generated services
│   ├── include/             # Shared handwritten example headers
│   ├── non_async/           # Synchronous implementation
│   ├── test/                # Benchmark runner and benchmark design
│   └── yaml/                # Calculator contract used by code generation
├── generated-via-yaml-examples/ # Standalone generated server/client examples
├── docs/                    # D-Bus usage and sdbusplus installation guides
├── tools/                   # Local sdbus++ and Meson helper tools
├── Dockerfile               # Reproducible development image definition
└── meson.build              # Top-level build configuration
```

Build directories such as `build/`, `build-docker/`, and `build-debug/` are
ignored by Git.

## Examples

### Basic examples

- [simple-dbuscall](basic-examples/simple-dbuscall/README.md): synchronous
  method calls and timeout handling.
- [use-systemd1](basic-examples/use-systemd1/README.md): querying systemd
  services through D-Bus.
- [emit-signal](basic-examples/emit-signal/README.md): emitting and receiving
  D-Bus signals.
- [asio-example](basic-examples/asio-example): integrating Boost.Asio with
  sdbusplus.
- [get-all-properties](basic-examples/get-all-properties): reading D-Bus
  properties.
- [list-users](basic-examples/list-users): calling a system service.
- [register-property](basic-examples/register-property): registering service
  properties.

### Generated examples

- [generated-via-yaml-examples](generated-via-yaml-examples/README.md): generates
  common, server, client, and event bindings from YAML.
- [benchmark-examples](benchmark-examples/README.md): compares synchronous,
  Asio, native coroutine, handwritten CRTP, and YAML-generated services.

## Build With Docker Image

The easiest way to build this project is the prebuilt image:
`johnbluedocker/sdbusplus-dev`.

It already includes the compiler, Meson, Ninja, Boost, systemd development
headers, Python generator dependencies, and sdbusplus. You do not need to
install the complete toolchain or build sdbusplus manually on the host. The
repository is mounted into the container, so source changes remain in the
workspace and build output can be discarded safely.

Pull the image once:

```console
docker pull johnbluedocker/sdbusplus-dev:latest
```

Build the project from the repository root:

```console
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  johnbluedocker/sdbusplus-dev:latest \
  bash -lc '
    meson setup build-docker --wipe
    meson compile -C build-docker
  '
```

For a persistent interactive development shell:

```console
docker run --rm -it \
  -v "$(pwd):/workspace" \
  -w /workspace \
  johnbluedocker/sdbusplus-dev:latest \
  /bin/bash
```

Then run inside the container:

```bash
meson setup build-docker --wipe
meson compile -C build-docker
```

When D-Bus runtime testing is needed, pass the host D-Bus socket and policy
directory as well:

```console
docker run --rm -it \
  -v /var/run/dbus:/var/run/dbus \
  -v /etc/dbus-1/system.d:/etc/dbus-1/system.d \
  -v "$(pwd):/workspace" \
  -w /workspace \
  -e DBUS_SYSTEM_BUS_ADDRESS=unix:path=/var/run/dbus/system_bus_socket \
  johnbluedocker/sdbusplus-dev:latest \
  /bin/bash
```

For an isolated container-only bus, use a session-configured `dbus-daemon` and
point sdbusplus at it explicitly. The benchmark defaults to the system bus, so
`dbus-run-session` alone is not sufficient:

```console
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  johnbluedocker/sdbusplus-dev:latest \
  bash -lc '
    python3 -m pip install -q pytest
    meson setup build-docker --wipe
    meson compile -C build-docker
    dbus-daemon --session --address=unix:path=/tmp/sdbusplus-bus \
      --nofork --nopidfile >/tmp/dbus.log 2>&1 &
    bus_pid=$!
    trap "kill $bus_pid" EXIT
    export DBUS_SYSTEM_BUS_ADDRESS=unix:path=/tmp/sdbusplus-bus
    MYCALC_BUILD_DIR=/workspace/build-docker/benchmark-examples \
    MYCALC_BUS=system \
    python3 -m pytest -s benchmark-examples/test/benchmark_compare.py
  '
```

This image is also defined by [Dockerfile](Dockerfile), so the environment is
documented and reproducible rather than being a collection of undocumented
host packages.

## Build On The Host

If the required dependencies are already installed, build directly with
Meson:

```bash
meson setup build --wipe
meson compile -C build
```

Install executables and the D-Bus policy when needed:

```bash
sudo meson install -C build
sudo systemctl reload dbus
```

The detailed dependency instructions are in
[docs/INSTALL_SDBUSPLUS.md](docs/INSTALL_SDBUSPLUS.md).

Useful build options include:

```bash
meson setup build --wipe \
  -Dbenchmark-examples-opt-mode=O3 \
  -Dbasic-examples=enabled \
  -Dbenchmark-examples=enabled \
  -Dgenerated-via-yaml-examples=enabled
```

The project uses three build groups. Disable a whole group when a shorter
build is useful:

```bash
meson setup build --wipe -Dbasic-examples=disabled
meson setup build --wipe -Dbenchmark-examples=disabled
meson setup build --wipe -Dgenerated-via-yaml-examples=disabled
```

`benchmark-examples-opt-mode` is the only additional project option; it selects
`default`, `O2`, or `O3` optimization for benchmark binaries.

Build selected executables with:

```bash
meson compile -C build yaml_generated_crtp_caculator yaml_generated_sleep_crtp_caculator
meson compile -C build calculator-server calculator-aserver calculator-client
```

## D-Bus Usage

Start D-Bus before running service examples if it is not already running. The
common `busctl`, `gdbus`, and `dbus-send` commands are collected separately in
[docs/DBUS_USAGE.md](docs/DBUS_USAGE.md).

```bash
sudo systemctl start dbus
```

For calculator service layout, method calls, properties, signals, and service
startup examples, see [benchmark-examples/README.md](benchmark-examples/README.md).
