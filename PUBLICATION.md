# Veröffentlichung als quelloffenes PS5-Projekt

## Fertige Struktur

`tools/export_public_source.py` erzeugt einen separaten, prüfbaren Source-Ordner
aus einer Positivliste. Er enthält Build-Skripte, eigene PS5-Adapter, Tests,
Patch-Dateien, Quellenmanifeste und öffentliche quelloffene Archive. DKR-R 1.0.6
wird aus seinem festgelegten öffentlichen Release-Commit geladen und gehasht.
Private Arbeitsordner und komplette Konsolenprotokolle werden nicht übernommen.
Es erfolgt kein GitHub-Upload, kein Push und keine Veröffentlichung eines Binaries.

Die Exportprüfung untersucht auch die Mitglieder der mitgelieferten Quellarchive
auf ROM-Endungen und ROM-Header. Ein neuer Source-Export besitzt keine alte
Git-Historie, in der früher versehentlich Spieldaten gelegen haben könnten.

## Nachbau und Updates

Andere Nutzer bauen den Spielkandidaten aus den öffentlichen Quellen mit einem
eigenen unterstützten US-Dump lokal. Die Hauptroute ist
ein US-1.0/1.1-Doppelbuild mit Englisch, Französisch und Deutsch direkt im
ursprünglichen Optionsmenü; Einzelbuilds bleiben möglich. PAL ist zurückgestellt. Generierte CPU-Funktionen und Spiele-Binaries bleiben private
Build-Produkte. Die reguläre Upstream-Patch-Pipeline bleibt maßgeblich. SDK-
Header, GPU-Bibliothek, Shader-Werkzeuge und Compiler sind getrennt festgelegt.

Ein neues DKR-R-Release wird als neuer exakter Commit und Archive-Hash aufgenommen.
Die Plattformpatches müssen darauf geprüft und ihre Dateihashes aktualisiert
werden. Die Pipeline wird neu erzeugt; ein Umbenennen der Versionsnummer allein
reicht nicht. Die bestätigte Renderer-Diagnose bleibt als eigener Rückfallstand.

## Bevor eine Spiel-Release angekündigt wird

Die PS5-Abnahme aus `GAME_BUILD.md` muss Start, sichtbares Spiel, Steuerung,
Ton, regulären Rennverlauf, Save/Load, Fokuswechsel, Ende und erneuten Start
belegen. Momentan lautet die korrekte Bezeichnung „native source preview“.
Andere Firmware-/Loader-Kombinationen bekommen eine eigene, nachvollziehbare
Prüfung statt einer pauschalen Kompatibilitätszusage.

Die Original-Lizenzhinweise bleiben erhalten. ROM-freie Quellen sind keine
automatische rechtliche Freigabe für das Originalspiel oder dessen abgeleiteten
Code. Das Projekt gewährt keine Nintendo/Rare-Rechte. Die Rechte und Pflichten
von MIT-/BSD-/Apache-/GPL-Abhängigkeiten sind unabhängig davon zu beachten.
Für eine spätere Binary-Release sind die vollständigen korrespondierenden
Quellen des tatsächlich verknüpften Stands bereitzustellen.

Die PS5-Interface-Schriften sind ausschließlich Selawik unter SIL OFL; die
anders lizenzierten Upstream-Schriften werden nicht verlinkt. Die Titelgrafiken
werden aus den ursprünglichen DKR-R-Projektbildern als überprüfte Build-Eingaben
bezogen; der Source-Export enthält nur Rezeptur und Quellenhinweise unter
`homebrew/DKR-R-Game/ARTWORK.md`. Es wird keine CC0-Freigabe für Upstream-Artwork
oder Marken behauptet. Lizenzprüfung und bekannte Grenzen
stehen in `LICENSE_REVIEW.md`, Tests in `VERIFICATION.md`.

## Repository vorbereiten

Ein neuer Export enthält eine englische Build-README, Bug-/Build-Fehler-Vorlagen,
Beitragsregeln, festgelegte GitHub-Action und ROM-freie Hostchecks. Die Source-
Prüfung läuft vor lokalen Builds. Die SHA256SUMS beschreiben den Exportstand;
spätere Änderungen brauchen eine neue Prüfsummenliste.

Für GitHub ausschließlich den neuen Source-Export benutzen. Den Arbeitsordner
dieses Chats wegen privater Build-Produkte und Daten nicht als Ganzes importieren.
Erst das leere Repository lokal initialisieren und die Positivliste übernehmen.
Der Export und sein ZIP sind prüfbar vorbereitet; ein GitHub-Konto und Ziel-
Repository werden nicht erfunden. Es erfolgt kein automatischer Push oder Kontakt
zum Upstream-Entwickler.

Titel-ID `PPSA94678` ist eine lokale Vorschau-ID und keine weltweit garantierte
Reservierung. Vor Installation auf einer weiteren Konsole müssen ID und Pfad
auf Kollisionen geprüft werden. Ein Installer darf keine fremden Dateien ersetzen,
keine beliebigen Anwendungen schließen und keinen Jailbreak- oder Whitelist-
Zustand verändern. Der enthaltene Installer verweigert abweichende Dateien und
startet den Titel nicht selbst.
