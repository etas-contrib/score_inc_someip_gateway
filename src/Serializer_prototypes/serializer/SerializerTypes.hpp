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
// (c) 2023-2024 Robert Bosch GmbH, ETAS GmbH & DENSO Corporation (original)
//
// Adapted for the Eclipse SCORE inc_someip_gateway project.
// ara::core types replaced with C++ Standard Library equivalents.

#ifndef COM_SERIALIZER_SERIALIZERTYPES_HPP
#define COM_SERIALIZER_SERIALIZERTYPES_HPP

#include <cstdint>

namespace com
{
namespace serializer
{

/**
 * @brief Enum that defines byte orders.
 */
enum class ByteOrder : uint8_t
{
    kBigEndian,
    kLittleEndian,
    kOpaque
};

/**
 * @brief Structure that defines serialization settings.
 *
 * This structure will be created per deployment configuration.
 */
struct alignas(8) SerializerSettings
{
    ByteOrder byteOrder{};
    uint8_t sizeStringLengthField{};
    uint8_t sizeArrayLengthField{};
    uint8_t sizeVectorLengthField{};
    uint8_t sizeMapLengthField{};
    uint8_t sizeStructLengthField{};
    uint8_t sizeUnionLengthField{};
    bool isDynamicLengthFieldSize{};
};

/**
 * @brief Enumeration to specify the wire type.
 */
enum class EWireType : uint8_t
{
    E_WIRETYPE_0    = 0U, //!< wire type is equal to 0: 8 Bit Data Base data type
    E_WIRETYPE_1    = 1U, //!< wire type is equal to 1: 16 Bit Data Base data type
    E_WIRETYPE_2    = 2U, //!< wire type is equal to 2: 32 Bit Data Base data type
    E_WIRETYPE_3    = 3U, //!< wire type is equal to 3: 64 Bit Data Base data type
    E_WIRETYPE_4    = 4U, //!< wire type is equal to 4: Complex static Data Type
    E_WIRETYPE_5    = 5U, //!< wire type is equal to 5: Complex dynamic Data Type with length field size 1 byte
    E_WIRETYPE_6    = 6U, //!< wire type is equal to 6: Complex dynamic Data Type with length field size 2 byte
    E_WIRETYPE_7    = 7U, //!< wire type is equal to 7: Complex dynamic Data Type with length field size 4 byte
    E_WIRETYPE_NONE = 8U  //!< No valid wire type (reserved bit activated)
};

/**
 * @brief Represents an invalid TLV data ID.
 */
constexpr static uint16_t DEFAULT_TLV_DATA_ID{0xFFFFU};

/**
 * @brief Represents an invalid length field size.
 */
constexpr static uint8_t INVALID_LENGTH_FIELD_SIZE{0xFFU};

} // namespace serializer
} // namespace com
#endif // COM_SERIALIZER_SERIALIZERTYPES_HPP
