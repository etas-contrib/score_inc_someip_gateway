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

/// @file
/// This file provides a "serializer" which actually doesn't serialize and just copies the memory.

#include <cstddef>
#include <cstring>

#include "src/serializer/pre_serialized_data.h"
#include "src/serializer/serializer.h"

using score::someip_gateway::serializer::PreSerializedData;

namespace {

constexpr std::size_t MAX_MESSAGE_SIZE = 1500U;  // TODO: Make configurable

score_com_serializer_result serialize(uint8_t* buffer, const size_t buffer_size, const void* object,
                                      size_t* written_bytes) {
    if (buffer == nullptr || object == nullptr) {
        return score_com_serializer_result_general_failure;
    }
    const auto* pre_serialized_data = static_cast<const PreSerializedData<0>*>(object);
    std::size_t message_size = pre_serialized_data->size;
    if (message_size > buffer_size) {
        return score_com_serializer_result_serialization_failure;
    }
    std::memcpy(buffer, pre_serialized_data->data, message_size);
    if (written_bytes != nullptr) {
        *written_bytes = message_size;
    }
    return score_com_serializer_result_ok;
}

score_com_serializer_result deserialize(const uint8_t* buffer, size_t buffer_size, void* object) {
    if (buffer == nullptr || object == nullptr) {
        return score_com_serializer_result_general_failure;
    }
    auto* pre_serialized_data = static_cast<PreSerializedData<0>*>(object);
    if (buffer_size > MAX_MESSAGE_SIZE) {
        return score_com_serializer_result_deserialization_failure;
    }
    std::memcpy(pre_serialized_data->data, buffer, buffer_size);
    pre_serialized_data->size = buffer_size;
    return score_com_serializer_result_ok;
}

};  // anonymous namespace

score_com_serializer_result score_com_serializer_init(const char* serializer_identifier,
                                                      size_t serializer_identifier_size) {
    return score_com_serializer_result_ok;
}

score_com_serializer_result score_com_serializer_deinit() { return score_com_serializer_result_ok; }

score_com_serializer_result score_com_serializer_get(
    const char* service_type, size_t service_type_size,
    enum score_com_serializer_element_type element_type, const char* element_name,
    size_t element_name_size, struct score_com_serializer* serializer) {
    if (serializer == nullptr) {
        return score_com_serializer_result_general_failure;
    }
    serializer->serialize = serialize;
    serializer->deserialize = deserialize;
    serializer->max_serialized_size = MAX_MESSAGE_SIZE;
    serializer->sizeof_object = sizeof(PreSerializedData<MAX_MESSAGE_SIZE>);
    static_assert(
        alignof(PreSerializedData<0>) == alignof(std::max_align_t),
        "We assume that the alignment of PreSerializedData is the maximum alignment required by "
        "any type, so it should be safe to set the serializer's alignof_object to that value.");
    serializer->alignof_object = alignof(PreSerializedData<0>);

    return score_com_serializer_result_ok;
}
