# Feature Specification: Eingangswahl auf HDMI und VGA reduzieren

**Feature Branch**: `003-reduce-inputs`

**Created**: 2026-05-23

**Status**: Draft

**Input**: User description: "es soll nur hdmi und vga ausgewählt werden. alles andere entfernen."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Eingang wählen (HDMI oder VGA) (Priority: P1)

Ein Benutzer möchte den aktiven Videoeingang des Beamers auf HDMI oder VGA umschalten. Component-, S-Video- und Composite-Eingänge sind nicht mehr auswählbar — weder über die Web-Oberfläche, noch über die REST API oder MQTT.

**Why this priority**: Die Vereinfachung der Oberfläche reduziert Bedienfehler und spiegelt die tatsächlich genutzten Eingänge im Feuerwehr-Präsentationsraum wider (HDMI für Laptops, VGA als Fallback).

**Independent Test**: Nach der Änderung zeigt die Web-Oberfläche nur noch die Buttons „HDMI" und „VGA". Ein Klick auf einen der Buttons schaltet den Beamer korrekt auf den gewählten Eingang um und bestätigt dies im Status-Badge.

**Acceptance Scenarios**:

1. **Given** die Web-Oberfläche ist geöffnet, **When** der Benutzer die Eingangssektion betrachtet, **Then** sind nur die Buttons „HDMI" und „VGA" sichtbar — keine Buttons für Component, S-Video oder Composite.
2. **Given** der Beamer ist eingeschaltet, **When** der Benutzer „HDMI" wählt, **Then** wechselt der Beamer auf HDMI und das Status-Badge zeigt „HDMI".
3. **Given** der Beamer ist eingeschaltet, **When** der Benutzer „VGA" wählt, **Then** wechselt der Beamer auf VGA und das Status-Badge zeigt „VGA".
4. **Given** ein API-Client sendet `POST /api/input` mit `{"input":"component"}`, **When** die Anfrage verarbeitet wird, **Then** antwortet der Adapter mit HTTP 400 (ungültiger Eingang).
5. **Given** ein MQTT-Client sendet `beamer/cmnd/input` mit Payload `component`, **When** die Nachricht verarbeitet wird, **Then** wird der Befehl ignoriert / abgelehnt.

---

### Edge Cases

- Was passiert, wenn der Beamer aktuell auf Component/S-Video/Composite eingestellt ist und der Adapter ihn abfragt? → Der Status wird korrekt angezeigt (Anzeige des tatsächlichen Zustands), jedoch ist kein Button zur Auswahl dieser Eingänge vorhanden.
- Was passiert, wenn der gespeicherte Zustand noch einen entfernten Eingang enthält? → Der Status-Badge zeigt den aktuellen Beamer-Eingang unverändert an; die aktive Hervorhebung eines Buttons erfolgt nur für HDMI und VGA.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Die Web-Oberfläche MUSS ausschließlich die Eingangsoptionen „HDMI" und „VGA" anbieten. Die Buttons für Component, S-Video und Composite MÜSSEN entfernt werden.
- **FR-002**: Die REST API MUSS Anfragen für die Eingänge `component`, `svideo` und `composite` mit HTTP 400 ablehnen. Nur `hdmi` und `vga` sind gültige Werte.
- **FR-003**: Der MQTT-Handler MUSS Nachrichten für `component`, `svideo` und `composite` auf dem `cmnd/input`-Topic ignorieren oder mit einer Fehlermeldung zurückweisen.
- **FR-004**: Die Szene „Präsentation starten" schaltet weiterhin automatisch auf HDMI — dieses Verhalten bleibt unverändert.
- **FR-005**: Der Status-Badge im Header zeigt den tatsächlich aktiven Eingang des Beamers unverändert an (auch wenn es sich um einen entfernten Eingang handelt).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Die Web-Oberfläche zeigt nach der Änderung genau 2 Eingangsbuttons (HDMI, VGA) — keine weiteren.
- **SC-002**: API-Aufrufe mit ungültigen Eingängen (component, svideo, composite) werden in 100 % der Fälle mit einem Fehler abgewiesen.
- **SC-003**: Die Umschaltzeit auf einen gültigen Eingang (HDMI/VGA) ändert sich nicht — kein Rückschritt gegenüber dem aktuellen Verhalten.
- **SC-004**: Die Szene „Präsentation starten" funktioniert weiterhin vollständig ohne Anpassung durch den Benutzer.

## Assumptions

- Der Acer H6512BD Beamer wird ausschließlich mit HDMI und VGA genutzt; Component, S-Video und Composite werden im Betrieb nicht benötigt.
- Die Validierung ungültiger Eingänge in der REST API soll aktiv mit HTTP 400 abgelehnt werden (nicht stillschweigend ignoriert), um Integrationsfehler frühzeitig zu erkennen.
- Das MQTT-Verhalten bei ungültigen Eingängen ist stilles Ignorieren (kein Error-Publish), da MQTT-Clients in der Regel keine Fehlerantworten auf Commands erwarten.
- Bestehende Clients, die bisher Component/S-Video/Composite genutzt haben, sind nicht vorhanden (Feuerwehr-Inhouse-Gerät).
