# Plume/RADV und eigener nativer PS5-Titel

Stand: 12. Oktober 2026. Dies ist eine Renderer-Diagnose, kein DKR-R-Spielport.

**Aktueller Titel im Kit: RT64 v9.** Der vollständige RT64-Renderer zeigt nun
eine selbst erzeugte türkise N64-RDP/VI-Fläche und schließt seinen eigenen Titel
nach dem Test selbst. Ausgabe und fehlerfreie Rückkehr ins Menü sind vom Nutzer
bestätigt. Seine Build-Anleitung steht in [RT64_BUILD.md](RT64_BUILD.md).
Die folgenden v5-Schritte und Messwerte dokumentieren den vorherigen Plume-
Basistest; sie sind nicht die Ausgabe des aktuellen v9-Titels.

## Bestätigter aktueller GPU-Test auf Firmware 13.60

Der native Titel **v5** hat auf der tatsächlichen Konsole Shader-Pipeline,
exakten GPU-Pixel-Readback und 200 Presents bestanden. Anschließend meldet er
Status 0 und wartet auf das Schließen durch den Systemdienst. Der Hilfsloader
schließt ausschließlich seine beim Start zurückgegebene App-ID; Ergebnis 0.
Der Nutzer bestätigt die Rückkehr ins normale PS5-Menü ohne Fehlermeldung.
Die genaue Binärprüfsumme und Grenzen stehen in
`evidence/native-gpu-acceptance.json`, die Ausgabe in
`evidence/native-launch-v5.log`. Dies belegt den kleinen Renderpfad, kein DKR-R-
Gameplay oder eine allgemeine Vulkan-Konformität auf dieser Firmware.

## Historische Startfehler und ihre Korrekturen

Die SDL-Probe lowmem-v2 ist über den Homebrew Launcher sichtbar, spielt mit
Kreuz einen Ton und beendet sich mit Kreis. Das hat der Nutzer bestätigt.

Der vollständige Plume/RADV-Payload wurde gebaut und ausgeführt. V1 scheiterte
vor seiner eigenen Ausgabe; der Nutzer meldete `0xa0020328
(system_xo_violation)`, Thread `scecloudclientappmain`. V2 korrigiert die beiden
Systemimporte von `.prx` auf `.sprx`. Seine Ausgabe belegt:

```text
[Agc] FS Table offset has shifted. This is not survivable.
radv/ps5: sceAgcInit(8) failed: 0x8a6c002f
plume-probe: instance=PASS api-request=1.2 resolver=static-RADV
Unable to find devices that support Vulkan.
plume-probe: FAIL device/features
plume-probe: exited status=2 heap-peak=16777216 heap-libc-fallbacks=0
```

Eine erzeugte Instance ist kein Nachweis eines nutzbaren Grafikgeräts. Pipeline,
GPU-Readback und Dreieck wurden bei diesem Lauf nicht erreicht. Die Ursache
darf deshalb nicht pauschal als fehlende GPU-Unterstützung von Firmware 13.60
bezeichnet werden. Der Aufrufkontext ist relevant: Die öffentliche
[SDK-AGC-Deklaration](https://github.com/mihawk-99/PS5_PayloadSDK/blob/b83202be73e930050e00de0f1b4d9ec46c0391bf/platform/include/ps5platform/agc.h)
beschreibt einen erfolgreichen Aufruf aus dem eigenen nativen Executable und
einen fehlgeschlagenen aus nachgeladenem anonymem Code.

`native/PPSA94677/` enthält daher einen eigenständigen nativen FSELF-Titel
„DKR-R GPU Probe v5“. Sein Start verwendet den nativen App-CRT, nicht den
Payload-CRT. Beide FSELF-Container bestanden Integritätschecks. Die enthaltene
`libc.prx` wurde ausschließlich aus dem öffentlichen Clean-room-Emitter und
seinen Textmanifesten erzeugt und entspricht exakt dessen erwarteter Prüfsumme:
`e6ff45d16adf687855cc3b33b0c8a4132b6504360b221e0a34c7e99fb3ba0036`.
Es wurde keine Sony-Laufzeit aus einer anderen Anwendung kopiert.

ShadowMount-Plus registrierte den Titel mit Ergebnis `0x00000000`. Die vier
Dateien wurden über HTTP bytegenau rückgelesen. Der bisherige Systemstart gab
`0x80aa001a` zurück, noch vor einer eigenen Logdatei. Die anfänglich mit `0644`
übertragenen zwei ausführbaren Dateien wurden inzwischen auf `0755` korrigiert.
V3 erreichte nach der Rechtekorrektur die Shader-Pipeline, scheiterte aber an
meinem nicht unterstützten Plume-Aufruf für Bild→Puffer. V4 verwendet den
expliziten Vulkan-Readback und bestand Rendering und Aufräumen; danach löste
`exit()` SIGSYS aus. V5 bleibt nach der Ressourcenfreigabe in `main`, wie die
native Laufzeitdokumentation verlangt, und wird durch den Systemdienst
regulär geschlossen. V5 und die Rückkehr ins normale Menü sind bestätigt.


## Reproduzierbar bauen

Dieser Build wurde auf CachyOS mit Clang/LLD 23.1.1 geprüft. Zusätzlich werden
Python 3 mit venv/pip, GNU Make, wget, unzip, tar, patch, pkg-config,
glslangValidator, LLVM einschließlich Entwicklungsbibliotheken/libclc,
SPIR-V Tools und LLVM-SPIRV benötigt. Das Skript legt Meson/Ninja und seine
Python-Abhängigkeiten in einer lokalen venv ab. Es installiert keine
Systempakete. Quellen, SDK und Compiler sind getrennt: Der GPU-Build benutzt
die vom RADV-Projekt festgelegte öffentliche SDK-v0.42-Basis mit SDK-Fork;
SDL und die Transfer-Installer verwenden weiterhin SDK v0.44.

Im Kit-Ordner:

```bash
export DKR_GPU_WORK="$PWD/work/vulkan"
bash build_gpu.sh
bash tools/build_native_gpu_title.sh "$DKR_GPU_WORK"
python3 tools/build_native_installer.py "$PWD/work/lowmem/sdk/ps5-payload-sdk"
```

Die dritte Zeile benötigt die bereits durch `build.sh` bereitgestellte
SDK-v0.44-Installation; bei anderem SDL-Arbeitsordner deren absoluten Pfad
verwenden. Sie erstellt `bin/dkrr-install-native-gpu.elf`. Der native Titel
liegt unter `native/PPSA94677/`. Ein erfolgreicher Build beweist weder den
Start auf einer bestimmten Konsole noch einen spielbaren DKR-R-Port.

`VULKAN_SOURCES.json` legt die öffentlichen Quellarchive samt SHA256 und
Revisionen fest. Das Plume-Archiv und die nötigen öffentlichen Werkzeuge liegen
im Kit; Mesa und die gepinnten Abhängigkeiten werden bei Bedarf geladen.
`tools/apply_plume_patch.py` prüft alle Originaldateien und verweigert abweichende
Änderungen. Mesa/RADV werden unverändert mit dem PS5-Winsys gebaut. Unser
Plume-Patch integriert den statischen RADV-Resolver und `VK_KHR_display`,
wählt eine unterstützte Display-Ebene und bevorzugt 1080p bei bis zu 60 Hz.
Raytracing wird für diesen ersten Rasterpfad nicht angefordert.

Der Hosttest `tests/plume_display_test.cpp` prüft nur Display-Auswahl und
Fehlerweitergabe mit nachgebildeten Vulkan-Funktionen. Er belegt keine GPU-
oder Firmware-Kompatibilität.

## Start und Protokoll

Der native Titel benötigt deinen bereits eingerichteten Relapse-/kstuff-lite-
und ShadowMount-Plus-Start. Kein etaHEN wurde installiert oder vorausgesetzt.
Ein Transfer-Installer kann die vier Dateien unter
`/data/homebrew/PPSA94677/` ablegen. Er verweigert vorhandene abweichende Dateien
und Symlinks. Bei exakt passendem Inhalt setzt er die für Eboot und Laufzeit
vorgesehenen Ausführungsrechte. Er ändert keine Whitelist oder Jailbreak-
Konfiguration. ShadowMount verwendet hier bereits `scanpath=/data/homebrew`.

```bash
python3 tools/send_elf.py YOUR_PS5_LAN_IP bin/dkrr-install-native-gpu.elf
```

Zum automatischen einzelnen Test wird der im Kit enthaltene
`bin/dkrr-native-launch-diag.elf` über denselben Sender ausgeführt. Er startet
nur PPSA94677 und schließt nur dessen zurückgegebene App-ID, nachdem die Probe
ihren Abschluss meldet. Ein manueller Start ohne Hilfsloader bleibt nach den
200 Bildern absichtlich offen: über die PS-Taste regulär „Spiel schließen“
wählen.

Das ist eine Bereitstellung des Tests. Der manuelle Start erfolgt über
„DKR-R GPU Probe v5“ auf dem Hauptbildschirm. Das native Programm versucht bei erreichtem `main` eine Logdatei in seinem
eigenen Datenordner und danach unter `/app0`. Auf dieser Konsole war die
Logdatei nicht verfügbar; deshalb gehen alle eigenen Ergebniszeilen zusätzlich
über `sceKernelDebugOutText` und wurden vom Hilfsloader erfasst.
Ein fehlendes Log allein ist kein endgültiger Fehlernachweis.

Ein erfolgreicher GPU-Test muss echte Pixel vom GPU-Readback vergleichen und
den Present-Pfad erreichen. Erwartetes Testbild: rotes Dreieck auf dunklem Blau
mit bewegtem weißen Balken. Die geplanten 200 Presents sind kein garantierter
Abbruch bei einem blockierenden Treiberaufruf. TV-Sichtbarkeit wird zusätzlich
vom Nutzer bestätigt; ein zurückgekehrter Present-Aufruf genügt dafür nicht.

## Grenzen und Spieldaten

Dieses Paket enthält keine ROM, Nintendo-/Rare-Assets oder Spiel-Payloads.
Die beiden eigenen Cartridge-Dumps wurden zunächst read-only auf die benötigten
US-Revisionen geprüft. Für die weitere lokale Spielintegration liegen inzwischen
geprüfte Arbeitskopien und erzeugte CPU-Codepakete unter `work/game/private/`.
Die Originaldateien bleiben unverändert. Keine ROM wurde zur PS5 übertragen;
ROMs und ROM-basierte Build-Produkte gehören nicht in dieses öffentliche
Renderer-Kit. Das Lesen vorhandener eigener Dateien begründet
keine Aussage über jede nationale Rechtslage; Weitergabe fremder Spielinhalte
ist nicht Bestandteil dieses Builds.

DKR-R benötigt weitere Arbeit an der gemeinsamen Spielintegration, Eingabe,
Ton, Speicherständen und Spieltests. Der native RT64-Basispfad ist bestätigt;
die CPU-Payloads und librecomp sind für PS5 kompiliert. Ein vollständiger
Spielbuild und Gameplay sind noch nicht nachgewiesen. Die offenen Punkte stehen
in `PORTIERUNG.md`.
