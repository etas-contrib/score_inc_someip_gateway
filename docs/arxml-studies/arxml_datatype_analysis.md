<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Analysis: Data Types in ARXML and Their Representation in vsomeip Code



**Update:** This extended analysis now also considers data types defined under `external/com-aap-communicationmanager` and their integration with the rest of the system.
## 6. Data Types from external/com-aap-communicationmanager

The `external/com-aap-communicationmanager` directory provides AUTOSAR-compliant communication middleware, including its own ARXML-derived data types and service definitions. These types are typically used for:

- Communication service interfaces (e.g., message transfer, service discovery)
- Standardized AUTOSAR data types (e.g., `ara::com`, `ara::log` types)
- Middleware configuration and diagnostics

### Example Data Types

- **ServiceDefinitionConfig**: Represents service interface definitions, including method/event signatures and data types.
- **FlatCfgReader**: Reads and interprets ARXML/Flatbuffer-based configuration, exposing types and values to the application.
- **ara::com::SomeipMessage**: A struct representing a SOME/IP message, including header and payload fields.

**Example (C++):**
```cpp
// Pseudocode for a SOME/IP message type from com-aap
struct SomeipMessage {
  std::uint32_t service_id;
  std::uint32_t method_id;
  std::vector<std::uint8_t> payload;
};

// Service definition config type
struct ServiceDefinitionConfig {
  std::string service_name;
  std::vector<MethodConfig> methods;
  // ...
};
```

### Integration with Application Data Types

Application-specific types (e.g., `WindowStatus`, `CommandArray`) are mapped to or wrapped by the middleware types for transmission. The FlatCfgReader and related config APIs ensure that the correct type information is available at runtime for serialization, deserialization, and validation.

**Integration Example:**
1. ARXML defines a service and its data types (application-specific and standard).
2. com-aap-communicationmanager provides the runtime and config reader for these types.
3. Application code uses the generated or wrapped types for business logic and communication.

---

This document explains how data types defined in AUTOSAR ARXML files are described and mapped in the vsomeip-based code within this repository. It also lists concrete examples of such data types as used in the test cases.

## 1. How Data Types Are Described in ARXML

In AUTOSAR ARXML files, data types are defined using XML elements such as:
- `<APPLICATION-PRIMITIVE-DATA-TYPE>`: For basic types (e.g., integer, boolean, float)
- `<APPLICATION-ARRAY-DATA-TYPE>`: For arrays
- `<APPLICATION-RECORD-DATA-TYPE>`: For structs/records
- `<COMPLEX-TYPE>`: For nested or composite types
- `<BASE-TYPE-REF>`: Reference to a base type (e.g., uint8, float32)

Each data type typically includes:
- `<SHORT-NAME>`: The symbolic name of the type
- `<CATEGORY>`: The kind of type (e.g., VALUE, ARRAY, RECORD)
- `<SW-DATA-DEF-PROPS>`: Properties such as base type, min/max, etc.

**Example (ARXML):**
```xml
<APPLICATION-PRIMITIVE-DATA-TYPE>
  <SHORT-NAME>WindowPosition</SHORT-NAME>
  <CATEGORY>VALUE</CATEGORY>
  <SW-DATA-DEF-PROPS>
    <BASE-TYPE-REF>uint8</BASE-TYPE-REF>
  </SW-DATA-DEF-PROPS>
</APPLICATION-PRIMITIVE-DATA-TYPE>
```


## 2.1. Construction of Complex Data Types in ARXML

Complex data types in ARXML are constructed using combinations of primitive types, arrays, and records (structs). The most common ARXML elements for complex types are:

- `<APPLICATION-RECORD-DATA-TYPE>`: Defines a struct-like type composed of multiple fields, each of which can be a primitive or another complex type.
- `<APPLICATION-ARRAY-DATA-TYPE>`: Defines an array of a given type and length.
- `<COMPLEX-TYPE>`: Used for nested or composite types (sometimes as a container for records and arrays).

**Example (ARXML Record Type):**
```xml
<APPLICATION-RECORD-DATA-TYPE>
  <SHORT-NAME>WindowStatus</SHORT-NAME>
  <ELEMENTS>
    <RECORD-ELEMENT>
      <SHORT-NAME>position</SHORT-NAME>
      <TYPE-TREF>WindowPosition</TYPE-TREF>
    </RECORD-ELEMENT>
    <RECORD-ELEMENT>
      <SHORT-NAME>command</SHORT-NAME>
      <TYPE-TREF>WindowCommand</TYPE-TREF>
    </RECORD-ELEMENT>
  </ELEMENTS>
</APPLICATION-RECORD-DATA-TYPE>
```

**Example (ARXML Array Type):**
```xml
<APPLICATION-ARRAY-DATA-TYPE>
  <SHORT-NAME>CommandArray</SHORT-NAME>
  <ELEMENT>
    <TYPE-TREF>WindowCommand</TYPE-TREF>
    <MAX-NUMBER-OF-ELEMENTS>4</MAX-NUMBER-OF-ELEMENTS>
  </ELEMENT>
</APPLICATION-ARRAY-DATA-TYPE>
```

These definitions allow for the construction of arbitrarily complex/nested types by referencing other types.

## 2.2. Representation of Complex Data Types in vsomeip Code

In the vsomeip-based C++ code, complex ARXML types are mapped as follows:

- **Record Types:** → `struct` or `class` with fields for each element.
- **Array Types:** → `std::array<T, N>` or `std::vector<T>`.
- **Nested Types:** → Structs containing other structs or arrays.

**Example (C++ Struct for Record Type):**
```cpp
struct WindowStatus {
    WindowPosition position;
    WindowCommand command;
};
```

**Example (C++ Array Type):**
```cpp
using CommandArray = std::array<WindowCommand, 4>;
```

**Example (Nested Complex Type):**
```cpp
struct WindowSystemState {
    std::array<WindowStatus, 2> window_statuses;
    bool emergency_lock;
};
```

The serializer logic recursively serializes each field according to its type, supporting deeply nested and composite types as defined in ARXML.

---

In the vsomeip-based code, ARXML data types are mapped to C++ types and structures. The mapping is typically performed by code generators or manually, following these conventions:

- **Primitive Types:**
  - ARXML `uint8` → `std::uint8_t` or `uint8_t` in C++
  - ARXML `boolean` → `bool` in C++
- **Array Types:**
  - ARXML array types → `std::array<T, N>` or `std::vector<T>`
- **Record/Struct Types:**
  - ARXML record types → `struct` or `class` in C++
- **Enums:**
  - ARXML enumerations → `enum class` or `enum` in C++

**Example (C++):**
```cpp
// From car_window_types.h
struct WindowPosition {
    std::uint8_t value;
};

enum class WindowCommand : std::uint8_t {
    UP = 0,
    DOWN = 1,
    STOP = 2
};
```

The vsomeip serializer/deserializer (see `src/someipd/someip_serializer.h` and `someip_serializer_impl.h`) is responsible for converting between these C++ types and the SOME/IP wire format, using the type information derived from ARXML.

## 3. Examples of Data Types in Test Cases

The test cases (see `tests/cpp/test_main.cpp` and related files) include examples of ARXML-derived data types:

| Data Type         | ARXML Definition         | C++ Representation         | Example Usage in Test |
|-------------------|-------------------------|----------------------------|-----------------------|
| WindowPosition    | Primitive (uint8)       | `struct WindowPosition`    | Serialization test    |
| WindowCommand     | Enum                    | `enum class WindowCommand` | Enum serialization    |
| WindowStatus      | Record/struct           | `struct WindowStatus`      | Struct serialization  |
| CommandArray      | Array                   | `std::array<WindowCommand, N>` | Array test        |

**Example Test Snippet:**
```cpp
// In test_main.cpp
WindowPosition pos{42};
EXPECT_EQ(serialize(pos), expected_bytes);

WindowCommand cmd = WindowCommand::UP;
EXPECT_EQ(serialize(cmd), expected_enum_bytes);

WindowStatus status{pos, cmd};
EXPECT_EQ(serialize(status), expected_struct_bytes);
```

## 4. Summary Table: ARXML to vsomeip Mapping

| ARXML Type                      | C++/vsomeip Representation         | Example File                |
|----------------------------------|------------------------------------|-----------------------------|
| APPLICATION-PRIMITIVE-DATA-TYPE  | `std::uint8_t`, `bool`, etc.       | car_window_types.h          |
| APPLICATION-ARRAY-DATA-TYPE      | `std::array<T, N>`, `std::vector`  | car_window_types.h          |
| APPLICATION-RECORD-DATA-TYPE     | `struct` or `class`                | car_window_types.h          |
| ENUMERATION                      | `enum class` or `enum`             | car_window_types.h          |

## 5. References
- ARXML files (see `examples/` and `src/` folders)
- C++ type definitions: `examples/car_window_sim/src/car_window_types.h`
- SOME/IP serializer: `src/someipd/someip_serializer.h`, `src/someipd/someip_serializer_impl.h`
- Test cases: `tests/cpp/test_main.cpp`

---
*This document provides a mapping between ARXML data type definitions and their C++/vsomeip representations, with concrete examples from the test suite.*
