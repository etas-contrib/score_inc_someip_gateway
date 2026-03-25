<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Plugin Architecture — ASIL Usability Study

> **Project:** `inc_someip_gateway` · **Date:** 2026-03-24
> **Author:** Architecture study (generated)
> **Scope:** Evaluate four mechanisms for integrating a C++ serializer as a
> replaceable module across QM, ASIL-A, ASIL-B, ASIL-C, ASIL-D.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Assumptions](#2-assumptions)
3. [Mechanism-by-Mechanism Analysis](#3-mechanism-by-mechanism-analysis)
   - 3.1 [Option 1 — Static Compile-Time Registration](#31-option-1--static-compile-time-registration)
   - 3.2 [Option 2 — In-Process Shared-Library Plugin (dlopen)](#32-option-2--in-process-shared-library-plugin-dlopen)
   - 3.3 [Option 3 — Out-of-Process Plugin via IPC](#33-option-3--out-of-process-plugin-via-ipc)
   - 3.4 [Option 4 — Service-Oriented / AUTOSAR Adaptive Style](#34-option-4--service-oriented--autosar-adaptive-style)
4. [Detailed ASIL-Level Evaluation](#4-detailed-asil-level-evaluation)
5. [Decision Matrix](#5-decision-matrix)
6. [Recommended Architecture by ASIL Level](#6-recommended-architecture-by-asil-level)
7. [Buildable Examples — Overview](#7-buildable-examples--overview)
8. [Verification Strategy](#8-verification-strategy)
9. [Gaps, Limitations, and Next Steps](#9-gaps-limitations-and-next-steps)

---

## 1. Executive Summary

This study evaluates four architectural patterns for making the SOME/IP
serializer (`src/serializer/`) a replaceable or selectable component:

| # | Mechanism | Core idea |
|---|---|---|
| 1 | **Static registration** | Compile-time factory; all code linked into one binary |
| 2 | **dlopen plugin** | Shared library loaded at runtime; C-ABI boundary |
| 3 | **IPC plugin** | Serializer runs in a separate process; communication via pipes/sockets |
| 4 | **Service-oriented** | Serializer exposed as a service with registration/discovery |

**Key finding:** For ASIL-B and above, static registration is the only mechanism
that can be straightforwardly justified in a safety argument.  Dynamic loading
(dlopen) is practical for QM and ASIL-A with safeguards.  IPC-based isolation
is the strongest option when mixed-criticality partitioning is required.
Service-oriented deployment is appropriate when the serializer is part of a
middleware ecosystem but adds certification complexity.

---

## 2. Assumptions

| # | Assumption |
|---|---|
| A1 | Target OS is Linux (development/CI) or QNX (production). |
| A2 | Bazel is the build system; examples use `rules_cc`. |
| A3 | The serializer interface is the existing `ISerializer<T>` / `IDeserializer<T>` from `src/serializer/`. |
| A4 | "Plugin" means the concrete serializer implementation can be selected without recompiling the consumer. |
| A5 | ISO 26262:2018 terminology and concepts apply. |
| A6 | All mechanisms are evaluated for a single-ECU deployment; distributed multi-ECU is out of scope. |
| A7 | C++17 is the minimum language standard. |
| A8 | For safety-rated code, the compiler and standard library are qualified per ISO 26262 Part 8. |

---

## 3. Mechanism-by-Mechanism Analysis

### 3.1 Option 1 — Static Compile-Time Registration

**Concept:**
All serializer implementations are compiled and linked into the same binary.
A factory or registry selects the active implementation at startup based on
configuration (e.g., a JSON config key or a compile-time `constexpr` flag).

```
┌─────────────────────────────────────────────┐
│              Single binary                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │BigEndian  │  │LittleEnd │  │  Opaque  │  │
│  │Serializer│  │Serializer│  │Serializer│  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  │
│       └──────────────┼──────────────┘       │
│              ┌───────▼───────┐              │
│              │  ISerializer  │              │
│              └───────────────┘              │
└─────────────────────────────────────────────┘
```

**When appropriate:**
- ASIL-B, ASIL-C, ASIL-D — highest confidence in the safety case
- When all variants are known at build time
- When binary size is not a critical constraint

**When it is a bad fit:**
- Need to swap serializer without rebuilding the entire application
- Extremely large variant space (dozens of plugins)
- OTA partial-update requirements

**Safety analysis:**

| Criterion | Assessment |
|---|---|
| Freedom from interference | **Excellent.** Single address space with full compiler visibility. No ABI boundary. |
| Fault containment | Medium. A bug in any serializer can corrupt the entire process, but this is mitigated by full static analysis and test coverage. |
| Determinism | **Excellent.** No runtime loading; memory layout fixed at link time. |
| Startup behavior | **Deterministic.** Factory runs once during init. |
| Runtime updateability | **None.** Must rebuild and redeploy. |
| ABI stability | **Not needed.** Everything is compiled together. |
| Diagnosability | High. Full symbol visibility; core dumps are complete. |
| Testability | **Excellent.** Standard unit testing applies. |
| Certification complexity | **Lowest.** Well-understood software architecture pattern. |
| Security | **Strongest.** No external code loading surface. |
| Memory overhead | All variants linked in, but unused code is stripped by linker (`--gc-sections`). |
| Recovery after failure | Process restart. |
| Traceability | Full. Every byte of code is in the build manifest. |

**Platform suitability:** Linux ✓ · QNX ✓ · AUTOSAR Adaptive ✓

**Required safeguards for ASIL-D:**
- MISRA C++:2023 compliance on all serializer code
- MC/DC coverage on selection logic
- Memory protection (stack canaries, ASLR where available)
- Defensive coding: input validation on every buffer boundary

---

### 3.2 Option 2 — In-Process Shared-Library Plugin (dlopen)

**Concept:**
The serializer implementation lives in a `.so` shared library.  At startup the
host process loads it via `dlopen()` and resolves a C-ABI factory function via
`dlsym()`.  The factory returns a pointer to the serializer interface.

```
┌──────────────────────────┐
│       Host process       │
│  ┌────────────────────┐  │
│  │  ISerializer<T> *  │◄─┼── dlsym("create_serializer")
│  └────────────────────┘  │
│           ▲               │
│   dlopen("libmyser.so")  │
└───────────┼──────────────┘
            │
┌───────────▼──────────────┐
│     libmyser.so          │
│  BigEndianSerializer     │
└──────────────────────────┘
```

**When appropriate:**
- QM and ASIL-A systems where runtime variant selection is valued
- Product-line engineering with many optional serializer flavours
- Diagnostic or test environments

**When it is a bad fit:**
- ASIL-C / ASIL-D: dynamic loading is very hard to justify
- Real-time paths where `dlopen` jitter is unacceptable
- Embedded targets with no MMU or limited OS support

**Safety analysis:**

| Criterion | Assessment |
|---|---|
| Freedom from interference | **Poor to medium.** Loaded code shares the host address space. A corrupted `.so` can overwrite any host memory. |
| Fault containment | **Weak.** No hardware isolation. Relies entirely on software discipline. |
| Determinism | **Medium.** `dlopen` involves filesystem I/O, dynamic relocation, and constructor execution — all non-deterministic. |
| Startup behavior | Non-deterministic: depends on filesystem state, `.so` size, relocation count. |
| Runtime updateability | **Excellent.** Swap the `.so` and restart (or `dlclose` + `dlopen` with care). |
| ABI stability | **Critical.** C-ABI boundary required. Mismatched struct layouts cause silent corruption. |
| Diagnosability | Medium. Separate debug symbols; `dladdr` can resolve function origin. |
| Testability | Good, but plugin must be tested in isolation *and* integrated. |
| Certification complexity | **High.** Must certify the host, the plugin, *and* the loading mechanism. Must argue dlopen/dlsym cannot introduce undefined behavior. |
| Security | **Risky.** An attacker who can replace the `.so` on disk gains code execution. Requires signature verification or read-only filesystem. |
| Memory overhead | Low. Only the loaded variant occupies memory. |
| Recovery after failure | Process restart (plugin crash = host crash). |
| Traceability | Medium. Must track `.so` version independently of host binary. |

**Platform suitability:** Linux ✓ · QNX ✓ · AUTOSAR Adaptive: discouraged

**Required safeguards for ASIL-A:**
- `.so` hash/signature verification before loading
- C-ABI boundary with explicit version field
- Defensive wrappers: catch SIGSEGV from plugin calls (unreliable)
- Prefer `dlopen` at startup only, not on the hot path
- Read-only filesystem mount for plugin directory

---

### 3.3 Option 3 — Out-of-Process Plugin via IPC

**Concept:**
The serializer runs in its own process.  The host sends raw data over a local
IPC channel (pipe, UNIX domain socket, QNX message passing), and receives the
serialized bytes back.  This provides hardware-enforced memory isolation.

```
┌────────────────────┐         ┌────────────────────┐
│  Host process      │  pipe / │  Serializer process │
│  (ASIL-rated)      │  UDS    │  (QM or lower ASIL) │
│                    │◄───────►│                      │
│  send(raw struct)  │         │  recv → serialize    │
│  recv(wire bytes)  │         │  send(wire bytes)    │
└────────────────────┘         └──────────────────────┘
```

**When appropriate:**
- Mixed-criticality: ASIL-D host + QM serializer
- When the serializer is untrusted or third-party
- QNX systems with native message-passing and process partitioning
- When serializer bugs must not crash the safety-critical host

**When it is a bad fit:**
- Low-latency / high-throughput paths (IPC adds microseconds to milliseconds)
- Simple deployments where the overhead is not justified
- Resource-constrained MCUs

**Safety analysis:**

| Criterion | Assessment |
|---|---|
| Freedom from interference | **Excellent.** OS-enforced memory isolation. A crash in the serializer process does not corrupt host memory. |
| Fault containment | **Excellent.** Process boundary + OS scheduler isolation. |
| Determinism | **Medium.** IPC latency depends on OS scheduler, buffer sizes, context switches. Can be made deterministic with RT scheduling and pre-allocated buffers. |
| Startup behavior | Slightly slower: two processes must start and handshake. |
| Runtime updateability | **Excellent.** Restart the serializer process with a new binary; host reconnects. |
| ABI stability | **Good.** The IPC wire protocol is the contract — independent of compiler ABI. |
| Diagnosability | **Good.** Each process has its own logs, core dumps, watchdog. |
| Testability | **Good.** Serializer process can be tested independently. |
| Certification complexity | **Medium-high.** Must certify the IPC channel and error-handling logic. But the serializer itself can be developed at a lower ASIL. |
| Security | **Good.** Can use file permissions, namespaces, seccomp on the serializer process. |
| Memory overhead | Higher: two process address spaces, IPC buffers. |
| Recovery after failure | **Excellent.** Host detects serializer death via broken pipe / timeout; restarts it. |
| Traceability | Good. Each binary is independently versioned. |

**Platform suitability:** Linux ✓ · QNX ✓ (native msg passing) · AUTOSAR Adaptive ✓

**Required safeguards for ASIL-B+:**
- Watchdog on both processes
- Timeout on every IPC call with defined fallback behavior
- CRC/checksum on IPC payloads if data integrity is safety-relevant
- Pre-allocated, pinned IPC buffers to avoid allocation jitter
- RT scheduling for the serializer process on the critical path

---

### 3.4 Option 4 — Service-Oriented / AUTOSAR Adaptive Style

**Concept:**
The serializer is deployed as a named service that the host discovers at
runtime.  A service registry maps service names to implementations (which may
be in-process, shared-library, or remote).  This is the pattern used by
AUTOSAR Adaptive `ara::com`.

```
┌─────────────────────────────────────────────────┐
│                   Service Registry              │
│  "BigEndianSerializer/1.0" → SerializerImplA    │
│  "OpaqueSerializer/1.0"   → SerializerImplB    │
└──────────────┬──────────────────────────────────┘
               │ lookup
┌──────────────▼──────────────────────────────────┐
│               Host Application                  │
│  auto* ser = registry.find("BigEndianSer/1.0"); │
│  ser->serialize(buf, size, &msg);               │
└─────────────────────────────────────────────────┘
```

**When appropriate:**
- Systems already using a service-oriented middleware (ara::com, MW COM)
- Product lines where serializer deployment varies per vehicle variant
- When service lifecycle management (start, stop, health) is needed

**When it is a bad fit:**
- Bare-metal or minimal RTOS environments
- Low-overhead paths where registry lookup is unacceptable
- Projects that do not already have a service infrastructure

**Safety analysis:**

| Criterion | Assessment |
|---|---|
| Freedom from interference | Depends on underlying transport (in-process, IPC, or remote). |
| Fault containment | Depends on deployment model. |
| Determinism | **Medium.** Service discovery adds startup latency. |
| Startup behavior | Non-trivial: registry must be populated before consumers start. |
| Runtime updateability | **Good.** Services can be replaced while maintaining the interface contract. |
| ABI stability | **Good.** Service contract (IDL or header) is the interface. |
| Diagnosability | **Good.** Health checks, service status, version queries. |
| Testability | Good. Service can be mocked/stubbed easily. |
| Certification complexity | **Highest.** Must certify the service framework itself (registry, discovery, lifecycle). |
| Security | Good if service framework supports authentication/authorization. |
| Memory overhead | Medium-high: framework infrastructure. |
| Recovery after failure | Depends on framework; typically supports restart/failover. |
| Traceability | Good. Service versions and deployment manifests provide full traceability. |

**Platform suitability:** Linux ✓ · QNX ✓ · AUTOSAR Adaptive ✓✓

**Required safeguards for ASIL-B+:**
- Service framework must be qualified or developed to the same ASIL level
- Deterministic startup sequence (no race between service and consumer)
- Timeouts and fallback on all service calls
- Service identity verification

---

## 4. Detailed ASIL-Level Evaluation

### QM (Quality Management — no safety requirement)

All four mechanisms are acceptable.  Choose based on engineering convenience:
- **Static** for simplicity
- **dlopen** for variant flexibility
- **IPC** if isolation is desired for robustness
- **Service** if middleware is already present

### ASIL-A

Static and dlopen (with safeguards) are both acceptable.  IPC is overkill
unless mixed-criticality is explicitly required.

- **Recommended:** Static registration
- **Acceptable:** dlopen with signature verification and defensive wrappers
- **Acceptable:** IPC if partitioning is already part of the architecture

### ASIL-B

Dynamic loading becomes harder to justify.  The safety case must argue that
`dlopen` cannot introduce systematic faults.

- **Recommended:** Static registration
- **Acceptable:** IPC-based isolation (serializer at lower ASIL)
- **Discouraged:** dlopen (requires extensive argumentation)
- **Conditional:** Service-oriented (only if framework is ASIL-B qualified)

### ASIL-C

- **Recommended:** Static registration
- **Acceptable:** IPC-based with hardware partitioning
- **Not recommended:** dlopen
- **Conditional:** Service-oriented (framework must be ASIL-C)

### ASIL-D

- **Recommended:** Static registration with MC/DC coverage, MISRA compliance
- **Acceptable:** IPC-based with QNX adaptive partitioning or Linux cgroups + RT scheduling
- **Not acceptable:** dlopen
- **Not acceptable:** Service-oriented (unless framework is ASIL-D qualified, which is extremely rare)

---

## 5. Decision Matrix

Scoring: ★★★ = excellent, ★★ = adequate, ★ = poor/risky, ✗ = not recommended

| Criterion | Static | dlopen | IPC | Service |
|---|:---:|:---:|:---:|:---:|
| Freedom from interference | ★★★ | ★ | ★★★ | ★★ |
| Fault containment | ★★ | ★ | ★★★ | ★★ |
| Determinism | ★★★ | ★★ | ★★ | ★★ |
| Startup predictability | ★★★ | ★★ | ★★ | ★ |
| Runtime updateability | ✗ | ★★★ | ★★★ | ★★★ |
| ABI stability | ★★★ | ★ | ★★★ | ★★ |
| Diagnosability | ★★★ | ★★ | ★★★ | ★★★ |
| Testability | ★★★ | ★★ | ★★★ | ★★★ |
| Certification complexity | ★★★ | ★ | ★★ | ★ |
| Security | ★★★ | ★ | ★★★ | ★★ |
| Memory / performance | ★★★ | ★★★ | ★ | ★★ |
| Recovery after failure | ★ | ★ | ★★★ | ★★ |
| Traceability | ★★★ | ★★ | ★★★ | ★★★ |
| **ASIL-D suitability** | **★★★** | **✗** | **★★** | **✗** |
| **ASIL-B suitability** | **★★★** | **★** | **★★★** | **★★** |
| **QM suitability** | **★★★** | **★★★** | **★★★** | **★★★** |

---

## 6. Recommended Architecture by ASIL Level

| ASIL Level | Primary Recommendation | Acceptable Alternative |
|---|---|---|
| **QM** | Static (simplest) | Any of the four |
| **ASIL-A** | Static | dlopen with safeguards |
| **ASIL-B** | Static | IPC with watchdog + timeout |
| **ASIL-C** | Static | IPC with HW partitioning |
| **ASIL-D** | Static (MC/DC, MISRA) | IPC with QNX adaptive partitioning |

---

## 7. Buildable Examples — Overview

All four examples live under `examples/serializer_plugin_*/` and share a
common `Message` struct and `ISerializerPlugin` interface.  Each demonstrates
a different integration mechanism.

| Example directory | Mechanism | Build target |
|---|---|---|
| `examples/serializer_plugin_static/` | Static factory | `//examples/serializer_plugin_static` |
| `examples/serializer_plugin_dlopen/` | dlopen shared lib | `//examples/serializer_plugin_dlopen` |
| `examples/serializer_plugin_ipc/` | Out-of-process pipe | `//examples/serializer_plugin_ipc:host` |
| `examples/serializer_plugin_service/` | In-process service registry | `//examples/serializer_plugin_service` |

See the source files for full code.  Build all with:

```bash
bazel build //examples/serializer_plugin_static //examples/serializer_plugin_dlopen //examples/serializer_plugin_ipc/... //examples/serializer_plugin_service
```

---

## 8. Verification Strategy

| Mechanism | Unit test | Integration test | Fault-injection test |
|---|---|---|---|
| Static | Google Test on each serializer | End-to-end with config selection | Corrupt config → verify fallback |
| dlopen | Test `.so` in isolation + host mock | Load real `.so` in host process | Missing `.so`, corrupt `.so`, ABI mismatch |
| IPC | Test serializer process standalone | Host + serializer process via pipe | Kill serializer mid-call, corrupt pipe data |
| Service | Mock registry + real service | Full registry + service lifecycle | Service unavailable, timeout, version mismatch |

---

## 9. Gaps, Limitations, and Next Steps

1. **QNX-specific evaluation.** This study focuses on Linux-available APIs.
   QNX message passing, adaptive partitioning, and POSIX compliance should be
   evaluated with a QNX SDP environment.

2. **AUTOSAR Adaptive integration.** Option 4 is described generically.  For
   production use, it should be mapped to `ara::com` or the project's MW COM
   framework.

3. **Performance benchmarking.** IPC overhead needs quantification on the
   target hardware with representative message sizes and call frequencies.

4. **Security hardening.** Plugin signature verification (dlopen case) is
   mentioned but not implemented in the example.

5. **Dynamic length-field / TLV serializers.** The examples use a simplified
   serializer.  Extending to the full `SerializerSettings`-aware implementation
   from `src/serializer/` is a natural next step.

6. **Compiler qualification.** For ASIL-C/D, the compiler and standard library
   must be qualified.  This is orthogonal to plugin architecture but a
   prerequisite.

---

*End of study.*
