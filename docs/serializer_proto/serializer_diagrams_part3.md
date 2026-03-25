<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

│─────────────────────────────────────────│
│  id      : uint32_t                     │
│  payload : char[64]                     │
│─────────────────────────────────────────│
│  + set_payload(string&)                 │
│  + get_payload() → string               │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│           «struct» WireBuffer            │
│─────────────────────────────────────────│
│  data   : uint8_t[128]                  │
│  length : uint32_t                      │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│     «interface» ISerializerPlugin        │
│─────────────────────────────────────────│
│  + serialize(msg, buf&)   → bool         │
│  + deserialize(buf, msg&) → bool         │
│  + name()                 → const char*  │
└─────────────────────────────────────────┘

Four example implementations:
  1. Static linking    (examples/serializer_plugin_static/)
  2. Dynamic loading   (examples/serializer_plugin_dlopen/)
  3. IPC-based         (examples/serializer_plugin_ipc/)
  4. Service-oriented  (examples/serializer_plugin_service/)
```

---

## 6. Sequence Diagrams

### 6.1 Serialize a Primitive Type (Core Library)

```
Caller                serialize<T>()           checkIfValueMustSwap()   memcpy
  │                        │                          │                   │
  │ serialize(val,buf,sz,  │                          │                   │
  │    settings)           │                          │                   │
  │───────────────────────▶│                          │                   │
  │                        │ sizeof(T) > bufSize?     │                   │
  │                        │ YES → return false       │                   │
  │                        │ NO  ↓                    │                   │
  │                        │ sizeof(T)==1?            │                   │
  │                        │ YES → *buf = (uint8_t)val│                   │
  │                        │       return true        │                   │
  │                        │ NO  ↓                    │                   │
  │                        │ checkIfValueMustSwap     │                   │
  │                        │──────────────────────────▶│                   │
  │                        │        true/false        │                   │
  │                        │◁─────────────────────────│                   │
  │                        │                          │                   │
  │                        │ if mustSwap:             │                   │
  │                        │   swapped = swap(val)    │                   │
  │                        │   memcpy(buf, &swapped)──┼──────────────────▶│
  │                        │ else:                    │                   │
  │                        │   memcpy(buf, &val) ─────┼──────────────────▶│
  │                        │                          │                   │
  │◁── true ──────────────│                          │                   │
```

### 6.2 Deserialize a String with Length Prefix (Core Library)

```
Caller              deserialize(string)    readLengthField()     memcpy
  │                       │                      │                  │
  │ deserialize(data,     │                      │                  │
  │   buf,sz,settings,    │                      │                  │
  │   readbytes)          │                      │                  │
  │──────────────────────▶│                      │                  │
  │                       │ lfSize = settings    │                  │
  │                       │  .sizeStringLF       │                  │
  │                       │                      │                  │
  │                       │ sz < lfSize+4?       │                  │
  │                       │ YES → return false   │                  │
  │                       │ NO  ↓                │                  │
  │                       │ readLengthField(     │                  │
  │                       │   lfSize, stringLen, │                  │
  │                       │   &buf, byteOrder)   │                  │
  │                       │─────────────────────▶│                  │
  │                       │   stringLen, buf     │                  │
  │                       │   advanced past LF   │                  │
  │                       │◁────────────────────│                  │
  │                       │                      │                  │
  │                       │ stringLen < 4?       │                  │
  │                       │ YES → return false   │                  │
  │                       │ (BOM=3 + null=1)     │                  │
  │                       │                      │                  │
  │                       │ Verify UTF-8 BOM:    │                  │
  │                       │ buf[0..2] == EF BB BF│                  │
  │                       │ NO  → return false   │                  │
  │                       │ YES ↓                │                  │
  │                       │                      │                  │
  │                       │ Verify null term:    │                  │
  │                       │ buf[stringLen-4]==0   │                  │
  │                       │ NO  → return false   │                  │
  │                       │ YES ↓                │                  │
  │                       │                      │                  │
  │                       │ data.assign(buf+3,   │                  │
  │                       │   stringLen-4)        │                  │
  │                       │ readbytes =           │                  │
  │                       │   stringLen + lfSize  │                  │
  │                       │                      │                  │
  │◁── true ─────────────│                      │                  │
```

### 6.3 Serialize a Compound Struct (Decorator Prototype)

> Example: `WindowInfo { pos: uint32, state: WindowState(uint32) }`

```
Caller       CompoundSerializer   MemberBinding       MemberBinding       PrimitiveSerializer
             <WindowInfo>         (pos:uint32)        (state:enum)        <uint32_t>
  │               │                    │                    │                    │
  │ serialize_to  │                    │                    │                    │
  │  (node, out)  │                    │                    │                    │
  │──────────────▶│                    │                    │                    │
  │               │ for each binding:  │                    │                    │
  │               │                    │                    │                    │
  │               │ serialize_member   │                    │                    │
  │               │  (obj, out)        │                    │                    │
  │               │───────────────────▶│                    │                    │
  │               │                    │ val = getter_(obj) │                    │
  │               │                    │   → obj.pos        │                    │
  │               │                    │ RefNode<uint32> n  │                    │
  │               │                    │                    │                    │
  │               │                    │ serializer_        │                    │
  │               │                    │ ->serialize_to     │                    │
  │               │                    │  (node, out)       │                    │
  │               │                    │────────────────────┼───────────────────▶│
  │               │                    │                    │                    │ val = node.get()
  │               │                    │                    │                    │ if swap: byteSwap
  │               │                    │                    │                    │ out.insert(bytes)
  │               │                    │◁───────────────────┼────────────────────│
  │               │◁──────────────────│                    │                    │
  │               │                    │                    │                    │
  │               │ serialize_member   │                    │                    │
  │               │  (obj, out)        │                    │                    │
  │               │────────────────────┼───────────────────▶│                    │
  │               │                    │                    │ val = getter_(obj) │
  │               │                    │                    │   → obj.state      │
  │               │                    │                    │ (enum → uint32)    │
  │               │                    │                    │                    │
  │               │                    │                    │ serializer_        │
  │               │                    │                    │ ->serialize_to     │
  │               │                    │                    │  (node, out) ─────▶│
  │               │                    │                    │                    │ (same as above)
  │               │                    │                    │◁───────────────────│
  │               │◁───────────────────┼───────────────────│                    │
  │               │                    │                    │                    │
  │◁─── out ─────│  (contains 8 bytes: [pos:4B][state:4B])│                    │
```

### 6.4 Factory Construction — CarWindow Bundle

```
User Code            CarWindowSomeipBundle    SomeipBaseType      CarWindowFactory
                                              SerializerFactory
  │                          │                      │                   │
  │ CarWindowSomeipBundle()  │                      │                   │
  │─────────────────────────▶│                      │                   │
  │                          │                      │                   │
  │                          │ base(kCarWindow      │                   │
  │                          │   SomeipSettings)    │                   │
  │                          │─────────────────────▶│                   │
  │                          │                      │ stores settings   │
  │                          │                      │ (BigEndian, etc.) │
  │                          │                      │                   │
  │                          │ app(base)            │                   │
  │                          │──────────────────────┼──────────────────▶│
  │                          │                      │                   │ stores base_ ref
  │                          │                      │                   │
  │◁─────── bundle ─────────│                      │                   │
  │                          │                      │                   │
  │ bundle.app               │                      │                   │
  │ .create_window_info      │                      │                   │
  │ _serializer()            │                      │                   │
  │──────────────────────────┼──────────────────────┼──────────────────▶│
  │                          │                      │                   │
  │                          │                      │   compound = new  │
  │                          │                      │   CompoundSerializer│
  │                          │                      │   <WindowInfo>    │
  │                          │                      │                   │
  │                          │                      │◁─ create_uint32 ──│
  │                          │                      │   → PrimSer<u32>  │
  │                          │                      │──────────────────▶│
  │                          │                      │                   │ add_member(ser,
  │                          │                      │                   │   get_pos, set_pos)
  │                          │                      │                   │
  │                          │                      │◁─ create_enum ────│
  │                          │                      │   <WindowState>   │
  │                          │                      │   → PrimSer<WS>  │
  │                          │                      │──────────────────▶│
  │                          │                      │                   │ add_member(ser,
  │                          │                      │                   │   get_state,
  │                          │                      │                   │   set_state)
  │                          │                      │                   │
  │◁─── shared_ptr<ISerializer<WindowInfo>> ────────┼───────────────────│
```

### 6.5 TLV Tag Encode / Decode Flow

```
                           ENCODE (serialize_to)
                           =====================

Caller       TlvSerializerDecorator<T>     writeTag()     writeLengthField()   inner_
  │                  │                         │                  │                │
  │ serialize_to     │                         │                  │                │
  │  (node, out)     │                         │                  │                │
  │─────────────────▶│                         │                  │                │
  │                  │ inner_->serialize_to     │                  │                │
  │                  │  (node, payload)         │                  │                │
  │                  │─────────────────────────┼──────────────────┼───────────────▶│
  │                  │◁────────────────────────┼──────────────────┼────────────────│
  │                  │                         │                  │                │
  │                  │ writeTag(tag[2],         │                  │                │
  │                  │   wireType, dataId)      │                  │                │
  │                  │────────────────────────▶│                  │                │
  │                  │    tag[0]: [R|WT|ID_hi] │                  │                │
  │                  │    tag[1]: [ID_lo]       │                  │                │
  │                  │◁───────────────────────│                  │                │
  │                  │                         │                  │                │
  │                  │ out.push_back(tag[0,1]) │                  │                │
  │                  │                         │                  │                │
  │                  │ if wireType ≥ 5:        │                  │                │
  │                  │   lfSize = computeSize  │                  │                │
  │                  │     OfLFBasedOnWT(wt)   │                  │                │
  │                  │   writeLengthField(     │                  │                │
  │                  │     lfSize, payLen,     │                  │                │
  │                  │     &lfBuf, byteOrder)  │                  │                │
  │                  │────────────────────────┼─────────────────▶│                │
  │                  │   out.insert(lfBuf)    │                  │                │
  │                  │                         │                  │                │
  │                  │ out.insert(payload)     │                  │                │
  │                  │                         │                  │                │
  │◁────── out ─────│                         │                  │                │
  │                  │                         │                  │                │
  │  Wire layout for dynamic complex type:                                        │
  │  [Tag: 2B] [LF: 1/2/4B] [Payload: N bytes]                                  │
  │  Wire layout for basic / static complex:                                      │
  │  [Tag: 2B] [Payload: N bytes]                                                │


                           DECODE (deserialize_from)
                           =========================

Caller       TlvSerializerDecorator<T>     readTag()      readLengthField()    inner_
  │                  │                         │                  │                │
  │ deserialize_from │                         │                  │                │
  │  (data, offset,  │                         │                  │                │
  │   node)          │                         │                  │                │
  │─────────────────▶│                         │                  │                │
  │                  │ readTag(data+offset,     │                  │                │
  │                  │   rWt, rId, sz, tagBytes)│                  │                │
  │                  │────────────────────────▶│                  │                │
  │                  │   rWt, rId, tagBytes=2  │                  │                │
  │                  │◁───────────────────────│                  │                │
  │                  │                         │                  │                │
  │                  │ offset += tagBytes      │                  │                │
  │                  │                         │                  │                │
  │                  │ if rWt ≥ 5:             │                  │                │
  │                  │   lfSize = compute...   │                  │                │
  │                  │   readLengthField(      │                  │                │
  │                  │     lfSize, payLen,     │                  │                │
  │                  │     &buf, byteOrder)    │                  │                │
  │                  │────────────────────────┼─────────────────▶│                │
  │                  │   offset += lfSize      │                  │                │
  │                  │                         │                  │                │
  │                  │ inner_->deserialize_from│                  │                │
  │                  │  (data, offset, node)   │                  │                │
  │                  │─────────────────────────┼──────────────────┼───────────────▶│
  │                  │   offset advanced past  │                  │                │
  │                  │   payload bytes         │                  │                │
  │                  │◁────────────────────────┼──────────────────┼────────────────│
  │◁────────────────│                         │                  │                │
```

### 6.6 AppError Serialize / Deserialize

```
                           SERIALIZE
                           =========

Caller           appErrorSerialize()      writeLengthField()     serialize<T>()
  │                     │                        │                      │
  │ appErrorSerialize   │                        │                      │
  │  (err, buf, sz)     │                        │                      │
  │────────────────────▶│                        │                      │
  │                     │ sz < 19?               │                      │
  │                     │ YES → return false     │                      │
  │                     │                        │                      │
  │                     │ writeLengthField(4,    │                      │
  │                     │   14, &buf, BigEndian) │                      │
  │                     │───────────────────────▶│  [union LF: 4B]     │
  │                     │                        │                      │
  │                     │ *buf = 1 (type sel)    │                      │
  │                     │ buf += 1               │  [type selector: 1B] │
  │                     │                        │                      │
  │                     │ writeLengthField(2,    │                      │
  │                     │   12, &buf, BigEndian) │                      │
  │                     │───────────────────────▶│  [struct LF: 2B]    │
  │                     │                        │                      │
  │                     │ serialize(domain, buf, 8, settings)           │
  │                     │──────────────────────────────────────────────▶│
  │                     │                        │  [domain: 8B BE]     │
  │                     │ buf += 8               │                      │
  │                     │                        │                      │
  │                     │ serialize(code, buf, 4, settings)             │
  │                     │──────────────────────────────────────────────▶│
  │                     │                        │  [code: 4B BE]       │
  │                     │                        │                      │
  │◁── true ───────────│                        │                      │
  │                     │                        │                      │
  │  Wire layout (19 bytes total):                                      │
  │  [unionLF:4][typeSel:1][structLF:2][domain:8][code:4]              │


                           DESERIALIZE
                           ===========

Caller           appErrorDeserialize()   readLengthField()     deserialize<T>()
  │                     │                       │                      │
  │ appErrorDeserialize │                       │                      │
  │  (err, buf, sz,     │                       │                      │
  │   readbytes)        │                       │                      │
  │────────────────────▶│                       │                      │
  │                     │ sz < 19?              │                      │
  │                     │ YES → return false    │                      │
  │                     │                       │                      │
  │                     │ readLengthField(4,    │                      │
  │                     │   unionLen, &buf, BE) │                      │
  │                     │──────────────────────▶│                      │
  │                     │ unionLen != 14?       │                      │
  │                     │ YES → return false    │                      │
  │                     │                       │                      │
  │                     │ typeSel = *buf        │                      │
  │                     │ typeSel != 1?         │                      │
  │                     │ YES → return false    │                      │
  │                     │ buf += 1              │                      │
  │                     │                       │                      │
  │                     │ readLengthField(2,    │                      │
  │                     │   structLen, &buf, BE)│                      │
  │                     │──────────────────────▶│                      │
  │                     │ structLen != 12?      │                      │
  │                     │ YES → return false    │                      │
  │                     │                       │                      │
  │                     │ deserialize(err.domain, buf, 8, settings, rb)│
  │                     │──────────────────────────────────────────────▶│
  │                     │ buf += 8              │                      │
  │                     │                       │                      │
  │                     │ deserialize(err.code, buf, 4, settings, rb)  │
  │                     │──────────────────────────────────────────────▶│
  │                     │                       │                      │
  │                     │ readbytes = 19        │                      │
  │◁── true ───────────│                       │                      │
```

---

## Appendix A — SFINAE Overload Resolution Map

The core library selects the correct `serialize()` / `deserialize()` / `computeSerializedSize()` overload via SFINAE:

```
Type                          Guard                           Header
─────────────────────────── ────────────────────────── ───────────────────────────
bool, int8..64, uint8..64,  EnableIfBasic<T>            SerializeBasicTypes.hpp
  float, double, enums                                  DeserializeBasicTypes.hpp
                                                        SerializerComputeSize.hpp

std::string                  (exact type match)          same headers

std::array<T,N> where        EnableIfBasic<T>            same headers
  T is basic

std::vector<T> where         EnableIfBasicAndNotBool<T>  same headers
  T is basic and not bool

std::vector<bool>            (exact type match)          same headers

std::map<K,V> where          EnableIfBasic<K> &&         same headers
  K,V are basic              EnableIfBasic<V>

AppError                     (exact type match)          AppErrorSerializers.hpp

Non-basic (struct, class)    EnableIfNotBasic<T>         — user must implement
                                                          ISerializer<T> /
                                                          IDeserializer<T>
```

---

## Appendix B — Wire Format Quick Reference

### TLV Tag (2 bytes)

```
Bit:  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
     ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
     │Rsv│  Wire Type  │               Data ID (12 bits)              │
     └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
```

### SOME/IP String (PRS_SOMEIP_00084..00086)

```
┌─────────────────┬──────────┬───────────────────┬─────────────┐
│ Length Field     │ UTF-8    │ String bytes       │ Null        │
│ (1/2/4 bytes)   │ BOM      │ (N bytes)          │ terminator  │
│                 │ EF BB BF │                    │ 00          │
│ value = N+3+1   │ (3 bytes)│                    │ (1 byte)    │
└─────────────────┴──────────┴───────────────────┴─────────────┘
```

### AppError (19 bytes, always big-endian)

```
┌──────────────┬───────────────┬────────────────┬────────────┬──────────┐
│ Union LF     │ Type Selector │ Struct LF      │ domain     │ code     │
│ (4 bytes)    │ (1 byte) = 1  │ (2 bytes)      │ (8 bytes)  │ (4 bytes)│
│ value = 14   │               │ value = 12     │ uint64 BE  │ int32 BE │
└──────────────┴───────────────┴────────────────┴────────────┴──────────┘
```
