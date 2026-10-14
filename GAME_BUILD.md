# DKR-R 1.0.6: native PS5 preview

Stand: 13. Oktober 2026. Der Nutzer hat DKR-R 1.0.6 auf seiner PS5 tatsächlich
mit Bild, Spielton und DualSense-Steuerung gespielt. HUD-Symbole erscheinen.
Offen sind einzelne Menüs/Schriften, das Adventure-Intro, gelegentliche
Mikroruckler und die vollständige Speicher-/Neustartabnahme. Vorhandene
Controller-Pak-Dateien wurden wegen fehlerhafter nativer Dateimetadaten beim
Neustart abgewiesen. Die gezielte Anpassung hat diesen Startfehler auf Hardware
überwunden und wieder Spielausgabe erreicht. Save/Load bleibt noch zu prüfen.

## Verwendeter Stand

Upstream ist die tatsächliche [Release 1.0.6](https://github.com/ThatGuyMcd/DKR-R/releases/tag/Version1.0.6),
Commit `c3eda4d368afbc729ed4a71312cce38d27f87c37`. Der bisherige 1.0.5-Kandidat
bleibt als lokaler Rückfallstand erhalten. Der auf Hardware bestätigte RT64-Test
v9 ist weiterhin der separate Titel `PPSA94677`; das Spiel benutzt `PPSA94678`.

Alle 1.0.6-Quellen und ihre reguläre Patch Pipeline sind enthalten. Die Vorschau
aktiviert zunächst Vulkan, 2× Originalauflösung, Original-Bildrate, vier
Renderer-Arbeits-Threads und kein MSAA. Enhanced Water/RTX, der SDL3-Hilfsprozess,
WebRTC/Quick Join und der externe Legacy-Importer sind deaktiviert. Die neuen
Upstream-Funktionen sind daher nicht sämtlich auf PS5 verfügbar oder getestet.
Insbesondere die neuen Wasser-/Lavaeffekte sind keine zugesicherte PS5-Funktion.

## Öffentliche Quellen, privater Spielbuild

Öffentlich geeignet sind die Plattformadapter, Build-Skripte, Tests, Patch-Dateien,
Quellen-Hashes und Lizenzhinweise. Das Exportskript stellt diese aus einer festen
Positivliste zusammen. Es veröffentlicht nichts auf GitHub.

ROMs, extrahierte Inhalte, generierte CPU-Payloads, Spiel-Binaries, Installer,
Profile, Saves und vollständige Konsolenprotokolle bleiben privat. Das vollständige
Spiel braucht einen eigenen unterstützten US-Dump; beide Revisionen lassen sich
optional gemeinsam bauen. Die angegebenen Originaldateien werden beim Build nur gelesen. Byte-Reihenfolge wird anhand des
Headers erkannt; Dateiendungen bestimmen nicht die Revision.

Die eigene `libc.prx` wird aus dem öffentlichen Quellcode-Emitter erzeugt.
Sony-Systembibliotheken werden importiert; es werden keine Sony-Dateien aus
einem Spiel oder der Konsole kopiert. Lizenzen und die Rechte an Spielcode und
Spieldaten sind getrennt zu betrachten. ROM-freie Verteilung erteilt keine Rechte
am Originalspiel; siehe die [Upstream-Regeln](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md)
und den [Lizenzhinweis](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/runtime-recomp/COPYING-NOTICE.md).

## Vollständig selbst bauen

Linux x86-64; geprüft mit Clang/LLD 23.1.1, CMake 3.31.6. Benötigt werden zusätzlich
GNU Make, Git, util-linux (`flock`), Python mit venv/pip, curl/wget, unzip/tar, patch, pkg-config,
PCRE2-Entwicklungsdateien, glslangValidator, ImageMagick (`magick` oder `convert`) sowie LLVM-/SPIR-V-Entwicklungswerkzeuge
für Mesa. Die Adaptertests brauchen SDL2 auf dem Host. SDKs und Compiler-Werkzeuge
stammen ausschließlich aus den festgelegten öffentlichen Quellen.

In einem eigenen Kit-Ordner mit ausreichend freiem Speicher:

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
bash build_game.sh
python3 tools/build_native_game_installer.py \
  "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
python3 tools/build_owned_rom_installer.py \
  "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK"
bash tools/build_save_metadata.sh "$DKR_SDL_WORK/sdk/ps5-payload-sdk"
```

Die Hauptroute baut beide US-Versionen in einer App. Beide ROM-Pfade ersetzen.
Für einen Einzelbuild genügt weiterhin `DKR_ROM` statt der beiden Versionsvariablen. `DKR_GPU_JOBS` begrenzt Parallelität (Standard 4).
`CMAKE_BIN` kann auf ein geeignetes CMake zeigen. Es werden keine Systempakete
installiert. Der Build verwendet frische, reservierte Quell- und Arbeitsordner;
fremde oder veränderte Quellen werden abgelehnt. Download-Archive werden geprüft.

Der Spielbuild erzeugt den zur Auswahl passenden Decomp-ELF, prüft den rekonstruierten ROM
gegen seine Original-SHA1 und komponiert die reguläre Upstream-Pipeline neu. Alle
Pflichthooks bleiben wirksam. Nur die gewählte Revision wird verlinkt. Generierte
Spielquellen werden nicht von Hand geändert. Optional kann `DKR_PIPELINE` auf eine
bereits entsprechend erzeugte private Pipeline zeigen; `DKR_GAME_CACHE` erlaubt
einen vorhandenen Download-Cache. Das ersetzt nicht die Prüfung ihrer Herkunft.

Das Ergebnis liegt unter:

```text
$DKR_GAME_WORK/private/native/PPSA94678/
$DKR_GAME_WORK/private/dkrr-install-game-1.0.6-preview.elf
$DKR_GAME_WORK/private/roms/dkr.us.v77.z64
$DKR_GAME_WORK/private/roms/dkr.us.v80.z64
```

Die ROM-Dateien gehören weder in das native Titelverzeichnis noch in den Installer.
Die vollständigen Debug-Symbole bleiben unter `native-link/llvm-pie.elf` für eine
spätere Zuordnung konkreter PS5-Absturzadressen erhalten.

## Konsole und Startweg

Die bisher bestätigte Grundlage ist die vom Nutzer angegebene PS5 13.60 mit
Relapse, WebKit Autoloader 0.6.1, kstuff-lite und ShadowMount-Plus, ohne etaHEN.
Die genaue kstuff-/ShadowMount-Version ist nicht verifiziert. Das ist keine
allgemeine Firmware- oder Jailbreak-Kompatibilitätszusage.

Das Spiel ist ein nativer FSELF-Titel. Der anonyme ELF-Empfänger dient zum
Übertragen des Installers. Ein direkt gesendetes GPU-Spiel-ELF ist kein geeigneter
Startweg: Der frühere anonyme GPU-Test scheiterte am AGC-Aufrufkontext.

Vor einer späteren Installation prüfen, dass `PPSA94678` nicht bereits einem
anderen Titel gehört. Der Installer verweigert abweichende vorhandene Dateien;
er mountet oder startet nichts und verändert keine Whitelist. Er legt nur seinen
eigenen Titel, den leeren `/data/dkrr/roms`-Ordner und zwei leere Spielprofile an. Der bestätigte Renderer-
Titel `PPSA94677` bleibt erhalten. Die eigene executable und clean-room libc
haben Modus `0755`. ShadowMount muss den eigenen Ordner als nativen Titel
registrieren; anschließend über das PS5-Menü starten.

Der eigene kanonische Dump muss separat bereitgestellt werden:

```text
/data/dkrr/roms/dkr.us.v77.z64
/data/dkrr/roms/dkr.us.v80.z64
```

Mindestens eine dieser Dateien ist nötig. Im Dual-Build ist US 1.0 die Vorgabe;
nur US 1.1 vorhanden wird automatisch gewählt. Im Spiel öffnet Touchpad die
Oberfläche. Unter **Play → Game version** lässt sich **US 1.0** oder **US 1.1 – Rev A**
für den nächsten Start wählen. Danach DKR-R regulär beenden und neu starten; die
laufende Sitzung und ihre Spielstände werden nicht zwischen Versionen gewechselt.
Die Auswahl schreibt nur `/data/dkrr/preferences/boot-revision.txt` mit `1.0`
oder `1.1`. Der Installer erstellt dafür einen eigenen beschreibbaren Ordner,
ohne die Rechte der ROM- oder Spielstandordner zu ändern. Die alte Datei
`/data/dkrr/boot-revision.txt` wird weiterhin gelesen, wenn noch keine neue Wahl
vorliegt; sie wird nicht verändert.
Die alten Werte `77`/`80` bleiben lesbar. Ein Build mit nur einer Revision verwendet
immer seine enthaltene Revision. Nicht enthaltene oder fehlende ROMs sind in der
Auswahl deaktiviert. Die Anwendung prüft den vollständigen ROM-Inhalt.
Ohne passenden Dump protokolliert sie den Fehler und beendet ihren eigenen Titel.
Die separat kopierten ROM-Dateien müssen für den nativen Titel lesbar sein
(Modus `0644`). Eine vom Payload angelegte Datei mit `0600` scheiterte beim
nativen Spieltest mit `EACCES`, obwohl ihr Inhalt korrekt war.

Die getrennten Profile sind `/data/dkrr/profile-us-v77` und
`/data/dkrr/profile-us-v80`. Darin liegen auch `logs/runtime.log` und Saves.
Neue Profile legt der Installer mit `0777` an, damit der native Titel darin
schreiben kann; vorhandene Profile und deren Rechte verändert er nicht.
Der native Dateiadapter prüft Namen und Dateitypen über die funktionierende
Verzeichnisschnittstelle. Symbolische Links und unbekannte Dateitypen lehnt er
ab. Auf der PS5 sind deshalb gewöhnliche Dateien und Verzeichnisse erforderlich.
Der native nullfs-Mount meldet den Hardlink-Zähler als `0`, obwohl der Datenträger
für dieselbe Datei `1` meldet. Die aktuelle Vorschau ergänzt den echten Zähler
über den unten beschriebenen Metadaten-Helfer. Die ursprüngliche strenge
Speicherprüfung gilt auch für Sperrdateien; eine ältere Ausnahme für leere Locks
ist entfernt. Speichern und erneutes Laden müssen noch vollständig auf der PS5
abgenommen werden.
Laufzeit- und Renderer-Meldungen werden zusammen gespeichert. Diese Dateien
bleiben privat. Die Touchpad-Taste ist für das Overlay vorgesehen; Options ist
Start. Ein Sitzungsende oder Neustartwunsch schließt die Vorschau; der Titel muss
danach erneut aus dem PS5-Menü gestartet werden. So bleibt kein unsichtbarer
Desktop-Launcher auf dem SDL-Dummyfenster zurück.

## Geplante Hardwareabnahme

1. Frische Jailbreak-Sitzung, Empfänger und ShadowMount prüfen; Titel-ID abgleichen.
2. Eigenen privaten Kandidaten installieren und einen eigenen Dump separat kopieren.
3. Start, Intro/Spielmenü, Controller einschließlich Sticks und Touchpad prüfen.
4. Eine reguläre Strecke spielen; Bild, Grafikfehler, Ton und Laufzeit beobachten.
5. Speicherstand erstellen, regulär beenden, neu starten und Laden prüfen.
6. PS-Menü, Fokuswechsel, Ende und erneuten Start ohne Fehlermeldung prüfen.
7. Erst danach Rev A, längere Sitzungen, mehrere Controller und höhere Grafikoptionen.

Ein Installations- oder Container-PASS ersetzt keinen dieser Spieltests. Bis zu
dieser Abnahme bleibt das Ergebnis eine private native Vorschau.

## Aktuelle native Speicher-Metadatenanpassung

Die native nullfs-Ansicht meldet für reguläre Dateien `nlink=0`. Der separate
Helfer `bin/dkrr-save-metadata.elf` liest die echten Metadaten ausschließlich für
Dateien unter den eigenen v77/v80-Speicherpfaden einschließlich deren
`save-backups` und `save-history`, bindet nur an Loopback und
liest weder Inhalte noch ROMs. Der Spieladapter behält die nativen stat-Angaben
bei und ergänzt ausschließlich die Verknüpfungszahl nach Identitätsprüfung und
erneuter nativer Abfrage. Ohne Helfer oder bei widersprüchlichen Angaben bleibt
sie unbekannt; die Schutzprüfung verweigert dann die Datei. Echte Mehrfachlinks
werden als Mehrfachlinks übernommen, niemals als eins ausgegeben.

```bash
bash tools/build_save_metadata.sh "$PWD/work/lowmem/sdk/ps5-payload-sdk"
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-save-metadata.elf --no-wait
```

Der Helfer ist pro Konsolenstart erneut zu starten. Der neue Quellstand bleibt
bis zum Prozessende oder Konsolenneustart verfügbar, auch nach langen Pausen;
einzelne Anfragen haben weiterhin kurze Zeitgrenzen. Der zusätzliche Startschritt
ist eine vorläufige Kompatibilitätslösung. Native Link-Zahl, korrektes Save/Load und nachhaltige
Speichersicherheit sind damit noch nicht vollständig auf Hardware abgenommen.
Das Host-Modell prüft unbekannte Metadaten, echte Aliase, Veränderungen während
der Prüfung und fremde Pfade. Es ersetzt keine Konsolenabnahme.

## Controller-Menü und Texturen

Der neue PS5-Menüstand ordnet gewöhnliche Einstellungsfelder und Karten
untereinander an. L1/R1 wechseln die Seiten; Steuerkreuz/Linksstick wählen,
Kreuz bestätigt, Kreis bricht ein aktives Feld/Popup ab oder kehrt von einer
ruhenden Seite ins Spiel zurück. Touchpad öffnet/schließt das Menü.
Der lokale Test nutzt echtes ImGui für Fokusreihenfolge, deaktivierte Einträge,
Scrollen und Kreuz-Aktivierung. Die Wirkung am Fernseher steht noch aus.

Das vom Nutzer gelieferte SR.GU Remastered 0.2.7 wurde privat mit dem tatsächlichen
Rice-Importer der Version 1.0.6 in 1.832 native Zuordnungen umgewandelt und auf
seiner eigenen PS5 per vollständiger Prüfsumme verifiziert. Das Paket wird nicht
exportiert. Ersatztexturen sind im Modern-Profil aktivierbar. Dateiintegrität
beweist nicht die vollständige grafische Abdeckung oder flüssige Bildausgabe.

## Ein ROM, freie PS5-Interface-Schriften

`DKR_ROM` wählt einen eigenen unterstützten US-Dump automatisch anhand der
vollständigen kanonischen SHA-1. Für einen optionalen Doppelbuild stattdessen
`DKR_ROM_V77` und `DKR_ROM_V80` setzen; `DKR_ROM` dabei nicht setzen. Die private
Auswahl wird ausdrücklich gespeichert. Zurückgebliebene Dateien anderer
Revisionen werden nicht automatisch einbezogen.

Die PS5-Oberfläche verwendet jetzt ausschließlich Selawik (SIL OFL 1.1).
Racing Banana und Jumpman sind aus den PS5-Quellen des Link-Ziels ausgeschlossen.
Die Original-Fontdateien des unveränderten Upstream-Downloads werden nicht in
unseren Quell-Export aufgenommen. Die lokalen Builds beider einzelnen Revisionen
haben die Abwesenheit dieser Font-Symbole nachgewiesen. Ihre sichtbare Ausgabe auf
PS5 wird später separat abgenommen.

Der optionale `dkrr-install-owned-rom.elf` enthält den eigenen ROM-Dump. Er wird
nur lokal erzeugt, verbleibt im privaten Arbeitsordner und darf niemals bei
GitHub, in Issues oder in einem geteilten Buildpaket landen. Er schreibt nur die
gewählten kanonischen ROM-Dateien unter `/data/dkrr/roms`, verweigert abweichende
vorhandene Dateien und gibt sie für die native Anwendung lesbar frei. Der
Spiel-Installer selbst enthält Spiel-CPU-Code, aber keinen ROM-Anhang. Auch er
bleibt privat. Quellprüfung und Testumfang: [VERIFICATION.md](VERIFICATION.md).

Die originalen DKR-R-Projektgrafiken für Icon/Hintergrund, Deutsch über die US-Spieldaten sowie Controller
Pak und Rumble sind in [CONSOLE_FEATURES.md](CONSOLE_FEATURES.md) beschrieben.
Offene Darstellungsfehler einschließlich Wasserflackern bei Tracks-Vorschauen
und gezielte Vergleichstests stehen in [RENDER_ISSUES.md](RENDER_ISSUES.md).

## Vorhandenen eigenen Titel aktualisieren

Der erste Installer verweigert abweichende vorhandene Dateien. Für eine neue
Spiel-Datei allein gibt es `tools/build_native_game_updater.py`. Für eine Revision
mit neuen Icons/Hintergründen oder Begleitdateien verwendet man den Bundle-Planer:

```bash
python3 tools/build_native_game_bundle_updater.py "$DKR_SDL_WORK/sdk/ps5-payload-sdk" \
  /absolute/private/previous/PPSA94678 \
  "$DKR_GAME_WORK/private/native/PPSA94678" \
  "$DKR_GAME_WORK/private/dkrr-update-bundle.elf" --plan-only
```

Der Plan enthält die vollständige Prüfsumme des vorherigen lokalen App-Manifests,
die geänderten Dateien und alle gemeinsam vorhandenen Dateien für die Vorprüfung.
Vor einem Konsolenupdate müssen die dort vorhandenen Bytes mit diesem Vorgänger
abgeglichen und das Spiel regulär geschlossen sein. Das lokal alte App-Verzeichnis
allein beweist nicht, welcher Stand gerade auf der Konsole liegt.

Mit dem überprüften `previous_manifest_sha256` lässt sich die private ELF bauen:

```bash
python3 tools/build_native_game_bundle_updater.py "$DKR_SDL_WORK/sdk/ps5-payload-sdk" \
  /absolute/private/previous/PPSA94678 \
  "$DKR_GAME_WORK/private/native/PPSA94678" \
  "$DKR_GAME_WORK/private/dkrr-update-bundle.elf" \
  --expected-previous-manifest-sha256 RECORDED_MANIFEST_SHA256 --sequence 17
```

Auch `DKR_GPU_WORK` muss auf den vorbereiteten GPU-Arbeitsordner zeigen. Der
Updater prüft zuerst alle gemeinsamen Dateien gegen die aufgezeichneten alten
oder neuen Bytes. Er verweigert unbekannte Dateien und symbolische Links auch
in Elternverzeichnissen. Pro Datei werden neue Bytes synchronisiert, überprüft
und atomar ersetzt; Metadaten und Spiel-Datei kommen nach ihren Begleitdateien.
Das gesamte Bundle ist keine atomare Transaktion: ein unterbrochener Lauf kann
bekannte alte/neue Dateien enthalten und nach erneuter Prüfung fortgesetzt werden.
Neue zusätzliche Dateien legt anschließend der normale verweigernde Installer an;
alte zusätzliche Dateien werden nicht gelöscht. Der lokale Vorgänger bleibt erhalten.

Der Builder verbindet sich nicht mit einer PS5 und startet keinen Titel. Die
generierte ELF enthält privaten ROM-abgeleiteten Spielcode und gehört nicht in
das öffentliche Quellpaket. Hosttests prüfen den tatsächlichen C-Updater mit
synthetischen Dateien, einschließlich Wiederholung, Fortsetzung und Verweigerung.

Die vollständige offene Aufgabenliste steht in [WORK_STATUS.md](WORK_STATUS.md).

### Kleiner Auswahlordner-Helfer bei bestehenden Installationen

Nach einem exakt geprüften Programmupdate muss für die Versionswahl kein voller
App-Installer erneut übertragen werden. Der kleine Helfer enthält ausschließlich
die unveränderte Titelidentität und legt `/data/dkrr/preferences` beschreibbar an:

```bash
python3 tools/build_native_game_installer.py "$DKR_SDL_WORK/sdk/ps5-payload-sdk" "$DKR_GAME_WORK" --preferences-only
```

Das Ergebnis ist `$DKR_GAME_WORK/private/dkrr-install-preferences.elf`. Es ersetzt
keinen Spielcode und enthält keine Grafik, ROM oder Save-Datei. Der bekannte
vollständige Installer ist deutlich größer; ein Transfer-Timeout gilt nicht als
Installation und darf nicht durch wiederholte Starts übergangen werden. Den
aktuellen App-Inhalt nachlesen und bei reinem Loader-Startfehler zuerst den
normalen Loader-Zustand wiederherstellen.

## US-Doppelbuild und Sprachen

Die aktuelle Zielroute ist US 1.0 + US 1.1; PAL-Integration ist zurückgestellt.
Im ursprünglichen Spielmenü **Options → Language** wechseln Links/Rechts durch
Englisch, Französisch und Deutsch. Beide US-Revisionspayloads erhalten denselben
geprüften Zusatz, behalten aber ihre eigenen Adressen und vorhandenen Profile.
Die Textdaten sind bereits in den eigenen US-ROMs vorhanden. Der Originalaufruf
zum Sprachwechsel und seine normale EEPROM-Speicherung bleiben zuständig.
Der Recompiled-Einstellungsdialog selbst ist weiterhin Englisch.

Eine alte private `DKR_PIPELINE` ohne den Sprachadapter wird beim nativen Build
abgewiesen; neu über die dokumentierte CPU-Pipeline generieren.
Deutsch-Anzeige und dauerhafte Sprachspeicherung müssen auf der PS5 noch geprüft
werden. Der letzte dort installierte Rückfallstand ist der früher akzeptierte
US-1.0-Build mit Startbildkorrektur; auch er wurde zuletzt vor einem neuen
Spielprotokoll vom Loader abgewiesen. Seit dem anschließenden Ausschalten wird
nur offline gearbeitet. Der neue Sprachbuild ist nicht installiert.

## Hauptmenühintergrund und Launch-Poster

Der aktuelle App-Ordner enthält das unveränderte Icon und `pic0.dds` für das
Hauptmenü. `pic1.dds` wird gemäß aktueller Nutzerentscheidung wieder bytegleich zu `pic0.dds` mitgeliefert; der bestätigte Ausblendeaufruf bleibt erhalten. Der Build allein entfernt
keine ältere Datei auf der Konsole. Der dafür vorbereitete Helfer prüft die
exakten eigenen Titel-/Bilddateien und bewahrt die früheren Launch-Bilder als
Rückfallkopien. Ablauf und Registrierungsgrenzen: [TITLE_PRESENTATION.md](TITLE_PRESENTATION.md).
Die sichtbare Hauptmenüdarstellung und das Verhalten ohne eigenes Launch-Bild
sind erst nach einem echten Konsolentest als bestanden zu bewerten.

## Private Quell-Beta und Updates

[PRIVATE_BETA.md](PRIVATE_BETA.md) beschreibt den Testumfang.
[UPDATE_GUIDE.md](UPDATE_GUIDE.md) bündelt Quellupdate, lokalen Eigenbau und den
inkrementellen eigenen Dateiaustausch mit exaktem Rückfallstand; ROMs und
Spielstände müssen dafür nicht erneut installiert werden.

## Optionale TV-Ausgabe

Die PS5-Optionen sind in den vorhandenen Grafikbereich eingebunden. 1080p/1440p/4K-Ausgabepuffer und Renderingauflösung sind getrennt. Der normale Build bleibt bei 60 Hz und unveränderten Metadaten. `DKR_PS5_HIGH_REFRESH_BUILD=1` erstellt ausdrücklich einen separaten 120-Hz-Teststand; normale Aktualisierungshelfer lehnen dessen geänderte Titelmetadaten ab. Voraussetzungen, belegte Schnittstellen und Abnahme: [DISPLAY_FEATURES.md](DISPLAY_FEATURES.md).
