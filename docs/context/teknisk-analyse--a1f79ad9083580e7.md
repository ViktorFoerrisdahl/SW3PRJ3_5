# Teknisk Analyse.docx

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Elaboration E2/Teknisk Analyse.docx](../Elaboration%20E2/Teknisk%20Analyse.docx)
- SHA-256: `ba291fbfa5fa44d84bd4d8e924fda6be96cdef7cbca71c7ada47b48eb92ea030`
- Status: tekst udtrukket – kontrollér original

> Automatisk tekstudtræk: kontrollér layout, formler og tabeller i originalen.
> 1 illustration(er) skal læses i originalen; billedindhold er ikke konverteret.

---

# Teknisk Analyse

## Mikrofon/hændelsdetektering:

## PTP:

##  

## Datastreaming: Mads

Med en sample rate på 44 KhZ er det 44,000 samples per sekund. Hvis vi antager at hver frame (2 samples fra de 2 mikrofoner per pi) er 32 bit (16bit per frame), og en ringbuffer på 10 sekunder, koster vores ringbuffer 3,840,000 bytes (3.84 MB).

Programmet skal hente ”bidder” af optagelsen løbende, og analysere de her bidder. Derfor skal vi have en buffer hvor de seneste samples ligger klar til analyse i en buffer.

En ringbuffer har et fast antal pladser. Når hukommelsen er fuld, overskriver vi de ældste data heri. Derfor har ringbufferen altid ”de seneste x minutter/sekunder”. Aldrig mere.

Chatten har lavet:

<a href="../Elaboration%20E2/Teknisk%20Analyse.docx" width="6.268055555555556in" height="2.41875in">Illustration: se originaldokumentet</a>

Som det kan ses, har vi wrap around ved sample nummer 8. Altså efter 0-7 er udfyldt, starter vi forfra ved index 0, og ligger sample 8 ind her.

Vores buffer skal tilgås af 2 dele af programmet:

- Én der skriver til bufferen

- Én der læser fra bufferen

Hver del har en pointer til arrayet. En write pointer, og en read pointer. Read pointeren skal altid være bag write pointeren, for der skal ikke kunne læses data, som ikke er blevet skrevet endnu. Read pointeren må heller ikke være så langt bagud, at write pointeren når at wrappe around, og overskrive data som read pointeren ikke har læst endnu.

ALSA er den del af linux som håndterer lyd streaming. Vores program interagerer ikke med hardwaren, men beder kernel om ALSA data via et syscall.

- ALSA leverer lyden i periods det kunne fx være 512 frames af gangen

- ALSA har sin egen ringbuffer, derfor er det VIGTIGT, at vores program henter data i tide, ellers overskriver ALSA ældste data inden vores program fanger det.

Vi skal bruge 2 ring buffers, fordi ALSA’s buffer er lille (måske 100 ms), og derfor flytter vores program data derfra, over i en større ringbuffer (måske 2 s).

Flowet er:

- Mikrofonerne sampler lyden

- Hardwaren ligger samples i ALSA bufferen

- Vores program tager en period fra ALSA bufferen, og lægger den over i vores egen buffer

- Detektoren læser fra ring bufferen, og leder efter et klap.

Hvis vi tæller alle samples siden start, bliver tælleren et ur: sample nr 48000 er præcis 1 sekund efter sample nr 0. De to mikrofoner på samme pi sampler på præcis samme tidspunkter, så sample nr. k betyder samme øjeblik på begge mikrofoner. Det samme gør sig gældende over de 2 forskellige pi’s.

Når klappet identificeres ved sample 1040 på den ene mikrofon, og klappet identificeres ved sample 1000 på den anden mikrofon, har vi der en tidsforskel på 40 samples, altså 40/48000 = 0.83 ms. Det er den tidsforskel hyperbler laves til TDOA algoritmen.

## TUI: 

## Pegepind og motor: 

### **Mulighed 1 - Servomotor**

Der bruges Servomotor, der peger på en bestemt vinkel, som bestemmes af systemet. Vinklen kan beregnes ud fra formlen

$$\theta = atan2(y - y_{m},x - x_{m})$$

Hvor $x_{m},y_{m}$ er servoens placering og $(x,y)$ er den estimerede lydposition. Problemet med denne formel er, at det kun er servomotorer uden continuos rotation, der rigtigt kan gøre brug af vinkelstyring og 360 graders servomotorer uden continuos rotation er sværere at finde. Motorer med continuos rotation gør brug af PWM signal til at styre rotationsretning og hastighed, så der skal bruges en encoder før at vinkelberegningen bliver en realitet, hvilket er en ekstra kompleksitet. Alternativt kan der muligvis bruges 2x $270{^\circ}$ servomotorer i et system som skaber fuld $360{^\circ}$ rotation.

### **Mulighed 2 - Steppermotor**

Stepper motor, der roterer med et vis antal steps. Denne har typisk altid 360+ graders rotation, men den har ofte ikke positionsfeedback, hvilket ligesom tidligere skaber ekstra kompleksitet.

### **Pegepind**

Der bruges en KY-008 laser module som pegepind.

## TDOA-algoritme: 

##
