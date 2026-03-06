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

/**
 * These mocks are injected into the real Proxy / Skeleton objects via
 * their public InjectMock() helpers so that unit tests can verify the
 * calls made by gatewayd source code without needing a real binding.
 */

#ifndef TESTS_UT_MOCKS_MOCK_ARA_COM_TYPES_H
#define TESTS_UT_MOCKS_MOCK_ARA_COM_TYPES_H

#include <gmock/gmock.h>

#include "score/mw/com/impl/mocking/i_proxy_event.h"
#include "score/mw/com/impl/mocking/i_skeleton_base.h"
#include "score/mw/com/impl/mocking/i_skeleton_event.h"

namespace score::someip_gateway::gatewayd::testing {

// -----------------------------------------------------------------------
// MockSkeletonBase – mocks OfferService / StopOfferService
// -----------------------------------------------------------------------
class MockSkeletonBase : public score::mw::com::impl::ISkeletonBase {
   public:
    MOCK_METHOD(score::ResultBlank, OfferService, (), (override));
    MOCK_METHOD(void, StopOfferService, (), (override));
};

// -----------------------------------------------------------------------
// MockSkeletonEvent<T> – mocks Allocate / Send
// -----------------------------------------------------------------------
template <typename SampleType>
class MockSkeletonEvent : public score::mw::com::impl::ISkeletonEvent<SampleType> {
   public:
    MOCK_METHOD(score::ResultBlank, Send, (const SampleType&), (override));
    MOCK_METHOD(score::ResultBlank, Send, (score::mw::com::impl::SampleAllocateePtr<SampleType>),
                (override));
    MOCK_METHOD((score::Result<score::mw::com::impl::SampleAllocateePtr<SampleType>>), Allocate, (),
                (override));
};

// -----------------------------------------------------------------------
// MockProxyEvent<T> – mocks Subscribe / SetReceiveHandler / GetNewSamples …
// -----------------------------------------------------------------------
template <typename SampleType>
class MockProxyEvent : public score::mw::com::impl::IProxyEvent<SampleType> {
   public:
    using Callback = typename score::mw::com::impl::IProxyEvent<SampleType>::Callback;

    MOCK_METHOD(score::ResultBlank, Subscribe, (const std::size_t), (override));
    MOCK_METHOD(void, Unsubscribe, (), (override));
    MOCK_METHOD(score::mw::com::impl::SubscriptionState, GetSubscriptionState, (),
                (const, override));
    MOCK_METHOD(std::size_t, GetFreeSampleCount, (), (const, override));
    MOCK_METHOD((score::Result<std::size_t>), GetNumNewSamplesAvailable, (), (override));
    MOCK_METHOD(score::ResultBlank, SetReceiveHandler, (score::mw::com::impl::EventReceiveHandler),
                (override));
    MOCK_METHOD(score::ResultBlank, UnsetReceiveHandler, (), (override));
    MOCK_METHOD((score::Result<std::size_t>), GetNewSamples, (Callback&&, const std::size_t),
                (override));
};

}  // namespace score::someip_gateway::gatewayd::testing

#endif  // TESTS_UT_MOCKS_MOCK_ARA_COM_TYPES_H
