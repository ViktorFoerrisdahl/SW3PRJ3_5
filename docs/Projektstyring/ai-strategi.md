# AI-strategi

Vi bruger AI som konsulent og som praktisk værktøj i ATLAS. AI skal hjælpe os med at forstå problemer, vurdere muligheder og udføre arbejde, mens gruppen beholder ansvaret for beslutninger og kvalitet.

## Konsulent og værktøj

Som konsulent bruges AI til at udfordre antagelser, forklare faglige emner og diskutere løsninger og deres konsekvenser. Som værktøj bruges AI til afgrænsede opgaver som kode, tests, review og dokumentationsudkast. Vi skal kunne forstå, begrunde og efterprøve det arbejde, vi tager i brug.

## Fælles projektkontekst

AI skal tage udgangspunkt i projektets aktuelle dokumentation og kode. Projektfilerne er sandhedskilden frem for tidligere chats. Antagelser og forslag skal kunne skelnes fra vedtagne beslutninger, og modstridende oplysninger skal fremhæves frem for at blive afgjort ved gæt.

`AGENTS.md` beskriver, hvilken kontekst agenter skal læse før rådgivning og ændringer. Ved brug af AI uden adgang til repoet skal vi selv give den relevante kontekst.

## Automatisk konvertering

Dokumenter lægges i den relevante mappe under `docs/`. CI genererer Markdown-kopier og et fælles indeks i `docs/context/`, så AI nemt kan finde og læse materialet. Kopierne opdateres, når kilder ændres, og henviser tilbage til originalerne. Vi redigerer derfor originalerne, ikke de genererede filer.

Konverteringen gør konteksten tilgængelig; den fortolker eller godkender ikke indholdet. Billeder, diagrammer og visse filformater kan kræve manuel læsning, hvilket markeres i indekset.

## Ansvar og kvalitet

AI-resultater gennemgås og efterprøves, før vi bruger dem. Væsentlige påstande kontrolleres mod kilder, og kodeændringer kontrolleres med relevante tests. AI-forslag bliver først projektbeslutninger, når gruppen har taget stilling og dokumenteret dem.

Vi følger projektets fælles terminologi og kodekonventioner: kode, kommentarer og commitbeskeder på engelsk og dokumentation på dansk.
