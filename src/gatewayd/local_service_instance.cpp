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

#include "local_service_instance.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>

#include "score/mw/com/com_error_domain.h"
#include "score/mw/com/types.h"
#include "score/socom/runtime.hpp"
#include "score/someip/constants.h"
#include "src/serializer/serializer.h"

using score::mw::com::GenericProxy;
using score::mw::com::SamplePtr;

namespace score::someip_gateway::gatewayd {

LocalServiceInstance::LocalServiceInstance(
    std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config,
    std::shared_ptr<const mw_someip_config::ServiceType> service_type_config,
    GenericProxy&& ipc_proxy, socom::Runtime& socom_runtime)
    : service_instance_config_(std::move(service_instance_config)),
      service_type_config_(std::move(service_type_config)),
      ipc_proxy_(std::move(ipc_proxy)),
      server_connector_(nullptr) {
    socom::Service_interface_identifier const iface{
        service_type_config_->service_type_name()->string_view(),
        {service_type_config_->service_version_major(),
         static_cast<uint16_t>(service_type_config_->service_version_minor())}};

    // TODO: Handle multiple instances. Needs to be converted from integer ID to string.
    // For initial impl, just use service name again.
    socom::Service_instance const inst{service_type_config_->service_type_name()->string_view()};

    socom::Server_service_interface_definition const server_config{
        iface, socom::to_num_of_methods(0),
        socom::to_num_of_events(service_type_config_->events()->size())};

    auto disabled_server_connector = socom_runtime.make_server_connector(
        server_config, inst,
        {
            .on_method_call = [](socom::Enabled_server_connector&, socom::Method_id, socom::Payload,
                                 socom::Method_call_reply_data_opt, socom::Posix_credentials const&)
                -> socom::Method_invocation::Uptr { return nullptr; },
            .on_event_subscription_change = [](socom::Enabled_server_connector&, socom::Event_id,
                                               socom::Event_state) {},
            .on_event_update_request = [](socom::Enabled_server_connector&, socom::Event_id) {},
            .on_method_call_payload_allocate =
                [](socom::Enabled_server_connector&,
                   socom::Method_id) -> score::Result<socom::Writable_payload> {
                return MakeUnexpected(socom::Error::logic_error_id_out_of_range);
            },
        });

    if (!disabled_server_connector.has_value()) {
        std::cerr << "[gatewayd] Failed to create server connector for '"
                  << service_type_config_->service_type_name()->string_view() << "'\n";
        return;
    }
    std::cout << "[gatewayd] LocalServiceInstance - Enabled server_connector for "
              << service_type_config_->service_type_name()->string_view() << std::endl;
    server_connector_ =
        socom::Disabled_server_connector::enable(std::move(disabled_server_connector).value());

    // Set up IPC event handlers
    auto& events = ipc_proxy_.GetEvents();

    socom::Event_id socom_event_id{0U};
    for (auto event_config : *service_type_config_->events()) {
        auto result = events.find(event_config->event_name()->string_view());
        if (result == events.cend()) {
            std::cerr << "[gatewayd] Failed to find " << event_config->event_name()->string_view()
                      << " event in ipc_proxy." << std::endl;
            ++socom_event_id;
            continue;
        }
        auto& ipc_event = result->second;

        auto service_type_name = service_type_config_->service_type_name()->string_view();
        auto event_name = event_config->event_name()->string_view();
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
        auto& event_context =
            event_contexts_.emplace(event_name, EventContext{event_config, serializer})
                .first->second;

        ipc_event.SetReceiveHandler([this, &ipc_event, &event_context]() {
            ipc_event.GetNewSamples(
                [&](SamplePtr<void> sample) {
                    std::cout << "[gatewayd] LocalServiceInstance - Calling GetNewSamples()"
                              << std::endl;
                    auto maybe_payload = server_connector_->allocate_event_payload(socom_event_id);
                    if (!maybe_payload.has_value()) {
                        std::cout << "[gatewayd] LocalServiceInstance - Failed to allocate event "
                                     "payload for event "
                                  << socom_event_id << ": " << maybe_payload.error().Message()
                                  << std::endl;
                        return;
                    }
                    auto& payload = *maybe_payload;
                    auto const copy_size =
                        std::min(payload.wdata().size(), ipc_event.GetSampleSize());
                    std::memcpy(payload.wdata().data(), sample.get(), copy_size);
                    std::cout << "[gatewayd] LocalServiceInstance - "
                                 "server_connector_->update_event for event_id "
                              << socom_event_id << " with payload size " << copy_size << std::endl;
                    server_connector_->update_event(socom_event_id, std::move(payload));
                },
                someip::kMaxSampleCount);
                someip::kMaxSampleCount);
        });

        ipc_event.Subscribe(someip::kMaxSampleCount);
        ++socom_event_id;
    }
}

namespace {
struct FindServiceContext {
    std::shared_ptr<const mw_someip_config::ServiceInstance> config;
    std::shared_ptr<const mw_someip_config::ServiceType> service_config;
    socom::Runtime* socom_runtime;
    std::vector<std::unique_ptr<LocalServiceInstance>>& instances;

    FindServiceContext(std::shared_ptr<const mw_someip_config::ServiceInstance> config_,
                       std::shared_ptr<const mw_someip_config::ServiceType> service_config_,
                       socom::Runtime& socom_runtime_,
                       std::vector<std::unique_ptr<LocalServiceInstance>>& instances_)
        : config(std::move(config_)),
          service_config(std::move(service_config_)),
          socom_runtime(&socom_runtime_),
          instances(instances_) {}
};

}  // namespace

Result<mw::com::FindServiceHandle> LocalServiceInstance::CreateAsyncLocalServices(
    std::shared_ptr<const mw_someip_config::ServiceInstance> service_instance_config,
    std::shared_ptr<const mw_someip_config::ServiceType> service_type_config,
    socom::Runtime& socom_runtime, std::vector<std::unique_ptr<LocalServiceInstance>>& instances) {
    if (service_instance_config == nullptr) {
        std::cerr << "[gatewayd] ERROR: Service instance config is nullptr!" << std::endl;
        return MakeUnexpected(score::mw::com::ComErrc::kInvalidConfiguration);
    }

    // TODO: Error handling for instance specifier creation
    auto instance_specifier = score::mw::com::InstanceSpecifier::Create(
                                  service_instance_config->instance_specifier()->str())
                                  .value();

    // TODO: StartFindService should be modified to handle arbitrarily large lambdas
    // or we need to check whether it is OK to stick with dynamic allocation here.
    auto context = std::make_unique<FindServiceContext>(
        service_instance_config, service_type_config, socom_runtime, instances);

    return GenericProxy::StartFindService(
        [context = std::move(context)](auto handles, auto find_handle) {
            auto& instance_config = context->config;
            auto& service_config = context->service_config;

            auto proxy_result = GenericProxy::Create(handles.front());
            if (!proxy_result.has_value()) {
                std::cerr << "[gatewayd] Proxy creation failed for instance specifier: "
                          << instance_config->instance_specifier()->string_view()
                          << "': " << proxy_result.error().Message() << std::endl;
                return;
            }

            // TODO: Add mutex if callbacks can run concurrently or use futures
            context->instances.push_back(std::make_unique<LocalServiceInstance>(
                instance_config, service_config, std::move(proxy_result).value(),
                *context->socom_runtime));

            std::cout << "[gatewayd] LocalServiceInstance created for instance specifier: "
                      << instance_config->instance_specifier()->string_view() << std::endl;

            GenericProxy::StopFindService(find_handle);
        },
        instance_specifier);
}

}  // namespace score::someip_gateway::gatewayd
