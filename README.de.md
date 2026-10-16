# DKR-R PS5 🏁

![Diddy Kong Racing Recompiled — Grafik des Originalprojekts](https://github.com/user-attachments/assets/c913bee2-029c-4e29-9921-0f6abe7a3bf1)

**Diddy Kong Racing Recompiled läuft nativ auf PlayStation 5.**
Ein inoffizieller PS5-Port von [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R).

[📦 Releases](https://github.com/drwellness666/DKR-R-PS5/releases) · [🇬🇧 English](README.md) · [📖 Hilfe](docs/README.md) · [🐛 Fehler melden](https://github.com/drwellness666/DKR-R-PS5/issues)

**Beta 3.2 Preview.** Kompakte Tastenbelegung und zusätzliche originale Controller-Töne.
Das Spiel wurde auf PS5 gespielt; Prüfungen der neuen Funktionen stehen im [Testbericht](docs/VALIDATION.md).

## 🎮 Was dieser Port ergänzt

| PS5-Erweiterung | Funktion |
|---|---|
| Native PS5-App | Start über ein PS5-Icon, mit passendem Hintergrund und Startgrafik. |
| Beide US-Revisionen | Unter **Touchpad → Play** zwischen **US 1.0 und US 1.1 / Rev A** für den nächsten Start wählen. |
| Gemeinsame Einrichtung | Steuerung, Bild, Ton und Texturauswahl beim Versionswechsel behalten. Fortschritt und Controller Paks bleiben getrennt. |
| Controllerfreundliche Menüs | Vollgroße Oberfläche bei 4K, klare Versionskarten, eine Controller-Grafik mit aktueller Belegung und kompakte Tastenzuordnung. Klassisches Gasgeben mit Kreuz erhält das Gas-Antippen. |
| DualSense-Extras | Optionale Ballon-Lichtfarben, Gyrosteuerung, L2-Item-Effekte und Item-, Sammel-, Münz-, Hupen- und Boostfeld-Töne über den Controller-Lautsprecher. Gemeinsam unter **Controls → PS5**. |
| TV-Ausgabe | Wahlweise 1080p, 1440p und **4K**, dazu experimentelle **120 Hz / VRR**. Match display folgt der aktiven Ausgabe; die tatsächlichen FPS hängen von Szene und Einstellungen ab. |
| Installation und Updates | Eine ROM-freie Installer-ELF, später eine Update-ELF. ROM-Import und Vorbereitung der Controller-Geräusche erfolgen auf der PS5. |
| Integriertes Speichern und Diagnose | Direkte native Speicherfunktion, lokale Fehlerprotokolle und eine Bildzeitmessung. Ein zusätzlicher Save-Helper gehört nicht zu dieser Version. |

Modern/Accurate-Darstellung, HD-Texturpakete, Mods, Online-Funktionen und
englische/französische/deutsche Spieltexte stammen aus **DKR-R**. Sie bleiben
erhalten; nicht jedes Paket, jeder Mod und jeder Online-Modus wurde auf PS5
getestet. Die Einstellungsoberfläche ist Englisch.

## 📦 Installieren

Benötigt werden eine PS5 mit eingerichteter **nativer Homebrew**, ein
**ELF-Empfänger**, **ShadowMountPlus** und beide eigenen sauberen
**US-ROM-Dumps 1.0 und 1.1**. PAL-ROMs werden nicht unterstützt. Testsitzungen
nutzten Firmware 13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite und
ShadowMountPlus mit einem LG C1.

1. **DKRR-Installer.elf** aus dem Beta-Release herunterladen.
2. Beide ROMs auf einen USB-Stick in den Ordner **DKRR** direkt im Hauptverzeichnis legen. Die Dateinamen sind egal.
3. Stick anschließen, DKR-R schließen und den Installer an den ELF-Empfänger senden. Auf die Abschlussmeldung warten.
4. Sobald ShadowMountPlus die App erkannt hat, **DKR-R** über das PS5-Icon starten.

Nach der Installation werden PC und USB-Stick zum Spielen nicht benötigt. Die
übliche Homebrew-Einrichtung bleibt nach einem Konsolenneustart erforderlich.
Für ein Update das Spiel schließen und die neue **DKRR-Update.elf** einmal
senden. Einstellungen, Fortschritt und Texturpakete bleiben auf der Konsole.
Für diesen ELF-Weg ist keine eigene Kompilation nötig.
[Installationshilfe](docs/INSTALLER.md) · [Updates](docs/UPDATES.md).

## 🎨 Optionales Texturpaket

**DKR REMASTERED von sr.gu** ist eine optionale Empfehlung; **v0.2.7** war in
unseren PS5-Sitzungen auf beiden US-Revisionen sichtbar. Die Bezugsquelle ist der
[offizielle DKR-R-Discord](https://discord.com/invite/AMWfXdBjNP), den auch das
Originalprojekt für sein Community-Texturpaket verlinkt. Unter **Textures**
importieren und **Modern** verwenden. Das Paket wird separat zu den Bedingungen
des Autors angeboten. [Paket-Hilfe](docs/CONTROLS.md#texture-packs-and-mods).

## ⚠️ Bekannte Fehler

- Kurzes Wasserflackern beim Einstieg in eine **Tracks-Vorschau**; Adventure-Wasser wurde als stabil gemeldet.
- Gelegentliche **Mikroruckler**, besonders bei aufwendigen Grafikeinstellungen oder Texturpaketen.
- HDR ist nicht verfügbar. Details zur Kompatibilität und zu experimentellen Funktionen stehen im [Testbericht](docs/VALIDATION.md).

[Bekannte Fehler und Kompatibilität](docs/KNOWN_ISSUES.md).

## 🛠️ Selbst bauen, Mitwirkende und Lizenzen

Lieber selbst kompilieren? [Linux / Windows mit WSL2](docs/BUILD.md).
[Welche Dateien ins Repository gehören](docs/FILES.md).

Danke an **ThatGuyMcd und die DKR-R-Mitwirkenden**, N64Recomp/N64ModernRuntime,
RT64 sowie die öffentlichen PS5-SDK- und PS5-Vulkan/Mesa/RADV-Projekte. Das
Titelbild ist aus DKR-R verlinkt; dessen App-Icon wird **POOTERMAN** zugeschrieben.

Die Adapter des Ports stehen unter [GPL-3.0-or-later](LICENSE); Abhängigkeiten
behalten ihre [eigenen Lizenzen](THIRD_PARTY.md). ROMs, extrahierte Controller-
Geräusche, Spielstände und fremde Texturpakete sind in geteilten Downloads nicht
enthalten. Nintendo, Rare, Microsoft und Sony unterstützen dieses Projekt nicht
offiziell. [Lizenzdetails](docs/LICENSES.md).
