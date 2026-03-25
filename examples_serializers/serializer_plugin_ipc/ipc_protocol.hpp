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
// IPC wire protocol shared between host and serializer child process.
#ifndef SERIALIZER_IPC_PROTOCOL_HPP
#define SERIALIZER_IPC_PROTOCOL_HPP

#include <cstdint>

namespace ipc {

/// Commands sent from host → child.
enum class Command : uint8_t {
    SERIALIZE   = 1,
    DESERIALIZE = 2,
    QUIT        = 0xFF,
};

/// Fixed-size request/response exchanged over the pipe.
/// Using POD types only for safe binary transfer.
struct Request {
    Command  cmd;
    uint32_t msg_id;
    char     msg_payload[64];
    uint8_t  wire_data[128];
    uint32_t wire_length;
};

struct Response {
    int32_t  status;       // 0 = success, <0 = error
    uint32_t msg_id;
    char     msg_payload[64];
    uint8_t  wire_data[128];
    uint32_t wire_length;
};

}  // namespace ipc

#endif  // SERIALIZER_IPC_PROTOCOL_HPP
