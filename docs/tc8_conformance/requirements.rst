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

TC8 Conformance Test Requirements
=================================

Feature Requirement
-------------------

The following feature requirement establishes TC8 SOME/IP conformance testing
as a formal verification activity for the SOME/IP Gateway's protocol stack.

.. feat_req:: TC8 SOME/IP Protocol Conformance
   :id: feat_req__tc8_conformance__conformance
   :status: valid
   :version: 1
   :tags: tc8, conformance, someip, verification
   :derived_from: stkh_req__docgen_enabled__example
   :satisfied_by: feat__someip_gateway
   :safety: QM
   :security: NO
   :reqtype: Functional
   :valid_from: v0.0.0

   The SOME/IP Gateway project shall verify protocol conformance of its
   SOME/IP stack (``someipd``) against OPEN Alliance TC8 SOME/IP test
   specifications at the wire protocol level, without requiring application
   processes.

   Note: The ``:derived_from:`` link targets a placeholder stakeholder requirement.
   This shall be updated to reference the upstream S-CORE stakeholder requirement
   for SOME/IP interoperability once it is formally defined.

Component Requirements — Service Discovery
------------------------------------------

The following component requirements define the high-priority TC8 conformance
tests for SOME/IP Service Discovery (SD), aligned with SOME/IP-SD Protocol
Specification (AUTOSAR PRS_SOMEIP_SD).

.. comp_req:: TC8 SD Offer Entry Format Validation
   :id: comp_req__tc8_conformance__sd_offer_format
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery
   :derived_from: feat_req__someip_gateway__sd_offer
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` transmits
   SOME/IP-SD OfferService entries on the configured multicast group
   with correct service ID, instance ID, major/minor version, and TTL
   fields upon startup.

   Note: Traces to SOME/IP-SD specification sections 4.1.2.1
   (OfferService entry format) and 4.1.2.3 (Service Entry fields).
   Covers TC8-SD-001 and TC8-SD-002 from the test strategy.

.. comp_req:: TC8 SD Cyclic Offer Timing
   :id: comp_req__tc8_conformance__sd_cyclic_timing
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, timing
   :derived_from: feat_req__someip_gateway__sd_cyclic_offer
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` repeats
   OfferService entries at the configured ``cyclic_offer_delay`` interval
   (±20% tolerance) during the main phase of Service Discovery.

   Note: Traces to SOME/IP-SD specification section 4.1.1
   (SD Phases — Main Phase, cyclic offer behavior).
   Covers TC8-SD-003 from the test strategy.

.. comp_req:: TC8 SD Multicast Response to FindService
   :id: comp_req__tc8_conformance__sd_find_mcast_rsp
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, timing
   :derived_from: feat_req__someip_gateway__sd_find_service
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` answers a
   FindService with a multicast OfferService for the specified timing and
   Unicast Flag conditions.

   Note: Traces to OA TC8 SOMEIPSRV_SD_BEHAVIOR_03 and
   SOMEIPSRV_SD_BEHAVIOR_04; see TC8-SDM-013 and TC8-SDM-014.

.. comp_req:: TC8 SD FindService Response
   :id: comp_req__tc8_conformance__sd_find_response
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery
   :derived_from: feat_req__someip_gateway__sd_find_service
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` responds to
   a SOME/IP-SD FindService entry for a known service with a unicast
   OfferService. The unknown-service no-response case is not currently
   covered by a linked test.

   Note: Traces to SOME/IP-SD specification section 4.1.2.2
   (FindService entry handling and response behavior).
   Covers known-service response behavior. TC8-SD-005 (unknown service)
   remains an unverified case.

.. comp_req:: TC8 SD Subscribe Eventgroup Lifecycle
   :id: comp_req__tc8_conformance__sd_sub_lifecycle
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, eventgroup
   :derived_from: feat_req__someip_gateway__sd_eventgroup_lifecycle
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` correctly
   handles the SubscribeEventgroup lifecycle: acknowledge valid
   subscriptions (SubscribeEventgroupAck), reject unknown eventgroups
   (SubscribeEventgroupNack with TTL=0), and honor StopSubscribeEventgroup
   by ceasing notifications.

   Note: Traces to SOME/IP-SD specification sections 4.1.2.4
   (SubscribeEventgroup), 4.1.2.5 (StopSubscribeEventgroup),
   and 4.1.2.6 (SubscribeEventgroupAck/Nack).
   Covers TC8-SD-006, TC8-SD-007, and TC8-SD-008 from the test strategy.

.. comp_req:: TC8 SD Subscription TTL Expiry
   :id: comp_req__tc8_conformance__sd_ttl_expiry
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, eventgroup, timing
   :derived_from: feat_req__someip_gateway__sd_eventgroup_lifecycle
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that event notifications cease
   after a subscription expires without renewal, conforming to
   OA TC8 SOMEIP_ETS_095.

.. comp_req:: TC8 SD Initial Delay and Repetitions Phase
   :id: comp_req__tc8_conformance__sd_phases_timing
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, timing
   :derived_from: feat_req__someip_gateway__sd_phase_timing
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify the configured repetition-phase
   OfferService intervals. Initial-wait bounds, repetition count, and the
   transition to main phase are not currently covered by a linked test.

   Note: Traces to SOME/IP-SD specification section 4.1.1
   (SD Phases — Initial Wait, Repetition, Main Phase).
   Partially covers TC8-SD-009; TC8-SD-010 remains unverified.

.. comp_req:: TC8 SD IPv4 Endpoint Option Validation
   :id: comp_req__tc8_conformance__sd_endpoint_option
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery
   :derived_from: feat_req__someip_gateway__sd_endpoint_options
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` includes a valid
   IPv4EndpointOption in OfferService SD entries, carrying the correct
   unicast address, port, and L4 protocol so clients can reach the offered
   service.

   Note: Traces to AUTOSAR SOME/IP-SD Protocol Specification R24-11,
   section 5.1.2.4.3 (IPv4 Endpoint Option); see TC8-SD-011.

.. comp_req:: TC8 SD Reboot Detection
   :id: comp_req__tc8_conformance__sd_reboot
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, reboot
   :derived_from: feat_req__someip_gateway__sd_reboot_handling
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` sets the reboot
   flag and resets its SD session ID after restart, and detects a peer reboot
   through the reboot flag and a lower session ID.

   Note: Traces to SOME/IP-SD Header reboot flag and Session ID handling;
   see TC8-SDLC-015 and TC8-SDLC-016.

.. comp_req:: TC8 SD Multicast Eventgroup Option
   :id: comp_req__tc8_conformance__sd_mcast_eg
   :status: valid
   :version: 1
   :tags: tc8, conformance, service_discovery, multicast
   :derived_from: feat_req__someip_gateway__sd_multicast_eventgroup
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` includes an
   IPv4 Multicast Option in the SubscribeEventgroupAck for eventgroups
   configured with a multicast address.

   Note: Traces to AUTOSAR SOME/IP-SD Protocol Specification R24-11,
   section 5.1.3.1.3 (Subscribe Eventgroup Acknowledgement); see TC8-SD-013.

Component Requirements — SOME/IP Message Format
-----------------------------------------------

.. comp_req:: TC8 SOME/IP Response Header Validation
   :id: comp_req__tc8_conformance__msg_resp_header
   :status: valid
   :version: 1
   :tags: tc8, conformance, message_format
   :derived_from: feat_req__someip_gateway__response_messages
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` returns
   SOME/IP RESPONSE messages with correct protocol version (0x01),
   message type (0x80), matching session ID, and matching client ID
   for each received REQUEST.

   Note: Traces to SOME/IP specification sections 4.1.4
   (Protocol Version), 4.1.6 (Message Type), and 4.1.3 (Request ID —
   Client ID / Session ID). Covers TC8-SOMEIP-MSG-001,
   TC8-SOMEIP-MSG-002, TC8-SOMEIP-MSG-005, and TC8-SOMEIP-MSG-008
   from the test strategy.

.. comp_req:: TC8 SOME/IP Error Return Codes
   :id: comp_req__tc8_conformance__msg_error_codes
   :status: valid
   :version: 1
   :tags: tc8, conformance, message_format, error_handling
   :derived_from: feat_req__someip_gateway__error_return_codes
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` returns
   the correct SOME/IP return codes for error conditions:
   ``E_UNKNOWN_SERVICE`` (0x02) for requests to non-existent services,
   ``E_UNKNOWN_METHOD`` (0x03) for invalid method IDs, and
   ``E_WRONG_INTERFACE_VERSION`` for interface version mismatches.

   Note: Traces to SOME/IP specification section 4.1.7 (Return Code)
   and the return code table (Table 4.14). Covers TC8-SOMEIP-MSG-003,
   TC8-SOMEIP-MSG-004, and TC8-SOMEIP-MSG-006 from the test strategy.

Component Requirements — Event Notification
-------------------------------------------

.. comp_req:: TC8 Event Notification Subscription Lifecycle
   :id: comp_req__tc8_conformance__evt_subscription
   :status: valid
   :version: 1
   :tags: tc8, conformance, events, notification
   :derived_from: feat_req__someip_gateway__event_notification_delivery
   :satisfied_by: comp__someipd
   :safety: QM
   :security: NO
   :reqtype: Functional

   The conformance test suite shall verify that ``someipd`` delivers
   NOTIFICATION messages (message type 0x02) with correct event ID
   only to endpoints with an active eventgroup subscription, and ceases
   delivery after StopSubscribeEventgroup.

   Note: Traces to SOME/IP specification section 5.1 (Events) and
   SOME/IP-SD section 4.1.2.4 (SubscribeEventgroup triggering
   notification delivery). Covers TC8-EVT-001 through TC8-EVT-004
   and TC8-EVT-006 from the test strategy.

Traceability Summary
--------------------

The table records whether testcase decorators currently link to each
component requirement. It does not report test execution results; those are
shown in ``traceability.rst`` when a test run is available.

* ``comp_req__tc8_conformance__sd_offer_format`` derives from ``sd_offer`` — linked.
* ``comp_req__tc8_conformance__sd_cyclic_timing`` derives from ``sd_cyclic_offer`` — linked.
* ``comp_req__tc8_conformance__sd_find_mcast_rsp`` derives from ``sd_find_service`` — partial; one linked case is expected-failing.
* ``comp_req__tc8_conformance__sd_find_response`` derives from ``sd_find_service`` — linked; unknown-service case not tested.
* ``comp_req__tc8_conformance__sd_sub_lifecycle`` derives from ``sd_eventgroup_lifecycle`` — linked.
* ``comp_req__tc8_conformance__sd_ttl_expiry`` derives from ``sd_eventgroup_lifecycle`` — partially linked.
* ``comp_req__tc8_conformance__sd_phases_timing`` derives from ``sd_phase_timing`` — partial; initial-wait and main-phase cases missing.
* ``comp_req__tc8_conformance__sd_endpoint_option`` derives from ``sd_endpoint_options`` — linked.
* ``comp_req__tc8_conformance__sd_mcast_eg`` derives from ``sd_multicast_eventgroup`` — linked; skipped on loopback environments.
* ``comp_req__tc8_conformance__sd_reboot`` derives from ``sd_reboot_handling`` — partially linked.
* ``comp_req__tc8_conformance__msg_resp_header`` derives from ``response_messages`` — no testcase decorator link.
* ``comp_req__tc8_conformance__msg_error_codes`` derives from ``error_return_codes`` — no testcase decorator link.
* ``comp_req__tc8_conformance__evt_subscription`` derives from ``event_notification_delivery`` — no testcase decorator link.
