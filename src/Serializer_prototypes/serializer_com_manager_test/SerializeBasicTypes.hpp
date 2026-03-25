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
// (c) 2023-2024 ETAS GmbH & DENSO Corporation (original)
//
// Adapted for the Eclipse SCORE inc_someip_gateway project.
// ara::core types replaced with C++ Standard Library equivalents.

#ifndef COM_SERIALIZER_SERIALIZEBASICTYPES_HPP
#define COM_SERIALIZER_SERIALIZEBASICTYPES_HPP

#include "SerializerComputeSize.hpp"
#include "SerializerUtils.hpp"

namespace com
{
namespace serializer
{

/**
 * @brief Function that serializes a primitive type.
 *
 * @tparam T The type of the value to be serialized.
 * @param val The value to be serialized.
 * @param buffer Pointer to the buffer where the serialized data will be stored.
 * @param bufferSize The size of the buffer.
 * @param settings The serializer settings.
 * @return True if the serialization is successful, false otherwise.
 */
template<typename T, EnableIfBasic<T> = true>
static inline bool serialize(T val,
                             uint8_t* buffer,
                             const uint32_t bufferSize,
                             SerializerSettings settings,
                             const bool = false)
{
    constexpr uint32_t typeSize{static_cast<uint32_t>(sizeof(T))};

    if ((typeSize > bufferSize) || (buffer == nullptr))
    {
        return false;
    }

    if (typeSize == 1U)
    {
        *buffer = static_cast<uint8_t>(val);
    }
    else if (checkIfValueMustSwap(settings.byteOrder))
    {
        T swapedValue{swap(val)};
        static_cast<void>(memcpy(buffer, &swapedValue, typeSize));
    }
    else
    {
        static_cast<void>(memcpy(buffer, &val, typeSize));
    }
    return true;
}

/**
 * @brief Function that serializes a string type.
 *
 * @param data The string value to be serialized.
 * @param buffer Pointer to the buffer where the serialized data will be stored.
 * @param bufferSize The size of the buffer.
 * @param settings The serializer settings.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return True if the serialization is successful, false otherwise.
 */
static inline bool serialize(const std::string& data,
                             uint8_t buffer[],
                             const uint32_t bufferSize,
                             SerializerSettings settings,
                             const bool hasTlvTag = false)
{
    uint8_t lengthFieldSize{0U};
    uint32_t serializedSize{0U};
    uint32_t sizeWithoutLengthField{0U};

    if (buffer == nullptr)
    {
        return false;
    }

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, hasTlvTag);
    }
    else
    {
        serializedSize  = computeSerializedSize(data, settings);
        lengthFieldSize = settings.sizeStringLengthField;
    }

    if (bufferSize < serializedSize)
    {
        return false;
    }

    sizeWithoutLengthField = serializedSize - lengthFieldSize;

    if (!writeLengthField(lengthFieldSize, sizeWithoutLengthField, &buffer, settings.byteOrder))
    {
        return false;
    }

    // utf-8 BOM
    buffer[0U] = 239U;
    buffer[1U] = 187U;
    buffer[2U] = 191U;
    buffer     = &buffer[3U];

    static_cast<void>(memcpy(buffer, data.data(), data.size()));

    // Null terminator
    buffer[data.size()] = 0U;

    return true;
}

/**
 * @brief Function that serializes an array of primitive types.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The size of the array.
 * @param data The array to be serialized.
 * @param buffer Pointer to the buffer where the serialized data will be stored.
 * @param bufferSize The size of the buffer.
 * @param settings The serializer settings.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return True if the serialization is successful, false otherwise.
 */
template<typename T, size_t N, EnableIfBasic<T> = true>
static inline bool serialize(const std::array<T, N>& data,
                             uint8_t buffer[],
                             const uint32_t bufferSize,
                             SerializerSettings settings,
                             const bool hasTlvTag = false)
{
    uint8_t lengthFieldSize{0U};
    uint32_t serializedSize{0U};
    uint32_t sizeWithoutLengthField{0U};
    constexpr uint32_t typeSize = static_cast<uint32_t>(sizeof(T));

    if (buffer == nullptr)
    {
        return false;
    }

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, hasTlvTag);
    }
    else
    {
        serializedSize  = computeSerializedSize(data, settings);
        lengthFieldSize = settings.sizeArrayLengthField;
    }

    if (bufferSize < serializedSize)
    {
        return false;
    }

    sizeWithoutLengthField = serializedSize - lengthFieldSize;
    if (lengthFieldSize != 0U)
    {
        if (!writeLengthField(lengthFieldSize, sizeWithoutLengthField, &buffer, settings.byteOrder))
        {
            return false;
        }
    }

    if (typeSize == 1U)
    {
        static_cast<void>(memcpy(buffer, data.data(), N));
    }
    else
    {
        uint32_t bufferPos{0U};
        for (const auto& value : data)
        {
            static_cast<void>(serialize(value, &buffer[bufferPos], typeSize, settings));
            bufferPos += typeSize;
        }
    }
    return true;
}

/**
 * @brief Function that serializes a vector of primitive types with the exception of bool.
 *
 * @tparam T The type of the elements in the vector.
 * @param data The vector to be serialized.
 * @param buffer Pointer to the buffer where the serialized data will be stored.
 * @param bufferSize The size of the buffer.
 * @param settings The serializer settings.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return True if the serialization is successful, false otherwise.
 */
template<typename T, EnableIfBasicAndNotBool<T> = true>
static inline bool serialize(const std::vector<T>& data,
                             uint8_t buffer[],
                             const uint32_t bufferSize,
                             SerializerSettings settings,
                             const bool hasTlvTag = false)
{
    uint8_t lengthFieldSize{0U};
    uint32_t serializedSize{0U};
    uint32_t sizeWithoutLengthField{0U};
    constexpr uint32_t typeSize = static_cast<uint32_t>(sizeof(T));

    if (buffer == nullptr)
    {
        return false;
    }

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, hasTlvTag);
    }
    else
    {
        serializedSize  = computeSerializedSize(data, settings);
        lengthFieldSize = settings.sizeVectorLengthField;
    }

    if (bufferSize < serializedSize)
    {
        return false;
    }

    sizeWithoutLengthField = serializedSize - lengthFieldSize;
    if (lengthFieldSize != 0U)
    {
        if (!writeLengthField(lengthFieldSize, sizeWithoutLengthField, &buffer, settings.byteOrder))
        {
            return false;
        }
    }

    if (typeSize == 1U)
    {
        static_cast<void>(memcpy(buffer, data.data(), data.size()));
    }
    else
    {
        uint32_t bufferPos{0U};
        for (const auto& value : data)
        {
            static_cast<void>(serialize(value, &buffer[bufferPos], typeSize, settings));
            bufferPos += typeSize;
        }
    }
    return true;
}

/**
 * @brief Function that serializes a vector of bool type.
 */
static inline bool serialize(const std::vector<bool>& data,
                             uint8_t buffer[],
                             const uint32_t bufferSize,
                             SerializerSettings settings,
                             const bool hasTlvTag = false)
{
    uint8_t lengthFieldSize{0U};
    uint32_t serializedSize{0U};
    uint32_t sizeWithoutLengthField{0U};

    if (buffer == nullptr)
    {
        return false;
    }

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, hasTlvTag);
    }
    else
    {
        serializedSize  = computeSerializedSize(data, settings);
        lengthFieldSize = settings.sizeVectorLengthField;
    }

    if (bufferSize < serializedSize)
    {
        return false;
    }

    sizeWithoutLengthField = serializedSize - lengthFieldSize;
    if (lengthFieldSize != 0U)
    {
        if (!writeLengthField(lengthFieldSize, sizeWithoutLengthField, &buffer, settings.byteOrder))
        {
            return false;
        }
    }

    uint32_t bufferPos{0U};
    for (const auto& value : data)
    {
        buffer[bufferPos] = static_cast<uint8_t>(value);
        bufferPos += 1U;
    }
    return true;
}

/**
 * @brief Function that will serialize a map of primitive types.
 *
 * @tparam K The type of the keys in the map.
 * @tparam V The type of the values in the map.
 * @param data The map to be serialized.
 * @param buffer Pointer to the buffer where the serialized data will be stored.
 * @param bufferSize The size of the buffer.
 * @param settings The serializer settings.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return True if the serialization is successful, false otherwise.
 */
template<typename K, typename V, EnableIfBasic<K> = true, EnableIfBasic<V> = true>
static inline uint32_t serialize(const std::map<K, V>& data,
                                 uint8_t buffer[],
                                 const uint32_t bufferSize,
                                 SerializerSettings settings,
                                 const bool hasTlvTag = false)
{
    uint8_t lengthFieldSize{0U};
    uint32_t serializedSize{0U};
    uint32_t sizeWithoutLengthField{0U};
    constexpr uint32_t keyTypeSize   = static_cast<uint32_t>(sizeof(K));
    constexpr uint32_t valueTypeSize = static_cast<uint32_t>(sizeof(V));

    if (buffer == nullptr)
    {
        return false;
    }

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, hasTlvTag);
    }
    else
    {
        serializedSize  = computeSerializedSize(data, settings);
        lengthFieldSize = settings.sizeMapLengthField;
    }

    if (bufferSize < serializedSize)
    {
        return false;
    }

    sizeWithoutLengthField = serializedSize - lengthFieldSize;
    if (!writeLengthField(lengthFieldSize, sizeWithoutLengthField, &buffer, settings.byteOrder))
    {
        return false;
    }

    uint32_t bufferPos{0U};
    for (const auto& pair : data)
    {
        static_cast<void>(serialize(pair.first, &buffer[bufferPos], keyTypeSize, settings));
        bufferPos += keyTypeSize;
        static_cast<void>(serialize(pair.second, &buffer[bufferPos], valueTypeSize, settings));
        bufferPos += valueTypeSize;
    }
    return true;
}

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_SERIALIZEBASICTYPES_HPP
