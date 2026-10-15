# DKR-R für PS5 — bauen, installieren und benutzen

Diese experimentelle Quell-Beta basiert auf DKR-R 1.0.6, Commit
`c3eda4d368afbc729ed4a71312cce38d27f87c37`. Du baust das Spiel auf einem
Linux-PC mit deinen eigenen rechtmäßig nutzbaren US-ROM-Dumps und überträgst
den privaten Build auf deine bereits für native Homebrew eingerichtete PS5.
Das Projekt installiert keinen Jailbreak und enthält kein fertiges Spiel.

## Was enthalten ist und was funktioniert

Eine App kann US 1.0 und US 1.1 enthalten. Im ursprünglichen Spiel-Optionsmenü
stehen Englisch, Französisch und Deutsch zur Verfügung. Die Recompiled-
Oberfläche bleibt Englisch. PAL-ROMs werden nicht benötigt oder unterstützt.

Der aktuelle Test vom 15.–16. Oktober 2026 bestätigt normalen Spielton/Bild und die
korrigierte Menügröße bei 4K. Klassische Steuerung, Vibration, Sprachwahl, Save/Reload auf
beiden Versionen und übernommene Einstellungen nach Versionswechsel sind
bestätigt. US 1.1 lädt das Remastered-Paket aus der gemeinsamen Bibliothek. Weitere aktuelle Ergebnisse und offene Fehler
stehen in [Testumfang](VALIDATION.md). **4K/119,88 Hz mit VRR Automatisch**
und ausgeschalteter Zusatzoption für nicht unterstützte Spiele sind bestätigt:
Die PS5 meldet VRR 48–120 und ALLM, am LG C1 wurden normales Bild/Ton und aktives
VRR bestätigt. Ballonfarben, Rückkehr zur normalen Controllerbeleuchtung und
bündige Versionskarten sind ebenfalls bestätigt. 120 Hz bleiben für andere
Kombinationen und Einstellungen experimentell; das garantiert keine 120 gerenderten
FPS in jeder Szene. Ein kurzes Wasserflackern in der Tracks-Vorschau und
Mikroruckler bleiben gemeldete Fehler.

Geprüfte Grundlage: PS5 13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite
(laut Nutzer ungefähr 1.11) und ShadowMountPlus (zuletzt protokolliert:
1.7-beta5fix1), ohne etaHEN. Das ist keine Zusage für jede Firmware/Kombination.

## 1. Voraussetzungen auf dem PC

Linux x86-64 und Bash. Der geprüfte Rechner verwendet Clang/LLD 23.1.1,
CMake 3.31.6 und Python 3.14. Python muss mindestens 3.12 sein. Benötigt werden
außerdem Make, Git, util-linux mit `flock`, curl/wget, tar/unzip, patch,
pkg-config, PCRE2-Entwicklungsdateien, glslangValidator, ImageMagick sowie
LLVM-/SPIR-V-Entwicklungswerkzeuge. Hosttests benötigen SDL2-Entwicklungsdateien.
Der Build installiert keine Systempakete und erstellt eigene Python-Umgebungen.
Für Quellen und Compilerprodukte sind viele Gigabyte freier Speicher nötig.

Die ROM-Prüfung akzeptiert nur saubere 12-MiB-Dumps mit diesen kanonischen SHA-1:

| Version | SHA-1 |
|---|---|
| US 1.0 | `0cb115d8716dbbc2922fda38e533b9fe63bb9670` |
| US 1.1 / Rev A | `6d96743d46f8c0cd0edb0ec5600b003c89b93755` |

Die Byte-Reihenfolge wird anhand des Inhalts erkannt; die Dateiendung bestimmt
nicht die Version. Deine Originaldatei wird nur gelesen. Ein Doppelbuild
benötigt beide Dumps. Wer nur einen besitzt, kann den Einzelbuild unten nutzen.

## 2. Quell-Beta bauen

Den Quell-ZIP entpacken oder das Repository klonen und im Projektordner eine
Bash öffnen. Alle folgenden Befehle laufen auf dem PC:

```bash
export DKR_PS5_WORK="$PWD/work/lowmem"
bash build.sh
export DKR_SDL_WORK="$DKR_PS5_WORK"
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
export DKR_RT64_WORK="$PWD/work/rt64"
bash build_rt64.sh
export DKR_GAME_WORK="$PWD/work/game"
unset DKR_ROM
export DKR_ROM_V77="/absoluter/Pfad/zum/eigenen-US-1.0-Dump.z64"
export DKR_ROM_V80="/absoluter/Pfad/zum/eigenen-US-1.1-Dump.z64"
bash build_integrated.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
python3 tools/build_native_game_installer.py   "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
python3 tools/build_owned_rom_installer.py   "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
bash tools/build_save_metadata.sh "$DKR_SDL_WORK/sdk/ps5-payload-sdk"
```

Beide ROM-Pfade durch die eigenen absoluten Pfade ersetzen. Der erste Build
lädt die gehashten öffentlichen Quellen und kann länger dauern. Der Builder
rekonstruiert die passenden Revisionen, prüft deren ROM-Hash und führt die
reguläre Upstream-Patch-Pipeline lokal aus. Es wird nichts hochgeladen oder
automatisch auf der PS5 gestartet. Bei einem Fehler an dieser Stelle stoppen
und die konkrete Fehlermeldung prüfen; erzeugte CPU-Quellen nicht manuell ändern.

`DKR_PS5_JOBS` und `DKR_GPU_JOBS` begrenzen die parallelen Jobs, standardmäßig vier.
`CMAKE_BIN` kann ein installiertes CMake auswählen.

**Nur einen Dump vorhanden:** Statt der ROM-Auswahl und `build_integrated.sh`:

```bash
unset DKR_ROM_V77 DKR_ROM_V80
export DKR_ROM="/absoluter/Pfad/zum/eigenen-unterstuetzten-US-Dump.z64"
export DKR_PS5_HIGH_REFRESH_BUILD=1 DKR_PS5_LINE_TABLES=1
bash build_game.sh
python3 tools/record_native_build.py "$DKR_GAME_WORK"
```

Die drei Installer-/Helfer-Baubefehle danach genauso ausführen. Nicht enthaltene
Versionen sind im Spiel nicht auswählbar.

## 3. Auf der eigenen PS5 installieren

Jailbreak, ShadowMountPlus und ELF-Empfänger wie gewohnt starten. DKR-R vorher
vollständig schließen. Die lokale Titel-ID `PPSA94678` und der Ordner
`/data/homebrew/PPSA94678` dürfen keiner anderen App gehören. Der Installer
verweigert abweichende vorhandene Dateien. Bestehende DKR-R-Installationen
werden über die [Update-Route](../docs/UPDATES.md) aktualisiert, nicht überschrieben.

Auf dem PC `YOUR_PS5_LAN_IP` durch die lokale IP der eigenen PS5 ersetzen:

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP   "$DKR_GAME_WORK/private/dkrr-install-game-1.0.6-preview.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP   "$DKR_GAME_WORK/private/dkrr-install-owned-rom.elf"
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-save-metadata.elf --no-wait
```

Der erste Installer enthält deinen lokal erzeugten Spielbuild, der zweite deine
ROMs. Beide bleiben privat und dürfen nicht in GitHub-Releases oder Bugberichte.
Die ROM-Dateien liegen danach unter `/data/dkrr/roms/dkr.us.v77.z64` und/oder
`dkr.us.v80.z64`; sie werden nicht im nativen Titelordner abgelegt.

ShadowMountPlus muss `/data/homebrew` scannen und den nativen Titel registrieren.
Danach **DKR-R 1.0.6 PS5 Preview** über sein PS5-Icon oder den bereits eingerichteten
nativen Launcher starten. `eboot.bin` direkt an den anonymen ELF-Empfänger zu
senden ist kein geeigneter Startweg für das GPU-Spiel.

Den kleinen `dkrr-save-metadata.elf`-Helfer nach jedem Konsolenneustart erneut
starten. Er liest nur echte Dateimetadaten der eigenen Save/Pak-Pfade über
Loopback-Port 9038. Damit bleiben die strengen Identitäts- und Aliasprüfungen
auch in der nativen nullfs-Ansicht wirksam. Er liest keine Save-Inhalte.
Eine erfolgreiche Übertragung allein bestätigt noch keinen Spielstart.

## 4. Steuerung und Versionswechsel

- Touchpad: Einstellungen öffnen/schließen.
- L1/R1: Einstellungsseiten wechseln; Steuerkreuz/Linksstick: auswählen.
- Kreuz: bestätigen; Kreis: zurück; Options: ursprüngliche Start-Taste.
- Klassisch: Kreuz gibt Gas, Quadrat bremst, L2 benutzt Items/hupt, R1 driftet.
  Das schnelle Antippen des Gases bleibt auf der digitalen Kreuz-Taste.
- **Controls → Button mapping** erlaubt eigene Belegungen und die Rückkehr zu
  Classic. Die zusätzliche R2/L2-Fahrbelegung wurde entfernt. Nur eine exakt
  passende alte Vorschau-Belegung wird auf PS5 automatisch auf Classic gesetzt;
  individuell veränderte Belegungen bleiben erhalten.

Unter **Play → Game version** US 1.0 oder US 1.1 wählen, den Titel regulär
beenden und erneut starten. Die Änderung gilt für den nächsten Start.

Beide Versionen verwenden dieselben Einstellungen für Controller, Bild, Ton,
UI/HUD, Mods und Texturen aus `/data/dkrr/profile-us-v77`. Die früheren US-1.1-
Konfigurationsdateien werden aufbewahrt. **Fortschritt und Controller Paks
bleiben getrennt pro Revision** in den bisherigen Profilordnern. Gemeinsame
Save States oder eine Konvertierung der EEPROM-Spielstände werden nicht zugesagt.

Optional unter **Controls → Device → Shared settings → DualSense light bar → Match held balloon color**:
Die Lichtleiste zeigt beim lokalen Spielen dezente Itemfarben. Blau steht für
Boost, Rot für Raketen, Grün für Fallen, Gelb für Schutzschild und Violett für
den mehrfarbigen Magnetballon. Die Itemfarben wurden auf der Konsole bestätigt.
Ohne Item, bei Pause und beim Ausschalten gibt der neue native Lichtreset die
Steuerung an das System zurück. Damit soll der normale Aus-/Dimmzustand erhalten
bleiben; die Rückkehr zum normalen Zustand wurde auf der Konsole bestätigt.
Standardmäßig aus, mit
gemeinsam übernommener Einstellung zwischen beiden Revisionen.

Standard-Vibration ist über SDL/ScePad angebunden. Adaptive Triggerwiderstände,
feine DualSense-Haptik, natives Gyro-/Lautsprecher-Backend und HDR gehören nicht
zu den implementierten Beta-Funktionen. Vorhandene Upstream-Menüpunkte sind
kein Nachweis für eine native PS5-Anbindung.

## 5. Texturen, Bild und Leistung

Eigene kompatible native RT64-Textur-ZIPs kommen nach
`/data/dkrr/profile-us-v77/texture-packs/` und werden unter Textures aktiviert.
Bereits vorhandene US-1.1-Pakete werden zusätzlich lesend gefunden. Auswahl und
Priorität gelten für beide Versionen; eine zweite große Kopie ist nicht nötig.
Rice-Pakete lassen sich zuvor mit dem offiziellen DKR-R-Desktop-Importer
konvertieren; siehe die [Upstream-Anleitung](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/TEXTURE_PACKS.md).
Das SR.GU-Paket oder andere fremde Texturen werden nicht mitgeliefert.

TV-Ausgabeauflösung und interne Renderauflösung sind verschiedene Einstellungen.
4K-Ausgabe bedeutet nicht, dass jede Spielszene intern mit nativer 4K-Auflösung
berechnet werden muss. Für einen ersten Lauf 60 Hz und eine moderate interne
Auflösung verwenden. VRR/ALLM werden von PS5 und Fernseher geregelt. VRR kann
auf Automatisch bleiben, „Auf nicht unterstützte Spiele anwenden“ bleibt aus.
120 Hz ist experimentell; die Menüanzeige beschreibt den nominellen Modus und
misst nicht die momentane HDMI-Taktung. 119,88/120 Hz wurden im VRR-Aus-Vergleich
gemeldet. Der neue Lauf mit VRR Automatisch meldet im PS5-Systemprotokoll
4K/119,88 Hz, VRR 48–120 und ALLM. Bild/Ton und aktives VRR wurden am
LG-Fernseher bestätigt. Das ist keine Zusage von120 gerenderten FPS in jeder Szene.

## 6. Updates und Fehlerberichte

[UPDATE_GUIDE.md](../docs/UPDATES.md) beschreibt geprüfte Quell-Updates, den lokalen
Neubuild und Änderungen an einer bekannten Installation mit passendem Rollback.
ROMs und Spielstände müssen dabei nicht erneut installiert werden. Ein automatischer
In-Game-Updater oder grafischer PC-Installer ist in dieser Beta nicht enthalten.

Die private Diagnose kann Klog, das eigene Journal und Bildabstände erfassen;
siehe [DIAGNOSTICS.md](../docs/DIAGNOSTICS.md). Für einen Bugbericht ROM-Revision, Quell-
Commit, Firmware/Loader und eine kurze Reproduktion angeben. Keine ROMs,
Spiel-Installer, Saves, Paks, vollständigen Konsolendumps oder Texturpakete anhängen.

## Rechte und Quellen

Unsere Adapter sind GPL-3.0-or-later; fremde Komponenten behalten ihre eigenen
Lizenzen und Hinweise. Die PS5-Oberfläche nutzt Selawik unter SIL OFL 1.1.
Titelbilder werden als gehashte öffentliche Build-Eingaben bezogen und nicht im
Quellpaket mitgeliefert. [LICENSE_REVIEW.md](../docs/LICENSES.md) und
[THIRD_PARTY.md](../THIRD_PARTY.md) dokumentieren Umfang und Grenzen. Ein ROM-freier
Port erteilt keine Rechte am Originalspiel oder seinen Grafiken/Marken.
Nintendo, Rare, Microsoft und Sony unterstützen oder billigen dieses Projekt nicht.
