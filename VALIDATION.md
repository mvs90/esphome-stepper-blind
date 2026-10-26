# Prüfstand: 26. Oktober 2026

ESPHome **2026.10.0**, Arduino-ESP8266 **3.1.2-esphome.3**, Board `d1_mini`.
Die Firmware-Builds liefen unter macOS mit dem ESPHomes nativen ESP8266-Buildsystem.
Die Zugangsdaten für alle Builds waren ausdrücklich künstliche Testwerte.

## Tatsächlich geprüft

| Prüfung | Ergebnis |
| --- | --- |
| Vollständiger D1-Mini-Build, allgemeines Beispiel, Endstop +10000 | Erfolgreich; RAM 30156 / 81920 Byte, Flash 381648 / 1044464 Byte |
| Vollständiger D1-Mini-Build, Schlafzimmer-Beispiel, Endstop −90000 | Erfolgreich; RAM 30184 / 81920 Byte, Flash 381660 / 1044464 Byte |
| API-Verschlüsselung und ESPHome-OTA mit Passwort | In beiden Firmware-Builds enthalten |
| YAML-Schema und C++-Codegenerierung | +1, −1, +90000, −90000 und +12345 akzeptiert; optionale Endstop-Zahl mit und ohne Block geprüft |
| Ungültige YAML-Endpositionen | 0, +90001, −90001 und 1.5 abgewiesen |
| Hosttests: Fahrt und Prozentpositionen | Endpositionen ±10000, ±90000, ±12345; Ziele 0, 25, 50, 75, 100 % offen |
| Hosttests: Rundung auf kleinen Fahrwegen | Endpositionen +1 und −1 |
| Hosttests: Steuerung | Stopp ohne Folgeschritt, Umkehr während Fahrt, neues Ziel auf aktuellem Schritt, Fahrtende unmittelbar vor erneutem Befehl |
| Hosttests: Initialisierung / Speicher | Erststart ohne Bewegung, erste Endposition ohne Fahrt gespeichert, Restore mit Vorrang vor YAML, unabhängige IDs, ungültige gespeicherte Werte, erneuter Versuch nach Speicherfehler, Shutdown stoppt Ziel |
| Hosttests: Endstop-Zahl | Änderung während Fahrt abgewiesen; gültige Änderung sowie gleicher Wert setzen auf offen; ungültige Eingaben werden abgewiesen und der bisherige Zahlenwert bleibt erhalten |
| Sanitizer | Hosttests ohne AddressSanitizer-/UndefinedBehaviorSanitizer-Befund |
| Öffentliche Dateien | Gegen die im Original enthaltenen Zugangsdaten geprüft; keine Originalschlüssel, Passwörter oder private WLAN-Domain enthalten |

Die Hosttests verwenden den echten Stepper-Kern aus dem installierten ESPHome und
den echten C++-Komponenten-Code. Cover-/Number-APIs, Zeit und Preference-Backend
sind simuliert. Der simulierte Speicher prüft Logik und Wiederholungen, keine realen
Flash-Schreibzyklen. Die Firmware-Builds prüfen Kompatibilität mit den tatsächlichen
ESPHome-APIs und dem ULN2003-Treiber, führen aber keine Motorbewegung aus.

## Behobene Fehler

- Position auch dann speichern, wenn direkt nach dem letzten Motorschritt ein
  neuer Befehl eintrifft oder ein neues Ziel genau dem aktuellen Schritt entspricht.
- Anfänglichen Endstop auch ohne erste Fahrt zur Speicherung vormerken.
- Fehlgeschlagene Speichervormerkung im Stillstand erneut versuchen.
- Motorziel bei geordnetem Shutdown einfrieren, bevor die Position gespeichert wird.
- Nicht endliche Cover-Ziele und nicht ganzzahlige Endstop-Eingaben abweisen.
- Vergleiche gespeicherter Schritte mit expliziten `int32_t`-Werten portabel halten.
- Das frühere Generator-Skript durch einen reinen ZIP-Packer ersetzen, damit es
  die gepflegte Implementierung nicht mehr mit einer veralteten Fassung überschreibt.

## Noch an Hardware zu prüfen

1. Vor Erstinstallation Rollo physisch vollständig öffnen. Keine alten Globals
   werden automatisch übernommen.
2. Drehrichtung und Schrittzahl vorsichtig bestimmen. Pins D0/D5/D6/D7, ULN2003,
   Versorgung und Mechanik prüfen. Maximal 250 Schritte/s in den Beispielen.
3. Öffnen, Schließen, 25/50/75 %, Stopp und unmittelbare Richtungswechsel unter
   realer Last testen. Prüfen, ob Schritte verloren gehen oder eine Umkehrpause
   benötigt wird.
4. Prüfen, dass `sleep_when_done: true` die Spulen nach Stopp/Fahrtende freigibt
   und die Mechanik ohne Haltemoment ihre Position hält.
5. Home-Assistant-Anzeige einschließlich Endstop-Ablehnung während Fahrt prüfen.
6. Endstop nur bei physisch offenem Rollo ändern. Positives und negatives Vorzeichen
   sowie erneute Eingabe desselben Werts prüfen.
7. Geordneten Neustart im Stillstand und während Fahrt prüfen. Nach Ablauf des
   Flash-Intervalls stromlos schalten und Wiederherstellung ohne Startfahrt prüfen.
8. Stromausfall während Fahrt separat prüfen: veraltete gespeicherte Positionen
   sind eine bekannte Grenze und erfordern gegebenenfalls manuelles Referenzieren.

Es wurden **keine Hardwaretests**, kein Geräte-Upload und keine reale
Home-Assistant-Integration durchgeführt. Andere ESPHome-Versionen sind nicht bestätigt.
