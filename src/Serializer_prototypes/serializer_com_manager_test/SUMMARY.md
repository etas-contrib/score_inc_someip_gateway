<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Module Summary (`src/serializer`)

## Class Diagram (ASCII Art)

```
+---------------------------------------------------+
|                com::serializer                    |
+---------------------------------------------------+
|                                                   |
|  +-------------------+     +-------------------+  |
|  | ISerializer<T>    |     | IDeserializer<T>  |  |
|  +-------------------+     +-------------------+  |
|  | +computeSerialized|     | +deserialize()    |  |
|  |   Size()          |     | +deserializeTlv() |  |
|  | +serialize()      |     +-------------------+  |
|  | +serializeTlv()   |                          |
|  +-------------------+                          |
|                                                   |
|  +-------------------+                            |
|  | SerializerTypes   |                            |
|  +-------------------+                            |
|  | - ByteOrder       |                            |
|  | - SerializerSett. |                            |
|  | - EWireType       |                            |
|  +-------------------+                            |
|                                                   |
|  +-------------------+                            |
|  | SerializeBasic... |                            |
|  +-------------------+                            |
|  | +serialize()      |                            |
|  +-------------------+                            |
|                                                   |
|  +-------------------+                            |
|  | SerializerUtils   |                            |
|  +-------------------+                            |
|  | +isBasicType()    |                            |
|  | +swap()           |                            |
|  +-------------------+                            |
|                                                   |
+---------------------------------------------------+
```

## Textual Description

The `src/serializer` module provides a flexible, extensible framework for serialization and deserialization of data types, designed for use in the SOME/IP communication stack. The design is based on generic C++ templates and interface classes, allowing for custom serializers/deserializers for any data type.

### Key Components

- **ISerializer<T>**: Abstract interface for serializing objects of type `T`.
  - Methods:
    - `computeSerializedSize(const T*)`: Returns the size needed to serialize an object.
    - `serialize(uint8_t*, uint32_t, const T*)`: Serializes an object into a buffer.
    - `serializeTlv(...)`: (Optional) Serializes with TLV encoding (default: not supported).
    - TLV-aware overloads are provided for extensibility.

- **IDeserializer<T>**: Abstract interface for deserializing objects of type `T`.
  - Methods:
    - `deserialize(const uint8_t*, uint32_t, T*, uint32_t&)`: Deserializes from a buffer.
    - `deserializeTlv(...)`: (Optional) TLV decoding (default: not supported).

- **SerializerTypes.hpp**: Defines enums and structs for serialization settings:
  - `ByteOrder`: Endianness (Big, Little, Opaque)
  - `SerializerSettings`: Per-deployment serialization configuration (byte order, length field sizes, etc.)
  - `EWireType`: Wire type enum for protocol-level type discrimination

- **SerializeBasicTypes.hpp**: Implements serialization for primitive types and strings, using the settings from `SerializerSettings`.

- **SerializerUtils.hpp**: Utility templates and functions for type traits, byte swapping, and enable-if logic for SFINAE-based specialization.

- **SerializerComputeSize.hpp**: Functions to compute the serialized size of various types, including primitives, strings, and arrays.

### Design Highlights

- **Template-based**: All interfaces and helpers are templated for maximum flexibility and type safety.
- **TLV Support**: Optional TLV (Type-Length-Value) encoding/decoding is supported via virtual methods, with default implementations signaling unsupported features.
- **Extensibility**: New types can be supported by implementing the `ISerializer`/`IDeserializer` interfaces for those types.
- **Settings-driven**: Serialization behavior (endianness, length field size, etc.) is controlled by the `SerializerSettings` struct, allowing for per-deployment customization.
- **No Ownership**: The interfaces do not manage memory for the objects being (de)serialized; the caller is responsible for buffer management.

---


## Configuration

The serializer can be configured using the `SerializerSettings` struct (see `SerializerTypes.hpp`). This struct allows you to control:

- **Byte Order**: Set via `byteOrder` (BigEndian, LittleEndian, Opaque)
- **Length Field Sizes**: Configure the number of bytes used for length fields in strings, arrays, vectors, maps, structs, and unions (e.g., `sizeStringLengthField`, `sizeArrayLengthField`, etc.)
- **Dynamic Length Field Size**: The `isDynamicLengthFieldSize` flag enables dynamic calculation of length field size, useful for TLV encoding.

These settings are passed to all serialization/deserialization functions and determine how data is packed/unpacked on the wire. This enables adaptation to different protocol or deployment requirements.

---

## Supported Data Types

### Base Types
- All C++ arithmetic types (integers, floating point, enums)
- `bool`
- `std::string` (with UTF-8 BOM and null terminator)

### Complex Types
- `std::array<T, N>` (fixed-size arrays of base types)
- `std::vector<T>` (variable-size arrays of base types, including special handling for `std::vector<bool>`)
- `std::map<K, V>` (maps of base types)

All of these types are supported via template specializations and SFINAE-based enable-if logic. For custom or nested types, you can implement your own `ISerializer`/`IDeserializer` specializations.

---

This design enables robust, AUTOSAR-compliant serialization for automotive middleware, with clear separation of interface, utility, and implementation logic.
