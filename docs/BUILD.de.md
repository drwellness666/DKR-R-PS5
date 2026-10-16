# Aus Quellen bauen und auf der PS5 installieren

[Englische Befehlsanleitung](BUILD.md) · [Windows / WSL2](WINDOWS.md)

Diese Quellversion wird auf dem PC gebaut. Danach läuft eine **native PS5-App**
auf der Konsole: ohne PC-Emulation, Streaming oder laufenden PC-Assistenten.
Der aktuelle Stand hat noch eine lokale Save-Dienst-Abhängigkeit auf der PS5.
Das ist in [Speicherstände und Start](SAVES.md) erklärt. Ein einzelner ELF-Installer
mit ROM-Auswahl direkt auf der Konsole ist das [nächste Installationsziel](INSTALLER.md).

## Der aktuelle einfache Weg

1. `DKR-R-PS5-source.zip` entpacken.
2. Im entpackten Ordner unter Linux/WSL2 `python3 setup.py` ausführen.
3. Eigene unterstützte ROMs auswählen, PC prüfen, bauen und installieren.
4. ShadowMountPlus nach `/data/homebrew` suchen lassen und das Spiel über sein
   PS5-Icon starten.

Fehlende Entwicklungswerkzeuge werden vor dem Bauen angezeigt. Der erste Build
lädt öffentliche Quellen und braucht Zeit und viele GB freien Speicher. ROMs
werden nicht auf GitHub hochgeladen. Der Assistent überträgt sie nur bei der
von dir gestarteten Installation auf deine eigene PS5.

## Voraussetzungen

Linux x86-64, Bash und Python ab 3.12. Unter Windows wird WSL2 verwendet;
Windows-Python allein ersetzt die Linux-Compiler und Build-Werkzeuge nicht.
Eine vollständige WSL2-Installation wurde für diese Beta noch nicht durchgetestet.
Die konkreten Pakete und manuellen Build-Befehle stehen in [Build](BUILD.md).

Die PS5 muss bereits native Homebrew starten können. Geprüft wurden Firmware
13.60, Relapse / WebKit Autoloader 0.6.1, kstuff-lite und ShadowMountPlus;
etaHEN ist dafür nicht erforderlich. Andere Kombinationen brauchen eigene Tests.

Benötigt wird ein eigener sauberer US-Dump von 1.0 und/oder 1.1 / Rev A,
jeweils 12 MiB. Die Byte-Reihenfolge wird erkannt; Originaldateien bleiben
unverändert. Mit beiden Dumps entsteht eine App mit Versionswahl. PAL ist
nicht unterstützt; Deutsch/Französisch/Englisch sind bereits in den US-Dumps.

## Installation und Speicherstände

Vor dem Übertragen muss DKR-R geschlossen sein. Der Erstinstaller verweigert
abweichende vorhandene App-Dateien; bestehende Installationen brauchen die
[Update-Route](UPDATES.md). ROMs und lokale Spiel-Installer nicht weitergeben.

Der Save-Dienst läuft auf der PS5 und speichert selbst keine Spielstände. Er
liefert Dateiinformationen, die im getesteten nativen Dateisystem fehlen.
Er endet beim Ausschalten der Konsole. Die bisher erfolgreichen Save-Tests
liefen mit diesem Dienst. Ein automatischer Start mit dem vorhandenen
Autoloader ist ein möglicher einmaliger Workaround; er ist bei dir noch nicht
als eingerichtet und kaltstartgeprüft bestätigt. Der fertige Installer soll
diese Abhängigkeit selbst regeln. [Details](SAVES.md).

## Im Spiel

Touchpad öffnet die Einstellungen, L1/R1 wechseln Seiten, Kreuz bestätigt und
Kreis geht zurück. Unter Play wird US 1.0 oder 1.1 für den nächsten Start gewählt.
Einstellungen und Textur-Auswahl sind gemeinsam, Fortschritt bleibt getrennt.
[Steuerung](CONTROLS.md), [Anzeige](DISPLAY.md), [bekannte Fehler](KNOWN_ISSUES.md)
und [Roadmap](ROADMAP.md) beschreiben den aktuellen Umfang.
