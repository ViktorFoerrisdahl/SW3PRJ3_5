# ai-strategi.md

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Projektstyring/ai-strategi.md](../Projektstyring/ai-strategi.md)
- SHA-256: `763e96038770fcfc91d746a1f7619836f3744c730e5ae93f343670f53255ca52`
- Status: klar til læsning



---

# AI-strategi

Vi bruger AI som konsulent og som praktisk værktøj i ATLAS. AI skal hjælpe os med at forstå problemer, vurdere muligheder og udføre arbejde, mens gruppen beholder ansvaret for beslutninger og kvalitet.

## Konsulent og værktøj

Som konsulent bruges AI til at udfordre antagelser, forklare faglige emner og diskutere løsninger og deres konsekvenser. Som værktøj bruges AI til afgrænsede opgaver som kode, tests, review og dokumentationsudkast. Vi skal kunne forstå, begrunde og efterprøve det arbejde, vi tager i brug.

## Fælles projektkontekst

AI skal tage udgangspunkt i projektets aktuelle dokumentation og kode. Projektfilerne er sandhedskilden frem for tidligere chats. Antagelser og forslag skal kunne skelnes fra vedtagne beslutninger, og modstridende oplysninger skal fremhæves frem for at blive afgjort ved gæt.

`AGENTS.md` beskriver, hvilken kontekst agenter skal læse før rådgivning og ændringer. Ved brug af AI uden adgang til repoet skal vi selv give den relevante kontekst.

## Automatisk konvertering

Dokumenter lægges i den relevante mappe under `docs/`. CI genererer Markdown-kopier og et fælles indeks i `docs/context/`, så AI nemt kan finde og læse materialet. Kopierne opdateres, når kilder ændres, og henviser tilbage til originalerne. Vi redigerer derfor originalerne, ikke de genererede filer.

Konverteringen gør konteksten tilgængelig; den fortolker eller godkender ikke indholdet. Tekst, tabeller, formler og illustrationer bevares så vidt muligt i kopierne. Billedtekst udtrækkes med OCR; diagramrelationer læses i den viste illustration. Indekset markerer konkrete mangler, så originalen kun skal åbnes, når der er behov for det.

## Ansvar og kvalitet

AI-resultater gennemgås og efterprøves, før vi bruger dem. Væsentlige påstande kontrolleres mod kilder, og kodeændringer kontrolleres med relevante tests. AI-forslag bliver først projektbeslutninger, når gruppen har taget stilling og dokumenteret dem.

Vi følger projektets fælles terminologi og kodekonventioner: kode, kommentarer og commitbeskeder på engelsk og dokumentation på dansk.
