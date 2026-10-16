# Selbst bauen

Der vorgesehene normale Weg ist der [universelle ELF-Installer](INSTALLER.md):
Beide eigenen US-Dumps auf USB, Installer senden, PS5-Icon starten. Anwender
brauchen dafür keine Kompilation, WSL-Installation oder bisherigen PC-Build.
Der neue Installer braucht noch seine Konsolenabnahme.

Für Entwickler stehen die [Build-Befehle](BUILD.md) einschließlich ROM-freier
ELF-Paketierung bereit. Alternativ startet Windows `Start-Windows.cmd`, Linux
`bash Start-Linux.sh`. Dieser ältere Assistent benötigt beide eigenen US-Dumps
und erzeugt einen privaten ROM-haltigen Installer, der nicht weitergegeben
werden darf. Er bietet fehlende PC-Pakete vorher an. [Windows-Einrichtung](WINDOWS.md).

Auf der PS5 müssen nativer Homebrew-Start, ELF-Empfänger und ShadowMountPlus
bereits eingerichtet sein. Vor der Installation DKR-R schließen, danach
auf die Erkennung durch ShadowMountPlus warten und das PS5-Icon starten. Ein laufender PC oder ein
zusätzlicher Save-Dienst gehören nicht zum neuen nativen Kandidaten.

US 1.0 und 1.1 lassen sich unter Touchpad → Play für den nächsten Start auswählen.
Einstellungen sind gemeinsam, Fortschritt und Paks getrennt. PAL ist nicht nötig
für Deutsch/Französisch und bleibt ununterstützt.

[Manuelle Build-Befehle](BUILD.md) · [Updates](UPDATES.md) · [Speicherstände](SAVES.md)
· [Noch offene Konsolentests](VALIDATION.md).
