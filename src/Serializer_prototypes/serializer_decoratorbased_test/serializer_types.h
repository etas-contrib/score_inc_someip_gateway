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
// Serializer Framework Types
//
// Pure framework layer — contains only the abstract interfaces, data holders,
// decorator pattern, and compound serializer with member bindings.
// No serialization logic, no byte-order code, no dependency on src/serializer/.
// =============================================================================

#ifndef READER_SERIALIZER_TYPES_H
#define READER_SERIALIZER_TYPES_H

#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

namespace reader {

/// The byte buffer type used throughout the serializer framework.
using ByteVector = std::vector<std::uint8_t>;

// =====================================================================
//  Node<T> / RefNode<T> — mutable data holders for (de)serialization
// =====================================================================

/// Abstract typed data holder.
template <typename T>
class Node {
public:
    virtual ~Node() = default;
    virtual const T& get() const = 0;
    virtual T& get() = 0;
};

/// Lightweight concrete Node wrapping an external reference.
template <typename T>
class RefNode final : public Node<T> {
public:
    explicit RefNode(T& ref) noexcept : ref_(ref) {}
    const T& get() const override { return ref_; }
    T& get() override { return ref_; }

private:
    T& ref_;
};

// =====================================================================
//  ISerializer<T> — core serializer interface
// =====================================================================

/// Non-templated base for heterogeneous storage of serializers.
class ISerializerBase {
public:
    virtual ~ISerializerBase() = default;
};

/// Core serializer interface.
///
/// serialize()        — returns a new ByteVector (convenient, allocates)
/// serialize_to()     — appends to existing buffer (zero extra allocs)
/// deserialize()      — reads from a ByteVector
/// deserialize_from() — reads from buffer at offset, advances offset
template <typename T>
class ISerializer : public ISerializerBase {
public:
    virtual ~ISerializer() = default;

    virtual ByteVector serialize(const Node<T>& obj) const = 0;

    virtual void serialize_to(const Node<T>& obj, ByteVector& out) const {
        auto tmp = serialize(obj);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }

    virtual void deserialize(const ByteVector& data, Node<T>& obj) const = 0;

    virtual void deserialize_from(const ByteVector& data,
                                  std::size_t& offset,
                                  Node<T>& obj) const {
        deserialize(data, obj);
    }
};

// =====================================================================
//  SerializerDecorator<T> — Decorator pattern base
// =====================================================================

/// Transparent wrapper that forwards all calls to an inner serializer.
/// Subclass and override specific methods to add behavior (e.g. TLV tags).
template <typename T>
class SerializerDecorator : public ISerializer<T> {
public:
    explicit SerializerDecorator(std::shared_ptr<ISerializer<T>> inner)
        : inner_(std::move(inner)) {}

    ByteVector serialize(const Node<T>& obj) const override {
        return inner_ ? inner_->serialize(obj) : ByteVector();
    }
    void serialize_to(const Node<T>& obj, ByteVector& out) const override {
        if (inner_) inner_->serialize_to(obj, out);
    }
    void deserialize(const ByteVector& data, Node<T>& obj) const override {
        if (inner_) inner_->deserialize(data, obj);
    }
    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<T>& obj) const override {
        if (inner_) inner_->deserialize_from(data, offset, obj);
    }

protected:
    std::shared_ptr<ISerializer<T>> inner_;
};

// =====================================================================
//  MemberBinding + CompoundSerializer<T> — Composite pattern
// =====================================================================

/// Type-erased interface for one member's serialization within a compound.
template <typename Outer>
class IMemberBinding {
public:
    virtual ~IMemberBinding() = default;
    virtual void serialize_member(const Outer& obj, ByteVector& out) const = 0;
    virtual void deserialize_member(const ByteVector& data,
                                    std::size_t& offset,
                                    Outer& obj) const = 0;
};

/// Concrete binding: holds a serializer + getter/setter lambdas.
template <typename Outer, typename Member>
class MemberBinding : public IMemberBinding<Outer> {
public:
    using Getter = std::function<Member(const Outer&)>;
    using Setter = std::function<void(Outer&, const Member&)>;

    MemberBinding(std::shared_ptr<ISerializer<Member>> serializer,
                  Getter getter, Setter setter)
        : serializer_(std::move(serializer)),
          getter_(std::move(getter)),
          setter_(std::move(setter)) {}

    void serialize_member(const Outer& obj, ByteVector& out) const override {
        Member val = getter_(obj);
        RefNode<Member> node(val);
        serializer_->serialize_to(node, out);
    }

    void deserialize_member(const ByteVector& data, std::size_t& offset,
                            Outer& obj) const override {
        Member val{};
        RefNode<Member> node(val);
        serializer_->deserialize_from(data, offset, node);
        setter_(obj, val);
    }

private:
    std::shared_ptr<ISerializer<Member>> serializer_;
    Getter getter_;
    Setter setter_;
};

/// Composite serializer for struct types.
///
/// Members are serialized/deserialized in the order they were added,
/// matching SOME/IP flat struct wire layout.
template <typename T>
class CompoundSerializer : public ISerializer<T> {
public:
    /// Add a typed member with getter/setter lambdas.
    template <typename Member>
    void add_member(std::shared_ptr<ISerializer<Member>> serializer,
                    typename MemberBinding<T, Member>::Getter getter,
                    typename MemberBinding<T, Member>::Setter setter) {
        bindings_.push_back(std::make_unique<MemberBinding<T, Member>>(
            std::move(serializer), std::move(getter), std::move(setter)));
    }

    ByteVector serialize(const Node<T>& obj) const override {
        ByteVector out;
        serialize_to(obj, out);
        return out;
    }

    void serialize_to(const Node<T>& obj, ByteVector& out) const override {
        for (const auto& b : bindings_) {
            b->serialize_member(obj.get(), out);
        }
    }

    void deserialize(const ByteVector& data, Node<T>& obj) const override {
        std::size_t offset = 0;
        deserialize_from(data, offset, obj);
    }

    void deserialize_from(const ByteVector& data, std::size_t& offset,
                          Node<T>& obj) const override {
        for (const auto& b : bindings_) {
            b->deserialize_member(data, offset, obj.get());
        }
    }

private:
    std::vector<std::unique_ptr<IMemberBinding<T>>> bindings_;
};

}  // namespace reader

#endif  // READER_SERIALIZER_TYPES_H
