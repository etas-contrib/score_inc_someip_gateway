/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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

// =============================================================================
// Demo: Layered CarWindow Serializer Architecture
//
// Demonstrates the four-layer architecture:
//   Layer 1: serializer_types.h       — framework interfaces
//   Layer 2: base_type_serializers.h  — BaseTypeSerializerFactory
//   Layer 3: abstract_app_factory.h   — AbstractCarWindowFactory
//   Layer 4: car_window_factory.h     — CarWindowFactory (concrete)
//
// Shows how the same data types are serialized differently depending on
// the deployment configuration (SOME/IP big-endian vs IPC native).
// =============================================================================

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "car_window_factory.h"

static void printBytes(const char* label, const reader::ByteVector& bytes) {
    std::cout << label << " (" << bytes.size() << " bytes): ";
    for (auto b : bytes) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << static_cast<int>(b) << ' ';
    }
    std::cout << std::dec << '\n';
}

int main() {
    using namespace car_window_types;

    std::cout << "=== Layered CarWindow Serializer Architecture Demo ===\n\n";

    // -------------------------------------------------------------------------
    // 1. Print deployment info (from ARXML SERVICE-INTERFACE-DEPLOYMENT)
    // -------------------------------------------------------------------------
    std::cout << "-- Deployment constants (from ARXML) --\n";
    std::cout << "  WindowInfo   serviceId="
              << reader::CarWindowFactory::window_info_service_id()
              << "  eventId="
              << reader::CarWindowFactory::window_info_event_id() << '\n';
    std::cout << "  WindowControl serviceId="
              << reader::CarWindowFactory::window_control_service_id()
              << "  eventId="
              << reader::CarWindowFactory::window_control_event_id() << '\n';
    std::cout << '\n';

    // -------------------------------------------------------------------------
    // 2. SOME/IP factory (big-endian, per ARXML SOMEIP-TRANSFORMATION-PROPS)
    // -------------------------------------------------------------------------
    std::cout << "-- SOME/IP deployment (big-endian) --\n";

    // Layer 2 + Layer 4 bundled for convenience
    reader::CarWindowSomeipBundle someip;

    WindowInfo info{0x12345678U, WindowState::Opening};
    reader::RefNode<WindowInfo> infoNode(info);

    auto infoSer = someip.app.create_window_info_serializer();
    auto someipBytes = infoSer->serialize(infoNode);
    printBytes("  WindowInfo BE", someipBytes);

    assert(someipBytes.size() == 8);
    assert(someipBytes[0] == 0x12 && someipBytes[3] == 0x78);
    assert(someipBytes[4] == 0x00 && someipBytes[7] == 0x01);
    std::cout << "  ✓ Big-endian byte order verified\n";

    // Round-trip
    WindowInfo infoBack{};
    reader::RefNode<WindowInfo> infoBackNode(infoBack);
    infoSer->deserialize(someipBytes, infoBackNode);
    assert(infoBack.pos == 0x12345678U && infoBack.state == WindowState::Opening);
    std::cout << "  ✓ WindowInfo round-trip: pos=0x" << std::hex << infoBack.pos
              << " state=" << std::dec << static_cast<uint32_t>(infoBack.state) << '\n';

    // WindowControl
    WindowControl ctrl{WindowCommand::Close};
    reader::RefNode<WindowControl> ctrlNode(ctrl);
    auto ctrlSer = someip.app.create_window_control_serializer();
    auto ctrlBytes = ctrlSer->serialize(ctrlNode);
    printBytes("  WindowControl BE", ctrlBytes);
    assert(ctrlBytes.size() == 4 && ctrlBytes[3] == 0x02);
    std::cout << "  ✓ WindowControl big-endian verified\n";

    WindowControl ctrlBack{};
    reader::RefNode<WindowControl> ctrlBackNode(ctrlBack);
    ctrlSer->deserialize(ctrlBytes, ctrlBackNode);
    assert(ctrlBack.command == WindowCommand::Close);
    std::cout << "  ✓ WindowControl round-trip: command="
              << static_cast<uint32_t>(ctrlBack.command) << '\n';

    // -------------------------------------------------------------------------
    // 3. IPC factory (opaque / native byte order)
    // -------------------------------------------------------------------------
    std::cout << "\n-- IPC deployment (opaque / native) --\n";

    reader::CarWindowIpcBundle ipc;
    auto ipcInfoSer = ipc.app.create_window_info_serializer();
    auto ipcBytes = ipcInfoSer->serialize(infoNode);
    printBytes("  WindowInfo native", ipcBytes);

    assert(ipcBytes.size() == 8);
    assert(ipcBytes[0] == 0x78);  // LSB first on LE host
    std::cout << "  ✓ Native byte order verified (no swap)\n";

    WindowInfo ipcInfoBack{};
    reader::RefNode<WindowInfo> ipcInfoBackNode(ipcInfoBack);
    ipcInfoSer->deserialize(ipcBytes, ipcInfoBackNode);
    assert(ipcInfoBack.pos == 0x12345678U && ipcInfoBack.state == WindowState::Opening);
    std::cout << "  ✓ IPC WindowInfo round-trip OK\n";

    // -------------------------------------------------------------------------
    // 4. Cross-deployment comparison
    // -------------------------------------------------------------------------
    std::cout << "\n-- Cross-deployment comparison --\n";
    bool bytesAreDifferent = (someipBytes != ipcBytes);
    std::cout << "  SOME/IP bytes == IPC bytes? "
              << (bytesAreDifferent ? "NO (correct!)" : "YES") << '\n';
    assert(bytesAreDifferent);

    // -------------------------------------------------------------------------
    // 5. Demonstrate base-type factory directly
    // -------------------------------------------------------------------------
    std::cout << "\n-- Base-type factory (Layer 2) --\n";
    auto u32Ser = someip.base.create_uint32_serializer();
    uint32_t testVal = 0xDEADBEEF;
    reader::RefNode<uint32_t> valNode(testVal);
    auto valBytes = u32Ser->serialize(valNode);
    printBytes("  0xDEADBEEF BE", valBytes);
    assert(valBytes[0] == 0xDE && valBytes[3] == 0xEF);
    std::cout << "  ✓ Primitive uint32 big-endian verified\n";

    std::cout << "\n=== All demo assertions passed. ===\n";
    return 0;
}
