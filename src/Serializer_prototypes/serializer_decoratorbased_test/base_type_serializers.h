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
// Base-Type Serializer Factory
//
// Layer 1: Abstract factory interface (BaseTypeSerializerFactory)
//          Pure virtual methods for every SOME/IP primitive, container,
//          and special type. Application-specific (struct) serializers
//          are NOT created here — they belong in the app factory.
//
// Layer 2: Concrete implementation (SomeipBaseTypeSerializerFactory)
//          Parameterized by com::serializer::SerializerSettings.
//          Implements every base type using configurable byte-order-aware
//          serializers backed by src/serializer/ free functions.
//
// Architecture:
//   - Complex (application) types shall ONLY use serializers obtained
//     from this factory to serialize their members.
//   - This ensures all byte-order and wire-format logic is centralized.
// =============================================================================

#ifndef READER_BASE_TYPE_SERIALIZERS_H
#define READER_BASE_TYPE_SERIALIZERS_H

#include "serializer_types.h"
#include "src/serializer/AppErrorSerializers.hpp"
#include "src/serializer/DeserializeBasicTypes.hpp"
#include "src/serializer/SerializeBasicTypes.hpp"
#include "src/serializer/SerializerComputeSize.hpp"
#include "src/serializer/SerializerTypes.hpp"
#include "src/serializer/SerializerUtils.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace reader {

// =====================================================================
//  Abstract interface — BaseTypeSerializerFactory
//
//  Provides creation methods for ALL SOME/IP base types.
//  Concrete application factories receive a reference to this and use
//  it to build their struct serializers.
// =====================================================================

class BaseTypeSerializerFactory {
public:
    virtual ~BaseTypeSerializerFactory() = default;

    // --- Integer types ---
    virtual std::shared_ptr<ISerializer<uint8_t>>  create_uint8_serializer()  const = 0;
    virtual std::shared_ptr<ISerializer<uint16_t>> create_uint16_serializer() const = 0;
    virtual std::shared_ptr<ISerializer<uint32_t>> create_uint32_serializer() const = 0;
    virtual std::shared_ptr<ISerializer<uint64_t>> create_uint64_serializer() const = 0;
    virtual std::shared_ptr<ISerializer<int8_t>>   create_int8_serializer()   const = 0;
    virtual std::shared_ptr<ISerializer<int16_t>>  create_int16_serializer()  const = 0;
    virtual std::shared_ptr<ISerializer<int32_t>>  create_int32_serializer()  const = 0;
    virtual std::shared_ptr<ISerializer<int64_t>>  create_int64_serializer()  const = 0;

    // --- Floating-point types ---
    virtual std::shared_ptr<ISerializer<float>>  create_float_serializer()  const = 0;
    virtual std::shared_ptr<ISerializer<double>> create_double_serializer() const = 0;

    // --- Bool (1 byte on wire) ---
    virtual std::shared_ptr<ISerializer<bool>> create_bool_serializer() const = 0;

    // --- SOME/IP string (BOM + length + null) ---
    virtual std::shared_ptr<ISerializer<std::string>> create_string_serializer() const = 0;

    // --- Enum (byte-order-aware, underlying type width) ---
    template <typename Enum>
    std::shared_ptr<ISerializer<Enum>> create_enum_serializer() const {
        static_assert(std::is_enum<Enum>::value,
                      "create_enum_serializer requires an enum type");
        return create_enum_serializer_impl<Enum>();
    }

    // --- Containers ---
    template <typename T>
    std::shared_ptr<ISerializer<std::vector<T>>> create_vector_serializer() const {
        return create_vector_serializer_impl<T>();
    }

    virtual std::shared_ptr<ISerializer<std::vector<bool>>>
    create_bool_vector_serializer() const = 0;

    template <typename T, std::size_t N>
    std::shared_ptr<ISerializer<std::array<T, N>>> create_array_serializer() const {
        return create_array_serializer_impl<T, N>();
    }

    template <typename K, typename V>
    std::shared_ptr<ISerializer<std::map<K, V>>> create_map_serializer() const {
        return create_map_serializer_impl<K, V>();
    }

    // --- AppError (domain + code, always big-endian) ---
    virtual std::shared_ptr<ISerializer<com::serializer::AppError>>
    create_app_error_serializer() const = 0;

    // --- TLV wrapper (decorate any serializer with a TLV tag) ---
    template <typename T>
    std::shared_ptr<ISerializer<T>> create_tlv_serializer(
        std::shared_ptr<ISerializer<T>> inner,
        uint16_t dataId,
        com::serializer::EWireType wireType) const {
        return create_tlv_serializer_impl<T>(std::move(inner), dataId, wireType);
    }

protected:
    // Virtual template trampolines (CRTP alternative for templated virtuals)
    // Concrete subclass overrides via the NVI-like pattern in the impl class.
    // We use type-erased helpers for the templated methods.

    // These are implemented in the concrete class below.
    virtual std::shared_ptr<ISerializerBase>
    create_enum_serializer_erased(std::size_t typeSize) const = 0;

    virtual std::shared_ptr<ISerializerBase>
    create_vector_serializer_erased(std::size_t elemSize) const = 0;

    virtual std::shared_ptr<ISerializerBase>
    create_array_serializer_erased(std::size_t elemSize, std::size_t count) const = 0;

    virtual std::shared_ptr<ISerializerBase>
    create_map_serializer_erased(std::size_t keySize, std::size_t valSize) const = 0;

    virtual std::shared_ptr<ISerializerBase>
    create_tlv_serializer_erased(std::shared_ptr<ISerializerBase> inner,
                                 uint16_t dataId,
                                 com::serializer::EWireType wireType) const = 0;

private:
    // Private typed trampolines that cast from erased to typed.
    // These are NOT virtual — they call the virtual erased version.
    template <typename Enum>
    std::shared_ptr<ISerializer<Enum>> create_enum_serializer_impl() const;

    template <typename T>
    std::shared_ptr<ISerializer<std::vector<T>>> create_vector_serializer_impl() const;

    template <typename T, std::size_t N>
    std::shared_ptr<ISerializer<std::array<T, N>>> create_array_serializer_impl() const;

    template <typename K, typename V>
    std::shared_ptr<ISerializer<std::map<K, V>>> create_map_serializer_impl() const;

    template <typename T>
    std::shared_ptr<ISerializer<T>> create_tlv_serializer_impl(
        std::shared_ptr<ISerializer<T>> inner,
        uint16_t dataId,
        com::serializer::EWireType wireType) const;
};

// =====================================================================
//  Internal: byte-swap utilities
// =====================================================================
namespace detail {

inline bool needsSwap(com::serializer::ByteOrder order) {
    if (order == com::serializer::ByteOrder::kOpaque) return false;
    static const uint32_t one{1U};
    bool hostIsLittle = (*reinterpret_cast<const uint8_t*>(&one) == 1U);
    return (order == com::serializer::ByteOrder::kBigEndian) == hostIsLittle;
}

template <typename T>
inline T byteSwap(T val) {
    auto* p = reinterpret_cast<uint8_t*>(&val);
    std::reverse(p, p + sizeof(T));
    return val;
}

}  // namespace detail

// =====================================================================
//  Serializer implementations (used by the concrete factory below)
// =====================================================================

/// Byte-order-aware serializer for any arithmetic or enum type.
template <typename T>
class PrimitiveSerializer : public ISerializer<T> {
    static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value,
                  "PrimitiveSerializer requires arithmetic or enum type");

public:
    explicit PrimitiveSerializer(com::serializer::SerializerSettings settings)
        : settings_(settings), swap_(detail::needsSwap(settings.byteOrder)) {}

    ByteVector serialize(const Node<T>& obj) const override {
        ByteVector out;
        out.reserve(sizeof(T));
        serialize_to(obj, out);
        return out;
    }

    void serialize_to(const Node<T>& obj, ByteVector& out) const override {
        T val = obj.get();
        if (swap_) val = detail::byteSwap(val);
        const auto* p = reinterpret_cast<const uint8_t*>(&val);
        out.insert(out.end(), p, p + sizeof(T));
    }

    void deserialize(const ByteVector& data, Node<T>& obj) const override {
        std::size_t offset = 0;
        deserialize_from(data, offset, obj);
    }

    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<T>& obj) const override {
        if (offset + sizeof(T) > data.size()) return;
        T val{};
        std::memcpy(&val, data.data() + offset, sizeof(T));
        if (swap_) val = detail::byteSwap(val);
        obj.get() = val;
        offset += sizeof(T);
    }

private:
    com::serializer::SerializerSettings settings_;
    bool swap_;
};

/// Bool serializer — 1 byte on wire (PRS_SOMEIP convention).
class BoolSerializer : public ISerializer<bool> {
public:
    explicit BoolSerializer(com::serializer::SerializerSettings /*settings*/) {}

    ByteVector serialize(const Node<bool>& obj) const override {
        return {static_cast<uint8_t>(obj.get() ? 1U : 0U)};
    }
    void serialize_to(const Node<bool>& obj, ByteVector& out) const override {
        out.push_back(static_cast<uint8_t>(obj.get() ? 1U : 0U));
    }
    void deserialize(const ByteVector& data, Node<bool>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<bool>& obj) const override {
        if (offset >= data.size()) return;
        obj.get() = (data[offset] != 0U);
        ++offset;
    }
};

/// SOME/IP string — BOM + length prefix + null terminator
/// (PRS_SOMEIP_00084 / 00085 / 00086).
class SomeipStringSerializer : public ISerializer<std::string> {
public:
    explicit SomeipStringSerializer(com::serializer::SerializerSettings s) : s_(s) {}

    ByteVector serialize(const Node<std::string>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }
    void serialize_to(const Node<std::string>& obj, ByteVector& out) const override {
        const auto& str = obj.get();
        uint32_t sz = com::serializer::computeSerializedSize(str, s_);
        if (sz == 0U) return;
        auto base = out.size();
        out.resize(base + sz);
        if (!com::serializer::serialize(str, out.data() + base, sz, s_))
            out.resize(base);
    }
    void deserialize(const ByteVector& data, Node<std::string>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<std::string>& obj) const override {
        if (offset >= data.size()) return;
        uint32_t rb = 0;
        if (com::serializer::deserialize(obj.get(), data.data() + offset,
                                         static_cast<uint32_t>(data.size() - offset), s_, rb))
            offset += rb;
    }

private:
    com::serializer::SerializerSettings s_;
};

/// Length-prefixed vector of arithmetic/enum elements.
template <typename T>
class VectorSerializer : public ISerializer<std::vector<T>> {
    static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value,
                  "VectorSerializer element must be arithmetic or enum");

public:
    explicit VectorSerializer(com::serializer::SerializerSettings s) : s_(s) {}

    ByteVector serialize(const Node<std::vector<T>>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }
    void serialize_to(const Node<std::vector<T>>& obj, ByteVector& out) const override {
        const auto& v = obj.get();
        uint32_t sz = com::serializer::computeSerializedSize(v, s_);
        if (sz == 0U && !v.empty()) return;
        auto base = out.size();
        out.resize(base + sz);
        if (!com::serializer::serialize(v, out.data() + base, sz, s_))
            out.resize(base);
    }
    void deserialize(const ByteVector& data, Node<std::vector<T>>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<std::vector<T>>& obj) const override {
        if (offset >= data.size()) return;
        uint32_t rb = 0;
        if (com::serializer::deserialize(obj.get(), data.data() + offset,
                                         static_cast<uint32_t>(data.size() - offset), s_, rb))
            offset += rb;
    }

private:
    com::serializer::SerializerSettings s_;
};

/// Special vector<bool> serializer.
class BoolVectorSerializer : public ISerializer<std::vector<bool>> {
public:
    explicit BoolVectorSerializer(com::serializer::SerializerSettings s) : s_(s) {}

    ByteVector serialize(const Node<std::vector<bool>>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }
    void serialize_to(const Node<std::vector<bool>>& obj, ByteVector& out) const override {
        const auto& v = obj.get();
        uint32_t sz = com::serializer::computeSerializedSize(v, s_);
        if (sz == 0U && !v.empty()) return;
        auto base = out.size();
        out.resize(base + sz);
        if (!com::serializer::serialize(v, out.data() + base, sz, s_))
            out.resize(base);
    }
    void deserialize(const ByteVector& data, Node<std::vector<bool>>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<std::vector<bool>>& obj) const override {
        if (offset >= data.size()) return;
        uint32_t rb = 0;
        if (com::serializer::deserialize(obj.get(), data.data() + offset,
                                         static_cast<uint32_t>(data.size() - offset), s_, rb))
            offset += rb;
    }

private:
    com::serializer::SerializerSettings s_;
};

/// Fixed-size array with optional length prefix.
template <typename T, std::size_t N>
class ArraySerializer : public ISerializer<std::array<T, N>> {
    static_assert(std::is_arithmetic<T>::value || std::is_enum<T>::value,
                  "ArraySerializer element must be arithmetic or enum");

public:
    explicit ArraySerializer(com::serializer::SerializerSettings s) : s_(s) {}

    ByteVector serialize(const Node<std::array<T, N>>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }
    void serialize_to(const Node<std::array<T, N>>& obj, ByteVector& out) const override {
        const auto& a = obj.get();
        uint32_t sz = com::serializer::computeSerializedSize(a, s_);
        if (sz == 0U) return;
        auto base = out.size();
        out.resize(base + sz);
        if (!com::serializer::serialize(a, out.data() + base, sz, s_))
            out.resize(base);
    }
    void deserialize(const ByteVector& data, Node<std::array<T, N>>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<std::array<T, N>>& obj) const override {
        if (offset >= data.size()) return;
        uint32_t rb = 0;
        if (com::serializer::deserialize(obj.get(), data.data() + offset,
                                         static_cast<uint32_t>(data.size() - offset), s_, rb))
            offset += rb;
    }

private:
    com::serializer::SerializerSettings s_;
};

/// Length-prefixed map of arithmetic/enum key-value pairs.
template <typename K, typename V>
class MapSerializer : public ISerializer<std::map<K, V>> {
    static_assert((std::is_arithmetic<K>::value || std::is_enum<K>::value) &&
                  (std::is_arithmetic<V>::value || std::is_enum<V>::value),
                  "MapSerializer key/value must be arithmetic or enum");

public:
    explicit MapSerializer(com::serializer::SerializerSettings s) : s_(s) {}

    ByteVector serialize(const Node<std::map<K, V>>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }
    void serialize_to(const Node<std::map<K, V>>& obj, ByteVector& out) const override {
        const auto& m = obj.get();
        uint32_t sz = com::serializer::computeSerializedSize(m, s_);
        if (sz == 0U && !m.empty()) return;
        auto base = out.size();
        out.resize(base + sz);
        com::serializer::serialize(m, out.data() + base, sz, s_);
    }
    void deserialize(const ByteVector& data, Node<std::map<K, V>>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<std::map<K, V>>& obj) const override {
        if (offset >= data.size()) return;
        uint32_t rb = 0;
        if (com::serializer::deserialize(obj.get(), data.data() + offset,
                                         static_cast<uint32_t>(data.size() - offset), s_, rb))
            offset += rb;
    }

private:
    com::serializer::SerializerSettings s_;
};

/// AppError serializer (domain:uint64 + code:int32, always big-endian).
class AppErrorSerializer : public ISerializer<com::serializer::AppError> {
public:
    ByteVector serialize(const Node<com::serializer::AppError>& obj) const override {
        ByteVector out(com::serializer::minBufferSize, 0U);
        com::serializer::appErrorSerialize(obj.get(), out.data(),
                                           static_cast<uint32_t>(out.size()));
        return out;
    }
    void serialize_to(const Node<com::serializer::AppError>& obj,
                      ByteVector& out) const override {
        auto base = out.size();
        out.resize(base + com::serializer::minBufferSize, 0U);
        com::serializer::appErrorSerialize(obj.get(), out.data() + base,
                                           com::serializer::minBufferSize);
    }
    void deserialize(const ByteVector& data,
                     Node<com::serializer::AppError>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<com::serializer::AppError>& obj) const override {
        if (offset + com::serializer::minBufferSize > data.size()) return;
        uint32_t rb = 0;
        com::serializer::appErrorDeserialize(obj.get(), data.data() + offset,
                                             static_cast<uint32_t>(data.size() - offset), rb);
        offset += rb;
    }
};

/// TLV decorator — wraps any serializer with a 2-byte TLV tag.
template <typename T>
class TlvSerializerDecorator : public SerializerDecorator<T> {
public:
    TlvSerializerDecorator(std::shared_ptr<ISerializer<T>> inner,
                           uint16_t dataId,
                           com::serializer::EWireType wireType,
                           com::serializer::SerializerSettings settings)
        : SerializerDecorator<T>(std::move(inner)),
          dataId_(dataId), wireType_(wireType), settings_(settings) {}

    ByteVector serialize(const Node<T>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }

    void serialize_to(const Node<T>& obj, ByteVector& out) const override {
        ByteVector payload;
        if (this->inner_) this->inner_->serialize_to(obj, payload);

        uint8_t tag[2];
        com::serializer::writeTag(tag, static_cast<uint8_t>(wireType_), dataId_);
        out.push_back(tag[0]);
        out.push_back(tag[1]);

        uint8_t wt = static_cast<uint8_t>(wireType_);
        if (wt >= 5U) {
            uint8_t lfSize = com::serializer::computeSizeOfLengthFieldBasedOnWireType(wt);
            uint8_t lfBuf[4] = {};
            uint8_t* p = lfBuf;
            com::serializer::writeLengthField(lfSize, static_cast<uint32_t>(payload.size()),
                                              &p, settings_.byteOrder);
            out.insert(out.end(), lfBuf, lfBuf + lfSize);
        }

        out.insert(out.end(), payload.begin(), payload.end());
    }

    void deserialize(const ByteVector& data, Node<T>& obj) const override {
        std::size_t off = 0;
        deserialize_from(data, off, obj);
    }

    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<T>& obj) const override {
        if (offset + 2 > data.size()) return;

        uint8_t rWt = 0;
        uint16_t rId = 0;
        uint32_t tagBytes = 0;
        if (!com::serializer::readTag(data.data() + offset, rWt, rId,
                                      static_cast<uint32_t>(data.size() - offset), tagBytes))
            return;
        offset += tagBytes;

        if (rWt >= 5U) {
            uint8_t lfSize = com::serializer::computeSizeOfLengthFieldBasedOnWireType(rWt);
            if (offset + lfSize > data.size()) return;
            uint32_t payLen = 0;
            const uint8_t* p = data.data() + offset;
            com::serializer::readLengthField(lfSize, payLen, &p, settings_.byteOrder);
            offset += lfSize;
        }

        if (this->inner_) this->inner_->deserialize_from(data, offset, obj);
    }

private:
    uint16_t dataId_;
    com::serializer::EWireType wireType_;
    com::serializer::SerializerSettings settings_;
};

// =====================================================================
//  Concrete: SomeipBaseTypeSerializerFactory
//
//  Parameterized by SerializerSettings (from ARXML or hardcoded).
//  Creates byte-order-aware serializers for every SOME/IP base type.
// =====================================================================

class SomeipBaseTypeSerializerFactory : public BaseTypeSerializerFactory {
public:
    explicit SomeipBaseTypeSerializerFactory(com::serializer::SerializerSettings settings)
        : s_(settings) {}

    const com::serializer::SerializerSettings& settings() const { return s_; }

    // --- Integer types ---
    std::shared_ptr<ISerializer<uint8_t>>  create_uint8_serializer()  const override { return std::make_shared<PrimitiveSerializer<uint8_t>>(s_); }
    std::shared_ptr<ISerializer<uint16_t>> create_uint16_serializer() const override { return std::make_shared<PrimitiveSerializer<uint16_t>>(s_); }
    std::shared_ptr<ISerializer<uint32_t>> create_uint32_serializer() const override { return std::make_shared<PrimitiveSerializer<uint32_t>>(s_); }
    std::shared_ptr<ISerializer<uint64_t>> create_uint64_serializer() const override { return std::make_shared<PrimitiveSerializer<uint64_t>>(s_); }
    std::shared_ptr<ISerializer<int8_t>>   create_int8_serializer()   const override { return std::make_shared<PrimitiveSerializer<int8_t>>(s_); }
    std::shared_ptr<ISerializer<int16_t>>  create_int16_serializer()  const override { return std::make_shared<PrimitiveSerializer<int16_t>>(s_); }
    std::shared_ptr<ISerializer<int32_t>>  create_int32_serializer()  const override { return std::make_shared<PrimitiveSerializer<int32_t>>(s_); }
    std::shared_ptr<ISerializer<int64_t>>  create_int64_serializer()  const override { return std::make_shared<PrimitiveSerializer<int64_t>>(s_); }

    // --- Floating-point ---
    std::shared_ptr<ISerializer<float>>  create_float_serializer()  const override { return std::make_shared<PrimitiveSerializer<float>>(s_); }
    std::shared_ptr<ISerializer<double>> create_double_serializer() const override { return std::make_shared<PrimitiveSerializer<double>>(s_); }

    // --- Bool ---
    std::shared_ptr<ISerializer<bool>> create_bool_serializer() const override { return std::make_shared<BoolSerializer>(s_); }

    // --- String ---
    std::shared_ptr<ISerializer<std::string>> create_string_serializer() const override { return std::make_shared<SomeipStringSerializer>(s_); }

    // --- Bool vector ---
    std::shared_ptr<ISerializer<std::vector<bool>>> create_bool_vector_serializer() const override { return std::make_shared<BoolVectorSerializer>(s_); }

    // --- AppError ---
    std::shared_ptr<ISerializer<com::serializer::AppError>> create_app_error_serializer() const override { return std::make_shared<AppErrorSerializer>(); }

protected:
    // --- Type-erased trampolines for templated factory methods ---

    std::shared_ptr<ISerializerBase>
    create_enum_serializer_erased(std::size_t /*typeSize*/) const override {
        // Not used directly — the typed trampoline is instantiated per-Enum.
        return nullptr;
    }

    std::shared_ptr<ISerializerBase>
    create_vector_serializer_erased(std::size_t /*elemSize*/) const override {
        return nullptr;
    }

    std::shared_ptr<ISerializerBase>
    create_array_serializer_erased(std::size_t /*elemSize*/, std::size_t /*count*/) const override {
        return nullptr;
    }

    std::shared_ptr<ISerializerBase>
    create_map_serializer_erased(std::size_t /*keySize*/, std::size_t /*valSize*/) const override {
        return nullptr;
    }

    std::shared_ptr<ISerializerBase>
    create_tlv_serializer_erased(std::shared_ptr<ISerializerBase> /*inner*/,
                                 uint16_t /*dataId*/,
                                 com::serializer::EWireType /*wireType*/) const override {
        return nullptr;
    }

    // Expose settings for the typed trampolines
    const com::serializer::SerializerSettings& get_settings() const { return s_; }

private:
    com::serializer::SerializerSettings s_;
};

// =====================================================================
//  Typed trampoline implementations for BaseTypeSerializerFactory
//  (these directly instantiate the templated serializers)
// =====================================================================

template <typename Enum>
std::shared_ptr<ISerializer<Enum>>
BaseTypeSerializerFactory::create_enum_serializer_impl() const {
    // We know the concrete factory holds settings — access via downcast
    // to SomeipBaseTypeSerializerFactory. This is safe because the template
    // method is only called on concrete instances.
    auto* concrete = dynamic_cast<const SomeipBaseTypeSerializerFactory*>(this);
    if (!concrete) return nullptr;
    return std::make_shared<PrimitiveSerializer<Enum>>(concrete->settings());
}

template <typename T>
std::shared_ptr<ISerializer<std::vector<T>>>
BaseTypeSerializerFactory::create_vector_serializer_impl() const {
    auto* concrete = dynamic_cast<const SomeipBaseTypeSerializerFactory*>(this);
    if (!concrete) return nullptr;
    return std::make_shared<VectorSerializer<T>>(concrete->settings());
}

template <typename T, std::size_t N>
std::shared_ptr<ISerializer<std::array<T, N>>>
BaseTypeSerializerFactory::create_array_serializer_impl() const {
    auto* concrete = dynamic_cast<const SomeipBaseTypeSerializerFactory*>(this);
    if (!concrete) return nullptr;
    return std::make_shared<ArraySerializer<T, N>>(concrete->settings());
}

template <typename K, typename V>
std::shared_ptr<ISerializer<std::map<K, V>>>
BaseTypeSerializerFactory::create_map_serializer_impl() const {
    auto* concrete = dynamic_cast<const SomeipBaseTypeSerializerFactory*>(this);
    if (!concrete) return nullptr;
    return std::make_shared<MapSerializer<K, V>>(concrete->settings());
}

template <typename T>
std::shared_ptr<ISerializer<T>>
BaseTypeSerializerFactory::create_tlv_serializer_impl(
    std::shared_ptr<ISerializer<T>> inner,
    uint16_t dataId,
    com::serializer::EWireType wireType) const {
    auto* concrete = dynamic_cast<const SomeipBaseTypeSerializerFactory*>(this);
    if (!concrete) return nullptr;
    return std::make_shared<TlvSerializerDecorator<T>>(
        std::move(inner), dataId, wireType, concrete->settings());
}

}  // namespace reader

#endif  // READER_BASE_TYPE_SERIALIZERS_H
