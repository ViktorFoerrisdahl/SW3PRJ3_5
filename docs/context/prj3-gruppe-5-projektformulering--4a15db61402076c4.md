# PRJ3 Gruppe 5 Projektformulering.pdf

> Genereret fil. Ret originalen, ikke denne kopi.

- Kilde: [Inception/PRJ3 Gruppe 5 Projektformulering.pdf](../Inception/PRJ3%20Gruppe%205%20Projektformulering.pdf)
- SHA-256: `b2cbf183af5169c07757abda84bd972a223dc6e6342c569baff9c7152d0ca88e`
- Status: tekst udtrukket – kontrollér original

> PDF-tekst kan miste læserækkefølge, formler, tabeller og illustrationer. Kontrollér originalen.

---

## Side 1

ATLAS
Acoustic TDOA Localization and Alert System
Forfattere - Gruppe 5
AU-ID Navn
au786897 Alexander Bak Thorsen
au786265 Otto German Jørgensen
au792290 August Due Iversen
au806025 Bertram Engelbrecht Agerskov
au785712 Emil Lytzen Beck
au786842 Viktor Kirkeby Førrisdahl
au785720 Mads Sejerskilde
au793145 Frederik Buskbjerg Reichle
Vejleder
Lars Mortensen


## Side 2

Projektformulering
Verden er i forandring i dag, og nye metoder at føre krig p˚ a truer kritisk infrastruktur. Krigsførelse med
droneteknologi er hurtigt blevet normen, og giver nye udfordringer p˚ a hvordan man billigt og sikkert kan
beskytte luftrummet. Den danske regering havde tidligere afsat 365 milliarder kroner til oprustning af
forsvaret i perioden 2024-2033. Per februar 2026 er der allerede brugt 356 milliarder kroner. Dette ˚ abner
naturligvis for mange muligheder indenfor udvikling af blandt andet droneteknologi. Nuværende radar-
systemer er dyre, og andre passive detektions-systemer kræver at kilden udsender radiosignaler. Vores
projekt bygger p˚ a en lokalisering af droner, ved hjælp af Time-Delay of Arrival (TDOA) mellem flere mikro-
foner. Systemet er billigt og modulært, der kan tilføjes en arbitrær mængde mikrofoner for øget præcision.
Systemet er tænkt som et autonomt detekteringssystem, som selv kan beskytte et lille defineret omr˚ ade.
Systemet best˚ ar af minimum 5 mikrofoner(for 3D lokalisering), som placeres rundt om omr˚ adet som skal
forsvares. Disse mikrofoner opfanger alle et lyd-event, og disse data sendes til en central node som løser et
ligningsystem, baseret p˚ a tidsforskellen mellem de forskellige mikrofonpar. Løsningen til ligningssystemet,
er de estimerede x,y,z koordinater af lydkilden.
Product Goal
M˚ alet er at lave et proof of concept, for et system som kan lokalisere droner ved TDOA lokalisering. Vi
har som m˚ al at implementere et detekteringssystem med 4 mikrofoner, som kan detektere et lydevent i 2D.
Systemet kan arbitrært udvides til 3D ved at introducere en ekstra mikrofon og den ekstra ubekendte (z).
Systemet afgrænses til kernefunktionaliteten: Detektering af lydevent, estimeret position af lydkilde, styring
af gevær mod lydkilden. Vi vil ikke klassificere forskellige lyd-begivenheder, og vi laver proof of concept med
detektering af et klap, istedet for detektering af en drone ud fra dens støj.
Page 2
