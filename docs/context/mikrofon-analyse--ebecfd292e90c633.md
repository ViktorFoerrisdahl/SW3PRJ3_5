# Mikrofon_Analyse.docx

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Elaboration E2/Mikrofon og Hændelsesdetektering/Mikrofon_Analyse.docx](../Elaboration%20E2/Mikrofon%20og%20H%C3%A6ndelsesdetektering/Mikrofon_Analyse.docx)
- SHA-256: `7e0d31c9bcfa524cb5abdcdfbe167358fce1034b425b9ff7b2540c6fb4e6330e`
- Status: tekst udtrukket – kontrollér original

> Automatisk tekstudtræk: kontrollér layout, formler og tabeller i originalen.

---

# Sammenligning Matrix

| Kriterie/Krav         | Analoge Mikrofoner                                                                             | I2S MEMS Mikrofoner                                                                                                       | USB Mikrofoner                                                                                                    |
|-----------------------|------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------|
| TDOA Præcision        | **Høj:** Næsten nul analog faseforsinkelse; En ADC vil sample synkront på hardware             | **Høj:** Altid fast forsinkelse; Stereo sample styring låser sykronsering.                                                | **Kritisk / Dårlig:** Software-buffere og asynkrone USB-clocks giver uforudsigelig jitter over $200\ \mu\text{s}$ |
| Lydtryk Opfyldt       | Kræver manuel kalibrering af analog forstærker for at undgå mætning eller for svagt signal     | **Opfyldt direkte:** Typisk AOP på $120\text{~dB~SPL}$ og følsomhed på $- 26\text{~dBFS}$; fanger let $75\text{~dB}$ klap | Varierer kraftigt afhængigt af automatisk gain control (AGC), som kan forvrænge transienter.                      |
| Hardware kompleksitet | **Høj:** Kræver operationsforstærkere, passive filtre, modstande, kondensatorer og ekstern ADC | **Lav:** Kræver blot 5 GPIO-forbindelser (3V3, GND, BCLK, WS, SD) direkte til RPi pr. par                                 | **Meget lav:** Plug-and-play USB-stik                                                                             |
| Opfylder 3x3 m felt   | **Dårlig:** Analoge kabler på op til 3 meter samler masser af elektromagnetisk støj og brum.   | **God:** Signal digitaliseres lokalt ved chippen; robust datastrøm (kræver blot ordentlig kabling).                       | **God:** Digital transmission, men begrænset af USB-kabellængder uden aktive repeatere.                           |
| Integration i C/C++   | Kræver SPI-driver eller bit-banging i C++ for at læse ekstern ADC                              | **Høj standardisering:** Standard Linux ALSA SoC-driver (libasound2) læser PCM-frames direkte                             | Standard ALSA, men bøvlet at holde to separate USB-enheder i fase                                                 |

# Valg af Mikrofon type: I2S MEMS

## Bibliotek: ”libasound2”
