# RT64: bestätigter nativer PS5-Renderer und Build

Stand: 12. Oktober 2026. Dies ist ein Renderer-Test, noch kein spielbarer DKR-R-Port.

## Hardwarestand

Die eigene Anwendung `PPSA94677`, Build v9, startet auf der vom Nutzer angegebenen
Firmware 13.60 mit Relapse, WebKit Autoloader 0.6.1, kstuff-lite und ShadowMount-Plus.
etaHEN wird in diesem Aufbau nicht verwendet. Die genaue kstuff-/ShadowMount-Version
ist weiterhin nicht verifiziert.

Der tatsächliche RT64-Renderer initialisiert seine Vulkan-Geräte, Shader und
Arbeits-Threads. Ein selbst erzeugter N64-RDP/VI-Befehl zeichnet eine türkise Fläche.
Der Nutzer hat diese Ausgabe gesehen. Der Test reicht 120 Bilder im Abstand von
100 ms ein, räumt RT64 auf und schließt seinen eigenen Titel über den Systemdienst.
Der Nutzer hat die Rückkehr ins normale PS5-Menü ohne Fehler bestätigt. 120
eingereichte Bilder sind weder ein FPS-Messwert noch ein Nachweis für 120 korrekt
angezeigte Bilder. Der maximale gemeldete Titel-Heap beträgt 117.440.512 Bytes;
es wurden keine libc-Heap-Ausweichzuweisungen gemeldet.

Genaue Binärprüfsumme und Grenzen: `evidence/native-rt64-acceptance.json`.
Gefiltertes Ergebnis: `evidence/rt64-native-v9-result.log`.

Der frühere Plume-Test v5 hat zusätzlich konkrete GPU-Pixel über Readback geprüft.
Sein Ergebnis steht separat in `evidence/native-gpu-acceptance.json`.
Der RT64-Test v9 enthält keinen Pixel-Readback, keinen ROM-Dump und keine Spieldaten.

## Reproduzieren auf Linux

Voraussetzungen und Quellen für SDL, Mesa/RADV und das öffentliche SDK stehen in
`README.md` und `GPU_BUILD.md`. Verwende die dort gepinnten Revisionen. Der
bestätigte lokale Werkzeugstand ist Clang/LLD 23.1.1 und CMake 3.31.6. Die
Shader-Erzeugung verwendet die prüfsummengeschützten Linux-DXC-Dateien aus
`RT64_SOURCES.json`.

Zunächst SDL und GPU bauen, jeweils in einem eigenen absoluten Arbeitsordner:

```bash
export DKR_PS5_WORK="$PWD/work/lowmem"
bash build.sh
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
```

Für RT64 einen frischen Ordner wählen. Ein bereits gefüllter, nicht markierter
Quellordner wird zum Schutz vorhandener Änderungen abgelehnt:

```bash
export DKR_SDL_WORK="$DKR_PS5_WORK"
export DKR_RT64_WORK="$PWD/work/rt64-build"
bash build_rt64.sh
```

Falls das passende CMake nicht im Suchpfad liegt, `CMAKE_BIN` auf dessen
ausführbare Datei setzen. Der Build lädt öffentliche, gepinnte Quellen und
prüft ihre SHA-256-Werte. Er erzeugt `rt64.a`, die Shader und den nativen
Testtitel unter `native/PPSA94677/`. Er lädt keine ROM und verändert keine Konsole.

## Start auf der PS5

`eboot.bin` und `sce_module/libc.prx` müssen ausführbar sein (`0755`). Die
clean-room `libc.prx` stammt aus dem öffentlichen Quellcode-Emitter; Sony-Dateien
werden nicht aus Spielen oder Systemmodulen kopiert.

Für eine erste Installation kann der eigene Installer gebaut werden:

```bash
python3 tools/build_native_installer.py "$DKR_PS5_WORK/sdk/ps5-payload-sdk"
```

Er schreibt ausschließlich die vier eigenen Dateien unter
`/data/homebrew/PPSA94677/`. Bei abweichenden vorhandenen Dateien bricht er ab.
Er verändert keine Jailbreak-Konfiguration und keine Whitelist. Übertragen wird
er bei laufendem ELF-Empfänger mit:

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-install-native-gpu.elf
```

ShadowMount-Plus muss diesen Ordner scannen und als nativen Titel registrieren.
Danach den Titel über das PS5-Menü starten. Er zeigt für ungefähr zwölf Sekunden
die türkise Fläche und schließt sich selbst. Die Firmware-/Startkonstellation
anderer Konsolen wurde nicht geprüft. Die eigentliche Grafik-Anwendung wird als
nativer Titel gestartet; der direkte anonyme Payload-Start scheiterte zuvor am
AGC-Aufrufkontext.

## Notwendige Plattformanpassungen

- Plume verwendet statisch gelinktes RADV und eine geprüfte KHR-Display-Auswahl
  statt eines Desktop-SDL-Vulkan-Fensters.
- RT64 erhält einen PS5-Fensteradapter für direkte VideoOut-Ausgabe.
- Desktop-Dateidialoge sind im synthetischen Renderer-Test deaktiviert; sie
  liefern bei Aufruf regulär Abbruch. Die ROM-Auswahl des Spiels ist separat
  zu integrieren.
- Zstd-Profiling-Hooks sind über `ZSTD_TRACE=0` abgeschaltet. Die Kompression
  bleibt unverändert nutzbar; optionale undefinierte ELF-Hooks können nicht
  als native SCE-Imports übernommen werden.
- `timegm` ist eine eigene UTC-Konvertierung ohne globale Zeitzonenänderung.
  Sie bestand 2.005 Vergleichsfälle gegen die Host-libc.
- Der emulierte Thread-Speicher wird eine pthread-Destruktor-Runde später
  freigegeben. Ein Host-Test mit AddressSanitizer reproduzierte ohne diese
  Änderung einen Use-after-free; mit der Änderung bestanden 32 Thread-Enden.
  Das originale SDK bleibt unverändert. `prepare_emutls.py` erzeugt eine
  prüfsummengeschützte Build-Kopie mit der Anpassung.
- Beim normalen Ende wird der eigene, anhand PID und Titel-ID geprüfte Titel
  nach der Ressourcenfreigabe über den Systemdienst geschlossen. Ein direktes
  `exit()` hatte zuvor nach erfolgreichem GPU-Test SIGSYS ausgelöst.

## Noch nicht nachgewiesen

N64-Texturen, tatsächliche DKR-Displaylisten, längere Spielsitzungen,
Controllerachsen, Rare-RSP-Audio-Timing und Speicherstände sind noch nicht
auf Hardware bestätigt. CPU-Payloads, ROMs und extrahierte Spieldaten werden
für die weitere lokale Spielintegration getrennt unter `work/game/private/`
gehalten und nicht in das öffentliche Renderer-Kit aufgenommen.
