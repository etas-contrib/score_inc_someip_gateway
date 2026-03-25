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

// =============================================================================
// Abstract Application Factory
//
// Layer 3: Provides a generic pure-virtual interface for creating
// application-specific serializers.
//
// Each application domain (e.g. CarWindow) defines its own abstract factory
// that declares the serializers it needs. The concrete factory (Layer 4)
// implements these by composing base-type serializers obtained from
// a BaseTypeSerializerFactory.
//
// This layer is INDEPENDENT of any concrete base-type implementation.
// It only depends on serializer_types.h for the ISerializer interface
// and on the application domain types.
// =============================================================================

#ifndef READER_ABSTRACT_APP_FACTORY_H
#define READER_ABSTRACT_APP_FACTORY_H

#include "serializer_types.h"
#include "examples/car_window_sim/src/car_window_types.h"

#include <memory>

namespace reader {

// =============================================================================
// AbstractCarWindowFactory
//
// Declares factory methods for every application-specific (struct/enum)
// serializer in the CarWindow domain.
//
// The concrete implementation shall compose these using ONLY base-type
// serializers from a BaseTypeSerializerFactory. Direct use of
// PrimitiveSerializer, SomeipStringSerializer, etc. is forbidden here.
// =============================================================================

class AbstractCarWindowFactory {
public:
    virtual ~AbstractCarWindowFactory() = default;

    /// Create a serializer for the WindowInfo struct.
    /// Wire layout: [pos:uint32] [state:WindowState(uint32)] — 8 bytes flat.
    virtual std::shared_ptr<ISerializer<car_window_types::WindowInfo>>
    create_window_info_serializer() const = 0;

    /// Create a serializer for the WindowControl struct.
    /// Wire layout: [command:WindowCommand(uint32)] — 4 bytes flat.
    virtual std::shared_ptr<ISerializer<car_window_types::WindowControl>>
    create_window_control_serializer() const = 0;
};

}  // namespace reader

#endif  // READER_ABSTRACT_APP_FACTORY_H
