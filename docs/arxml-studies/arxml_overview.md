<!--
SPDX-License-Identifier: Apache-2.0
SPDX-FileCopyrightText: 2026 Contributors to the Eclipse Foundation
-->

# ARXML Files in Example Code: Overview and Parameter Analysis

This document provides an overview of the AUTOSAR XML (ARXML) files available in the example code of this repository. It highlights their common features and identifies which parameters are generic (common to all ARXMLs) and which are specific to the application domain.

## 1. ARXML Files in the Example Code

The example code (see `examples/car_window_sim/` and related folders) may include ARXML files for service/interface definitions, communication configurations, and application-specific data types. (Note: If no ARXMLs are present, this document serves as a template for future integration.)

### Typical ARXML Locations
- `examples/car_window_sim/` (or subfolders)
- `src/gatewayd/etc/` (for reference configs)
- `src/network_service/interfaces/` (for interface definitions)

## 2. Common Features of ARXML Files

ARXML files in AUTOSAR projects typically share the following features:

| Feature                | Description                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| XML Structure          | All ARXMLs use a standardized XML schema defined by AUTOSAR.                |
| Namespace Declarations | Use AUTOSAR-specific XML namespaces.                                        |
| Top-Level Elements     | `<AUTOSAR>`, `<AR-PACKAGES>`, `<AR-PACKAGE>`, `<ELEMENTS>`, etc.            |
| UUIDs                  | Each element may have a unique identifier (UUID) for traceability.          |
| Short/Long Names       | `<SHORT-NAME>` and `<LONG-NAME>` for human-readable identification.          |
| Versioning             | May include schema version and tool version metadata.                       |
| References             | Use of `<REF>` elements to link to other ARXML-defined entities.            |

## 3. Common Parameters (Generic)

The following parameters are typically found in all ARXML files, regardless of application:

| Parameter         | Description                                      |
|-------------------|--------------------------------------------------|
| SHORT-NAME        | Unique name for the element                      |
| UUID              | Universally unique identifier                    |
| CATEGORY          | Type/category of the element (e.g., SERVICE)     |
| ADMIN-DATA        | Tool and authoring metadata                      |
| DESC              | Description of the element                       |
| LONG-NAME         | Extended human-readable name                     |
| VERSION           | AUTOSAR schema/tool version                      |
| AR-PACKAGES       | Container for packages of elements               |
| ELEMENTS          | List of defined elements (types, ports, etc.)    |

## 4. Application-Specific Parameters

Parameters that are specific to the application (e.g., car window simulation) typically include:

| Parameter/Section         | Description                                                      |
|--------------------------|------------------------------------------------------------------|
| DATA-TYPEs               | Custom data types (e.g., `WindowPosition`, `WindowCommand`)      |
| SERVICE-INTERFACE        | Application-specific service definitions (e.g., `CarWindowService`)|
| PORT-PROTOTYPE           | Ports for application communication                              |
| EVENT/OPERATION          | Events or operations unique to the application                   |
| INIT-VALUE               | Initial values for application signals                           |
| APPLICATION-SW-COMPONENT | Application software component definitions                       |
| MAPPINGs                 | Mapping of application signals to communication channels         |

## 5. Example: Car Window Simulation ARXML (Hypothetical)

```xml
<AUTOSAR>
  <AR-PACKAGES>
    <AR-PACKAGE>
      <SHORT-NAME>CarWindowTypes</SHORT-NAME>
      <ELEMENTS>
        <APPLICATION-PRIMITIVE-DATA-TYPE>
          <SHORT-NAME>WindowPosition</SHORT-NAME>
          <CATEGORY>VALUE</CATEGORY>
          <SW-DATA-DEF-PROPS>
            <BASE-TYPE-REF>uint8</BASE-TYPE-REF>
          </SW-DATA-DEF-PROPS>
        </APPLICATION-PRIMITIVE-DATA-TYPE>
        <!-- ... more types ... -->
      </ELEMENTS>
    </AR-PACKAGE>
    <AR-PACKAGE>
      <SHORT-NAME>CarWindowService</SHORT-NAME>
      <ELEMENTS>
        <SERVICE-INTERFACE>
          <SHORT-NAME>CarWindowControl</SHORT-NAME>
          <OPERATIONS>
            <CLIENT-SERVER-OPERATION>
              <SHORT-NAME>SetWindowPosition</SHORT-NAME>
              <ARGUMENTS>
                <ARGUMENT>
                  <SHORT-NAME>position</SHORT-NAME>
                  <TYPE-TREF>WindowPosition</TYPE-TREF>
                </ARGUMENT>
              </ARGUMENTS>
            </CLIENT-SERVER-OPERATION>
          </OPERATIONS>
        </SERVICE-INTERFACE>
      </ELEMENTS>
    </AR-PACKAGE>
  </AR-PACKAGES>
</AUTOSAR>
```

## 6. Summary Table: Common vs. Application-Specific Parameters

| Parameter/Section         | Common to All ARXMLs | Application-Specific |
|--------------------------|:--------------------:|:-------------------:|
| SHORT-NAME                |          ✓           |          ✓          |
| UUID                      |          ✓           |          ✓          |
| CATEGORY                  |          ✓           |          ✓          |
| ADMIN-DATA                |          ✓           |          ✓          |
| DESC/LONG-NAME            |          ✓           |          ✓          |
| VERSION                   |          ✓           |                     |
| AR-PACKAGES/ELEMENTS      |          ✓           |          ✓          |
| DATA-TYPEs                |                      |          ✓          |
| SERVICE-INTERFACE         |                      |          ✓          |
| PORT-PROTOTYPE            |                      |          ✓          |
| EVENT/OPERATION           |                      |          ✓          |
| INIT-VALUE                |                      |          ✓          |
| APPLICATION-SW-COMPONENT  |                      |          ✓          |
| MAPPINGs                  |                      |          ✓          |

## 7. References
- [AUTOSAR XML Schema Documentation](https://www.autosar.org/standards/classic-platform/classic-platform-xml-schema/)
- Example ARXMLs in this repository (see `examples/` and `src/` folders)

---
*This document is intended as a living reference. Please update as new ARXMLs or application features are added to the example code.*
