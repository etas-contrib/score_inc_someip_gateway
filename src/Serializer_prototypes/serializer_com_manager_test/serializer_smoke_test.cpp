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

// Compile-time smoke test: include every public header and exercise basic functionality.

#include "src/serializer/AppErrorSerializers.hpp"
#include "src/serializer/DeserializeBasicTypes.hpp"
#include "src/serializer/IDeserializer.hpp"
#include "src/serializer/ISerializer.hpp"
#include "src/serializer/SerializeBasicTypes.hpp"
#include "src/serializer/SerializerComputeSize.hpp"
#include "src/serializer/SerializerTypes.hpp"
#include "src/serializer/SerializerUtils.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

using namespace com::serializer;

int main()
{
    // --- 1. Round-trip a uint32_t ---
    SerializerSettings settings{};
    settings.byteOrder = ByteOrder::kBigEndian;

    const uint32_t original{0x12345678U};
    uint8_t buf[64]{};

    bool ok = serialize(original, buf, sizeof(buf), settings);
    assert(ok);

    uint32_t restored{0U};
    uint32_t readBytes{0U};
    ok = deserialize(restored, buf, sizeof(buf), settings, readBytes);
    assert(ok);
    assert(readBytes == sizeof(uint32_t));
    assert(restored == original);

    // --- 2. Round-trip a std::string ---
    settings.sizeStringLengthField = 4U;
    const std::string hello{"Hello"};
    uint8_t strBuf[64]{};

    ok = serialize(hello, strBuf, sizeof(strBuf), settings);
    assert(ok);

    std::string helloBack;
    readBytes = 0U;
    ok = deserialize(helloBack, strBuf, sizeof(strBuf), settings, readBytes);
    assert(ok);
    assert(helloBack == hello);

    // --- 3. Round-trip an AppError ---
    AppError errOut{42ULL, -1};
    uint8_t errBuf[64]{};
    ok = appErrorSerialize(errOut, errBuf, sizeof(errBuf));
    assert(ok);

    AppError errIn{};
    readBytes = 0U;
    ok = appErrorDeserialize(errIn, errBuf, sizeof(errBuf), readBytes);
    assert(ok);
    assert(errIn.domain == 42ULL);
    assert(errIn.code == -1);

    // --- 4. computeWireType smoke test ---
    uint8_t wt = computeWireType<uint8_t>(settings, 0U);
    assert(wt == 0U);
    wt = computeWireType<uint32_t>(settings, 0U);
    assert(wt == 2U);

    std::cout << "All serializer smoke tests passed.\n";
    return 0;
}
