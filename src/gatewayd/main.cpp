/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

#include <atomic>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#include "local_service_instance.h"
#include "remote_service_instance.h"
#include "score/mw/com/runtime.h"
#include "score/mw/com/types.h"
#include "src/gatewayd/gatewayd_config_generated.h"
#include "src/network_service/interfaces/control_channel.h"
#include "src/network_service/interfaces/message_transfer.h"

#ifdef __QNX__
#include "score/message_passing/qnx_dispatch/qnx_dispatch_client_factory.h"
#else
#include "score/message_passing/unix_domain/unix_domain_client_factory.h"
#endif

// In the main file we are not in any namespace
using namespace score::someip_gateway::gatewayd;
namespace control_channel = score::someip_gateway::network_service::interfaces::control_channel;

using score::someip_gateway::network_service::interfaces::message_transfer::
    SomeipMessageTransferSkeleton;

// Global flag to control application shutdown
static std::atomic<bool> shutdown_requested{false};

// Signal handler for graceful shutdown
void termination_handler(int /*signal*/) {
    std::cout << "Received termination signal. Initiating graceful shutdown..." << std::endl;
    shutdown_requested.store(true);
}

int main(int argc, const char* argv[]) {
    // Register signal handlers for graceful shutdown
    std::signal(SIGTERM, termination_handler);
    std::signal(SIGINT, termination_handler);

    // Read config data
    // TODO: Be more flexible with the path
    // TODO: Use memory mapped file instead of copying into buffer
    std::ifstream config_file;
    config_file.open("src/gatewayd/etc/gatewayd_config.bin", std::ios::binary | std::ios::in);

    if (!config_file.is_open()) {
        std::cerr << "Error: Could not open config file 'src/gatewayd/etc/gatewayd_config.bin'"
                  << std::endl;
        return 1;
    }

    config_file.seekg(0, std::ios::end);
    std::streampos length = config_file.tellg();

    if (length <= 0) {
        std::cerr << "Error: Invalid config file size: " << length << std::endl;
        config_file.close();
        return 1;
    }

    config_file.seekg(0, std::ios::beg);
    auto config_buffer = std::shared_ptr<char>(new char[length]);
    config_file.read(config_buffer.get(), length);
    config_file.close();

    auto config =
        std::shared_ptr<const config::Root>(config_buffer, config::GetRoot(config_buffer.get()));

    score::mw::com::runtime::InitializeRuntime(argc, argv);

    // Suppress "AUTOSAR C++14 A16-0-1" rule findings.
    // This is the standard way to determine if it runs on QNX or Unix
    // coverity[autosar_cpp14_a16_0_1_violation]
#ifdef __QNX__
    score::message_passing::QnxDispatchClientFactory client_factory{};
    // coverity[autosar_cpp14_a16_0_1_violation]
#else
    score::message_passing::UnixDomainClientFactory client_factory{};
    // coverity[autosar_cpp14_a16_0_1_violation]
#endif
    auto control_client = client_factory.Create(control_channel::PROTOCOL_CONFIG, {});
    control_client->Start(nullptr, nullptr);

    // TODO: Need to come up with a proper scheme how to generate instance specifiers
    auto create_result = SomeipMessageTransferSkeleton::Create(
        score::mw::com::InstanceSpecifier::Create(std::string("gatewayd/gatewayd_messages"))
            .value());
    // TODO: Error handling
    auto someip_message_skeleton = std::move(create_result).value();

    // TODO: Error handling
    (void)someip_message_skeleton.OfferService();

    // Create service instances from configuration
    if (config->local_service_instances() == nullptr) {
        std::cerr << "No local service instances configured" << std::endl;
        return 1;
    }

    std::vector<std::unique_ptr<LocalServiceInstance>> local_service_instances;
    for (auto service_instance_config : *config->local_service_instances()) {
        LocalServiceInstance::CreateAsyncLocalService(
            std::shared_ptr<const config::ServiceInstance>(config, service_instance_config),
            someip_message_skeleton, local_service_instances);
    }

    // Create service instances from configuration
    if (config->remote_service_instances() == nullptr) {
        std::cerr << "No remote service instances configured" << std::endl;
        return 1;
    }

    std::vector<std::unique_ptr<RemoteServiceInstance>> remote_service_instances;
    for (auto service_instance_config : *config->remote_service_instances()) {
        RemoteServiceInstance::CreateAsyncRemoteService(
            std::shared_ptr<const config::ServiceInstance>(config, service_instance_config),
            remote_service_instances);
    }

    std::cout << "Gateway started, waiting for shutdown signal..." << std::endl;

    // Main loop - run until shutdown is requested
    while (!shutdown_requested.load()) {
        control_channel::Request request{.command_id = control_channel::CommandId::Foo};
        score::cpp::span<const std::uint8_t> request_message(
            reinterpret_cast<const std::uint8_t*>(&request), sizeof(request));
        auto result = control_client->SendWaitReply(request_message, {});
        if (!result.has_value()) {
            std::cerr << "Error sending control message: " << result.error().ToString()
                      << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Shutting down gateway..." << std::endl;

    return 0;
}
