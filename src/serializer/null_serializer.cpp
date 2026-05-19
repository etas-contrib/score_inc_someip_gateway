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
#include <list>

#include "src/serializer/pre_serialized_data.h"
#include "src/serializer/serializer.h"

using score::someip_gateway::serializer::get_size_of_pre_serialized_data;
using score::someip_gateway::serializer::PreSerializedData;

struct score_com_serializer {
    std::size_t max_serialized_size;
};

namespace {

// TODO: This should probably be a map or directly pointer into the flatbuffer config.
std::list<score_com_serializer>& get_serializers() {
    static std::list<score_com_serializer> serializers;
    return serializers;
}

};  // anonymous namespace

score_com_serializer_result score_com_serializer_serialize(const struct score_com_serializer*,
                                                           uint8_t* buffer, size_t buffer_size,
                                                           const void* object,
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

score_com_serializer_result score_com_serializer_deserialize(
    const struct score_com_serializer* serializer, const uint8_t* buffer, size_t buffer_size,
    void* object) {
    if (serializer == nullptr || buffer == nullptr || object == nullptr) {
        return score_com_serializer_result_general_failure;
    }
    auto* pre_serialized_data = static_cast<PreSerializedData<0>*>(object);
    if (buffer_size > serializer->max_serialized_size) {
        return score_com_serializer_result_deserialization_failure;
    }
    std::memcpy(pre_serialized_data->data, buffer, buffer_size);
    pre_serialized_data->size = buffer_size;
    return score_com_serializer_result_ok;
}

std::size_t score_com_serializer_get_max_serialized_size(
    const struct score_com_serializer* serializer) {
    if (serializer == nullptr) {
        return 0;
    }
    return serializer->max_serialized_size;
}

std::size_t score_com_serializer_get_sizeof_object(const struct score_com_serializer* serializer) {
    if (serializer == nullptr) {
        return 0;
    }
    return get_size_of_pre_serialized_data(serializer->max_serialized_size);
}

std::size_t score_com_serializer_get_alignof_object(const struct score_com_serializer*) {
    return alignof(PreSerializedData<0>);
}

score_com_serializer_result score_com_serializer_init(const char* serializer_identifier,
                                                      size_t serializer_identifier_size) {
    get_serializers();
    return score_com_serializer_result_ok;
}

score_com_serializer_result score_com_serializer_deinit() {
    get_serializers().clear();
    return score_com_serializer_result_ok;
}

score_com_serializer_result score_com_serializer_get(
    const char* service_type, size_t service_type_size,
    enum score_com_serializer_element_type element_type, const char* element_name,
    size_t element_name_size, struct score_com_serializer** serializer) {
    if (serializer == nullptr) {
        return score_com_serializer_result_general_failure;
    }

    constexpr std::size_t MAX_MESSAGE_SIZE = 1500;  // TODO: Make configurable
    static_assert(sizeof(PreSerializedData<MAX_MESSAGE_SIZE>) ==
                      get_size_of_pre_serialized_data(MAX_MESSAGE_SIZE),
                  "Size of PreSerializedData does not match expected value.");
    struct score_com_serializer new_serializer {
        // TODO: Get this from config
        .max_serialized_size = MAX_MESSAGE_SIZE,
    };
    *serializer = &get_serializers().emplace_back(std::move(new_serializer));

    return score_com_serializer_result_ok;
}
