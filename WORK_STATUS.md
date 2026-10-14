# Gemeinsamer PS5-Teststand — 2026-10-15

Am 14. Oktober hat der Nutzer beide US-Versionen im selben integrierten DKR-R
1.0.6-Titel gestartet und gespielt. Bild und Ton waren normal. Das originale
Icon, der Wolkenhintergrund im Hauptmenü und dasselbe Motiv beim Start wurden
bestätigt. Der Startschirm verschwindet nach der ersten fertigen Präsentation.
Ein reguläres Schließen und erneuter Start desselben Builds funktionierten.
Diese Abnahme gehört zu `integrated-ad598d532cbc05e1da8b2623`.

Der neue, ausschließlich offline gebaute Kandidat heißt
`integrated-a28af6c874bac873eec0645f`. Er enthält alle bisherigen Funktionen und
die unten beschriebenen Korrekturen. Er wurde nach dem ausdrücklichen
Ausschalten der PS5 nicht übertragen oder dort getestet. Es gibt genau einen
aktuellen neuen Testkandidaten; der bestätigte Vorgänger bleibt als Vergleich
und für eine bewusst angeforderte Notfallwiederherstellung erhalten.

- Die Recompiled-Oberfläche behält bei 1440p/4K ihre logische Größe und erhält
  den tatsächlichen Framebuffer-Skalierungsfaktor. Der bisher zu kleine
  Menübereich war ein Skalierungsfehler. Echte ImGui-Frames und Clip-/Viewport-
  Verträge sind lokal geprüft; die TV-Abnahme der Änderung steht aus.
- Die Grafiktreiberstatistik schreibt im normalen Betrieb nicht mehr alle zehn
  Sekunden unter der Submit-Sperre ins Dateiprotokoll. Bei explizitem
  `power_profile=1` bleibt sie verfügbar. Fehlerausgaben bleiben erhalten.
- Die 120-Hz-Prüfung wiederholt ihre Messung bei zunächst langsamer Ausgabe
  höchstens viermal. Fehlgeschlagene Aufrufe und der bisherige 12,5-ms-Grenzwert
  bleiben erhalten. Der letzte Konsolenlauf nahm den Modus zunächst an, maß
  jedoch 20,872 ms und stellte 59,94 Hz wieder her. Ein Übergangs- oder
  VRR-Leerlaufeinfluss ist unbestätigt; 120 Hz wird nicht als gelöst behauptet.
- Die Controllerseite erklärt die ausdrückliche Aktivierung von **R2 / L2
  driving** mit Kreuz. Die zuletzt gespeicherten Profile waren **Classic**:
  L2 war dort Z/Hupe. Eigene Belegungen werden nicht automatisch überschrieben.
  Jede US-Version behält ihre eigene Auswahl und Rückgängig-Funktion.
- Ein separat vorbereiteter kleiner Helfer kopiert das exakt geprüfte, bereits
  vorhandene eigene SR.GU-Paket vom US-1.0-Profil ins bestehende US-1.1-Profil.
  Er enthält keine Texturen. Einstellungen und Spielstände bleiben erhalten;
  abweichende Ziele, Symlinks und fremde temporäre Dateien werden abgelehnt.
  Die Konsolenkopie und Auswahl in Textures stehen noch aus.
- Der normale Installer provisioniert das eigene Diagnoseverzeichnis. Ein
  begrenzter Leser kann ein konkret benanntes Journal desselben Builds lesen,
  auch wenn der Webserver die privaten Dateien mit Modus 0600 nicht öffnen
  kann. Er wechselt nur seine eigene Benutzeridentität und schreibt nichts.

Die beiden bisherigen Minutenanzeigen meldeten jeweils sechs Pausen über
50 ms bei ungefähr 59,5 Present-Rückgaben/s. Menüzeit, Strecke und Kalt-/Warmlauf
waren nicht kontrolliert. Die Zehn-Sekunden-Protokollierung ist ein konkreter
Verdacht, keine bewiesene Ursache. Ein ruckelfreier Lauf oder ein gemessener
Leistungsgewinn wird erst nach einem passenden Vergleich behauptet.

Der vollständige native DKRPortGame-Build mit -O3, privaten Zeilentabellen,
Link und gültigem FSELF ist bestanden. Beide tatsächlichen Treiberobjekte
wurden aus den exakt geprüften gepinnten Quellen neu kompiliert. SDK/SDL und
die bereits validierte eigene CPU-Pipeline wurden ausdrücklich wiederverwendet.
Alle 54 Abhängigkeitspatches werden nach dem Anwenden zurückgeprüft; zwei neue
Treiberänderungen besitzen zusätzliche Vorher-/Nachher-Prüfsummen. Details und
aktuelle Prüfresultate stehen in [CHECKS.json](CHECKS.json).

Noch offen: TV-Abnahme der neuen Skalierung und Leistungsänderung, tatsächliche
120-Hz-/VRR-/ALLM-Wirkung, der kurze Wasserflash beim Tracks-Vorschaueinstieg,
kontrollierter Adventure-Intro-Vergleich, Sprache/Umlaute sowie vollständiges
Save/Pak-Schreiben und Wiederladen. Native HDR-Ausgabe und erweiterte
DualSense-Effekte haben keinen qualifizierten Backend-Vertrag und erhalten
keine Scheinoptionen. PAL bleibt nach Nutzerentscheidung zurückgestellt.

Quellen und Eigenbauanleitung bleiben im privaten Entwurfs-PR. Main und die
veröffentlichte Quell-Beta werden nicht ersetzt. ROMs, generierte Spielquellen,
Texturpakete, Spiel-Binaries und Konsolenprotokolle bleiben privat. Ein
Quellupdate ist möglich; Eigenbau und eigene inkrementelle Installation werden
ausdrücklich ausgelöst. [INTEGRATED_TEST.md](INTEGRATED_TEST.md),
[DIAGNOSTICS.md](DIAGNOSTICS.md), [UPDATE_GUIDE.md](UPDATE_GUIDE.md).
