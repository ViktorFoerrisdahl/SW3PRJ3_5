# Referat #3 - Møde med søren 150926.docx

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Referater/Referat #3 - Møde med søren 150926.docx](../Referater/Referat%20%233%20-%20M%C3%B8de%20med%20s%C3%B8ren%20150926.docx)
- SHA-256: `0a96716fe7b37ea56aeb3b0562198470df090504992d1ff8eb069b01217202f2`
- Status: tekst udtrukket – kontrollér original

> Automatisk tekstudtræk: kontrollér layout, formler og tabeller i originalen.

---

- Bekymret for implementeringen med ptp.

- Hvis vi lykkedes med at implementere synkroniseret clocks, minder resten om nogle af de projekter der er lavet tidligere -\> andre krydskorrelerings opgaver med lokalisering.

- Mikrofoner kan vi bruge små dumme mikrofoner, hvis vi laver et kredsløb med forstærker og få modstande

- Ellers kan en standard mikrofon bruges.

- Ift datastreaming har vi ikke et skarpt reeltidskrav, men der skal alligevel tænkes godt igennem hvordan vi håndterer dataopsamling, og hvordan vi behandler samples. En ringbuffer som der overskriver ældste værdier, og så en event tærskel, som så ”klipper” noget af bufferen ud og sender til proccessing.

- Til fredag:

  - Mads undersøger mikrofoner. Hvad giver mening? Ingen ADC på pi, skal være en hat hvis analog mikrofon. Er digital mikrofon overhovedet muligt? Mere forsinkelse?

  - Frederik og Alexander vil forsøge at synce clocken på to raspberry pi’s, for at få et indtryk af hvor kompliceret det er. Hvis det er helt galt, må vi implementere systemet som en enkelt pi med 4 mikrofoner.
