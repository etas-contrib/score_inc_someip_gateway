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
// Concrete plugin shared library — Big-Endian serializer.
// Compiled as a .so and loaded by the host via dlopen.
#include "examples_serializers/serializer_plugin_dlopen/plugin_abi.h"
#include <string.h>

extern "C" {

uint32_t serializer_plugin_abi_version(void) {
    return SERIALIZER_PLUGIN_ABI_VERSION;
}

const char* serializer_plugin_name(void) {
    return "DlopenBigEndianPlugin";
}

int serializer_plugin_serialize(const struct PluginMessage* msg,
                                struct PluginWireBuffer* buf) {
    if (!msg || !buf) return -1;
    buf->data[0] = (uint8_t)((msg->id >> 24) & 0xFF);
    buf->data[1] = (uint8_t)((msg->id >> 16) & 0xFF);
    buf->data[2] = (uint8_t)((msg->id >>  8) & 0xFF);
    buf->data[3] = (uint8_t)((msg->id >>  0) & 0xFF);
    memcpy(&buf->data[4], msg->payload, sizeof(msg->payload));
    buf->length = 4 + (uint32_t)sizeof(msg->payload);
    return 0;
}

int serializer_plugin_deserialize(const struct PluginWireBuffer* buf,
                                  struct PluginMessage* msg) {
    if (!buf || !msg) return -1;
    if (buf->length < 4 + sizeof(msg->payload)) return -1;
    msg->id = ((uint32_t)buf->data[0] << 24)
            | ((uint32_t)buf->data[1] << 16)
            | ((uint32_t)buf->data[2] <<  8)
            | ((uint32_t)buf->data[3] <<  0);
    memcpy(msg->payload, &buf->data[4], sizeof(msg->payload));
    return 0;
}

}  // extern "C"
