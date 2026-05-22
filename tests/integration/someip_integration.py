# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************
"""SOME/IP-SD and event-notification tests across two QEMU/QNX guests."""

from __future__ import annotations

import logging
import os
import shlex
import signal
import subprocess
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator

import pytest

from tests.integration.someip_network_analyzer import (
    PcapSummary,
    SdSnapshot,
    ServiceId,
    SomeIpEvent,
    analyze,
)

logger = logging.getLogger(__name__)


# Topology and timing

PRIMARY_IP = "192.168.87.2"
SECONDARY_IP = "192.168.87.3"
HOST_NAMES = {PRIMARY_IP: "someipd", SECONDARY_IP: "sample_client"}

TCPDUMP_INTERFACE = "virbr0"
TCPDUMP_WARMUP_SECONDS = 2
GATEWAYD_TO_SOMEIPD_DELAY = 3
SOMEIPD_TO_CLIENT_DELAY = 2
SD_SETTLE_SECONDS = 5
EVENT_OBSERVATION_SECONDS = 10
NEGATIVE_OBSERVATION_SECONDS = 5


# Remote service descriptors


@dataclass(frozen=True)
class RemoteService:
    """A service launched on a QNX guest. log_path=None skips log capture."""

    name: str
    binary: str
    args: tuple[str, ...] = ()
    env: tuple[tuple[str, str], ...] = ()
    log_path: str | None = None


GATEWAYD = RemoteService(
    name="gatewayd",
    binary="/usr/bin/gatewayd",
    args=(
        "-config_file",
        "/etc/gatewayd/gatewayd_config.bin",
        "--service_instance_manifest",
        "/etc/gatewayd/mw_com_config.json",
    ),
)

SOMEIPD = RemoteService(
    name="someipd",
    binary="/usr/bin/someipd",
    args=(
        "-someipd_config",
        "/etc/someipd/someipd_config.json",
        "--service_instance_manifest",
        "/etc/someipd/mw_com_config.json",
    ),
    env=(("VSOMEIP_CONFIGURATION", "/etc/someipd/vsomeip.json"),),
    log_path="/tmp/someipd.log",
)

SAMPLE_CLIENT = RemoteService(
    name="sample_client",
    binary="/usr/bin/sample_client",
    env=(("VSOMEIP_CONFIGURATION", "/etc/sample_client/vsomeip.json"),),
    log_path="/tmp/sample_client.log",
)

IPC_BENCHMARKS = RemoteService(
    name="ipc_benchmarks",
    binary="/usr/bin/tests/ipc_benchmarks",
    args=(
        "--service_instance_manifest",
        "/etc/benchmarks/benchmark_mw_com_config.json",
        "--benchmark_min_time=0.001s",
        "--benchmark_repetitions=1",
    ),
)


# Expectations


@dataclass(frozen=True)
class EventExpectation:
    """Expected fields of a single SOME/IP notification from one host."""

    service_id: int
    event_id: int
    payload_size: int


EXPECTED_SD: dict[str, SdSnapshot] = {
    PRIMARY_IP: SdSnapshot(
        offers={ServiceId(0x1234, 0x5678)},
        finds={ServiceId(0x4321, 0x5678)},
        subscribes={ServiceId(0x4321, 0x5678, eventgroup=0x4465)},
    ),
    SECONDARY_IP: SdSnapshot(
        offers={ServiceId(0x4321, 0x5678)},
        finds={ServiceId(0x1234, 0x5678)},
        subscribes={ServiceId(0x1234, 0x5678, eventgroup=0x4465)},
    ),
}

EXPECTED_EVENTS: dict[str, EventExpectation] = {
    PRIMARY_IP: EventExpectation(service_id=0x1234, event_id=0x8778, payload_size=32),
    SECONDARY_IP: EventExpectation(service_id=0x4321, event_id=0x8778, payload_size=32),
}


# Output dir


def _outputs_dir() -> Path:
    """Bazel's undeclared outputs dir, falling back to /tmp for local runs."""
    out = os.environ.get("TEST_UNDECLARED_OUTPUTS_DIR")
    if out:
        path = Path(out)
        path.mkdir(parents=True, exist_ok=True)
        return path
    return Path("/tmp")


# Remote service control


def _start(qemu, svc: RemoteService):
    # ITF's execute_async wraps in `sh -lc "echo $$; cd / && <cmd>"` but
    # exposes no env / redirect; fold both into the binary_path slot and
    # exec so the binary inherits the PID that stop() will signal.
    parts = [f"{k}={shlex.quote(v)}" for k, v in svc.env]
    parts.append("exec")
    parts.append(shlex.quote(svc.binary))
    parts.extend(shlex.quote(a) for a in svc.args)
    if svc.log_path:
        parts.append(f">{shlex.quote(svc.log_path)} 2>&1")
    return qemu.execute_async(" ".join(parts))


def _fetch_log(qemu, svc: RemoteService, dest: Path) -> None:
    if not svc.log_path:
        return
    rc, output = qemu.execute(f"/proc/boot/cat {svc.log_path}")
    if rc != 0:
        logger.warning("Failed to fetch %s log: rc=%s", svc.name, rc)
        return
    dest.write_bytes(output)
    logger.info("%s log saved to %s", svc.name, dest)


# Host-side packet capture


def _start_tcpdump(pcap: Path, log_fh) -> subprocess.Popen:
    """Launch tcpdump on the bridge interface, writing pcap and stderr→log_fh."""
    proc = subprocess.Popen(
        [
            "tcpdump",
            "-i",
            TCPDUMP_INTERFACE,
            "-w",
            str(pcap),
            f"host {PRIMARY_IP} or host {SECONDARY_IP}",
        ],
        stdout=subprocess.DEVNULL,
        stderr=log_fh,
    )
    time.sleep(TCPDUMP_WARMUP_SECONDS)
    return proc


def _stop_tcpdump(proc: subprocess.Popen) -> None:
    """SIGINT tcpdump and wait for it to flush; force-kill on timeout."""
    proc.send_signal(signal.SIGINT)
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


# Per-test session


@dataclass
class Session:
    """Per-test scaffold: tcpdump + a list of started services. Call
    ``analyze()`` to stop the capture and parse the pcap."""

    pcap: Path
    outputs_dir: Path
    test_name: str
    _runs: list[tuple[object, RemoteService, object]] = field(default_factory=list)
    _tcpdump: subprocess.Popen | None = None

    def start(self, qemu, svc: RemoteService) -> None:
        handle = _start(qemu, svc)
        self._runs.append((qemu, svc, handle))

    def analyze(self) -> PcapSummary:
        self._stop_capture()
        return analyze(self.pcap)

    def _stop_capture(self) -> None:
        if self._tcpdump is None:
            return
        _stop_tcpdump(self._tcpdump)
        self._tcpdump = None


def _teardown_session(sess: Session) -> None:
    """Stop capture, fetch logs, stop services. Per-step failures are logged."""
    try:
        sess._stop_capture()
    except Exception as exc:
        logger.warning("Failed to stop tcpdump: %r", exc)
    for qemu, svc, _ in sess._runs:
        try:
            _fetch_log(qemu, svc, sess.outputs_dir / f"{sess.test_name}_{svc.name}.log")
        except Exception as exc:
            logger.warning("Failed to fetch log for %s: %r", svc.name, exc)
    for _, svc, handle in sess._runs:
        try:
            handle.stop()
        except Exception as exc:
            logger.warning("Failed to stop %s: %r", svc.name, exc)


@pytest.fixture
def session(request) -> Iterator[Session]:
    """Set up tcpdump + a Session, tear them down after the test."""
    out = _outputs_dir()
    name = request.node.name
    sess = Session(
        pcap=out / f"{name}_tcpdump.pcap",
        outputs_dir=out,
        test_name=name,
    )
    log_path = out / f"{name}_tcpdump.log"
    with open(log_path, "w") as log_fh:
        proc = _start_tcpdump(sess.pcap, log_fh)
        assert proc.poll() is None, f"tcpdump exited early: {log_path.read_text()}"
        sess._tcpdump = proc
        try:
            yield sess
        finally:
            _teardown_session(sess)


# Assertion helpers


def _assert_sd_matches(ip: str, expected: SdSnapshot, actual: SdSnapshot) -> None:
    name = HOST_NAMES.get(ip, ip)
    logger.info("CHECKING: %s (%s)", name, ip)
    for label in ("offers", "finds", "subscribes"):
        exp = getattr(expected, label)
        act = getattr(actual, label)
        logger.info("  %s:", label.upper())
        logger.info("    expected: %s", sorted(map(str, exp)))
        logger.info("    actual:   %s", sorted(map(str, act)))
        assert exp == act, f"[{name}] {label} mismatch"


def _assert_sd_partial(ip: str, snap: SdSnapshot) -> None:
    """One host alone: offers and finds present, no completed subscriptions."""
    name = HOST_NAMES.get(ip, ip)
    assert snap.offers, f"[{name}] expected offers, got none"
    assert snap.finds, f"[{name}] expected finds, got none"
    assert not snap.subscribes, f"[{name}] expected no subscribes, got {snap.subscribes}"


def _assert_event_matches(
    ip: str, expected: EventExpectation, events: list[SomeIpEvent]
) -> None:
    name = HOST_NAMES.get(ip, ip)
    logger.info("CHECKING: %s (%s)", name, ip)
    matching = [e for e in events if e.service_id == expected.service_id]
    assert matching, (
        f"[{name}] no events for service 0x{expected.service_id:04x}"
    )
    e = matching[0]
    assert e.method_id == expected.event_id, (
        f"[{name}] event_id mismatch: "
        f"expected 0x{expected.event_id:04x}, got 0x{e.method_id:04x}"
    )
    assert e.payload_size == expected.payload_size, (
        f"[{name}] payload_size mismatch: "
        f"expected {expected.payload_size}, got {e.payload_size}"
    )


# Launch sequences. The SOME/IP stack needs services to come up in order;
# inter-service sleeps stay at the test level for visibility.


def _start_primary_stack(session: Session, qemu) -> None:
    session.start(qemu, GATEWAYD)
    time.sleep(GATEWAYD_TO_SOMEIPD_DELAY)
    session.start(qemu, SOMEIPD)
    time.sleep(SOMEIPD_TO_CLIENT_DELAY)


def _start_secondary_stack(session: Session, qemu) -> None:
    session.start(qemu, SAMPLE_CLIENT)


# Tests


class TestSomeIPSD:
    """SOME/IP-SD: offers (advertised), finds (sought), subscribes (paired)."""

    def test_someip_sd_offers_finds_subscriptions(self, target, session: Session) -> None:
        """Both hosts up — verify offers, finds, and subscriptions match."""
        _start_primary_stack(session, target.primary)
        _start_secondary_stack(session, target.secondary)
        time.sleep(SD_SETTLE_SECONDS)

        summary = session.analyze()
        for ip, expected in EXPECTED_SD.items():
            assert ip in summary.sd, f"no SD data for {HOST_NAMES[ip]} ({ip})"
            _assert_sd_matches(ip, expected, summary.sd[ip])

    def test_negative_both_qemus_no_services(self, target, session: Session) -> None:
        """No services running — no SOME/IP-SD traffic should appear."""
        time.sleep(NEGATIVE_OBSERVATION_SECONDS)
        summary = session.analyze()
        assert not summary.sd, f"expected silent network, found: {list(summary.sd)}"

    def test_negative_only_qemu1_with_services(self, target, session: Session) -> None:
        """Only the primary stack — offers + finds, but no subscriptions complete."""
        _start_primary_stack(session, target.primary)
        time.sleep(SD_SETTLE_SECONDS)

        summary = session.analyze()
        assert SECONDARY_IP not in summary.sd, "secondary should be silent"
        assert PRIMARY_IP in summary.sd, "primary should have SD traffic"
        _assert_sd_partial(PRIMARY_IP, summary.sd[PRIMARY_IP])

    def test_negative_only_qemu2_with_services(self, target, session: Session) -> None:
        """Only the secondary stack — offers + finds, but no subscriptions complete."""
        _start_secondary_stack(session, target.secondary)
        time.sleep(SD_SETTLE_SECONDS)

        summary = session.analyze()
        assert PRIMARY_IP not in summary.sd, "primary should be silent"
        assert SECONDARY_IP in summary.sd, "secondary should have SD traffic"
        _assert_sd_partial(SECONDARY_IP, summary.sd[SECONDARY_IP])

    def test_someip_event_data_transfer(self, target, session: Session) -> None:
        """Run ipc_benchmarks and verify SOME/IP notifications flow both ways."""
        _start_primary_stack(session, target.primary)
        _start_secondary_stack(session, target.secondary)
        time.sleep(SD_SETTLE_SECONDS)
        session.start(target.primary, IPC_BENCHMARKS)
        time.sleep(EVENT_OBSERVATION_SECONDS)

        summary: PcapSummary = analyze(session.pcap)
        for ip, expected in EXPECTED_EVENTS.items():
            _assert_event_matches(ip, expected, summary.events.get(ip, []))
