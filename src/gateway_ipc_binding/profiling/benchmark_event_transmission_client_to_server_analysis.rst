..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

benchmark_event_transmission_client_to_server Profiling Analysis
================================================================

Scope
-----

- Benchmark target: ``//src/gateway_ipc_binding/benchmark:gateway_ipc_binding_benchmark``
- Benchmark case profiled: ``benchmark_event_transmission_client_to_server/1024/manual_time``
- Build mode used for final analysis: ``-c opt --features=-tsan``
- Profiling command: ``perf record -F 999 -g --call-graph dwarf,16384``

Artifacts
---------

- Flamegraph, final non-TSan: ``profiling/event_transmission_client_to_server_1024_notsan_flamegraph.svg``
- Perf data, final non-TSan: ``profiling/perf_event_c2s_1024_notsan_ok.data``
- Folded stacks: ``profiling/perf_event_c2s_1024_notsan_ok.folded``
- Percent summary: ``profiling/perf_event_c2s_1024_notsan_ok_percent_summary.txt``

Benchmark result used for profile
---------------------------------

- ``benchmark_event_transmission_client_to_server/1024/manual_time``: 4382 ns manual time, 4210 ns CPU time, 139426 iterations, 222.851 MiB/s
- Previous result: about 5228 ns manual time
- Improvement: about 16.2 percent latency reduction

Important caveat
----------------

An initial profile was collected while ThreadSanitizer instrumentation was active because ``user.bazelrc`` contained ``common --features=tsan``.
That profile was dominated by ``__tsan_*`` internals and is not representative for release bottlenecks.
The final analysis below uses the non-TSan profile.

Bottlenecks, non-TSan
---------------------

Percentages below are cumulative stack presence from folded stacks and indicate where execution time is spent across call paths.

1. Syscall and scheduler heavy path

   - ``entry_SYSCALL_64_after_hwframe``: about 74.15 percent
   - ``do_syscall_64``: about 73.13 percent
   - ``x64_sys_call``: about 69.68 percent
   - ``schedule`` and ``__schedule`` combined: about 30.38 percent
   - Interpretation: the event transmission path remains dominated by kernel transitions, waiting and wakeup, and scheduling overhead rather than payload processing.

2. Poll and recv and send loop overhead

   - ``score::os::SysPollImpl::poll``: about 26.22 percent
   - ``__GI___poll``: about 25.56 percent
   - ``score::os::SocketImpl::recvmsg``: about 30.74 percent
   - ``score::os::SocketImpl::sendmsg``: about 8.78 percent
   - ``do_sys_poll``: about 21.39 percent
   - ``__x64_sys_recvmsg``: about 28.49 percent
   - ``__x64_sys_sendmsg``: about 8.07 percent
   - Interpretation: the receive path is now a larger share of the workload than the send path, while poll overhead remains central.

3. Message handling and dispatch path

   - ``score::message_passing::detail::ClientConnection::ProcessInputEvent``: about 33.26 percent
   - ``score::message_passing::UnixDomainEngine::ReceiveProtocolMessage``: about 31.23 percent
   - ``score::message_passing::detail::UnixDomainServer::ServerConnection::ProcessInput``: about 13.95 percent
   - ``score::gateway_ipc_binding::Gateway_ipc_binding_base::handle_event_update_message``: about 7.08 percent
   - ``score::gateway_ipc_binding::Gateway_ipc_binding_base::handle_request_service_message``: about 7.63 percent
   - ``score::gateway_ipc_binding::Gateway_ipc_binding_base::handle_payload_consumed_message``: about 6.47 percent
   - Interpretation: client-side input processing is more prominent than server-side processing, and event handling itself has become cheaper.

4. Shared memory management overhead

   - ``score::gateway_ipc_binding::Shared_memory_managers::payload_consumed``: about 5.16 percent
   - ``unordered_map::erase`` for the shared memory allocation map: about 4.00 percent
   - ``Shared_memory_allocation::~Shared_memory_allocation``: about 3.64 percent
   - ``Read_only_shared_memory_payload::__on_zero_shared``: about 3.34 percent
   - Interpretation: shared memory slot lifecycle management contributes a visible share of the remaining cost.

5. Synchronization overhead

   - ``std::__1::mutex::lock``: about 4.99 percent
   - ``__GI___lll_lock_wait``: about 4.47 percent
   - ``std::__1::condition_variable::wait``: about 4.21 percent
   - ``do_futex`` and ``futex_wait``: about 10.49 percent and 8.24 percent
   - ``std::__1::shared_ptr::~shared_ptr`` and ``__release_shared``: about 7.26 percent and 7.04 percent
   - Interpretation: mutex contention, futex waiting, and shared pointer lifetime management remain material costs.

6. Kernel-side micro-costs

   - ``fdget``: about 2.63 percent
   - ``simple_copy_to_iter``: about 3.93 percent
   - ``copy_msghdr_from_user``: about 4.34 percent
   - ``consume_skb``: about 8.21 percent
   - ``kfree_skbmem`` and ``kmem_cache_free``: about 3.87 percent each
   - Interpretation: socket buffer allocation and freeing, plus data copy operations in the kernel, remain steady overhead.

Improvement suggestions
-----------------------

1. Reduce syscall frequency per event.

   - batch event updates where semantics allow
   - coalesce notifications and transmit multiple updates in one message
   - prefer fewer, larger transfers over many small transfer operations

2. Optimize shared memory slot lifecycle.

   - replace per-event hash-map insert and erase with a slot pool and index-based lookup
   - reuse allocation entries rather than creating and destroying them each time

3. Replace or reduce poll wakeups.

   - move away from poll-loop patterns where the transport layer allows it
   - tune poll timeout and wake strategy to reduce short sleep cycles

4. Reduce mutex contention in hot callbacks.

   - review whether a lock-free or reader-writer pattern can replace the mutex on the event delivery path
   - consider per-connection locks to reduce contention scope

5. Reduce kernel socket buffer churn.

   - use pre-allocated socket buffers or zero-copy sendmsg paths if supported by the transport layer

Comparison with previous profile
--------------------------------

.. list-table::
   :header-rows: 1

   * - Metric
     - Previous
     - Current
     - Change
   * - Benchmark latency
     - about 5228 ns
     - about 4382 ns
     - -16.2 percent
   * - ``entry_SYSCALL_64_after_hwframe``
     - 72.31 percent
     - 74.15 percent
     - +1.84 percentage points
   * - ``handle_event_update_message``
     - 11.92 percent
     - 7.08 percent
     - -4.84 percentage points
   * - ``SysPollImpl::poll``
     - 25.76 percent
     - 26.22 percent
     - +0.46 percentage points
   * - ``SocketImpl::recvmsg``
     - 24.77 percent
     - 30.74 percent
     - +5.97 percentage points
   * - ``SocketImpl::sendmsg``
     - 16.79 percent
     - 8.78 percent
     - -8.01 percentage points
   * - ``schedule`` and ``__schedule``
     - 32.01 percent
     - 30.38 percent
     - -1.63 percentage points
   * - Shared memory map operations
     - not visible
     - about 16 percent
     - new
   * - ``shared_ptr`` reference counting
     - not visible
     - about 14 percent
     - new

Key takeaways
-------------

- The event update handling path improved significantly, helping drive the 16 percent latency reduction.
- The send path cost dropped substantially, suggesting fewer or more efficient send operations.
- Shared memory management and reference counting are now visible as the next optimization targets.
- Overall syscall dominance remains, so the workload is still kernel-bound.

Stability note
--------------

- The profiled run completed successfully with 139426 iterations and no failures.
- The previous stability issue, failed benchmark event transmission, was not observed in this run.
