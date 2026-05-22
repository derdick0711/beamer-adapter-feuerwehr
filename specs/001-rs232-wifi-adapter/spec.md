# Feature Specification: RS232-WiFi Beamer Adapter

**Feature Branch**: `001-rs232-wifi-adapter`

**Created**: 2026-05-22

**Status**: Draft

**Input**: User description: "ich möchte gerne einen rs-232 zu wlan adapter für den beamer bauen. dabei soll der beamer mit an/aus, eingang auswählen, schwarz schalten und einem status angesprochen werden. die kommunikation soll via rest api und mqtt erfolgen. der esp soll zur initialen konfig ein accespoint bereitstellen, der dann die auswahl des eigentlichen wlans bereitstellt. auch eine feste ip soll konfiguriert werden können. zum zurücksetzen des wlans soll der reset button 5s gedrückt werden. weiter soll via der ip eine webpage durch den esp32 bereitgestellt werden, der steuerung wie oben beschrieben ermöglicht."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Beamer einschalten und steuern via REST API (Priority: P1)

Ein Feuerwehrmann möchte den Beamer aus einem anderen Raum oder von einem Automationssystem
heraus einschalten, den Eingang wählen und die Projektion starten, ohne physischen Zugang
zum Beamer oder Adapter zu benötigen.

**Why this priority**: REST API ist die Kernfunktion — ohne funktionierende Beamer-Steuerung
hat der Adapter keinen Nutzen. P1 liefert sofort messbaren Betriebswert.

**Independent Test**: Der Beamer kann vollständig über HTTP-Requests (ein/aus, Eingang,
Schwarzbild, Status) gesteuert werden, ohne dass ein MQTT-Broker oder ein Browser vorhanden
sein muss.

**Acceptance Scenarios**:

1. **Given** der Adapter ist mit dem WLAN verbunden, **When** ein HTTP POST auf `/api/power`
   mit `{"state": "on"}` gesendet wird, **Then** schaltet der Beamer ein und die API
   antwortet mit HTTP 200 und dem neuen Status.
2. **Given** der Beamer ist eingeschaltet, **When** ein HTTP POST auf `/api/input` mit
   `{"input": "hdmi1"}` gesendet wird, **Then** wechselt der Beamer zum gewählten Eingang.
3. **Given** der Beamer ist eingeschaltet, **When** ein HTTP POST auf `/api/blank` mit
   `{"enabled": true}` gesendet wird, **Then** wird das Bild schwarz geschaltet.
4. **Given** der Adapter läuft, **When** ein HTTP GET auf `/api/status` gesendet wird,
   **Then** antwortet die API mit dem aktuellen Betriebsstatus des Beamers (ein/aus,
   aktiver Eingang, Schwarzbild-Status).

---

### User Story 2 - Beamer-Steuerung via MQTT (Priority: P2)

Ein Heimautomations- oder Feuerwehr-Leitsystem möchte den Beamer über MQTT-Nachrichten
steuern, um ihn in bestehende Automationsabläufe zu integrieren.

**Why this priority**: MQTT ermöglicht die Integration in Automationssysteme (z.B. Home
Assistant, Node-RED). Setzt funktionierende RS232-Kommunikation (P1) voraus.

**Independent Test**: Der Beamer kann über MQTT-Topics (publish) gesteuert und sein
Status (subscribe) empfangen werden, unabhängig von der Web-Oberfläche.

**Acceptance Scenarios**:

1. **Given** der Adapter ist mit einem MQTT-Broker verbunden, **When** auf das Topic
   `beamer/cmnd/power` der Wert `on` gepublisht wird, **Then** schaltet der Beamer
   ein und der Adapter publiziert den neuen Status auf `beamer/stat`.
2. **Given** der Adapter läuft, **When** er eine Verbindung zum MQTT-Broker herstellt,
   **Then** abonniert er automatisch alle Steuerungs-Topics und publiziert einen
   Verbindungsstatus (Last Will Testament bei Verbindungsverlust).
3. **Given** der MQTT-Broker ist vorübergehend nicht erreichbar, **When** die Verbindung
   wiederhergestellt wird, **Then** verbindet sich der Adapter automatisch neu ohne
   manuelle Intervention.

---

### User Story 3 - Web-Oberfläche für direkte Steuerung (Priority: P2)

Ein Nutzer möchte den Beamer über einen Webbrowser direkt über die IP-Adresse des Adapters
steuern, ohne zusätzliche Apps oder APIs kennen zu müssen.

**Why this priority**: Die Web-Oberfläche bietet einen sofort nutzbaren, geräteunabhängigen
Zugang für alle Nutzer ohne technische Vorkenntnisse.

**Independent Test**: Alle Beamer-Steuerfunktionen (ein/aus, Eingang, Schwarzbild, Status)
sind über einen Webbrowser auf der IP-Adresse des Adapters vollständig nutzbar.

**Acceptance Scenarios**:

1. **Given** der Adapter ist im WLAN eingebunden, **When** ein Nutzer die IP-Adresse im
   Browser öffnet, **Then** wird eine Steuerseite mit Buttons für Ein/Aus, Eingangswahl,
   Schwarzbild und Statusanzeige geladen.
2. **Given** die Web-Oberfläche ist geöffnet, **When** der Nutzer auf „Einschalten"
   klickt, **Then** sendet die Seite den Befehl und zeigt den aktualisierten Status an.
3. **Given** die Web-Oberfläche ist geöffnet, **When** der Nutzer einen Eingang auswählt,
   **Then** wechselt der Beamer zum gewählten Eingang und die Seite bestätigt die Auswahl.

---

### User Story 4 - Initiale WLAN-Konfiguration via Access Point (Priority: P1)

Ein Nutzer möchte den Adapter ohne vorherige WLAN-Konfiguration in Betrieb nehmen. Der
Adapter soll sich beim ersten Start als WLAN-Zugangspunkt anmelden, über den die WLAN-
Zugangsdaten und optional eine feste IP eingegeben werden können.

**Why this priority**: Ohne WLAN-Konfiguration ist kein Betrieb möglich. Dieser Schritt
blockiert alle anderen Stories.

**Independent Test**: Ein werksneuer (oder zurückgesetzter) Adapter öffnet einen Access
Point. Über eine Konfigurationsseite können WLAN-SSID, Passwort und optionale feste IP
eingetragen und gespeichert werden. Nach dem Neustart verbindet sich der Adapter mit dem
konfigurierten WLAN.

**Acceptance Scenarios**:

1. **Given** der Adapter hat keine gespeicherten WLAN-Daten, **When** er eingeschaltet
   wird, **Then** öffnet er einen eigenen WLAN-Access-Point mit einem erkennbaren Namen
   und eine Konfigurationsseite ist über einen Browser erreichbar.
2. **Given** die Konfigurationsseite ist geöffnet, **When** der Nutzer SSID und Passwort
   des Ziel-WLANs eingibt und speichert, **Then** verbindet sich der Adapter nach einem
   Neustart automatisch mit dem konfigurierten WLAN.
3. **Given** die Konfigurationsseite ist geöffnet, **When** der Nutzer eine feste IP-
   Adresse, Gateway und Subnetzmaske eingibt, **Then** nutzt der Adapter diese feste IP
   nach dem Neustart statt DHCP.
4. **Given** der Adapter ist konfiguriert und im Betrieb, **When** der Reset-Button
   5 Sekunden lang gedrückt wird, **Then** werden die gespeicherten WLAN-Daten gelöscht
   und der Adapter startet im Access-Point-Modus neu.

---

### Edge Cases

- Wenn der Beamer auf einen RS232-Befehl nicht antwortet (Timeout): REST gibt HTTP 503
  zurück, MQTT publiziert Fehlerstatus auf `beamer/stat`; der zuletzt bekannte
  Beamer-Status bleibt erhalten. Kein automatischer Retry.
- Wenn der AP-Konfigurationsmodus nach 5 Minuten keine gespeicherte Konfiguration
  erhält: Adapter startet neu und öffnet erneut den Access-Point-Modus.
- Was passiert, wenn das konfigurierte WLAN nicht erreichbar ist (z.B. Router ausgefallen)?
- Was passiert, wenn mehrere Clients gleichzeitig widersprüchliche Befehle senden?
- Was passiert, wenn der MQTT-Broker beim Start des Adapters noch nicht erreichbar ist?

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Das System MUSS den Beamer über RS232 ein- und ausschalten können.
- **FR-002**: Das System MUSS alle per RS232 adressierbaren Eingänge des Acer H6512BD
  unterstützen (HDMI, VGA/D-Sub, Composite, S-Video, Component). Die konkreten
  RS232-Codes werden in der Planungsphase recherchiert.
- **FR-003**: Das System MUSS das Bild des Beamers schwarz schalten und wieder freigeben
  können (Blank/Unblank).
- **FR-004**: Das System MUSS den aktuellen Status des Beamers abfragen und zurückgeben
  können (Betriebszustand, aktiver Eingang, Schwarzbild-Status). Bei RS232-Timeout MUSS
  HTTP 503 (REST) bzw. ein Fehlerstatus auf `beamer/stat` (MQTT) zurückgegeben werden;
  der zuletzt bekannte Status bleibt gespeichert. Kein automatischer Retry.
- **FR-005**: Das System MUSS alle Beamerfunktionen über eine REST API bereitstellen.
- **FR-006**: Das System MUSS alle Beamerfunktionen über MQTT publish/subscribe
  bereitstellen. Steuer-Topics: `beamer/cmnd/power`, `beamer/cmnd/input`,
  `beamer/cmnd/blank`. Status-Topic: `beamer/stat` (JSON-Payload mit aktuellem
  Betriebszustand). Last-Will-Testament auf `beamer/stat` bei Verbindungsverlust.
- **FR-007**: Das System MUSS eine im Browser aufrufbare Web-Oberfläche bereitstellen,
  die alle Steuerfunktionen abbildet und den Status anzeigt.
- **FR-008**: Das System MUSS beim ersten Start (oder nach WLAN-Reset) einen WLAN-
  Access-Point mit Konfigurationsseite bereitstellen. Wird innerhalb von 5 Minuten
  keine Konfiguration gespeichert, MUSS der Adapter neu starten und erneut im
  Access-Point-Modus erscheinen.
- **FR-009**: Die Konfigurationsseite MUSS die Auswahl eines vorhandenen WLANs (SSID-
  Scan), Passworteingabe und optionale Eingabe einer festen IP ermöglichen.
- **FR-010**: Das System MUSS die WLAN-Konfiguration nichtflüchtig speichern (übersteht
  Stromausfall und Neustart).
- **FR-011**: Das System MUSS die WLAN-Konfiguration durch 5 Sekunden langes Drücken des
  Reset-Buttons löschen und in den Access-Point-Modus zurückfallen.
- **FR-012**: Das System MUSS sich nach Verbindungsverlust zum WLAN automatisch
  reconnecten.
- **FR-013**: Das System MUSS sich nach Verbindungsverlust zum MQTT-Broker automatisch
  reconnecten.
- **FR-014**: Die MQTT-Verbindung MUSS anonym erfolgen (kein Username/Passwort).
  Der Broker MUSS anonyme Verbindungen erlauben.

### Key Entities

- **BeamerCommand**: Steuerbefehl (Typ: power/input/blank, Parameter, Zeitstempel)
- **BeamerStatus**: Aktueller Zustand (Betrieb: on/off/unknown, Eingang, Blank: ja/nein)
- **NetworkConfig**: WLAN-Zugangsdaten (SSID, Passwort, IP-Modus: DHCP/statisch,
  statische IP, Gateway, Subnetzmaske)
- **MqttConfig**: Broker-Adresse, Port, Topic-Präfix (Standard: `beamer`).
  Verbindung anonym (kein Username/Passwort). Steuer-Topics unter `{prefix}/cmnd/*`,
  Status-Topic `{prefix}/stat`.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Ein Nutzer kann den Beamer innerhalb von 3 Sekunden nach Absenden eines
  Befehls (REST oder MQTT) ein- oder ausschalten.
- **SC-002**: Die initiale WLAN-Konfiguration kann von einem technisch versierten Nutzer
  in unter 3 Minuten abgeschlossen werden.
- **SC-003**: Der Adapter verbindet sich nach einem WLAN-Ausfall ohne manuelle Eingriffe
  wieder, sobald das WLAN erneut verfügbar ist.
- **SC-004**: 100 % der dokumentierten Beamerfunktionen (ein/aus, Eingang, Schwarzbild,
  Status) sind über alle drei Kanäle (REST, MQTT, Web) erreichbar.
- **SC-005**: Ein WLAN-Reset durch 5-Sekunden-Druck des Reset-Buttons gelingt zuverlässig
  in 100 % der Versuche.
- **SC-006**: Die Web-Oberfläche ist auf einem Smartphone-Browser ohne zusätzliche
  Software nutzbar.

## Clarifications

### Session 2026-05-22

- Q: Wie soll die MQTT Topic-Struktur aufgebaut sein? → A: Tasmota-Stil — Steuerbefehle auf `beamer/cmnd/{command}`, Status auf `beamer/stat`; Topic-Präfix konfigurierbar.
- Q: Was passiert bei RS232-Timeout (kein Beamer-Response)? → A: Sofort HTTP 503 / MQTT-Fehlerstatus zurückgeben, kein Retry, letzter bekannter Status bleibt erhalten.
- Q: Was passiert, wenn der AP-Konfigurationsmodus kein Timeout hat und kein Nutzer konfiguriert? → A: Nach 5 Minuten ohne gespeicherte Konfiguration Neustart und erneut AP-Modus öffnen.
- Q: Welche Beamer-Eingänge sollen unterstützt werden? → A: Alle per RS232 adressierbaren Eingänge des H6512BD (HDMI, VGA/D-Sub, Composite, S-Video, Component).
- Q: Soll die MQTT-Verbindung Authentifizierung unterstützen? → A: Nein — Adapter verbindet sich immer anonym; Broker muss anonyme Verbindungen erlauben.

## Assumptions

- Der Acer H6512BD unterstützt RS232-Steuerung; die genauen Befehle (Baud-Rate,
  Befehlsformat) werden in der Planungsphase recherchiert.
- Als Build-System und Entwicklungsumgebung wird ausschließlich PlatformIO verwendet
  (VS Code + PlatformIO Plugin). Andere Build-Systeme (idf.py, Arduino IDE) sind
  nicht zulässig.
- Der MQTT-Broker (z.B. Mosquitto) wird extern betrieben und ist nicht Teil dieses
  Adapters.
- Die Web-Oberfläche erfordert keine Benutzerauthentifizierung (Absicherung erfolgt
  durch Netzwerksegmentierung im Feuerwehr-LAN).
- Es ist immer nur ein Beamer pro Adapter-Instanz angeschlossen.
- Der Reset-Button ist der physische BOOT/EN-Button des ESP32-S3 DevKits oder ein
  dedizierter GPIO-Pin.
- Bluetooth wird in dieser Version nicht genutzt (reserviert für zukünftige Erweiterungen).
- Die Statusabfrage liefert den tatsächlichen Beamer-Status via RS232-Abfrage; falls
  der Beamer keine Statusantwort liefert, wird der zuletzt bekannte Zustand zurückgegeben.
