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

SOME/IP Protocol Features
=========================

These local feature requirements define the SOME/IP Gateway behavior in scope
for TC8 conformance. They derive from the local interoperability stakeholder
requirement and selected mirrored Open SOME/IP stakeholder requirements; each
mirrored ID links to the pinned upstream source line. The import also retains
the remaining upstream requirement identifiers.

Service Discovery
-----------------

.. feat_req:: SOME/IP-SD OfferService Format
   :id: feat_req__someip_gateway__sd_offer
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_47, stkh_req__extspec__someipsd_253
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall publish configured services using OfferService entries
   whose service identity, version, and lifetime fields reflect the configured
   service.

.. feat_req:: SOME/IP-SD Cyclic Offers
   :id: feat_req__someip_gateway__sd_cyclic_offer
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_79, stkh_req__extspec__someipsd_81
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall repeat active OfferService entries in the main phase
   according to its configured cyclic offer interval.

.. feat_req:: SOME/IP-SD FindService Responses
   :id: feat_req__someip_gateway__sd_find_service
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_83, stkh_req__extspec__someipsd_90,
                  stkh_req__extspec__someipsd_91, stkh_req__extspec__someipsd_824
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall respond to FindService entries for configured services
   using the response transport and timing behavior selected for the current
   Service Discovery state.

.. feat_req:: SOME/IP-SD Eventgroup Subscription Lifecycle
   :id: feat_req__someip_gateway__sd_eventgroup_subs
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_321, stkh_req__extspec__someipsd_322,
                  stkh_req__extspec__someipsd_332, stkh_req__extspec__someipsd_333,
                  stkh_req__extspec__someipsd_613, stkh_req__extspec__someipsd_614,
                  stkh_req__extspec__someipsd_618, stkh_req__extspec__someipsd_619
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall accept supported eventgroup subscriptions, reject
   unsupported subscriptions, and stop event delivery when a subscription is
   explicitly stopped or expires.

.. feat_req:: SOME/IP-SD Phase Timing
   :id: feat_req__someip_gateway__sd_phase_timing
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_62, stkh_req__extspec__someipsd_63,
                  stkh_req__extspec__someipsd_64, stkh_req__extspec__someipsd_67,
                  stkh_req__extspec__someipsd_73, stkh_req__extspec__someipsd_75,
                  stkh_req__extspec__someipsd_76
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall apply the configured initial-wait, repetition, and main
   phases to active Service Discovery entries.

.. feat_req:: SOME/IP-SD IPv4 Endpoint Options
   :id: feat_req__someip_gateway__sd_endpoint_options
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_127, stkh_req__extspec__someipsd_128,
                  stkh_req__extspec__someipsd_129, stkh_req__extspec__someipsd_681,
                  stkh_req__extspec__someipsd_849, stkh_req__extspec__someipsd_850
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall encode configured IPv4 service endpoints in OfferService
   entries and use those endpoints consistently for the offered service.

.. feat_req:: SOME/IP-SD Reboot Handling
   :id: feat_req__someip_gateway__sd_reboot_handling
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_41, stkh_req__extspec__someipsd_765,
                  stkh_req__extspec__someipsd_863, stkh_req__extspec__someipsd_871
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall maintain Service Discovery session and reboot state so
   that peers can detect its restart and the gateway can recognize peer
   restarts.

.. feat_req:: SOME/IP-SD Multicast Eventgroups
   :id: feat_req__someip_gateway__sd_multicast_eg
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someipsd_723, stkh_req__extspec__someipsd_724,
                  stkh_req__extspec__someipsd_763, stkh_req__extspec__someipsd_854
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall acknowledge configured multicast eventgroup subscriptions
   with the corresponding multicast endpoint information.

SOME/IP Messages and Events
---------------------------

.. feat_req:: SOME/IP Response Messages
   :id: feat_req__someip_gateway__response_messages
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someip_79, stkh_req__extspec__someip_83,
                  stkh_req__extspec__someip_90, stkh_req__extspec__someip_95,
                  stkh_req__extspec__someip_141
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall construct SOME/IP response messages with the protocol,
   message, and request identifiers required to correlate each response with
   its request.

.. feat_req:: SOME/IP Error Return Codes
   :id: feat_req__someip_gateway__error_return_codes
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someip_144, stkh_req__extspec__someip_371,
                  stkh_req__extspec__someip_816
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall encode configured SOME/IP return codes in response
   messages; optional protocol error codes shall only be claimed when enabled
   by the gateway configuration.

.. feat_req:: SOME/IP Event Notification Delivery
   :id: feat_req__someip_gateway__event_delivery
   :status: valid
   :version: 1
   :derived_from: stkh_req__someip_gw__interoperability,
                  stkh_req__extspec__someip_361, stkh_req__extspec__someip_807,
                  stkh_req__extspec__someipsd_1297
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.1.0

   The gateway shall deliver SOME/IP event notifications according to active
   eventgroup subscriptions and stop delivery when those subscriptions end.
