..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
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

SOMEIP Gateway Architecture
===========================

Components
----------

.. comp:: SOME/IP Stack Daemon
   :id: comp__someipd
   :status: valid
   :safety: QM
   :security: NO

   The SOME/IP stack daemon (QM), wrapping vsomeip for all network I/O
   and SOME/IP Service Discovery.

Design decisions
----------------

.. toctree::
   :maxdepth: 1

   dec_someipgw_registration.rst
   tc8_conformance_testing.rst
