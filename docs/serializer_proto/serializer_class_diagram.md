<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Framework Class Diagram

This diagram describes the class structure for the SOME/IP serializer framework as implemented in the gateway project.

```
+---------------------+           +---------------------+
|   ISerializerBase   |<----------|  ISerializer<T>     |
+---------------------+           +---------------------+
                                    | +serialize()      |
                                    | +deserialize()    |
                                    +-------------------+
                                            ^
                                            |
+---------------------+   +---------------------+   +---------------------+
| UInt8BaseSerializer |   | UInt16BaseSerializer|   | StringBaseSerializer|
+---------------------+   +---------------------+   +---------------------+
         ^                        ^                        ^
         |                        |                        |
         |                        |                        |
+---------------------+   +---------------------+   +---------------------+
|   Node<uint8_t>     |   |   Node<uint16_t>    |   |   Node<string>      |
+---------------------+   +---------------------+   +---------------------+
         ^                        ^                        ^
         |                        |                        |
+---------------------+   +---------------------+   +---------------------+
|    UInt8Node        |   |    UInt16Node       |   |    StringNode       |
+---------------------+   +---------------------+   +---------------------+

+---------------------------+
| CompoundSerializer<T>     |
+---------------------------+
| +add_member_serializer()  |
| +serialize()              |
| +deserialize()            |
+---------------------------+
         ^
         |
+---------------------------+
| SerializerDecorator<T>    |
+---------------------------+
| +serialize()              |
| +deserialize()            |
+---------------------------+

+-----------------------------+
| AbstractSomeIpFactory       |
+-----------------------------+
| +create_uint8_serializer()  |
| +create_uint16_serializer() |
| +create_string_serializer() |
| +create_compound_serializer()|
+-----------------------------+
         ^
         |
+-----------------------------+
| ConcreteSomeIpFactory       |
+-----------------------------+
| +create_string_uint16_compound() |
| +create_mycompound_serializer()  |
+-----------------------------+
```

---

**Legend:**
- `ISerializerBase` is the non-templated base for all serializers.
- `ISerializer<T>` is the main interface for (de)serialization.
- `UInt8BaseSerializer`, `UInt16BaseSerializer`, `StringBaseSerializer` are concrete implementations for primitive types.
- `CompoundSerializer<T>` is for composite types.
- `SerializerDecorator<T>` allows decorator pattern for serializers.
- `Node<T>` is the abstract data holder; `UInt8Node`, `UInt16Node`, `StringNode` are concrete implementations.
- `AbstractSomeIpFactory` is the abstract factory; `ConcreteSomeIpFactory` implements it for your types.
