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
// SPDX-License-Identifier: Apache-2.0
// C-ABI boundary header for dlopen-based serializer plugins.
//
// This header defines the contract between the host and any .so plugin.
// It uses only C types to avoid C++ ABI issues across compilation units.
#ifndef SERIALIZER_PLUGIN_ABI_H
#define SERIALIZER_PLUGIN_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Plugin ABI version — host checks this to reject incompatible plugins.
#define SERIALIZER_PLUGIN_ABI_VERSION 1

/// Wire buffer exchanged across the ABI boundary.
struct PluginWireBuffer {
    uint8_t  data[128];
    uint32_t length;
};

/// Flat C message matching plugin::Message layout.
struct PluginMessage {
    uint32_t id;
    char     payload[64];
};

/// Every plugin .so must export these symbols:
///   uint32_t serializer_plugin_abi_version(void);
///   const char* serializer_plugin_name(void);
///   int serializer_plugin_serialize(const struct PluginMessage* msg, struct PluginWireBuffer* buf);
///   int serializer_plugin_deserialize(const struct PluginWireBuffer* buf, struct PluginMessage* msg);

typedef uint32_t    (*fn_abi_version)(void);
typedef const char* (*fn_name)(void);
typedef int         (*fn_serialize)(const struct PluginMessage*, struct PluginWireBuffer*);
typedef int         (*fn_deserialize)(const struct PluginWireBuffer*, struct PluginMessage*);

#ifdef __cplusplus
}
#endif

#endif  // SERIALIZER_PLUGIN_ABI_H
