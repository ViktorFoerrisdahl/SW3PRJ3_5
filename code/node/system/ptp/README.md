# PTP på mikrofonnoderne

`PtpService` starter og stopper lokale systemd-tjenester for `ptp4l` og
`phc2sys`. Begge mikrofonnoder bruger samme klasse med forskellig `PtpRole`.
Den centrale node bruger PTP-status til lokalisering uden selv at starte PTP-tjenester.

```cpp
PtpService ptp(PtpRole::Slave);
if (!ptp.start())
{
    std::cerr << "Could not start PTP services.\n";
    ptp.stop();
    return 1;
}
// Run the node's work here.
if (!ptp.stop())
{
    std::cerr << "Could not stop PTP services.\n";
    return 1;
}
```

`start()` starter tjenesterne og returnerer, før urene nødvendigvis er låst.
Kald `readOffsetBudgetNs(offset_ns)` for at læse aktuel status. Ved gyldig status returnerer funktionen `true` og skriver offsetbudgettet i `offset_ns`. Klassen opretter ingen
 baggrundstråd. `start()` og `stop()` returnerer
`true`, hvis systemkommandoen lykkes, ellers `false`. Controlleren håndterer
fejlen og kalder `stop()` ved nedlukning. Hvis opstarten fejler, kan den ene
tjeneste være startet; kald derfor også `stop()` i det tilfælde.
Brug ét objekt pr. node. Stop statusaflæsningen, før du stopper tjenesterne.

## Opsætning på Raspberry Pi

Runtime kræver Linux med systemd, `eth0`, en PTP Hardware Clock og linuxptp.
Kommandoerne nedenfor køres fra repoets rod på **begge** mikrofonnoder:

```bash
sudo apt update
sudo apt install linuxptp ethtool cmake g++
sudo install -m 644 code/node/system/ptp/systemd/*.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo ethtool -T eth0

cmake -S . -B build
cmake --build build --target ptp_node
```

Stop tidligere manuelle `ptp4l`- og `phc2sys`-processer før opstart, så flere
processer ikke forsøger at regulere samme ur. Ethernet-adresserne skal være
forskellige; opstillingen bruger `172.16.15.2/16` og `172.16.15.3/16`.

På **slaven** slås systemets NTP-synkronisering fra, da `phc2sys` skal styre
systemuret:

```bash
sudo timedatectl set-ntp false
```

Kontrollér også eventuelle selvstændige `chronyd`- eller `ntpd`-tjenester.
Grandmasterens systemur er tidsreferencen; dens tid og annoncerede UTC-offset
skal være korrekt. `phc2sys -w` henter UTC-offset fra `ptp4l` frem for at
antage, at hardwareur og systemur bruger samme tidsskala.

Start grandmasteren først:

```bash
sudo ./build/code/node/system/ptp/ptp_node grandmaster
```

Kontrollér grandmasterens `phc2sys`-log i en anden terminal:

```bash
journalctl -u atlas-phc-grandmaster.service -f
```

Vent på låst servo (`s2` eller `s3`) og en stabil, lille offset. Start derefter
slaven på den anden Pi:

```bash
sudo ./build/code/node/system/ptp/ptp_node slave
```

Eksemplet starter tjenesterne og venter på Enter, som stopper dem igen.
Ctrl+C håndteres ikke; afbrydes programmet sådan, fortsætter tjenesterne.
Stop dem i så fald manuelt, eksempelvis på slaven:

```bash
sudo systemctl stop atlas-phc-slave.service atlas-ptp-slave.service
```

`sudo` bruges her til systemd-styring. Ved integration
kan programmet i stedet få specifikke systemd- og journaltilladelser, så hele
lydprogrammet ikke behøver at køre som root. Der installeres eller startes
ingen tjenester som del af CMake-buildet.

### Genstart af mikrofonnoden ved fejl

Når de fire PTP-tjenester ovenfor er installeret, kan systemd også køre
selve mikrofonnoden. Installér først `libasound2-dev`, og kør derefter:

```bash
cmake -S . -B build
cmake --build build --target atlas_node
sudo install -m 755 build/code/node/system/atlas_node /usr/local/bin/atlas_node
sudo install -m 644 code/node/system/systemd/atlas-node.service /etc/systemd/system/
sudoedit /etc/atlas-node.conf
```

Sæt `ORCHESTRATOR_IP` til den centrale nodes IP-adresse i konfigurationsfilen,
f.eks. `ORCHESTRATOR_IP=172.16.15.2`. Stop en eventuel manuelt startet
mikrofonnode, og kør `sudo systemctl daemon-reload` og
`sudo systemctl enable --now atlas-node.service`.
Følg loggen med `journalctl -u atlas-node.service -f`. Ved fejl genstarter
systemd programmet efter tre sekunder. Programmet venter igen på PTP og
forbinder til den centrale node.

## Manuel kontrol

Følg begge logs på slaven:

```bash
journalctl -u atlas-ptp-slave.service -u atlas-phc-slave.service -f
```

Se efter porttilstanden `SLAVE`, låst servo og stabile offsets i både
`ptp4l` og `phc2sys`. At tjenesterne er startet betyder ikke, at urene er låst.
PTP synkroniserer ikke ADC’ernes samplingure.

## Automatisk status og lokalisering

Kør `ptp_node grandmaster status` eller `ptp_node slave status`, mens
tjenesterne kører. Programmet udskriver nodens bidrag til den estimerede
urafvigelse i ns. Exitkode 2 betyder, at status ikke kan bruges.
Statuskontrol kræver adgang til journalen og `ptp4l`-tjenestens lokale
management-socket.

`readOffsetBudgetNs(offset_ns)` kræver den forventede porttilstand og en frisk
servoaflæsning fra den aktuelle tjenestekørsel. Den bruger højst tre
sekunder gamle loglinjer og accepterer servotilstand `s2` eller `s3`.
Grandmasteren bidrager med den absolutte afvigelse mellem systemuret og
PTP-hardwareuret. Slaven bidrager også med den absolutte afvigelse mellem
hardwareurene, som `ptp4l` estimerer.

Når `atlas_node` kører, aflæser `NodeTCP` PTP-status én gang i sekundet i
en særskilt tråd. Den opretter en `PTP_STATUS`-besked med `node_id`,
`offset_budget_ns` og `synchronized`. Kun TCP-tråden skriver til socketten;
den sender den seneste status og lydhændelser som hver sin JSON-linje. Dermed
blokerer en systemkommando til statuskontrol ikke afsendelsen af lydhændelser,
og to tråde kan ikke blande deres JSON-beskeder.

Orchestratoren registrerer modtagelsestidspunktet med sit monotone ur og
gemmer den seneste status fra hver node. Den markerer status som afbrudt ved
tab af TCP-forbindelsen. `TdoaThread` kalder `Ptp_ComputePosition()` før den
almindelige TDOA-beregning. Funktionen afviser lokalisering, hvis en node er
afbrudt, hvis status mangler, er ugyldig eller er over to sekunder gammel,
eller hvis det samlede offsetbudget overstiger 200 µs. Fristerne på tre og to
sekunder er valgt i implementeringen.

Den centrale nodes terminal viser hver modtaget status med nodens ID og
offsetbudget, når noden er synkroniseret. Terminalen melder også, når begge
noder er klar, og hvorfor lokalisering sættes på pause ved udfald. Ved afviste
hændelsespar gemmer JSON-loggen hændelsens tidspunkt og en `rejection_reason`.

Statusforløbet skal stadig afprøves på to Raspberry Pi-enheder: afbryd PTP
eller Ethernet og kontrollér, at lokaliseringen stopper, og at den først
genoptages efter frisk, gyldig status fra begge noder. Softwaretesten er ikke
en hardwarevalidering af denne adfærd.

## Softwaretest

```bash
cmake --build build --target ptp_service_test ptp_localization_test
ctest --test-dir build -R ptp --output-on-failure
```

Testene kan køres på macOS og Linux. Servicetesten kører `ptp_node`
med simulerede systemkommandoer til start, stop og statuskontrol.
Lokaliseringstesten kontrollerer JSON-formatet, grænsen på 200 µs samt
afvisning ved forbindelsestab, forældelse og manglende aflæsning.

## Kilder

- [ptp4l](https://www.linuxptp.org/documentation/ptp4l/): hardwarefilter, roller og management-sockets.
- [phc2sys](https://www.linuxptp.org/documentation/phc2sys/): urretning og UTC/PTP-offset.
