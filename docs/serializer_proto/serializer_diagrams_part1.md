<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Framework — Class & Sequence Diagrams

> **Scope:** `src/serializer/` (core library, committed on `serializer_prototype` branch)
> and `src/serializer_decoratorbased_test/` (decorator/factory prototype).
>
> Diagrams use ASCII art for maximum portability (renders in any Markdown viewer).

---

## Table of Contents

1. [Overview — Layered Architecture](#1-overview--layered-architecture)
2. [Core Library Class Diagram (`src/serializer/`)](#2-core-library-class-diagram-srcserializer)
3. [Decorator Prototype Class Diagram (`src/serializer_decoratorbased_test/`)](#3-decorator-prototype-class-diagram-srcserializer_decoratorbased_test)
4. [Relationship Between Core Library and Decorator Prototype](#4-relationship-between-core-library-and-decorator-prototype)
5. [Plugin Interface Class Diagram (`examples/serializer_plugin_common/`)](#5-plugin-interface-class-diagram-examplesserializer_plugin_common)
6. [Sequence Diagrams](#6-sequence-diagrams)
   - 6.1 [Serialize a Primitive Type (Core Library)](#61-serialize-a-primitive-type-core-library)
   - 6.2 [Deserialize a String with Length Prefix (Core Library)](#62-deserialize-a-string-with-length-prefix-core-library)
   - 6.3 [Serialize a Compound Struct (Decorator Prototype)](#63-serialize-a-compound-struct-decorator-prototype)
   - 6.4 [Factory Construction — CarWindow Bundle](#64-factory-construction--carwindow-bundle)
   - 6.5 [TLV Tag Encode / Decode Flow](#65-tlv-tag-encode--decode-flow)
   - 6.6 [AppError Serialize / Deserialize](#66-apperror-serialize--deserialize)

---

## 1. Overview — Layered Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Application Layer                            │
│   CarWindowFactory, ARXML-derived deployment constants & settings   │
│   (src/serializer_decoratorbased_test/car_window_factory.h)         │
├─────────────────────────────────────────────────────────────────────┤
│                   Abstract Application Factory                      │
│   AbstractCarWindowFactory (pure virtual per-domain factory)        │
│   (src/serializer_decoratorbased_test/abstract_app_factory.h)       │
├─────────────────────────────────────────────────────────────────────┤
│              Framework / Type-System Layer                           │
│   ISerializerBase, ISerializer<T>, Node<T>, RefNode<T>,             │
│   SerializerDecorator<T>, IMemberBinding<T>,                        │
│   MemberBinding<Outer,Member>, CompoundSerializer<T>                │
│   (src/serializer_decoratorbased_test/serializer_types.h)           │
├─────────────────────────────────────────────────────────────────────┤
│            Base-Type Serializer Factory Layer                        │
│   BaseTypeSerializerFactory (abstract),                             │
│   SomeipBaseTypeSerializerFactory (concrete),                       │
│   PrimitiveSerializer<T>, BoolSerializer, SomeipStringSerializer,   │
│   VectorSerializer<T>, BoolVectorSerializer, ArraySerializer<T,N>,  │
│   MapSerializer<K,V>, AppErrorSerializer, TlvSerializerDecorator<T> │
│   (src/serializer_decoratorbased_test/base_type_serializers.h)      │
├─────────────────────────────────────────────────────────────────────┤
│              SOME/IP Core Serializer Library                        │
│   Free-function overloads: serialize(), deserialize(),              │
│   computeSerializedSize() for primitives, strings, arrays,         │
│   vectors, maps (with TLV variants)                                 │
│   ISerializer<T>, IDeserializer<T> (abstract interfaces)            │
│   SerializerSettings, ByteOrder, EWireType                          │
│   Utility: swap(), writeLengthField(), readLengthField(),           │
│            writeTag(), readTag(), skipUnknownMember()                │
│   (src/serializer/*.hpp)                                            │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. Core Library Class Diagram (`src/serializer/`)

> Namespace: `com::serializer`

```
┌────────────────────────────────────────────────────────────────────┐
│                           «enum class»                             │
│                            ByteOrder                               │
│────────────────────────────────────────────────────────────────────│
│  kBigEndian    : uint8_t                                           │
│  kLittleEndian : uint8_t                                           │
│  kOpaque       : uint8_t                                           │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│                          «struct»                                  │
│                     SerializerSettings                              │
│                      alignas(8)                                    │
│────────────────────────────────────────────────────────────────────│
│  byteOrder                : ByteOrder                              │
│  sizeStringLengthField    : uint8_t                                │
│  sizeArrayLengthField     : uint8_t                                │
│  sizeVectorLengthField    : uint8_t                                │
│  sizeMapLengthField       : uint8_t                                │
│  sizeStructLengthField    : uint8_t                                │
│  sizeUnionLengthField     : uint8_t                                │
│  isDynamicLengthFieldSize : bool                                   │
└────────────────────────────────────────────────────────────────────┘
