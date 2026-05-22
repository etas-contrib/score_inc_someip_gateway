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

#include "remote_service_instance.h"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>

#include "score/containers/non_relocatable_vector.h"
#include "score/mw/com/types.h"
#include "score/socom/runtime.hpp"
#include "score/someip/constants.h"
#include "src/serializer/serializer.h"

using score::mw::com::GenericProxy;
using score::mw::com::SamplePtr;
using score::someip::EventId;

namespace score::someip_gateway::gatewayd {

RemoteServiceInstance::RemoteServiceInstance(
    std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config,
    std::shared_ptr<const mw_someip_config::ServiceType> service_type_config,
    score::mw::com::GenericSkeleton&& ipc_skeleton, socom::Runtime& socom_runtime)
    : service_instance_config_(std::move(service_instance_config)),
      service_type_config_(std::move(service_type_config)),
      ipc_skeleton_(std::move(ipc_skeleton)) {
    // TODO: Error handling
    (void)ipc_skeleton_.OfferService();

    auto service_type_name = service_type_config_->service_type_name()->string_view();
    for (auto event_config : *service_type_config_->events()) {
        auto event_name = event_config->event_name()->string_view();

        auto events_it = ipc_skeleton_.GetEvents().find(*event_config->event_name());
        if (events_it == ipc_skeleton_.GetEvents().cend()) {
            std::cerr << "[gatewayd] Event '" << event_name << "' not found in IPC skeleton"
                      << std::endl;
            continue;
        }
        auto& ipc_event = const_cast<score::mw::com::GenericSkeletonEvent&>(events_it->second);

        const score_com_serializer* serializer = nullptr;
        auto get_result =
            score_com_serializer_get(service_type_name.data(), service_type_name.size(),
                                     score_com_serializer_element_type_event, event_name.data(),
                                     event_name.size(), &serializer);
        if (get_result != score_com_serializer_result_ok) {
            std::cerr << "[gatewayd] Failed to get serializer for " << service_type_name
                      << "::" << event_name << std::endl;
            continue;
        }

        event_contexts_.emplace(event_config->event_id(),
                                EventContext{event_config, serializer, &ipc_event});
    }

    socom::Service_interface_identifier const iface{
        service_type_config_->service_type_name()->string_view(),
        {service_type_config_->service_version_major(),
         static_cast<uint16_t>(service_type_config_->service_version_minor())}};

    socom::Service_instance const inst{service_type_config_->service_type_name()->string_view()};

    socom::Service_interface_definition const client_config{
        iface, socom::to_num_of_methods(0),
        socom::to_num_of_events(service_type_config_->events()->size())};

    // Callbacks capture `this`. on_service_state_change fires only after make_client_connector
    // returns (initial state is always not_available), so client_connector_ is set by then.
    auto connector_result = socom_runtime.make_client_connector(
        client_config, inst,
        {
            .on_service_state_change =
                [this](socom::Client_connector const&, socom::Service_state state,
                       socom::Server_service_interface_definition const&) {
                    std::cout << "[gatewayd] RemoteServiceInstance - client_connector "
                                 "on_service_state_change: "
                              << "new state=" << static_cast<int>(state) << std::endl;
                    if (state != socom::Service_state::available) {
                        return;
                    }
                    std::cout << "[gatewayd] RemoteServiceInstance - client_connector "
                                 "on_service_state_change: service is now available, subscribing "
                                 "to events\n";
                    for (std::size_t i = 0; i < service_type_config_->events()->size(); ++i) {
                        (void)client_connector_->subscribe_event(static_cast<socom::Event_id>(i),
                                                                 socom::Event_mode::update);
                    }
                },
            .on_event_update =
                [this](socom::Client_connector const&, socom::Event_id event_id,
                       socom::Payload payload) { forward_event(event_id, std::move(payload)); },
            .on_event_requested_update = [](socom::Client_connector const&, socom::Event_id,
                                            socom::Payload) {},
            .on_event_payload_allocate = [](socom::Client_connector const&, socom::Event_id)
                -> score::Result<socom::Writable_payload> {
                auto buffer = std::make_unique<std::byte[]>(someip::kMaxMessageSize);
                auto* const data_ptr = buffer.get();
                socom::Writable_payload::Writable_span const span{data_ptr,
                                                                  someip::kMaxMessageSize};
                return socom::Writable_payload{span, socom::kNoSlotHandle,
                                               [buf = std::move(buffer)]() mutable noexcept {}};
            },
        });

    if (!connector_result.has_value()) {
        std::cerr << "[gatewayd] Failed to create client connector for '"
                  << service_type_config_->service_type_name()->string_view() << "'\n";
        return;
    }
    client_connector_ = std::move(connector_result).value();
}

void RemoteServiceInstance::forward_event(socom::Event_id event_id, socom::Payload payload) {
    auto const event_index = static_cast<std::size_t>(event_id);
    auto const* const events = service_type_config_->events();
    if (event_index >= events->size()) {
        std::cerr << "[gatewayd] event_id " << event_index << " out of range, dropping\n";
        return;
    }
    auto const* const event_config = (*events)[event_index];

    auto events_it = ipc_skeleton_.GetEvents().find(event_config->event_name()->string_view());
    if (events_it == ipc_skeleton_.GetEvents().cend()) {
        std::cerr << "[gatewayd] Event '" << event_config->event_name()->string_view()
                  << "' not found in IPC skeleton, dropping\n";
        return;
    }
    auto& ipc_event = const_cast<score::mw::com::GenericSkeletonEvent&>(events_it->second);

    auto maybe_sample = ipc_event.Allocate();
    if (!maybe_sample.has_value()) {
        std::cerr << "[gatewayd] Failed to allocate IPC sample: " << maybe_sample.error().Message()
                  << "\n";
        return;
    }
    auto sample = std::move(maybe_sample).value();

    auto deserialize_result = score_com_serializer_deserialize(
        event_context.serializer, reinterpret_cast<const uint8_t*>(payload.data()), payload.size(),
        sample.Get());
    if (deserialize_result != score_com_serializer_result_ok) {
        std::cerr << "[gatewayd] Deserialization failed for event 0x" << std::hex << rec_event_id
                  << std::dec << ", dropping" << std::endl;
        return;
    }

    event_context.ipc_event->Send(std::move(sample));
    std::cout << "[gatewayd] Forwarded event 0x" << std::hex << rec_event_id << std::dec
              << " to IPC subscribers" << std::endl;
},
            someip::kMaxSampleCount);
});

someip_message_proxy_.message_.Subscribe(someip::kMaxSampleCount);
}

namespace {
struct FindServiceContext {
    std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config;
    std::shared_ptr<const mw_someip_config::ServiceType> service_type_config;
    score::mw::com::GenericSkeleton skeleton;
    std::vector<std::unique_ptr<RemoteServiceInstance>>& instances;

    FindServiceContext(
        std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config_,
        std::shared_ptr<const mw_someip_config::ServiceType> service_type_config_,
        score::mw::com::GenericSkeleton&& skeleton_,
        std::vector<std::unique_ptr<RemoteServiceInstance>>& instances_)
        : service_instance_config(std::move(service_instance_config_)),
          service_type_config(std::move(service_type_config_)),
          skeleton(std::move(skeleton_)),
          instances(instances_) {}
};

}  // namespace

Result<mw::com::FindServiceHandle> RemoteServiceInstance::CreateAsyncRemoteService(
    std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config,
    std::shared_ptr<const mw_someip_config::ServiceType> service_type_config,
    socom::Runtime& socom_runtime, std::vector<std::unique_ptr<RemoteServiceInstance>>& instances) {
    if (service_instance_config == nullptr) {
        std::cerr << "[gatewayd] ERROR: Service instance config is nullptr!\n";
        return;
    }

    score::containers::NonRelocatableVector<score::mw::com::EventInfo> events(
        service_type_config->events()->size());

    auto service_type_name = service_type_config->service_type_name()->string_view();

    for (const auto& event : *service_type_config->events()) {
        if (event == nullptr) {
            std::cerr << "[gatewayd] ERROR: Encountered nullptr in events configuration!\n";
            return;
        }

        auto event_name = event->event_name()->string_view();
        const score_com_serializer* serializer = nullptr;
        auto get_result =
            score_com_serializer_get(service_type_name.data(), service_type_name.size(),
                                     score_com_serializer_element_type_event, event_name.data(),
                                     event_name.size(), &serializer);
        if (get_result != score_com_serializer_result_ok) {
            std::cerr << "[gatewayd] Failed to get serializer for " << service_type_name
                      << "::" << event_name << std::endl;
            return MakeUnexpected(score::mw::com::ComErrc::kInvalidConfiguration);
        }

        score::mw::com::DataTypeMetaInfo type_info{
            score_com_serializer_get_sizeof_type(serializer),
            score_com_serializer_get_alignof_type(serializer)};
        events.emplace_back(score::mw::com::EventInfo{event_name, type_info});
    }

    score::mw::com::GenericSkeletonServiceElementInfo service_element_info;
    service_element_info.events = events;

    // TODO: Error handling for instance specifier creation
    auto ipc_instance_specifier = score::mw::com::InstanceSpecifier::Create(
                                      service_instance_config->instance_specifier()->str())
                                      .value();

    auto create_ipc_result =
        score::mw::com::GenericSkeleton::Create(ipc_instance_specifier, service_element_info);
    if (!create_ipc_result.has_value()) {
        std::cerr << "[gatewayd] Failed to create IPC skeleton for '"
                  << service_instance_config->instance_specifier()->string_view()
                  << "': " << create_ipc_result.error().Message() << "\n";
        return;
    }
    auto ipc_skeleton = std::move(create_ipc_result).value();

    instances.push_back(std::make_unique<RemoteServiceInstance>(
        service_instance_config, service_type_config, std::move(ipc_skeleton), socom_runtime));

    std::cout << "[gatewayd] RemoteServiceInstance created for instance specifier: "
              << service_instance_config->instance_specifier()->string_view() << "\n";
}

}  // namespace score::someip_gateway::gatewayd
