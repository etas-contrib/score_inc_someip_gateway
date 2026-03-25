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
// Common serializer plugin interface and message type shared by all examples.
#ifndef SERIALIZER_PLUGIN_COMMON_HPP
#define SERIALIZER_PLUGIN_COMMON_HPP

#include <cstdint>
#include <cstring>
#include <string>

namespace plugin {

/// A trivial message used across all examples.
struct Message {
    uint32_t    id{0};
    char        payload[64]{};  // fixed-size for C-ABI safety

    /// Helper: set payload from std::string (truncates).
    void set_payload(const std::string& s) {
        std::strncpy(payload, s.c_str(), sizeof(payload) - 1);
        payload[sizeof(payload) - 1] = '\0';
    }
    std::string get_payload() const { return std::string(payload); }
};

/// Serialized wire format — fixed-size for simplicity.
struct WireBuffer {
    uint8_t  data[128]{};
    uint32_t length{0};
};

/// Abstract serializer interface (C++ side).
class ISerializerPlugin {
public:
    virtual ~ISerializerPlugin() = default;

    /// Serialize msg into buf.  Returns true on success.
    virtual bool serialize(const Message& msg, WireBuffer& buf) const = 0;

    /// Deserialize buf into msg.  Returns true on success.
    virtual bool deserialize(const WireBuffer& buf, Message& msg) const = 0;

    /// Human-readable name for diagnostics.
    virtual const char* name() const = 0;
};

}  // namespace plugin

#endif  // SERIALIZER_PLUGIN_COMMON_HPP
