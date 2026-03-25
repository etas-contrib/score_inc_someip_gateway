<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Correlation Between ARXML Configuration Parameters and Serializer Format

This document explains how configuration parameters defined in ARXML files relate to the serializer format used in the vsomeip-based code, and how configuration files and serialization logic are intertwined in the system.

## 1. Overview: ARXML and Serializer Format

- **ARXML Files:**
  - Define data types, service interfaces, and communication parameters in a standardized XML format (AUTOSAR).
  - Specify the structure, types, and semantics of messages exchanged between components.
- **Serializer Format (vsomeip):**
  - Implements the logic to convert C++ representations of ARXML-defined types to/from SOME/IP wire format.
  - Relies on the type definitions and structure provided by ARXML (directly or via generated code).

## 2. How ARXML Parameters Influence Serialization

- **Data Type Mapping:**
  - ARXML data types (primitive, array, record, enum) are mapped to C++ types/structs.
  - The serializer uses this mapping to know how to pack/unpack bytes for each field.
- **Field Order and Alignment:**
  - The order of fields in ARXML records determines the serialization order in the wire format.
  - Alignment and padding rules (if any) are derived from ARXML or AUTOSAR conventions.
- **Value Ranges and Constraints:**
  - ARXML may specify min/max values, scaling, or units, which can be enforced or validated during serialization/deserialization.
- **Service/Message IDs:**
  - ARXML defines service and method/event IDs, which are used in the SOME/IP header and serializer logic to route and interpret messages.

## 3. How Configuration Files and Serialization Are Intertwined

- **Config Files (JSON/Flatbuffer):**
  - Specify which ARXML-defined services, methods, and data types are active/configured for a given deployment.
  - Provide runtime parameters (e.g., instance IDs, offered/consumed services, network endpoints) that guide the serializer on what to expect and how to interpret messages.
- **Serializer Initialization:**
  - At startup, the system reads the config file (e.g., `gatewayd_config.bin` or `mw_com_config.json`).
  - The config file references ARXML-defined types and services, informing the serializer which types to handle and how.
- **Dynamic vs. Static Binding:**
  - Some systems generate C++ code from ARXML at build time (static binding), while others may interpret ARXML/config at runtime (dynamic binding).
  - In this codebase, the config file acts as the bridge: it encodes ARXML-derived structure for the serializer to use at runtime.

## 4. Example: Data Flow

1. **ARXML defines** a service interface `CarWindowService` with a method `SetWindowPosition(WindowPosition)`.
2. **Config file** specifies that `CarWindowService` is to be offered/consumed, with instance IDs and endpoints.
3. **Code generator** or manual mapping creates C++ types for `WindowPosition` and the service interface.
4. **Serializer** uses the C++ types and config info to serialize/deserialize SOME/IP messages according to the ARXML-defined structure.

## 5. Summary Table: ARXML, Config, and Serializer Roles

| Layer         | Role/Responsibility                                      | Example Artifact                |
|---------------|---------------------------------------------------------|---------------------------------|
| ARXML         | Define types, services, IDs, field order, constraints   | `CarWindowService.arxml`        |
| Config File   | Select/activate ARXML elements, set runtime parameters  | `gatewayd_config.json`/`.bin`   |
| Serializer    | Encode/decode C++ types per ARXML/config structure      | `someip_serializer_impl.h`      |

## 6. Interdependency Diagram

```
[ARXML Types/Services]
        ↓
  [Config File: selects, parametrizes]
        ↓
  [C++ Types & Serializer Logic]
        ↓
  [SOME/IP Wire Format]
```

## 7. Conclusion

- ARXML files define the canonical structure and semantics of data and services.
- Config files select and parameterize which ARXML elements are active in a deployment.
- The serializer uses both the ARXML-derived C++ types and the config file to correctly encode/decode SOME/IP messages.
- All three layers are tightly coupled: changes in ARXML or config require corresponding updates in the serializer logic or deployment.

---
*This document clarifies the relationship between ARXML configuration, runtime config files, and the serializer format in the vsomeip-based system.*
