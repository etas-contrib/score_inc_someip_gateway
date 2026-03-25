<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# ARXML Configuration Parameters — Complete JSON Schema

This document provides a comprehensive analysis of **every configuration parameter** that
the `com-aap-communication-manager` toolchain extracts from AUTOSAR Adaptive ARXML files.
The analysis is based on:

* The **gateway** Flatbuffers schema (`com_flatcfg.fbs` — consumed by `someip_domain_gateway`)
* The **client** Flatbuffers schema (`comclient_flatcfg.fbs` — consumed by each user process)
* The C++ config-reader structs (`ServiceDefinitionConfig.hpp`, `ConfigSecOcTypes.hpp`)
* The FreeMarker generator templates (`HelperMacros.ftl`, `ServiceSerializer_h.ftl`, `ServiceMapping_h.ftl`)
* Real test-data JSON files from the `tests/studio-dsl` directory

The goal is to define a **single, human-writable JSON schema** that could replace the ARXML
tooling pipeline for our `inc_someip_gateway` use case while retaining every degree of freedom
that ARXML provides.

---

## 1. High-Level Structure

The ARXML model splits configuration into two distinct outputs, each with its own
Flatbuffers schema. The table below maps these to the consumers:

| Output file | FBS namespace | Consumer binary | Purpose |
|---|---|---|---|
| `com_<proc>__SWCL_flatcfg.bin` | `COMFlatBuffer` | `someip_domain_gateway` | Service routing: which service instances exist, their SOME/IP IDs, events, methods, fields, SecOC & S2S signal mappings |
| `comclient_<proc>__SWCL_flatcfg.bin` | `COMCLIENTFlatBuffer` | Each user application | Full deployment per process: SOME/IP, IPC, and DDS bindings, E2E protection, instance specifiers, thread pool size |

A unified JSON schema must cover **both** outputs.

---

## 2. Complete JSON Schema

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "AUTOSAR Adaptive COM Configuration",
  "description": "Unified JSON schema covering all ARXML-derived configuration parameters for the com-aap-communication-manager (gateway + client configs).",
  "type": "object",
  "properties": {
    "functionCluster": {
      "type": "string",
      "description": "Identifies the function cluster. 'COM' for gateway, 'COMCLIENT' for user processes.",
      "enum": ["COM", "COMCLIENT"]
    },
    "versionMajor": {
      "type": "integer",
      "description": "Major version of the configuration schema."
    },
    "versionMinor": {
      "type": "integer",
      "description": "Minor version of the configuration schema."
    },
    "threadPoolSize": {
      "type": "integer",
      "minimum": 0,
      "maximum": 255,
      "default": 0,
      "description": "COMCLIENT only. Number of pre-allocated connection listener threads for the process (connectionThreadpoolSize in ARXML). 0 = dynamic allocation."
    },

    "serviceInterfaces": {
      "type": "array",
      "description": "Abstract service interface definitions (design-level, no deployment IDs).",
      "items": { "$ref": "#/definitions/ServiceInterface" }
    },

    "someipDeployments": {
      "type": "array",
      "description": "SOME/IP service interface deployments: maps a service interface to SOME/IP IDs.",
      "items": { "$ref": "#/definitions/SomeipDeployment" }
    },
    "ipcDeployments": {
      "type": "array",
      "description": "IPC (local) service interface deployments.",
      "items": { "$ref": "#/definitions/IpcDeployment" }
    },
    "ddsDeployments": {
      "type": "array",
      "description": "DDS service interface deployments.",
      "items": { "$ref": "#/definitions/DdsDeployment" }
    },

    "providedSomeipInstances": {
      "type": "array",
      "description": "Provided (skeleton) SOME/IP service instances.",
      "items": { "$ref": "#/definitions/ProvidedSomeipInstance" }
    },
    "requiredSomeipInstances": {
      "type": "array",
      "description": "Required (proxy) SOME/IP service instances.",
      "items": { "$ref": "#/definitions/RequiredSomeipInstance" }
    },
    "providedIpcInstances": {
      "type": "array",
      "items": { "$ref": "#/definitions/ProvidedIpcInstance" }
    },
    "requiredIpcInstances": {
      "type": "array",
      "items": { "$ref": "#/definitions/RequiredIpcInstance" }
    },
    "providedDdsInstances": {
      "type": "array",
      "items": { "$ref": "#/definitions/ProvidedDdsInstance" }
    },
    "requiredDdsInstances": {
      "type": "array",
      "items": { "$ref": "#/definitions/RequiredDdsInstance" }
    },

    "e2eProfileConfigs": {
      "type": "array",
      "description": "Shared E2E profile configuration objects, referenced by protection props.",
      "items": { "$ref": "#/definitions/E2EProfileConfig" }
    },

    "instanceSpecifiers": {
      "type": "array",
      "description": "COMCLIENT only. Maps ara::core::InstanceSpecifier names to numeric IDs.",
      "items": { "$ref": "#/definitions/InstanceSpecifier" }
    },

    "secOcSecureComProps": {
      "type": "array",
      "description": "Gateway only. SecOC Secure Communication Properties per transport protocol.",
      "items": { "$ref": "#/definitions/SecOcSecureComProps" }
    },
    "someipServiceInstanceToMachineMappings": {
      "type": "array",
      "description": "Gateway only. Maps service instances to machines and assigns per-protocol SecOC.",
      "items": { "$ref": "#/definitions/SomeipServiceInstanceToMachineMapping" }
    },

    "iSignalTriggerings": {
      "type": "array",
      "description": "I-Signal triggering definitions for Signal-to-Service (S2S) translation.",
      "items": { "$ref": "#/definitions/ISignalTriggering" }
    },
    "iSignals": {
      "type": "array",
      "description": "COMCLIENT only. Individual I-Signal definitions.",
      "items": { "$ref": "#/definitions/ISignal" }
    },
    "iSignalGroups": {
      "type": "array",
      "description": "COMCLIENT only. Groups of I-Signals for S2S with optional E2E protection.",
      "items": { "$ref": "#/definitions/ISignalGroup" }
    },
    "iSignalIPdus": {
      "type": "array",
      "description": "COMCLIENT only. I-Signal I-PDU container definitions.",
      "items": { "$ref": "#/definitions/ISignalIPdu" }
    },
    "iSignalIPduMappings": {
      "type": "array",
      "description": "COMCLIENT only. Byte-level packing of I-Signals into I-PDUs.",
      "items": { "$ref": "#/definitions/ISignalIPduMapping" }
    },

    "pduTriggerings": {
      "type": "array",
      "description": "Gateway only. PDU triggering containers referencing I-Signal triggerings.",
      "items": { "$ref": "#/definitions/PduTriggering" }
    },
    "securedIPdus": {
      "type": "array",
      "description": "Gateway only. Secured I-PDU definitions for S2S SecOC protection.",
      "items": { "$ref": "#/definitions/SecuredIPdu" }
    },
    "serviceInstanceToSignalMappings": {
      "type": "array",
      "description": "Gateway only. Maps service instance events to I-Signal triggerings for S2S.",
      "items": { "$ref": "#/definitions/ServiceInstanceToSignalMapping" }
    }
  },
  "required": ["functionCluster", "versionMajor", "versionMinor"],

  "definitions": {

    "TransportProtocol": {
      "type": "string",
      "enum": ["Udp", "Tcp"],
      "description": "SOME/IP transport layer protocol."
    },

    "ByteOrder": {
      "type": "string",
      "enum": ["MostSignificantByteFirst", "MostSignificantByteLast", "Opaque"],
      "default": "MostSignificantByteFirst",
      "description": "Byte order for serialization (ARXML: ApSomeipTransformationProps.byteOrder)."
    },

    "DataIdMode": {
      "type": "string",
      "enum": ["All16Bit", "Alternating8Bit", "Lower8Bit", "Lower12Bit"],
      "description": "E2E data ID interpretation mode."
    },

    "DeploymentType": {
      "type": "string",
      "enum": ["IPC", "SOMEIP", "DDS"],
      "description": "Binding type for the service deployment."
    },

    "ServiceInterface": {
      "type": "object",
      "description": "Design-level service interface (no deployment-specific IDs).",
      "properties": {
        "name": { "type": "string", "description": "Fully qualified interface name (e.g. 'com.example.MyService')." },
        "fields": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "name": { "type": "string" },
              "hasGetter": { "type": "boolean", "default": false },
              "hasSetter": { "type": "boolean", "default": false },
              "hasNotifier": { "type": "boolean", "default": false }
            },
            "required": ["name"]
          }
        },
        "events": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "name": { "type": "string" },
              "shortName": { "type": "string" }
            },
            "required": ["name"]
          }
        }
      },
      "required": ["name"]
    },

    "SomeipEvent": {
      "type": "object",
      "description": "SOME/IP event deployment parameters.",
      "properties": {
        "name": { "type": "string", "description": "Event short name." },
        "id": { "type": "integer", "minimum": 0, "maximum": 65535, "description": "SOME/IP Event ID." },
        "maxSerializationSize": { "type": "integer", "default": 10240, "description": "Maximum serialized payload size in bytes (from @size annotation or ARXML SomeipEventDeployment). Default 10 kB." },
        "maxPublisherSlots": { "type": "integer", "default": 20, "description": "Maximum number of sample slots the publisher can hold (from @maxslotsperpublisher)." },
        "maxSubscriberSlots": { "type": "integer", "default": 10, "description": "Maximum maxSampleCount a subscriber may request (from @maxslotspersubscriber)." },
        "transportProtocol": { "$ref": "#/definitions/TransportProtocol" },
        "segmentLength": { "type": "integer", "default": 1404, "description": "SOME/IP-TP maximum segment payload length in bytes." },
        "sessionHandling": { "type": "boolean", "default": true, "description": "Enable SOME/IP session ID handling for this event." },
        "signalBased": { "type": "boolean", "default": false, "description": "True if this event uses Signal-to-Service (S2S) translation." }
      },
      "required": ["name", "id"]
    },

    "SomeipMethod": {
      "type": "object",
      "description": "SOME/IP method deployment parameters.",
      "properties": {
        "name": { "type": "string", "description": "Method short name." },
        "id": { "type": "integer", "minimum": 0, "maximum": 65535, "description": "SOME/IP Method ID." },
        "requestMaxSerializationSize": { "type": "integer", "default": 10240, "description": "Maximum serialized size of the request payload." },
        "responseMaxSerializationSize": { "type": "integer", "default": 10240, "description": "Maximum serialized size of the response payload. 0 for fire-and-forget." },
        "maxSlots": { "type": "integer", "default": 20, "description": "Maximum concurrent in-flight method call slots." },
        "transportProtocol": { "$ref": "#/definitions/TransportProtocol" }
      },
      "required": ["name", "id"]
    },

    "SomeipField": {
      "type": "object",
      "description": "SOME/IP field deployment — a composite of notifier (event), getter (method), and setter (method).",
      "properties": {
        "name": { "type": "string" },
        "hasNotifier": { "type": "boolean", "default": false },
        "hasGetter": { "type": "boolean", "default": false },
        "hasSetter": { "type": "boolean", "default": false },
        "notifier": { "$ref": "#/definitions/SomeipEvent", "description": "Notifier (event) sub-deployment. Only present if hasNotifier=true." },
        "setter": { "$ref": "#/definitions/SomeipMethod", "description": "Setter (method) sub-deployment. Only present if hasSetter=true." },
        "getter": { "$ref": "#/definitions/SomeipMethod", "description": "Getter (method) sub-deployment. Only present if hasGetter=true." }
      },
      "required": ["name"]
    },

    "IpcEvent": {
      "type": "object",
      "description": "IPC event deployment (local-only, no transport protocol).",
      "properties": {
        "name": { "type": "string" },
        "id": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "maxSerializationSize": { "type": "integer", "default": 10240 },
        "maxPublisherSlots": { "type": "integer", "default": 20 },
        "maxSubscriberSlots": { "type": "integer", "default": 10 }
      },
      "required": ["name", "id"]
    },

    "IpcMethod": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "id": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "requestMaxSerializationSize": { "type": "integer", "default": 10240 },
        "responseMaxSerializationSize": { "type": "integer", "default": 10240 },
        "maxSlots": { "type": "integer", "default": 20 }
      },
      "required": ["name", "id"]
    },

    "IpcField": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "hasNotifier": { "type": "boolean" },
        "hasGetter": { "type": "boolean" },
        "hasSetter": { "type": "boolean" },
        "notifier": { "$ref": "#/definitions/IpcEvent" },
        "setter": { "$ref": "#/definitions/IpcMethod" },
        "getter": { "$ref": "#/definitions/IpcMethod" }
      },
      "required": ["name"]
    },

    "DdsEvent": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "topicName": { "type": "string" },
        "transportProtocol": { "type": "string" },
        "eventTopicAccessRule": { "$ref": "#/definitions/DdsTopicAccessRule" }
      },
      "required": ["name", "topicName"]
    },

    "DdsMethod": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "id": { "type": "integer" },
        "requestMaxSerializationSize": { "type": "integer" },
        "responseMaxSerializationSize": { "type": "integer" },
        "maxSlots": { "type": "integer" }
      },
      "required": ["name", "id"]
    },

    "DdsField": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "hasNotifier": { "type": "boolean" },
        "hasGetter": { "type": "boolean" },
        "hasSetter": { "type": "boolean" },
        "notifier": { "$ref": "#/definitions/DdsEvent" },
        "setter": { "$ref": "#/definitions/DdsMethod" },
        "getter": { "$ref": "#/definitions/DdsMethod" }
      }
    },

    "DdsTopicAccessRule": {
      "type": "object",
      "description": "DDS Security: per-topic access control rules.",
      "properties": {
        "enableDiscoveryProtection": { "type": "boolean", "default": false },
        "enableLivelinessProtection": { "type": "boolean", "default": false },
        "enableReadAccessControl": { "type": "boolean", "default": false },
        "enableWriteAccessControl": { "type": "boolean", "default": false },
        "dataProtectionKind": {
          "type": "string",
          "enum": ["None", "Sign", "EncryptAndSign", "SignWithOriginAuthentication", "EncryptAndSignWithOriginAuthentication"],
          "default": "None"
        },
        "metadataProtectionKind": {
          "type": "string",
          "enum": ["None", "Sign", "EncryptAndSign", "SignWithOriginAuthentication", "EncryptAndSignWithOriginAuthentication"],
          "default": "None"
        }
      }
    },

    "SomeipDeployment": {
      "type": "object",
      "description": "Complete SOME/IP deployment for one service interface.",
      "properties": {
        "serviceInterfaceName": { "type": "string", "description": "Fully qualified service interface name." },
        "serviceId": { "type": "integer", "minimum": 0, "maximum": 65535, "description": "SOME/IP Service ID (16-bit)." },
        "majorVersion": { "type": "integer", "minimum": 0, "maximum": 255, "description": "Major version (8-bit)." },
        "minorVersion": { "type": "integer", "minimum": 0, "description": "Minor version (32-bit). 0xFFFFFFFE = any version." },
        "events": { "type": "array", "items": { "$ref": "#/definitions/SomeipEvent" } },
        "methods": { "type": "array", "items": { "$ref": "#/definitions/SomeipMethod" } },
        "fields": { "type": "array", "items": { "$ref": "#/definitions/SomeipField" } }
      },
      "required": ["serviceInterfaceName", "serviceId", "majorVersion", "minorVersion"]
    },

    "IpcDeployment": {
      "type": "object",
      "description": "IPC deployment for one service interface.",
      "properties": {
        "serviceInterfaceName": { "type": "string" },
        "serviceId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "majorVersion": { "type": "integer", "minimum": 0, "maximum": 255 },
        "minorVersion": { "type": "integer", "minimum": 0 },
        "events": { "type": "array", "items": { "$ref": "#/definitions/IpcEvent" } },
        "methods": { "type": "array", "items": { "$ref": "#/definitions/IpcMethod" } },
        "fields": { "type": "array", "items": { "$ref": "#/definitions/IpcField" } }
      },
      "required": ["serviceInterfaceName", "serviceId", "majorVersion", "minorVersion"]
    },

    "DdsDeployment": {
      "type": "object",
      "description": "DDS deployment for one service interface.",
      "properties": {
        "serviceInterfaceName": { "type": "string" },
        "serviceId": { "type": "integer" },
        "majorVersion": { "type": "integer" },
        "minorVersion": { "type": "integer" },
        "fieldReplyTopicName": { "type": "string" },
        "fieldRequestTopicName": { "type": "string" },
        "methodReplyTopicName": { "type": "string" },
        "methodRequestTopicName": { "type": "string" },
        "transportProtocol": { "type": "string" },
        "fieldTopicsAccessRule": { "$ref": "#/definitions/DdsTopicAccessRule" },
        "methodTopicsAccessRule": { "$ref": "#/definitions/DdsTopicAccessRule" },
        "events": { "type": "array", "items": { "$ref": "#/definitions/DdsEvent" } },
        "methods": { "type": "array", "items": { "$ref": "#/definitions/DdsMethod" } },
        "fields": { "type": "array", "items": { "$ref": "#/definitions/DdsField" } }
      },
      "required": ["serviceInterfaceName", "serviceId"]
    },

    "ProvidedSomeipInstance": {
      "type": "object",
      "description": "A provided (skeleton) SOME/IP service instance.",
      "properties": {
        "name": { "type": "string", "description": "Instance name (used as ara::core::InstanceSpecifier)." },
        "instanceId": { "type": "integer", "minimum": 0, "description": "SOME/IP Instance ID (32-bit in config, 16-bit on wire)." },
        "deployment": { "type": "string", "description": "Reference to a someipDeployments entry (e.g. by name or index)." },
        "eventProtectionProps": { "type": "array", "items": { "$ref": "#/definitions/E2EProtectionProps" } },
        "methodProtectionProps": { "type": "array", "items": { "$ref": "#/definitions/E2EProtectionProps" } },
        "signalTriggeringToEventMap": { "type": "array", "items": { "$ref": "#/definitions/SignalTriggeringToEventMap" } }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "RequiredSomeipInstance": {
      "type": "object",
      "description": "A required (proxy) SOME/IP service instance.",
      "properties": {
        "name": { "type": "string" },
        "instanceId": { "type": "integer", "minimum": 0 },
        "requiredMinVersion": { "type": "integer", "description": "Minimum required minor version. 0xFFFFFFFF = any version." },
        "minimumMinorVersionFlag": { "type": "boolean", "default": false, "description": "True if requiredMinVersion is a minimum (>=), false if exact match." },
        "deployment": { "type": "string" },
        "blacklistVersions": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "minorVersion": { "type": "integer" }
            }
          },
          "description": "List of minor versions to reject."
        },
        "eventProtectionProps": { "type": "array", "items": { "$ref": "#/definitions/E2EProtectionProps" } },
        "methodProtectionProps": { "type": "array", "items": { "$ref": "#/definitions/E2EProtectionProps" } },
        "signalTriggeringToEventMap": { "type": "array", "items": { "$ref": "#/definitions/SignalTriggeringToEventMap" } }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "ProvidedIpcInstance": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "instanceId": { "type": "integer" },
        "deployment": { "type": "string" }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "RequiredIpcInstance": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "instanceId": { "type": "integer" },
        "requiredMinVersion": { "type": "integer" },
        "minimumMinorVersionFlag": { "type": "boolean" },
        "deployment": { "type": "string" }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "ProvidedDdsInstance": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "instanceId": { "type": "integer" },
        "domainId": { "type": "integer", "description": "DDS Domain ID." },
        "qosProfile": { "type": "string", "description": "DDS QoS profile name." },
        "discoveryType": {
          "type": "string",
          "enum": ["DomainParticipantUserDataQos", "Topic"]
        },
        "resourceIdentifierType": {
          "type": "string",
          "enum": ["Partition", "TopicPrefix", "InstanceId"]
        },
        "deployment": { "type": "string" }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "RequiredDdsInstance": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "instanceId": { "type": "integer" },
        "requiredMinVersion": { "type": "integer" },
        "minimumMinorVersionFlag": { "type": "boolean" },
        "domainId": { "type": "integer" },
        "qosProfile": { "type": "string" },
        "discoveryType": {
          "type": "string",
          "enum": ["DomainParticipantUserDataQos", "Topic"]
        },
        "deployment": { "type": "string" },
        "blocklistVersions": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": { "minorVersion": { "type": "integer" } }
          }
        }
      },
      "required": ["name", "instanceId", "deployment"]
    },

    "E2EProtectionProps": {
      "type": "object",
      "description": "End-to-End protection properties for a specific event or method.",
      "properties": {
        "dataIds": { "type": "array", "items": { "type": "integer" }, "description": "E2E data ID list." },
        "minDataLength": { "type": "integer", "description": "Minimum expected protected data length." },
        "maxDataLength": { "type": "integer", "description": "Maximum expected protected data length." },
        "dataLength": { "type": "integer", "description": "Exact protected data length (for S2S profiles)." },
        "sourceId": { "type": "integer", "description": "E2E source identifier." },
        "componentId": { "type": "integer", "minimum": 0, "maximum": 65535, "description": "Unique component ID for E2E state machine." },
        "profileConfig": { "type": "string", "description": "Reference to an E2EProfileConfig entry." }
      },
      "required": ["dataIds", "profileConfig"]
    },

    "E2EProfileConfig": {
      "type": "object",
      "description": "E2E profile configuration (shared across multiple protection props).",
      "properties": {
        "profileName": {
          "type": "string",
          "description": "AUTOSAR E2E profile name.",
          "enum": ["PROFILE_01", "PROFILE_02", "PROFILE_04", "PROFILE_05", "PROFILE_06", "PROFILE_07", "PROFILE_11", "PROFILE_22", "PROFILE_4m"]
        },
        "clearFromValidToInvalid": { "type": "boolean", "default": false, "description": "Reset state machine on transition from valid to invalid." },
        "maxDeltaCounter": { "type": "integer", "description": "Maximum allowed gap in the E2E counter." },
        "dataIdMode": { "$ref": "#/definitions/DataIdMode" },
        "maxErrorStateInit": { "type": "integer", "minimum": 0, "maximum": 255 },
        "maxErrorStateInvalid": { "type": "integer", "minimum": 0, "maximum": 255 },
        "maxErrorStateValid": { "type": "integer", "minimum": 0, "maximum": 255 },
        "minOkStateInit": { "type": "integer", "minimum": 0, "maximum": 255 },
        "minOkStateInvalid": { "type": "integer", "minimum": 0, "maximum": 255 },
        "minOkStateValid": { "type": "integer", "minimum": 0, "maximum": 255 },
        "windowSizeInit": { "type": "integer", "minimum": 0, "maximum": 255 },
        "windowSizeInvalid": { "type": "integer", "minimum": 0, "maximum": 255 },
        "windowSizeValid": { "type": "integer", "minimum": 0, "maximum": 255 }
      },
      "required": ["profileName"]
    },

    "InstanceSpecifier": {
      "type": "object",
      "description": "Maps an ara::core::InstanceSpecifier string to numeric service/instance IDs.",
      "properties": {
        "name": { "type": "string", "description": "Instance specifier string (e.g. '/MyApp/RPort_MyService')." },
        "instanceId": { "type": "integer" },
        "serviceId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "deploymentType": { "$ref": "#/definitions/DeploymentType" }
      },
      "required": ["name", "instanceId", "serviceId", "deploymentType"]
    },

    "SecOcSecureComProps": {
      "type": "object",
      "description": "SecOC Secure Communication Properties — per transport protocol.",
      "properties": {
        "authAlgorithm": { "type": "string", "description": "Authentication algorithm identifier." },
        "authInfoTxLength": { "type": "integer", "description": "Length in bits of the authentication code to transmit." },
        "freshnessValueLength": { "type": "integer", "description": "Complete length in bits of the Freshness Value." },
        "freshnessValueTxLength": { "type": "integer", "description": "Length in bits of the Freshness Value included in payload." },
        "authenticationBuildAttempts": { "type": "integer", "default": 1, "description": "Number of times to attempt MAC creation." },
        "authenticationRetries": { "type": "integer", "default": 1, "description": "Number of times to attempt MAC verification." }
      }
    },

    "ElementSecureComConfig": {
      "type": "object",
      "description": "Per-element (event/method/field) SecOC configuration within a service instance.",
      "properties": {
        "freshnessValueId": { "type": "integer" },
        "dataId": { "type": "integer" },
        "eventId": { "type": "integer", "minimum": 0, "maximum": 65535, "description": "Applies SecOC to event with this ID." },
        "fieldNotifierId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "getterCallId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "getterReturnId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "setterCallId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "setterReturnId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "methodCallId": { "type": "integer", "minimum": 0, "maximum": 65535 },
        "methodReturnId": { "type": "integer", "minimum": 0, "maximum": 65535 }
      }
    },

    "ComSecOcToCryptoKeySlotMapping": {
      "type": "object",
      "description": "Gateway only. Maps a crypto key slot specifier to a SecOC secure com props entry.",
      "properties": {
        "cryptoKeySlotSpecifier": { "type": "string", "description": "Reference to a crypto key slot." },
        "secureComProps": { "type": "string", "description": "Reference to a SecOcSecureComProps entry." }
      }
    },

    "SomeipServiceInstanceToMachineMapping": {
      "type": "object",
      "description": "Gateway only. Associates service instances with a machine and optional per-protocol SecOC.",
      "properties": {
        "secureTcp": { "$ref": "#/definitions/SecOcSecureComProps", "description": "SecOC props applied to all TCP elements of this mapping." },
        "secureUdp": { "$ref": "#/definitions/SecOcSecureComProps", "description": "SecOC props applied to all UDP elements of this mapping." },
        "serviceInstances": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "serviceInstanceRef": { "type": "string", "description": "Reference to a SomeipServiceInstance." }
            }
          }
        }
      }
    },

    "SignalTriggeringToEventMap": {
      "type": "object",
      "description": "Maps an S2S I-Signal triggering to a service event for Signal-to-Service translation.",
      "properties": {
        "eventName": { "type": "string" },
        "iSignalTriggering": { "type": "string", "description": "Reference to an ISignalTriggering." }
      },
      "required": ["eventName", "iSignalTriggering"]
    },

    "ISignalTriggering": {
      "type": "object",
      "description": "I-Signal triggering definition for S2S.",
      "properties": {
        "shortName": { "type": "string" },
        "iSignal": { "type": "string", "description": "Reference to an ISignal." },
        "iSignalGroup": { "type": "string", "description": "Reference to an ISignalGroup." }
      }
    },

    "ISignal": {
      "type": "object",
      "description": "Individual I-Signal definition.",
      "properties": {
        "name": { "type": "string" },
        "type": { "type": "string", "enum": ["Primitive", "Array"], "description": "Signal data type category." },
        "length": { "type": "integer", "description": "Signal length in bits." },
        "iSignalIPduMapping": { "type": "string", "description": "Reference to an ISignalIPduMapping." }
      },
      "required": ["name", "type", "length"]
    },

    "ISignalGroup": {
      "type": "object",
      "description": "Group of I-Signals with optional S2S E2E protection.",
      "properties": {
        "iSignals": {
          "type": "array",
          "items": { "type": "string", "description": "Reference to ISignal entries." }
        },
        "s2sProtectionProps": { "type": "array", "items": { "$ref": "#/definitions/E2EProtectionProps" } }
      }
    },

    "ISignalIPdu": {
      "type": "object",
      "description": "I-Signal I-PDU container.",
      "properties": {
        "length": { "type": "integer", "description": "PDU length in bytes." },
        "unusedBitPattern": { "type": "integer", "minimum": 0, "maximum": 255, "default": 0 }
      }
    },

    "ISignalIPduMapping": {
      "type": "object",
      "description": "Byte-level mapping of an I-Signal into an I-PDU.",
      "properties": {
        "packingByteOrder": { "$ref": "#/definitions/ByteOrder" },
        "startPosition": { "type": "integer", "description": "Bit start position within the I-PDU." },
        "iSignalIPdu": { "type": "string", "description": "Reference to an ISignalIPdu." }
      }
    },

    "PduTriggering": {
      "type": "object",
      "description": "Gateway only. Groups I-Signal triggerings into a PDU context.",
      "properties": {
        "iSignalTriggeringsRefs": {
          "type": "array",
          "items": { "type": "string", "description": "Reference to ISignalTriggering entries." }
        }
      }
    },

    "SecuredIPdu": {
      "type": "object",
      "description": "Gateway only. Secured I-PDU for S2S SecOC protection.",
      "properties": {
        "authInfoTxLength": { "type": "integer" },
        "freshnessValueLength": { "type": "integer" },
        "freshnessValueTxLength": { "type": "integer" },
        "dataId": { "type": "integer" },
        "freshnessValueId": { "type": "integer" },
        "authenticationBuildAttempts": { "type": "integer" },
        "authenticationRetries": { "type": "integer" },
        "payload": { "type": "string", "description": "Reference to a PduTriggering." }
      }
    },

    "ServiceInstanceToSignalMapping": {
      "type": "object",
      "description": "Gateway only. Maps a SOME/IP service instance's events to I-Signal triggerings.",
      "properties": {
        "serviceInstance": { "type": "string", "description": "Reference to a SomeipServiceInstance." },
        "signalBasedMappings": {
          "type": "array",
          "items": {
            "type": "object",
            "properties": {
              "iSignalTriggering": { "type": "string" },
              "event": { "type": "string", "description": "Reference to a ServiceInterfaceEvent." }
            }
          }
        }
      }
    },

    "ApSomeipTransformationProps": {
      "type": "object",
      "description": "Per-component (event/method/field) SOME/IP serialization transformation properties. These are extracted from ARXML ApSomeipTransformationProps and used by the code generator to produce serializer settings.",
      "properties": {
        "byteOrder": { "$ref": "#/definitions/ByteOrder", "default": "MostSignificantByteFirst" },
        "sizeOfArrayLengthField": {
          "type": "integer",
          "enum": [0, 1, 2, 4],
          "default": 4,
          "description": "Size in bytes of the length field prepended to fixed-size arrays. 0 = no length field for fixed arrays, but dynamic arrays (vectors/maps) still use 4 bytes."
        },
        "sizeOfStringLengthField": {
          "type": "integer",
          "enum": [0, 1, 2, 4],
          "default": 4,
          "description": "Size in bytes of the length field prepended to strings. 0 maps to 4."
        },
        "sizeOfStructLengthField": {
          "type": "integer",
          "enum": [0, 1, 2, 4],
          "default": 4,
          "description": "Size in bytes of the length field prepended to structs. 0 = no struct length field."
        },
        "sizeOfUnionLengthField": {
          "type": "integer",
          "enum": [0, 1, 2, 4],
          "default": 4,
          "description": "Size in bytes of the length field for union payloads. 0 maps to 4."
        },
        "sizeOfUnionTypeSelectorField": {
          "type": "integer",
          "enum": [0, 1, 2, 4],
          "default": 1,
          "description": "Size in bytes of the union type selector (discriminator). 0 maps to 1."
        },
        "isDynamicLengthFieldSize": {
          "type": "boolean",
          "default": false,
          "description": "When true, length fields for dynamic containers are sized to fit the actual payload rather than using a fixed width. See TPS_MANI_01186 section 3.18.3.2."
        }
      }
    }
  }
}
```

---

## 3. Parameter Inventory by ARXML Origin

The table below maps every parameter in the schema back to its ARXML source element and
lists which configuration output (gateway, client, or both) it appears in.

### 3.1. Service Interface Design

| Parameter | ARXML Element | Gateway | Client |
|---|---|---|---|
| `serviceInterfaces[].name` | `ServiceInterface.shortName` (namespace path) | ✅ | ✅ |
| `serviceInterfaces[].events[].shortName` | `VariableDataPrototype.shortName` | ✅ | — |
| `serviceInterfaces[].fields[].hasGetter/Setter/Notifier` | `Field.hasGetter`, `Field.hasSetter`, `Field.hasNotifier` | ✅ | ✅ |

### 3.2. SOME/IP Deployment (per service interface)

| Parameter | ARXML Element | Gateway | Client | Default |
|---|---|---|---|---|
| `serviceId` | `SomeipServiceInterfaceDeployment.serviceInterfaceId` | ✅ | ✅ | — |
| `majorVersion` | `.majorVersion` | ✅ | ✅ | — |
| `minorVersion` | `.minorVersion` | ✅ | ✅ | — |
| `events[].id` | `SomeipEventDeployment.eventId` | ✅ | ✅ | — |
| `events[].maxSerializationSize` | `@size` annotation / `AdminData` | ✅ | ✅ | 10240 |
| `events[].maxPublisherSlots` | `@maxslotsperpublisher` annotation | ✅ | ✅ | 20 |
| `events[].maxSubscriberSlots` | `@maxslotspersubscriber` annotation | ✅ | ✅ | 10 |
| `events[].transportProtocol` | `SomeipEventDeployment.transportProtocol` | ✅ | ✅ | — |
| `events[].segmentLength` | `SomeipEventDeployment.maximumSegmentLength` | ✅ | ✅ | 1404 |
| `events[].sessionHandling` | `SomeipEventDeployment.sessionHandling` | — | ✅ | true |
| `events[].signalBased` | Presence of `ServiceInstanceToSignalMapping` | ✅ | ✅ | false |
| `methods[].id` | `SomeipMethodDeployment.methodId` | ✅ | ✅ | — |
| `methods[].requestMaxSerializationSize` | `@size` or computed from arguments | ✅ | ✅ | 10240 |
| `methods[].responseMaxSerializationSize` | `@size` or computed from arguments | ✅ | ✅ | 10240 |
| `methods[].maxSlots` | `@maxslots` annotation | ✅ | ✅ | 20 |
| `methods[].transportProtocol` | `SomeipMethodDeployment.transportProtocol` | ✅ | — | — |
| `methods[].isFireAndForget` | `ClientServerOperation.fireAndForget` keyword | ✅ | — | false |
| `fields[].notifier_eventId` | Field notifier event ID | ✅ | ✅ | — |
| `fields[].get_methodId` | Field getter method ID | ✅ | ✅ | — |
| `fields[].set_methodId` | Field setter method ID | ✅ | ✅ | — |
| `fields[].notifierMaximumSegmentLength` | TP segment length for field notifier | ✅ | — | 1404 |
| `fields[].notifierProtocol` | Transport protocol for field notifier | ✅ | — | — |
| `fields[].getterProtocol` | Transport protocol for field getter | ✅ | — | — |
| `fields[].setterProtocol` | Transport protocol for field setter | ✅ | — | — |

### 3.3. Service Instance Configuration

| Parameter | ARXML Element | Gateway | Client | Default |
|---|---|---|---|---|
| `instanceId` | `SomeipProvidedServiceInstance.serviceInstanceId` | ✅ | ✅ | — |
| `isProvider` | Inferred from Provided/Required | ✅ | — | — |
| `clientId` | `SomeipServiceInstanceToMachineMapping.clientId` | ✅ | — | 0 |
| `requiredMinVersion` | `RequiredServiceInstance.minorVersion` | ✅ | ✅ | — |
| `minimumMinorVersionFlag` | `RequiredServiceInstance.minimumMinorVersionFlag` | — | ✅ | false |
| `blacklistVersions[]` | `ServiceInstanceCollectionSet.blocklistedVersions` | — | ✅ | — |

### 3.4. Serialization Transformation Properties

| Parameter | ARXML Element | Default SOMEIP | Default IPC |
|---|---|---|---|
| `byteOrder` | `ApSomeipTransformationProps.byteOrder` | Big Endian | Opaque |
| `sizeOfArrayLengthField` | `.sizeOfArrayLengthField` | 4 | 0 |
| `sizeOfStringLengthField` | `.sizeOfStringLengthField` | 4 | 4 |
| `sizeOfStructLengthField` | `.sizeOfStructLengthField` | 0 | 0 |
| `sizeOfUnionLengthField` | `.sizeOfUnionLengthField` | 4 | 4 |
| `sizeOfUnionTypeSelectorField` | `.sizeOfUnionTypeSelectorField` | 1 | 1 |
| `isDynamicLengthFieldSize` | `TPS_MANI_01186 §3.18.3.2` | false | false |

### 3.5. E2E Protection (client only)

| Parameter | ARXML Element | Default |
|---|---|---|
| `profileName` | `EndToEndTransformationDescription.profileName` | — |
| `clearFromValidToInvalid` | `.clearFromValidToInvalid` | false |
| `maxDeltaCounter` | `.maxDeltaCounter` | — |
| `dataIdMode` | `.dataIdMode` | All16Bit |
| `maxErrorState{Init,Invalid,Valid}` | `.maxErrorState*` | — |
| `minOkState{Init,Invalid,Valid}` | `.minOkState*` | — |
| `windowSize{Init,Invalid,Valid}` | `.windowSize*` | — |
| `dataIds[]` | `EndToEndTransformationISignalProps.dataIds` | — |
| `minDataLength` / `maxDataLength` | `.minDataLength` / `.maxDataLength` | — |
| `sourceId` | `.sourceId` | — |
| `componentId` | Unique per E2E state machine | — |

### 3.6. SecOC Configuration (gateway only)

| Parameter | ARXML Element | Default |
|---|---|---|
| `authAlgorithm` | `SecOcSecureComProps.authAlgorithm` | — |
| `authInfoTxLength` | `.authInfoTxLength` | — |
| `freshnessValueLength` | `.freshnessValueLength` | — |
| `freshnessValueTxLength` | `.freshnessValueTxLength` | — |
| `authenticationBuildAttempts` | `.authenticationBuildAttempts` | 1 |
| `authenticationRetries` | `.authenticationRetries` | 1 |
| `elementSecOcConfigs[].freshnessValueId` | `ServiceInterfaceElementSecureComConfig.freshnessValueId` | — |
| `elementSecOcConfigs[].dataId` | `.dataId` | — |
| `elementSecOcConfigs[].eventId` | `.eventId` | — |
| `elementSecOcConfigs[].methodCallId/ReturnId` | `.methodCallId` / `.methodReturnId` | — |
| `elementSecOcConfigs[].fieldNotifierId` | `.fieldNotifierId` | — |
| `elementSecOcConfigs[].getterCallId/ReturnId` | `.getterCallId` / `.getterReturnId` | — |
| `elementSecOcConfigs[].setterCallId/ReturnId` | `.setterCallId` / `.setterReturnId` | — |

### 3.7. S2S Signal Mapping

| Parameter | ARXML Element |
|---|---|
| `iSignalTriggerings[].shortName` | `ISignalTriggering.shortName` |
| `iSignals[].name` | `ISignal.shortName` |
| `iSignals[].type` | `ISignal` primitive vs array |
| `iSignals[].length` | `ISignal.iSignalLength` |
| `iSignalIPduMappings[].packingByteOrder` | `ISignalMapping.packingByteOrder` |
| `iSignalIPduMappings[].startPosition` | `ISignalMapping.startPosition` |
| `iSignalIPdus[].length` | `ISignalIPdu.length` |
| `iSignalIPdus[].unusedBitPattern` | `ISignalIPdu.unusedBitPattern` |
| `securedIPdus[].dataId` | `SecuredIPdu.dataId` |
| `securedIPdus[].freshnessValueId` | `SecuredIPdu.freshnessValueId` |

### 3.8. DDS-Specific (client only)

| Parameter | ARXML Element |
|---|---|
| `domainId` | `DdsServiceInstance.domainId` |
| `qosProfile` | `DdsServiceInstance.qosProfile` |
| `discoveryType` | `DdsServiceInstance.discoveryType` |
| `resourceIdentifierType` | `DdsProvidedServiceInstance.resourceIdentifierType` |
| `fieldReplyTopicName` / `fieldRequestTopicName` | `DdsServiceInterfaceDeployment` topic names |
| `methodReplyTopicName` / `methodRequestTopicName` | `DdsServiceInterfaceDeployment` topic names |
| `enableDiscoveryProtection` | `DdsTopicAccessRule.*` |
| `dataProtectionKind` / `metadataProtectionKind` | `DdsTopicAccessRule.*` |

### 3.9. Process-Level Configuration

| Parameter | ARXML / Environment | Default |
|---|---|---|
| `threadPoolSize` | `Process.connectionThreadpoolSize` | 0 (dynamic) |
| `ECUCFG_ENV_VAR_ROOTFOLDER` | Environment variable | — |
| `PIPC_PREALLOCATE_THREADS` | Environment variable | OFF |

---

## 4. Total Parameter Count

| Category | Unique Parameters |
|---|---|
| Service Interface Design | 5 |
| SOME/IP Deployment | 22 |
| IPC Deployment | 12 |
| DDS Deployment | 18 |
| Service Instance | 8 |
| Serialization Transformation | 7 |
| E2E Protection | 16 |
| SecOC | 15 |
| S2S Signal Mapping | 10 |
| Process-Level | 3 |
| **Total** | **~116** |

---

## 5. Schema Validation Example

Below is a minimal example that would validate against the schema above — a gateway
configuration with one SOME/IP service containing an event, a method, and a field:

```json
{
  "functionCluster": "COM",
  "versionMajor": 1,
  "versionMinor": 0,

  "serviceInterfaces": [
    {
      "name": "com.score.examples.CarWindowService",
      "events": [{ "name": "WindowPosition", "shortName": "WindowPosition" }],
      "fields": [
        {
          "name": "WindowState",
          "hasGetter": true,
          "hasSetter": true,
          "hasNotifier": true
        }
      ]
    }
  ],

  "someipDeployments": [
    {
      "serviceInterfaceName": "com.score.examples.CarWindowService",
      "serviceId": 4660,
      "majorVersion": 1,
      "minorVersion": 0,
      "events": [
        {
          "name": "WindowPosition",
          "id": 32769,
          "maxSerializationSize": 256,
          "maxPublisherSlots": 10,
          "maxSubscriberSlots": 5,
          "transportProtocol": "Udp",
          "segmentLength": 1404,
          "sessionHandling": true,
          "signalBased": false
        }
      ],
      "methods": [
        {
          "name": "MoveWindow",
          "id": 1,
          "requestMaxSerializationSize": 64,
          "responseMaxSerializationSize": 64,
          "maxSlots": 10,
          "transportProtocol": "Tcp"
        }
      ],
      "fields": [
        {
          "name": "WindowState",
          "hasNotifier": true,
          "hasGetter": true,
          "hasSetter": true,
          "notifier": {
            "name": "WindowState_notifier",
            "id": 32770,
            "maxSerializationSize": 128,
            "maxPublisherSlots": 5,
            "maxSubscriberSlots": 3,
            "transportProtocol": "Udp"
          },
          "getter": {
            "name": "WindowState_get",
            "id": 2,
            "requestMaxSerializationSize": 0,
            "responseMaxSerializationSize": 128,
            "maxSlots": 5,
            "transportProtocol": "Tcp"
          },
          "setter": {
            "name": "WindowState_set",
            "id": 3,
            "requestMaxSerializationSize": 128,
            "responseMaxSerializationSize": 0,
            "maxSlots": 5,
            "transportProtocol": "Tcp"
          }
        }
      ]
    }
  ],

  "providedSomeipInstances": [
    {
      "name": "com.score.examples.CarWindowService_1",
      "instanceId": 1,
      "deployment": "com.score.examples.CarWindowService"
    }
  ],
  "requiredSomeipInstances": [
    {
      "name": "com.score.examples.CarWindowService_proxy",
      "instanceId": 1,
      "requiredMinVersion": 0,
      "minimumMinorVersionFlag": true,
      "deployment": "com.score.examples.CarWindowService"
    }
  ]
}
```

---

## 6. Relevance for `inc_someip_gateway`

For the `inc_someip_gateway` project, the relevant subset is:

1. **SOME/IP Deployment** — all 22 parameters are directly applicable to the
   `SomeipSerializer<E>` and `SomeipMessageHandler` in `src/someipd/`
2. **Service Instance** — all 8 parameters are required by `LocalServiceInstance`
   and `RemoteServiceInstance` in `src/gatewayd/`
3. **Serialization Transformation** — all 7 parameters drive the endianness policy
   and length-field settings consumed by `SomeipSerializer<EndianPolicy>`
4. **E2E Protection** — 16 parameters, needed when E2E support is added to the
   `PayloadPipeline<Transformer, Serializer>`
5. **SecOC** — 15 parameters, needed when SecOC support is added
6. **S2S** — 10 parameters, needed for signal-based event translation

The IPC and DDS deployment parameters are **not** directly relevant since `someipd` only handles
the SOME/IP network binding. However, the schema retains them for completeness as the
`gatewayd` process does interact with the IPC layer via MW COM.

### 6.1. Mapping to `gatewayd_config.fbs`

The existing `gatewayd_config.fbs` (in `src/gatewayd/etc/`) is a **greatly simplified**
subset of the gateway FBS schema above. The current `gatewayd` config covers:

| Current gatewayd field | Equivalent in this schema |
|---|---|
| `service_id` | `someipDeployments[].serviceId` |
| `instance_id` | `providedSomeipInstances[].instanceId` |
| `major_version` / `minor_version` | `someipDeployments[].majorVersion` / `minorVersion` |

Adopting the full JSON schema would allow `gatewayd` to also receive per-event, per-method,
and per-field configuration (serialization sizes, slots, transport protocols) without
requiring code changes to the Flatbuffers schema.

### 6.2. Mapping to `SomeipSerializer<E>`

The `ApSomeipTransformationProps` definition in the schema maps directly to the
`SomeipSerializer` endianness policy template parameter and potential future
`SerializerSettings` struct:

| Schema field | Serializer impact |
|---|---|
| `byteOrder` | Selects `BigEndianPolicy` vs `LittleEndianPolicy` template arg |
| `sizeOfArrayLengthField` | Array length prefix width in `Serialize<std::array<T,N>>` |
| `sizeOfStringLengthField` | String length prefix width in `Serialize<std::string>` |
| `sizeOfStructLengthField` | Struct length prefix in TLV-enabled structs |
| `sizeOfUnionLengthField` | Union payload length prefix width |
| `sizeOfUnionTypeSelectorField` | Union discriminator width |
| `isDynamicLengthFieldSize` | Controls whether length fields are resized to fit actual payload |
