# Hændelsesdetektering.docx

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Elaboration E2/Mikrofon og Hændelsesdetektering/Hændelsesdetektering.docx](../Elaboration%20E2/Mikrofon%20og%20H%C3%A6ndelsesdetektering/H%C3%A6ndelsesdetektering.docx)
- SHA-256: `d9dd00fb0c101d0fc2743385a3b2255e4f1a0b76acd560ce39239eec92f5cdf5`
- Status: tekst udtrukket – kontrollér original

> Automatisk tekstudtræk: kontrollér layout, formler og tabeller i originalen.

---

# Hændelsesdetektering

Når der registreres en lydhændelse i mikrofonerne, skal der afgøres om lyden kom fra en drone eller ej samt hvor den kom fra. Da vi ved hjælp af TDOA (henvis til TDOA analyse) bestemmer præcis hvor lyden kom fra. Det vil sige at hændelsesdetekteringen skal afgøre om der kom en lyd fra en “drone” samt hvornår lyden ankom til hver mikrofon. Dette er altså de to problemer som skal løses:

1.  At afgøre om lyden er kommet fra en drone

2.  At bestemme hvornår lyden ankom til hver mikrofon

## At bestemme om lyden er kommet fra en drone

Lyden kommer løbende fra vores datastream (henvis til datastream) i en ringbuffer der konstant opdateres. Til hændelsesdetekteringen tilgås bufferen via dens read pointer. Hvert index indeholder 16-bit rå signed PCM-data med talværdi mellem -32768 og 32767.

### PCM tærskel

Vi vil registrere lyden som et klap hvis den overstiger 75 dB, det er dyrt og unødvendigt at omregne for hver rå værdi, så derfor skal der på forhånd udregnes en værdi i PCM-data for vores tærskel.

Vi starter med at finde fuld skala for den digitale repræsentation:

$$A_{fullscale} = 2^{(N - 1)} - 1$$

Her er N antallet af bits i mikrofonens output. Det afhænger både af mikrofonen og hvordan driveren pakker vores samples.

Herefter vil vi finde den rå amplitude ved databladets reference lydtryk:

$$A_{ref} = A_{fullscale} \cdot 10^{\frac{S_{dBFS}}{20}}$$

Her er $S_{dBFS}$ mikrofonens sensitivity spec fra databladet.

Nu vil vi finde den rå PCM-tærskel svarende til de 75 dB:

$$THRESHOLD_{RAW} = A_{ref} \cdot 10^{\frac{SPL_{target} - SPL_{ref}}{20}}$$

Her er $SPL_{target}$ den tærskel i dB vi har bestemt (75 dB), og $SPL_{ref}$ er det lydtryksniveau sensitiviteten er målt ved - typisk angivet i databladet.

Disse 3 skridt kan samles i en formel:

$$THRESHOLD_{RAW} = (2^{N - 1} - 1) \cdot 10^{\frac{S_{dBFS} + (SPL_{target} - SPL_{ref})}{20}}$$

### Tjek om lyd er oversteget tærskel

Da et klap er en meget kort transient, skal RMS-vinduet ikke være alt for stort, vi vil gerne have et 2-5 ms vindue til at vurdere om en hændelse har fundet sted:

$$N \approx 2 - 5\ ms \Longrightarrow 100 - 250\ samples\ ved\ 44,1\ kHz$$

Altså vil vi gerne læse mellem 100-250 samples fra ringbufferen ad gangen. Vi tager altså positionen af read pointeren og kigger på bidden i mellem dens eget indeks i bufferen, og dens indeks + antallet af samples. Hvis dette skulle struktureres i et array, ville det se således ud:

$$\lbrack readpointer,readpointer + 200\rbrack\ for\ N = 200\ samples$$
