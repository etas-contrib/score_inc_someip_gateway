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
// Option 1 — Static compile-time registration.
//
// All serializer variants are compiled into the binary.  A factory function
// selects the active one based on a string key (simulating a config file).
//
// Contract:
//   Input  : a Message struct + a serializer name string
//   Output : serialized WireBuffer, then deserialized Message (round-trip)
//   Failure: unknown serializer name → error message, exit 1
//   Safety : no dynamic loading, no IPC, deterministic startup
//
// Build : bazel build //examples/serializer_plugin_static
// Run   : bazel-bin/examples/serializer_plugin_static/static_demo big_endian
//         bazel-bin/examples/serializer_plugin_static/static_demo little_endian

#include "examples_serializers/serializer_plugin_common/common.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

// ---------- Concrete serializer A: Big-Endian style ----------

namespace {

class BigEndianSerializer final : public plugin::ISerializerPlugin {
public:
    bool serialize(const plugin::Message& msg, plugin::WireBuffer& buf) const override {
        // Store id in big-endian order, then payload bytes.
        buf.data[0] = static_cast<uint8_t>((msg.id >> 24) & 0xFF);
        buf.data[1] = static_cast<uint8_t>((msg.id >> 16) & 0xFF);
        buf.data[2] = static_cast<uint8_t>((msg.id >>  8) & 0xFF);
        buf.data[3] = static_cast<uint8_t>((msg.id >>  0) & 0xFF);
        std::memcpy(&buf.data[4], msg.payload, sizeof(msg.payload));
        buf.length = 4 + sizeof(msg.payload);
        return true;
    }
    bool deserialize(const plugin::WireBuffer& buf, plugin::Message& msg) const override {
        if (buf.length < 4 + sizeof(msg.payload)) return false;
        msg.id = (static_cast<uint32_t>(buf.data[0]) << 24)
               | (static_cast<uint32_t>(buf.data[1]) << 16)
               | (static_cast<uint32_t>(buf.data[2]) <<  8)
               | (static_cast<uint32_t>(buf.data[3]) <<  0);
        std::memcpy(msg.payload, &buf.data[4], sizeof(msg.payload));
        return true;
    }
    const char* name() const override { return "BigEndianSerializer"; }
};

// ---------- Concrete serializer B: Little-Endian style ----------

class LittleEndianSerializer final : public plugin::ISerializerPlugin {
public:
    bool serialize(const plugin::Message& msg, plugin::WireBuffer& buf) const override {
        buf.data[0] = static_cast<uint8_t>((msg.id >>  0) & 0xFF);
        buf.data[1] = static_cast<uint8_t>((msg.id >>  8) & 0xFF);
        buf.data[2] = static_cast<uint8_t>((msg.id >> 16) & 0xFF);
        buf.data[3] = static_cast<uint8_t>((msg.id >> 24) & 0xFF);
        std::memcpy(&buf.data[4], msg.payload, sizeof(msg.payload));
        buf.length = 4 + sizeof(msg.payload);
        return true;
    }
    bool deserialize(const plugin::WireBuffer& buf, plugin::Message& msg) const override {
        if (buf.length < 4 + sizeof(msg.payload)) return false;
        msg.id = (static_cast<uint32_t>(buf.data[3]) << 24)
               | (static_cast<uint32_t>(buf.data[2]) << 16)
               | (static_cast<uint32_t>(buf.data[1]) <<  8)
               | (static_cast<uint32_t>(buf.data[0]) <<  0);
        std::memcpy(msg.payload, &buf.data[4], sizeof(msg.payload));
        return true;
    }
    const char* name() const override { return "LittleEndianSerializer"; }
};

// ---------- Static factory ----------

using FactoryFn = std::unique_ptr<plugin::ISerializerPlugin>(*)();

const std::unordered_map<std::string, FactoryFn>& registry() {
    static const std::unordered_map<std::string, FactoryFn> r{
        {"big_endian",    []() -> std::unique_ptr<plugin::ISerializerPlugin> {
            return std::make_unique<BigEndianSerializer>(); }},
        {"little_endian", []() -> std::unique_ptr<plugin::ISerializerPlugin> {
            return std::make_unique<LittleEndianSerializer>(); }},
    };
    return r;
}

}  // namespace

int main(int argc, char* argv[]) {
    const char* key = (argc > 1) ? argv[1] : "big_endian";

    auto it = registry().find(key);
    if (it == registry().end()) {
        std::fprintf(stderr, "ERROR: unknown serializer '%s'\n", key);
        return 1;
    }

    auto serializer = it->second();
    std::printf("Selected serializer: %s\n", serializer->name());

    // --- Round-trip demo ---
    plugin::Message orig;
    orig.id = 0xDEADBEEF;
    orig.set_payload("Hello from static plugin");

    plugin::WireBuffer wire{};
    if (!serializer->serialize(orig, wire)) {
        std::fprintf(stderr, "Serialization failed\n");
        return 1;
    }
    std::printf("Serialized %u bytes.  First 4 wire bytes: %02X %02X %02X %02X\n",
                wire.length, wire.data[0], wire.data[1], wire.data[2], wire.data[3]);

    plugin::Message restored{};
    if (!serializer->deserialize(wire, restored)) {
        std::fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    std::printf("Round-trip: id=0x%08X payload=\"%s\"\n", restored.id, restored.get_payload().c_str());

    if (restored.id != orig.id || restored.get_payload() != orig.get_payload()) {
        std::fprintf(stderr, "MISMATCH!\n");
        return 1;
    }
    std::printf("OK — round-trip verified.\n");
    return 0;
}
