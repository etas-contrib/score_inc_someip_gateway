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
#include <cstdlib>
#include <iostream>
#include <set>
#include <thread>
#include <vsomeip/defines.hpp>
#include <vsomeip/primitive_types.hpp>
#include <vsomeip/vsomeip.hpp>

#include "score/mw/com/runtime.h"
#include "score/span.hpp"
#include "src/network_service/interfaces/message_transfer.h"
#include "src/someipd/vsomeip_config_reader.h"

static const std::size_t max_sample_count = 10;

using score::someip_gateway::network_service::interfaces::message_transfer::
    SomeipMessageTransferProxy;
using score::someip_gateway::network_service::interfaces::message_transfer::
    SomeipMessageTransferSkeleton;
using score::someip_gateway::someipd::SomeipDConfig;

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

    std::string someipd_config_path = "etc/someipd_config.json";
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "-someipd_config") {
            someipd_config_path = argv[i + 1];
            break;
        }
    }

    SomeipDConfig config{};
    try {
        config = score::someip_gateway::someipd::ReadSomeipDConfig(someipd_config_path);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load someipd config: " << ex.what() << std::endl;
        return 1;
    }

    score::mw::com::runtime::InitializeRuntime(argc, argv);

    auto runtime = vsomeip::runtime::get();
    auto application = runtime->create_application("someipd");
    if (!application->init()) {
        std::cerr << "App init failed";
        return 1;
    }

    std::thread([application, config]() {
        auto handles =
            SomeipMessageTransferProxy::FindService(
                score::mw::com::InstanceSpecifier::Create(std::string("someipd/gatewayd_messages"))
                    .value())
                .value();

        {  // Proxy for receiving messages from gatewayd to be sent via SOME/IP
            auto proxy = SomeipMessageTransferProxy::Create(handles.front()).value();
            proxy.message_.Subscribe(max_sample_count);

            // Skeleton for transmitting messages from the network to gatewayd
            auto create_result = SomeipMessageTransferSkeleton::Create(
                score::mw::com::InstanceSpecifier::Create(std::string("someipd/someipd_messages"))
                    .value());
            // TODO: Error handling
            auto skeleton = std::move(create_result).value();
            (void)skeleton.OfferService();

            // Register message handlers for all subscribed services
            for (const auto& svc : config.subscribed_services) {
                for (const auto& ev : svc.events) {
                    application->register_message_handler(
                        svc.service_id, svc.instance_id, ev.event_id,
                        [&skeleton](const std::shared_ptr<vsomeip::message>& msg) {
                            auto maybe_message = skeleton.message_.Allocate();
                            if (!maybe_message.has_value()) {
                                std::cerr << "Failed to allocate SOME/IP message:"
                                          << maybe_message.error().Message() << std::endl;
                                return;
                            }
                            auto message_sample = std::move(maybe_message).value();
                            memcpy(message_sample->data + VSOMEIP_FULL_HEADER_SIZE,
                                   msg->get_payload()->get_data(),
                                   msg->get_payload()->get_length());
                            message_sample->size =
                                msg->get_payload()->get_length() + VSOMEIP_FULL_HEADER_SIZE;
                            skeleton.message_.Send(std::move(message_sample));
                        });
                }

                application->request_service(svc.service_id, svc.instance_id);
                for (const auto& ev : svc.events) {
                    std::set<vsomeip::eventgroup_t> groups{ev.eventgroup_id};
                    application->request_event(svc.service_id, svc.instance_id, ev.event_id, groups,
                                               vsomeip::event_type_e::ET_EVENT);
                    application->subscribe(svc.service_id, svc.instance_id, ev.eventgroup_id);
                }
            }

            // Offer all configured local services to the SOME/IP network.
            // Order matters: offer_event → offer_service (local) → update_service_configuration
            // (promote to network). update_service_configuration requires the service to already
            // be offered locally — see vsomeip application.hpp docs.
            for (const auto& svc : config.offered_services) {
                for (const auto& ev : svc.events) {
                    std::set<vsomeip::eventgroup_t> groups{ev.eventgroup_id};
                    application->offer_event(svc.service_id, svc.instance_id, ev.event_id, groups);
                }
                application->offer_service(svc.service_id, svc.instance_id);
                if (svc.unreliable_port != 0) {
                    // Expose the locally offered service on the network via UDP.
                    // This replaces the "services" section in vsomeip.json.
                    application->update_service_configuration(
                        svc.service_id, svc.instance_id, svc.unreliable_port,
                        /*reliable=*/false, /*magic_cookies_enabled=*/false, /*offer=*/true);
                }
            }

            auto payload = vsomeip::runtime::get()->create_payload();

            std::cout << "SOME/IP daemon started, waiting for messages..." << std::endl;

            // Process messages until shutdown is requested
            while (!shutdown_requested.load()) {
                // TODO: Use ReceiveHandler + async runtime instead of polling
                proxy.message_.GetNewSamples(
                    [&](auto message_sample) {
                        score::cpp::span<const std::byte> message(message_sample->data,
                                                                  message_sample->size);

                        // Check if sample size is valid and contains at least a SOME/IP header
                        if (message.size() < VSOMEIP_FULL_HEADER_SIZE) {
                            std::cerr << "Received too small sample (size: " << message.size()
                                      << ", expected at least: " << VSOMEIP_FULL_HEADER_SIZE
                                      << "). Skipping message." << std::endl;
                            return;
                        }

                        // TODO: Here we need to find a better way how to pass the message to
                        // vsomeip. There doesn't seem to be a public way to just wrap the existing
                        // buffer.
                        auto payload_data = message.subspan(VSOMEIP_FULL_HEADER_SIZE);
                        payload->set_data(
                            reinterpret_cast<const vsomeip_v3::byte_t*>(payload_data.data()),
                            payload_data.size());
                        application->notify(service_id, instance_id, event_id, payload);
                    },
                    max_sample_count);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            std::cout << "Shutting down SOME/IP daemon..." << std::endl;
        }

        application->stop();
    }).detach();

    application->start();
}
