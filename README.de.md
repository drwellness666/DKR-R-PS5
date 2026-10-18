# DKR-R PS5 🏁

![Diddy Kong Racing Recompiled — Grafik des Originalprojekts](https://github.com/user-attachments/assets/c913bee2-029c-4e29-9921-0f6abe7a3bf1)

**Diddy Kong Racing Recompiled, nativ auf PlayStation 5.**
Ein inoffizieller PS5-Port von [DKR-R 1.0.6](https://github.com/ThatGuyMcd/DKR-R). Die originale Rekompilierung stammt von **ThatGuyMcd und den DKR-R-Mitwirkenden**.

[📦 Downloads](https://github.com/drwellness666/DKR-R-PS5/releases) · [🇬🇧 English](README.md) · [📖 Hilfe](docs/README.md) · [❤️ Mitwirkende](CREDITS.md) · [🐛 Fehler melden](https://github.com/drwellness666/DKR-R-PS5/issues)

**Ein kostenloses Open-Source-Hobbyprojekt für die Community.** Der Quellcode der PS5-Integration, des Installers, Updaters und der Bauwerkzeuge liegt unter [GPL-3.0-or-later](LICENSE) bei.

**Version 1.0 ist in Vorbereitung.** Die endgültigen Installer- und Update-Downloads sind noch nicht verfügbar.

## 🎮 Für die PS5 angepasst

- Start vom PS5-Hauptmenü mit dem originalen DKR-R-Icon und Wolkenhintergrund.
- Zwischen **US 1.0 und US 1.1 / Rev A** wechseln. Einstellungen und Texturpakete werden gemeinsam verwendet; Spielstände bleiben getrennt.
- Klassische Steuerung und controllerfreundliche Menüs mit einer Grafik der aktuellen Tastenbelegung.
- Optionale **DualSense-Ballonfarben, Gyrosteuerung, adaptive L2-Effekte und Controller-Töne** unter **Controls → PS5**.
- Bis zu **4K**, optionales **HDR10** und experimentelle **120 Hz / VRR**. [Bildeinstellungen](docs/DISPLAY.md).
- Einmal installieren und auf der Konsole spielen. Spätere Updates lassen sich im Spiel installieren.

Modern/Accurate-Darstellung, Texturpakete, Mods, Online-Funktionen und englische/französische/deutsche Spieltexte stammen aus DKR-R. Das Einstellungsmenü ist Englisch. Hinweise zur Kompatibilität stehen in den [bekannten Fehlern](docs/KNOWN_ISSUES.md).

## 📦 Installieren

Benötigt werden eine PS5 mit eingerichteter **nativer Homebrew**, **ShadowMountPlus**, ein **ELF-Empfänger** und eigene saubere **US-ROM-Dumps 1.0 und 1.1**. PAL-ROMs werden nicht unterstützt.

1. **DKRR-Installer.elf** aus dem passenden Release herunterladen.
2. Beide ROM-Dateien auf einen USB-Stick in den Ordner **DKRR** direkt im Hauptverzeichnis legen. Die Dateinamen sind egal.
3. Stick anschließen, DKR-R schließen und den Installer mit dem gewohnten Windows- oder Linux-Sender an den ELF-Empfänger der PS5 senden.
4. Auf die Abschlussmeldung und die Erkennung durch ShadowMountPlus warten. Anschließend **DKR-R** über sein PS5-Icon starten.

**Eine Kompilation ist nicht nötig.** Nach der Installation brauchen das Spiel und seine Internet-Updates weder PC noch USB-Stick noch laufenden ELF-Empfänger. Nach einem Konsolenneustart bleibt die übliche Homebrew-Einrichtung erforderlich.

[Installationshilfe](docs/INSTALLER.md) · [Updates](docs/UPDATES.md) · [Steuerung und Sprachen](docs/CONTROLS.md)

## 🎨 Optionales Texturpaket

**DKR REMASTERED von sr.gu** ist eine separate Empfehlung. Bezugsquelle ist der [offizielle DKR-R-Discord](https://discord.com/invite/AMWfXdBjNP). Unter **Textures** importieren und **Modern** wählen. Version 0.2.7 wurde auf beiden US-Revisionen verwendet. Das Paket ist hier nicht enthalten. [Paket-Hilfe](docs/CONTROLS.md#texture-packs-and-mods).

## ⚠️ Bekannte Fehler

- Beim Einstieg in eine **Tracks-Vorschau** kann das Wasser kurz aufblitzen.
- Gelegentliche **Mikroruckler**, besonders bei aufwendigen Grafikeinstellungen oder Texturpaketen.
- HDR, der neueste Installer/Updater und neuere Controller-Funktionen brauchen noch ihre abschließenden PS5-Tests. Mods, Online-Spiel und andere Homebrew-Setups sind nur begrenzt geprüft.

[Kompatibilität und Funktionsstand](docs/KNOWN_ISSUES.md) · [Geplante Verbesserungen](docs/ROADMAP.md)

## ❤️ Mitwirkende und Lizenzen

Dieser Port baut auf **[ThatGuyMcd / DKR-R](https://github.com/ThatGuyMcd/DKR-R)**, **[DavidSM64s Dekompilierung](https://github.com/DavidSM64/Diddy-Kong-Racing)**, **[N64Recomp](https://github.com/N64Recomp)**, **[RT64](https://github.com/rt64/rt64)** und der **öffentlichen PS5-Homebrew-Community** auf. Das originale Icon stammt von **[POOTERMAN](https://www.deviantart.com/pooterman)**.

**[Alle Credits und Projektlinks →](CREDITS.md)** · [Originale Lizenzen und Hinweise](docs/LICENSES.md)

**Mit KI-Unterstützung entwickelt / Vibe Coding:** Dieser Hinweis bezieht sich auf unsere PS5-Portierung, nicht auf die Urheberschaft des ursprünglichen DKR-R oder seiner Abhängigkeiten.

Abhängigkeiten, Schriftarten und Grafiken behalten ihre eigenen Bedingungen. Eigene, rechtmäßig nutzbare ROMs sind erforderlich. Geteilte Downloads enthalten keine ROMs, extrahierten Controller-Geräusche oder Texturpakete. Dieses Fanprojekt ist nicht mit Nintendo, Rare, Microsoft oder Sony verbunden und wird von ihnen nicht unterstützt.

Selbst kompilieren oder mitarbeiten? [Entwicklerübersicht](docs/DEVELOPMENT.md).
