<!--
  Sync Impact Report
  Version change: [TEMPLATE] → 1.0.0
  Modified principles: N/A (initial population from template)
  Added sections: Core Principles (I–V), Hardware Constraints, Development Workflow, Governance
  Removed sections: N/A
  Templates requiring updates:
    ✅ constitution.md — this file (updated)
    ✅ plan-template.md — Constitution Check section is generic; aligns with embedded firmware context
    ✅ spec-template.md — no changes required; template is technology-agnostic
    ✅ tasks-template.md — no changes required; phase structure aligns with firmware development
    ✅ commands/ — no command template files found
  Follow-up TODOs:
    - TODO(ACER_RS232_PROTOCOL): Confirm exact RS232 command set for Acer H6512BD
      (baud rate, command format, power-on/off sequences)
    - TODO(NETWORK_PROTOCOL): Decide WiFi control protocol (HTTP REST / MQTT / WebSocket)
    - TODO(AUTH): Define authentication for the remote control endpoint
-->

# Beamer Adapter Feuerwehr Constitution

## Core Principles

### I. RS232-Serial Communication

All projector control MUST be implemented via RS232 serial communication through the MAX3232
level converter to the Acer H6512BD. No alternative control path (IR, GPIO, network-direct)
is permitted. Serial parameters (baud rate, data bits, stop bits, parity) MUST match the
Acer H6512BD RS232 specification exactly and MUST be configurable without recompilation.

**Rationale**: The MAX3232 is the single, defined hardware bridge between the ESP32-S3
(3.3V UART) and the projector (RS232 ±12V). Routing all control through it ensures a
predictable, testable communication path.

### II. WiFi Remote Control

The ESP32-S3 MUST expose a WiFi-based interface for receiving control commands remotely.
The chosen protocol MUST be simple and stateless where possible. The system MUST reconnect
automatically after WiFi loss without requiring physical intervention or a reboot.
WiFi credentials MUST be stored in NVS and MUST NOT be hardcoded in source files.

**Rationale**: Fire department deployments require remote activation and control of
projection equipment without physical access to the adapter device.

### III. Reliability-First (Feuerwehr-Grade)

The system MUST become fully operational within 30 seconds of power application. Command
execution MUST complete or time out within 10 seconds. On any unrecoverable error the
system MUST log the fault and return to a known idle state — it MUST NOT remain in a
broken state requiring manual reboot. Hardware watchdog timers MUST be enabled at all
times. RS232 transmission errors MUST be detected and reported.

**Rationale**: Emergency services require equipment that works reliably every time it is
called upon. Failure during an operational scenario is not acceptable.

### IV. Embedded-Efficient Design

Firmware MUST be designed for the ESP32-S3 resource constraints (16 MB Flash, 8 MB PSRAM).
Dynamic heap allocations MUST be minimized; prefer static or stack-allocated buffers for
serial framing and command queues. The firmware MUST use the Arduino-ESP32 framework
managed via PlatformIO. Memory usage MUST be reviewed before any release.

**Rationale**: Embedded systems have finite, non-expandable resources. Unbounded heap
allocation leads to unpredictable failures in long-running, unattended deployments.

### V. Simplicity and Minimal Footprint

Each feature MUST have a directly named, concrete operational use case in the fire
department workflow. Features without a stated use case MUST NOT be added. The firmware
configuration (WiFi credentials, RS232 parameters, beamer address) MUST be changeable at
runtime via NVS without recompilation. The codebase MUST remain understandable by a single
developer without additional onboarding documentation.

**Rationale**: This device is a single-purpose RS232 adapter. Complexity that does not serve
the Feuerwehr mission introduces failure modes with no operational benefit (YAGNI).

## Hardware Constraints

The following constraints are fixed by the available hardware and MUST be respected in all
design and implementation decisions:

- **MCU**: ESP32-S3 DevKitC-1 N16R8 — Xtensa LX7 dual-core, 240 MHz, 16 MB Flash, 8 MB PSRAM
- **Serial Bridge**: MAX3232 chip board — connects ESP32-S3 UART (3.3 V) to Acer beamer RS232 port (±12 V)
- **Target Device**: Acer H6512BD projector — DLP, RS232 DB-9 control port
- **Connectivity**: WiFi 802.11 b/g/n (2.4 GHz); Bluetooth 5.0 reserved for potential future use
- **Power Supply**: USB-C (5 V) or dedicated 5 V rail to the DevKit
- The MAX3232 board MUST be wired to a hardware UART peripheral of the ESP32-S3 (software
  serial emulation is prohibited due to timing reliability requirements)

## Development Workflow

- **Language / Framework**: C/C++ via Arduino-ESP32 framework (managed by PlatformIO)
- **Build System**: PlatformIO ONLY — `idf.py` or bare ESP-IDF CLI are prohibited.
  All builds, uploads, and library management MUST go through PlatformIO.
- **Flashing**: USB-C to ESP32-S3 DevKit; `pio run -t upload` / PlatformIO IDE upload
- **Validation**: Hardware-in-the-loop testing against the physical Acer H6512BD is the
  primary acceptance method. Unit tests for RS232 command encoding/decoding are encouraged
  and should run on host (native target) where possible.
- **Secrets**: WiFi SSID/password and any access credentials MUST NOT be committed to the
  repository. Use NVS provisioning or a `.env`/`secrets.h` excluded via `.gitignore`.
- **Logging**: Serial monitor output (USB) is the primary debug channel. Log levels MUST be
  configurable via a compile-time flag; verbose logging MUST be disabled in release builds.

## Governance

This constitution defines the non-negotiable constraints for the Beamer Adapter Feuerwehr
firmware project. It supersedes all other practices and ad-hoc guidelines.

**Amendment procedure**: Amendments require a written justification documenting the
operational need, the hardware impact, and any migration plan for existing firmware.
Amendments MUST be ratified before implementation proceeds and MUST increment the version.

**Versioning policy**:
- MAJOR: Removal or fundamental redefinition of an existing principle
- MINOR: New principle, constraint, or workflow section added
- PATCH: Clarifications, wording improvements, or non-semantic refinements

**Compliance review**: All feature plans (plan.md) MUST include a Constitution Check section
verifying adherence to Principles I–V before the implementation phase begins.

**Version**: 1.0.1 | **Ratified**: 2026-05-22 | **Last Amended**: 2026-05-22
