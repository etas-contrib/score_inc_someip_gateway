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
// (c) 2020-2025 Robert Bosch GmbH, ETAS GmbH & DENSO Corporation (original)
//
// Adapted for the Eclipse SCORE inc_someip_gateway project.

#ifndef COM_SERIALIZER_ISERIALIZER_HPP
#define COM_SERIALIZER_ISERIALIZER_HPP

#include <cstdint>
#include <limits>

namespace com
{
namespace serializer
{

template<typename SampleType_T>
class ISerializer
{
protected:
    /** @brief Default constructor. */
    ISerializer() = default;

public:
    /** @brief Copy constructor not allowed. */
    ISerializer(const ISerializer&) = delete;

    /** @brief Move constructor not allowed. */
    ISerializer(ISerializer&&) = delete;

    /** @brief Virtual destructor required to allow deleting via the interface. */
    virtual ~ISerializer() = default;

    /** @brief Copy assignment operator not allowed. */
    ISerializer& operator=(const ISerializer&) = delete;

    /** @brief Move assignment operator not allowed. */
    ISerializer& operator=(ISerializer&&) = delete;

    /**
     * @brief Compute the serialized size of @p objectp.
     *
     * Used to determine how much buffer space to allocate before calling serialize().
     */
    virtual uint32_t computeSerializedSize(const SampleType_T* objectp) = 0;

    /**
     * @brief Compute serialized size including TLV overhead (two-arg overload).
     *
     * Default implementation returns max uint32_t to signal "not supported".
     * Override in TLV-aware serializers.
     */
    virtual uint32_t computeSerializedSizeTlv(const SampleType_T*, uint8_t&)
    {
        return std::numeric_limits<uint32_t>::max();
    }

    /**
     * @brief Compute serialized size including TLV overhead (one-arg overload).
     *
     * Delegates to the two-arg overload, discarding the length-field-size output.
     */
    virtual uint32_t computeSerializedSizeTlv(const SampleType_T* objectp)
    {
        uint8_t lengthFieldSizeOut{};
        return computeSerializedSizeTlv(objectp, lengthFieldSizeOut);
    }

    /**
     * @brief Serialize @p objectp into @p targetbuffer.
     *
     * @param targetbuffer  destination buffer
     * @param maxsize       size of the destination buffer in bytes
     * @param objectp       pointer to the object to serialize
     * @return true on success, false if the buffer is too small or an error occurs
     */
    virtual bool serialize(uint8_t* targetbuffer, uint32_t maxsize, const SampleType_T* objectp) = 0;

    /**
     * @brief Serialize @p objectp into @p targetbuffer with TLV encoding.
     *
     * Default implementation returns false ("not supported").
     * Override in TLV-aware serializers.
     */
    virtual bool serializeTlv(uint8_t*, uint32_t, const SampleType_T*)
    {
        return false;
    }
};

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_ISERIALIZER_HPP
