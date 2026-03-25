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

#ifndef COM_SERIALIZER_SERIALIZERCOMPUTESIZE_HPP
#define COM_SERIALIZER_SERIALIZERCOMPUTESIZE_HPP

#include "SerializerUtils.hpp"

namespace com
{
namespace serializer
{

/**
 * @brief Function that will return the serialized size of primitive types.
 */
template<typename T, EnableIfBasic<T> = true>
static constexpr uint32_t computeSerializedSize(T, SerializerSettings)
{
    return static_cast<uint32_t>(sizeof(T));
}

/**
 * @brief Function that will return the serialized size of string type.
 */
static inline uint32_t computeSerializedSize(const std::string& data, SerializerSettings settings)
{
    // +1U because of the null terminator
    // +3U because of the utf-8 BOM
    size_t dataSize{data.size()};
    if ((std::numeric_limits<uint32_t>::max() < (dataSize + 1U + 3U + settings.sizeStringLengthField)))
    {
        return 0U;
    }
    return static_cast<uint32_t>(dataSize) + 1U + 3U + settings.sizeStringLengthField;
}

/**
 * @brief Function that will return the serialized size of an array of primitive types.
 */
template<typename T, size_t N, EnableIfBasic<T> = true>
static inline uint32_t computeSerializedSize(const std::array<T, N>& arr, SerializerSettings settings)
{
    if (std::numeric_limits<uint32_t>::max() < (arr.size() * sizeof(T) + settings.sizeArrayLengthField))
    {
        return 0U;
    }
    return static_cast<uint32_t>(arr.size() * sizeof(T) + settings.sizeArrayLengthField);
}

/**
 * @brief Function that will return the serialized size of a vector of primitive types.
 */
template<typename T, EnableIfBasic<T> = true>
static inline uint32_t computeSerializedSize(const std::vector<T>& data, SerializerSettings settings)
{
    if ((static_cast<size_t>(std::numeric_limits<uint32_t>::max()) - static_cast<size_t>(settings.sizeVectorLengthField))
            / sizeof(T)
        < data.size())
    {
        return 0U;
    }
    return static_cast<uint32_t>(data.size() * sizeof(T) + settings.sizeVectorLengthField);
}

/**
 * @brief Function that will return the serialized size of a map of primitive types.
 */
template<typename K, typename V, EnableIfBasic<K> = true, EnableIfBasic<V> = true>
static inline uint32_t computeSerializedSize(const std::map<K, V>& data, SerializerSettings settings)
{
    if ((static_cast<size_t>(std::numeric_limits<uint32_t>::max()) - static_cast<size_t>(settings.sizeMapLengthField))
            / (sizeof(K) + sizeof(V))
        < data.size())
    {
        return 0U;
    }

    return static_cast<uint32_t>(data.size() * (sizeof(K) + sizeof(V)) + settings.sizeMapLengthField);
}

/**
 * NOTE:
 * The following functions were introduced to make supporting the TLV feature
 * easier and more convenient, while avoiding introducing too many changes to the original codebase.
 * These functions can replace the above ones in the future, if required.
 */

/**
 * @brief Computes the serialized size of an integral or a floating-point type.
 *
 * @tparam T The type for which the serialized size is computed.
 * @param lengthFieldSize The length field size, which for an integral or a floating-point type is always zero.
 * @return The computed serialized size of the value.
 */
template<typename T, EnableIfBasic<T> = true>
static constexpr uint32_t computeSerializedSize(T,
                                                const SerializerSettings&,
                                                uint8_t& lengthFieldSize,
                                                const bool = false)
{
    lengthFieldSize = 0U;
    return static_cast<uint32_t>(sizeof(T));
}

/**
 * @brief Function that computes the serialized size of a string type.
 *
 * @param data The string data to compute the serialized size for.
 * @param settings The serializer settings.
 * @param lengthFieldSize The length field size, which is set based on the serialized size.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return The computed serialized size of the string.
 */
static inline uint32_t computeSerializedSize(const std::string& data,
                                             const SerializerSettings& settings,
                                             uint8_t& lengthFieldSize,
                                             const bool hasTlvTag = false)
{
    lengthFieldSize = 0U;
    std::size_t dataSize{3U + data.size() + 1U}; // UTF-8 BOM, data size, null terminator

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        if (dataSize <= 0xFFU)
        {
            lengthFieldSize = 1U;
        }
        else if (dataSize <= 0xFFFFU)
        {
            lengthFieldSize = 2U;
        }
        else
        {
            lengthFieldSize = 4U;
        }
    }
    else
    {
        lengthFieldSize = settings.sizeStringLengthField;
    }

    std::size_t serializedSize{dataSize + lengthFieldSize};

    // Return 0 if the serialized size exceeds the maximum size limit
    if (std::numeric_limits<uint32_t>::max() < serializedSize)
    {
        return 0U;
    }

    return static_cast<uint32_t>(serializedSize);
}

/**
 * @brief Function that computes the serialized size of an array of primitive types.
 *
 * @tparam T The type of the elements in the array.
 * @tparam N The size of the array.
 * @param data The array to compute the serialized size for.
 * @param settings The serializer settings.
 * @param lengthFieldSize The length field size, which is set based on the serialized size.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return The computed serialized size of the array.
 */
template<typename T, size_t N, EnableIfBasic<T> = true>
static inline uint32_t computeSerializedSize(const std::array<T, N>& arr,
                                             const SerializerSettings& settings,
                                             uint8_t& lengthFieldSize,
                                             const bool hasTlvTag = false)
{
    lengthFieldSize = 0U;
    std::size_t dataSize{arr.size() * sizeof(T)};

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        if (dataSize <= 0xFFU)
        {
            lengthFieldSize = 1U;
        }
        else if (dataSize <= 0xFFFFU)
        {
            lengthFieldSize = 2U;
        }
        else
        {
            lengthFieldSize = 4U;
        }
    }
    else
    {
        lengthFieldSize = settings.sizeArrayLengthField;
    }

    std::size_t serializedSize{dataSize + lengthFieldSize};

    // Check if the serialized size exceeds the maximum size limit
    if (std::numeric_limits<uint32_t>::max() < serializedSize)
    {
        return 0U;
    }

    return static_cast<uint32_t>(serializedSize);
}

/**
 * @brief Function that computes the serialized size of a vector of primitive types.
 *
 * @tparam T The type of the elements in the vector.
 * @param data The vector to compute the serialized size for.
 * @param settings The serializer settings.
 * @param lengthFieldSize The length field size, which is set based on the serialized size.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return The computed serialized size of the vector.
 */
template<typename T, EnableIfBasic<T> = true>
static inline uint32_t computeSerializedSize(const std::vector<T>& data,
                                             const SerializerSettings& settings,
                                             uint8_t& lengthFieldSize,
                                             const bool hasTlvTag = false)
{
    lengthFieldSize = 0U;
    std::size_t dataSize{data.size() * sizeof(T)};

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        if (dataSize <= 0xFFU)
        {
            lengthFieldSize = 1U;
        }
        else if (dataSize <= 0xFFFFU)
        {
            lengthFieldSize = 2U;
        }
        else
        {
            lengthFieldSize = 4U;
        }
    }
    else
    {
        lengthFieldSize = settings.sizeVectorLengthField;
    }

    std::size_t serializedSize{dataSize + lengthFieldSize};

    // Check if the serialized size exceeds the maximum size limit
    if (std::numeric_limits<uint32_t>::max() < serializedSize)
    {
        return 0U;
    }

    return static_cast<uint32_t>(serializedSize);
}

/**
 * @brief Function that computes the serialized size of a map of primitive types.
 *
 * @tparam K The type of the keys in the map.
 * @tparam V The type of the values in the map.
 * @param data The map to compute the serialized size for.
 * @param settings The serializer settings.
 * @param lengthFieldSize The length field size, which is set based on the serialized size.
 * @param hasTlvTag Flag indicating whether the serialized data should include a TLV tag.
 * @return The computed serialized size of the map.
 */
template<typename K, typename V, EnableIfBasic<K> = true, EnableIfBasic<V> = true>
static inline uint32_t computeSerializedSize(const std::map<K, V>& data,
                                             const SerializerSettings& settings,
                                             uint8_t& lengthFieldSize,
                                             const bool hasTlvTag = false)
{
    lengthFieldSize = 0U;
    std::size_t dataSize{data.size() * (sizeof(K) + sizeof(V))};

    if (settings.isDynamicLengthFieldSize && hasTlvTag)
    {
        if (dataSize <= 0xFFU)
        {
            lengthFieldSize = 1U;
        }
        else if (dataSize <= 0xFFFFU)
        {
            lengthFieldSize = 2U;
        }
        else
        {
            lengthFieldSize = 4U;
        }
    }
    else
    {
        lengthFieldSize = settings.sizeMapLengthField;
    }

    std::size_t serializedSize{dataSize + lengthFieldSize};

    // Check if the serialized size exceeds the maximum size limit
    if (serializedSize > std::numeric_limits<uint32_t>::max())
    {
        return 0U;
    }

    return static_cast<uint32_t>(serializedSize);
}

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_SERIALIZERCOMPUTESIZE_HPP
