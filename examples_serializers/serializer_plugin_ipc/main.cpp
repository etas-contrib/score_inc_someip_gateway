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
// Option 3 — Host process that communicates with the serializer child via pipes.
//
// Contract:
//   Input  : path to serializer_child binary (argv[1])
//   Output : round-trip serialize→deserialize of a Message via IPC
//   Failure: child crash, pipe error, timeout → error message, exit 1
//   Safety : memory isolation via process boundary; child can be lower ASIL
//
// Build : bazel build //examples/serializer_plugin_ipc/...
// Run   : bazel-bin/examples/serializer_plugin_ipc/ipc_host \
//             bazel-bin/examples/serializer_plugin_ipc/serializer_child

#include "ipc_protocol.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <path-to-serializer_child>\n", argv[0]);
        return 1;
    }

    // Create two pipes: host→child and child→host.
    int to_child[2];    // to_child[0]=read end (child), to_child[1]=write end (host)
    int from_child[2];  // from_child[0]=read end (host), from_child[1]=write end (child)
    if (pipe(to_child) != 0 || pipe(from_child) != 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // --- Child process ---
        dup2(to_child[0], STDIN_FILENO);
        dup2(from_child[1], STDOUT_FILENO);
        close(to_child[0]);
        close(to_child[1]);
        close(from_child[0]);
        close(from_child[1]);
        execl(argv[1], argv[1], nullptr);
        perror("execl");
        _exit(127);
    }

    // --- Host process ---
    close(to_child[0]);
    close(from_child[1]);

    auto send_request = [&](const ipc::Request& req, ipc::Response& resp) -> bool {
        ssize_t w = ::write(to_child[1], &req, sizeof(req));
        if (w != static_cast<ssize_t>(sizeof(req))) return false;
        ssize_t r = ::read(from_child[0], &resp, sizeof(resp));
        if (r != static_cast<ssize_t>(sizeof(resp))) return false;
        return true;
    };

    // --- Serialize ---
    ipc::Request req{};
    req.cmd    = ipc::Command::SERIALIZE;
    req.msg_id = 0xDEADBEEF;
    std::strncpy(req.msg_payload, "Hello from IPC host", sizeof(req.msg_payload) - 1);

    ipc::Response resp{};
    if (!send_request(req, resp) || resp.status != 0) {
        std::fprintf(stderr, "Serialize RPC failed (status=%d)\n", resp.status);
        kill(pid, SIGTERM);
        return 1;
    }
    std::printf("Serialized %u bytes via child process.  First 4: %02X %02X %02X %02X\n",
                resp.wire_length, resp.wire_data[0], resp.wire_data[1],
                resp.wire_data[2], resp.wire_data[3]);

    // --- Deserialize (send the wire data back) ---
    ipc::Request dreq{};
    dreq.cmd = ipc::Command::DESERIALIZE;
    std::memcpy(dreq.wire_data, resp.wire_data, sizeof(dreq.wire_data));
    dreq.wire_length = resp.wire_length;

    ipc::Response dresp{};
    if (!send_request(dreq, dresp) || dresp.status != 0) {
        std::fprintf(stderr, "Deserialize RPC failed (status=%d)\n", dresp.status);
        kill(pid, SIGTERM);
        return 1;
    }
    std::printf("Round-trip: id=0x%08X payload=\"%s\"\n", dresp.msg_id, dresp.msg_payload);

    if (dresp.msg_id != req.msg_id || std::strcmp(dresp.msg_payload, req.msg_payload) != 0) {
        std::fprintf(stderr, "MISMATCH!\n");
        kill(pid, SIGTERM);
        return 1;
    }
    std::printf("OK — round-trip verified via IPC.\n");

    // --- Quit child ---
    ipc::Request qreq{};
    qreq.cmd = ipc::Command::QUIT;
    ::write(to_child[1], &qreq, sizeof(qreq));

    close(to_child[1]);
    close(from_child[0]);
    waitpid(pid, nullptr, 0);
    return 0;
}
