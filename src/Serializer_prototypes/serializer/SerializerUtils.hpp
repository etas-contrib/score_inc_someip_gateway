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
// (c) 2022-2024 Robert Bosch GmbH, ETAS GmbH & DENSO Corporation (original)
//
// Adapted for the Eclipse SCORE inc_someip_gateway project.
// ara::core types replaced with C++ Standard Library equivalents.

#ifndef COM_SERIALIZER_SERIALIZERUTILS_HPP
#define COM_SERIALIZER_SERIALIZERUTILS_HPP

#include "SerializerTypes.hpp"

#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace com
{
namespace serializer
{

/**
 * @brief Expression that check if a type is either primitive or enum.
 */
template<typename T>
static constexpr bool isBasicType()
{
    return std::is_arithmetic<T>::value || std::is_enum<T>::value;
}

/**
 * @brief Expression that will enable a function if the type is basic.
 */
template<typename T>
using EnableIfBasic = std::enable_if_t<isBasicType<T>(), bool>;

/**
 * @brief Expression that will enable a function if the type is basic but its not bool.
 */
template<typename T>
using EnableIfBasicAndNotBool = std::enable_if_t<isBasicType<T>() && !std::is_same<T, bool>::value, bool>;

/**
 * @brief Expression that will enable a function if the type is not basic.
 */
template<typename T>
using EnableIfNotBasic = std::enable_if_t<!isBasicType<T>(), bool>;

/**
 * @brief Function that checks if a primitive type must be swapped because of different byteOrder.
 */
static bool checkIfValueMustSwap(ByteOrder configByteOrder)
{
    uint8_t isConfigLittle;

    if (configByteOrder == ByteOrder::kOpaque)
    {
        return false;
    }
    else if (configByteOrder == ByteOrder::kLittleEndian)
    {
        isConfigLittle = 1U;
    }
    else
    {
        isConfigLittle = 0U;
    }
    static const uint32_t x{1U};
    return (*reinterpret_cast<const uint8_t*>(&x) != isConfigLittle);
}

/**
 * @brief Set the data of src to the data of dst, in reverse order.
 */
static inline void setDataReverseOrder(uint8_t src[], uint8_t dst[], size_t length)
{
    for (size_t i = 0U; i < length; i++)
    {
        dst[i] = src[length - 1U - i];
    }
}

/**
 * @brief Swaps the bytes in a basic type. Basic types are: signed/unsigned ints, float, double and enums.
 */
template<typename T, EnableIfBasic<T> = true>
inline T swap(T val)
{
    constexpr size_t typeSize{sizeof(T)};
    uint8_t tmp[typeSize]{0U};
    setDataReverseOrder(reinterpret_cast<uint8_t*>(&val), tmp, typeSize);
    return *reinterpret_cast<T*>(tmp);
}

/**
 * @brief Function that will write the length value into the buffer.
 *
 * The buffer will be auto-incremented past the sizeLengthField if the function returns True. This function will return
 * False if the length field value does not fit in the sizeLengthField.
 */
static inline bool writeLengthField(uint8_t sizeLengthField,
                                    uint32_t length,
                                    uint8_t* buffer[],
                                    ByteOrder configByteOrder)
{
    if ((4U < sizeLengthField) || (length > (0xFFFFFFFFUL >> (32U - (sizeLengthField * 8U)))) || (buffer == nullptr))
    {
        return false;
    }

    if (sizeLengthField == 1U)
    {
        **buffer = static_cast<uint8_t>(length & 0xFFU);
    }
    else if (sizeLengthField == 2U)
    {
        uint16_t lengthAsUInt16{static_cast<uint16_t>(length & 0xFFFFU)};
        if (checkIfValueMustSwap(configByteOrder))
        {
            lengthAsUInt16 = swap(lengthAsUInt16);
        }
        static_cast<void>(memcpy(*buffer, &lengthAsUInt16, sizeof(uint16_t)));
    }
    else
    {
        if (checkIfValueMustSwap(configByteOrder))
        {
            length = swap(length);
        }
        static_cast<void>(memcpy(*buffer, &length, sizeof(uint32_t)));
    }

    *buffer = &(*buffer)[sizeLengthField];
    return true;
}

/**
 * @brief Function that will read the length value from the buffer.
 *
 * The buffer will be auto-incremented past the sizeLengthField.
 */
static inline void readLengthField(uint8_t sizeLengthField,
                                   uint32_t& length,
                                   const uint8_t* buffer[],
                                   ByteOrder configByteOrder)
{
    if (buffer == nullptr)
    {
        return;
    }

    if (sizeLengthField == 1U)
    {
        length = **buffer;
    }
    else if (sizeLengthField == 2U)
    {
        uint16_t lengthAsUInt16{0U};
        static_cast<void>(memcpy(&lengthAsUInt16, *buffer, sizeof(uint16_t)));
        if (checkIfValueMustSwap(configByteOrder))
        {
            lengthAsUInt16 = swap(lengthAsUInt16);
        }
        length = lengthAsUInt16;
    }
    else
    {
        static_cast<void>(memcpy(&length, *buffer, sizeof(uint32_t)));
        if (checkIfValueMustSwap(configByteOrder))
        {
            length = swap(length);
        }
    }
    *buffer = &(*buffer)[sizeLengthField];
}

/**
 * @brief Function that writes the TLV tag.
 *
 * @param buffer Pointer to the buffer where the tag will be written.
 * @param wireType The wire type of the tag.
 * @param dataId The data ID of the tag.
 * @return Returns true if the tag was successfully written, false otherwise.
 */
static inline bool writeTag(uint8_t* buffer, const uint8_t wireType, const uint16_t dataId)
{
    if (buffer == nullptr)
    {
        return false;
    }

    // Tag structure:
    // - Reserved: bit 7 of the first byte
    // - Wire type: bit 6-4 of the first byte
    // - Data ID: bit 3-0 of the first byte and bit 7-0 of the second byte
    *(buffer++) = static_cast<uint8_t>((0x00U | (wireType << 4) | (dataId >> 8)) & 0xFFU);
    *(buffer++) = static_cast<uint8_t>(dataId & 0xFFU);
    return true;
}

/**
 * @brief Function that reads the tag value from the buffer.
 *
 * @param buffer Pointer to the buffer where the tag will be read.
 * @param wireType The wire type of the tag.
 * @param dataId The data ID of the tag.
 * @param bufferSize Number of bytes allowed to read.
 * @param readBytes Number of read bytes.
 * @return Read tag status (true = success, false = failure).
 */
static inline bool readTag(const uint8_t* buffer,
                           uint8_t& wireType,
                           uint16_t& dataId,
                           const uint32_t bufferSize,
                           uint32_t& readBytes)
{
    if ((bufferSize < 2U) || (buffer == nullptr))
    {
        return false;
    }

    uint16_t tlvTag{0U};

    // Read the tag value from the buffer
    tlvTag = static_cast<uint16_t>((static_cast<uint16_t>(buffer[0]) << 8) | (buffer[1] & 0x00FFU));

    wireType = static_cast<uint8_t>(tlvTag >> 12);
    dataId   = static_cast<uint16_t>(tlvTag & 0X0FFFU);

    readBytes = 2U;

    return true;
}

/**
 * @brief Function that computes the size of the length field.
 *
 * @param length The length of the data.
 * @return The size of the length field.
 */
static constexpr uint8_t computeSizeOfLengthField(const uint32_t length)
{
    if (length == 0U)
    {
        return 0U;
    }
    else if (length <= std::numeric_limits<uint8_t>::max())
    {
        return 1U;
    }
    else if (length <= std::numeric_limits<uint16_t>::max())
    {
        return 2U;
    }
    else
    {
        return 4U;
    }
}

/**
 * @brief Function that computes the wire type based on the data types and a size length field.
 *
 * This is a specialization for complex types (arrays, classes, and std::string).
 *
 * @tparam T The data type.
 * @param settings The serializer settings.
 * @param lengthFieldSize The size length field.
 * @return The wire type.
 */
template<typename T, EnableIfNotBasic<T> = true>
uint8_t computeWireType(const SerializerSettings& settings, const uint8_t lengthFieldSize)
{
    if (settings.isDynamicLengthFieldSize && lengthFieldSize != 0U)
    {
        switch (lengthFieldSize)
        {
        case 1U:
            return 5U;
        case 2U:
            return 6U;
        default:
            return 7U;
        }
    }
    else
    {
        return 4U;
    }
}

/**
 * @brief Function that computes the wire type based on the data types and a size length field.
 *
 * This is a specialization for arithmetic and enum types.
 *
 * @tparam T The data type.
 * @param settings The serializer settings.
 * @param lengthFieldSize The size length field.
 * @return The wire type.
 */
template<typename T, EnableIfBasic<T> = true>
uint8_t computeWireType(const SerializerSettings&, const uint8_t)
{
    // Ensure T is of a size that is explicitly handled by the switch statement.
    static_assert(sizeof(T) == 1U || sizeof(T) == 2U || sizeof(T) == 4U || sizeof(T) == 8U,
                  "computeBasicWireType<T> called with T of unsupported size. T must be 1, 2, 4, or 8 bytes.");

    if (sizeof(T) == 1U)
    {
        return 0U;
    }
    else if (sizeof(T) == 2U)
    {
        return 1U;
    }
    else if (sizeof(T) == 4U)
    {
        return 2U;
    }
    else // sizeof(T) == 8U
    {
        return 3U;
    }
}

/**
 * @brief Function that computes the length field size based on the wire type.
 *
 * @param wireType The wire Type.
 * @return The length field size.
 *         - 1U: length field size 1 byte.
 *         - 2U: length field size 2 bytes.
 *         - 4U: length field size 4 bytes.
 *         - 0U: length field size 0 bytes. For complex data type, uses the statically configured length field size.
 *         - 0xFF: invalid value. Invalid wire type received.
 */
static inline uint8_t computeSizeOfLengthFieldBasedOnWireType(const uint8_t wireType)
{
    switch (static_cast<EWireType>(wireType))
    {
    case EWireType::E_WIRETYPE_0:
    case EWireType::E_WIRETYPE_1:
    case EWireType::E_WIRETYPE_2:
    case EWireType::E_WIRETYPE_3:
    case EWireType::E_WIRETYPE_4:
        return 0U;
    case EWireType::E_WIRETYPE_5:
        return 1U;
    case EWireType::E_WIRETYPE_6:
        return 2U;
    case EWireType::E_WIRETYPE_7:
        return 4U;
    case EWireType::E_WIRETYPE_NONE:
    default:
        return INVALID_LENGTH_FIELD_SIZE;
    }
}

/**
 * @brief Skips the bytes of an unknown member in a TLV deserialization context.
 *
 * @param buffer Pointer to the buffer from which data is to be skipped.
 * @param bufferSize Number of bytes allowed to skip.
 * @param wireType The wire type that determines the skip logic.
 * @param lengthFieldSize The size of the length field in bytes.
 * @param settings The deserializer settings, including byte order and array length field size.
 * @param skipBytes Output parameter that will be set to the total number of bytes skipped.
 * @return Returns true if bytes were successfully skipped, false otherwise.
 */
static inline bool skipUnknownMember(const uint8_t* buffer,
                                     const uint32_t bufferSize,
                                     const uint8_t wireType,
                                     const uint8_t lengthFieldSize,
                                     const SerializerSettings& settings,
                                     uint32_t& skipBytes)
{
    if ((bufferSize < lengthFieldSize) || (buffer == nullptr))
    {
        return false;
    }

    uint32_t lengthFieldOfUnknowDataId{0U};
    skipBytes = 0U;

    if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_0)
    {
        skipBytes = 1U;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_1)
    {
        skipBytes = 2U;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_2)
    {
        skipBytes = 4U;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_3)
    {
        skipBytes = 8U;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_4)
    {
        // In the context of TLV, Array, Struct, Variant, Map, and String all utilize the same size
        // for their length field. The buffer will be auto-incremented past the settings.sizeArrayLengthField.
        readLengthField(settings.sizeArrayLengthField, lengthFieldOfUnknowDataId, &buffer, settings.byteOrder);
        skipBytes = settings.sizeArrayLengthField + lengthFieldOfUnknowDataId;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_5)
    {
        if (1U != lengthFieldSize)
        {
            return false;
        }
        readLengthField(lengthFieldSize, lengthFieldOfUnknowDataId, &buffer, settings.byteOrder);
        skipBytes = lengthFieldSize + lengthFieldOfUnknowDataId;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_6)
    {
        if (2U != lengthFieldSize)
        {
            return false;
        }
        readLengthField(lengthFieldSize, lengthFieldOfUnknowDataId, &buffer, settings.byteOrder);
        skipBytes = lengthFieldSize + lengthFieldOfUnknowDataId;
    }
    else if (static_cast<EWireType>(wireType) == EWireType::E_WIRETYPE_7)
    {
        if (4U != lengthFieldSize)
        {
            return false;
        }
        readLengthField(lengthFieldSize, lengthFieldOfUnknowDataId, &buffer, settings.byteOrder);
        skipBytes = lengthFieldSize + lengthFieldOfUnknowDataId;
    }
    else // E_WIRETYPE_NONE
    {
        return false;
    }

    return true;
}

} // namespace serializer
} // namespace com
#endif // COM_SERIALIZER_SERIALIZERUTILS_HPP
