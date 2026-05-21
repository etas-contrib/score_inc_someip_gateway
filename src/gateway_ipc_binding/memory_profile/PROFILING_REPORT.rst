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

Memory Profiling Report: gateway_ipc_binding_memory
===================================================

Build configuration
-------------------

- Build type: Release with ``-c opt``
- Compilation optimizations: enabled
- Thread sanitizer: disabled by removing the ``-tsan`` feature
- Binary: ``gateway_ipc_binding_memory`` dedicated memory profiling application
- Binary path: ``bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_memory``
- Binary size: about 23 MB
- Bazel version: 8.4.1
- Build date: 2026-04-27

Profiling setup
---------------

- Tools: GNU memusage and Valgrind Massif 3.22.0
- Workload: 1,000,000 iterations with 1 MB payload
- Test duration: about 5.4 seconds with memusage and about 48 seconds with Massif
- Profiling data: ``memusage.data``, ``memusage.png``, and ``massif.out.1``

Memory usage summary
--------------------

GNU memusage statistics
~~~~~~~~~~~~~~~~~~~~~~~

- Total heap allocated: 201,941 bytes, about 197 KB cumulative, all during setup
- Peak heap usage: 176,034 bytes, about 172 KB
- Peak stack usage: 10,960 bytes, about 11 KB

Allocation operations
~~~~~~~~~~~~~~~~~~~~~

- Total malloc calls: 373
- Total realloc calls: 0
- Total calloc calls: 3
- Total free calls: 377
- Failed allocations: 0

Valgrind Massif statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~

- Peak heap usage, useful: 176,002 bytes, about 172 KB
- Peak heap usage, total with overhead: 178,800 bytes, about 175 KB
- Number of snapshots: 65
- Heap profile shape: flat and stable across the entire run

Allocation pattern histogram
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All 373 allocations occur during setup, with diverse sizes across initialization:

- 32 to 47 bytes: 73 blocks, 19 percent
- 16 to 31 bytes: 56 blocks, 14 percent
- 48 to 63 bytes: 40 blocks, 10 percent
- 0 to 15 bytes: 35 blocks, 9 percent
- 4096 to 4111 bytes: 24 blocks, 6 percent
- 112 to 127 bytes: 17 blocks, 4 percent
- 240 to 255 bytes: 16 blocks, 4 percent
- Other sizes: 112 blocks, 30 percent

Execution performance
---------------------

Dedicated memory profiling application with 1 MB payload and 1 million iterations.

Event transmission
~~~~~~~~~~~~~~~~~~

- Total benchmark time: 3,475 ms
- Total execution time: 5,435 ms including setup and teardown
- Latency per iteration: about 3.5 microseconds
- Iterations: 1,000,000

Per-iteration allocation cost
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Malloc calls per iteration: 0
- Bytes allocated per iteration: 0
- Net heap growth per iteration: 0

Massif top allocators at peak snapshot 40
-----------------------------------------

.. list-table::
   :header-rows: 1

   * - Allocator
     - Size
     - Percent of peak
   * - ``ClientConnection::ClientConnection`` vector reserve
     - 81,920 B
     - 45.8
   * - ``ConsoleRecorderFactory::CreateConsoleLoggingBackend`` logging backend
     - 24,792 B
     - 13.9
   * - ``LogRecord`` circular allocator logging buffer
     - 16,384 B
     - 9.2
   * - Other, 153 places below threshold
     - 23,794 B
     - 13.3
   * - Allocator overhead, extra heap
     - 2,798 B
     - 1.6

Key findings
------------

1. Allocation-free hot path: zero heap allocations during 1 million iterations, with all 373 mallocs occurring during initialization only.
2. Stable memory profile: peak heap is about 172 KB useful bytes and stays flat across the run.
3. Peak memory: about 172 KB useful heap and about 175 KB total with overhead.
4. No leaks: free and allocation counts balance as expected.
5. Dramatic allocation reduction: 373 total malloc calls, down from more than 1 million in the previous run.
6. Dominant allocator: ``ClientConnection`` buffer reserve accounts for about 46 percent of peak heap.
7. Logging overhead: console logging backend accounts for about 23 percent of peak heap.

Comparison with previous runs
-----------------------------

.. list-table::
   :header-rows: 1

   * - Metric
     - Apr 22
     - Apr 27 previous
     - Current
     - Change vs previous
   * - Total malloc calls
     - 6,021,543
     - 1,005,137
     - 373
     - -99.96 percent
   * - Total heap allocated
     - 283 MB
     - 168 MB
     - 197 KB
     - -99.9 percent
   * - Peak heap memusage
     - 172 KB
     - 174 KB
     - 172 KB
     - -1 percent
   * - Peak heap Massif useful
     - 172 KB
     - 173 KB
     - 172 KB
     - -1 percent
   * - Benchmark latency
     - 3.9 microseconds
     - 3.6 microseconds
     - 3.5 microseconds
     - -3 percent
   * - Total execution time
     - 6.2 s
     - 5.7 s
     - 5.4 s
     - -5 percent
   * - Binary size
     - 18 MB
     - 23 MB
     - 23 MB
     - 0 percent
   * - Bazel version
     - 8.0.0
     - 8.4.1
     - 8.4.1
     - same
   * - Malloc calls per iteration
     - about 6
     - about 1
     - 0
     - -100 percent

Notable result: per-iteration heap allocations have been completely eliminated. The 373 total malloc calls all occur during initialization. Cumulative heap allocation dropped from 168 MB to 197 KB, confirming the event transmission hot path is now allocation-free.

Recommendations
---------------

- Allocation-free hot path: zero per-iteration allocations confirms real-time suitability without allocator jitter.
- Light memory footprint: about 172 KB useful heap is suitable for embedded and real-time systems.
- Flat allocation profile: zero heap growth over 1 million iterations confirms stable memory behavior.
- Logging overhead: around 23 percent of peak heap is logging infrastructure, so production builds could use a lighter backend.
- ``ClientConnection`` buffer: the 80 KB reserve is a one-time setup cost and acceptable for long-running services.

Output files
------------

- ``memusage.data``: raw profiling data file
- ``memusage.png``: memory usage timeline graph
- ``massif.out.1``: Valgrind Massif heap profile
- ``benchmark_run.log``: memusage execution log
- ``massif_run.log``: Massif execution log
- ``PROFILING_REPORT.rst``: this report
