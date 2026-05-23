# Feature Specification: Shelly-Integration & Präsentationsautomatisierung

**Feature Branch**: `002-shelly-automation`

**Created**: 2026-05-23

**Status**: Draft

**Input**: User description: "zusätzlich sollen über diesen esp32 2 Shellies angesprochen werden. 1 Shelly 1plus für die Steuerung des Deckenlichts (ein/aus) sowie ein Shelly 2plus pm zur steuerung der Leinwand (auf/ab/stop/position). Eine automatisierte Funktion soll: Leinwand runter, Licht aus, Beamer an, HDMI Eingang. Eine weitere Funktion: Leinwand auf, Licht an, Beamer aus. Diese Funktionen sollen als automation auch via REST und MQTT verfügbar sein. Die IPs der Shellies sollen konfigurierbar sein in einer Admin-Oberfläche."

**Korrektur (2026-05-23)**: Shelly Generation 3. Shelly 1 als Mini-Variante (Shelly 1 Mini Gen3).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Präsentation starten (Ein-Knopf-Automatisierung) (Priority: P1)

Ein Feuerwehrmann möchte mit einem einzigen Tastendruck die gesamte Präsentationstechnik
aktivieren: Leinwand fährt herunter, Deckenlicht geht aus, Beamer schaltet ein und wählt
den HDMI-Eingang. Kein manuelles Schalten mehrerer Geräte notwendig.

**Why this priority**: Diese Automatisierung ist der Hauptnutzen der Erweiterung und deckt
den häufigsten Anwendungsfall im Feuerwehreinsatz ab.

**Independent Test**: Ein einzelner Tastendruck (Web-Button, REST-Aufruf oder MQTT-Befehl)
löst die vollständige Startsequenz aus. Alle vier Aktionen werden in der richtigen
Reihenfolge ausgeführt. Der Benutzer muss danach kein Gerät manuell bedienen.

**Acceptance Scenarios**:

1. **Given** alle Geräte sind erreichbar, **When** die Funktion „Präsentation starten"
   ausgelöst wird (Button, REST oder MQTT), **Then** fährt die Leinwand herunter, das
   Deckenlicht schaltet aus, der Beamer schaltet ein und wählt HDMI — in dieser
   Reihenfolge, innerhalb von 15 Sekunden.
2. **Given** die Funktion wird ausgelöst, **When** ein Gerät (Shelly oder Beamer) nicht
   antwortet, **Then** werden die übrigen Aktionen trotzdem ausgeführt und der Fehler
   wird dem Aufrufer gemeldet (Teilfehler, kein Abbruch).
3. **Given** die Funktion ist gestartet, **When** der Status abgefragt wird, **Then**
   wird zurückgemeldet, welche Schritte erfolgreich waren und welche fehlgeschlagen sind.

---

### User Story 2 - Präsentation beenden (Ein-Knopf-Automatisierung) (Priority: P1)

Ein Feuerwehrmann möchte nach der Präsentation mit einem Tastendruck alle Geräte in den
Ausgangszustand zurückversetzen: Leinwand fährt hoch, Deckenlicht geht an, Beamer
schaltet aus.

**Why this priority**: Gleichrangig zu US1 — ohne Beenden-Funktion ist der Raum nach
jeder Nutzung nicht im Ausgangszustand.

**Independent Test**: Ein einzelner Tastendruck (Web-Button, REST-Aufruf oder MQTT-Befehl)
löst die vollständige Beenden-Sequenz aus und bringt alle Geräte in den Ausgangszustand.

**Acceptance Scenarios**:

1. **Given** eine Präsentation läuft, **When** die Funktion „Präsentation beenden" ausgelöst
   wird, **Then** fährt die Leinwand hoch, das Deckenlicht schaltet ein und der Beamer
   schaltet aus — innerhalb von 15 Sekunden.
2. **Given** die Funktion wird ausgelöst, **When** ein Gerät nicht antwortet, **Then**
   werden die übrigen Aktionen trotzdem ausgeführt und der Fehler gemeldet.
3. **Given** die Funktion ist abgeschlossen, **When** der Status abgefragt wird, **Then**
   wird der Ergebnisstatus aller drei Aktionen zurückgegeben.

---

### User Story 3 - Deckenlicht manuell steuern (Priority: P2)

Ein Nutzer möchte das Deckenlicht unabhängig von der Automatisierung ein- oder ausschalten
können — z.B. um Licht einzuschalten ohne den Beamer zu starten.

**Why this priority**: Ergänzt die Automatisierung um manuelle Einzelsteuerung; setzt den
Shelly-1-Plus-Anschluss (US1/US2) voraus.

**Independent Test**: Deckenlicht lässt sich per Web-Button, REST-Aufruf und MQTT
unabhängig ein- und ausschalten. Status wird korrekt zurückgegeben.

**Acceptance Scenarios**:

1. **Given** der Adapter läuft, **When** der Benutzer „Licht ein" wählt (Web/REST/MQTT),
   **Then** schaltet der Shelly 1 Plus das Deckenlicht ein.
2. **Given** das Licht ist an, **When** der Benutzer „Licht aus" wählt, **Then** schaltet
   der Shelly 1 Plus das Licht aus.
3. **Given** das Licht ist geschaltet, **When** der Status abgefragt wird, **Then** wird
   der aktuelle Schaltzustand (an/aus) zurückgegeben.

---

### User Story 4 - Leinwand manuell steuern (Priority: P2)

Ein Nutzer möchte die Leinwand unabhängig von der Automatisierung auf- und abfahren sowie
stoppen können.

**Why this priority**: Ergänzt die Automatisierung; nötig für flexible Nutzung (z.B.
Leinwand teilweise ausfahren).

**Independent Test**: Leinwand lässt sich per Web-Button, REST-Aufruf und MQTT in alle
Richtungen steuern. Stop funktioniert jederzeit.

**Acceptance Scenarios**:

1. **Given** der Adapter läuft, **When** der Benutzer „Leinwand runter" wählt, **Then**
   fährt der Shelly 2PM Plus die Leinwand abwärts.
2. **Given** die Leinwand fährt, **When** der Benutzer „Stop" wählt, **Then** stoppt die
   Leinwand sofort.
3. **Given** der Adapter läuft, **When** der Benutzer „Leinwand hoch" wählt, **Then**
   fährt die Leinwand aufwärts.
4. **Given** der Adapter läuft, **When** eine Zielposition (0–100 %) angegeben wird,
   **Then** fährt die Leinwand in die gewünschte Position.

---

### User Story 5 - Admin-Oberfläche für Shelly-Konfiguration (Priority: P2)

Ein Techniker möchte die IP-Adressen der Shellies und weitere Geräteparameter ohne
Neuflashen der Firmware ändern können — über eine passwortgeschützte Admin-Seite im
Webbrowser.

**Why this priority**: Ohne konfigurierbare IPs ist das System nicht auf andere
Netzwerkumgebungen übertragbar.

**Independent Test**: Über die Admin-Seite können IP-Adresse von Shelly 1 Plus und
Shelly 2PM Plus geändert und gespeichert werden. Nach dem Speichern verwendet der Adapter
die neuen Adressen ohne Neustart der Firmware.

**Acceptance Scenarios**:

1. **Given** der Adapter läuft, **When** ein Nutzer `/admin` im Browser öffnet und das
   Admin-Passwort eingibt, **Then** erscheint die Konfigurationsseite mit den aktuellen
   Shelly-IP-Adressen.
2. **Given** die Admin-Seite ist geöffnet, **When** der Nutzer eine neue IP einträgt und
   speichert, **Then** speichert der Adapter die neue Adresse dauerhaft und verwendet sie
   ab sofort.
3. **Given** jemand ohne Passwort `/admin` aufruft, **Then** wird der Zugriff verweigert.

---

### Edge Cases

- Was passiert, wenn ein Shelly während der Automatisierung nicht erreichbar ist
  (z.B. Stromausfall, Netzwerkfehler)?
- Was passiert, wenn die Leinwand bereits in der Endposition ist und der entsprechende
  Befehl erneut gesendet wird?
- Was passiert, wenn „Präsentation starten" ausgelöst wird, bevor die vorherige
  Sequenz abgeschlossen ist (gleichzeitige Aufrufe)?
- Was passiert, wenn die Shelly-IP falsch konfiguriert ist (ungültige Adresse, kein
  Shelly an dieser IP)?
- Was passiert, wenn der Beamer-RS232-Timeout auftritt während die Shelly-Befehle
  bereits ausgeführt wurden (teilweise Ausführung)?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Das System MUSS den Shelly 1 Plus (Deckenlicht) über seine REST-Schnittstelle
  ein- und ausschalten können.
- **FR-002**: Das System MUSS den Shelly 2PM Plus (Leinwand) über seine REST-Schnittstelle
  aufwärts, abwärts und stoppen können.
- **FR-003**: Das System MUSS den Shelly 2PM Plus auf eine Zielposition (0–100 %)
  fahren können.
- **FR-004**: Das System MUSS eine Automatisierungsfunktion „Präsentation starten" bereitstellen,
  die in folgender Reihenfolge ausführt: (1) Leinwand runter, (2) Licht aus, (3) Beamer ein,
  (4) HDMI-Eingang wählen.
- **FR-005**: Das System MUSS eine Automatisierungsfunktion „Präsentation beenden"
  bereitstellen: (1) Leinwand hoch, (2) Licht ein, (3) Beamer aus.
- **FR-006**: Beide Automatisierungsfunktionen MÜSSEN über REST API aufrufbar sein.
- **FR-007**: Beide Automatisierungsfunktionen MÜSSEN über MQTT aufrufbar sein
  (Topics: `beamer/cmnd/scene/start` und `beamer/cmnd/scene/stop`).
- **FR-008**: Beide Automatisierungsfunktionen MÜSSEN als Buttons in der Web-Oberfläche
  sichtbar und auslösbar sein.
- **FR-009**: Das System MUSS bei Teilfehlern (einzelnes Gerät nicht erreichbar) die
  übrigen Aktionen einer Sequenz trotzdem ausführen und einen Statusbericht zurückgeben.
- **FR-010**: Das System MUSS die IP-Adressen beider Shellies dauerhaft in der Gerätekonfiguration
  speichern (übersteht Neustart).
- **FR-011**: Die Admin-Oberfläche MUSS per Passwort geschützt sein und die Konfiguration
  beider Shelly-IPs sowie weiterer Geräteparameter ermöglichen.
- **FR-012**: Änderungen in der Admin-Oberfläche MÜSSEN ohne Neuflashen der Firmware
  sofort wirksam sein.
- **FR-013**: Das System MUSS den aktuellen Schaltzustand des Deckenlichts und den
  Fahrzustand der Leinwand abfragen und zurückgeben können.

### Key Entities

- **ShellySwitchDevice**: Shelly 1 Mini Gen3 — IP-Adresse, Name, Schaltzustand (an/aus), erreichbar (bool)
- **ShellyRollerDevice**: Shelly 2PM Gen3 — IP-Adresse, Name, Fahrzustand (moving-up/moving-down/stopped), Position (0–100 %), erreichbar (bool)
- **SceneResult**: Ergebnis einer Automatisierungssequenz — Liste von Schritt-Ergebnissen (Gerät, Aktion, Erfolg/Fehler)
- **AdminConfig**: Admin-Passwort (Hash), Shelly-IPs, weitere konfigurierbare Parameter

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Die vollständige „Präsentation starten"-Sequenz (alle 4 Schritte) wird
  innerhalb von 15 Sekunden nach Auslösung abgeschlossen, wenn alle Geräte erreichbar sind.
- **SC-002**: Die vollständige „Präsentation beenden"-Sequenz (3 Schritte) wird innerhalb
  von 15 Sekunden abgeschlossen.
- **SC-003**: Bei Ausfall eines einzelnen Geräts werden 100 % der verbleibenden Sequenzschritte
  trotzdem ausgeführt (keine Unterbrechung der Sequenz bei Teilfehlern).
- **SC-004**: 100 % der Automatisierungsfunktionen sind über alle drei Kanäle
  (Web-Button, REST, MQTT) auslösbar.
- **SC-005**: IP-Konfiguration über die Admin-Oberfläche ist in unter 2 Minuten
  abgeschlossen und ohne Neustart wirksam.
- **SC-006**: Die manuelle Einzel-Steuerung von Licht und Leinwand ist unabhängig von
  den Automatisierungen nutzbar.

## Assumptions

- Die Shellies sind im gleichen Netzwerksegment wie der ESP32-Adapter erreichbar (LAN/WLAN).
- Der Shelly 1 Mini Gen3 wird über seine integrierte HTTP-RPC-API angesprochen (Shelly Gen3 API, identisch mit Gen2 RPC-Methoden).
- Der Shelly 2PM Gen3 wird ebenfalls über seine HTTP-RPC-API angesprochen (Shelly Gen3 Roller/Cover API).
- Das Admin-Passwort ist einfach (kein Benutzermanagement, ein gemeinsames Passwort).
- Die Sequenz-Schritte werden sequenziell ausgeführt (nicht parallel), um mechanische
  Konflikte zu vermeiden (z.B. Leinwand fährt, während Licht noch an ist).
- Die Leinwand-Position (0 % = vollständig eingerollt, 100 % = vollständig ausgerollt)
  entspricht der Shelly-2PM-Konvention.
- Der MQTT-Topic-Präfix (`beamer`) ist derselbe wie in Feature 001 (aus NVS-Konfiguration).
- Es gibt keine Timeout-Wartezeit zwischen den Sequenzschritten — der nächste Schritt
  startet, sobald der aktuelle Befehl an das Gerät abgeschickt wurde (Fire-and-forget
  pro Schritt, außer beim Beamer RS232 mit ACK-Wartezeit).
