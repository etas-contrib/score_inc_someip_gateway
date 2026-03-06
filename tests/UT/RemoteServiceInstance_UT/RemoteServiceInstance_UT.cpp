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

/// Tests cover:
/// - RemoteServiceInstance::CreateAsyncRemoteService (nullptr path)
/// - RemoteServiceInstance constructor (OfferService, SetReceiveHandler, Subscribe)
/// - Receive-handler message processing (GetNewSamples, Allocate, Send, error paths)

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <functional>
#include <memory>
#include <vector>

#include "score/mw/com/impl/com_error.h"
#include "score/mw/com/impl/mocking/proxy_event_mock.h"
#include "score/mw/com/impl/mocking/proxy_wrapper_class_test_view.h"
#include "score/mw/com/impl/mocking/skeleton_base_mock.h"
#include "score/mw/com/impl/mocking/skeleton_event_mock.h"
#include "score/mw/com/impl/mocking/skeleton_wrapper_class_test_view.h"
#include "score/mw/com/impl/mocking/test_type_utilities.h"
#include "score/mw/com/impl/proxy_base.h"
#include "score/mw/com/types.h"
#include "src/gatewayd/remote_service_instance.h"
#include "src/network_service/interfaces/message_transfer.h"
#include "tests/UT/mocks/mock_config_builder.h"
#include "tests/performance_benchmarks/echo_service.h"

namespace score::someip_gateway::gatewayd {
namespace {

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

using score::mw::com::impl::ProxyEventMock;
using score::mw::com::impl::SkeletonBaseMock;
using score::mw::com::impl::SkeletonEventMock;
using testing::MockConfigBuilder;

using network_service::interfaces::message_transfer::SomeipMessage;
using SomeipMTProxy = network_service::interfaces::message_transfer::SomeipMessageTransferProxy;

using score::mw::com::impl::MakeFakeSampleAllocateePtr;
using score::mw::com::impl::MakeFakeSamplePtr;
using score::mw::com::impl::NamedProxyEventMock;
using score::mw::com::impl::NamedSkeletonEventMock;
using score::mw::com::impl::ProxyWrapperClassTestView;
using score::mw::com::impl::SkeletonWrapperClassTestView;

// Convenience: return a successful ResultBlank
static score::ResultBlank OkBlank() { return score::cpp::blank{}; }

// ---------------------------------------------------------------------------
// CreateAsyncRemoteService – nullptr path (existing tests)
// ---------------------------------------------------------------------------
class RemoteServiceInstanceCreateAsyncTest : public ::testing::Test {
   protected:
    void SetUp() override { instances_.clear(); }
    std::vector<std::unique_ptr<RemoteServiceInstance>> instances_;
};

TEST_F(RemoteServiceInstanceCreateAsyncTest, NullConfig_ReturnsError) {
    auto result = RemoteServiceInstance::CreateAsyncRemoteService(nullptr, instances_);
    EXPECT_FALSE(result.has_value())
        << "Expected error result when service_instance_config is nullptr";
}

TEST_F(RemoteServiceInstanceCreateAsyncTest, NullConfig_InstancesVectorUnchanged) {
    ASSERT_TRUE(instances_.empty());
    auto result = RemoteServiceInstance::CreateAsyncRemoteService(nullptr, instances_);
    EXPECT_TRUE(instances_.empty())
        << "Instances vector must not be modified when config is nullptr";
}

TEST_F(RemoteServiceInstanceCreateAsyncTest, NullConfig_ErrorCodeIsInvalidConfiguration) {
    auto result = RemoteServiceInstance::CreateAsyncRemoteService(nullptr, instances_);
    ASSERT_FALSE(result.has_value());
    const auto actual_code = *result.error();
    const auto expected_code =
        static_cast<result::ErrorCode>(mw::com::impl::ComErrc::kInvalidConfiguration);
    EXPECT_EQ(actual_code, expected_code) << "Expected kInvalidConfiguration error code";
}

TEST_F(RemoteServiceInstanceCreateAsyncTest, NullConfig_ErrorHasMessage) {
    auto result = RemoteServiceInstance::CreateAsyncRemoteService(nullptr, instances_);
    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().Message().empty())
        << "Error should carry a human-readable message from the ComErrorDomain";
}

TEST_F(RemoteServiceInstanceCreateAsyncTest, NullConfig_MultipleCallsAllReturnError) {
    for (int i = 0; i < 3; ++i) {
        auto result = RemoteServiceInstance::CreateAsyncRemoteService(nullptr, instances_);
        EXPECT_FALSE(result.has_value()) << "Call #" << i << " should return error";
    }
    EXPECT_TRUE(instances_.empty())
        << "No instances should be created across multiple failed calls";
}

// ---------------------------------------------------------------------------
// Constructor & message-processing tests
// ---------------------------------------------------------------------------
class RemoteServiceInstanceCtorTest : public ::testing::Test {
   protected:
    // Event name strings – must match echo_service.h / message_transfer.h
    static constexpr std::string_view kTiny{"echo_response_tiny"};
    static constexpr std::string_view kSmall{"echo_response_small"};
    static constexpr std::string_view kMedium{"echo_response_medium"};
    static constexpr std::string_view kLarge{"echo_response_large"};
    static constexpr std::string_view kXLarge{"echo_response_xlarge"};
    static constexpr std::string_view kXXLarge{"echo_response_xxlarge"};
    static constexpr std::string_view kMsg{"message"};

    RemoteServiceInstanceCtorTest()
        : skel_events_{kTiny, kSmall, kMedium, kLarge, kXLarge, kXXLarge}, proxy_events_{kMsg} {}

    void SetUp() override {
        config_ = MockConfigBuilder()
                      .SetInstanceSpecifier("test/remote_svc")
                      .AddEvent(0x8001, "evt")
                      .Build();
    }

    void TearDown() override { score::mw::com::impl::ResetInstanceIdentifierConfiguration(); }

    // Build mocked skeleton + proxy, inject our mock objects, return both.
    struct Deps {
        echo_service::EchoResponseSkeleton skeleton;
        SomeipMTProxy proxy;
    };

    Deps BuildDeps() {
        auto skeleton = SkeletonWrapperClassTestView<echo_service::EchoResponseSkeleton>::Create(
            skel_base_mock_, skel_events_);
        auto proxy = ProxyWrapperClassTestView<SomeipMTProxy>::Create(proxy_events_);
        // Register the event in the ProxyBase events_ map so the move ctor succeeds
        score::mw::com::impl::ProxyBaseView{proxy}.RegisterEvent(kMsg, proxy.message_);
        // Inject mock after registration
        proxy.message_.InjectMock(MsgMock());
        return {std::move(skeleton), std::move(proxy)};
    }

    // Shorthand mock accessors
    SkeletonBaseMock& SkelBase() { return skel_base_mock_; }
    SkeletonEventMock<echo_service::EchoResponseTiny>& TinyEvt() {
        return std::get<0>(skel_events_).mock;
    }
    ProxyEventMock<SomeipMessage>& MsgMock() { return std::get<0>(proxy_events_).mock; }

    std::shared_ptr<const config::ServiceInstance> config_;

   private:
    SkeletonBaseMock skel_base_mock_;
    std::tuple<NamedSkeletonEventMock<echo_service::EchoResponseTiny>,
               NamedSkeletonEventMock<echo_service::EchoResponseSmall>,
               NamedSkeletonEventMock<echo_service::EchoResponseMedium>,
               NamedSkeletonEventMock<echo_service::EchoResponseLarge>,
               NamedSkeletonEventMock<echo_service::EchoResponseXLarge>,
               NamedSkeletonEventMock<echo_service::EchoResponseXXLarge>>
        skel_events_;
    std::tuple<NamedProxyEventMock<SomeipMessage>> proxy_events_;
};

// --- Constructor wiring ---

TEST_F(RemoteServiceInstanceCtorTest, CallsOfferService) {
    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_)).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));
}

TEST_F(RemoteServiceInstanceCtorTest, SetsReceiveHandlerOnMessageEvent) {
    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_)).Times(1).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));
}

TEST_F(RemoteServiceInstanceCtorTest, SubscribesWithMaxSampleCount) {
    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_)).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), Subscribe(10)).Times(1).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));
}

// --- Receive handler: message too small ---

TEST_F(RemoteServiceInstanceCtorTest, MsgTooSmall_DoesNotAllocate) {
    std::function<void()> captured_handler;

    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_))
        .WillOnce(Invoke(
            [&captured_handler](score::mw::com::impl::EventReceiveHandler h) -> score::ResultBlank {
                auto sp = std::make_shared<score::mw::com::impl::EventReceiveHandler>(std::move(h));
                captured_handler = [sp]() { (*sp)(); };
                return OkBlank();
            }));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));

    // GetNewSamples delivers a message that is < 16 bytes
    EXPECT_CALL(MsgMock(), GetNewSamples(_, 10))
        .WillOnce(Invoke([](ProxyEventMock<SomeipMessage>::Callback&& cb,
                            const std::size_t) -> score::Result<std::size_t> {
            auto msg = std::make_unique<SomeipMessage>();
            msg->size = 4;  // too small
            std::memset(msg->data, 0, 4);
            cb(MakeFakeSamplePtr<SomeipMessage>(std::move(msg)));
            return {std::size_t{1}};
        }));

    EXPECT_CALL(TinyEvt(), Allocate()).Times(0);

    ASSERT_TRUE(captured_handler);
    captured_handler();
}

// --- Receive handler: Allocate fails ---

TEST_F(RemoteServiceInstanceCtorTest, AllocateFails_DoesNotSend) {
    std::function<void()> captured_handler;

    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_))
        .WillOnce(Invoke(
            [&captured_handler](score::mw::com::impl::EventReceiveHandler h) -> score::ResultBlank {
                auto sp = std::make_shared<score::mw::com::impl::EventReceiveHandler>(std::move(h));
                captured_handler = [sp]() { (*sp)(); };
                return OkBlank();
            }));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));

    EXPECT_CALL(MsgMock(), GetNewSamples(_, 10))
        .WillOnce(Invoke([](ProxyEventMock<SomeipMessage>::Callback&& cb,
                            const std::size_t) -> score::Result<std::size_t> {
            auto msg = std::make_unique<SomeipMessage>();
            msg->size = 32;
            std::memset(msg->data, 0xAB, 32);
            cb(MakeFakeSamplePtr<SomeipMessage>(std::move(msg)));
            return {std::size_t{1}};
        }));

    EXPECT_CALL(TinyEvt(), Allocate())
        .WillOnce(Return(score::MakeUnexpected(mw::com::impl::ComErrc::kCommunicationStackError)));

    EXPECT_CALL(TinyEvt(),
                Send(::testing::An<
                     score::mw::com::impl::SampleAllocateePtr<echo_service::EchoResponseTiny>>()))
        .Times(0);

    ASSERT_TRUE(captured_handler);
    captured_handler();
}

// --- Receive handler: happy path ---

TEST_F(RemoteServiceInstanceCtorTest, ValidMessage_AllocatesAndSends) {
    std::function<void()> captured_handler;

    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_))
        .WillOnce(Invoke(
            [&captured_handler](score::mw::com::impl::EventReceiveHandler h) -> score::ResultBlank {
                auto sp = std::make_shared<score::mw::com::impl::EventReceiveHandler>(std::move(h));
                captured_handler = [sp]() { (*sp)(); };
                return OkBlank();
            }));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));

    EXPECT_CALL(MsgMock(), GetNewSamples(_, 10))
        .WillOnce(Invoke([](ProxyEventMock<SomeipMessage>::Callback&& cb,
                            const std::size_t) -> score::Result<std::size_t> {
            auto msg = std::make_unique<SomeipMessage>();
            msg->size = 48;  // 16-byte header + 32 payload
            std::memset(msg->data, 0, 16);
            std::memset(msg->data + 16, 0x42, 32);
            cb(MakeFakeSamplePtr<SomeipMessage>(std::move(msg)));
            return {std::size_t{1}};
        }));

    EXPECT_CALL(TinyEvt(), Allocate())
        .WillOnce(Invoke(
            []() -> score::Result<
                     score::mw::com::impl::SampleAllocateePtr<echo_service::EchoResponseTiny>> {
                auto s = std::make_unique<echo_service::EchoResponseTiny>();
                return MakeFakeSampleAllocateePtr<echo_service::EchoResponseTiny>(std::move(s));
            }));

    EXPECT_CALL(TinyEvt(),
                Send(::testing::An<
                     score::mw::com::impl::SampleAllocateePtr<echo_service::EchoResponseTiny>>()))
        .Times(1)
        .WillOnce(Return(OkBlank()));

    ASSERT_TRUE(captured_handler);
    captured_handler();
}

// --- Receive handler: two samples processed ---

TEST_F(RemoteServiceInstanceCtorTest, TwoSamples_BothProcessed) {
    std::function<void()> captured_handler;

    EXPECT_CALL(SkelBase(), OfferService()).WillOnce(Return(OkBlank()));
    EXPECT_CALL(MsgMock(), SetReceiveHandler(_))
        .WillOnce(Invoke(
            [&captured_handler](score::mw::com::impl::EventReceiveHandler h) -> score::ResultBlank {
                auto sp = std::make_shared<score::mw::com::impl::EventReceiveHandler>(std::move(h));
                captured_handler = [sp]() { (*sp)(); };
                return OkBlank();
            }));
    EXPECT_CALL(MsgMock(), Subscribe(10)).WillOnce(Return(OkBlank()));

    auto d = BuildDeps();
    RemoteServiceInstance inst(config_, std::move(d.skeleton), std::move(d.proxy));

    EXPECT_CALL(MsgMock(), GetNewSamples(_, 10))
        .WillOnce(Invoke([](ProxyEventMock<SomeipMessage>::Callback&& cb,
                            const std::size_t) -> score::Result<std::size_t> {
            for (int i = 0; i < 2; ++i) {
                auto msg = std::make_unique<SomeipMessage>();
                msg->size = 20;
                std::memset(msg->data, 0, 20);
                cb(MakeFakeSamplePtr<SomeipMessage>(std::move(msg)));
            }
            return {std::size_t{2}};
        }));

    EXPECT_CALL(TinyEvt(), Allocate())
        .Times(2)
        .WillRepeatedly(Invoke(
            []() -> score::Result<
                     score::mw::com::impl::SampleAllocateePtr<echo_service::EchoResponseTiny>> {
                auto s = std::make_unique<echo_service::EchoResponseTiny>();
                return MakeFakeSampleAllocateePtr<echo_service::EchoResponseTiny>(std::move(s));
            }));

    EXPECT_CALL(TinyEvt(),
                Send(::testing::An<
                     score::mw::com::impl::SampleAllocateePtr<echo_service::EchoResponseTiny>>()))
        .Times(2)
        .WillRepeatedly(Return(OkBlank()));

    ASSERT_TRUE(captured_handler);
    captured_handler();
}

}  // namespace
}  // namespace score::someip_gateway::gatewayd

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
