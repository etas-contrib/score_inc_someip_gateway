<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Framework — Architecture & Software Design

> **Module:** `src/reader/`
> **Language:** C++17 (header-only, Bazel)
> **Namespace:** `reader`
> **Specification Basis:** AUTOSAR PRS SOME/IP Protocol (PRS_SOMEIP_00084 – 00373)

---

## 1. Purpose & Scope

The `src/reader/` module provides a **layered, extensible serialization framework** for
SOME/IP payloads. It bridges the gap between the low-level wire codec (`src/serializer/`)
and application-specific data types defined in AUTOSAR ARXML.

### Goals

| Goal | Rationale |
|---|---|
| **Strict separation of concerns** | Wire-format details (byte order, length fields, TLV) are isolated from application struct layout. |
| **Single point of control for base types** | All primitive/container serializers are created through one factory, ensuring consistent wire-format configuration across the entire application. |
| **Composability** | Struct serializers are composed purely from base-type serializers using the Composite pattern — no hand-written byte-offset code. |
| **Swappable implementations** | The base-type factory is injected via interface; alternative implementations (mock, little-endian, IPC-optimised) can be substituted without touching application-level code. |
| **Code-generation readiness** | Layer 4 (concrete application factory) mirrors the output of an ARXML → C++ code generator. All deployment constants, settings presets, and struct member bindings are separated cleanly. |

### Non-Goals

- This module does **not** handle the 16-byte SOME/IP message header (that is `SomeipMessageHandler` in `src/someipd/`).
- It does **not** perform service discovery, routing, or transport.
- It does **not** parse ARXML at runtime; configuration is compile-time (`constexpr`).

---

## 2. Layered Architecture

The framework follows a strict **four-layer** architecture. Each layer depends only
on the layer(s) below it, never upward.

```
┌─────────────────────────────────────────────────────────────────────┐
│  Layer 4 — Concrete Application Factory                             │
│  car_window_factory.h                                               │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ CarWindowFactory : AbstractCarWindowFactory                   │  │
│  │   create_window_info_serializer()   → CompoundSerializer<>   │  │
│  │   create_window_control_serializer()→ CompoundSerializer<>   │  │
│  │   Deployment constants, Settings presets, Bundle helpers      │  │
│  └───────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 3 — Abstract Application Factory                             │
│  abstract_app_factory.h                                             │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ AbstractCarWindowFactory                                      │  │
│  │   pure virtual: create_window_info_serializer()               │  │
│  │   pure virtual: create_window_control_serializer()            │  │
│  └───────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 2 — Base-Type Serializer Factory + Implementations           │
│  base_type_serializers.h                                            │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ BaseTypeSerializerFactory (abstract)                          │  │
│  │   create_{uint8..uint64, int8..int64}_serializer()            │  │
│  │   create_{float, double, bool, string}_serializer()           │  │
│  │   create_{enum, vector, array, map}_serializer<T>()           │  │
│  │   create_{bool_vector, app_error, tlv}_serializer()           │  │
│  ├───────────────────────────────────────────────────────────────┤  │
│  │ SomeipBaseTypeSerializerFactory : BaseTypeSerializerFactory   │  │
│  │   parameterized by SerializerSettings                         │  │
│  ├───────────────────────────────────────────────────────────────┤  │
│  │ Serializer implementations:                                   │  │
│  │   PrimitiveSerializer<T>, BoolSerializer,                     │  │
│  │   SomeipStringSerializer, VectorSerializer<T>,                │  │
│  │   BoolVectorSerializer, ArraySerializer<T,N>,                 │  │
│  │   MapSerializer<K,V>, AppErrorSerializer,                     │  │
│  │   TlvSerializerDecorator<T>                                   │  │
│  └───────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  Layer 1 — Framework Types (pure interfaces)                        │
│  serializer_types.h                                                 │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ ByteVector, Node<T>, RefNode<T>                               │  │
│  │ ISerializerBase, ISerializer<T>                               │  │
│  │ SerializerDecorator<T>                                        │  │
│  │ IMemberBinding<Outer>, MemberBinding<Outer,Member>            │  │
│  │ CompoundSerializer<T>                                         │  │
│  └───────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  External — Low-level Wire Codec (not part of src/reader/)          │
│  src/serializer/  (8 headers, com::serializer namespace)            │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ SerializerTypes.hpp   — ByteOrder, SerializerSettings, EWire │  │
│  │ SerializerUtils.hpp   — swap, length fields, TLV tags        │  │
│  │ SerializerComputeSize.hpp — computeSerializedSize()          │  │
│  │ SerializeBasicTypes.hpp   — serialize() overloads            │  │
│  │ DeserializeBasicTypes.hpp — deserialize() overloads          │  │
│  │ AppErrorSerializers.hpp   — AppError struct, ser/deser       │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### Dependency Rule

```
Layer 4  →  Layer 3  →  Layer 1
   ↓                       ↑
Layer 2  ──────────────────┘
   ↓
src/serializer/  (external low-level codec)
```

- **Layer 1** has **zero** external dependencies (pure STL).
- **Layer 2** depends on Layer 1 and `src/serializer/`.
- **Layer 3** depends on Layer 1 and application domain types only.
- **Layer 4** depends on Layers 1–3 and `src/serializer/` (for `SerializerSettings` constants).

---

## 3. Design Patterns

The framework employs four Gang of Four (GoF) design patterns:

### 3.1 Abstract Factory (GoF)

Two levels of abstract factory provide separation between base-type creation and
application-type creation.

```
                    BaseTypeSerializerFactory          AbstractCarWindowFactory
                         (Layer 2)                          (Layer 3)
                    ┌──────────────────┐              ┌──────────────────────┐
                    │ «interface»       │              │ «interface»           │
                    │ create_uint8()    │              │ create_window_info()  │
                    │ create_uint16()   │              │ create_window_ctrl()  │
                    │ create_string()   │              └──────────┬───────────┘
                    │ create_enum<E>()  │                         │
                    │ create_vector<T>()│                         │
                    │ ...               │                         │
                    └────────┬─────────┘                         │
                             │                                   │
                    ┌────────┴─────────┐              ┌──────────┴───────────┐
                    │ SomeipBaseType    │◄─────────────│ CarWindowFactory     │
                    │ SerializerFactory │  injected    │ (Layer 4)            │
                    │ (Layer 2)        │  via ctor     │ compose from base    │
                    └──────────────────┘              └──────────────────────┘
```

**Key Invariant:** `CarWindowFactory` creates struct serializers using **only**
serializers obtained from the injected `BaseTypeSerializerFactory`. It never
directly instantiates `PrimitiveSerializer`, `SomeipStringSerializer`, etc.

### 3.2 Composite (GoF)

`CompoundSerializer<T>` implements the Composite pattern. A struct serializer is
a composition of member serializers, each bound via `MemberBinding<Outer, Member>`.

```
              ISerializer<WindowInfo>
                      ▲
                      │
            CompoundSerializer<WindowInfo>
                  ┌───┴───┐
                  │bindings│
                  └───┬───┘
          ┌───────────┼───────────────────┐
  MemberBinding       │          MemberBinding
  <WindowInfo,    (order)      <WindowInfo,
   uint32_t>                    WindowState>
      │                             │
 ISerializer<uint32_t>    ISerializer<WindowState>
 (from factory)           (from factory)
```

Members are serialized/deserialized in registration order, matching the SOME/IP
flat struct wire layout (PRS_SOMEIP_00092).

### 3.3 Decorator (GoF)

`SerializerDecorator<T>` is a transparent forwarding wrapper. The primary use
case is `TlvSerializerDecorator<T>`, which wraps any inner serializer and
prepends a 2-byte TLV tag (+ optional length field) per PRS_SOMEIP_00373.

```
  ISerializer<uint32_t>
         ▲
         │
  SerializerDecorator<uint32_t>
         ▲
         │
  TlvSerializerDecorator<uint32_t>
         │
         ├── tag:   [wireType|dataId]  (2 bytes)
         ├── len:   [payloadLength]    (0–4 bytes, wire-type dependent)
         └── inner: PrimitiveSerializer<uint32_t>
```

### 3.4 Strategy (GoF)

The byte-order policy is a runtime strategy selected via `SerializerSettings.byteOrder`:

| Value | Behavior |
|---|---|
| `kBigEndian` | Always swap on little-endian hosts (network byte order, per PRS_SOMEIP_00368) |
| `kLittleEndian` | Always swap on big-endian hosts |
| `kOpaque` | No swap — native host order (used for IPC on same-endian systems) |

The swap decision is computed once at serializer construction time (`detail::needsSwap()`)
and cached as a `bool` member for zero-overhead runtime checks.

---

## 4. File Inventory

| File | Layer | LOC | Responsibility |
|---|---|---|---|
| `serializer_types.h` | 1 | ~230 | Pure framework interfaces, data holders, Decorator/Composite patterns |
| `base_type_serializers.h` | 2 | ~700 | Abstract + concrete base-type factory, all serializer implementations |
| `abstract_app_factory.h` | 3 | ~70 | Abstract interface for application-specific serializer creation |
| `car_window_factory.h` | 4 | ~210 | Concrete CarWindow factory, deployment constants, settings presets |
| `extended_serializer_test.cpp` | test | ~530 | 16-section test suite covering all layers |
| `car_window_arxml_factory_demo.cpp` | demo | ~120 | Interactive demo with hexdump output |
| `BUILD.bazel` | build | ~40 | Bazel targets: library, test, demo |

---

## 5. Type Catalogue

### 5.1 Framework Types (Layer 1)

| Type | Kind | Description |
|---|---|---|
| `ByteVector` | alias | `std::vector<uint8_t>` — the universal wire buffer |
| `Node<T>` | abstract class | Mutable typed data holder; abstracts ownership |
| `RefNode<T>` | concrete class | Lightweight `Node<T>` wrapping an external `T&` |
| `ISerializerBase` | abstract class | Non-templated base for heterogeneous containers |
| `ISerializer<T>` | abstract class | Core serializer interface with 4 methods |
| `SerializerDecorator<T>` | abstract class | Transparent forwarding wrapper (Decorator base) |
| `IMemberBinding<Outer>` | abstract class | Type-erased member serialization within a struct |
| `MemberBinding<Outer, Member>` | concrete class | Binds an `ISerializer<Member>` to getter/setter lambdas |
| `CompoundSerializer<T>` | concrete class | Composite serializer: ordered list of member bindings |

### 5.2 ISerializer\<T\> Interface Contract

```cpp
class ISerializer<T> : public ISerializerBase {
    // Serialize: value → bytes
    virtual ByteVector serialize(const Node<T>& obj) const = 0;
    virtual void serialize_to(const Node<T>& obj, ByteVector& out) const;

    // Deserialize: bytes → value
    virtual void deserialize(const ByteVector& data, Node<T>& obj) const = 0;
    virtual void deserialize_from(const ByteVector& data,
                                  std::size_t& offset, Node<T>& obj) const;
};
```

- `serialize()` — allocates and returns a new `ByteVector`.
- `serialize_to()` — **appends** to an existing buffer (zero-copy path).
- `deserialize()` — reads from byte 0 of the buffer.
- `deserialize_from()` — reads at `offset`, **advances** `offset` by bytes consumed.

The `_to`/`_from` variants enable efficient **streaming** serialization of compound types
without intermediate buffer copies.

### 5.3 Serializer Implementations (Layer 2)

| Class | Template | Wire Format | PRS Reference |
|---|---|---|---|
| `PrimitiveSerializer<T>` | `T`: arithmetic or enum | Raw `sizeof(T)` bytes, byte-order-aware | PRS_SOMEIP_00368 |
| `BoolSerializer` | — | 1 byte: `0x00` (false) / `0x01` (true) | PRS_SOMEIP_00106 |
| `SomeipStringSerializer` | — | `[length][BOM:EF BB BF][chars][null]` | PRS_SOMEIP_00084–00086 |
| `VectorSerializer<T>` | `T`: arithmetic/enum | `[length][elem₁]...[elemₙ]` | PRS_SOMEIP_00128 |
| `BoolVectorSerializer` | — | `[length][0x00/0x01]...` | PRS_SOMEIP_00128 |
| `ArraySerializer<T,N>` | `T`: arithmetic/enum | `[length?][elem₁]...[elemₙ]` (fixed N) | PRS_SOMEIP_00106 |
| `MapSerializer<K,V>` | `K,V`: arithmetic/enum | `[length][k₁v₁]...[kₙvₙ]` | PRS_SOMEIP_00128 |
| `AppErrorSerializer` | — | `[domain:u64][code:i32]` (always BE) | Application-level error |
| `TlvSerializerDecorator<T>` | any `T` | `[tag:2B][length?:0–4B][payload]` | PRS_SOMEIP_00373 |

### 5.4 Configuration: SerializerSettings

From `src/serializer/SerializerTypes.hpp` (`com::serializer::SerializerSettings`):

```cpp
struct SerializerSettings {
    ByteOrder byteOrder;               // kBigEndian / kLittleEndian / kOpaque
    uint8_t sizeStringLengthField;     // 0, 1, 2, or 4 bytes
    uint8_t sizeArrayLengthField;      // 0 = no prefix (fixed-size arrays)
    uint8_t sizeVectorLengthField;     // typically 4 (32-bit length prefix)
    uint8_t sizeMapLengthField;        // typically 4
    uint8_t sizeStructLengthField;     // 0 = flat concat (no struct length)
    uint8_t sizeUnionLengthField;      // typically 4
    bool    isDynamicLengthFieldSize;  // AUTOSAR dynamic length encoding
};
```

These settings are derived 1:1 from the ARXML `SOMEIP-TRANSFORMATION-PROPS` element.
The framework provides compile-time presets (e.g. `kCarWindowSomeipSettings`,
`kCarWindowIpcSettings`) that mirror specific ARXML deployments.

---

## 6. Application Integration: CarWindow Example

### 6.1 Domain Types

Defined in `examples/car_window_sim/src/car_window_types.h`:

```cpp
enum class WindowState   : uint32_t { Stopped=0, Opening=1, Closing=2, Open=3, Closed=4 };
enum class WindowCommand  : uint32_t { Stop=0, Open=1, Close=2 };

struct WindowInfo    { uint32_t pos; WindowState state; };   // 8 bytes on wire
struct WindowControl { WindowCommand command; };              // 4 bytes on wire
```

### 6.2 ARXML-Derived Constants

From `SOMEIP-SERVICE-INTERFACE-DEPLOYMENT`:

| Constant | Value | ARXML Element |
|---|---|---|
| `WindowInfoDeployment::kServiceId` | 6432 | SERVICE-INTERFACE-ID |
| `WindowInfoDeployment::Events::kWindowInfo` | 1 | EVENT-ID |
| `WindowControlDeployment::kServiceId` | 6433 | SERVICE-INTERFACE-ID |
| `WindowControlDeployment::Events::kWindowControl` | 2 | EVENT-ID |

From `SOMEIP-TRANSFORMATION-PROPS / CarWindow_SomeipTransformationProps`:

| ARXML Property | Value | Settings Field |
|---|---|---|
| BYTE-ORDER | MOST-SIGNIFICANT-BYTE-FIRST | `kBigEndian` |
| SIZE-OF-STRING-LENGTH-FIELD | 32 | `4` |
| SIZE-OF-ARRAY-LENGTH-FIELD | 0 | `0` |
| SIZE-OF-STRUCT-LENGTH-FIELD | 0 | `0` (flat concat) |
| SIZE-OF-UNION-LENGTH-FIELD | 32 | `4` |
| IS-DYNAMIC-LENGTH-FIELD-SIZE | false | `false` |

### 6.3 Serialization Flow

```
Application code                          Framework
─────────────────────────────────────────────────────────

WindowInfo info{75, WindowState::Opening}
                 │
                 ▼
         RefNode<WindowInfo> node(info)
                 │
                 ▼
  CarWindowFactory::create_window_info_serializer()
         │                                    │
         │  ┌─ CompoundSerializer<WindowInfo> │
         │  │                                 │
         │  │  member 1: base_.create_uint32_serializer()
         │  │     └─ PrimitiveSerializer<uint32_t>(BE)
         │  │  member 2: base_.create_enum_serializer<WindowState>()
         │  │     └─ PrimitiveSerializer<WindowState>(BE)
         │  └─────────────────────────────────│
         ▼                                    │
  serializer->serialize(node)                 │
         │                                    │
         │  serialize_to for each member:     │
         │    pos=75   → [00 00 00 4B]        │
         │    state=1  → [00 00 00 01]        │
         ▼                                    │
  ByteVector: [00 00 00 4B 00 00 00 01]       │
              ─────────────────────────       │
                    8 bytes BE                 │
```

### 6.4 Deployment Switching

The same factory hierarchy supports multiple deployments:

```cpp
// SOME/IP deployment (big-endian, for network)
CarWindowSomeipBundle someip;
auto ser = someip.app.create_window_info_serializer();
// → bytes: [12 34 56 78 00 00 00 01]   (BE)

// IPC deployment (native byte order, no swap)
CarWindowIpcBundle ipc;
auto ser = ipc.app.create_window_info_serializer();
// → bytes: [78 56 34 12 01 00 00 00]   (LE host, native)
```

The application code is identical — only the `SerializerSettings` differ.

### 6.5 Integration Point

`examples/car_window_sim/src/car_window_serializer_helpers.cpp` provides the
C-style convenience functions used by the gateway runtime:

```cpp
std::vector<uint8_t> SerializeWindowInfo(const WindowInfo& info);
WindowInfo DeserializeWindowInfo(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> SerializeWindowControl(const WindowControl& control);
WindowControl DeserializeWindowControl(const std::vector<uint8_t>& bytes);
```

Internally these use a singleton `FactoryBundle` (Layer 2 + Layer 4) with
`kCarWindowSomeipSettings`.

---

## 7. Extension Guide

### 7.1 Adding a New Base Type

1. Implement `class NewTypeSerializer : public ISerializer<NewType>` in `base_type_serializers.h`.
2. Add `create_new_type_serializer()` as a pure virtual in `BaseTypeSerializerFactory`.
3. Implement the method in `SomeipBaseTypeSerializerFactory`.

### 7.2 Adding a New Application Domain

1. Define domain types (structs, enums) in a separate header.
2. Create `AbstractFooFactory` in a new file (Layer 3):
   ```cpp
   class AbstractFooFactory {
       virtual shared_ptr<ISerializer<FooStruct>> create_foo_serializer() const = 0;
   };
   ```
3. Create `FooFactory : AbstractFooFactory` (Layer 4):
   ```cpp
   class FooFactory : public AbstractFooFactory {
       explicit FooFactory(const BaseTypeSerializerFactory& base) : base_(base) {}
       shared_ptr<ISerializer<FooStruct>> create_foo_serializer() const override {
           auto compound = make_shared<CompoundSerializer<FooStruct>>();
           compound->add_member<uint32_t>(base_.create_uint32_serializer(), ...);
           return compound;
       }
   };
   ```
4. The existing `SomeipBaseTypeSerializerFactory` works unchanged.

### 7.3 Adding a New Wire-Format Backend

Implement a new class deriving from `BaseTypeSerializerFactory`:

```cpp
class FlatbufferBaseTypeSerializerFactory : public BaseTypeSerializerFactory {
    // Implement all create_*_serializer() methods using Flatbuffer encoding
};
```

All existing application factories (`CarWindowFactory`, etc.) work unchanged —
they only depend on the `BaseTypeSerializerFactory` interface.

### 7.4 Adding TLV to a Member

Wrap any member serializer with TLV at the factory composition level:

```cpp
auto raw_ser = base_.create_uint32_serializer();
auto tlv_ser = base_.create_tlv_serializer<uint32_t>(raw_ser, dataId, wireType);
compound->add_member<uint32_t>(tlv_ser, getter, setter);
```

---

## 8. Build Targets

| Target | Type | Description |
|---|---|---|
| `//src/reader:reader_serializers` | `cc_library` | Header-only library, all 4 layers |
| `//src/reader:extended_serializer_test` | `cc_binary` | 16-section test suite |
| `//src/reader:arxml_factory_demo` | `cc_binary` | Interactive demo with hexdump |

Build:
```bash
bazel build //src/reader:reader_serializers
```

Test:
```bash
bazel run //src/reader:extended_serializer_test
```

Demo:
```bash
bazel run //src/reader:arxml_factory_demo
```

---

## 9. Design Decisions & Rationale

### D1: Header-Only Implementation

**Decision:** All framework and serializer code is header-only (templates).

**Rationale:** The serializer types are heavily templated (`ISerializer<T>`,
`CompoundSerializer<T>`, `PrimitiveSerializer<T>`, etc.). A header-only approach
avoids explicit template instantiation, keeps the Bazel build graph simple (single
`cc_library` with `hdrs` only), and enables the compiler to inline hot serialization
paths.

### D2: Factory Injection over Inheritance

**Decision:** `CarWindowFactory` receives a `const BaseTypeSerializerFactory&`
via constructor injection, rather than inheriting from it.

**Rationale:** This enforces the architectural boundary: application factories
**compose** base-type serializers but **do not extend** the base-type factory.
It also makes the dependency explicit and testable (a mock
`BaseTypeSerializerFactory` can be injected for unit testing).

### D3: CompoundSerializer with Lambda Bindings

**Decision:** Struct member access is via `std::function<Member(const Outer&)>`
getter/setter lambdas rather than pointer-to-member.

**Rationale:** Lambdas are more flexible — they support computed properties,
type conversions (e.g. enum→underlying), and non-public members. The
`std::function` overhead is negligible compared to the I/O-bound serialization
workload.

### D4: Node\<T\> Indirection

**Decision:** Serializers operate on `Node<T>&` rather than `T&` directly.

**Rationale:** The `Node<T>` abstraction allows future extensions such as
ownership-managing nodes (e.g. nodes that allocate on deserialize), tracing
nodes (log every access), or shared-memory-backed nodes — without changing
the serializer interface.

### D5: Compile-Time SerializerSettings

**Decision:** Settings presets (`kCarWindowSomeipSettings`, `kCarWindowIpcSettings`)
are `static constexpr`.

**Rationale:** These values come from ARXML at build time and never change at
runtime. Making them `constexpr` enables the compiler to propagate constants into
serializer constructors and potentially optimise branch-on-swap decisions.

### D6: Bundle Structs for Lifetime Management

**Decision:** `CarWindowSomeipBundle` / `CarWindowIpcBundle` are aggregate structs
that own both the base-type factory and the application factory.

**Rationale:** The `CarWindowFactory` holds a `const&` to the base factory. The
bundle ensures both objects have the same lifetime, preventing dangling references.
This is simpler and more efficient than `shared_ptr` co-ownership for stateless
factory pairs.

---

## 10. SOME/IP Wire Format Reference

### 10.1 Primitive Types

```
Type        Wire Size    Byte Order
────────    ─────────    ──────────
uint8_t     1 byte       n/a
uint16_t    2 bytes      per settings.byteOrder
uint32_t    4 bytes      per settings.byteOrder
uint64_t    8 bytes      per settings.byteOrder
int8_t      1 byte       n/a
int16_t     2 bytes      per settings.byteOrder
int32_t     4 bytes      per settings.byteOrder
int64_t     8 bytes      per settings.byteOrder
float       4 bytes      per settings.byteOrder (IEEE 754)
double      8 bytes      per settings.byteOrder (IEEE 754)
bool        1 byte       0x00 / 0x01
```

### 10.2 SOME/IP String (PRS_SOMEIP_00084–00086)

```
┌──────────────────┬──────────┬──────────────┬──────┐
│ Length Field      │ BOM      │ Characters   │ Null │
│ (N bytes, BE)    │ EF BB BF │ UTF-8 data   │ 00   │
│ N = settings.    │ 3 bytes  │ variable     │ 1B   │
│ sizeStringLen    │          │              │      │
└──────────────────┴──────────┴──────────────┴──────┘
Length field value = sizeof(BOM) + sizeof(chars) + sizeof(null)
```

### 10.3 Vector / Array / Map

```
Vector<T>:  [length field (N bytes)] [elem₁] [elem₂] ... [elemₙ]
Array<T,N>: [length field (N bytes)] [elem₁] [elem₂] ... [elemₙ]  (N=0 → no prefix)
Map<K,V>:   [length field (N bytes)] [k₁][v₁] [k₂][v₂] ...
```

### 10.4 TLV Tag (PRS_SOMEIP_00373)

```
Bit layout of 2-byte tag:
┌───┬─────────┬──────────────────────────┐
│ R │ WireType│ DataID                   │
│ 1 │ 3 bits  │ 12 bits                  │
└───┴─────────┴──────────────────────────┘
  15   14–12      11–0

Wire types 0–4: no length field (size implied by type)
Wire types 5–7: length field of 1/2/4 bytes follows the tag
```

---

## 11. Test Coverage

The test suite (`extended_serializer_test.cpp`) provides 16 test sections:

| # | Test | Layer Exercised |
|---|---|---|
| 1 | Primitive byte order (BE/Opaque, all integer widths, float, double) | 2 |
| 2 | Bool serializer | 2 |
| 3 | SOME/IP string (BOM, length, null, round-trip) | 2 |
| 4 | Vector serializer | 2 |
| 5 | Bool vector serializer | 2 |
| 6 | Array serializer | 2 |
| 7 | Map serializer | 2 |
| 8 | AppError serializer | 2 |
| 9 | TLV decorator (wire type 2, fixed size) | 2 |
| 9b | TLV complex type (wire type 7, 4-byte length) | 2 |
| 10 | Enum serializer via factory | 2 |
| 11 | CompoundSerializer (manual composition) | 1 + 2 |
| 12 | CarWindowFactory — WindowInfo | 4 (all layers) |
| 13 | CarWindowFactory — WindowControl | 4 (all layers) |
| 14 | Cross-deployment (SOME/IP vs IPC byte order) | 4 (all layers) |
| 15 | serialize_to append semantics | 1 + 2 |
| 16 | deserialize_from offset advancement | 1 + 2 |

---

## 12. Glossary

| Term | Definition |
|---|---|
| **ARXML** | AUTOSAR XML — the machine-readable description of service interfaces, deployments, and transformation properties |
| **PRS_SOMEIP** | AUTOSAR Protocol Requirements Specification for SOME/IP |
| **TLV** | Tag-Length-Value encoding for optional/dynamic struct members |
| **BOM** | Byte Order Mark (`EF BB BF` for UTF-8) |
| **Wire type** | SOME/IP TLV wire type (0–7), determines the size of the length field |
| **Deployment** | A specific configuration of byte order, length field sizes, and service IDs |
| **Base type** | A SOME/IP primitive or container type (uint, string, vector, etc.) |
| **Application type** | A domain-specific struct composed from base types (e.g. WindowInfo) |
| **CompoundSerializer** | Composite pattern implementation that serializes struct members in order |
| **Node\<T\>** | Mutable typed data holder — abstracts value ownership from serialization |
