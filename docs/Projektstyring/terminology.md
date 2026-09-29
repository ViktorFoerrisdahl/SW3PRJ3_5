# Terminologi og skrivemåde

Denne liste fastlægger fælles sprogbrug. Definitionerne er sproglige konventioner, ikke nye arkitektur- eller hardwarebeslutninger. Kode, kodekommentarer og commitbeskeder skrives på engelsk; dokumentation skrives på dansk.

## Projektets begreber

| Foretrukken betegnelse i dokumentation | Engelsk term i kode og kommentarer | Betydning og brug |
| --- | --- | --- |
| ATLAS | ATLAS | Acoustic TDOA Localization and Alert System. Brug store bogstaver. |
| lydkilde | sound source | Den fysiske kilde, som udsender et akustisk signal; i proof of concept et klap. |
| akustisk signal | acoustic signal | Lyden før systemet har afgjort, om den indeholder en relevant hændelse. |
| lydhændelse | sound event | En hændelse identificeret i det optagne signal. Undgå skift mellem lydevent, sound-event og lydbegivenhed. |
| hændelsesdetektering | event detection | Afgørelse af, om en relevant lydhændelse findes, og bestemmelse af dens tidspunkt. |
| klassifikation | classification | Bestemmelse af lydkildens type. Er ikke det samme som hændelsesdetektering. |
| mikrofonnode | microphone node | En node, der opsamler lyd fra tilsluttede mikrofoner. Antal mikrofoner afhænger af den dokumenterede konfiguration. |
| central node | central node | Rollen, der samler data og beregner position. Rollen betyder ikke nødvendigvis en ekstra fysisk Raspberry Pi. |
| Raspberry Pi | Raspberry Pi | Produktnavnet. Brug »Raspberry Pi-enheder« i flertal; undgå rapsberry og pi's. |
| TDOA | time difference of arrival | Forskel i et signals ankomsttid mellem to mikrofoner. Projektformuleringens »Time-Delay of Arrival« normaliseres sprogligt hertil. |
| lokalisering | localization | Estimering af lydkildens position; en position er et estimat med måleusikkerhed. |
| multilateration | multilateration | Positionsberegning ud fra afstandsforskelle eller tilsvarende tidsforskelle. Brug ikke triangulering som synonym. |
| mindste kvadraters metode | least squares | En løsningsmetode, der minimerer en sum af kvadrerede fejl. |
| krydskorrelation | cross-correlation | Sammenligning af to signaler ved forskellige tidsforskydninger. |
| timestamp | timestamp | Et tidspunkt knyttet til en eksplicit tidsreference. Brug ikke modtagelsestid som synonym for optagelsestid. |
| ursynkronisering | clock synchronization | Tilpasning af ure mellem noder. Angiv, om der menes systemur eller samplingur. |
| PTP | Precision Time Protocol | Protokol til ursynkronisering. Skriv linuxptp om softwarepakken og `ptp4l` om programmet. |
| samplingur | sample clock | Uret, der bestemmer optagelsens samplingtidspunkter; skeln fra systemuret. |
| sample | sample | Én måleværdi fra én kanal. Flertal: samples. |
| frame | frame | Ét sample fra hver kanal ved samme samplingtidspunkt. Antal bytes afhænger af kanaler og sampleformat. |
| samplingfrekvens | sample rate | Antal samples pr. sekund pr. kanal, angivet i Hz. En bestemt værdi er ikke fastlagt af denne liste. |
| PCM | pulse-code modulation | Digital repræsentation af signalværdier; bitdybde og pakning skal angives særskilt. |
| ringbuffer | ring buffer | Buffer med fast kapacitet og cirkulær indeksering. Overskrivningspolitikken skal beskrives. |
| ALSA | Advanced Linux Sound Architecture | Linux' lydinfrastruktur. `libasound` er bibliotekets navn; `libasound2` er et pakkenavn. |
| I²S | I2S | Digital lydgrænseflade. Brug I²S i løbende dokumenttekst og I2S i kode. |
| MEMS-mikrofon | MEMS microphone | Mikrofon baseret på mikroelektromekanisk teknologi. Brug I²S MEMS-mikrofon om den type, mikrofonanalysen vælger. |
| TUI | text-based user interface | Tekstbaseret brugergrænseflade. Må ikke bruges som synonym for alle former for visualisering. |
| proof of concept | proof of concept | Afgrænset demonstration af princippets gennemførlighed; forkortes PoC efter første forklaring. |

## Skriveregler

- Forklar forkortelsen første gang i et selvstændigt dokument: »tidsforskel mellem ankomsttidspunkter (TDOA)«.
- Brug konsekvent dansk i den forklarende tekst, også når et fagudtryk er engelsk. Originalcitater og kildetitler bevares.
- Skriv `2D` og `3D`, `Ethernet`, `JSON`, `UDP`, `TCP` og `C/C++` konsekvent.
- Brug mellemrum mellem tal og enhed: 48 kHz, 16 bit, 2 ms og 3 × 3 m. Hz og kHz har denne kapitalisering; skriv ikke KhZ.
- Dansk løbende tekst bruger decimalkomma, f.eks. 0,83 ms. Kode og maskinlæsbare data bruger formatets sædvanlige decimalpunkt.
- Angiv reference for dB-værdier, f.eks. dB SPL eller dBFS. De to størrelser er ikke udskiftelige.
- Brug datoformatet YYYY-MM-DD i nye filnavne og maskinlæsbare data. Historiske filnavne bevares.
- Variabelnavne angiver relevante enheder, f.eks. `sample_rate_hz`, `timestamp_ns` og `position_x_m`.

## Kildegrundlag og åbne spørgsmål

Listen bygger på projektformuleringen i `Inception/`, referat #4 og analyserne i `Elaboration E2/`. Betegnelser fra et tidligere chatresumé, f.eks. ToI og Orchestrator, gøres ikke til vedtagne komponentnavne uden belæg i de relevante projektfiler og diagrammer.

Projektformuleringens Product Goal udelader klassifikation, mens hændelsesdetekteringsanalysen omtaler at afgøre, om lyd kommer fra en drone. Dette skal afklares fagligt; fælles ordvalg løser ikke konflikten.
