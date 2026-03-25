/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
// SPDX-License-Identifier: Apache-2.0
// Option 2 — Host that loads a serializer plugin via dlopen.
//
// Contract:
//   Input  : path to a .so plugin (argv[1])
//   Output : round-trip serialize→deserialize of a Message
//   Failure: missing .so, ABI mismatch, serialize/deserialize error → exit 1
//   Safety : C-ABI boundary; host validates ABI version before use
//
// Build : bazel build //examples/serializer_plugin_dlopen:dlopen_demo
//         bazel build //examples/serializer_plugin_dlopen:big_endian_plugin
// Run   : bazel-bin/examples/serializer_plugin_dlopen/dlopen_demo \
//             bazel-bin/examples/serializer_plugin_dlopen/libbig_endian_plugin.so

#include "examples_serializers/serializer_plugin_dlopen/plugin_abi.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <path-to-plugin.so>\n", argv[0]);
        return 1;
    }

    // --- Load plugin ---
    void* handle = dlopen(argv[1], RTLD_NOW);
    if (!handle) {
        std::fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
    }

    // --- Resolve symbols ---
    auto get_version    = reinterpret_cast<fn_abi_version>(dlsym(handle, "serializer_plugin_abi_version"));
    auto get_name       = reinterpret_cast<fn_name>(dlsym(handle, "serializer_plugin_name"));
    auto do_serialize   = reinterpret_cast<fn_serialize>(dlsym(handle, "serializer_plugin_serialize"));
    auto do_deserialize = reinterpret_cast<fn_deserialize>(dlsym(handle, "serializer_plugin_deserialize"));

    if (!get_version || !get_name || !do_serialize || !do_deserialize) {
        std::fprintf(stderr, "Symbol resolution failed: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

    // --- ABI version check ---
    uint32_t ver = get_version();
    if (ver != SERIALIZER_PLUGIN_ABI_VERSION) {
        std::fprintf(stderr, "ABI mismatch: host=%u plugin=%u\n",
                     SERIALIZER_PLUGIN_ABI_VERSION, ver);
        dlclose(handle);
        return 1;
    }

    std::printf("Loaded plugin: %s  (ABI v%u)\n", get_name(), ver);

    // --- Round-trip demo ---
    PluginMessage orig{};
    orig.id = 0xCAFEBABE;
    std::strncpy(orig.payload, "Hello from dlopen host", sizeof(orig.payload) - 1);

    PluginWireBuffer wire{};
    if (do_serialize(&orig, &wire) != 0) {
        std::fprintf(stderr, "Serialization failed\n");
        dlclose(handle);
        return 1;
    }
    std::printf("Serialized %u bytes.  First 4: %02X %02X %02X %02X\n",
                wire.length, wire.data[0], wire.data[1], wire.data[2], wire.data[3]);

    PluginMessage restored{};
    if (do_deserialize(&wire, &restored) != 0) {
        std::fprintf(stderr, "Deserialization failed\n");
        dlclose(handle);
        return 1;
    }
    std::printf("Round-trip: id=0x%08X payload=\"%s\"\n", restored.id, restored.payload);

    if (restored.id != orig.id || std::strcmp(restored.payload, orig.payload) != 0) {
        std::fprintf(stderr, "MISMATCH!\n");
        dlclose(handle);
        return 1;
    }
    std::printf("OK — round-trip verified.\n");

    dlclose(handle);
    return 0;
}
