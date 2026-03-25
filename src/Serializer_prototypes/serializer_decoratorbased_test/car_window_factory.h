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
// CarWindow Concrete Factory
//
// Layer 4: Implements the AbstractCarWindowFactory by composing
// application-specific serializers entirely from base-type serializers
// obtained through a BaseTypeSerializerFactory.
//
// This file also contains:
//   - ARXML-derived deployment constants (service IDs, event IDs, versions)
//   - ARXML-derived SerializerSettings presets (SOME/IP and IPC)
//   - Convenience factory constructors
//
// IMPORTANT: This factory does NOT directly instantiate PrimitiveSerializer,
// SomeipStringSerializer, etc. It ONLY uses the BaseTypeSerializerFactory
// interface to obtain base-type serializers. This ensures:
//   - All byte-order and wire-format logic is centralized in Layer 2
//   - The concrete base-type implementation can be swapped (e.g. for testing)
//   - The application factory is a pure composition of base types
//
// In a real code-gen pipeline, this file would be auto-generated from ARXML.
// =============================================================================

#ifndef READER_CAR_WINDOW_FACTORY_H
#define READER_CAR_WINDOW_FACTORY_H

#include "abstract_app_factory.h"
#include "base_type_serializers.h"
#include "serializer_types.h"
#include "examples/car_window_sim/src/car_window_types.h"
#include "src/serializer/SerializerTypes.hpp"

#include <cstdint>
#include <memory>

namespace reader {

// =============================================================================
// ARXML-derived deployment constants
// =============================================================================

/// From: SOMEIP-SERVICE-INTERFACE-DEPLOYMENT / WindowInfoInterface_SomeipDeployment
struct WindowInfoDeployment {
    static constexpr uint16_t kServiceId    = 6432U;
    static constexpr uint8_t  kMajorVersion = 1U;
    static constexpr uint8_t  kMinorVersion = 0U;

    struct Events {
        static constexpr uint16_t kWindowInfo = 1U;
    };
};

/// From: SOMEIP-SERVICE-INTERFACE-DEPLOYMENT / WindowControlInterface_SomeipDeployment
struct WindowControlDeployment {
    static constexpr uint16_t kServiceId    = 6433U;
    static constexpr uint8_t  kMajorVersion = 1U;
    static constexpr uint8_t  kMinorVersion = 0U;

    struct Events {
        static constexpr uint16_t kWindowControl = 2U;
    };
};

// =============================================================================
// ARXML-derived SerializerSettings presets
// =============================================================================

/// From: SOMEIP-TRANSFORMATION-PROPS / CarWindow_SomeipTransformationProps
///
///   BYTE-ORDER                   = MOST-SIGNIFICANT-BYTE-FIRST  → kBigEndian
///   SIZE-OF-STRING-LENGTH-FIELD  = 32                           → 4
///   SIZE-OF-ARRAY-LENGTH-FIELD   = 0                            → 0
///   SIZE-OF-STRUCT-LENGTH-FIELD  = 0                            → 0
///   SIZE-OF-UNION-LENGTH-FIELD   = 32                           → 4
///   IS-DYNAMIC-LENGTH-FIELD-SIZE = false                        → false
static constexpr com::serializer::SerializerSettings kCarWindowSomeipSettings{
    com::serializer::ByteOrder::kBigEndian,
    4U,     // sizeStringLengthField   (32 bits)
    0U,     // sizeArrayLengthField    (none)
    4U,     // sizeVectorLengthField   (32 bits)
    4U,     // sizeMapLengthField      (32 bits)
    0U,     // sizeStructLengthField   (none — flat concat)
    4U,     // sizeUnionLengthField    (32 bits)
    false   // isDynamicLengthFieldSize
};

/// IPC variant — same layout but opaque byte order (no swap on same-endian host)
static constexpr com::serializer::SerializerSettings kCarWindowIpcSettings{
    com::serializer::ByteOrder::kOpaque,
    4U, 0U, 4U, 4U, 0U, 4U, false
};

// =============================================================================
// CarWindowFactory — Concrete implementation of AbstractCarWindowFactory
//
// Composes struct serializers using CompoundSerializer<T> with member
// bindings. Each member's serializer is obtained from the injected
// BaseTypeSerializerFactory.
// =============================================================================

class CarWindowFactory : public AbstractCarWindowFactory {
public:
    /// @param base  Base-type serializer factory (typically SomeipBaseTypeSerializerFactory)
    explicit CarWindowFactory(const BaseTypeSerializerFactory& base)
        : base_(base) {}

    // --- Deployment info accessors (from ARXML service interface deployment) ---

    static constexpr uint16_t window_info_service_id()     { return WindowInfoDeployment::kServiceId; }
    static constexpr uint16_t window_info_event_id()       { return WindowInfoDeployment::Events::kWindowInfo; }
    static constexpr uint16_t window_control_service_id()  { return WindowControlDeployment::kServiceId; }
    static constexpr uint16_t window_control_event_id()    { return WindowControlDeployment::Events::kWindowControl; }

    // --- Application-specific serializer factories ---

    /// WindowInfo: { pos:uint32, state:WindowState(uint32) }
    /// Composed as CompoundSerializer with two member bindings.
    std::shared_ptr<ISerializer<car_window_types::WindowInfo>>
    create_window_info_serializer() const override {
        auto compound = std::make_shared<CompoundSerializer<car_window_types::WindowInfo>>();

        // Member 1: pos — uint32
        compound->add_member<uint32_t>(
            base_.create_uint32_serializer(),
            [](const car_window_types::WindowInfo& wi) -> uint32_t {
                return wi.pos;
            },
            [](car_window_types::WindowInfo& wi, const uint32_t& v) {
                wi.pos = v;
            }
        );

        // Member 2: state — WindowState (enum : uint32)
        compound->add_member<car_window_types::WindowState>(
            base_.create_enum_serializer<car_window_types::WindowState>(),
            [](const car_window_types::WindowInfo& wi) -> car_window_types::WindowState {
                return wi.state;
            },
            [](car_window_types::WindowInfo& wi, const car_window_types::WindowState& v) {
                wi.state = v;
            }
        );

        return compound;
    }

    /// WindowControl: { command:WindowCommand(uint32) }
    /// Composed as CompoundSerializer with one member binding.
    std::shared_ptr<ISerializer<car_window_types::WindowControl>>
    create_window_control_serializer() const override {
        auto compound = std::make_shared<CompoundSerializer<car_window_types::WindowControl>>();

        // Member 1: command — WindowCommand (enum : uint32)
        compound->add_member<car_window_types::WindowCommand>(
            base_.create_enum_serializer<car_window_types::WindowCommand>(),
            [](const car_window_types::WindowControl& wc) -> car_window_types::WindowCommand {
                return wc.command;
            },
            [](car_window_types::WindowControl& wc, const car_window_types::WindowCommand& v) {
                wc.command = v;
            }
        );

        return compound;
    }

private:
    const BaseTypeSerializerFactory& base_;
};

// =============================================================================
// Convenience factory constructors — named after deployment variants
// =============================================================================

/// Create a CarWindowFactory for the SOME/IP wire format (big-endian, per ARXML).
/// Returns both the base-type factory and the application factory.
struct CarWindowSomeipBundle {
    SomeipBaseTypeSerializerFactory base;
    CarWindowFactory app;

    CarWindowSomeipBundle()
        : base(kCarWindowSomeipSettings), app(base) {}
};

/// Create a CarWindowFactory for IPC (opaque byte order, no swap).
struct CarWindowIpcBundle {
    SomeipBaseTypeSerializerFactory base;
    CarWindowFactory app;

    CarWindowIpcBundle()
        : base(kCarWindowIpcSettings), app(base) {}
};

}  // namespace reader

#endif  // READER_CAR_WINDOW_FACTORY_H
