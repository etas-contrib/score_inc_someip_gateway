<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# ARXML and Serializer — Comprehensive Reference

> **Scope:** This document consolidates all ARXML- and serializer-related documentation from
> `docs/` and `src/serializer/` into a single, authoritative reference for the
> `inc_someip_gateway` project.
>
> **Source files merged:**
> `docs/arxml_overview.md` · `docs/arxml_datatype_analysis.md` ·
> `docs/arxml_config_serializer_correlation.md` · `docs/arxml_to_json_schema.md` ·
> `docs/config_parameters_overview.md` · `docs/serializer_class_diagram.md` ·
> `src/serializer/SUMMARY.md`

---

## Table of Contents

1. [ARXML Overview](#1-arxml-overview)
2. [ARXML Data Types and C++ Mapping](#2-arxml-data-types-and-c-mapping)
3. [JSON Configuration Schema (ARXML → JSON)](#3-json-configuration-schema-arxml--json)
4. [Configuration Parameters Reference](#4-configuration-parameters-reference)
5. [ARXML ↔ Config ↔ Serializer Correlation](#5-arxml--config--serializer-correlation)
6. [Serializer Framework (src/serializer)](#6-serializer-framework-srcserializer)
7. [End-to-End Data Flow](#7-end-to-end-data-flow)

---

## 1. ARXML Overview

AUTOSAR XML (ARXML) is the canonical description language for software components, service
interfaces, and communication deployments in AUTOSAR Adaptive projects. In this project
ARXML feeds the code-generation and configuration pipeline rather than being consumed at
runtime.

### 1.1 Where ARXML fits in the repository

| Location | Purpose |
|---|---|
| `examples/car_window_sim/` | Application-level service / data-type definitions |
| `src/gatewayd/etc/` | Gateway routing and instance configuration |
| `src/network_service/interfaces/` | Transport interface definitions |
| `external/com-aap-communicationmanager` | Upstream AUTOSAR COM middleware ARXML artifacts |

### 1.2 Common structural features

| Feature | Description |
|---|---|
| XML structure | Standardized AUTOSAR schema, namespace `http://autosar.org/schema/...` |
| Top-level elements | `<AUTOSAR>`, `<AR-PACKAGES>`, `<AR-PACKAGE>`, `<ELEMENTS>` |
| UUIDs | Every element carries a `UUID` for cross-tool traceability |
| Short / long names | `<SHORT-NAME>` (machine ID), `<LONG-NAME>` (human label) |
| References | `<REF DEST="...">` links elements across packages |
| Versioning | Schema and tool versions embedded in `<ADMIN-DATA>` |

### 1.3 Generic vs. application-specific parameters

| Parameter / Section | Generic | Application-specific |
|---|:---:|:---:|
| `SHORT-NAME`, `UUID`, `CATEGORY`, `DESC` | ✓ | ✓ |
| `VERSION`, `ADMIN-DATA` | ✓ | |
| `AR-PACKAGES` / `ELEMENTS` containers | ✓ | ✓ |
| `DATA-TYPE` definitions | | ✓ |
| `SERVICE-INTERFACE` | | ✓ |
| `PORT-PROTOTYPE` | | ✓ |
| `EVENT` / `OPERATION` | | ✓ |
| `INIT-VALUE` | | ✓ |
| `APPLICATION-SW-COMPONENT` | | ✓ |
| `MAPPING` elements | | ✓ |

### 1.4 Minimal ARXML example (car-window service)

```xml
<AUTOSAR>
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>CarWindowTypes</SHORT-NAME>
      <ELEMENTS>
        <APPLICATION-PRIMITIVE-DATA-TYPE>
          <SHORT-NAME>WindowPosition</SHORT-NAME>
          <CATEGORY>VALUE</CATEGORY>
          <SW-DATA-DEF-PROPS>
            <BASE-TYPE-REF DEST="SwBaseType">/AUTOSAR/uint8</BASE-TYPE-REF>
          </SW-DATA-DEF-PROPS>
        </APPLICATION-PRIMITIVE-DATA-TYPE>
      </ELEMENTS>
    </AR-PACKAGE>
    <AR-PACKAGE>
      <SHORT-NAME>CarWindowService</SHORT-NAME>
      <ELEMENTS>
        <SERVICE-INTERFACE>
          <SHORT-NAME>CarWindowControl</SHORT-NAME>
          <OPERATIONS>
            <CLIENT-SERVER-OPERATION>
              <SHORT-NAME>SetWindowPosition</SHORT-NAME>
              <ARGUMENTS>
                <ARGUMENT-DATA-PROTOTYPE>
                  <SHORT-NAME>position</SHORT-NAME>
                  <TYPE-TREF DEST="ApplicationPrimitiveDataType">
                    /CarWindowTypes/WindowPosition
                  </TYPE-TREF>
                </ARGUMENT-DATA-PROTOTYPE>
              </ARGUMENTS>
            </CLIENT-SERVER-OPERATION>
          </OPERATIONS>
        </SERVICE-INTERFACE>
      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>
```

---

## 2. ARXML Data Types and C++ Mapping

### 2.1 ARXML type hierarchy

| ARXML Element | Semantics | C++ equivalent |
|---|---|---|
| `APPLICATION-PRIMITIVE-DATA-TYPE` | Scalar (integer, float, bool) | `uint8_t`, `float`, `bool` |
| `APPLICATION-ARRAY-DATA-TYPE` | Fixed or variable-length sequence | `std::array<T,N>` / `std::vector<T>` |
| `APPLICATION-RECORD-DATA-TYPE` | Struct / composite type | `struct` / `class` |
| `ENUMERATION` | Enumerated values with base type | `enum class` |
| `BASE-TYPE-REF` | Reference to a primitive base type | stdint types |

### 2.2 Building complex types

ARXML supports arbitrary nesting. Records can contain other records or arrays:

```xml
<!-- Composite record type -->
<APPLICATION-RECORD-DATA-TYPE>
  <SHORT-NAME>WindowStatus</SHORT-NAME>
  <ELEMENTS>
    <RECORD-ELEMENT>
      <SHORT-NAME>position</SHORT-NAME>
      <TYPE-TREF>/CarWindowTypes/WindowPosition</TYPE-TREF>
    </RECORD-ELEMENT>
    <RECORD-ELEMENT>
      <SHORT-NAME>command</SHORT-NAME>
      <TYPE-TREF>/CarWindowTypes/WindowCommand</TYPE-TREF>
    </RECORD-ELEMENT>
  </ELEMENTS>
</APPLICATION-RECORD-DATA-TYPE>

<!-- Fixed-size array -->
<APPLICATION-ARRAY-DATA-TYPE>
  <SHORT-NAME>CommandArray</SHORT-NAME>
  <ELEMENT>
    <TYPE-TREF>/CarWindowTypes/WindowCommand</TYPE-TREF>
    <MAX-NUMBER-OF-ELEMENTS>4</MAX-NUMBER-OF-ELEMENTS>
  </ELEMENT>
</APPLICATION-ARRAY-DATA-TYPE>
```

Corresponding C++:

```cpp
enum class WindowCommand : uint8_t { UP = 0, DOWN = 1, STOP = 2 };
struct WindowPosition { uint8_t value; };

struct WindowStatus {
    WindowPosition position;
    WindowCommand  command;
};

using CommandArray = std::array<WindowCommand, 4>;

// Deeply nested example
struct WindowSystemState {
    std::array<WindowStatus, 2> windows;
    bool emergency_lock;
};
```

### 2.3 Full ARXML → C++ type table

| ARXML Type | C++ Representation | Serializer type trait |
|---|---|---|
| Primitive (uint8) | `uint8_t` | `isBasicType<T>()` → true |
| Primitive (float32) | `float` | `isBasicType<T>()` → true |
| Primitive (bool) | `bool` | `isBasicType<T>()` → true |
| `ENUMERATION` | `enum class : uint8_t` | `isBasicType<T>()` → true |
| Array (fixed) | `std::array<T, N>` | `EnableIfBasic<T>` overload |
| Array (dynamic) | `std::vector<T>` | `EnableIfBasicAndNotBool<T>` overload |
| Record | `struct` | Custom `ISerializer<T>` impl |
| String | `std::string` | Dedicated `serialize(string)` overload |
| Map | `std::map<K, V>` | `EnableIfBasic<K,V>` overload |

### 2.4 Middleware types (`com-aap-communicationmanager`)

The upstream com-aap middleware provides ARXML-derived runtime types used for the
actual transmission pipeline:

| Type | Role |
|---|---|
| `ServiceDefinitionConfig` | Holds service / method / event signatures derived from ARXML |
| `FlatCfgReader` | Parses `COMFlatBuffer` `.bin` files generated by the ARXML toolchain |
| `ara::com::SomeipMessage` | Wire-format SOME/IP message (header + payload) |

Application types (e.g. `WindowStatus`) are mapped to or wrapped by these middleware
types before going on the wire.

---

## 3. JSON Configuration Schema (ARXML → JSON)

### 3.1 Motivation

The `com-aap-communication-manager` toolchain transforms ARXML into `COMFlatBuffer`
binary files. A unified human-writable JSON schema is being defined as a lighter
replacement, covering both the gateway (`COMFlatBuffer` / `someip_domain_gateway`)
and the client (`COMCLIENTFlatBuffer` / user-process) configurations.

### 3.2 Top-level fields

| JSON field | Type | Consumer | Description |
|---|---|---|---|
| `functionCluster` | `"COM"` or `"COMCLIENT"` | both | Identifies the configuration target |
| `versionMajor` / `versionMinor` | integer | both | Schema version |
| `threadPoolSize` | integer | COMCLIENT | Listener thread pool size (0 = dynamic) |
| `serviceInterfaces` | array | both | Design-level service interface definitions |
| `someipDeployments` | array | both | SOME/IP deployment binding (IDs, transport) |
| `ipcDeployments` | array | COMCLIENT | IPC deployment binding |
| `ddsDeployments` | array | COMCLIENT | DDS deployment binding |
| `providedSomeipInstances` | array | both | Skeleton SOME/IP instances offered |
| `requiredSomeipInstances` | array | both | Proxy SOME/IP instances consumed |
| `e2eProfileConfigs` | array | both | E2E protection profile objects |
| `instanceSpecifiers` | array | COMCLIENT | `ara::core::InstanceSpecifier` → numeric ID |
| `secOcSecureComProps` | array | gateway | SecOC properties per transport protocol |
| `iSignalTriggerings` / `iSignals` / `iSignalGroups` | array | COMCLIENT | S2S signal definitions |
| `serviceInstanceToSignalMappings` | array | gateway | Maps events to I-Signal triggerings |

### 3.3 Byte order values

| JSON value | Meaning | Serializer constant |
|---|---|---|
| `"MostSignificantByteFirst"` | Big-endian (AUTOSAR default, PRS_SOMEIP_00368) | `ByteOrder::kBigEndian` |
| `"MostSignificantByteLast"` | Little-endian | `ByteOrder::kLittleEndian` |
| `"Opaque"` | Pass-through, no byte swap | `ByteOrder::kOpaque` |

### 3.4 Serialization-property fields per event/method

| JSON field | Maps to `SerializerSettings` field | Description |
|---|---|---|
| `byteOrder` | `byteOrder` | Wire-level endianness |
| `sizeStringLengthField` | `sizeStringLengthField` | Bytes used for string length prefix |
| `sizeArrayLengthField` | `sizeArrayLengthField` | Bytes for array length prefix |
| `sizeVectorLengthField` | `sizeVectorLengthField` | Bytes for vector length prefix |
| `sizeStructLengthField` | `sizeStructLengthField` | Bytes for struct length prefix (TLV only) |
| `isDynamicLengthFieldSize` | `isDynamicLengthFieldSize` | Dynamic length field sizing for TLV |

---

## 4. Configuration Parameters Reference

### 4.1 com-aap SOME/IP Config Reader (FlatCfgReader)

The `FlatCfgReader` parses `COMFlatBuffer` binaries generated from ARXML.

#### Service-level parameters

| Parameter | Type | Description |
|---|---|---|
| `serviceId` | `uint16_t` | SOME/IP Service ID |
| `instanceId` | `uint16_t` | SOME/IP instance ID |
| `majorVer` / `minorVer` | `uint8_t` / `uint32_t` | Interface version |
| `isProvided` | `bool` | Server (`true`) vs. client (`false`) role |
| `clientId` | `uint16_t` | Client ID for SecOC / routing |

#### Event parameters

| Parameter | Type | Description |
|---|---|---|
| `eventId` | `uint16_t` | SOME/IP Event ID |
| `serializationSize` | `size_t` | Max payload size for pre-allocation |
| `transportProtocol` | `UDP` or `TCP` | Wire transport for this event |
| `isSignalBased` | `bool` | True when event carries S2S I-Signal data |
| `publisherSlots` / `subscriberSlots` | `size_t` | PIPC ring-buffer depths |
| `maximumSegmentLength` | `uint32_t` | SOME/IP-TP segment limit |

#### Method parameters

| Parameter | Type | Description |
|---|---|---|
| `methodId` | `uint16_t` | SOME/IP Method ID |
| `requestSerializationSize` / `responseSerializationSize` | `size_t` | Max payload sizes |
| `isFireAndForget` | `bool` | No response expected |
| `maxSlots` | `size_t` | Concurrent call slots |

#### Field parameters (notifier / getter / setter)

| Parameter | Description |
|---|---|
| `fieldNotifierId` | Event ID for field change notification |
| `fieldGetterId` / `fieldSetterId` | Method IDs for getter / setter |
| `fieldHasNotifierGetterSetter` | Bitmask encoding which sub-elements are present |
| `serializationSize` | Max payload per sub-element |

### 4.2 Gatewayd routing config (`gatewayd_config.json`)

**Root keys:**

| Key | Type | Description |
|---|---|---|
| `local_service_instances` | array | Services hosted here, exposed to SOME/IP network |
| `remote_service_instances` | array | External SOME/IP services made locally available |

**Per `ServiceInstance`:**

| Field | Type | Req. | Description |
|---|---|---|---|
| `instance_specifier` | string | **yes** | Must match `serviceInstances[*].instanceSpecifier` in MW COM manifest |
| `someip_service_id` | uint16 | no | SOME/IP Service ID |
| `someip_service_version_major` | uint8 | no | Major version |
| `someip_service_version_minor` | uint32 | no | Minor version (default `0xFFFFFFFF` = any) |
| `events[*].event_name` | string | **yes** | Logical name; must match MW COM event name |
| `events[*].someip_method_id` | uint16 | no | SOME/IP Event/Method ID on the wire |

### 4.3 MW COM manifests (`mw_com_config.json`)

Both `gatewayd` and `someipd` use MW COM manifests with the same schema.

**`serviceTypes[*]`**

| Path | Description |
|---|---|
| `serviceTypeName` | Fully-qualified COM type (e.g. `/gatewayd/SomeipMessageService`) |
| `version.major` / `version.minor` | Type version |
| `bindings[*].binding` | Transport binding (`"SHM"`) |
| `bindings[*].serviceId` | COM service ID |
| `bindings[*].events[*].eventName` / `.eventId` | Per-event registration |

**`serviceInstances[*]`**

| Path | Description |
|---|---|
| `instanceSpecifier` | Unique specifier (e.g. `"gatewayd/gatewayd_messages"`) |
| `serviceTypeName` | Links to `serviceTypes[*]` |
| `instances[*].instanceId` | COM instance ID |
| `instances[*].asil-level` | ASIL level (`"QM"`, `"ASIL-B"`, …) |
| `instances[*].binding` | `"SHM"` |
| `instances[*].events[*].numberOfSampleSlots` | Ring-buffer depth |
| `instances[*].events[*].maxSubscribers` | Max subscriber count |

### 4.4 Cross-component consistency rules

| Relationship | Constraint |
|---|---|
| `gatewayd_config.json` ↔ `gatewayd/mw_com_config.json` | `instance_specifier` must equal `serviceInstances[*].instanceSpecifier` |
| `gatewayd/mw_com_config.json` ↔ `someipd/mw_com_config.json` | Same `serviceTypeName` and compatible `instanceSpecifier` names on both sides |
| ARXML / FlatCfg ↔ `gatewayd_config.json` | `someip_service_id` values must match the SOME/IP Service IDs assigned in ARXML deployments |

---

## 5. ARXML ↔ Config ↔ Serializer Correlation

### 5.1 How ARXML parameters influence serialization

| ARXML Concept | Serializer impact |
|---|---|
| Field order in `APPLICATION-RECORD-DATA-TYPE` | Determines byte order of fields in the SOME/IP payload |
| `byteOrder` in `ApSomeipTransformationProps` | Maps to `SerializerSettings.byteOrder` |
| `sizeStringLengthField` | Maps to `SerializerSettings.sizeStringLengthField` |
| `sizeArrayLengthField` | Maps to `SerializerSettings.sizeArrayLengthField` |
| `isDynamicLengthFieldSize` | Maps to `SerializerSettings.isDynamicLengthFieldSize` |
| TLV tag usage | Selects `serializeTlv()` / `deserializeTlv()` path |
| Service / Method ID | Goes into the 16-byte SOME/IP header (outside the payload serializer) |

### 5.2 Three-layer role table

| Layer | Responsibility | Example artifact |
|---|---|---|
| **ARXML** | Canonical definition of types, services, IDs, field order, constraints | `CarWindowService.arxml` |
| **Config file** | Selects which ARXML elements are active; sets deployment parameters | `gatewayd_config.json`, `COMFlatBuffer.bin` |
| **Serializer** | Encodes / decodes C++ types to/from SOME/IP wire bytes | `src/serializer/`, `someip_serializer_impl.h` |

### 5.3 Interdependency diagram

```
  +------------------------------------------+
  |           ARXML source                   |
  |  (types, service IDs, byteOrder, ...)    |
  +------------------+-----------------------+
                     |  FlatCfg / code-gen toolchain
                     v
  +------------------------------------------+
  |         Config files                     |
  |  (gatewayd_config.json,                  |
  |   mw_com_config.json,                    |
  |   COMFlatBuffer.bin)                     |
  +------------------+-----------------------+
                     |  loaded at startup
                     v
  +------------------------------------------+
  |   C++ types + SerializerSettings         |
  |  (byteOrder, lengthFieldSizes, ...)       |
  +------------------+-----------------------+
                     |  serialize() / deserialize()
                     v
  +------------------------------------------+
  |         SOME/IP wire bytes               |
  |   16-byte header + serialized payload    |
  +------------------------------------------+
```

---

## 6. Serializer Framework (`src/serializer`)

### 6.1 Module class diagram

```
namespace com::serializer
===============================================================

  +------------------------+       +------------------------+
  |   ISerializer<T>       |       |   IDeserializer<T>     |
  +------------------------+       +------------------------+
  | +computeSerializedSize()|      | +deserialize()         |
  | +computeSerializedSizeT|       | +deserializeTlv()      |
  |   lv()  [2 overloads]  |       +------------------------+
  | +serialize()            |
  | +serializeTlv()         |
  +------------------------+

  +----------------------------------------------------------+
  |  SerializerSettings  (struct, alignas(8))                |
  +----------------------------------------------------------+
  |  byteOrder               : ByteOrder (Big/Little/Opaque) |
  |  sizeStringLengthField   : uint8_t                       |
  |  sizeArrayLengthField    : uint8_t                       |
  |  sizeVectorLengthField   : uint8_t                       |
  |  sizeMapLengthField      : uint8_t                       |
  |  sizeStructLengthField   : uint8_t                       |
  |  sizeUnionLengthField    : uint8_t                       |
  |  isDynamicLengthFieldSize: bool                          |
  +----------------------------------------------------------+

  +----------------------+   +----------------------------+
  |  SerializeBasicTypes |   |  SerializerComputeSize     |
  +----------------------+   +----------------------------+
  | serialize<T>(prim)   |   | computeSerializedSize<T>() |
  | serialize(string)    |   | computeSerializedSize(str) |
  | serialize(array)     |   | computeSerializedSize(arr) |
  | serialize(vector)    |   | computeSerializedSize(vec) |
  | serialize(vector<bool>)  | computeSerializedSize(map) |
  | serialize(map<K,V>)  |   +----------------------------+
  +----------------------+

  +----------------------+   +----------------------------+
  |  SerializerUtils     |   |  EWireType (enum)          |
  +----------------------+   +----------------------------+
  | isBasicType<T>()     |   | E_WIRETYPE_0 (8-bit)       |
  | swap<T>(val)         |   | E_WIRETYPE_1 (16-bit)      |
  | writeLengthField()   |   | E_WIRETYPE_2 (32-bit)      |
  | checkIfValueMustSwap |   | E_WIRETYPE_3 (64-bit)      |
  | EnableIfBasic<T>     |   | E_WIRETYPE_4 (complex, static) |
  | EnableIfNotBasic<T>  |   | E_WIRETYPE_5..7 (complex,  |
  +----------------------+   |   1/2/4-byte length field) |
                             +----------------------------+

Concrete-factory pattern:

  ISerializerBase <-- ISerializer<T>
                             ^
              +--------------+--------------+
  UInt8BaseSerializer  UInt16BaseSerializer  StringBaseSerializer
              ^                 ^                    ^
          UInt8Node         UInt16Node           StringNode

  CompoundSerializer<T>   SerializerDecorator<T>
           ^
  AbstractSomeIpFactory <-- ConcreteSomeIpFactory
```

### 6.2 Configuration via `SerializerSettings`

```cpp
com::serializer::SerializerSettings settings;
settings.byteOrder               = ByteOrder::kBigEndian; // AUTOSAR default
settings.sizeStringLengthField   = 4;  // 4-byte length prefix for strings
settings.sizeArrayLengthField    = 4;  // 4-byte length prefix for arrays
settings.sizeVectorLengthField   = 4;
settings.sizeMapLengthField      = 4;
settings.sizeStructLengthField   = 0;  // 0 = no length prefix (static struct)
settings.isDynamicLengthFieldSize = false;
```

Every `serialize()` / `deserialize()` free function accepts a `SerializerSettings`
value, so different deployments can coexist in the same process.

### 6.3 Supported data types

#### Base types

| C++ type | Wire size | Notes |
|---|---|---|
| `uint8_t` / `int8_t` | 1 byte | No byte swap |
| `uint16_t` / `int16_t` | 2 bytes | Swapped when `byteOrder != host` |
| `uint32_t` / `int32_t` / `float` | 4 bytes | Same |
| `uint64_t` / `int64_t` / `double` | 8 bytes | Same |
| `bool` | 1 byte | Stored as `uint8_t` (0/1) |
| Any `enum class` with arithmetic base | `sizeof(enum)` | Via `isBasicType<T>()` |
| `std::string` | `sizeStringLengthField` + 3 (BOM) + len + 1 (NUL) | UTF-8 BOM `EF BB BF` always prepended |

#### Complex types

| C++ type | Length field | Notes |
|---|---|---|
| `std::array<T, N>` | `sizeArrayLengthField` bytes (may be 0) | Static-size array of base types |
| `std::vector<T>` (T != bool) | `sizeVectorLengthField` bytes | Dynamic, each element serialized in order |
| `std::vector<bool>` | `sizeVectorLengthField` bytes | Each bool packed as 1 byte |
| `std::map<K, V>` (K, V basic) | `sizeMapLengthField` bytes | Key then value, in iteration order |

#### TLV encoding

When `isDynamicLengthFieldSize = true` and a TLV tag is present, length field sizes
are computed dynamically via `computeSerializedSizeTlv()`. The `EWireType` tag
encodes both type width and length-field size:

| EWireType | Meaning |
|---|---|
| `E_WIRETYPE_0` | 8-bit base type |
| `E_WIRETYPE_1` | 16-bit base type |
| `E_WIRETYPE_2` | 32-bit base type |
| `E_WIRETYPE_3` | 64-bit base type |
| `E_WIRETYPE_4` | Complex static type (no length field) |
| `E_WIRETYPE_5` | Complex dynamic type, 1-byte length field |
| `E_WIRETYPE_6` | Complex dynamic type, 2-byte length field |
| `E_WIRETYPE_7` | Complex dynamic type, 4-byte length field |

### 6.4 Extending the serializer for custom / composite types

1. Specialize `ISerializer<MyType>` and `IDeserializer<MyType>`.
2. Implement `computeSerializedSize()`, `serialize()`, and `deserialize()`.
3. Optionally override `serializeTlv()` / `deserializeTlv()` for TLV.
4. **The caller owns all buffers — the serializer never allocates memory.**

```cpp
class MyTypeSerializer : public com::serializer::ISerializer<MyType> {
public:
    uint32_t computeSerializedSize(const MyType* obj) override {
        return sizeof(obj->field_a) + sizeof(obj->field_b);
    }
    bool serialize(uint8_t* buf, uint32_t maxSize, const MyType* obj) override {
        bool ok = com::serializer::serialize(obj->field_a, buf, maxSize, settings_);
        ok = ok && com::serializer::serialize(obj->field_b,
                       buf + sizeof(obj->field_a),
                       maxSize - sizeof(obj->field_a), settings_);
        return ok;
    }
private:
    com::serializer::SerializerSettings settings_{};
};
```

---

## 7. End-to-End Data Flow

```
  ARXML source
  (type definitions, service IDs,
   byteOrder, lengthFieldSize, ...)
          |
          |  com-aap / ECU config DSL
          v
  COMFlatBuffer .bin  <--------------------- gatewayd_config.json
          |                                        |
          |  FlatCfgReader (startup)               |
          v                               LocalServiceInstance /
  ServiceDefinitionConfig                 RemoteServiceInstance
  (eventId, serializationSize, ...)               |
          |                                        |
          +------------------+---------------------+
                             |  SomeipMessageHandler
                             v
                   SerializerSettings
                   (byteOrder, lengthFieldSizes)
                             |
                   ISerializer<T> / free functions
                   (SerializeBasicTypes.hpp, ...)
                             |
                             v
                    SOME/IP wire frame
          +------------------------------------------+
          |  16-byte header (service/method/...)     |
          |  + serialized payload                    |
          +------------------------------------------+
                             |
                       vsomeip / someipd
                             |
                       MW COM IPC (SHM)
                             |
                          gatewayd
```

### 7.1 Concrete example — `CarWindowControl.SetWindowPosition`

| Step | Actor | Artifact |
|---|---|---|
| 1 | ARXML defines `WindowPosition` (uint8), service ID `0x1234`, method ID `0x0001`, byteOrder BigEndian | `CarWindowService.arxml` |
| 2 | FlatCfg toolchain generates `COMFlatBuffer.bin`; gateway config sets `someip_service_id: 0x1234` | `gatewayd_config.json` |
| 3 | `gatewayd` loads config → `LocalServiceInstance` created | `local_service_instance.cpp` |
| 4 | Application serializes `WindowPosition{42}` with `SerializerSettings{byteOrder=Big, sizeArrayLengthField=4}` | `src/serializer/` |
| 5 | Result: single byte `0x2A`, prepended by 16-byte SOME/IP header | wire |
| 6 | `someipd` receives frame → `SomeipMessageHandler::BuildInboundMessage()` → forwarded to gatewayd over SHM | `src/someipd/` |

---

*This is a living document. Update it when ARXML schemas, config parameters, or serializer capabilities change.*
