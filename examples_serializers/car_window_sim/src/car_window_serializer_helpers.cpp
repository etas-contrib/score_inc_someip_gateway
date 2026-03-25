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
#include "car_window_serializer_helpers.h"

#include "src/reader/car_window_factory.h"

namespace car_window_types {

// -----------------------------------------------------------------------
// Singleton accessors — factory objects are stateless so we create them
// once and reuse across all calls.
// Uses the layered architecture:
//   Layer 2: SomeipBaseTypeSerializerFactory (base-type factory)
//   Layer 4: CarWindowFactory (application factory)
// -----------------------------------------------------------------------
namespace {

struct FactoryBundle {
    reader::SomeipBaseTypeSerializerFactory base;
    reader::CarWindowFactory app;

    FactoryBundle()
        : base(reader::kCarWindowSomeipSettings), app(base) {}
};

const FactoryBundle& factory_bundle() {
    static const FactoryBundle instance;
    return instance;
}

}  // namespace

std::vector<std::uint8_t> SerializeWindowInfo(const WindowInfo& info) {
    WindowInfo copy = info;
    reader::RefNode<WindowInfo> node(copy);
    auto ser = factory_bundle().app.create_window_info_serializer();
    return ser->serialize(node);
}

WindowInfo DeserializeWindowInfo(const std::vector<std::uint8_t>& bytes) {
    WindowInfo info{};
    info.state = WindowState::Stopped;
    reader::RefNode<WindowInfo> node(info);
    auto ser = factory_bundle().app.create_window_info_serializer();
    ser->deserialize(bytes, node);
    return info;
}

std::vector<std::uint8_t> SerializeWindowControl(const WindowControl& control) {
    WindowControl copy = control;
    reader::RefNode<WindowControl> node(copy);
    auto ser = factory_bundle().app.create_window_control_serializer();
    return ser->serialize(node);
}

WindowControl DeserializeWindowControl(const std::vector<std::uint8_t>& bytes) {
    WindowControl control{};
    control.command = WindowCommand::Stop;
    reader::RefNode<WindowControl> node(control);
    auto ser = factory_bundle().app.create_window_control_serializer();
    ser->deserialize(bytes, node);
    return control;
}

}  // namespace car_window_types
