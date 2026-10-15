# Quellen, Lizenzen und Buildumfang

Die Diagnosen sind eigene, unter GPL-3.0-or-later bereitgestellte
Programme ohne Spielcode. Sie verwenden folgende Komponenten:

| Komponente | Stand | Lizenz/Quelle |
|---|---|---|
| PS5 payload SDK | v0.44, `f5d40f02770e9b0001dd9e18056eca70b3da8b40` | überwiegend GPL-3.0-or-later; FreeBSD-Header mit eigenen BSD-Hinweisen; vollständiger Git-Quellstand in `vendor/ps5-sdk-source.tar.gz` |
| PS5 SDL2 | `ee4c47dc0d617b3bc8f35108f9956baf228a1322` | zlib, vollständiger Git-Quellstand in `vendor/ps5-SDL.tar.gz` |
| libsamplerate | 0.2.2, `c96f5e3de9c4488f4e6c97f59f5245f22fda22f7` | BSD-2-Clause, vollständiger Git-Quellstand in `vendor/libsamplerate.tar.gz` |
| LLVM libc++, libc++abi, libunwind | SDK-v0.44-Build, SDK-Skript nennt LLVM 18.1.8 | Apache-2.0 WITH LLVM-exception; Hinweise in `licenses/`; bevorzugte Quellen beim LLVM-Projekt |
| N64ModernRuntime | `ae1ffbb909d9f93c88c41830deb539f7feef5ed2` | GPL-3.0, `vendor/N64ModernRuntime.tar.gz`; wird nicht in die Diagnose-ELFs gelinkt |
| N64Recomp | `81213c1831fab2521a6a5459c67b63437d67e253` | MIT, `vendor/N64Recomp.tar.gz`; für Runtime-Compilecheck benötigte Header, nicht in die Diagnose-ELFs gelinkt |

Die Softwarebild-Diagnose linkt SDL2, libsamplerate und den Payload-SDK-CRT/
libc-Hilfscode statisch. Die C++-Diagnose linkt außerdem die mit dem SDK
bereitgestellten C++-Runtime-Archive. SCE-Systembibliotheken werden als Imports
angesprochen; ihre Inhalte werden nicht mitgeliefert.

Source-Archive enthalten den jeweils angegebenen Git-Quellstand, einschließlich
Original-Lizenzdateien. N64Recomp-Unterprojekte sind nicht eingeschlossen, weil
diese für den enthaltenen reinen `ultramodern`-Compilecheck nicht verwendet
werden. Wer den kompletten Recompiler baut, muss seine gepinnten Submodule
zusätzlich beziehen. Das Runtime-Archiv ist unmodifiziert; unser Patch liegt
separat in `patches/` und wurde auf einer isolierten Arbeitskopie geprüft.
Das SDL-Archiv bleibt ebenfalls unverändert. Die neue Bilddiagnose verwendet
zusätzlich `patches/0002-sdl-ps5-sized-video-buffers.patch`, der die
Bildpuffergröße, SCE-Fehlerausgaben und Ressourcenfreigabe anpasst. Der Patch und
seine Prüfsummen gehören bei Weitergabe der neuen Binärdatei zum Quellpaket.

- [PS5 payload SDK](https://github.com/ps5-payload-dev/sdk/tree/f5d40f02770e9b0001dd9e18056eca70b3da8b40)
- [PS5 SDL2](https://github.com/ps5-payload-dev/SDL/tree/ee4c47dc0d617b3bc8f35108f9956baf228a1322)
- [libsamplerate](https://github.com/libsndfile/libsamplerate/tree/c96f5e3de9c4488f4e6c97f59f5245f22fda22f7)
- [LLVM 18.1.8 Quellen](https://github.com/llvm/llvm-project/tree/llvmorg-18.1.8)
- [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime/tree/ae1ffbb909d9f93c88c41830deb539f7feef5ed2)
- [N64Recomp](https://github.com/N64Recomp/N64Recomp/tree/81213c1831fab2521a6a5459c67b63437d67e253)

Die neuen GPU-Diagnosen enthalten Plume, PS5 RADV/Mesa und den öffentlichen
SDK-Fork. DKR-R und RT64 sind weiterhin nicht in diesen Diagnose-Binaries
enthalten. Für ein späteres Spielpaket gelten zusätzliche
Lizenz-, Quellen- und Assetpflichten, siehe `PORTIERUNG.md` und die jeweiligen
Upstream-Hinweise. Keine ROM, extrahierten Nintendo/Rare-Assets, Saves,
Sony-SDK-Bestandteile oder Sony-Systemmodule sind enthalten.


## Neue native GPU-Diagnose

Plume (`d890ac899e505fb30040e037a4037cdeca68f033`, MIT) liegt unmodifiziert in
`vendor/Plume.tar.gz`, mit eigenem PS5-Patch daneben. Die benötigten öffentlichen
PS5_Vulkan-Werkzeuge und Clean-room-Laufzeitquellen liegen mit Original-
Copyright/Lizenzhinweisen in `vendor/PS5_Vulkan-build-support.tar.gz`.
Der darin enthaltene Emitter wurde von BlackBearReloaded entwickelt; die
Integration und RADV-Rezepte von Mihawk-99 stehen unter GPL-3.0-or-later.
Sony-Systemmodule werden ausschließlich importiert, nicht mitgeliefert.

PS5_Mesa, SDK-Fork, Vulkan-Header, volk und VulkanMemoryAllocator werden aus den
in `VULKAN_SOURCES.json` festgelegten öffentlichen Quellen und Revisionen
bezogen. Mesa/RADV enthält überwiegend MIT-Code; die Dateien behalten ihre
jeweiligen Hinweise. Der SDK-Fork und seine Plattformschicht stehen unter
GPL-3.0-or-later, einschließlich der bestehenden Lizenzhinweise für Header.
volk und VMA stehen unter MIT, Vulkan-Headers unter Apache-2.0. zlib 1.3.1 und
1.3.2 samt ihren Lizenzdateien sind für Ziel- bzw. Host-Paketbau im Kit.
Die jeweiligen Originalquellen und unser Buildskript sind zum Nachbauen
angegeben; keine Sony-SDK- oder Sony-Laufzeitdatei wurde als Quelle verwendet.

Die Schutzprüfung des nativen Updaters verwendet die vorhandene öffentliche
streamende SHA-1-Implementierung aus `PS5_PayloadSDK/crt/nid.c` (John Törnblom,
GPL-3.0-or-later). Das generierte Arbeitsmodul behält deren Copyright-Text.
Die Prüfsumme erkennt unsere bekannten vorherigen Build-Dateien und ist kein
allgemeiner Authentifizierungsdienst. Ein fremder/abweichender Stand wird vor
jeder Ersetzung verweigert.

## Nativer RT64-Test v9

Der aktuelle FSELF-Titel enthält RT64 (MIT), Plume (MIT), RADV/ACO/Mesa und die
öffentliche PS5-Plattformlaufzeit. Die exakten Archive und Revisionen stehen in
`RT64_SOURCES.json` und `VULKAN_SOURCES.json`; eigene Änderungen liegen separat
in `patches/` und `src/`. Die RT64-Abhängigkeiten enthalten unter anderem
ImGui/ImPlot (MIT), re-spirv und Zstd (BSD/GPL-Duallizenz). Die Originalhinweise
bleiben in den gepinnten Quellarchiven erhalten. Die DXC-Dateien werden aus dem
pinnierten öffentlichen RT64-Hosttools-Repository bezogen, nicht aus Sony-SDKs.

Der TLS-Buildadapter verändert ausschließlich die Cleanup-Bedingung der
LLVM-`emutls.c` aus dem öffentlichen SDK-Fork. Der erzeugte Quelltext behält
seinen Apache-2.0-with-LLVM-exception-Hinweis. `prepare_emutls.py` prüft vor der
Änderung die vollständige Originalprüfsumme. Private Nintendo-ROMs, daraus
extrahierte Inhalte und die beiden CPU-Payload-Archive liegen nur im lokalen
Build-Arbeitsordner und gehören nicht zum Renderer-Kit.

## Private Spielvorschau und öffentlicher Source-Entwurf

Der aktuelle Spielstand verwendet DKR-R 1.0.6, Commit
`c3eda4d368afbc729ed4a71312cce38d27f87c37`. Die ursprünglichen DKR-R-Quellen
stehen unter MIT; ihre Lizenz bleibt im heruntergeladenen Originalarchiv erhalten.
Der gemeinsame Runtime-Build verknüpft GPL-3.0-Code aus N64ModernRuntime und
der öffentlichen PS5-Plattformschicht. Eigene PS5-Adapter dieses Kits sind
GPL-3.0-or-later. Die zusätzlichen Quellen (Decomp, ELFIO, fmt, Rabbitizer,
SLJIT, toml++, GekkoNet, Monocypher, Mbed TLS, miniz, o1heap, xxHash und xz)
sind in `GAME_SOURCES.json` festgelegt; ihre Originalhinweise werden unverändert
bezogen. GNU binutils ist ein lokales GPL-Buildwerkzeug. IDO-static-recomp wird
aus dem öffentlichen decompals-Release als Host-Werkzeug geladen, nicht aus
einem Sony-SDK.

Der öffentliche Export enthält keine ROM, keinen extrahierten Spielinhalt,
keine generierten Spiel-CPU-Payloads und keinen Spiel-Installer. Im privaten
Spielbuild werden zusätzlich die von Upstream bereitgestellten Interface-
Assets und Fonts verwendet; deren Hinweise stehen im Upstream-Quellstand.
Eine ROM-freie Veröffentlichung gewährt keine Rechte am Originalspiel.

`src/ps5_game_crt.cpp` ist eine Startdiagnose-Anpassung von
`tooling/native/app_crt.cpp` des gepinnten PS5_Vulkan-Buildsupports
(Copyright 2026 BlackBearReloaded, GPL-3.0-or-later).
`src/ps5_getcwd.c` übernimmt die Verzeichnistraversierung aus dem öffentlichen
PS5_PayloadSDK, Commit `b5efad528e0ac1b8289f39d72a0e890ff68ff0a6`,
`platform/src/directory.c`, SHA-256
`b092b6a628b9a9e9852da6b0995d848e871a7d7f91602b4da7b1a7e9277deed9`.
Copyright 2026 Mihawk und GPL-3.0-or-later bleiben im Adapter erhalten.
Die Integration bindet die Standardnamen an die bereits gepinnte
Verzeichnisschicht, nutzt `PATH_MAX` und erhält Fehler von `stat`.
Die übrige SDK-Version und der bestätigte Renderer werden dafür nicht ersetzt.

## Öffentlicher PS5-Selbstbau ab 13. Oktober 2026

Ein unterstützter eigener US-Dump genügt. Die PS5-Ziele sind nach v77/v80
konfigurierbar; nur gewählte CPU-Payloads werden verlinkt. Dies ändert keine
Rechte am Originalspiel. Der Repository-Export enthält keine erzeugten
Spiel-Payloads, ROM-Daten oder das vom Nutzer gelieferte SR.GU-Texturenpaket.

Die PS5-Oberfläche verwendet ausschließlich die unveränderten Selawik-Faces
(Regular/Semibold) von Microsoft unter SIL OFL 1.1. Font-Bytes werden aus dem
gepinnnten DKR-R-Quellstand lokal gebaut. Racing Banana und Jumpman sind von
unserem PS5-Linkziel ausgeschlossen. Die OFL samt Copyright-Hinweis liegt unter
`licenses/DKR-R-Selawik-OFL.txt` und im privaten App-Build unter `assets/licenses/`.
Die GPL-, MIT-, LLVM-, SDL- und libsamplerate-Hinweise werden ebenfalls beigelegt.
Die Lizenztexte der weiteren Abhängigkeiten bleiben in ihren gepinnten
Originalquellen. Für eine eventuelle Binary-Verteilung ist das vollständige
Quellen-/Hinweispaket des tatsächlich verlinkten Stands zusätzlich zu prüfen.

Die aktuelle Titelgrafik-Rezeptur verwendet das DKR-R-Launcher-Icon und die
Originalgrafik aus dessen README. SHA-256, Bezugsquellen und Upstream-Credits sind
in `TITLE_ART_SOURCES.json` festgelegt. Die Anwendungsgrafik wird Upstream zufolge
POOTERMAN zugeschrieben. Diese Bilder behalten die Upstream-/Künstlerbedingungen;
die frühere CC0-Notiz für eigene generische Entwürfe gilt nicht für sie. Sie werden
als öffentliche lokale Build-Eingaben bezogen, nicht als Titelbilder im Source-ZIP
mitgegeben. Frühere generierte Bilder und extrahierte Modellversuche sind vom
aktuellen Export ausgeschlossen.

Bundled N64ModernRuntime and RT64 source archives omit upstream contributor-instruction documents. Build source and original copyright/licence files are retained unchanged; archive hashes record the repack.
