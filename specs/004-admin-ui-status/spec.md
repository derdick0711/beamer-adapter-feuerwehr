# Feature Specification: Admin UI Status & Configuration Improvements

**Feature Branch**: `004-admin-ui-status`

**Created**: 2026-05-25

**Status**: Draft

**Input**: User description: "Die Admin Seite soll wie folgt überarbeitet werden: Die Shelly Konfiguration soll anzeigen, ob eine IP eingetragen wurde. Die Mqtt Daten sollen angezeigt werden, wenn diese konfiguriert wurden. allgemein sollen keine Werte angezeigt werden, wenn nichts konfiguriert ist. Die WLAN und IP Einstellungen sollen auch angezeigt werden, insbesondere die Einstellungen für eine feste ip. Beim Speichern sollen nur veränderungen gespeichert werden. Alles andere bleibt unberührt. Auf der index.html soll die aktuelle IP und angezeigt werden. Wenn MQTT genutzt wird soll das auch angezeigt werden (einfach als Status: ON/OFF). via mqtt soll die aktuelle IP auch gepostet werden, retained, wenn änderung, dann update."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - View Current Configuration Status (Priority: P1)

As a technician opening the admin page, I want to immediately see which settings are already configured, so I know at a glance what needs to be set up and what is already working.

**Why this priority**: Without this, every visit to the admin page shows empty fields regardless of what's already stored — making it impossible to confirm configuration without testing each device manually.

**Independent Test**: Open the admin page after saving a Shelly IP; the saved IP must be pre-filled in its field. Unconfigured optional sections must be collapsed or hidden.

**Acceptance Scenarios**:

1. **Given** a Shelly IP has been saved, **When** the admin page loads, **Then** the IP field is pre-filled with the stored value and a visual indicator confirms it is configured.
2. **Given** MQTT has not been configured, **When** the admin page loads, **Then** the MQTT section shows a collapsed/empty state with no partially-filled fields.
3. **Given** MQTT has been configured (host, port, prefix), **When** the admin page loads, **Then** all MQTT fields are pre-filled with their stored values.
4. **Given** no static IP is configured, **When** the admin page loads, **Then** the static IP fields are hidden or clearly marked as inactive.

---

### User Story 2 - View and Edit WiFi & IP Settings (Priority: P1)

As a technician, I want to see the current WiFi network and optional static IP configuration on the admin page, so I can review and change network settings without needing serial access.

**Why this priority**: Network settings (especially static IP) are critical for a fire station environment where predictable addressing is required; they must be visible and editable in the web UI.

**Independent Test**: Open the admin page; the current WiFi SSID and static IP toggle state must be displayed. Enabling static IP reveals address/gateway/subnet fields.

**Acceptance Scenarios**:

1. **Given** the device is connected to a WiFi network, **When** the admin page loads, **Then** the current SSID is displayed (read-only or editable).
2. **Given** static IP is disabled, **When** the admin page loads, **Then** the static IP toggle is off and address fields are hidden.
3. **Given** static IP is enabled with address/gateway/subnet stored, **When** the admin page loads, **Then** all static IP fields are pre-filled and visible.
4. **Given** the technician toggles static IP on, **When** they fill in the fields and save, **Then** the stored network config is updated with the new static IP settings.

---

### User Story 3 - Partial Save (Only Changed Fields) (Priority: P2)

As a technician, I want saving the admin form to only update the values I have changed, so that leaving a field empty does not accidentally erase a stored setting.

**Why this priority**: Without partial-save, a technician who only wants to update a single IP could inadvertently wipe passwords or other settings by leaving those fields blank.

**Independent Test**: Save the admin form with only one field changed; inspect stored config and confirm all other fields retain their previous values.

**Acceptance Scenarios**:

1. **Given** a Shelly IP and admin password are already stored, **When** the technician changes only the Shelly IP and saves, **Then** the password remains unchanged and only the Shelly IP is updated.
2. **Given** MQTT is configured, **When** the technician opens the admin page, changes no MQTT fields, and saves, **Then** MQTT settings are unchanged.
3. **Given** an empty password field on save, **When** the form is submitted, **Then** the existing password hash is retained (empty = no change).

---

### User Story 4 - IP Address and MQTT Status on Main Page (Priority: P2)

As a user or technician on the main control page, I want to see the device's current IP address and whether MQTT is active, so I can confirm the device is correctly set up at a glance.

**Why this priority**: Fire station users often need to confirm which device they are controlling; the IP and MQTT status provide quick orientation without navigating to the admin page.

**Independent Test**: Load the main page after device boot; the current IP address must be visible in the status bar. If MQTT is enabled and connected, "MQTT: ON" badge is shown.

**Acceptance Scenarios**:

1. **Given** the device is connected to WiFi, **When** the main page loads, **Then** the device's IP address is displayed in the status bar.
2. **Given** MQTT is enabled and the broker is reachable, **When** the main page loads, **Then** a badge showing "MQTT: ON" is visible.
3. **Given** MQTT is disabled, **When** the main page loads, **Then** no MQTT badge is shown (or a hidden/off state is used).

---

### User Story 5 - IP Address Published via MQTT (Priority: P3)

As a home-automation or network administrator, I want the device to automatically publish its current IP address to an MQTT topic as a retained message, so that other systems can always discover the device's address.

**Why this priority**: Lower priority as this benefits integrators rather than daily operators; the main UI features are more immediately impactful.

**Independent Test**: Subscribe to `<prefix>/ip` on the broker; after device boot the retained message with the IP must appear. Change network settings and reboot; the retained message must update.

**Acceptance Scenarios**:

1. **Given** MQTT is enabled, **When** the device connects to WiFi and MQTT, **Then** it publishes its IP address to `<prefix>/ip` as a retained message.
2. **Given** a retained IP message exists, **When** the device reboots and reconnects with the same IP, **Then** the retained message is re-published (refreshed).
3. **Given** the device's IP changes (e.g., after static IP config change), **When** the device reconnects, **Then** the new IP is published, replacing the old retained message.

---

### Edge Cases

- What happens when the admin page loads but the device cannot read its stored config (e.g., LittleFS error)? Fields should default to empty/unconfigured state, not show corrupt data.
- What if the MQTT broker is configured but unreachable? MQTT status badge should reflect the actual connection state, not just the enabled toggle.
- What if static IP fields are visible but saved with partially filled values? Validation must require all three fields (address, gateway, subnet) or none.
- What if the WiFi SSID contains special characters? Display must handle UTF-8 or at minimum ASCII-safe output.

## Requirements *(mandatory)*

### Functional Requirements

**Admin Page — Configuration Display**

- **FR-001**: The admin page MUST pre-fill each field with its currently stored value when the page loads.
- **FR-002**: Fields or sections that have no stored value MUST be presented in a visually distinct unconfigured state (empty field with placeholder, or collapsed section).
- **FR-003**: The Shelly device section MUST show a visual indicator (e.g., green dot, "configured" label) next to each IP field that has a stored value.
- **FR-004**: The MQTT section MUST be hidden or collapsed when MQTT has never been configured; it MUST be expanded and populated when a broker host has been stored.
- **FR-005**: The admin page MUST display a WiFi/Network section with an editable SSID field and a masked password field (empty = no change on save). Changes take effect on next restart.
- **FR-005a**: A "Restart required" notice MUST be shown after saving WiFi credential changes, consistent with the MQTT restart banner pattern.
- **FR-006**: When static IP is disabled, the address/gateway/subnet fields MUST be hidden. When enabled, all three fields MUST be visible and pre-filled with stored values.

**Admin Page — Partial Save**

- **FR-007**: Submitting the admin form MUST only update a stored value if the corresponding field has a non-empty value that differs from the stored value. This applies to ALL fields including Shelly IPs — an empty Shelly IP field means "no change", not "clear this value".
- **FR-008**: An empty password field on save MUST leave the existing password hash unchanged.
- **FR-009**: An empty username field on save MUST leave the existing username unchanged.
- **FR-010**: An empty MQTT host field on save MUST leave the existing MQTT configuration unchanged (unless MQTT enable toggle is explicitly changed).
- **FR-011**: Disabling the MQTT toggle and saving MUST update the enabled flag to false, but leave host/port/prefix values intact so they can be re-enabled later.

**Main Page — Status Display**

- **FR-012**: The main page MUST display the device's current IP address in the status bar on load.
- **FR-013**: The main page MUST show an "MQTT: ON" badge when MQTT is enabled and the broker connection is active. This state MUST be refreshed on the same polling interval used for existing beamer status updates (not only on page load).
- **FR-014**: The MQTT badge MUST NOT be shown when MQTT is disabled or the broker is unreachable.

**MQTT — IP Publishing**

- **FR-015**: When MQTT is enabled, the device MUST publish its current IP address to the topic `<prefix>/ip` as a retained message upon MQTT connection.
- **FR-016**: The IP topic MUST be updated whenever the device reconnects to MQTT with a (potentially new) IP address.

### Key Entities

- **Device Configuration**: Stored settings for Shelly IPs, WiFi credentials, static IP, MQTT broker, admin credentials. Partial updates preserve fields not explicitly changed.
- **MQTT Connection State**: Runtime state (connected / disconnected / disabled) distinct from the configured enable flag.
- **Network Address**: Current assigned IP (dynamic or static), published as a retained MQTT message.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A technician can confirm all currently stored settings by opening the admin page once — no saved setting requires a separate lookup.
- **SC-002**: Saving a change to one field leaves all other stored values identical to their pre-save state (zero unintended overwrites).
- **SC-003**: The device's IP address is visible on the main page within the normal page-load time (under 3 seconds on local WiFi).
- **SC-004**: After MQTT connection, the retained IP message is published within 5 seconds and verifiable via an MQTT subscriber.
- **SC-005**: The MQTT status badge on the main page reflects actual broker connectivity (not just the configured toggle), updates within one polling cycle after a broker connect or disconnect event, and never shows a stale state for more than the polling interval.

## Clarifications

### Session 2026-05-25

- Q: What should happen when a Shelly IP field is left empty on save? → A: Empty = keep existing value; Shelly IPs follow the same partial-save rule as all other fields.
- Q: Should WiFi SSID and password be editable on the admin page? → A: Yes, editable; changes take effect on next restart.
- Q: Should the MQTT status badge on the main page reflect live connection state or page-load state only? → A: Live — same polling interval as existing beamer status updates.

## Assumptions

- The device is always accessed over the local WiFi network (no internet-facing deployment).
- WiFi credentials (SSID/password) can be updated via the admin page; changes are stored and take effect on next restart. First-boot provisioning via WiFiManager captive portal is unaffected.
- Static IP configuration entered in the admin page takes effect after a device restart; no in-session apply is required.
- The MQTT topic prefix is already configurable (stored as `prefix` in MQTT config); the IP topic is `<prefix>/ip`.
- The main page fetches status data from a `/status` or `/api/status` endpoint; the IP address is included in that response.
- A "retained" MQTT message is published using the broker's retain flag so subscribers always receive the last value on connect.
