#!/usr/bin/env bash
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
# Ensures tests run in an isolated network namespace with loopback
# multicast routing for SOME/IP-SD.
#
# Three modes (tried in order):
#   1. linux-sandbox namespace (preferred) — when Bazel's linux-sandbox
#      successfully creates a network namespace (--sandbox_default_allow_network
#      =false + requires-fakeroot tag), only the loopback interface exists.
#      We just add the multicast route.
#   2. unshare fallback — when linux-sandbox cannot create namespaces
#      (e.g. AppArmor blocks CLONE_NEWUSER in devcontainers without the
#      unblock action), we detect the host network (non-lo interfaces
#      present) and fall back to unshare for isolation.
#   3. passthrough — if both fail, run the test directly and let
#      conftest.py's require_tc8_environment fixture skip gracefully
#      with an actionable message.

# Detect whether linux-sandbox created an isolated network namespace.
# In an isolated namespace only "lo" exists; on the host network
# additional interfaces (eth0, etc.) are visible.
has_non_lo_interface() {
    ip -brief link show 2>/dev/null | grep -qv '^lo '
}

if ! has_non_lo_interface; then
    # Inside linux-sandbox namespace — just add the multicast route.
    ip route add 224.0.0.0/4 dev lo 2>/dev/null || true
    exec "$@"
fi

# Fallback: linux-sandbox did not isolate the network.
# Try unshare; on success it replaces this process.
unshare --user --net --map-root-user -- bash -c '
    ip link set lo up
    ip route add 224.0.0.0/4 dev lo
    exec "$@"
' -- "$@" && exit 0

# Both methods failed — run directly and let conftest.py skip gracefully.
exec "$@"
