# CNN Warehouse.md

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Inception/Noter/CNN Warehouse.md](../Inception/Noter/CNN%20Warehouse.md)
- SHA-256: `4109104e874302b3ee20755ef68ad14db421ac65c0372f9e509bbe0e5eb88f6c`
- Status: tekst kopieret

> Relative links i kildeteksten er relative til originalfilens mappe.

---

# CNN-lager

## Idé

Et lille lager bygges som en model, der ses ovenfra af et kamera. Lageret har 4–5 reoler, lagerpladser og tydelige gange.

CNN-modellen klassificerer hver lagerplads som enten **ledig** eller **optaget**. En controller bruger denne information til at håndtere simulerede ordrer og leveringer så effektivt som muligt.

## Systemoverblik

- **Kamera-Raspberry Pi:** Tager billeder og kører CNN-inferens.
- **Lager-controller Raspberry Pi:** Vedligeholder lagerkortet og modtager opdateringer om ledige og optagede pladser.
- **Ordresimulator:** Opretter simulerede ordrer og pakkeleveringer.
- **Rutefinding:** Bruger BFS eller A* til at finde en rute gennem gangene til den rigtige lagerplads.
- **Valgfri aktuator:** En LED, lille pegepind eller miniature-robot kan vise den planlagte rute.

```text
Kamera-Pi
    │ opdateringer om lagerpladser
    ▼
Controller-Pi ─── simulerede ordrer ─── Ordresimulator
    │
    └── korteste rute ─── lager-model / aktuator
```

Eksempel på besked:

```json
{
  "bay": "S2-3",
  "state": "occupied",
  "confidence": 0.94
}
```

## Minimumsprojekt

- En lagermodel med 5 reoler og faste lagerpladser.
- Ét kamera placeret ovenfra.
- En CNN-model, der genkender ledige og optagede pladser.
- To Raspberry Pis, der er forbundet over netværket.
- En controller, der gemmer lagerets aktuelle tilstand.
- Simulerede ordrer, som rutes ved hjælp af BFS eller A*.
- En visualisering af optagede pladser, ordrer og ruter.

Al test kan udføres på bordmodellen med kasser, printede billeder og kontrollerede lysforhold.

## Relevante kurser

- **SW3SYS:** Processer, concurrency, distribuerede komponenter og Raspberry Pi-interaktion.
- **E3KNP:** Netværkskommunikation, JSON-beskeder og TCP/UDP.
- **Algoritmer og datastrukturer:** Grafrepræsentation, køer, hash maps, BFS og A*.
- **SW3DSB:** Billedforbehandling og filtrering før CNN-klassifikation.

## Mulige ansvarsområder

1. CNN, datasæt og kameraopsætning.
2. Netværksprotokol og håndtering af beskeder.
3. Lager-controller og fælles systemtilstand.
4. Rutefinding og planlægning af ordrer.
5. Test, simulering og visualisering.

## Vigtigste udfordringer

- Pålidelig klassifikation under forskellige lysforhold og placeringer af pakker.
- At holde lagerets tilstand opdateret, når beskeder forsinkes eller mistes.
- At vælge nye ruter, hvis lagerpladser bliver optaget under en ordre.
- At køre systemet hovedsageligt i C/C++ på Raspberry Pis.
