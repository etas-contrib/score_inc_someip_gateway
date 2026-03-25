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

#ifndef COM_SERIALIZER_DESERIALIZEBASICTYPES_HPP
#define COM_SERIALIZER_DESERIALIZEBASICTYPES_HPP

#include "SerializerComputeSize.hpp"
#include "SerializerUtils.hpp"

namespace com
{
namespace serializer
{

/**
 * @brief Function that will deserialize a primitive type.
 *
 * @tparam T The type of the value to be deserialized.
 * @param val The value to be deserialized.
 * @param buffer Pointer to the buffer where the serialized data is stored.
 * @param bufferSize Number of bytes to deserialize.
 * @param settings The deserializer settings.
 * @param readbytes The number of bytes read from the buffer if the deserialization is successful.
 * @param lengthFieldSize The size of the length field in bytes. For a primitive type, this is always 0.
 * @return True if the deserialization is successful, false otherwise.
 */
template<typename T, EnableIfBasic<T> = true>
static inline bool deserialize(T& val,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               const SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t = 0U)
{
    constexpr uint32_t typeSize = static_cast<uint32_t>(sizeof(T));

    if ((bufferSize < typeSize) || (buffer == nullptr))
    {
        return false;
    }

    if (typeSize == 1U)
    {
        val = static_cast<T>(*buffer);
    }
    else
    {
        static_cast<void>(memcpy(&val, buffer, typeSize));
        if (checkIfValueMustSwap(settings.byteOrder))
        {
            val = swap(val);
        }
    }
    readbytes = typeSize;
    return true;
}

/**
 * @brief Function that will deserialize a string type.
 *
 * @param data The string that will store the data.
 * @param buffer Pointer to the buffer where the serialized data is stored.
 * @param bufferSize Number of bytes to deserialize.
 * @param settings The deserializer settings.
 * @param readbytes The number of bytes read from the buffer if the deserialization is successful.
 * @param lengthFieldSize The size of the length field in bytes. If no value is passed, the static configured length
 * field size for strings is used.
 * @return True if the deserialization is successful, false otherwise.
 */
static inline bool deserialize(std::string& data,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t lengthFieldSize = 0U)
{
    if (buffer == nullptr)
    {
        return false;
    }

    if (0U == lengthFieldSize)
    {
        lengthFieldSize = settings.sizeStringLengthField;
    }

    // BOM 3 + Null terminator 1
    if ((4U < lengthFieldSize) || bufferSize < (lengthFieldSize + 3U + 1U))
    {
        return false;
    }

    uint32_t stringLength{0U};
    readLengthField(lengthFieldSize, stringLength, &buffer, settings.byteOrder);
    // Check if string length is longer than the length of BOM and Termination
    if (stringLength < 3U + 1U)
    {
        return false;
    }

    if ((buffer[0U] != 239U) || (buffer[1U] != 187U) || (buffer[2U] != 191U))
    {
        return false;
    }
    buffer = &buffer[3U];

    // Check if the buffer is large enough
    if (((std::numeric_limits<uint32_t>::max() - stringLength) < lengthFieldSize)
        || (bufferSize < (lengthFieldSize + stringLength)))
    {
        return false;
    }

    // check null-termination utf-8 (encoding-specific, one or two null-characters needed)
    if (buffer[stringLength - 3U - 1U] != 0U)
    {
        return false;
    }

    data.assign(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(stringLength) - 3U - 1U);
    readbytes = stringLength + lengthFieldSize;

    return true;
}

/**
 * @brief Function that will deserialize an array of primitive types.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The size of the array.
 * @param data The array that will store the data.
 * @param buffer Pointer to the buffer where the serialized data is stored.
 * @param bufferSize Number of bytes to deserialize.
 * @param settings The deserializer settings.
 * @param readbytes The number of bytes read from the buffer if the deserialization is successful.
 * @param lengthFieldSize The size of the length field in bytes. If no value is passed, the static configured length
 * field size for array is used.
 * @return True if the deserialization is successful, false otherwise.
 */
template<typename T, size_t N, EnableIfBasic<T> = true>
static inline bool deserialize(std::array<T, N>& data,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t lengthFieldSize = 0U)
{
    uint32_t serializedSize{0U};

    if (0U != lengthFieldSize)
    {
        // use dynamic length field size
        serializedSize = computeSerializedSize(data, settings, lengthFieldSize, true);
    }
    else
    {
        // use the statically configured length field size
        lengthFieldSize = settings.sizeArrayLengthField;
        serializedSize  = computeSerializedSize(data, settings);
    }

    constexpr uint32_t typeSize = static_cast<uint32_t>(sizeof(T));
    if ((bufferSize < serializedSize) || (buffer == nullptr))
    {
        return false;
    }

    if (lengthFieldSize != 0U)
    {
        uint32_t arrayLength{0U};
        readLengthField(lengthFieldSize, arrayLength, &buffer, settings.byteOrder);
        if (arrayLength != N * typeSize)
        {
            return false;
        }
    }

    if (1U == typeSize)
    {
        static_cast<void>(memcpy(data.data(), buffer, N));
    }
    else
    {
        uint32_t bufferPos{0U};
        for (uint32_t i = 0U; i < N; i++)
        {
            uint32_t bytesRead{0U};
            static_cast<void>(deserialize(data[i], &buffer[bufferPos], typeSize, settings, bytesRead));
            bufferPos += typeSize;
        }
    }

    readbytes = serializedSize;
    return true;
}

/**
 * @brief Function that will deserialize a vector of primitive types with the exception of bool.
 *
 * @tparam T The type of the elements in the vector.
 * @param data The vector that will store the data.
 * @param buffer Pointer to the buffer where the serialized data is stored.
 * @param bufferSize Number of bytes to deserialize.
 * @param settings The deserializer settings.
 * @param readbytes The number of bytes read from the buffer if the deserialization is successful.
 * @param lengthFieldSize The size of the length field in bytes. If no value is passed, the static configured length
 * field size for vector is used.
 * @return True if the deserialization is successful, false otherwise.
 */
template<typename T, EnableIfBasicAndNotBool<T> = true>
static inline bool deserialize(std::vector<T>& data,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t lengthFieldSize = 0U)
{
    constexpr uint32_t typeSize{static_cast<uint32_t>(sizeof(T))};
    uint32_t length{0U};

    if (0U == lengthFieldSize)
    {
        lengthFieldSize = settings.sizeVectorLengthField;
    }

    if ((bufferSize < lengthFieldSize) || (buffer == nullptr))
    {
        return false;
    }

    readLengthField(lengthFieldSize, length, &buffer, settings.byteOrder);
    if (bufferSize < lengthFieldSize + length)
    {
        return false;
    }

    if (typeSize == 1U)
    {
        data.resize(length);
        static_cast<void>(memcpy(data.data(), buffer, data.size()));
    }
    else
    {
        if (length % typeSize != 0U)
        {
            return false;
        }

        data.resize(static_cast<typename std::vector<T>::size_type>(length) / typeSize);

        auto it{data.begin()};

        uint32_t bufferPos{0U};

        while (it != data.end())
        {
            uint32_t bytesRead{0U};
            static_cast<void>(deserialize(*it, &buffer[bufferPos], typeSize, settings, bytesRead));
            bufferPos += typeSize;

            it++;
        }
    }

    readbytes = lengthFieldSize + length;
    return true;
}

/**
 * @brief Function that will deserialize a vector of bool type.
 */
static inline bool deserialize(std::vector<bool>& data,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t lengthFieldSize = 0U)
{
    uint32_t length{0U};

    if (0U == lengthFieldSize)
    {
        lengthFieldSize = settings.sizeVectorLengthField;
    }

    if ((bufferSize < lengthFieldSize) || (buffer == nullptr))
    {
        return false;
    }

    readLengthField(lengthFieldSize, length, &buffer, settings.byteOrder);
    if (bufferSize < lengthFieldSize + length)
    {
        return false;
    }

    data.resize(length);

    auto it{data.begin()};

    uint32_t bufferPos{0U};

    while (it != data.end())
    {
        *it = static_cast<bool>(buffer[bufferPos]);
        bufferPos += 1U;
        it++;
    }

    readbytes = lengthFieldSize + length;
    return true;
}

/**
 * @brief Function that will deserialize a map of primitive types.
 *
 * @tparam K The type of the keys in the map.
 * @tparam V The type of the values in the map.
 * @param data The map that will store the data.
 * @param buffer Pointer to the buffer where the serialized data is stored.
 * @param bufferSize Number of bytes to deserialize.
 * @param settings The deserializer settings.
 * @param readbytes The number of bytes read from the buffer if the deserialization is successful.
 * @param lengthFieldSize The size of the length field in bytes. If no value is passed, the static configured length
 * field size for map is used.
 * @return True if the deserialization is successful, false otherwise.
 */
template<typename K, typename V, EnableIfBasic<K> = true, EnableIfBasic<V> = true>
static inline bool deserialize(std::map<K, V>& data,
                               const uint8_t buffer[],
                               const uint32_t bufferSize,
                               SerializerSettings settings,
                               uint32_t& readbytes,
                               uint8_t lengthFieldSize = 0U)
{
    constexpr uint32_t keyTypeSize{static_cast<uint32_t>(sizeof(K))};
    constexpr uint32_t valueTypeSize{static_cast<uint32_t>(sizeof(V))};
    uint32_t length{0U};

    if (0U == lengthFieldSize)
    {
        lengthFieldSize = settings.sizeMapLengthField;
    }

    if ((bufferSize < lengthFieldSize) || (buffer == nullptr))
    {
        return false;
    }

    readLengthField(lengthFieldSize, length, &buffer, settings.byteOrder);
    if (bufferSize < lengthFieldSize + length)
    {
        return false;
    }

    if (length % (keyTypeSize + valueTypeSize) != 0U)
    {
        return false;
    }

    uint32_t numPairs{length / (keyTypeSize + valueTypeSize)};
    uint32_t bufferPos{0U};
    while (numPairs > 0U)
    {
        uint32_t bytesRead{0U};
        K key{};
        V value{};
        static_cast<void>(deserialize(key, &buffer[bufferPos], keyTypeSize, settings, bytesRead));
        bufferPos += keyTypeSize;
        static_cast<void>(deserialize(value, &buffer[bufferPos], valueTypeSize, settings, bytesRead));
        bufferPos += valueTypeSize;
        data[key] = value;
        numPairs--;
    }
    readbytes = lengthFieldSize + length;
    return true;
}

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_DESERIALIZEBASICTYPES_HPP
