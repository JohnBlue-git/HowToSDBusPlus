#!/usr/bin/env python3

from __future__ import annotations

import os
import shutil
import signal
import subprocess
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

import pytest

SERVICE = "xyz.openbmc_project.Calculator"
INTERFACE = "xyz.openbmc_project.Calculator"

TARGETS: Dict[str, str] = {
    "non_async_caculator": "/xyz/openbmc_project/calculator",
    "boost_asio_caculator": "/xyz/openbmc_project/calculator",
    "boost_asio_crtp_caculator": "/xyz/openbmc_project/calculator/decimal",
    "sdbusplus_async_caculator": "/xyz/openbmc_project/calculator",
    "sdbusplus_async_crtp_caculator": "/xyz/openbmc_project/calculator/decimal",
    "sdbusplus_async_sleep_crtp_caculator": "/xyz/openbmc_project/calculator/decimal",
    "yaml_generated_crtp_caculator": "/xyz/openbmc_project/calculator/decimal",
    "yaml_generated_sleep_crtp_caculator": "/xyz/openbmc_project/calculator/decimal",
}


@dataclass
class BenchResult:
    name: str
    throughput_ops: float
    avg_latency_ms: float
    peak_rss_kb: int
    peak_hwm_kb: int


@dataclass
class BenchConfig:
    build_dir: Path
    iterations: int
    warmup: int
    startup_timeout: float
    bus_mode: str
    use_sudo: bool
    target_names: List[str]


@dataclass
class ServiceContext:
    name: str
    object_path: str
    binary: Path
    proc: subprocess.Popen


def run_cmd(cmd: List[str], *, timeout: Optional[float] = None, quiet: bool = True) -> subprocess.CompletedProcess:
    stdout = subprocess.DEVNULL if quiet else None
    stderr = subprocess.PIPE if quiet else None
    return subprocess.run(cmd, stdout=stdout, stderr=stderr, text=True, timeout=timeout, check=False)


def read_memory_kb(pid: int) -> tuple[int, int]:
    status_path = Path(f"/proc/{pid}/status")
    if not status_path.exists():
        return (0, 0)

    rss_kb = 0
    hwm_kb = 0
    for line in status_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        if line.startswith("VmRSS:"):
            parts = line.split()
            if len(parts) >= 2:
                rss_kb = int(parts[1])
        elif line.startswith("VmHWM:"):
            parts = line.split()
            if len(parts) >= 2:
                hwm_kb = int(parts[1])
    return (rss_kb, hwm_kb)


def wait_for_service(bus_mode: str, service: str, object_path: str, timeout_sec: float) -> bool:
    deadline = time.monotonic() + timeout_sec
    cmd = ["busctl", f"--{bus_mode}", "introspect", service, object_path]

    while time.monotonic() < deadline:
        cp = run_cmd(cmd, timeout=2)
        if cp.returncode == 0:
            return True
        time.sleep(0.1)
    return False


def call_multiply(bus_mode: str, object_path: str, x: int, y: int) -> bool:
    cmd = [
        "busctl",
        f"--{bus_mode}",
        "call",
        SERVICE,
        object_path,
        INTERFACE,
        "Multiply",
        "xx",
        str(x),
        str(y),
    ]
    cp = run_cmd(cmd, timeout=5)
    return cp.returncode == 0


def start_service(binary: Path, use_sudo: bool) -> subprocess.Popen:
    cmd: List[str] = []
    if use_sudo:
        cmd.extend(["sudo", "-n"])
    cmd.append(str(binary))

    return subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
        start_new_session=True,
    )


def stop_service(proc: subprocess.Popen) -> None:
    if proc.poll() is not None:
        return
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        return

    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            return


def benchmark_running_service(
    proc: subprocess.Popen,
    object_path: str,
    *,
    iterations: int,
    warmup: int,
    bus_mode: str,
) -> BenchResult:
    peak_rss_kb = 0
    peak_hwm_kb = 0
    stop_event = threading.Event()

    def monitor() -> None:
        nonlocal peak_rss_kb, peak_hwm_kb
        while not stop_event.is_set() and proc.poll() is None:
            rss_kb, hwm_kb = read_memory_kb(proc.pid)
            peak_rss_kb = max(peak_rss_kb, rss_kb)
            peak_hwm_kb = max(peak_hwm_kb, hwm_kb)
            time.sleep(0.02)

    monitor_thread = threading.Thread(target=monitor, daemon=True)
    monitor_thread.start()

    for i in range(warmup):
        ok = call_multiply(bus_mode, object_path, (i % 97) + 3, (i % 89) + 2)
        if not ok:
            stop_event.set()
            monitor_thread.join(timeout=1)
            raise RuntimeError("warmup failed")

    start = time.perf_counter()
    for i in range(iterations):
        ok = call_multiply(bus_mode, object_path, (i % 193) + 11, (i % 181) + 7)
        if not ok:
            stop_event.set()
            monitor_thread.join(timeout=1)
            raise RuntimeError(f"benchmark call failed at iteration {i}")
    elapsed = time.perf_counter() - start

    stop_event.set()
    monitor_thread.join(timeout=1)

    rss_kb, hwm_kb = read_memory_kb(proc.pid)
    peak_rss_kb = max(peak_rss_kb, rss_kb)
    peak_hwm_kb = max(peak_hwm_kb, hwm_kb)

    throughput = iterations / elapsed if elapsed > 0 else 0.0
    avg_latency_ms = (elapsed * 1000.0 / iterations) if iterations > 0 else 0.0

    return BenchResult(
        name="",
        throughput_ops=throughput,
        avg_latency_ms=avg_latency_ms,
        peak_rss_kb=peak_rss_kb,
        peak_hwm_kb=peak_hwm_kb,
    )


def print_results(results: List[BenchResult]) -> None:
    print("\nComprehensive comparison (handling speed + memory usage)")
    print("=" * 76)
    print(
        f"{'Executable':36} {'Ops/s':>10} {'Avg ms/op':>12} {'Peak RSS(KiB)':>14} {'Peak HWM(KiB)':>14}"
    )
    print("-" * 76)

    for result in results:
        print(
            f"{result.name:36} {result.throughput_ops:10.2f} {result.avg_latency_ms:12.4f} {result.peak_rss_kb:14d} {result.peak_hwm_kb:14d}"
        )

    print("\nRanking")
    best_speed = sorted(results, key=lambda x: x.throughput_ops, reverse=True)
    best_mem = sorted(results, key=lambda x: x.peak_hwm_kb)
    print("- Speed (high to low): " + " > ".join(x.name for x in best_speed))
    print("- Memory (low to high by Peak HWM): " + " < ".join(x.name for x in best_mem))


def load_config() -> BenchConfig:
    build_dir = Path(os.getenv("MYCALC_BUILD_DIR", "build/benchmark-examples"))
    iterations = int(os.getenv("MYCALC_ITERATIONS", "300"))
    warmup = int(os.getenv("MYCALC_WARMUP", "40"))
    startup_timeout = float(os.getenv("MYCALC_STARTUP_TIMEOUT", "8.0"))
    bus_mode = os.getenv("MYCALC_BUS", "system")
    use_sudo = os.getenv("MYCALC_USE_SUDO", "0") in {"1", "true", "TRUE", "yes", "YES"}
    only_raw = os.getenv("MYCALC_ONLY", "").strip()

    if bus_mode not in {"system", "user"}:
        raise ValueError("MYCALC_BUS must be 'system' or 'user'")

    if only_raw:
        requested = [item.strip() for item in only_raw.split(",") if item.strip()]
        unknown = [name for name in requested if name not in TARGETS]
        if unknown:
            raise ValueError("MYCALC_ONLY contains unknown executables: " + ", ".join(unknown))
        target_names = requested
    else:
        target_names = list(TARGETS.keys())

    return BenchConfig(
        build_dir=build_dir,
        iterations=iterations,
        warmup=warmup,
        startup_timeout=startup_timeout,
        bus_mode=bus_mode,
        use_sudo=use_sudo,
        target_names=target_names,
    )


CONFIG = load_config()
if os.getenv("PYTEST_XDIST_WORKER"):
    pytest.skip("benchmark_compare.py must run sequentially (no pytest-xdist)", allow_module_level=True)

pytestmark = pytest.mark.skipif(shutil.which("busctl") is None, reason="busctl not found")


def _available_targets() -> List[str]:
    if not CONFIG.build_dir.exists():
        return []

    names: List[str] = []
    for target_name in CONFIG.target_names:
        if (CONFIG.build_dir / target_name).exists():
            names.append(target_name)
    return names


AVAILABLE_TARGETS = _available_targets()
if not AVAILABLE_TARGETS:
    pytest.skip(
        f"no benchmark targets found under {CONFIG.build_dir}; build first",
        allow_module_level=True,
    )


@pytest.fixture(scope="session")
def bench_results() -> List[BenchResult]:
    results: List[BenchResult] = []
    yield results
    if results:
        print_results(results)


@pytest.fixture(params=AVAILABLE_TARGETS)
def target_name(request: pytest.FixtureRequest) -> str:
    return str(request.param)


@pytest.fixture
def service_ctx(target_name: str) -> ServiceContext:
    binary = CONFIG.build_dir / target_name
    object_path = TARGETS[target_name]

    proc = start_service(binary, CONFIG.use_sudo)
    if proc.poll() is not None:
        stderr = (proc.stderr.read() or "").strip() if proc.stderr else ""
        raise RuntimeError(f"failed to start {binary.name}: {stderr}")

    ready = wait_for_service(CONFIG.bus_mode, SERVICE, object_path, CONFIG.startup_timeout)
    if not ready:
        stop_service(proc)
        raise RuntimeError(f"service not ready for {binary.name} on {object_path}")

    ctx = ServiceContext(
        name=target_name,
        object_path=object_path,
        binary=binary,
        proc=proc,
    )
    try:
        yield ctx
    finally:
        stop_service(proc)


def test_multiply_speed_and_memory(service_ctx: ServiceContext, bench_results: List[BenchResult]) -> None:
    result = benchmark_running_service(
        service_ctx.proc,
        service_ctx.object_path,
        iterations=CONFIG.iterations,
        warmup=CONFIG.warmup,
        bus_mode=CONFIG.bus_mode,
    )
    result.name = service_ctx.name
    bench_results.append(result)

    assert result.throughput_ops > 0
    assert result.peak_hwm_kb >= 0
