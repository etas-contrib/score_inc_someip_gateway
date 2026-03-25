<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

┌────────────────────────────────────────────────────────────────────┐
│                           «enum class»                             │
│                            EWireType                               │
│────────────────────────────────────────────────────────────────────│
│  E_WIRETYPE_0    = 0  (8-bit base)                                 │
│  E_WIRETYPE_1    = 1  (16-bit base)                                │
│  E_WIRETYPE_2    = 2  (32-bit base)                                │
│  E_WIRETYPE_3    = 3  (64-bit base)                                │
│  E_WIRETYPE_4    = 4  (complex static)                             │
│  E_WIRETYPE_5    = 5  (complex dynamic, 1-byte length)             │
│  E_WIRETYPE_6    = 6  (complex dynamic, 2-byte length)             │
│  E_WIRETYPE_7    = 7  (complex dynamic, 4-byte length)             │
│  E_WIRETYPE_NONE = 8  (invalid/reserved)                           │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│                    «interface, template»                            │
│                ISerializer< SampleType_T >                         │
│                      (ISerializer.hpp)                              │
│────────────────────────────────────────────────────────────────────│
│  # ISerializer() = default                                         │
│  + ~ISerializer() virtual                                          │
│  + ISerializer(const&) = delete                                    │
│  + ISerializer(&&) = delete                                        │
│  + operator=(const&) = delete                                      │
│  + operator=(&&) = delete                                          │
│────────────────────────────────────────────────────────────────────│
│  + computeSerializedSize(objectp: const T*)                        │
│        → uint32_t                                    «pure virtual» │
│  + computeSerializedSizeTlv(objectp: const T*, lfs: uint8_t&)     │
│        → uint32_t                 «virtual, default = UINT32_MAX»  │
│  + computeSerializedSizeTlv(objectp: const T*)                     │
│        → uint32_t            «virtual, delegates to 2-arg variant» │
│  + serialize(buf: uint8_t*, maxsize: uint32_t, objectp: const T*)  │
│        → bool                                        «pure virtual» │
│  + serializeTlv(buf: uint8_t*, maxsize: uint32_t, objectp: const T*)│
│        → bool                       «virtual, default = false»     │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│                    «interface, template»                            │
│              IDeserializer< SampleType_T >                         │
│                    (IDeserializer.hpp)                              │
│────────────────────────────────────────────────────────────────────│
│  # IDeserializer() = default                                       │
│  + ~IDeserializer() virtual                                        │
│  + IDeserializer(const&) = delete                                  │
│  + IDeserializer(&&) = delete                                      │
│  + operator=(const&) = delete                                      │
│  + operator=(&&) = delete                                          │
│────────────────────────────────────────────────────────────────────│
│  + deserialize(buf: const uint8_t*, len: uint32_t,                 │
│                objectp: T*, readbytes: uint32_t&)                  │
│        → bool                                        «pure virtual» │
│  + deserializeTlv(buf: const uint8_t*, len: uint32_t,              │
│                   objectp: T*, readbytes: uint32_t&, lfs: uint8_t) │
│        → bool                       «virtual, default = false»     │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│                          «struct»                                  │
│                          AppError                                  │
│                    (AppErrorSerializers.hpp)                        │
│────────────────────────────────────────────────────────────────────│
│  domain : uint64_t                                                 │
│  code   : int32_t                                                  │
└────────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════
                    Free Functions — by header
═══════════════════════════════════════════════════════════════════════

┌────────────────────────────────────────────────────────────────────┐
│              SerializerUtils.hpp — SFINAE type traits               │
│────────────────────────────────────────────────────────────────────│
│  isBasicType<T>()              → constexpr bool                    │
│  EnableIfBasic<T>              = enable_if_t<isBasicType<T>()>     │
│  EnableIfBasicAndNotBool<T>    = enable_if_t<isBasicType<T>()      │
│                                   && !is_same<T,bool>>             │
│  EnableIfNotBasic<T>           = enable_if_t<!isBasicType<T>()>    │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│          SerializerUtils.hpp — byte-order & wire utilities          │
│────────────────────────────────────────────────────────────────────│
│  checkIfValueMustSwap(ByteOrder) → bool                            │
│  setDataReverseOrder(src[], dst[], len)                             │
│  swap<T>(val) → T                  «EnableIfBasic<T>»              │
│  writeLengthField(size, len, buf**, ByteOrder) → bool              │
│  readLengthField(size, len&, buf**, ByteOrder)                     │
│  writeTag(buf*, wireType, dataId) → bool                           │
│  readTag(buf*, wt&, id&, bufSize, readBytes&) → bool              │
│  computeSizeOfLengthField(length) → constexpr uint8_t              │
│  computeWireType<T>(settings, lfSize) → uint8_t                   │
│      «EnableIfNotBasic: returns 4-7 based on lengthFieldSize»      │
│      «EnableIfBasic:    returns 0-3 based on sizeof(T)»            │
│  computeSizeOfLengthFieldBasedOnWireType(wt) → uint8_t             │
│  skipUnknownMember(buf*, bufSize, wt, lfSize, settings,            │
│                    skipBytes&) → bool                               │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│   SerializerComputeSize.hpp — size computation overloads           │
│────────────────────────────────────────────────────────────────────│
│  «2-arg overloads (legacy)»                                        │
│  computeSerializedSize(T, Settings)              → uint32_t        │
│  computeSerializedSize(string&, Settings)        → uint32_t        │
│  computeSerializedSize(array<T,N>&, Settings)    → uint32_t        │
│  computeSerializedSize(vector<T>&, Settings)     → uint32_t        │
│  computeSerializedSize(map<K,V>&, Settings)      → uint32_t        │
│                                                                    │
│  «4-arg overloads (TLV-aware)»                                     │
│  computeSerializedSize(T, Settings&, lfSize&, hasTlv) → uint32_t  │
│  computeSerializedSize(string&, Settings&, lfSize&, hasTlv)       │
│  computeSerializedSize(array<T,N>&, Settings&, lfSize&, hasTlv)   │
│  computeSerializedSize(vector<T>&, Settings&, lfSize&, hasTlv)    │
│  computeSerializedSize(map<K,V>&, Settings&, lfSize&, hasTlv)     │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│   SerializeBasicTypes.hpp — serialization overloads                │
│────────────────────────────────────────────────────────────────────│
│  serialize(T val, buf*, bufSize, Settings, hasTlv) → bool          │
│  serialize(string&, buf[], bufSize, Settings, hasTlv) → bool       │
│  serialize(array<T,N>&, buf[], bufSize, Settings, hasTlv) → bool   │
│  serialize(vector<T>&, buf[], bufSize, Settings, hasTlv) → bool    │
│      «EnableIfBasicAndNotBool<T>»                                  │
│  serialize(vector<bool>&, buf[], bufSize, Settings, hasTlv) → bool │
│  serialize(map<K,V>&, buf[], bufSize, Settings, hasTlv) → uint32_t │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│   DeserializeBasicTypes.hpp — deserialization overloads             │
│────────────────────────────────────────────────────────────────────│
│  deserialize(T&, buf[], bufSize, Settings, readbytes&, lfSize)     │
│      → bool                                                        │
│  deserialize(string&, buf[], bufSize, Settings, readbytes&, lfSize)│
│      → bool                                                        │
│  deserialize(array<T,N>&, buf[], bufSize, Settings, readbytes&,    │
│              lfSize) → bool                                        │
│  deserialize(vector<T>&, buf[], bufSize, Settings, readbytes&,     │
│              lfSize) → bool   «EnableIfBasicAndNotBool<T>»         │
│  deserialize(vector<bool>&, buf[], bufSize, Settings, readbytes&,  │
│              lfSize) → bool                                        │
│  deserialize(map<K,V>&, buf[], bufSize, Settings, readbytes&,      │
│              lfSize) → bool                                        │
└────────────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────────────┐
│   AppErrorSerializers.hpp — application error wire format          │
│────────────────────────────────────────────────────────────────────│
│  appErrorSettings : constexpr SerializerSettings                   │
│  appErrorSerialize(AppError, buf[], bufSize)     → bool            │
│  appErrorDeserialize(AppError&, buf[], bufSize, readbytes&) → bool │
│                                                                    │
│  Wire format:                                                      │
│  [union LF: 4B] [type selector: 1B] [struct LF: 2B]               │
│  [domain: 8B BE] [code: 4B BE]  — total = 19 bytes                │
└────────────────────────────────────────────────────────────────────┘

### Constants (`SerializerTypes.hpp`)

| Name                      | Value    | Description                          |
|---------------------------|----------|--------------------------------------|
| `DEFAULT_TLV_DATA_ID`    | `0xFFFF` | Sentinel for missing TLV data ID    |
| `INVALID_LENGTH_FIELD_SIZE` | `0xFF` | Sentinel for invalid length field   |

---

## 3. Decorator Prototype Class Diagram (`src/serializer_decoratorbased_test/`)

> Namespace: `reader` — wraps `com::serializer` free functions in OOP classes.

```
                      ┌───────────────────┐
                      │  ISerializerBase  │
                      │  (pure virtual)   │
                      │  ~ISerializerBase │
                      └────────┬──────────┘
                               │ ▲ inherits
                               │
            ┌──────────────────┴──────────────────────────────┐
            │                                                  │
            │            «interface, template»                  │
            │           ISerializer< T >                       │
            │                (reader::)                        │
            ├──────────────────────────────────────────────────┤
            │  + serialize(Node<T>&)         → ByteVector      │
            │  + serialize_to(Node<T>&, out) → void            │
            │  + deserialize(ByteVector&, Node<T>&) → void     │
            │  + deserialize_from(data, offset&, Node<T>&)     │
            └─────────┬────────────────────┬───────────────────┘
                      │                    │
           ┌──────────┘                    └──────────────────┐
           │ ▲ inherits                         ▲ inherits    │
           │                                                   │
┌──────────┴───────────┐            ┌─────────────────────────┴──────┐
│ SerializerDecorator<T>│           │  CompoundSerializer< T >       │
│    (Decorator base)   │           │  (Composite for structs)       │
│───────────────────────│           │────────────────────────────────│
│ # inner_: shared_ptr  │           │ - bindings_: vec<unique_ptr   │
│   <ISerializer<T>>    │           │    <IMemberBinding<T>>>        │
│───────────────────────│           │────────────────────────────────│
│ Forwards all calls    │           │ + add_member<Member>(          │
│ to inner_.            │           │     serializer, getter, setter)│
│ Override to decorate. │           │ + serialize / deserialize:     │
└──────────┬────────────┘           │   iterates bindings_ in order  │
           │ ▲ inherits             └────────────────────────────────┘
           │                                        │
           │                                        │ contains ▼
┌──────────┴────────────────┐      ┌────────────────┴────────────────┐
│ TlvSerializerDecorator<T> │      │       IMemberBinding< Outer >   │
│───────────────────────────│      │  (type-erased member interface)  │
│ - dataId_ : uint16_t     │      │────────────────────────────────│
│ - wireType_ : EWireType  │      │  + serialize_member(Outer&,out) │
│ - settings_ : Settings   │      │  + deserialize_member(          │
│───────────────────────────│      │       data, offset&, Outer&)    │
│ serialize_to: writes tag  │      └────────────────┬────────────────┘
│   + optional length field │                       │ ▲ inherits
│   + delegates payload to  │                       │
│   inner_.                 │      ┌────────────────┴────────────────┐
│ deserialize_from: reads   │      │  MemberBinding<Outer, Member>   │
│   tag, optional LF,       │      │────────────────────────────────│
│   delegates to inner_.    │      │ - serializer_ : shared_ptr     │
└───────────────────────────┘      │    <ISerializer<Member>>        │
                                   │ - getter_ : function            │
                                   │ - setter_ : function            │
                                   │────────────────────────────────│
                                   │ (bridges Outer ↔ Member via    │
                                   │  getter/setter lambdas)        │
                                   └─────────────────────────────────┘

┌─────────────────────────────────┐    ┌──────────────────────────────┐
│         Node< T >               │    │     RefNode< T > (final)     │
│      (abstract data holder)     │    │────────────────────────────│
│─────────────────────────────────│    │ - ref_ : T&                  │
│ + get() const → const T&        │◁───│ + get() const → const T&     │
│ + get()       → T&              │    │ + get()       → T&           │
└─────────────────────────────────┘    └──────────────────────────────┘
```

### Concrete Serializer Classes (all in `base_type_serializers.h`)

```
ISerializer<T>
  ├── PrimitiveSerializer<T>          — arithmetic / enum (byte-swap aware)
  ├── BoolSerializer                  — bool → 1 byte
  ├── SomeipStringSerializer          — BOM + length prefix + null
  ├── VectorSerializer<T>             — length-prefixed vector<T>
  ├── BoolVectorSerializer            — vector<bool> specialization
  ├── ArraySerializer<T,N>            — fixed-size array with optional LF
  ├── MapSerializer<K,V>              — length-prefixed map<K,V>
  └── AppErrorSerializer              — domain:uint64 + code:int32, big-endian
```

### Factory Hierarchy

```
┌──────────────────────────────────────────────────┐
│          BaseTypeSerializerFactory               │
│             (abstract)                           │
│──────────────────────────────────────────────────│
│ «pure virtual»                                   │
│  + create_uint8_serializer()  → shared_ptr       │
│  + create_uint16_serializer() → shared_ptr       │
│  + create_uint32_serializer() → shared_ptr       │
│  + create_uint64_serializer() → shared_ptr       │
│  + create_int8_serializer()   → shared_ptr       │
│  + create_int16_serializer()  → shared_ptr       │
│  + create_int32_serializer()  → shared_ptr       │
│  + create_int64_serializer()  → shared_ptr       │
│  + create_float_serializer()  → shared_ptr       │
│  + create_double_serializer() → shared_ptr       │
│  + create_bool_serializer()   → shared_ptr       │
│  + create_string_serializer() → shared_ptr       │
│  + create_bool_vector_serializer() → shared_ptr  │
│  + create_app_error_serializer()   → shared_ptr  │
│ «template methods (not virtual)»                 │
│  + create_enum_serializer<Enum>()                │
│  + create_vector_serializer<T>()                 │
│  + create_array_serializer<T,N>()                │
│  + create_map_serializer<K,V>()                  │
│  + create_tlv_serializer<T>(inner, dataId, wt)   │
│ «protected type-erased trampolines (virtual)»    │
│  # create_enum_serializer_erased(typeSize)       │
│  # create_vector_serializer_erased(elemSize)     │
│  # create_array_serializer_erased(elemSize, N)   │
│  # create_map_serializer_erased(keySize, valSize)│
│  # create_tlv_serializer_erased(inner, id, wt)   │
└─────────────────────┬────────────────────────────┘
                      │ ▲ inherits
┌─────────────────────┴────────────────────────────┐
│      SomeipBaseTypeSerializerFactory              │
│──────────────────────────────────────────────────│
│ - s_ : SerializerSettings                        │
│──────────────────────────────────────────────────│
│ + settings() → const SerializerSettings&         │
│ Implements all pure virtuals by instantiating     │
│ PrimitiveSerializer<T>, BoolSerializer, etc.     │
│ Protected erased trampolines return nullptr       │
│ (typed trampolines use dynamic_cast instead).     │
└──────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────┐
│        AbstractCarWindowFactory                  │
│            (abstract)                            │
│──────────────────────────────────────────────────│
│ + create_window_info_serializer()    «pure virt» │
│     → shared_ptr<ISerializer<WindowInfo>>        │
│ + create_window_control_serializer() «pure virt» │
│     → shared_ptr<ISerializer<WindowControl>>     │
└─────────────────────┬────────────────────────────┘
                      │ ▲ inherits
┌─────────────────────┴────────────────────────────┐
│            CarWindowFactory                       │
│──────────────────────────────────────────────────│
│ - base_ : const BaseTypeSerializerFactory&       │
│──────────────────────────────────────────────────│
│ + CarWindowFactory(base)                         │
│ + create_window_info_serializer()                │
│     → CompoundSerializer with 2 members:         │
│       1. pos:uint32 via base_.create_uint32      │
│       2. state:WindowState via base_.create_enum │
│ + create_window_control_serializer()             │
│     → CompoundSerializer with 1 member:          │
│       1. command:WindowCommand via create_enum   │
│──────────────────────────────────────────────────│
│ «static» window_info_service_id()     → 6432     │
│ «static» window_info_event_id()       → 1        │
│ «static» window_control_service_id()  → 6433     │
│ «static» window_control_event_id()    → 2        │
└──────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────┐
│  CarWindowSomeipBundle / CarWindowIpcBundle       │
│──────────────────────────────────────────────────│
│  base : SomeipBaseTypeSerializerFactory           │
│  app  : CarWindowFactory                          │
│──────────────────────────────────────────────────│
│  Constructor wires base → app automatically.      │
│  SomeipBundle uses kCarWindowSomeipSettings       │
│  (BigEndian, string LF=4, etc.)                   │
│  IpcBundle uses kCarWindowIpcSettings             │
│  (Opaque byte order).                             │
└──────────────────────────────────────────────────┘
```

---

## 4. Relationship Between Core Library and Decorator Prototype

```
┌─────────────────────────────────┐         ┌────────────────────────┐
│ src/serializer/                 │         │ serializer_decoratorbased│
│ (Core Library)                  │         │ _test/ (Prototype)      │
│                                 │         │                         │
│ SerializerSettings ─────────────┼────────▶│ Used by all factories  │
│ ByteOrder, EWireType            │         │ and serializers         │
│                                 │         │                         │
│ serialize(), deserialize(),     │         │ Called from within:     │
│ computeSerializedSize()  ───────┼────────▶│  SomeipStringSerializer │
│ (free function overloads)       │         │  VectorSerializer<T>    │
│                                 │         │  BoolVectorSerializer   │
│ writeLengthField(),             │         │  ArraySerializer<T,N>   │
│ readLengthField(),       ───────┼────────▶│  MapSerializer<K,V>     │
│ writeTag(), readTag()           │         │  TlvSerializerDecorator │
│                                 │         │  AppErrorSerializer     │
│ appErrorSerialize(),     ───────┼────────▶│  (delegates to core)    │
│ appErrorDeserialize()           │         │                         │
│                                 │         │ PrimitiveSerializer<T>  │
│ swap()                   ───────┼────────▶│  uses detail::byteSwap │
│ checkIfValueMustSwap()   ───────┼────────▶│  uses detail::needsSwap│
│ (Note: prototype re-implements  │         │  (own implementation)   │
│  swap logic in detail:: ns)     │         │                         │
│                                 │         │                         │
│ ISerializer<SampleType_T>       │         │ reader::ISerializer<T>  │
│ IDeserializer<SampleType_T>     │         │ (separate hierarchy,    │
│ (abstract interfaces for        │         │  different method sigs: │
│  generated code to implement)   │         │  uses Node<T>, ByteVec │
│                                 │         │  vs raw uint8_t*)       │
└─────────────────────────────────┘         └────────────────────────┘

Key insight:  The core library provides FREE FUNCTIONS for basic type
serialization. The decorator prototype wraps them in an OOP class
hierarchy with the Abstract Factory, Decorator, and Composite patterns.
The two ISerializer<T> interfaces are SEPARATE (different namespaces,
different signatures). Generated application code would implement
com::serializer::ISerializer<T>; the decorator prototype offers
reader::ISerializer<T> as a higher-level alternative.
```

---

## 5. Plugin Interface Class Diagram (`examples/serializer_plugin_common/`)

> Namespace: `plugin`

```
┌─────────────────────────────────────────┐
│            «struct» Message              │
