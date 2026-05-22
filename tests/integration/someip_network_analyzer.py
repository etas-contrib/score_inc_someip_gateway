#!/usr/bin/env python3
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
"""Single-pass scapy parser for SOME/IP and SOME/IP-SD traffic in a pcap."""

from __future__ import annotations

import logging
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator

from scapy.all import IP, UDP, conf, rdpcap

logger = logging.getLogger(__name__)
conf.verb = 0


# Protocol constants

SD_PORT = 30490
SD_SERVICE_ID = 0xFFFF
SD_METHOD_ID = 0x8100
SOMEIP_NOTIFICATION_TYPE = 0x02

_ENTRY_TYPE_FIND = 0x00
_ENTRY_TYPE_OFFER = 0x01
_ENTRY_TYPE_SUBSCRIBE = 0x06
_ENTRY_TYPE_STOP_SUBSCRIBE_ACK = 0x07

_SOMEIP_HEADER_LEN = 16
_SD_HEADER_LEN = 8
_SD_ENTRY_LEN = 16

# Length in SOME/IP header covers request_id (4B) + proto/iface/type/retcode (4B) + payload.
_SOMEIP_FIXED_TAIL_LEN = 8


@dataclass(frozen=True)
class ServiceId:
    service: int
    instance: int
    eventgroup: int | None = None

    def __str__(self) -> str:
        base = f"Service 0x{self.service:04x}.0x{self.instance:04x}"
        if self.eventgroup is None:
            return base
        return f"{base}, eventgroup 0x{self.eventgroup:04x}"


@dataclass
class SdSnapshot:
    offers: set[ServiceId] = field(default_factory=set)
    finds: set[ServiceId] = field(default_factory=set)
    subscribes: set[ServiceId] = field(default_factory=set)

    @property
    def is_empty(self) -> bool:
        return not (self.offers or self.finds or self.subscribes)


@dataclass(frozen=True)
class SomeIpEvent:
    service_id: int
    method_id: int
    payload_size: int


@dataclass
class PcapSummary:
    sd: dict[str, SdSnapshot] = field(default_factory=dict)
    events: dict[str, list[SomeIpEvent]] = field(default_factory=dict)


def _iter_sd_entries(payload: bytes) -> Iterator[tuple[int, ServiceId]]:
    if len(payload) < _SOMEIP_HEADER_LEN + _SD_HEADER_LEN:
        return
    service_id, method_id = struct.unpack_from("!HH", payload, 0)
    if service_id != SD_SERVICE_ID or method_id != SD_METHOD_ID:
        return

    (entries_length,) = struct.unpack_from("!I", payload, _SOMEIP_HEADER_LEN + 4)
    start = _SOMEIP_HEADER_LEN + _SD_HEADER_LEN
    end = min(start + entries_length, len(payload))

    offset = start
    while offset + _SD_ENTRY_LEN <= end:
        entry_type = payload[offset]
        svc, inst = struct.unpack_from("!HH", payload, offset + 4)

        eventgroup: int | None = None
        if entry_type in (_ENTRY_TYPE_SUBSCRIBE, _ENTRY_TYPE_STOP_SUBSCRIBE_ACK):
            (eg,) = struct.unpack_from("!H", payload, offset + 14)
            eventgroup = eg if eg not in (0, 0xFFFF) else None

        yield entry_type, ServiceId(svc, inst, eventgroup)
        offset += _SD_ENTRY_LEN


def _decode_notification(payload: bytes) -> SomeIpEvent | None:
    """Return a SomeIpEvent if payload is a non-SD SOME/IP notification."""
    if len(payload) < _SOMEIP_HEADER_LEN:
        return None
    service_id, method_id, length = struct.unpack_from("!HHI", payload, 0)
    if service_id == SD_SERVICE_ID and method_id == SD_METHOD_ID:
        return None
    msg_type = payload[14]
    if msg_type != SOMEIP_NOTIFICATION_TYPE:
        return None
    return SomeIpEvent(
        service_id=service_id,
        method_id=method_id,
        payload_size=max(length - _SOMEIP_FIXED_TAIL_LEN, 0),
    )


# Public API


_PCAP_GLOBAL_HEADER_LEN = 24


def analyze(pcap_file: str | Path) -> PcapSummary:
    """Walk ``pcap_file`` once and return all SOME/IP findings, keyed by source IP.

    Missing or header-only pcaps yield an empty summary instead of raising.
    """
    path = Path(pcap_file)
    if not path.exists() or path.stat().st_size <= _PCAP_GLOBAL_HEADER_LEN:
        return PcapSummary()

    sd_by_host: dict[str, SdSnapshot] = defaultdict(SdSnapshot)
    events_by_host: dict[str, list[SomeIpEvent]] = defaultdict(list)

    for pkt in rdpcap(str(path)):
        if not (pkt.haslayer(IP) and pkt.haslayer(UDP)):
            continue
        src = pkt[IP].src
        payload = bytes(pkt[UDP].payload)

        if pkt[UDP].dport == SD_PORT or pkt[UDP].sport == SD_PORT:
            snap = sd_by_host[src]
            for entry_type, sid in _iter_sd_entries(payload):
                if entry_type == _ENTRY_TYPE_OFFER:
                    snap.offers.add(ServiceId(sid.service, sid.instance))
                elif entry_type == _ENTRY_TYPE_FIND:
                    snap.finds.add(ServiceId(sid.service, sid.instance))
                elif entry_type == _ENTRY_TYPE_SUBSCRIBE:
                    snap.subscribes.add(sid)
            continue

        event = _decode_notification(payload)
        if event is not None:
            events_by_host[src].append(event)

    return PcapSummary(
        sd={ip: snap for ip, snap in sd_by_host.items() if not snap.is_empty},
        events=dict(events_by_host),
    )


# CLI

_DEFAULT_HOSTS = {"192.168.87.2": "someipd", "192.168.87.3": "sample_client"}


def _log_snapshot(name: str, ip: str, snap: SdSnapshot) -> None:
    logger.info("SUMMARY: %s (%s)", name, ip)
    for label, items in (
        ("OFFERS", snap.offers),
        ("FINDS", snap.finds),
        ("SUBSCRIBED SUCCESSFULLY TO", snap.subscribes),
    ):
        if not items:
            logger.info("%s: None detected", label)
            continue
        logger.info("%s:", label)
        for sid in sorted(items, key=lambda s: (s.service, s.instance, s.eventgroup or 0)):
            logger.info("    %s", sid)


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(message)s")

    if len(sys.argv) < 2:
        logger.error("Usage: python3 someip_network_analyzer.py <pcap_file>")
        return 1

    path = Path(sys.argv[1])
    if not path.exists():
        logger.error("pcap file not found: %s", path)
        return 1

    logger.info("Analyzing %s for SOME/IP-SD traffic...", path)
    summary = analyze(path)
    for ip, name in _DEFAULT_HOSTS.items():
        snap = summary.sd.get(ip)
        if snap is None:
            logger.info("No data found for %s (%s)", name, ip)
        else:
            _log_snapshot(name, ip, snap)
    return 0


if __name__ == "__main__":
    sys.exit(main())
