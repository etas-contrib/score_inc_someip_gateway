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

#include "vsomeip_network_server.h"

#include <cerrno>
#include <iostream>

#include "score/message_passing/i_server_connection.h"
#include "src/network_service/interfaces/control_channel.h"

namespace score::someip_gateway::someipd {

VsomeipNetworkServer::VsomeipNetworkServer(message_passing::IServerConnection&) {}

VsomeipNetworkServer::~VsomeipNetworkServer() = default;

score::cpp::expected_blank<score::os::Error> VsomeipNetworkServer::OnMessageSent(
    score::message_passing::IServerConnection& connection,
    score::cpp::span<const std::uint8_t> message) noexcept {
    return score::cpp::make_unexpected(score::os::Error::createFromErrno(EPERM));
}

score::cpp::expected_blank<score::os::Error> VsomeipNetworkServer::OnMessageSentWithReply(
    score::message_passing::IServerConnection& connection,
    score::cpp::span<const std::uint8_t> message) noexcept {
    if (message.size() != sizeof(control_channel::Request)) {
        return score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL));
    }
    auto request = reinterpret_cast<const control_channel::Request*>(message.data());
    switch (request->command_id) {
        case control_channel::CommandId::Foo: {
            this->Process(request->command_data.foo);
        } break;
        case control_channel::CommandId::Bar: {
            this->Process(request->command_data.bar);
        } break;
        default: {
            return score::cpp::make_unexpected(score::os::Error::createFromErrno(EINVAL));
        }
    }
    return connection.Reply({});
}

void VsomeipNetworkServer::OnDisconnect(
    score::message_passing::IServerConnection& connection) noexcept {}

void VsomeipNetworkServer::Process(const control_channel::FooCommand&) {
    std::cout << "Processing FooCommand" << std::endl;
}

void VsomeipNetworkServer::Process(const control_channel::BarCommand&) {
    std::cout << "Processing BarCommand" << std::endl;
}

}  // namespace score::someip_gateway::someipd
