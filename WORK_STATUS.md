# Arbeitsstand und verbleibende Aufgaben

Ziel ist **eine native App mit US 1.0 und US 1.1** sowie
Englisch, Französisch und Deutsch direkt im ursprünglichen Spielmenü. PAL ist
zurückgestellt. „Lokal geprüft“ ist keine Konsolenabnahme.

## Aktueller Stand

Der neue US-Doppelbuild ist nativ kompiliert und verlinkt; sein FSELF hat gültige
Integrität. Beide Payloads und der Sprachadapter sind im Ergebnis enthalten.
Die PS5 ist nach Nutzerentscheidung ausgeschaltet. Es wird ausschließlich
offline gearbeitet; dieser neue Sprachbuild ist dort weder installiert noch
getestet. Die eigenen Dumps und früheren Rückfallstände bleiben privat erhalten. Der
aktuelle 34-Dateien-Kandidat behält Icon und Hauptmenümotiv bytegleich, verwendet
auf aktuellen Nutzerwunsch dasselbe Bild auch beim Launch und entfernt den zusätzlichen Seitenleistenstreifen.

| Aufgabe | Stand | Noch erforderlicher Abschlussnachweis |
|---|---|---|
| DKR-R 1.0.6 / US-Doppelbuild | Quellen gepinnt, beide US-Payloads neu erzeugt, nativer Build/Link und Integrität lokal bestanden | US 1.0 und US 1.1 tatsächlich spielen |
| Versionsauswahl | Touchpad → Play → Game version; Auswahl wirkt beim nächsten Start, getrennte vorhandene Profile; tatsächlicher Adapter lokal geprüft | Controllerwahl, reguläres Beenden und Neustart beider Revisionen |
| Englisch / Französisch / Deutsch | Originale Options → Language für beide US-Versionen erweitert; echter generierter Menü-/Getter-/Settercode lokal geprüft, normale Text-/EEPROM-Aufrufe bleiben zuständig | Deutsche Texte/Umlaute/Dialoge und dauerhafte Spracheinstellung auf PS5 |
| Startbildkorrektur | Startoverlay nach erstem fertigem Frame ausblenden; früherer US-1.0-Lauf vom Nutzer mit Bild und normalem Ton bestätigt | Neuer Doppelbuild, Fokuswechsel und zweiter Start |
| Schriften / Save-Namen | OFL-Schriften lokal geprüft; Play/Online und Adventure-Save-Namen vom Nutzer sichtbar bestätigt | Weitere Seiten und deutscher Spieltext |
| Controlleroberfläche | Einspaltige Felder, Tastenhinweise, Kreis zurück; tatsächliche ImGui-Navigation geprüft; neuer Play-Inhalt gebaut, Zusatzstreifen entfernt; tatsächliche Play-Navigation geprüft | Neueste Play-Seite ohne Artefakt am Fernseher |
| Tracks / Wasser | Vorschau sichtbar, kurzer Wasserflash nur beim Einstieg; Adventure stabil | Ursache mit derselben Vorschau kalt/warm messen |
| Mikroruckler | Weiterhin vom Nutzer berichtet; lokale Auswertung geprüft, Zusatzmessung standardmäßig aus | Gültiger Spielmesslauf, einzelne belegte Korrekturen vergleichen |
| Adventure-Intro | Ursache offen | Frischen Spielstart und vorhandene Saves vergleichen |
| Spielstände / Controller Pak | Metadatenadapter lokal geprüft; früherer Start mit vorhandenen Dateien gelungen; Ordnernamen bleiben erhalten | Schreiben, normal beenden, neu starten, tatsächlich laden |
| Rumble | Tatsächlicher SDL-Backend-Code mit modelliertem Transport geprüft | Physische DualSense-Vibration und Stopp |
| Icon / Hintergrund | Icon bytegleich erhalten, identisches Motiv als Hauptmenü-/Startbild, bestätigtes Ausblenden erhalten; Registrierungsprüfung vorbereitet | Hintergrund beim Anwählen im PS5-Hauptmenü; Startposter ist kein Ersatz |
| GitHub-Quellpaket | ROM-freie Positivliste, Lizenzhinweise, Eigenbauanleitung und Host-Prüfungen | Privates Repository DKR-R-PS5 angelegt; vollständigen geprüften Quellstand und CI übernehmen, anschließend Hardwareabnahme; keine Spiel-Binary hochladen |
| PAL / EUR | Private passende Decomp-Builds und Quellenadressprüfung erhalten; native Integration zurückgestellt | Kein aktueller Arbeitsauftrag |

## Letzter Konsolenstand

Der zuvor akzeptierte US-1.0-Build mit Startbildfix ist wiederhergestellt:
SHA256 `a878f66d9dd1d851d5bd974e8e8185f6c2c5b87e8af5a2c4c8df641bf55cfdad`.
Die Zusatzmessung ist ausgeschaltet. Danach wies der Loader sowohl einen neueren
Doppelbuild als auch diesen identischen Rückfallstand mit „Can't start game or
app“ vor einem neuen Spielprotokoll ab, auch beim manuellen Start. Das grenzt
den Doppelbuild nicht als Ursache ein; die Startursache ist unbestätigt.
Ein Neustart wurde vorgeschlagen und zunächst abgelehnt; anschließend schaltete
der Nutzer die Konsole selbst aus. Keine weiteren Konsolenaktionen sind erfolgt.
Der frühere unqualifizierte Systemstatus-Payload bleibt gesperrt und wird weder
wieder ausgeführt noch veröffentlicht.

## Nächste spätere Konsolenabnahme

1. In einer ausdrücklich wieder aufgenommenen Konsolensitzung zuerst den
   unveränderten Rückfallstand und den regulären Loader-Startweg prüfen.
2. Erst nach bestätigtem Ende der eigenen App den neuen Doppelbuild über den
   geprüften eigenen Updatepfad installieren. Keine fremden Titel beenden.
3. Beide US-Versionen mit ihren getrennten vorhandenen Profilen testen; Auswahl
   und Neustart müssen nachvollziehbar zur gewählten Revision führen.
4. Im ursprünglichen Optionsmenü alle drei Sprachen auswählen, Texte prüfen und
   Deutsch nach normalem Speichern/Beenden/Neustart wieder laden.
5. Neueste Play-Seite ohne Zusatzstreifen, Controllerbedienung und Save-/Pak-Persistenz
   prüfen. Vorhandene Daten erhalten.
6. Danach Rumble, Adventure-Intro, Hauptmenühintergrund sowie gezielte Wasser-/
   Rucklervergleiche abnehmen. Nur eine Vergleichsoption gleichzeitig ändern.

Details: [Prüfbelege](VERIFICATION.md), [Bild/Timing](RENDER_ISSUES.md),
[Konsolenfunktionen](CONSOLE_FEATURES.md), [Eigenbau](GAME_BUILD.md),
[Veröffentlichung](PUBLICATION.md), [zurückgestellte PAL-Vorarbeiten](PAL_PORT.md).

Hintergrund/Launch-Abgrenzung und sicherer eigener Updateweg:
[TITLE_PRESENTATION.md](TITLE_PRESENTATION.md). Die Source-Auswertung erklärt
auch die ursprünglichen Adventure-Intro-Bedingungen; bestehende Saves werden
nicht für einen erzwungenen Intro-Test zurückgesetzt.

## Private Quell-Beta und vereinfachter Updateweg

[PRIVATE_BETA.md](PRIVATE_BETA.md) bündelt Eigenbau, Grenzen und Betatest-Gates.
Ein privates Update-/Rollback-Paar ist nativ gebaut und mit Manifesten eingefroren.
Der tatsächliche kombinierte C-Helfer besteht lokale Sanitizer-Tests für Vorprüfung,
Update, Wiederholung, Teilzustandsfortsetzung und exakte Rückkehr zum Vorgänger.
Der Sender prüft beide Dateien vor einem ausdrücklich angeforderten Versand und
verlangt den passenden Abschlussmarker. Er verbindet sich standardmäßig nicht.
Diese Helfer sind noch nicht auf der Konsole ausgeführt. Der Dateiaustausch ist
pro Datei atomar; Stromausfall oder vollständige Bundle-Atomarität sind nicht geprüft.

Der neutrale Audiopfad vermeidet nun unnötige Rundung/Fließkommaarbeit und wurde
mit dem echten ursprünglichen Mixer über alle int16-Werte verglichen. Der
kontrollierte RADV-Diskcache-Vergleich ist über `shader_cache=1` vorbereitet,
standardmäßig aber aus. Ohne Konsolenvergleich wird keine Rucklerkorrektur behauptet.
Der Quellrelease-Prüfer und Download sowie der explizite lokale Updatebuild sind
umgesetzt; automatische Installation im laufenden Spiel ist kein Beta-Feature.

## Optionales Controllerprofil, 2026-10-14

Ein separater nächster Teststand enthält pro Spieler R2/L2-Gas/Bremse mit L1-Items,
klassische Belegung und Rückgängig, PlayStation-Tastennamen und vertikale
Belegungsfelder. Die tatsächlichen Eingabe-, Settings-, Capture- und
ImGui-Navigationstests sowie nativer Build/Link sind lokal geprüft. Der bisher
gefrorene Beta.1-Kandidat und seine Rückfallartefakte bleiben erhalten. Keine
Konsolenverbindung/Installation; physische Eingaben und Speichern/Wiederladen auf
PS5 stehen noch aus. Adaptive Trigger, detaillierte Haptik und native Gyro-Samples
sind weiterhin Vorhaben mit den dokumentierten Backend-Grenzen.
