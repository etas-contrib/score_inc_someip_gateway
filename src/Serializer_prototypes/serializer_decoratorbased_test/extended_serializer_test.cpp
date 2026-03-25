/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

// =============================================================================
// Layered Serializer Architecture — Comprehensive Test Suite
//
// Tests the four-layer architecture:
//   Layer 1: serializer_types.h       (framework — Node, ISerializer, Compound)
//   Layer 2: base_type_serializers.h  (BaseTypeSerializerFactory + all impls)
//   Layer 3: abstract_app_factory.h   (AbstractCarWindowFactory)
//   Layer 4: car_window_factory.h     (CarWindowFactory — concrete composition)
//
// Test sections:
//   1.  Primitive byte order (BE / Opaque) via base-type factory
//   2.  Bool serializer via factory
//   3.  SOME/IP string via factory
//   4.  Vector serializer via factory
//   5.  Bool vector serializer via factory
//   6.  Array serializer via factory
//   7.  Map serializer via factory
//   8.  AppError serializer via factory
//   9.  TLV decorator via factory
//  10.  Enum serializer via factory
//  11.  CompoundSerializer (manual composition)
//  12.  CarWindowFactory: WindowInfo (Layer 4)
//  13.  CarWindowFactory: WindowControl (Layer 4)
//  14.  Cross-deployment (SOME/IP vs IPC)
//  15.  serialize_to append
//  16.  deserialize_from offset
// =============================================================================

#include "car_window_factory.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace reader;
using namespace com::serializer;

// Helper: hexdump
static void hexdump(const ByteVector& v, const char* label) {
    std::printf("  %s (%zu bytes):", label, v.size());
    for (auto b : v) std::printf(" %02X", b);
    std::printf("\n");
}

// =============================================================================
//  Settings presets for testing
// =============================================================================

static constexpr SerializerSettings kBE{
    ByteOrder::kBigEndian,
    4U, 0U, 4U, 4U, 0U, 4U, false
};

static constexpr SerializerSettings kOpaque{
    ByteOrder::kOpaque,
    4U, 0U, 4U, 4U, 0U, 4U, false
};

// =============================================================================
//  Test 1: Primitive byte order via BaseTypeSerializerFactory
// =============================================================================
static void test_primitive_byte_order() {
    std::printf("\n--- Test 1: Primitive byte order via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);

    // uint32 BE
    auto u32_ser = factory.create_uint32_serializer();
    uint32_t val = 0x12345678U;
    RefNode<uint32_t> node(val);
    auto bytes = u32_ser->serialize(node);
    assert(bytes.size() == 4);
    assert(bytes[0] == 0x12 && bytes[1] == 0x34 && bytes[2] == 0x56 && bytes[3] == 0x78);
    std::printf("  PASS: uint32 BE serialization\n");

    // Round-trip
    uint32_t restored = 0;
    RefNode<uint32_t> rnode(restored);
    u32_ser->deserialize(bytes, rnode);
    assert(restored == 0x12345678U);
    std::printf("  PASS: uint32 BE round-trip\n");

    // Opaque (native order)
    SomeipBaseTypeSerializerFactory opaque_factory(kOpaque);
    auto opaque_ser = opaque_factory.create_uint32_serializer();
    auto native_bytes = opaque_ser->serialize(node);
    uint32_t native_check = 0;
    std::memcpy(&native_check, native_bytes.data(), 4);
    assert(native_check == 0x12345678U);
    std::printf("  PASS: uint32 Opaque round-trip (native)\n");

    // int16 BE
    auto i16_ser = factory.create_int16_serializer();
    int16_t v16 = 0x0102;
    RefNode<int16_t> n16(v16);
    auto b16 = i16_ser->serialize(n16);
    assert(b16.size() == 2 && b16[0] == 0x01 && b16[1] == 0x02);
    std::printf("  PASS: int16 BE serialization\n");

    // float BE round-trip
    auto f_ser = factory.create_float_serializer();
    float fval = 1.0f;
    RefNode<float> fnode(fval);
    auto fbytes = f_ser->serialize(fnode);
    assert(fbytes.size() == 4);
    float fresult = 0.0f;
    RefNode<float> frnode(fresult);
    f_ser->deserialize(fbytes, frnode);
    assert(fresult == 1.0f);
    std::printf("  PASS: float BE round-trip\n");

    // double BE round-trip
    auto d_ser = factory.create_double_serializer();
    double dval = 3.14;
    RefNode<double> dnode(dval);
    auto dbytes = d_ser->serialize(dnode);
    assert(dbytes.size() == 8);
    double dresult = 0.0;
    RefNode<double> drnode(dresult);
    d_ser->deserialize(dbytes, drnode);
    assert(dresult == 3.14);
    std::printf("  PASS: double BE round-trip\n");

    // All integer widths
    {
        auto s = factory.create_uint8_serializer();
        uint8_t v = 42;
        RefNode<uint8_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 1 && b[0] == 42);
        std::printf("  PASS: uint8\n");
    }
    {
        auto s = factory.create_uint16_serializer();
        uint16_t v = 0x1234;
        RefNode<uint16_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 2 && b[0] == 0x12 && b[1] == 0x34);
        std::printf("  PASS: uint16 BE\n");
    }
    {
        auto s = factory.create_uint64_serializer();
        uint64_t v = 1;
        RefNode<uint64_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 8 && b[7] == 0x01);
        std::printf("  PASS: uint64 BE\n");
    }
    {
        auto s = factory.create_int8_serializer();
        int8_t v = -1;
        RefNode<int8_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 1 && b[0] == 0xFF);
        std::printf("  PASS: int8\n");
    }
    {
        auto s = factory.create_int32_serializer();
        int32_t v = -1;
        RefNode<int32_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 4 && b[0] == 0xFF && b[3] == 0xFF);
        std::printf("  PASS: int32 BE\n");
    }
    {
        auto s = factory.create_int64_serializer();
        int64_t v = -1;
        RefNode<int64_t> n(v);
        auto b = s->serialize(n);
        assert(b.size() == 8 && b[0] == 0xFF && b[7] == 0xFF);
        std::printf("  PASS: int64 BE\n");
    }
}

// =============================================================================
//  Test 2: Bool serializer via factory
// =============================================================================
static void test_bool_serializer() {
    std::printf("\n--- Test 2: Bool serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_bool_serializer();

    bool t = true;
    RefNode<bool> tn(t);
    auto bytes = ser->serialize(tn);
    assert(bytes.size() == 1 && bytes[0] == 1U);
    std::printf("  PASS: bool true → 0x01\n");

    bool f = false;
    RefNode<bool> fn(f);
    auto fbytes = ser->serialize(fn);
    assert(fbytes[0] == 0U);
    std::printf("  PASS: bool false → 0x00\n");

    bool restored = false;
    RefNode<bool> rn(restored);
    ser->deserialize(bytes, rn);
    assert(restored == true);
    std::printf("  PASS: bool round-trip\n");
}

// =============================================================================
//  Test 3: SOME/IP String via factory
// =============================================================================
static void test_someip_string() {
    std::printf("\n--- Test 3: SOME/IP String via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_string_serializer();

    std::string hello = "Hello";
    RefNode<std::string> node(hello);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "serialized 'Hello'");

    // [4-byte len=9 BE] [EF BB BF] [H e l l o] [00] = 13 bytes
    assert(bytes.size() == 13);
    assert(bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x09);
    assert(bytes[4] == 0xEF && bytes[5] == 0xBB && bytes[6] == 0xBF);
    assert(bytes[7] == 'H' && bytes[12] == 0x00);
    std::printf("  PASS: string serialization\n");

    std::string restored;
    RefNode<std::string> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored == "Hello");
    std::printf("  PASS: string round-trip: '%s'\n", restored.c_str());
}

// =============================================================================
//  Test 4: Vector serializer via factory
// =============================================================================
static void test_vector_serializer() {
    std::printf("\n--- Test 4: Vector serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_vector_serializer<uint16_t>();

    std::vector<uint16_t> vec = {0x0102, 0x0304, 0x0506};
    RefNode<std::vector<uint16_t>> node(vec);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "vector<uint16>");

    assert(bytes.size() == 10);  // 4 + 6
    assert(bytes[0] == 0x00 && bytes[3] == 0x06);
    assert(bytes[4] == 0x01 && bytes[5] == 0x02);
    std::printf("  PASS: vector serialization\n");

    std::vector<uint16_t> restored;
    RefNode<std::vector<uint16_t>> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.size() == 3 && restored[0] == 0x0102);
    std::printf("  PASS: vector round-trip\n");
}

// =============================================================================
//  Test 5: Bool vector via factory
// =============================================================================
static void test_bool_vector_serializer() {
    std::printf("\n--- Test 5: Bool vector via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_bool_vector_serializer();

    std::vector<bool> bv = {true, false, true, true};
    RefNode<std::vector<bool>> node(bv);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "vector<bool>");

    assert(bytes.size() == 8);  // 4 + 4
    assert(bytes[4] == 0x01 && bytes[5] == 0x00 && bytes[6] == 0x01 && bytes[7] == 0x01);
    std::printf("  PASS: bool vector serialization\n");

    std::vector<bool> restored;
    RefNode<std::vector<bool>> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.size() == 4 && restored[0] == true && restored[1] == false);
    std::printf("  PASS: bool vector round-trip\n");
}

// =============================================================================
//  Test 6: Array serializer via factory
// =============================================================================
static void test_array_serializer() {
    std::printf("\n--- Test 6: Array serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_array_serializer<uint32_t, 3>();

    std::array<uint32_t, 3> arr = {0x11223344U, 0x55667788U, 0xAABBCCDDU};
    RefNode<std::array<uint32_t, 3>> node(arr);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "array<uint32,3>");

    assert(bytes.size() == 12);  // no length prefix (sizeArrayLengthField=0)
    assert(bytes[0] == 0x11 && bytes[3] == 0x44);
    assert(bytes[4] == 0x55 && bytes[7] == 0x88);
    std::printf("  PASS: array serialization\n");

    std::array<uint32_t, 3> restored = {};
    RefNode<std::array<uint32_t, 3>> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored[0] == 0x11223344U && restored[2] == 0xAABBCCDDU);
    std::printf("  PASS: array round-trip\n");
}

// =============================================================================
//  Test 7: Map serializer via factory
// =============================================================================
static void test_map_serializer() {
    std::printf("\n--- Test 7: Map serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_map_serializer<uint8_t, uint16_t>();

    std::map<uint8_t, uint16_t> m;
    m[0x01] = 0x1234;
    m[0x02] = 0x5678;
    RefNode<std::map<uint8_t, uint16_t>> node(m);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "map<uint8,uint16>");

    assert(bytes.size() == 10);  // 4 + 2*(1+2)
    assert(bytes[3] == 0x06);
    std::printf("  PASS: map serialization\n");

    std::map<uint8_t, uint16_t> restored;
    RefNode<std::map<uint8_t, uint16_t>> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.size() == 2 && restored[0x01] == 0x1234 && restored[0x02] == 0x5678);
    std::printf("  PASS: map round-trip\n");
}

// =============================================================================
//  Test 8: AppError serializer via factory
// =============================================================================
static void test_app_error_serializer() {
    std::printf("\n--- Test 8: AppError serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_app_error_serializer();

    AppError err{0x123456789ABCDEF0ULL, -42};
    RefNode<AppError> node(err);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "AppError");
    assert(bytes.size() == minBufferSize);
    std::printf("  PASS: AppError size = %u bytes\n", minBufferSize);

    AppError restored{0, 0};
    RefNode<AppError> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.domain == 0x123456789ABCDEF0ULL && restored.code == -42);
    std::printf("  PASS: AppError round-trip\n");
}

// =============================================================================
//  Test 9: TLV decorator via factory
// =============================================================================
static void test_tlv_decorator() {
    std::printf("\n--- Test 9: TLV decorator via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);

    // Wire type 2 (32-bit data), data ID = 0x001
    auto inner = factory.create_uint32_serializer();
    auto tlv = factory.create_tlv_serializer<uint32_t>(inner, 0x001, EWireType::E_WIRETYPE_2);

    uint32_t val = 0xDEADBEEF;
    RefNode<uint32_t> node(val);
    auto bytes = tlv->serialize(node);
    hexdump(bytes, "TLV(uint32)");

    assert(bytes.size() == 6);
    assert(bytes[0] == 0x20 && bytes[1] == 0x01);
    assert(bytes[2] == 0xDE && bytes[3] == 0xAD && bytes[4] == 0xBE && bytes[5] == 0xEF);
    std::printf("  PASS: TLV tag + payload\n");

    uint32_t restored = 0;
    RefNode<uint32_t> rnode(restored);
    tlv->deserialize(bytes, rnode);
    assert(restored == 0xDEADBEEF);
    std::printf("  PASS: TLV round-trip\n");
}

// =============================================================================
//  Test 9b: TLV with complex type (wire type 7 → 4-byte length field)
// =============================================================================
static void test_tlv_complex_type() {
    std::printf("\n--- Test 9b: TLV with complex type ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto inner = factory.create_string_serializer();
    auto tlv = factory.create_tlv_serializer<std::string>(inner, 0x010, EWireType::E_WIRETYPE_7);

    std::string val = "TLV";
    RefNode<std::string> node(val);
    auto bytes = tlv->serialize(node);
    hexdump(bytes, "TLV(string)");

    // [2-byte tag] [4-byte TLV-length] [4-byte string length] [BOM 3B] [T L V] [null]
    assert(bytes.size() == 17);
    std::printf("  PASS: TLV complex size = 17\n");

    std::string restored;
    RefNode<std::string> rnode(restored);
    tlv->deserialize(bytes, rnode);
    assert(restored == "TLV");
    std::printf("  PASS: TLV string round-trip: '%s'\n", restored.c_str());
}

// =============================================================================
//  Test 10: Enum serializer via factory
// =============================================================================
static void test_enum_serializer() {
    std::printf("\n--- Test 10: Enum serializer via factory ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_enum_serializer<car_window_types::WindowState>();

    auto state = car_window_types::WindowState::Closing;
    RefNode<car_window_types::WindowState> node(state);
    auto bytes = ser->serialize(node);
    assert(bytes.size() == 4 && bytes[3] == 0x02);
    std::printf("  PASS: enum BE (Closing=2)\n");

    auto restored = car_window_types::WindowState::Stopped;
    RefNode<car_window_types::WindowState> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored == car_window_types::WindowState::Closing);
    std::printf("  PASS: enum round-trip\n");
}

// =============================================================================
//  Test 11: CompoundSerializer (manual composition)
// =============================================================================
static void test_compound_serializer() {
    std::printf("\n--- Test 11: CompoundSerializer (manual composition) ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);

    auto compound = std::make_shared<CompoundSerializer<car_window_types::WindowInfo>>();

    compound->add_member<uint32_t>(
        factory.create_uint32_serializer(),
        [](const car_window_types::WindowInfo& wi) -> uint32_t { return wi.pos; },
        [](car_window_types::WindowInfo& wi, const uint32_t& v) { wi.pos = v; }
    );
    compound->add_member<car_window_types::WindowState>(
        factory.create_enum_serializer<car_window_types::WindowState>(),
        [](const car_window_types::WindowInfo& wi) -> car_window_types::WindowState { return wi.state; },
        [](car_window_types::WindowInfo& wi, const car_window_types::WindowState& v) { wi.state = v; }
    );

    car_window_types::WindowInfo info{75, car_window_types::WindowState::Opening};
    RefNode<car_window_types::WindowInfo> node(info);
    auto bytes = compound->serialize(node);
    hexdump(bytes, "CompoundSerializer<WindowInfo>");
    assert(bytes.size() == 8);
    assert(bytes[0] == 0x00 && bytes[3] == 0x4B);  // pos=75
    assert(bytes[4] == 0x00 && bytes[7] == 0x01);  // state=Opening(1)
    std::printf("  PASS: manual compound serialization\n");

    car_window_types::WindowInfo restored{0, car_window_types::WindowState::Stopped};
    RefNode<car_window_types::WindowInfo> rnode(restored);
    compound->deserialize(bytes, rnode);
    assert(restored.pos == 75 && restored.state == car_window_types::WindowState::Opening);
    std::printf("  PASS: manual compound round-trip\n");
}

// =============================================================================
//  Test 12: CarWindowFactory — WindowInfo (Layer 4)
// =============================================================================
static void test_car_window_factory_window_info() {
    std::printf("\n--- Test 12: CarWindowFactory — WindowInfo ---\n");

    // Layer 2: base-type factory
    SomeipBaseTypeSerializerFactory base(kCarWindowSomeipSettings);
    // Layer 4: application factory
    CarWindowFactory appFactory(base);

    auto ser = appFactory.create_window_info_serializer();

    car_window_types::WindowInfo info{0x12345678U, car_window_types::WindowState::Opening};
    RefNode<car_window_types::WindowInfo> node(info);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "WindowInfo via factory");

    assert(bytes.size() == 8);
    // pos=0x12345678 in BE
    assert(bytes[0] == 0x12 && bytes[1] == 0x34 && bytes[2] == 0x56 && bytes[3] == 0x78);
    // state=Opening(1) in BE
    assert(bytes[4] == 0x00 && bytes[5] == 0x00 && bytes[6] == 0x00 && bytes[7] == 0x01);
    std::printf("  PASS: WindowInfo serialization via CarWindowFactory\n");

    car_window_types::WindowInfo restored{0, car_window_types::WindowState::Stopped};
    RefNode<car_window_types::WindowInfo> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.pos == 0x12345678U);
    assert(restored.state == car_window_types::WindowState::Opening);
    std::printf("  PASS: WindowInfo round-trip via CarWindowFactory\n");

    // Verify deployment constants
    assert(CarWindowFactory::window_info_service_id() == 6432U);
    assert(CarWindowFactory::window_info_event_id() == 1U);
    std::printf("  PASS: deployment constants\n");
}

// =============================================================================
//  Test 13: CarWindowFactory — WindowControl (Layer 4)
// =============================================================================
static void test_car_window_factory_window_control() {
    std::printf("\n--- Test 13: CarWindowFactory — WindowControl ---\n");

    SomeipBaseTypeSerializerFactory base(kCarWindowSomeipSettings);
    CarWindowFactory appFactory(base);

    auto ser = appFactory.create_window_control_serializer();

    car_window_types::WindowControl ctrl{car_window_types::WindowCommand::Close};
    RefNode<car_window_types::WindowControl> node(ctrl);
    auto bytes = ser->serialize(node);
    hexdump(bytes, "WindowControl via factory");

    assert(bytes.size() == 4);
    assert(bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x00 && bytes[3] == 0x02);
    std::printf("  PASS: WindowControl serialization\n");

    car_window_types::WindowControl restored{car_window_types::WindowCommand::Stop};
    RefNode<car_window_types::WindowControl> rnode(restored);
    ser->deserialize(bytes, rnode);
    assert(restored.command == car_window_types::WindowCommand::Close);
    std::printf("  PASS: WindowControl round-trip\n");

    assert(CarWindowFactory::window_control_service_id() == 6433U);
    assert(CarWindowFactory::window_control_event_id() == 2U);
    std::printf("  PASS: deployment constants\n");
}

// =============================================================================
//  Test 14: Cross-deployment (SOME/IP vs IPC)
// =============================================================================
static void test_cross_deployment() {
    std::printf("\n--- Test 14: Cross-deployment comparison ---\n");

    // SOME/IP bundle
    CarWindowSomeipBundle someip;
    auto someipSer = someip.app.create_window_info_serializer();

    // IPC bundle
    CarWindowIpcBundle ipc;
    auto ipcSer = ipc.app.create_window_info_serializer();

    car_window_types::WindowInfo info{0x12345678U, car_window_types::WindowState::Opening};
    RefNode<car_window_types::WindowInfo> node(info);

    auto someipBytes = someipSer->serialize(node);
    auto ipcBytes = ipcSer->serialize(node);

    hexdump(someipBytes, "SOME/IP (BE)");
    hexdump(ipcBytes, "IPC (native)");

    assert(someipBytes != ipcBytes);
    std::printf("  PASS: SOME/IP ≠ IPC bytes (correct on LE host)\n");

    // Both must round-trip correctly
    car_window_types::WindowInfo someipBack{}, ipcBack{};
    RefNode<car_window_types::WindowInfo> sn(someipBack), in(ipcBack);
    someipSer->deserialize(someipBytes, sn);
    ipcSer->deserialize(ipcBytes, in);
    assert(someipBack.pos == 0x12345678U && someipBack.state == car_window_types::WindowState::Opening);
    assert(ipcBack.pos == 0x12345678U && ipcBack.state == car_window_types::WindowState::Opening);
    std::printf("  PASS: both deployments round-trip correctly\n");
}

// =============================================================================
//  Test 15: serialize_to append
// =============================================================================
static void test_serialize_to_append() {
    std::printf("\n--- Test 15: serialize_to append ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_uint32_serializer();

    ByteVector buf = {0xFF, 0xFF};
    uint32_t val = 0x01020304;
    RefNode<uint32_t> node(val);
    ser->serialize_to(node, buf);

    assert(buf.size() == 6);
    assert(buf[0] == 0xFF && buf[1] == 0xFF);
    assert(buf[2] == 0x01 && buf[3] == 0x02 && buf[4] == 0x03 && buf[5] == 0x04);
    std::printf("  PASS: serialize_to appends correctly\n");
}

// =============================================================================
//  Test 16: deserialize_from with offset
// =============================================================================
static void test_deserialize_from_offset() {
    std::printf("\n--- Test 16: deserialize_from with offset ---\n");

    SomeipBaseTypeSerializerFactory factory(kBE);
    auto ser = factory.create_uint16_serializer();

    ByteVector buf = {0xFF, 0xFF, 0x01, 0x02, 0x03, 0x04};
    std::size_t offset = 2;

    uint16_t v1 = 0;
    RefNode<uint16_t> n1(v1);
    ser->deserialize_from(buf, offset, n1);
    assert(v1 == 0x0102 && offset == 4);
    std::printf("  PASS: first deserialize at offset=2\n");

    uint16_t v2 = 0;
    RefNode<uint16_t> n2(v2);
    ser->deserialize_from(buf, offset, n2);
    assert(v2 == 0x0304 && offset == 6);
    std::printf("  PASS: second deserialize at offset=4\n");
}

// =============================================================================
int main() {
    std::printf("=== Layered Serializer Architecture Test Suite ===\n");

    test_primitive_byte_order();
    test_bool_serializer();
    test_someip_string();
    test_vector_serializer();
    test_bool_vector_serializer();
    test_array_serializer();
    test_map_serializer();
    test_app_error_serializer();
    test_tlv_decorator();
    test_tlv_complex_type();
    test_enum_serializer();
    test_compound_serializer();
    test_car_window_factory_window_info();
    test_car_window_factory_window_control();
    test_cross_deployment();
    test_serialize_to_append();
    test_deserialize_from_offset();

    std::printf("\n=== All 16 tests passed. ===\n");
    return 0;
}
