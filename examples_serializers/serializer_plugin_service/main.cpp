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
// Option 4 — Service-oriented serializer registry.
//
// A lightweight service registry maps service names + versions to concrete
// serializer implementations.  This mimics AUTOSAR Adaptive ara::com style
// service discovery without pulling in the full middleware stack.
//
// Contract:
//   Input  : service name string (argv[1], default "BigEndianSerializer/1.0")
//   Output : round-trip serialize→deserialize
//   Failure: service not found, version mismatch → error, exit 1
//   Safety : all in-process; deterministic if registry is populated at init
//
// Build : bazel build //examples/serializer_plugin_service
// Run   : bazel-bin/examples/serializer_plugin_service/service_demo
//         bazel-bin/examples/serializer_plugin_service/service_demo "LittleEndianSerializer/1.0"

#include "examples_serializers/serializer_plugin_common/common.hpp"

#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// Service registry — a minimal stand-in for ara::com service discovery.
// ============================================================================

namespace service {

struct ServiceId {
    std::string name;
    uint32_t    version_major{1};
    uint32_t    version_minor{0};

    bool operator==(const ServiceId& o) const {
        return name == o.name
            && version_major == o.version_major
            && version_minor == o.version_minor;
    }
};

struct ServiceIdHash {
    size_t operator()(const ServiceId& id) const {
        size_t h = std::hash<std::string>{}(id.name);
        h ^= std::hash<uint32_t>{}(id.version_major) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(id.version_minor) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

using Factory = std::function<std::unique_ptr<plugin::ISerializerPlugin>()>;

class Registry {
public:
    static Registry& instance() {
        static Registry r;
        return r;
    }

    void offer(const ServiceId& id, Factory factory) {
        services_[id] = std::move(factory);
        std::printf("[Registry] Service offered: %s/%u.%u\n",
                    id.name.c_str(), id.version_major, id.version_minor);
    }

    std::unique_ptr<plugin::ISerializerPlugin> find(const ServiceId& id) const {
        auto it = services_.find(id);
        if (it == services_.end()) return nullptr;
        return it->second();
    }

    /// List all available services (diagnostic).
    void list() const {
        for (const auto& [id, _] : services_) {
            std::printf("  - %s/%u.%u\n", id.name.c_str(),
                        id.version_major, id.version_minor);
        }
    }

private:
    Registry() = default;
    std::unordered_map<ServiceId, Factory, ServiceIdHash> services_;
};

/// Helper: self-registering service.  Use as a file-scope static.
struct AutoRegister {
    AutoRegister(const ServiceId& id, Factory f) {
        Registry::instance().offer(id, std::move(f));
    }
};

}  // namespace service

// ============================================================================
// Concrete serializer implementations.
// ============================================================================

namespace {

class BigEndianSerializer final : public plugin::ISerializerPlugin {
public:
    bool serialize(const plugin::Message& msg, plugin::WireBuffer& buf) const override {
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

// --- Self-register at static-init time ---
static service::AutoRegister reg_be(
    {"BigEndianSerializer", 1, 0},
    [] { return std::make_unique<BigEndianSerializer>(); });

static service::AutoRegister reg_le(
    {"LittleEndianSerializer", 1, 0},
    [] { return std::make_unique<LittleEndianSerializer>(); });

}  // namespace

// ============================================================================
// Main — consumer that discovers the service by name.
// ============================================================================

int main(int argc, char* argv[]) {
    // Parse "Name/Major.Minor" from argv[1], default BigEndianSerializer/1.0
    std::string spec = (argc > 1) ? argv[1] : "BigEndianSerializer/1.0";
    service::ServiceId wanted;

    auto slash = spec.find('/');
    if (slash != std::string::npos) {
        wanted.name = spec.substr(0, slash);
        if (std::sscanf(spec.c_str() + slash + 1, "%u.%u",
                        &wanted.version_major, &wanted.version_minor) < 1) {
            wanted.version_major = 1;
            wanted.version_minor = 0;
        }
    } else {
        wanted.name = spec;
    }

    std::printf("Requesting service: %s/%u.%u\n",
                wanted.name.c_str(), wanted.version_major, wanted.version_minor);

    auto serializer = service::Registry::instance().find(wanted);
    if (!serializer) {
        std::fprintf(stderr, "Service not found.  Available services:\n");
        service::Registry::instance().list();
        return 1;
    }
    std::printf("Bound to: %s\n", serializer->name());

    // --- Round-trip demo ---
    plugin::Message orig;
    orig.id = 0xFACEFEED;
    orig.set_payload("Hello from service consumer");

    plugin::WireBuffer wire{};
    if (!serializer->serialize(orig, wire)) {
        std::fprintf(stderr, "Serialization failed\n");
        return 1;
    }
    std::printf("Serialized %u bytes.  First 4: %02X %02X %02X %02X\n",
                wire.length, wire.data[0], wire.data[1], wire.data[2], wire.data[3]);

    plugin::Message restored{};
    if (!serializer->deserialize(wire, restored)) {
        std::fprintf(stderr, "Deserialization failed\n");
        return 1;
    }
    std::printf("Round-trip: id=0x%08X payload=\"%s\"\n",
                restored.id, restored.get_payload().c_str());

    if (restored.id != orig.id || restored.get_payload() != orig.get_payload()) {
        std::fprintf(stderr, "MISMATCH!\n");
        return 1;
    }
    std::printf("OK — round-trip verified via service registry.\n");
    return 0;
}
