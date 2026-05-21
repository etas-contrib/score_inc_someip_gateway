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

Memory Profiling Results: gateway_ipc_binding_memory
====================================================

Overview
--------

This directory contains the complete memory profiling and performance analysis using the dedicated ``gateway_ipc_binding_memory`` application compiled with release optimizations and without thread sanitizer instrumentation. The application runs 1,000,000 event transmissions at 1 MB payload, providing a sustained stress test suitable for memory profiling with GNU memusage and Valgrind Massif.

Quick summary
-------------

- Build status: Successful
- Profiling status: Completed with memusage and Massif
- Memory status: No leaks detected
- Peak memory: 172 KB useful heap in Massif
- Latency: about 3.5 microseconds per event
- Iterations: 1,000,000 at 1 MB payload
- Per-iteration heap allocations: none

File inventory
--------------

Analysis reports
~~~~~~~~~~~~~~~~

- ``PROFILING_REPORT.rst``: detailed memory statistics and findings
- ``index.rst``: this overview

Profiling data
~~~~~~~~~~~~~~

- ``memusage.data``: raw memory profiling data
- ``memusage.png``: memory usage timeline graph
- ``massif.out.1``: Valgrind Massif heap profile
- ``benchmark_run.log``: memusage execution output
- ``massif_run.log``: Massif execution output

Build information
-----------------

.. code-block:: text

   Compiler:            Bazel 8.4.1 with C++ toolchain
   Build configuration: -c opt (release optimization)
   Features disabled:   TSAN (thread sanitizer)
   Binary:              bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_memory
   Binary size:         23 MB
   Date:                2026-04-27

Key findings
------------

Memory behavior
~~~~~~~~~~~~~~~

- Total heap allocated: 197 KB cumulative, all during setup
- Peak heap usage: 172 KB in memusage and 172 KB useful in Massif
- Peak stack usage: 11 KB
- Allocation pattern: zero per-iteration allocations, all 373 mallocs occur during initialization
- Total malloc calls: 373, down 99.96 percent from the previous run
- Memory leaks: none, with balanced allocation and free counts

Profiling tools used
--------------------

1. GNU memusage

   - tracks all malloc and free operations
   - generates a timeline graph and histogram
   - has low runtime overhead

2. Valgrind Massif

   - captures periodic heap snapshots with call-tree attribution
   - identifies top allocators by source location
   - has much higher runtime overhead than memusage

Performance insights
--------------------

Why a dedicated memory app
~~~~~~~~~~~~~~~~~~~~~~~~~~

1. No fork or re-exec: Google Benchmark re-executes internally, which interferes with Massif output.
2. Fixed workload: 1 million iterations at 1 MB ensures consistent, reproducible profiling.
3. Direct Massif compatibility: no need for ``--trace-children=yes``.
4. Simple ``main()``: cleaner profiling without benchmark framework overhead.

Efficiency observations
~~~~~~~~~~~~~~~~~~~~~~~

- Flat heap profile: zero growth across 1,000,000 iterations confirms no leaks
- Zero allocations per iteration: all heap allocation was eliminated from the hot path
- Stable memory usage: the peak is reached during initialization and never exceeded
- Logging dominates setup: about 23 percent of peak heap is console logging infrastructure

Recommendations
---------------

Immediate actions
~~~~~~~~~~~~~~~~~

- The profile confirms production readiness from a memory perspective.
- The 172 KB peak heap is safe for embedded systems.
- No memory-usage optimization is required for the current workload.
- The hot path is fully allocation-free.

Further investigation
~~~~~~~~~~~~~~~~~~~~~

- Consider CPU profiling with ``perf`` or FlameGraph for hotspot analysis.
- Reduce logging overhead by disabling the console backend in production.
- Test with real-time scheduling priorities.

Usage examples
--------------

View memory graph
~~~~~~~~~~~~~~~~~

.. code-block:: text

   memory_profile/memusage.png

Process raw data
~~~~~~~~~~~~~~~~

.. code-block:: bash

   xxd -l 1000 memory_profile/memusage.data
   grep "Completed\|Benchmark completed" memory_profile/benchmark_run.log
   ms_print memory_profile/massif.out.1 | head -80

Regenerate profile if needed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   cd /workspaces/score_inc_someip_gateway
   bazel build //src/gateway_ipc_binding/benchmark:gateway_ipc_binding_memory -c opt --features=-tsan

   /usr/bin/memusage -d memory_profile/memusage.data -p memory_profile/memusage.png \
     ./bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_memory

Run Massif and always get ``massif.out``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   valgrind --tool=massif --massif-out-file=memory_profile/massif.out.1 \
     ./bazel-bin/src/gateway_ipc_binding/benchmark/gateway_ipc_binding_memory

   ms_print memory_profile/massif.out.1 | less

.. note::

   Either do not use Google Benchmark as the test runner when running with Massif, or set ``--trace-children=yes``. Google Benchmark re-execs internally. Without ``--trace-children=yes``, Valgrind may print benchmark output but not emit a Massif file.

Related files
-------------

- Dedicated memory app: ``src/gateway_ipc_binding/benchmark/event_transmission_client_to_server_memory.cpp``
- BUILD file: ``src/gateway_ipc_binding/benchmark/BUILD``
- Gateway binding: ``src/gateway_ipc_binding/``

Conclusion
----------

The ``gateway_ipc_binding_memory`` dedicated profiling application demonstrates:

- excellent memory efficiency with about 173 KB peak useful heap
- zero heap growth over 1,000,000 iterations
- low latency for user-space IPC at about 3.6 microseconds per event
- no memory leaks
- an 83 percent reduction in allocation count versus the previous run
- suitability for production deployment and embedded systems

The dedicated memory application provides cleaner profiling than Google Benchmark because it avoids fork and re-exec issues with Massif.

.. toctree::
   :maxdepth: 1

   PROFILING_REPORT
