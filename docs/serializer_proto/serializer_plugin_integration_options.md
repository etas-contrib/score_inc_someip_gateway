<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# Serializer Plugin Integration in ASIL Systems

This document lists mechanisms and frameworks suitable for integrating the serializer as a loadable plugin in safety-critical (ASIL) automotive systems. Each option is evaluated for suitability in terms of safety, isolation, and maintainability.

---

## 1. Dynamic Shared Library Loading (dlopen/dlsym)
- **Mechanism:** Load `.so` (shared object) files at runtime using POSIX APIs (`dlopen`, `dlsym`, `dlclose`).
- **Pros:**
  - Standard on Linux/QNX
  - Allows late binding and versioning
  - Can restrict exported symbols (visibility)
- **Cons:**
  - Requires careful interface design (C ABI or stable C++ ABI)
  - Error handling and fallback logic needed
  - Not all safety assessors accept dynamic loading unless strictly controlled
- **ASIL Notes:**
  - Acceptable if plugin interface is strictly validated and loaded code is certified or isolated

## 2. Static Plugin Registration (Compile-Time)
- **Mechanism:** Register plugin serializers at build time via factory pattern or static registry
- **Pros:**
  - No runtime loading risk
  - Full compile-time type safety
  - Simpler for safety assessment
- **Cons:**
  - No runtime extensibility
  - Requires full system rebuild for new plugins
- **ASIL Notes:**
  - Preferred for highest ASIL levels (ASIL-B, ASIL-D)

## 3. IPC-based Plugin (Out-of-Process)
- **Mechanism:** Run serializer plugin as a separate process, communicate via IPC (e.g., UNIX domain sockets, QNX message passing, or shared memory)
- **Pros:**
  - Strong isolation (memory, faults)
  - Can restart plugin process on failure
  - Allows mixed ASIL/QM partitioning
- **Cons:**
  - IPC overhead
  - More complex deployment
- **ASIL Notes:**
  - Recommended for mixed-criticality systems or when plugin is not ASIL-certified

## 4. In-Process Plugin Frameworks
- **Examples:**
  - [Boost.Extension](https://www.boost.org/doc/libs/release/libs/extension/)
  - [Qt Plugin System](https://doc.qt.io/qt-5/plugins-howto.html)
  - [Poco Foundation Plugins](https://pocoproject.org/docs/Poco.ClassLoader.html)
- **Pros:**
  - Abstracts platform-specific loading
  - Supports versioning, metadata, and interface checks
- **Cons:**
  - Adds third-party dependencies
  - Must ensure framework itself is ASIL-compliant or isolated
- **ASIL Notes:**
  - Use only if framework is qualified or can be isolated/verified

## 5. AUTOSAR Adaptive Platform Service Discovery
- **Mechanism:** Use AUTOSAR Adaptive's service-oriented communication and dynamic service discovery to load/activate serializer services at runtime
- **Pros:**
  - Standardized for automotive
  - Integrates with Adaptive Platform lifecycle and safety mechanisms
- **Cons:**
  - Requires full AUTOSAR Adaptive stack
  - More heavyweight than direct plugin loading
- **ASIL Notes:**
  - Suitable for high-ASIL Adaptive systems

---

## Summary Table

| Mechanism                | Runtime Extensible | Isolation | ASIL Suitability |
|-------------------------|--------------------|-----------|------------------|
| dlopen/dlsym            | Yes                | In-process| B/C (with care)  |
| Static registration     | No                 | N/A       | A/B/C/D          |
| IPC-based plugin        | Yes                | Strong    | B/C/D            |
| In-process frameworks   | Yes                | In-process| B (if qualified) |
| AUTOSAR Adaptive SDC    | Yes                | Platform  | B/C/D            |

---

**Recommendation:**
- For highest ASIL levels, prefer static registration or IPC-based plugins with strong interface validation and isolation.
- If runtime extensibility is required, use IPC or qualified plugin frameworks, and ensure all loaded code is safety-assessed.
