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
// *********************************************************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the terms of the Apache License Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *********************************************************************************************************************
//
// Based on the SOME/IP serializer from com-aap-communication-manager
// (c) 2023-2024 DENSO Corporation & Robert Bosch GmbH (original)
//
// Adapted for the Eclipse SCORE inc_someip_gateway project.

#ifndef COM_SERIALIZER_APPERRORSERIALIZERS_HPP
#define COM_SERIALIZER_APPERRORSERIALIZERS_HPP

#include "DeserializeBasicTypes.hpp"
#include "SerializeBasicTypes.hpp"

namespace com
{
namespace serializer
{

struct AppError
{
    uint64_t domain;
    int32_t code;
};

// SWS_CM_EXT_10428_1
static constexpr uint8_t sizeOfUnionLengthField{4U};
static constexpr uint8_t sizeOfUnionTypeSelectorField{1U};
static constexpr uint8_t sizeOfStructLengthField{2U};

static constexpr uint32_t domainSize{static_cast<uint32_t>(sizeof(uint64_t))};
static constexpr uint32_t codeSize{static_cast<uint32_t>(sizeof(int32_t))};
static constexpr uint32_t structLengthFieldValue{domainSize + codeSize};

static constexpr uint32_t unionLengthFieldValue{structLengthFieldValue + sizeOfStructLengthField};
static constexpr uint32_t unionTypeSelectorValue{1U};

static constexpr uint32_t minBufferSize{unionLengthFieldValue + sizeOfUnionLengthField + sizeOfUnionTypeSelectorField};
static constexpr ByteOrder appErrorByteOrder{ByteOrder::kBigEndian};

static constexpr SerializerSettings
    appErrorSettings{appErrorByteOrder, 2U, 4U, 4U, 4U, sizeOfStructLengthField, sizeOfUnionLengthField};

/**
 * @brief Function used to serialize app errors.
 *
 * Returns false in case the provided buffer is too small.
 */
inline bool appErrorSerialize(AppError appError, uint8_t buffer[], const uint32_t bufferSize)
{
    if ((bufferSize < minBufferSize) || (buffer == nullptr))
    {
        return false;
    }

    // write union length field
    static_cast<void>(writeLengthField(sizeOfUnionLengthField, unionLengthFieldValue, &buffer, appErrorByteOrder));
    // write union type selector
    *buffer = unionTypeSelectorValue;
    buffer  = &buffer[1U];
    // write struct length field
    static_cast<void>(writeLengthField(sizeOfStructLengthField, structLengthFieldValue, &buffer, appErrorByteOrder));
    // serialize domain
    static_cast<void>(serialize(appError.domain, buffer, domainSize, appErrorSettings));
    buffer = &buffer[domainSize];
    // serialize code
    static_cast<void>(serialize(appError.code, buffer, codeSize, appErrorSettings));
    return true;
}

/**
 * @brief Function used to deserialize app errors.
 *
 * Returns false in case the provided buffer is too small, or if the fields do not have the correct values.
 */
inline bool appErrorDeserialize(AppError& appError,
                                const uint8_t buffer[],
                                const uint32_t bufferSize,
                                uint32_t& readbytes)
{
    if ((bufferSize < minBufferSize) || (buffer == nullptr))
    {
        return false;
    }
    // read union length value
    uint32_t readUnionLengthValue{};
    static_cast<void>(readLengthField(sizeOfUnionLengthField, readUnionLengthValue, &buffer, appErrorByteOrder));
    if (readUnionLengthValue != unionLengthFieldValue)
    {
        return false;
    }
    // read union type selector value
    uint32_t readUnionSelectorValue{*buffer};
    if (readUnionSelectorValue != unionTypeSelectorValue)
    {
        return false;
    }
    buffer = &buffer[1U];
    // read struct length value
    uint32_t readStructLengthValue{};
    static_cast<void>(readLengthField(sizeOfStructLengthField, readStructLengthValue, &buffer, appErrorByteOrder));
    if (readStructLengthValue != structLengthFieldValue)
    {
        return false;
    }
    // read domain
    static_cast<void>(deserialize(appError.domain, buffer, domainSize, appErrorSettings, readbytes));
    buffer = &buffer[domainSize];
    // read code
    static_cast<void>(deserialize(appError.code, buffer, codeSize, appErrorSettings, readbytes));

    // override readbytes
    readbytes = minBufferSize;

    return true;
}

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_APPERRORSERIALIZERS_HPP
