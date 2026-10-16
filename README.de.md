# DKR-R für PS5

Inoffizieller nativer PS5-Port von [Diddy Kong Racing Recompiled](https://github.com/ThatGuyMcd/DKR-R), auf Basis von **DKR-R 1.0.6**.

**[Quell-Beta herunterladen](https://github.com/drwellness666/DKR-R-PS5/releases/tag/ps5-beta.2.1)** · [English](README.md) · [Bekannte Fehler](docs/KNOWN_ISSUES.md) · [Roadmap](docs/ROADMAP.md)

## Das funktioniert

- US **1.0 und 1.1 / Rev A**, einzeln oder zusammen. Auswahl unter **Touchpad → Play**.
- Originale Spieltexte auf Deutsch, Englisch und Französisch. Gemeinsame Einstellungen und Texturpaket-Auswahl; Spielstände bleiben pro Version getrennt.
- Klassische DualSense-Steuerung, Vibration und optionale Ballon-Lichtfarben mit Rückkehr zur normalen Controllerbeleuchtung.
- Passende Menüs, PS5-Icon/Hintergrund und optional **4K / 120 Hz mit VRR**. Getestet auf PS5 **13.60**, Relapse / WebKit Autoloader, kstuff-lite und ShadowMountPlus, mit LG C1.

Der Port ist eine **Beta**. Kurzes Wasserflackern in der Tracks-Vorschau und gelegentliche Mikroruckler sind noch gemeldet. [Bekannte Fehler](docs/KNOWN_ISSUES.md). 120-Hz-Ausgabe bedeutet nicht durchgehend 120 gerenderte FPS.

## Loslegen

Zum Bauen dieser Quellversion brauchst du **Linux mit x86-64** oder [**Windows mit WSL2**](docs/WINDOWS.md), **Python ab 3.12**, deinen eigenen sauberen **US-ROM-Dump von 1.0 und/oder 1.1** sowie eine PS5 mit eingerichtetem nativen Homebrew-Start, ELF-Empfänger und ShadowMountPlus. PAL-ROMs werden nicht unterstützt.

1. **DKR-R-PS5-source.zip** aus dem Release herunterladen und entpacken.
2. Im entpackten Ordner ein Terminal öffnen und starten:

   ```bash
   python3 setup.py
   ```

3. Im lokalen Assistenten: **ROM(s) auswählen → PC prüfen → bauen → lokale PS5-IP eingeben → installieren**. Fehlende PC-Voraussetzungen zeigt der Assistent vorher an. Danach `/data/homebrew` mit ShadowMountPlus scannen und DKR-R über sein PS5-Icon starten.

Der erste Build lädt öffentliche Quellen und braucht Zeit. Die ROM bleibt auf deinem PC und wird erst bei der ausdrücklich gestarteten Installation auf deine eigene PS5 übertragen. Eine abweichende bestehende Installation wird geschützt; dafür die [Update-Anleitung](docs/UPDATES.md) nutzen. Das installierte Spiel läuft nativ auf der PS5 ohne laufenden PC. Der aktuelle Stand benötigt noch einen lokalen Save-Dienst auf der Konsole: [Erklärung und direkter Lösungsweg](docs/SAVES.md). Ein [einziger ELF-Installer mit ROM-Auswahl auf der PS5](docs/INSTALLER.md) ist das nächste Installationsziel und noch keine fertige Funktion.

Die ausführlichen Befehle bleiben verfügbar: [Manuell bauen/installieren](docs/BUILD.de.md) · [Steuerung und Texturen](docs/CONTROLS.md) · [Bildausgabe](docs/DISPLAY.md).

## Fehler melden

[Issue erstellen](https://github.com/drwellness666/DKR-R-PS5/issues) und ROM-Version, Firmware/Loader, Port-Version sowie die Schritte zum Fehler nennen. Keine ROMs, Spielbuilds, Saves oder vollständigen Konsolenlogs anhängen.

## Credits und Rechte

Danke an **ThatGuyMcd und die DKR-R-Mitwirkenden**, N64Recomp/N64ModernRuntime, RT64 und die öffentlichen PS5-SDK-/Mesa-/RADV-Projekte. Unsere Adapter sind [GPL-3.0-or-later](LICENSE); fremde Komponenten behalten ihre [eigenen Lizenzen](THIRD_PARTY.md).

Das Projekt verteilt keine ROMs, extrahierten Spieldaten, erzeugten Spielcode oder fertigen Spielprogramme. Du baust lokal mit deinem eigenen rechtmäßig nutzbaren Dump; die erzeugten Spiel- und ROM-Installer bleiben privat. Nintendo, Rare, Microsoft und Sony unterstützen oder billigen dieses Projekt nicht. [Lizenzdetails](docs/LICENSES.md).
