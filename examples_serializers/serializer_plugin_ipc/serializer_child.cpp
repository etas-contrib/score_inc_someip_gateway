/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
// SPDX-License-Identifier: Apache-2.0
// Option 3 — Serializer child process (out-of-process plugin).
//
// Reads Request structs from stdin, writes Response structs to stdout.
// This process can run at a lower ASIL than the host, providing
// hardware-enforced memory isolation.
//
// Build : bazel build //examples/serializer_plugin_ipc:serializer_child
// (Not run directly; spawned by the host.)

#include "ipc_protocol.hpp"

#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace {

bool do_serialize(const ipc::Request& req, ipc::Response& resp) {
    // Big-endian encoding.
    resp.wire_data[0] = static_cast<uint8_t>((req.msg_id >> 24) & 0xFF);
    resp.wire_data[1] = static_cast<uint8_t>((req.msg_id >> 16) & 0xFF);
    resp.wire_data[2] = static_cast<uint8_t>((req.msg_id >>  8) & 0xFF);
    resp.wire_data[3] = static_cast<uint8_t>((req.msg_id >>  0) & 0xFF);
    std::memcpy(&resp.wire_data[4], req.msg_payload, sizeof(req.msg_payload));
    resp.wire_length = 4 + static_cast<uint32_t>(sizeof(req.msg_payload));
    resp.status = 0;
    return true;
}

bool do_deserialize(const ipc::Request& req, ipc::Response& resp) {
    if (req.wire_length < 4 + sizeof(resp.msg_payload)) {
        resp.status = -1;
        return false;
    }
    resp.msg_id = (static_cast<uint32_t>(req.wire_data[0]) << 24)
                | (static_cast<uint32_t>(req.wire_data[1]) << 16)
                | (static_cast<uint32_t>(req.wire_data[2]) <<  8)
                | (static_cast<uint32_t>(req.wire_data[3]) <<  0);
    std::memcpy(resp.msg_payload, &req.wire_data[4], sizeof(resp.msg_payload));
    resp.status = 0;
    return true;
}

}  // namespace

int main() {
    ipc::Request  req{};
    ipc::Response resp{};

    while (true) {
        ssize_t n = ::read(STDIN_FILENO, &req, sizeof(req));
        if (n <= 0) break;  // pipe closed or error
        if (static_cast<size_t>(n) != sizeof(req)) break;

        std::memset(&resp, 0, sizeof(resp));

        if (req.cmd == ipc::Command::QUIT) {
            break;
        } else if (req.cmd == ipc::Command::SERIALIZE) {
            do_serialize(req, resp);
        } else if (req.cmd == ipc::Command::DESERIALIZE) {
            do_deserialize(req, resp);
        } else {
            resp.status = -2;  // unknown command
        }

        ssize_t w = ::write(STDOUT_FILENO, &resp, sizeof(resp));
        if (w <= 0) break;
    }
    return 0;
}
