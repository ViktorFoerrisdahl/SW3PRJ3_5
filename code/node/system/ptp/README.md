# PTP på mikrofonnoderne

`PtpService` starter og stopper lokale systemd-tjenester for `ptp4l` og
`phc2sys`. Begge mikrofonnoder bruger samme klasse med forskellig `PtpRole`.
Orchestratoren behøver ikke modulet.

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
Brug ét objekt pr. node, og kald metoderne fra samme controllertråd.

## Opsætning på Raspberry Pi

Runtime kræver Linux med systemd, `eth0`, en PTP Hardware Clock og linuxptp.
Kommandoerne nedenfor køres fra repoets rod på **begge** mikrofonnoder:

```bash
sudo apt update
sudo apt install linuxptp ethtool cmake g++
sudo install -m 644 code/ptp/systemd/*.service /etc/systemd/system/
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
sudo ./build/code/ptp/ptp_node grandmaster
```

Kontrollér grandmasterens `phc2sys`-log i en anden terminal:

```bash
journalctl -u atlas-phc-grandmaster.service -f
```

Vent på låst servo (`s2` eller `s3`) og en stabil, lille offset. Start derefter
slaven på den anden Pi:

```bash
sudo ./build/code/ptp/ptp_node slave
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

Orchestratoren kan kalde `Ptp_ComputePosition()` fra biblioteket
`ptp_localization` for et par hændelser fra de to konfigurerede noder.
Datastreamingmodulet skal sende status fra begge noder, også mellem
lydhændelser, og melde ugyldig status ved udfald. Den centrale node bruger
sit monotone ur til at registrere modtagelsen og markerer noden som
afbrudt ved forbindelsestab. Funktionen afviser lokalisering, hvis en
node er afbrudt, hvis dens status mangler, er ugyldig eller er over to
sekunder gammel, eller hvis det samlede budget overstiger 200 µs.
Fristerne på tre og to sekunder er valgt i implementeringen.

Koblingen til den centrale node og transporten af status hører til
datastreamingmodulet. Vi tester dem sammen på to Raspberry Pi-enheder.
Her kontrollerer vi P2 med den tidsreference, lydoptagelsen bruger, og
ser efter, om lokaliseringen stopper ved udfald og genoptages, når
begge noder igen har gyldig status. Frigiv kun hændelser optaget under
gyldig synkronisering.

## Softwaretest

```bash
cmake --build build --target ptp_node ptp_localization_test
ctest --test-dir build -R ptp --output-on-failure
```

Testene kan køres på macOS og Linux. Servicetesten kræver Python 3 og
simulerer systemkommandoerne til start, stop og statuskontrol.
Lokaliseringstesten kontrollerer grænsen på 200 µs og afviser status
ved forbindelsestab, forældelse og manglende aflæsning.

## Kilder

- [ptp4l](https://www.linuxptp.org/documentation/ptp4l/): hardwarefilter, roller og management-sockets.
- [phc2sys](https://www.linuxptp.org/documentation/phc2sys/): urretning og UTC/PTP-offset.
