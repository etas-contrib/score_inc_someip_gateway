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
"""Pytest plugin that boots TWO QEMU/QNX guests in parallel via Linux bridge.

Re-uses ITF's public Python classes (`QemuProcess`, `QemuTarget`,
`load_configuration`, `pre_tests_phase`, `Target`, `determine_target_scope`)
without modifying ITF. The fixture name `target_init` is the documented seam
used by ITF's bundled plugins; pytest's "last definition wins" rule means this
override takes effect when the plugin is enabled via `-p dual_qemu_plugin`.

Bridge networking: both guests boot the same IFS image; the QNX
network_setup.sh in the image picks an IP from the NIC MAC
(:01 -> 192.168.87.2, :02 -> 192.168.87.3). The host bridge `virbr0` must be
set up before running tests via `sudo deployment/qemu/setup_bridge.sh`.
"""
import logging
import threading

import pytest

from score.itf.core.target import Target
from score.itf.plugins.core import determine_target_scope
from score.itf.plugins.qemu.checks import pre_tests_phase
from score.itf.plugins.qemu.config import load_configuration
from score.itf.plugins.qemu.qemu import Qemu
from score.itf.plugins.qemu.qemu_process import QemuProcess
from score.itf.plugins.qemu.qemu_target import QemuTarget


logger = logging.getLogger(__name__)


# ITF's stock `Qemu` omits `mac=` on each `-device virtio-net-pci`, so without
# this both parallel instances would inherit QEMU's default 52:54:00:12:34:56
# and clobber virbr0's MAC-learning table. The QNX network_setup.sh in the
# guest image also keys IP selection off the trailing byte of this MAC.
GUEST_A_MAC = "52:54:00:12:34:01"
GUEST_B_MAC = "52:54:00:12:34:02"


def pytest_addoption(parser):
    parser.addoption(
        "--qemu-config-a",
        required=True,
        help="Path to the JSON config for the FIRST QEMU instance.",
    )
    parser.addoption(
        "--qemu-config-b",
        required=True,
        help="Path to the JSON config for the SECOND QEMU instance.",
    )
    parser.addoption(
        "--qemu-image-a",
        required=True,
        help="Path to the QNX IFS image for guest A (bridge IP 192.168.87.2).",
    )
    parser.addoption(
        "--qemu-image-b",
        required=True,
        help="Path to the QNX IFS image for guest B (bridge IP 192.168.87.3).",
    )


class DualQemuTarget(Target):
    """Holds two QemuTargets. Exposes them as `.primary` and `.secondary`.

    Forwards the standard `Target` interface to `.primary` so any test that
    just uses the conventional ``target.execute(...)`` keeps working. Tests
    that need both QEMUs gate themselves with capability ``dual_qemu``.
    """

    def __init__(self, primary: QemuTarget, secondary: QemuTarget):
        super().__init__(capabilities={"ssh", "sftp", "dual_qemu"})
        self.primary = primary
        self.secondary = secondary

    def execute(self, command):
        return self.primary.execute(command)

    def execute_async(self, *args, **kwargs):
        return self.primary.execute_async(*args, **kwargs)

    def upload(self, local_path, remote_path):
        return self.primary.upload(local_path, remote_path)

    def download(self, remote_path, local_path):
        return self.primary.download(remote_path, local_path)

    def restart(self):
        self.primary.restart()
        self.secondary.restart()


def _stop_if_running(name: str, qproc) -> None:
    """Idempotent shutdown for a single QemuProcess.

    Skips processes that never started or already exited; swallows exceptions
    so that the second guest's stop call still runs even if the first raised.
    """
    if qproc is None:
        logger.info(f"{name}: never started, nothing to stop.")
        return
    try:
        sub = getattr(qproc._qemu, "_subprocess", None)
        if sub is None or sub.poll() is not None:
            logger.info(f"{name}: already exited, skipping stop.")
            return
        logger.info(f"{name}: stopping...")
        qproc.stop()
        logger.info(f"{name}: stopped.")
    except Exception as exc:
        logger.warning(f"{name}: stop raised {exc!r}, ignoring.")


class _MacAwareQemu(Qemu):
    """Qemu subclass that stamps a unique MAC onto every virtio-net-pci NIC.

    The override target is name-mangled: ``Qemu.__network_devices_args``
    resolves to ``_Qemu__network_devices_args``, which is what
    ``__build_qemu_command`` actually calls. We redefine that exact attribute
    so the parent's command builder picks up our version.
    """

    def __init__(self, *args, mac_address: str, **kwargs):
        super().__init__(*args, **kwargs)
        self._mac_address = mac_address

    def _Qemu__network_devices_args(self):
        args = super()._Qemu__network_devices_args()
        for i, token in enumerate(args):
            if token.startswith("virtio-net-pci,netdev=t"):
                args[i] = f"{token},mac={self._mac_address}"
        return args


def _build_qemu_process(image_path, cfg, mac_address: str) -> QemuProcess:
    proc = QemuProcess(
        image_path,
        cfg.qemu_ram_size,
        cfg.qemu_num_cores,
        network_adapters=[n.name for n in cfg.networks],
        port_forwarding=cfg.port_forwarding,
    )
    proc._qemu = _MacAwareQemu(
        image_path,
        cfg.qemu_ram_size,
        cfg.qemu_num_cores,
        network_adapters=[n.name for n in cfg.networks],
        port_forwarding=cfg.port_forwarding,
        mac_address=mac_address,
    )
    return proc


@pytest.fixture(scope=determine_target_scope)
def target_init(request):
    cfg_a = load_configuration(request.config.getoption("--qemu-config-a"))
    cfg_b = load_configuration(request.config.getoption("--qemu-config-b"))
    image_a = request.config.getoption("--qemu-image-a")
    image_b = request.config.getoption("--qemu-image-b")

    proc_a = _build_qemu_process(image_a, cfg_a, mac_address=GUEST_A_MAC)
    proc_b = _build_qemu_process(image_b, cfg_b, mac_address=GUEST_B_MAC)

    logger.info("Starting QEMU-A and QEMU-B in parallel...")
    t_a = threading.Thread(target=proc_a.start, name="qemu-a-start", daemon=True)
    t_b = threading.Thread(target=proc_b.start, name="qemu-b-start", daemon=True)
    t_a.start()
    t_b.start()
    t_a.join()
    t_b.join()
    logger.info("Both QEMU processes started.")

    target_a = QemuTarget(proc_a, cfg_a)
    target_b = QemuTarget(proc_b, cfg_b)

    pre_tests_phase(target_a)
    pre_tests_phase(target_b)

    try:
        yield DualQemuTarget(target_a, target_b)
    finally:
        logger.info("Tearing down dual QEMU targets sequentially.")
        _stop_if_running("qemu-a", proc_a)
        _stop_if_running("qemu-b", proc_b)
