# ESPHome Stepper Blind

Externe ESPHome-Cover-Komponente für eine Schrittmotor-Jalousie. Der vorhandene
ESPHome-Stepper-Treiber bleibt bestehen und wird über `stepper_id` angebunden.
Getestete ESPHome-Version: **2026.10.0**. Hardware: ESP8266 D1 Mini mit ULN2003.

## Installation

[example.yaml](example.yaml) enthält eine vollständige, allgemeine Konfiguration.
[examples/schlafzimmer-jalousie-fenster4.yaml](examples/schlafzimmer-jalousie-fenster4.yaml)
ist die vereinfachte Fassung der ursprünglichen Geräte-YAML.
Beide verwenden die Komponente direkt aus diesem öffentlichen Repository:

```yaml
external_components:
  - source: github://mvs90/esphome-stepper-blind@v0.2.0
    components: [stepper_blind]

stepper:
  - platform: uln2003
    id: my_stepper
    pin_a: D0
    pin_b: D5
    pin_c: D6
    pin_d: D7
    max_speed: 250 steps/s
    sleep_when_done: true

cover:
  - platform: stepper_blind
    id: blinded
    name: Jalousie
    device_class: blind
    stepper_id: my_stepper
    closed_position: 10000
    endstop:
      name: Endstop
      id: hass_endstop
      entity_category: config
    speed:
      name: Geschwindigkeit
      entity_category: config
      mode: slider
      initial_value: 250 steps/s
```

Die Blöcke `endstop` und `speed` sind unabhängig voneinander optional. Die übrigen Geräteeinstellungen
(WLAN, API, OTA, Logger und Board) aus dem vollständigen Beispiel ergänzen.
`secrets.yaml.example` als `secrets.yaml` neben die Geräte-YAML kopieren und lokal
ausfüllen. Bestehenden API-Verschlüsselungsschlüssel und OTA-Zugang übernehmen.
Die Beispieldatei enthält absichtlich keinen gültigen API-Schlüssel.
`secrets.yaml` niemals veröffentlichen.

Alternativ den Ordner `components` neben die Geräte-YAML kopieren und als Quelle nutzen:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [stepper_blind]
```

## Verhalten und Konfiguration

- Öffnen, Schließen, Stopp und Prozentpositionen stehen als Cover zur Verfügung.
- **0 Schritte = offen / 100 %**, `closed_position` Schritte = geschlossen / 0 %.
- `closed_position`: ganze Zahl von −90000 bis 90000, außer 0; Standard: 10000.
- Das Vorzeichen bestimmt die Schrittrichtung. Bei −10000 entsprechen 75 % offen
  dem Ziel −2500; bei +10000 dem Ziel +2500. Zwischenwerte werden auf Schritte gerundet.
- Während einer Fahrt wird der Zustand spätestens alle 500 ms im Komponentenloop
  gemeldet, zusätzlich sofort bei Befehlen und beim erkannten Fahrtende.
- Neue Fahrbefehle ersetzen das laufende Ziel. Stopp setzt das Ziel unmittelbar auf
  den aktuellen Schritt. Beschleunigung und Motorschritte übernimmt ESPHomes Treiber.
- Jede gültige Endstop-Eingabe im Stillstand setzt **auch bei identischem Wert** die
  Schrittposition ohne Motorfahrt auf 0/offen. Das Rollo muss dafür physisch offen sein.
  0, nicht ganze Zahlen, ungültige Werte und Änderungen während der Fahrt werden abgewiesen.
- Gespeicherter Endstop und Position werden gemeinsam wiederhergestellt, ohne Startfahrt.
  Der gespeicherte Endstop hat Vorrang vor dem YAML-Wert, auch nach einem YAML-Update.

`endstop` ist eine Schrittzahl zur Konfiguration, **kein physischer Endschalter**.
Die Komponenten-ID (`id: blinded`) stabil halten: sie bestimmt den Speicherschlüssel.
Den Motor ausschließlich über dieses Cover steuern; direkte Stepper-Aktionen,
weitere Cover am gleichen Motor und externe Positionsänderungen sind nicht unterstützt.

## Optionale Geschwindigkeit (ab v0.2.0)

Mit dem `speed`-Block erhält Home Assistant einen Regler in **Schritten pro Sekunde**.
Der Bereich reicht von **1** bis zum YAML-Wert `max_speed` des über `stepper_id`
verbundenen Motors. In den Beispielen sind das **1–250 Schritte/s**. Der Regler
kann diese Obergrenze nicht überschreiten. Ungültige und nicht endliche Werte
werden abgewiesen; der bisherige Wert bleibt erhalten. Die UI-Schrittweite ist 1;
endliche Zwischenwerte innerhalb des Bereichs sind über die API ebenfalls erlaubt.

`initial_value` ist optional und akzeptiert eine Zahl oder z. B. `150 steps/s`.
Ohne Angabe beginnt die Komponente mit `stepper.max_speed`. Der Startwert muss
mindestens 1 und höchstens die konfigurierte Motorgrenze sein. Bei aktivem
`speed`-Block muss auch `stepper.max_speed` endlich und mindestens 1 sein.

Änderungen wirken über den vorhandenen Stepper-Treiber auch während einer Fahrt,
ohne das Fahrziel oder den Positionszähler zu ändern. Ein Reglereingriff im
Stillstand startet den Motor nicht. Bei einem Neustart wird erneut der YAML-Startwert
verwendet: die Reglereinstellung wird **nicht gespeichert**. Das Speicherformat
für Position und Endstop bleibt unverändert und ist mit v0.1.0 kompatibel.
Ohne `speed`-Block greift die Komponente nicht in die Geschwindigkeit ein.

Die gewählte Geschwindigkeit ist ein Sollwert beziehungsweise eine Obergrenze;
die tatsächlich erreichte Schrittfrequenz hängt auch von Beschleunigung und
Loop-Laufzeit ab. Die 250 Schritte/s sind die bisherige Konfiguration und keine
Messung der physikalischen Motorgrenze. Der ULN2003 bezeichnet den Treiber,
nicht den konkreten Motor. Höhere Werte müssen zur Motorspezifikation, Versorgung
und Last passen und an Hardware geprüft werden; Schrittverluste verfälschen die
angenommene Position. Die Beispiele erhöhen die bisherige Grenze deshalb nicht.

Beschleunigung und Verzögerung können weiterhin unter `stepper` mit
`acceleration` und `deceleration` konfiguriert werden. Die Geschwindigkeitsänderung
verwendet ESPHomes `set_max_speed` samt `on_update_speed`, wie die eingebaute
`stepper.set_speed`-Aktion. Insbesondere eine Verringerung kann beim ULN2003
unmittelbar auf den neuen Maximalwert begrenzen; eine sanfte Rampe für einen
Reglereingriff wird nicht zusätzlich implementiert.

## Migration und Referenzieren

Die bisherigen `globals`, das Template-Cover, die Template-Endstop-Zahl und die
zugehörige `on_boot`-Wiederherstellung entfernen. Andere, unabhängige Automationen
beibehalten. Die ursprünglichen „Correct Up/Down“-Buttons hatten nur Log-Ausgaben
und entfallen. Die bisher invertierte Prozentfahrt ist korrigiert.

Alte Globals werden **nicht importiert**. Vor der ersten Installation das Rollo
physisch vollständig öffnen: ohne gültigen Speicherstand wird 0/offen angenommen.
Danach den Endstop passend einstellen und die Fahrt vorsichtig prüfen.
Bei falschem Positionsbezug das Rollo physisch öffnen und im Stillstand den
Endstop erneut setzen. Ohne optionale Endstop-Zahl diese zur Neueinstellung
vorübergehend konfigurieren. Eine automatische Referenzfahrt gibt es nicht.

## Speicherung und Grenzen

Position und Endstop werden beim ersten Start, bei Fahrtende, Stopp,
Endstop-Eingabe und geordnetem Shutdown zur Speicherung vorgemerkt. Beim Shutdown
wird zuvor das Motorziel auf den aktuellen Schritt gesetzt. Unveränderte Werte
lösen keine erneute Speicherung aus; fehlgeschlagene Vormerkungen werden im
Stillstand erneut versucht. Ungültige gespeicherte Werte werden als Paar verworfen.

ESPHome puffert Flash-Schreibvorgänge; das Standardintervall beträgt eine Minute.
Die Beispiele aktivieren `esp8266.restore_from_flash`. Stromausfall während einer
Fahrt oder vor dem Flash-Schreiben kann eine veraltete Position hinterlassen.
Der gemeinsam gespeicherte Datensatz garantiert keine atomare, stromausfallsichere
Flash-Aktualisierung. Verlorene Schritte und manuelles Bewegen werden nicht erkannt.
Mit `sleep_when_done: true` liefert der Motor im Stillstand kein Haltemoment.
Abrupte Richtungswechsel können mechanisch Schritte verlieren; die reale Mechanik
muss dafür getestet werden. Die Komponente fügt keine Umkehrpause hinzu.

## Prüfstand / Entwicklung

Siehe [VALIDATION.md](VALIDATION.md) für durchgeführte Prüfungen und offene Hardwaretests.
Reproduzierbare Tests mit installiertem ESPHome 2026.10.0 und einem C++-Compiler:

```sh
python tests/run.py
python tests/test_config.py
python tests/build_examples.py
```

Die Hosttests kompilieren den echten Komponenten-Code und den echten
ESPHome-Stepper-Kern mit simulierten Cover-, Number-, Speicher- und Zeit-APIs.
AddressSanitizer und UndefinedBehaviorSanitizer prüfen diese Testausführung.
Die Firmware-Builds prüfen zusätzlich die echten ESPHome-APIs und den ULN2003-Treiber.
Diese Tests ersetzen keine Hardwaretests.

Dokumentation:
[External Components](https://esphome.io/components/external_components/),
[Stepper](https://esphome.io/components/stepper/),
[Preferences](https://esphome.io/components/esphome/#preferences-component).

## Projekt unterstützen

Wenn dir dieses Projekt hilft, kannst du seine Weiterentwicklung und Pflege
freiwillig über [GitHub Sponsors](https://github.com/sponsors/mvs90) oder
[PayPal](https://www.paypal.me/mvs90) unterstützen.
Vielen Dank für deine Unterstützung!
