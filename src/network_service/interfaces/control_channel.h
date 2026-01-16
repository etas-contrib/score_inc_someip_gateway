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

#ifndef SRC_NETWORK_SERVICE_INTERFACES_CONTROL_CHANNEL
#define SRC_NETWORK_SERVICE_INTERFACES_CONTROL_CHANNEL

#include "score/message_passing/service_protocol_config.h"

/// Service for exchanging control information.
/// Used between gatewayd and someipd for signalling service availability etc.
namespace score::someip_gateway::network_service::interfaces::control_channel {

extern "C" {
struct FooCommand {};
struct BarCommand {};

enum class CommandId : std::uint8_t {
    Foo,
    Bar,
};

struct Request {
    // TODO: Make this a template?
    // TODO: Make this more closely match the Repr(C) enums from Rust.
    // Is that already contained in abi-compatible data types?
    CommandId command_id;
    union {
        FooCommand foo;
        BarCommand bar;
    } command_data;
};
}  // extern "C"

constexpr score::message_passing::ServiceProtocolConfig PROTOCOL_CONFIG{
    .identifier = "score_someipd_control_channel",
    .max_send_size = sizeof(Request),
};

}  // namespace score::someip_gateway::network_service::interfaces::control_channel

#endif  // SRC_NETWORK_SERVICE_INTERFACES_CONTROL_CHANNEL
