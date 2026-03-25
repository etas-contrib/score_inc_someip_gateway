<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# com-aap-communication-manager Configuration Summary

The `com-aap-communication-manager` is configured through a multi-stage process involving AUTOSAR XML (`arxml`), Harmony Application Design Language (`.hadl`) files, and environment variables. These are compiled into binary `flatcfg` files for runtime use.

---

## 1. Primary Configuration: ARXML to Flatbuffers

The core configuration workflow starts with `.arxml` files, which define the system's services, deployments, and network properties. These are processed by tools like `vrte_fs` and `flatc` to generate the final binary configurations.

### 1.1. `someip_domain_gateway` Configuration

The `someip_domain_gateway` binary requires three main `flatcfg` files:

| Filename Pattern | Description |
|------------------|-------------|
| `com_<proc_name>__SWCL_1_flatcfg.bin` | Configures the main COM (Communication Management) behavior, including service routing. |
| `log_<proc_name>__SWCL_flatcfg.bin` | Defines the logging behavior and contexts for the gateway. |
| `someip_<proc_name>__SWCL_flatcfg.bin`| Specifies the SOME/IP network configuration, including IP addresses, ports (TCP/UDP), and protocol settings for deployed services. |

### 1.2. User Application Configuration

Each user application (process) requires its own `flatcfg` file that contains the specific deployment information for the services it provides or consumes.

---

## 2. Service-Level Configuration: HADL Annotations

The `.hadl` files, which define the service interfaces, can be annotated to fine-tune runtime behavior.

| Annotation / Keyword | Description | Default Value |
|----------------------|-------------|---------------|
| `@size("...")` | Specifies the serialized data size (in bytes) for an event or method. This is used to pre-allocate shared memory buffers. For methods, the size should be the maximum of the input and output arguments. | `10240` (10 kB) |
| `@maxslotsperpublisher("...")` | Restricts the number of event samples a provider can hold in memory while waiting for all subscribers to handle them. This prevents a slow subscriber from consuming excessive memory on the provider side. | `20` |
| `@maxslotspersubscriber("...")` | Defines the maximum `maxSampleCount` that a subscriber is allowed to request. If a subscriber requests more, the subscription call will fail. | `10` |
| `(tlvId:...)` | Assigns a Tag-Length-Value (TLV) identifier to individual struct members or method arguments. This enables optional fields, allowing for backward and forward compatibility in data structures. | None |
| `fireAndForget` | A keyword applied to a method declaration to indicate that it is a one-way call and the client should not expect a response. | N/A (Methods are request-response by default) |

---

## 3. Runtime Environment Configuration

The runtime behavior can also be influenced by environment variables.

| Environment Variable | Component | Description |
|------------------------|-----------|-------------|
| `ECUCFG_ENV_VAR_ROOTFOLDER` | All | Specifies the root directory on the target filesystem where the `flatcfg` configuration files are located (e.g., `/opt/vrte/etc/config/`). |
| `PIPC_PREALLOCATE_THREADS` | `arapipcd` | Controls thread allocation for the IPC daemon. Set to `ON` to pre-allocate a fixed number of listener threads at startup, or `OFF` (default) for dynamic allocation as clients connect. |

---

## 4. Default Transformation Properties

If not explicitly configured in the `ApSomeipTransformationProps` in the `arxml` deployment, the following default sizes are used for length fields during SOME/IP serialization:

- **`sizeOfArrayLengthField`**: 4 Bytes
- **`sizeOfStringLengthField`**: 4 Bytes
- **`sizeOfStructLengthField`**: 4 Bytes
- **`sizeOfUnionLengthField`**: 4 Bytes

---

## 5. Logging Contexts

The logging framework uses specific context IDs for different components, which can be configured in the logging configuration file.

| Context ID | Component / Purpose |
|------------|---------------------|
| `PIPC` | P-IPC (Inter-Process Communication) library |
| `GGCR` | Generic Gateway Config Reader |
| `SIGW` | SOME/IP Gateway |
| `COM` | COM Libraries (generic) |
| `SOIP` | SOME/IP Libraries |

---

## 6. Configuration Generation and Loading Architecture

### 6.1. Configuration File Generation Mechanism

The binary `flatcfg` files are not created manually. They are the output of a multi-stage toolchain that transforms high-level AUTOSAR definitions into an efficient, machine-readable format.

The process is as follows:

1.  **Input Artifacts**: The primary source of truth is a collection of `.arxml` files. These files holistically describe the system's service interfaces, data types, network bindings, and machine-specific deployments.

2.  **Step 1: `vrte_fs` Tool**: A specialized command-line tool, `vrte_fs`, is used to parse the `.arxml` files and generate intermediate configuration files.
    *   **Command**: `vrte_fs <arxmls> flatbuffers -sc <software_cluster> -fc com -o <gen-dir>`
    *   **Output**: This command produces two human-readable files:
        *   A **Flatbuffers schema** (e.g., `com_flatcfg.fbs`): Defines the structure, tables, and types of the configuration data.
        *   A **JSON file** (e.g., `com_flatcfg.json`): Contains the actual configuration data, conforming to the generated schema.

3.  **Step 2: `flatc` (Flatbuffers Compiler)**: The standard Flatbuffers compiler takes the schema and JSON files to produce the final binary configuration.
    *   **Command**: `flatc -o <gen-dir> -b -c <schema.fbs> <data.json>`
    *   **Output**: A binary `flatcfg` file (e.g., `com_flatcfg.bin`). This file is compact, efficient, and ready for use on the target.

4.  **Step 3: Renaming and Deployment**: The generated binary file must be renamed to match the pattern expected by the runtime components: `com_<proc_name>__<software_cluster>_flatcfg.bin`. It is then deployed to the target filesystem in the directory specified by the `ECUCFG_ENV_VAR_ROOTFOLDER` environment variable.

This entire flow is typically automated within a build pipeline.

### 6.2. Configuration Reader Architecture

The "configuration reader" in this architecture is not a traditional parser for text files like JSON or XML at runtime. Instead, it is a highly efficient **Flatbuffers verifier and accessor**.

*   **Loading**: At startup, a process like `someip_domain_gateway` or a user application locates its required `.bin` files using the `ECUCFG_ENV_VAR_ROOTFOLDER` environment variable and the expected filename pattern. The binary data is then loaded directly into a memory buffer.

*   **Zero-Copy Access**: The key architectural advantage of Flatbuffers is that the data in the buffer does not need to be parsed or deserialized into a different in-memory representation. The process can access the configuration values directly from the raw buffer in a "zero-copy" manner.

*   **Efficiency**: This approach is extremely fast and memory-efficient, which is critical for resource-constrained automotive systems and for minimizing startup times. It avoids the runtime overhead of parsing text and dynamic memory allocation that would be associated with reading JSON or XML files directly.

*   **Type Safety**: The generated C++ accessor functions from the Flatbuffers schema provide a type-safe API for accessing configuration data, preventing errors that could arise from manual data extraction.

In summary, the configuration architecture prioritizes runtime performance and safety by shifting the complexity of parsing and validation into the offline build toolchain. The runtime components consume a pre-compiled, optimized binary format that can be read with minimal overhead.
