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

#ifndef TESTS_UT_MOCKS_MOCK_CONFIG_BUILDER_H
#define TESTS_UT_MOCKS_MOCK_CONFIG_BUILDER_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "flatbuffers/flatbuffers.h"
#include "src/gatewayd/gatewayd_config_generated.h"

namespace score::someip_gateway::gatewayd::testing {

/// Common test constants for service configuration.
namespace test_constants {
constexpr std::uint16_t kDefaultServiceId = 0x1234;
constexpr std::uint8_t kDefaultVersionMajor = 1;
constexpr std::uint32_t kDefaultVersionMinor = 0;
constexpr std::uint16_t kDefaultMethodId = 0x8001;
constexpr const char* kDefaultInstanceSpecifier = "test/service_instance";
constexpr const char* kDefaultEventName = "test_event";
}  // namespace test_constants

/// Helper class to build FlatBuffers-based ServiceInstance configs for unit testing.
///
/// Usage:
///   auto config = MockConfigBuilder()
///       .SetServiceId(0x1234)
///       .AddEvent(0x8001, "my_event")
///       .Build();
class MockConfigBuilder {
   public:
    MockConfigBuilder& SetServiceId(std::uint16_t id) {
        service_id_ = id;
        return *this;
    }

    MockConfigBuilder& SetVersionMajor(std::uint8_t version) {
        version_major_ = version;
        return *this;
    }

    MockConfigBuilder& SetVersionMinor(std::uint32_t version) {
        version_minor_ = version;
        return *this;
    }

    MockConfigBuilder& SetInstanceSpecifier(const std::string& specifier) {
        instance_specifier_ = specifier;
        return *this;
    }

    MockConfigBuilder& AddEvent(std::uint16_t method_id, const std::string& event_name) {
        events_.push_back({method_id, event_name});
        return *this;
    }

    /// Builds and returns a shared_ptr to a const ServiceInstance.
    /// The returned pointer is valid as long as the shared_ptr is alive, since
    /// the underlying FlatBuffer data is co-owned via aliasing constructor.
    std::shared_ptr<const config::ServiceInstance> Build() {
        flatbuffers::FlatBufferBuilder fbb(1024);

        // Build events
        std::vector<flatbuffers::Offset<config::Event>> event_offsets;
        for (const auto& evt : events_) {
            event_offsets.push_back(
                config::CreateEventDirect(fbb, evt.method_id, evt.event_name.c_str()));
        }

        auto events_vector = fbb.CreateVectorOfSortedTables<config::Event>(&event_offsets);
        auto instance_specifier_offset = fbb.CreateString(instance_specifier_);

        auto si = config::CreateServiceInstance(fbb, service_id_, version_major_, version_minor_,
                                                instance_specifier_offset, events_vector);
        fbb.Finish(si);

        // Copy the FlatBuffer into shared memory so the ServiceInstance pointer stays valid
        auto buf_size = fbb.GetSize();
        auto buf = std::shared_ptr<std::uint8_t>(new std::uint8_t[buf_size],
                                                 std::default_delete<std::uint8_t[]>());
        std::memcpy(buf.get(), fbb.GetBufferPointer(), buf_size);

        const auto* service_instance = flatbuffers::GetRoot<config::ServiceInstance>(buf.get());

        // Use aliasing constructor: shares ownership of buf, but points to service_instance
        return std::shared_ptr<const config::ServiceInstance>(buf, service_instance);
    }

   private:
    struct EventConfig {
        std::uint16_t method_id;
        std::string event_name;
    };

    std::uint16_t service_id_ = test_constants::kDefaultServiceId;
    std::uint8_t version_major_ = test_constants::kDefaultVersionMajor;
    std::uint32_t version_minor_ = test_constants::kDefaultVersionMinor;
    std::string instance_specifier_ = test_constants::kDefaultInstanceSpecifier;
    std::vector<EventConfig> events_;
};

}  // namespace score::someip_gateway::gatewayd::testing

#endif  // TESTS_UT_MOCKS_MOCK_CONFIG_BUILDER_H
