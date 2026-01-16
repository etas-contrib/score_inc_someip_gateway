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

#ifndef SRC_SOMEIPD_VSOMEIP_NETWORK_SERVER
#define SRC_SOMEIPD_VSOMEIP_NETWORK_SERVER

#include "score/message_passing/i_connection_handler.h"
#include "src/network_service/interfaces/control_channel.h"

namespace score::someip_gateway::someipd {

namespace control_channel = score::someip_gateway::network_service::interfaces::control_channel;

// TODO: Abstract the generic concept and move it to network_service
class VsomeipNetworkServer final : public score::message_passing::IConnectionHandler {
   public:
    VsomeipNetworkServer(message_passing::IServerConnection& connection);
    ~VsomeipNetworkServer() override;

    score::cpp::expected_blank<score::os::Error> OnMessageSent(
        score::message_passing::IServerConnection& connection,
        score::cpp::span<const std::uint8_t> message) noexcept override;

    score::cpp::expected_blank<score::os::Error> OnMessageSentWithReply(
        score::message_passing::IServerConnection& connection,
        score::cpp::span<const std::uint8_t> message) noexcept override;

    void OnDisconnect(score::message_passing::IServerConnection& connection) noexcept override;

    void Process(const control_channel::FooCommand&);
    void Process(const control_channel::BarCommand&);
};
}  // namespace score::someip_gateway::someipd

#endif  // SRC_SOMEIPD_VSOMEIP_NETWORK_SERVER
