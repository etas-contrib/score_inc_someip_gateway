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

/// @file Unit tests for LocalServiceInstance
///
/// Tests cover:
/// - LocalServiceInstance::CreateAsyncLocalService (static factory method)
/// - MockConfigBuilder utility verification

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "score/mw/com/impl/com_error.h"
#include "score/mw/com/types.h"
#include "src/gatewayd/local_service_instance.h"
#include "src/network_service/interfaces/message_transfer.h"
#include "tests/UT/mocks/mock_config_builder.h"

namespace score::someip_gateway::gatewayd {
namespace {

using network_service::interfaces::message_transfer::SomeipMessageTransferSkeleton;
using testing::MockConfigBuilder;

// ---------------------------------------------------------------------------
// Test fixture for LocalServiceInstance::CreateAsyncLocalService
// ---------------------------------------------------------------------------
class LocalServiceInstanceCreateAsyncTest : public ::testing::Test {
   protected:
    void SetUp() override { instances_.clear(); }

    /// Returns a dummy skeleton reference. This is safe for tests where the factory
    /// method returns early (e.g. nullptr config) and never accesses the skeleton.
    SomeipMessageTransferSkeleton& GetDummySkeleton() {
        return reinterpret_cast<SomeipMessageTransferSkeleton&>(dummy_skeleton_storage_);
    }

    std::vector<std::unique_ptr<LocalServiceInstance>> instances_;

   private:
    alignas(std::max_align_t) char dummy_skeleton_storage_[8192]{};
};

// --- CreateAsyncLocalService: nullptr config path ---

TEST_F(LocalServiceInstanceCreateAsyncTest, NullConfig_ReturnsError) {
    auto result =
        LocalServiceInstance::CreateAsyncLocalService(nullptr, GetDummySkeleton(), instances_);

    EXPECT_FALSE(result.has_value())
        << "Expected error result when service_instance_config is nullptr";
}

TEST_F(LocalServiceInstanceCreateAsyncTest, NullConfig_InstancesVectorUnchanged) {
    ASSERT_TRUE(instances_.empty());

    auto result =
        LocalServiceInstance::CreateAsyncLocalService(nullptr, GetDummySkeleton(), instances_);

    EXPECT_TRUE(instances_.empty())
        << "Instances vector must not be modified when config is nullptr";
}

TEST_F(LocalServiceInstanceCreateAsyncTest, NullConfig_ErrorCodeIsInvalidConfiguration) {
    auto result =
        LocalServiceInstance::CreateAsyncLocalService(nullptr, GetDummySkeleton(), instances_);

    ASSERT_FALSE(result.has_value());

    const auto actual_code = *result.error();
    const auto expected_code =
        static_cast<result::ErrorCode>(mw::com::impl::ComErrc::kInvalidConfiguration);

    EXPECT_EQ(actual_code, expected_code) << "Expected kInvalidConfiguration error code";
}

TEST_F(LocalServiceInstanceCreateAsyncTest, NullConfig_ErrorHasMessage) {
    auto result =
        LocalServiceInstance::CreateAsyncLocalService(nullptr, GetDummySkeleton(), instances_);

    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().Message().empty())
        << "Error should carry a human-readable message from the ComErrorDomain";
}

// ---------------------------------------------------------------------------
// Test fixture for MockConfigBuilder verification
// ---------------------------------------------------------------------------
class MockConfigBuilderTest : public ::testing::Test {};

TEST_F(MockConfigBuilderTest, DefaultBuild_ReturnsNonNull) {
    auto config = MockConfigBuilder().AddEvent(0x8001, "evt").Build();

    ASSERT_NE(config, nullptr);
}

TEST_F(MockConfigBuilderTest, DefaultBuild_ServiceIdMatchesDefault) {
    auto config = MockConfigBuilder().AddEvent(0x8001, "evt").Build();

    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->someip_service_id(), testing::test_constants::kDefaultServiceId);
}

TEST_F(MockConfigBuilderTest, CustomServiceId_IsPreserved) {
    constexpr std::uint16_t custom_id = 0xABCD;
    auto config = MockConfigBuilder().SetServiceId(custom_id).AddEvent(0x8001, "evt").Build();

    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->someip_service_id(), custom_id);
}

TEST_F(MockConfigBuilderTest, VersionMajor_IsPreserved) {
    constexpr std::uint8_t version = 5;
    auto config = MockConfigBuilder().SetVersionMajor(version).AddEvent(0x8001, "evt").Build();

    ASSERT_NE(config, nullptr);
    EXPECT_EQ(config->someip_service_version_major(), version);
}

TEST_F(MockConfigBuilderTest, InstanceSpecifier_IsPreserved) {
    const std::string specifier = "my_app/my_service";
    auto config =
        MockConfigBuilder().SetInstanceSpecifier(specifier).AddEvent(0x8001, "evt").Build();

    ASSERT_NE(config, nullptr);
    ASSERT_NE(config->instance_specifier(), nullptr);
    EXPECT_EQ(config->instance_specifier()->str(), specifier);
}

TEST_F(MockConfigBuilderTest, Events_ArePreserved) {
    auto config =
        MockConfigBuilder().AddEvent(0x8001, "event_alpha").AddEvent(0x8002, "event_beta").Build();

    ASSERT_NE(config, nullptr);
    ASSERT_NE(config->events(), nullptr);
    EXPECT_EQ(config->events()->size(), 2u);
}

TEST_F(MockConfigBuilderTest, EventMethodId_IsPreserved) {
    constexpr std::uint16_t method_id = 0x9001;
    auto config = MockConfigBuilder().AddEvent(method_id, "test_event").Build();

    ASSERT_NE(config, nullptr);
    ASSERT_NE(config->events(), nullptr);
    ASSERT_GE(config->events()->size(), 1u);
    EXPECT_EQ(config->events()->Get(0)->someip_method_id(), method_id);
}

TEST_F(MockConfigBuilderTest, EventName_IsPreserved) {
    const std::string event_name = "my_custom_event";
    auto config = MockConfigBuilder().AddEvent(0x8001, event_name).Build();

    ASSERT_NE(config, nullptr);
    ASSERT_NE(config->events(), nullptr);
    ASSERT_GE(config->events()->size(), 1u);
    ASSERT_NE(config->events()->Get(0)->event_name(), nullptr);
    EXPECT_EQ(config->events()->Get(0)->event_name()->str(), event_name);
}

}  // namespace
}  // namespace score::someip_gateway::gatewayd

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
