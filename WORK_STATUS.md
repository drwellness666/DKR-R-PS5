# Aktueller gemeinsamer Teststand — 2026-10-14

Es gibt jetzt eine gemeinsame Kandidatenlinie. [INTEGRATED_TEST.md](INTEGRATED_TEST.md)
führt sämtliche besprochenen Bereiche und die noch nötigen Abschlussnachweise
in einer Tabelle auf. Historische Einzelstände sind nur erhaltene Rückfallarchive,
keine vorgeschriebene Folge von zwanzig Tests. Die PS5 wird in dieser Offlinephase
weder kontaktiert noch verändert.

Enthalten sind DKR-R 1.0.6, beide eigenen US-Revisionspayloads, EN/FR/DE im
Originalmenü, Versionswahl für den nächsten Start, bestehende getrennte Profile,
OFL-Schriften und Save-Namen, gefüllte Play-Seite, einspaltige Controllerfelder,
reparierter Seitenleistenmarker, optionales R2/L2-Profil mit Rückgängig,
vorhandenes Rumble/Memory Pak, ursprüngliches Icon und identisches Hintergrund-/
Startmotiv mit bestätigtem Ausblendeaufruf. Der neutrale Audiopfad bleibt bitgenau.
PAL ist nach Nutzerentscheidung zurückgestellt.

Im selben Build sind 1080p/1440p/4K und 60/120 Hz optional für den nächsten Start
wählbar; ohne gespeicherte Vorgabe bleibt 1080p/60. VRR/ALLM werden weiterhin vom
PS5-/TV-System gesteuert. Native HDR-Ausgabe und erweiterte DualSense-Effekte
haben noch keinen qualifizierten Backend-Vertrag und erhalten keine Scheinoption.

Die neue [Diagnose](DIAGNOSTICS.md) verbindet eine Build-Kennung mit dem exakten
privaten eboot/ELF-Hash, einem kleinen Absturzjournal und einer ausdrücklich
aktivierbaren, auf 60 Sekunden begrenzten Messung erfolgreicher Present-Rückgaben.
Der vorhandene FPS-Overlay bleibt zuständig für Live-Anzeigen. Optional kann ein
separater Hintergrund-Logger die tatsächlich ausgegebenen Systemmeldungen an den
PC weiterreichen; keine FPS-Hooks oder fremden Prozessbeendigungen.

Native UI-/Plume-Kompilierung und gemeinsamer Link/FSELF bestehen lokal.
Signal-/Journal-/Messungs-/UI-Tests, lokaler tatsächlicher Relay-Transport und
exakte ELF-Zuordnung sind geprüft. Der Metadaten-Updater erlaubt nur den expliziten
attribute3-Übergang 0 -> 0x40040 bei unveränderten sonstigen Feldern, prüft alle
drei eigenen Kopien vor Änderungen und erhält ROMs/Profile/Spielstände. Keine
app.db-Schreibzugriffe oder unqualifizierten System-IPC-Aufrufe.

Weiter offen sind die physische Abnahme dieses kombinierten Stands, der
Hauptmenühintergrund trotz korrekt vorhandener Dateien, der kurze Wasserflash
beim Tracks-Vorschaueinstieg, gemeldete Mikroruckler, Adventure-Intro-Bedingungen,
Save/Pak-Schreiben und Wiederladen sowie native Ausgabe-/Controllerwirkung.
Es wird weder fehlerfreie Konsolenfunktion noch eine gemessene Rucklerkorrektur
behauptet. Ein Loaderfehler vor einer neuen Spielsession wird zuerst als
Start-/Registrierungsfrage untersucht, nicht automatisch durch Build-Rücknahme.

Die private Quell-Beta und ihr main-Stand bleiben erhalten. Neue Quellen bleiben
im privaten Entwurfs-PR; Eigenbau erfolgt mit eigenen Dumps, ohne Veröffentlichung
von ROMs, generierten CPU-Spieldaten, Texturpaketen oder Spiel-Binaries. Der
Quellrelease-Prüfer kann Updates herunterladen; Eigenbau und inkrementelle eigene
Installation bleiben ausdrücklich ausgelöst. Kein automatischer Binärinstaller
im laufenden Spiel. [UPDATE_GUIDE.md](UPDATE_GUIDE.md).

Historischer Konsolenstand: Nutzer spielte US 1.0 mit Bild/Ton/HUD, Save-Schriften
und Icon. Zuletzt akzeptierter Bildfix-eboot:
`a878f66d9dd1d851d5bd974e8e8185f6c2c5b87e8af5a2c4c8df641bf55cfdad`.
Später verweigerte der Loader sowohl einen neueren Doppelbuild als auch diesen
identischen Rückfallstand vor einem neuen Spielprotokoll. Ursache unbestätigt.
Der frühere unqualifizierte Systemstatus-Payload bleibt gesperrt.

## Echter Eigenbau und Update-Erhalt

Ein zusätzlicher kompletter Neuaufbau hat eine zuvor ungetestete Lücke gezeigt: Abhängigkeitspatches konnten unter einem äußeren Git-Repository still übersprungen werden. Die korrigierte Vorbereitung prüft/wendet alle 54 ausgewählten Upstream-Patches in isolierten Arbeitskopien an, liest die Änderungen zurück und erhält das äußere Repository. Der tatsächliche gesamte native DKRPortGame-Build einschließlich Link/FSELF ist anschließend bestanden. SDK/GPU/SDL und die schon geprüfte CPU-Pipeline wurden dabei ausdrücklich wiederverwendet.

Der lokale Updatebuild prüft seinen exakten Vorgänger jetzt vor der teuren Arbeit und behält dessen qualifizierte normale/120-Hz-fähige Deklaration bei. Ein widersprechender expliziter Buildflag wird abgelehnt. Der integrierte Releasebuild erzeugt private Quellzeilentabellen bei unverändertem -O3. Es gibt weiterhin nur einen aktuellen Konsolen-Testkandidaten; frühere eingefrorene Artefakte bleiben Archive.
