

## TDOA lokalisering af sound event
**Mål:** distribueret system der bestemmer positionen af et klap i et 2D-plan(3D-plan?) ud fra tidsforskelle mellem mikrofoner

## Konceptet
Fire mikrofoner, to på hver Raspberry Pi. Et klap rammer dem på forskellige tidspunkter. 4 mics -> 3 hyperbler -> 3 ligninger 2 ubekendte -> lstsq -> skæringspunktet er positionen. Distribueret constraint ($\ge2$ pi's) er ikke forced: uden fysisk adskilte mikrofoner er der ingen tidsforskel at måle.


## Opstilling
- $1.5m^2$ , mikrofoner i hjørnerne
- 2x raspberry pi's med 2x mics (kort kabel > 30cm, for at sikre timing sikkerhed)
- Pi's forbundet via ethernet.
- Faktisk muligt med kun 2 pi's, hvor én af dem også står for beregningerne -> mere interresant med 3, hvor timestamps streames til en buffer på "beregner pi'en"

To mikrofoner på samme Pi deler samplingsklokke -> tidsforskellen inden for et par er præcis uden synkronisering.
- 
- eller 3D (stort set samme koncept, bare skaleret op med endu en dimension)

## Pi1 controller
- Modtager timestamps, samt data om lydevent fra de 2 andre pi's.
- Beregner TDOA ud fra disse time stamps
- styrer clock?
- Datastreaming(ringbuffer etc, til data som kommer fra 2 pi's)
- Lille pegepind med steppermotor, som peger mod sound-event?


## Mics controller (2 pi's)
- Opfanger sound event, og streamer det til controller PI'en
- Synkroniserede clocks mellem alle pi's.
- Threading, for at sikre at der ikke er noget forsinkelse.
- Timing kritisk kode. Vi er nede og skal sørge for at så lidt forsinkelse som muligt, påvirker måleresultaterne. Performant kode på de 2 mics controllers.

## Sværeste dele af projektet
- **Digital signalbehandling:** Skal undersøges hvordan vi skal opfange dette sound event. Hvis vi skal finde et klap, er 'transient event' nok det der skal opfanges. Research tung del af projektet, og meget arbejde her. Hvis det ikke er et transient event, skal det undersøges hvordan samme del på sinuskurven måles ved f.eks 250hz drone støj
- **KNP:** KRITISK at clocks er synkroniserede. UDP Protokol til at holde clocks synkroniserede altid. Ved ikke hvor svært. Andre muligheder end UDP kan undersøges
- **SYS:** i forlængelse af KNP. ALLE MICS SKAL VÆRE SYNKRONISEREDE. Derfor kan vi ikke kører polling based/sekventiel afvikling af programmer, vi skal multithreade meget.
- **DSA:** Datastreaming(ringbuffer). Ved ikke om DSA kommer til at bruges når TDOA beregnings algoritmen skal skrives??

## Ansvarsområder?
1. Lydopsamling og mic setup
2. Hændelsesdetektion
3. Synkronisering og concurrency
4. Netværk og datastreaming
5. lokaliseringsløser (TDOA)


## Todo
- bestil mikrofoner
- aftal hvordan data streames (json format, hvilke felter etc) så folk kan arbejde med antagelser
- simulator skal levere testdata hurtigt, så vi kan arbejde med mock data, inden systemet kører.





en server node løser TDOA multilateration og viser positionen.
	- DSB(Båndpas filter, envelope, cross-correlation).
		- Båndpas filter til et snævert frekvensområde hvor "klapppet" er i. Sound event hvis over en hvis DB?
	- ALG(Multilateration, least squares løsning af ligningssystemet)
	- KNP(UDP + Websocket)
	- Største arbejde bliver clock-sync (UDP), da timing er utrolig kritisk. Vigtigt med god concurrency, ellers dur det ikke.
	- MQTT broker for UDP? fjollet


