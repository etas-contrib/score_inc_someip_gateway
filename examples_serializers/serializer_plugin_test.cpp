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
// SPDX-License-Identifier: Apache-2.0
// Integration test for all serializer plugins

#include "examples_serializers/serializer_plugin_common/common.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>

// Test implementation of the BigEndianSerializer
class BigEndianSerializer final : public plugin::ISerializerPlugin {
public:
    bool serialize(const plugin::Message& msg, plugin::WireBuffer& buf) const override {
        buf.data[0] = static_cast<uint8_t>((msg.id >> 24) & 0xFF);
        buf.data[1] = static_cast<uint8_t>((msg.id >> 16) & 0xFF);
        buf.data[2] = static_cast<uint8_t>((msg.id >>  8) & 0xFF);
        buf.data[3] = static_cast<uint8_t>((msg.id >>  0) & 0xFF);
        std::memcpy(&buf.data[4], msg.payload, sizeof(msg.payload));
        buf.length = 4 + sizeof(msg.payload);
        return true;
    }
    bool deserialize(const plugin::WireBuffer& buf, plugin::Message& msg) const override {
        if (buf.length < 4 + sizeof(msg.payload)) return false;
        msg.id = (static_cast<uint32_t>(buf.data[0]) << 24)
               | (static_cast<uint32_t>(buf.data[1]) << 16)
               | (static_cast<uint32_t>(buf.data[2]) <<  8)
               | (static_cast<uint32_t>(buf.data[3]) <<  0);
        std::memcpy(msg.payload, &buf.data[4], sizeof(msg.payload));
        return true;
    }
    const char* name() const override { return "BigEndianSerializer"; }
};

class LittleEndianSerializer final : public plugin::ISerializerPlugin {
public:
    bool serialize(const plugin::Message& msg, plugin::WireBuffer& buf) const override {
        buf.data[0] = static_cast<uint8_t>((msg.id >>  0) & 0xFF);
        buf.data[1] = static_cast<uint8_t>((msg.id >>  8) & 0xFF);
        buf.data[2] = static_cast<uint8_t>((msg.id >> 16) & 0xFF);
        buf.data[3] = static_cast<uint8_t>((msg.id >> 24) & 0xFF);
        std::memcpy(&buf.data[4], msg.payload, sizeof(msg.payload));
        buf.length = 4 + sizeof(msg.payload);
        return true;
    }
    bool deserialize(const plugin::WireBuffer& buf, plugin::Message& msg) const override {
        if (buf.length < 4 + sizeof(msg.payload)) return false;
        msg.id = (static_cast<uint32_t>(buf.data[3]) << 24)
               | (static_cast<uint32_t>(buf.data[2]) << 16)
               | (static_cast<uint32_t>(buf.data[1]) <<  8)
               | (static_cast<uint32_t>(buf.data[0]) <<  0);
        std::memcpy(msg.payload, &buf.data[4], sizeof(msg.payload));
        return true;
    }
    const char* name() const override { return "LittleEndianSerializer"; }
};

class SerializerPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_message.id = 0xDEADBEEF;
        test_message.set_payload("Test payload for serializer");
    }

    plugin::Message test_message;
    plugin::WireBuffer wire_buffer;
    plugin::Message result_message;
};

TEST_F(SerializerPluginTest, BigEndianRoundTrip) {
    BigEndianSerializer serializer;

    EXPECT_TRUE(serializer.serialize(test_message, wire_buffer));
    EXPECT_GT(wire_buffer.length, 0);

    // Check big-endian byte order
    EXPECT_EQ(wire_buffer.data[0], 0xDE);
    EXPECT_EQ(wire_buffer.data[1], 0xAD);
    EXPECT_EQ(wire_buffer.data[2], 0xBE);
    EXPECT_EQ(wire_buffer.data[3], 0xEF);

    EXPECT_TRUE(serializer.deserialize(wire_buffer, result_message));
    EXPECT_EQ(result_message.id, test_message.id);
    EXPECT_STREQ(result_message.payload, test_message.payload);

    EXPECT_STREQ(serializer.name(), "BigEndianSerializer");
}

TEST_F(SerializerPluginTest, LittleEndianRoundTrip) {
    LittleEndianSerializer serializer;

    EXPECT_TRUE(serializer.serialize(test_message, wire_buffer));
    EXPECT_GT(wire_buffer.length, 0);

    // Check little-endian byte order
    EXPECT_EQ(wire_buffer.data[0], 0xEF);
    EXPECT_EQ(wire_buffer.data[1], 0xBE);
    EXPECT_EQ(wire_buffer.data[2], 0xAD);
    EXPECT_EQ(wire_buffer.data[3], 0xDE);

    EXPECT_TRUE(serializer.deserialize(wire_buffer, result_message));
    EXPECT_EQ(result_message.id, test_message.id);
    EXPECT_STREQ(result_message.payload, test_message.payload);

    EXPECT_STREQ(serializer.name(), "LittleEndianSerializer");
}

TEST_F(SerializerPluginTest, MessageHelpers) {
    plugin::Message msg;
    std::string test_str = "Hello, World!";

    msg.set_payload(test_str);
    EXPECT_EQ(msg.get_payload(), test_str);

    // Test truncation of long strings
    std::string long_str(100, 'X');
    msg.set_payload(long_str);
    EXPECT_LT(msg.get_payload().length(), long_str.length());
    EXPECT_EQ(msg.get_payload().length(), 63); // sizeof(payload) - 1
}

TEST_F(SerializerPluginTest, InvalidDeserialization) {
    BigEndianSerializer serializer;

    // Test with buffer too short
    plugin::WireBuffer short_buffer;
    short_buffer.length = 2; // Too short

    EXPECT_FALSE(serializer.deserialize(short_buffer, result_message));
}
