# Veröffentlichungsprüfung: PS5-Integration für DKR-R 1.0.6

Stand: 13. Oktober 2026. Dies ist eine technische Lizenz- und Inhaltsprüfung,
keine verbindliche juristische Freigabe des Originalspiels.

## Sinnvoller erster Veröffentlichungsumfang

Ein eigenständiges Integrationsrepository mit unseren PS5-Adaptern, Patch-Dateien,
Build-Anleitung, Tests, exakten Quellenrevisionen und Lizenzhinweisen. Der Export
bezieht DKR-R aus seinem gepinnten öffentlichen Quellstand. Er enthält dessen
Schriftdateien und generierte Spielprogramme nicht. Jeder Nutzer erzeugt den
Spielbuild lokal mit einem eigenen, rechtmäßig nutzbaren Dump einer unterstützten US-Revision.
Die Bezeichnung lautet zunächst „experimental PS5 source preview“.

Originaler DKR-R-Projektcode ist MIT-lizenziert. N64ModernRuntime und Teile der
öffentlichen PS5-Laufzeit sind GPL-3.0; eigene Adapter sind GPL-3.0-or-later.
Die Lizenzen sind komponentenbezogen. Ein globaler LICENSE-Text ersetzt fremde
Copyright-Hinweise nicht. Für verteilte kombinierte ausführbare Programme gelten
zusätzlich die Anforderungen an korrespondierende Quellen des tatsächlich
verteilten Stands. Unser Source-Export ist kein Nachweis, dass ein beliebiges
ROM-abgeleitetes Spiel-Binary frei verteilt werden darf.

## Bereinigter PS5-Interface-Stand

Die PS5-Konfiguration linkt jetzt ausschließlich Selawik unter SIL OFL 1.1.
Racing Banana und Jumpman wurden aus dem PS5-Linkziel und dem nativen Font-Loader
entfernt. Beide einzelnen Revisionen wurden damit vollständig gebaut; eine
Symbolprüfung bestätigt das Fehlen beider Font-Payloads. Originale OFL- und
Copyright-Hinweise werden dem privaten App-Build beigelegt. Unser Repository
enthält keine DKR-R-Fontbytes; der Builder bezieht den gepinnten Upstream-Stand.

Die folgenden Upstream-Bedingungen bleiben für andere Varianten, einen
vollständigen DKR-R-Fork oder einen anders gebauten Binary-Download relevant.
Sie sind kein verbliebener Font-Blocker dieses Selawik-PS5-Linkziels.

## Grenzen eines späteren fertigen Spiel-Downloads

- **Racing Banana:** Upstream nennt eine vom Projektinhaber bereitgestellte
  Interface-Schrift und fordert öffentliche Verteiler ausdrücklich auf, die
  Weitergaberechte zu bestätigen. Die vorhandene Dokumentation liefert dafür
  keine allgemeine ausdrückliche Freigabe.
- **Jumpman:** Die beigefügte Pixel-Sagas-Lizenz unterscheidet persönlichen,
  nichtkommerziellen Gebrauch, direkte Font-Downloads und Einbettung in Software.
  Die Lizenz erlaubt ausdrücklich die Einbettung in Software; ein kostenloser
  PS5-Port ist damit nicht pauschal ausgeschlossen. Diese Bedingungen dürfen
  nicht als uneingeschränkte MIT/GPL-Freigabe ausgegeben werden. Ein öffentlicher
  Font-/Binary-Umfang muss diese Regeln gesondert erfüllen. Der vollständige
  Selawik-Wechsel war eine vorsorgliche Projektentscheidung, keine technische
  PS5-Notwendigkeit und kein Nachweis, dass die bisherigen Schriftfehler von
  Jumpman oder Racing Banana verursacht wurden.
- **Selawik:** Regular und Semibold sind mit SIL OFL 1.1 dokumentiert. Ein
  öffentlich weitergebbarer PS5-Interface-Stand kann diese Schrift verwenden,
  unter Beibehaltung der OFL und ihrer Namensbedingungen.
- **Aus ROMs erzeugter Code:** Die jetzige native eboot.bin und deren Installer
  enthalten lokal erzeugte Spiel-CPU-Funktionen. Sie bleiben im privaten
  Buildbereich. Ein fehlender ROM-Anhang ist keine automatische Freigabe dieser
  abgeleiteten Programme.
- **SR.GU-Paket:** Das vom Nutzer bereitgestellte Remastered-Paket wurde nur für
  seine eigene PS5 importiert. Im ZIP ist keine allgemeine Weitergabelizenz
  enthalten. Es gehört weder in dieses Repository noch in dessen Releases;
  andere Tester beziehen es selbst beim Autor nach dessen Bedingungen.
- **Interface-Bilder:** Bei einem späteren vollständigen DKR-R-Fork oder
  Spiel-Download müssen auch die Upstream-Interface-Bilder und ihre Provenienz
  geprüft werden. Die Titelgrafik-Rezeptur bezieht jetzt das ursprüngliche
  DKR-R-Launcher-Icon und die README-Grafik als gehashte öffentliche Build-Eingaben.
  Der Source-Export enthält Rezeptur und Provenienz, keine dieser Titelgrafikdateien
  und keine extrahierten Figurenmodelle. Upstream-/Künstlerrechte bleiben bestehen;
  es wird keine CC0-Freigabe für diese Bilder behauptet. Die vom Nutzer verworfenen
  generierten Motive und Modellversuche sind nicht Teil des aktuellen Exports.

## Inhalts- und Quellenprüfungen

Der öffentliche Export benutzt eine Positivliste, untersucht mitgelieferte
Quellarchive zusätzlich und vermeidet die private Arbeits- und Konsolenhistorie.
Ausgeschlossen sind ROMs, extrahierte Spieldaten, generierte CPU-Payloads,
Spiel-Binaries, Installer, Profile, Saves, Controller-Paks, das Remastered-Paket,
Übertragungstoken und Sony-Systemmodule. Ein solcher Scan beweist die geprüften
Dateieigenschaften; er beweist keine Herkunft oder Rechte für jeden denkbaren
Quellbestandteil.

Originalhinweise bleiben in den Quellarchiven erhalten. Zusätzlich werden die
lesbaren Lizenzdateien unter `licenses/` exportiert. Die Builds laden öffentliche
SDK-/GPU-Quellen; vorhandene Sony-Bibliotheken werden importiert und nicht kopiert.

## Betatester und Kontakt zum Upstream-Entwickler

Eine Anfrage an den Entwickler kann jetzt mit dem bestätigten Stand erfolgen:
DKR-R 1.0.6 wurde auf einer PS5 mit gemeldeter Firmware 13.60 und
Relapse/WebKit Autoloader 0.6.1/kstuff-lite/ShadowMount-Plus tatsächlich gespielt.
Bekannte offene Punkte sind einzelne Menüs/Schriften, das Adventure-Intro,
Speichern/Neustart und gelegentliche Mikroruckler. Die zusätzliche native Speicher-Metadatenanpassung hat den Neustartfehler
überwunden und erneut Spielausgabe erreicht. Vollständiges Save/Load sowie die
neue Controller-Oberfläche und der OFL-Schriftstand werden noch geprüft.
Kein Stabilitäts- oder Firmware-Kompatibilitätsversprechen ableiten.

Um technische Hilfe, Tests und eine Klärung der Interface-Schriftrechte bitten.
Die Zustimmung eines Open-Source-Entwicklers ersetzt keine Rechte von Nintendo,
Rare oder anderen Asset-Autoren. Keine ROMs oder private Builds an die Anfrage
anhängen. GitHub-Veröffentlichung und Nachrichtenversand sind noch nicht erfolgt.

## Primäre Grundlagen

- [DKR-R Runtime licensing notice](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/runtime-recomp/COPYING-NOTICE.md)
- [DKR-R Asset policy](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/docs/ASSET_POLICY.md)
- [DKR-R Third party notices](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/THIRD_PARTY.md)
- [SIL Open Font License](https://openfontlicense.org/open-font-license-official-text/)
- [§ 69e UrhG](https://www.gesetze-im-internet.de/urhg/__69e.html): Die dortige
  Interoperabilitätsausnahme hat konkrete Bedingungen und ist keine pauschale
  Erlaubnis zur Veröffentlichung rekonstruierter Spiele.
