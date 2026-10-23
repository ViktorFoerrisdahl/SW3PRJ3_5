# ATLAS – SW3PRJ3_5

ATLAS (Acoustic TDOA Localization and Alert System) er gruppe 5's projekt på 3. semester. Projektet undersøger akustisk lokalisering ved hjælp af tidsforskelle mellem mikrofoner (TDOA). Det beskrevne proof of concept bruger fire mikrofoner og et klap til at estimere lydkildens position i 2D.

Projektet er under udvikling. Projektformulering, analyser og referater beskriver både beslutninger og åbne spørgsmål.

## Find det, du har brug for

| Placering | Indhold |
| --- | --- |
| [`docs/Inception/`](docs/Inception/) | Projektformulering, samarbejdskontrakt og tidlige idéer |
| [`docs/Elaboration E1/`](docs/Elaboration%20E1/) | Materiale fra første elaboration |
| [`docs/Elaboration E2/`](docs/Elaboration%20E2/) | Tekniske analyser og diagrammer |
| [`docs/Referater/`](docs/Referater/) | Mødereferater og aftaler |
| [`docs/Projektstyring/ai-strategi.md`](docs/Projektstyring/ai-strategi.md) | Principper for AI-brug og fælles projektkontekst |
| [`docs/Projektstyring/terminology.md`](docs/Projektstyring/terminology.md) | Fælles ordvalg, forkortelser og skrivemåde |
| [`docs/Projektstyring/code-conventions.md`](docs/Projektstyring/code-conventions.md) | Konventioner for kode og Doxygen |
| [`docs/context/INDEX.md`](docs/context/INDEX.md) | Genereret indgang til dokumenterne for AI-agenter |
| [`code/`](code/) | Kode, tests og hjælpeværktøjer; endnu ingen systemimplementering |
| [`AGENTS.md`](AGENTS.md) | Obligatorisk læse- og arbejdsvejledning for AI-agenter |
| [`.github/workflows/`](.github/workflows/) | Automatisk konvertering og kontrol |

## Sprog

Kode, identifikatorer, kodekommentarer og commitbeskeder er på **engelsk**. Dokumentation er på **dansk**, med etablerede fagudtryk som angivet i terminologilisten. De historiske kildedokumenter bevares uændret.

## Tilføj dokumentation

GitHub Actions bygger Markdown-kopier og et kildeindeks i `docs/context/`. Ved push gemmes opdateringerne automatisk på samme branch, hvis branchens regler tillader det. Pull requests kontrolleres uden skriveadgang og får den genererede kontekst som et downloadbart artifact. Se [AI-strategien](docs/Projektstyring/ai-strategi.md) for principperne bag den fælles kontekst.

Ret altid originalen. Diagrammer, eksterne genveje og ikke-understøttede formater bliver synlige i indekset med en besked om manuel læsning. Deres indhold må ikke antages at være konverteret.
