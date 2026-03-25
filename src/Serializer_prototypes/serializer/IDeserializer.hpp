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

#ifndef COM_SERIALIZER_IDESERIALIZER_HPP
#define COM_SERIALIZER_IDESERIALIZER_HPP

#include <cstdint>

namespace com
{
namespace serializer
{

template<typename SampleType_T>
class IDeserializer
{
protected:
    /** @brief Default constructor. */
    IDeserializer() = default;

public:
    /** @brief Copy constructor not allowed. */
    IDeserializer(const IDeserializer&) = delete;

    /** @brief Move constructor not allowed. */
    IDeserializer(IDeserializer&&) = delete;

    /** @brief Virtual destructor required to allow deleting via the interface. */
    virtual ~IDeserializer() = default;

    /** @brief Copy assignment operator not allowed. */
    IDeserializer& operator=(const IDeserializer&) = delete;

    /** @brief Move assignment operator not allowed. */
    IDeserializer& operator=(IDeserializer&&) = delete;

    /**
     * @brief Deserialize from @p receivebuffer into @p objectp.
     *
     * @param receivebuffer  source buffer containing serialized data
     * @param length         number of bytes available in the buffer
     * @param objectp        pointer to the object to populate
     * @param readbytes      [out] number of bytes consumed from the buffer
     * @return true on success, false if the buffer is too small or malformed
     */
    virtual bool deserialize(const uint8_t* receivebuffer,
                             uint32_t length,
                             SampleType_T* objectp,
                             uint32_t& readbytes) = 0;

    /**
     * @brief Deserialize from @p buffer with TLV encoding.
     *
     * Default implementation returns false ("not supported").
     * Override in TLV-aware deserializers.
     */
    virtual bool deserializeTlv(const uint8_t*,
                                uint32_t,
                                SampleType_T*,
                                uint32_t&,
                                uint8_t)
    {
        return false;
    }
};

} // namespace serializer
} // namespace com

#endif // COM_SERIALIZER_IDESERIALIZER_HPP
