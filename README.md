# Opdracht Julia

## Casus

Een [fractal](https://en.wikipedia.org/wiki/Fractal) is een afbeelding die zelfgelijkend is. Bekende voorbeelden zijn de [Mandelbrot set](https://en.wikipedia.org/wiki/Mandelbrot_set) en de [Sierpiński triangle](https://en.wikipedia.org/wiki/Sierpi%C5%84ski_triangle). De [Julia set](https://en.wikipedia.org/wiki/Julia_set) is een soort veralgemenisering van de Mandelbrot set, waarbij een startpunt in de 2D ruimte wordt genomen. In dit practicum gaan we onderstaande animatie renderen. Ieder frame is een Julia fractal, waarbij de animatie ontstaat doordat parameters per frame veranderen.

![](img/julia.gif)

### Complexe getallen (achtergrond)

Om deze 2D-ruimte weer te geven maken we gebruik van de [complexe getallen](https://en.wikipedia.org/wiki/Complex_number) $\mathbb{C}$. Dit is een uitbreiding van de reele $\mathbb{R}$ waar we bekend mee zijn, maar nu gebruiken we twee getallen: het reeele deel dat voor de $x$-as staat, en een imaginair deel voor de $y$-as. Samen vormen de twee getallen een 2D vector waar een paar extra operaties op van toepassing zijn. We schrijven een complex getal doorgaans niet als kolom van getallen in haakjes, maar als $a + bi$, met $a, b \in \mathbb{R}$. $a$ geeft een waarde aan van links naar rechts op de $x$-as, $b$ een waarde van onder naar boven op de $y$-as.

Het belangrijkste verschil tussen complexe getallen en "gewone" 2D-vectoren, is dat we een extra multiplicatie-operatie $\cdot : \mathbb{C} \times \mathbb{C} \to \mathbb{C}$ hebben, die we net als bij gewone vermenigvuldiging meestal niet schrijven ($ab$ is hetzelfde als $a \cdot b$). Met deze operatie vermenigvuldigen we twee complexe getallen, en krijgen we een nieuw complex getal terug. Voor gewone 2D-vectoren bestaat deze operatie niet, en hebben we alleen een inwendig product dat een gewoon (scalair) getal oplevert.

Laten we als voorbeeld kijken naar de getallen $2 + 3i$ en $5 + 7i$. Om deze met elkaar te vermenigvuldigen doen we niet veel anders dan bij het vermenigvuldigen van andere getallen, behalve dat we de waarde $i^2$ het getal $-1$ toekennen. Dit ziet er als volgt uit:

$(2+3i) \cdot (5+7i) = 2 \cdot 5 + 2 \cdot 7i + 3i \cdot 5 + 3i \cdot 7i = 10 + 14i + 15i + 21i^2 = 10 + 14i + 15i + 21\cdot(-1) = 10 + 14i + 15i - 21 = -11 + 29i$

![](img/complex.png)

De berekening zie je hierboven ook grafisch uitgebeeld. Als we een complex getal beschouwen als vector en kijken naar de lengte en hoek vanaf de x-as, dan heeft het nieuwe getal de som van de hoeken van de input-getallen, en het product van de lengtes.

### In C++

In C++ gebruiken we [`std::complex<double>`](https://en.cppreference.com/w/cpp/numeric/complex) om complexe getallen op te slaan. Alle rekenkundige operaties zijn dan automatisch beschikbaar:

```cpp
std::complex<double> a = 2.0 + 3i;
std::complex<double> b = 5.0 + 7i;
cout << a * b << endl;                // (-11,29)
```

Voor het plotten van een Julia fractal hebben we constante $c \in \mathbb{C}$ nodig. Wij gebruiken hiervoor het getal $r\cos{a}+ r\sin{a}i$, met $r = 0.7885$ en $a$ afhankelijk van het framenummer. Het huidige framenummer `t` wordt gedeeld door het totale aantal frames per volle rotatie `CYCLE_FRAMES`, en geschaald naar $[0..2\pi)$. Door de sinus en cosinus ontstaat een herhalende beweging van het punt door de 2D ruimte - na `CYCLE_FRAMES` is deze op dezelfde plaats als waar deze begon. De code voor het getal $c$ is hieronder gegeven en staat ook al in de template code. Dit getal is per frame constant.

```cpp
double a = 2 * std::numbers::pi * t / CYCLE_FRAMES;
double r = 0.7885;
std::complex<double> c = r * cos(a) + static_cast<std::complex<double>>(1i) * r * sin(a);
```

Het tweede complexe getal wat in onze berekeningen naar boven komt is $z$. Dit getal verschilt per pixel. Effectief beginnen we met een afbeelding gecentreerd rond het punt $(0.75, 0)$, en zoomen we in over tijd. De berekening voor dit getal staat hieronder in C++ code gegeven.

```cpp
double x_y_range = 2;

//double scale = 1.5 - 1.45 * t / FRAMES;                           // iets simpeler
double scale = 1.5 - 1.45 * log(1 + 9.0 * t / FRAMES) / log(10);    // iets interessanter om naar te kijken

std::complex<double> z = 2 * x_y_range * std::complex(static_cast<double>(x)/WIDTH, static_cast<double>(y)/HEIGHT)
    - std::complex(x_y_range*3/4, x_y_range);

z *= scale;
```

#### Toelichting (optioneel)

Uitgaande van het eerste frame (`t == 0`) en een `WIDTH` en `HEIGHT` van $800 \times 600$ komt de pixel $(0, 0)$ overeen met $-2.25 - 3i$, en de pixel $(800, 600)$ met $3.75 + 3i$.
Gedurende de animatie wordt dit ieder frame iets meer geschaald, totdat het laatste frame loopt van $-0.075 - 0.1i$ tot $0.125 + 0.1i$.
De schaalfactor drukken we uit in een losse variabele die we op verschillende manieren kunnen berekenen. Gebruik hiervoor de voorbeeldcode of wordt zelf creatief.

### Algoritme

Voor het plotten van een Julia set kiezen we een escape radius $r$ (deze komt overeen met de `x_y_range` hierboven). Nu gaan we per frame, voor iedere pixel de $z$ steeds de berekening $z := z^2 + c$ uitvoeren. Dit herhalen we totdat $z$ binnen een cirkel met straal $r$ om de oorsprong komt, d.w.z. $\lVert z \rVert < r$. Voor sommige punten zal dit nooit gebeuren, dus stoppen we na een vast aantal iteraties (in de pseudo-code hieronder $81$) en kleuren we de pixel zwart. Voor de punten die voor dat punt binnen de cirkel eindigen, kiezen we een kleur gebaseerd op het aantal iteraties, traditioneel is dit een (deel van de) regenboog, met volle saturatie en helderheid.

In pseudocode ziet dit proces er als volgt uit:

```
escape_radius = 2
max_iter = 81

for each pixel (x, y) do:
    z = scaled pixel # Hierboven uitgelegd

    iteration = 0

    while norm(z) < escape_radius and iteration < max_iter:
        z = z^2 + c
        iteration = iteration + 1

    if iteration == max_iteration:
        pixel(x, y) = black
    else:
        colour_hue = f(iteration / max_iter)       # (0..360)
        pixel(x, y) = HSV(colour_hue, 100%, 100%)
```

Voor het berekenen van de kleur is in het template een `inline constexpr` functie `COLOURISE` beschikbaar die van blauw omgekeerd door het spectrum loopt, gebaseerd op de HSV conversie. Je mag deze as-is gebruiken, aanpassen, of hier zelf een eigen functie voor bedenken.

### Images en Pixels

Voor het schrijven van de frames en video gebruiken we [`cImg`](https://cimg.eu), bijgevoegd in de template. Deze library wordt aan het einde door de root-node gebruikt, tussentijds gebruiken we een eigen `frame` datastructuur. Deze bevat alle pixelwaardes in een enkele flat array, waardoor deze met een custom `MPI_Datatype` over nodes uit te wisselen is. 

We gebruiken de volgende types / synoniemen:

- `byte` is een 8 bits kleurwaarde voor R, G, of B, opgeslagen als `unsigned char`.
- `pixel` is een `std::array<byte, 3>` en bevat een totale kleurwaarde.
- `frame` is een `WIDTH`$\times$`HEIGHT` array van pixels, opgeslagen als een `byte[HEIGHT*WIDTH*3]`.
- `animation` is een class met daarin een `std::vector<frame>`.

De API van `frame` is als volgt:

- `frame()` - Default constructor, geeft een volledig zwarte frame van `WIDTH`$\times$`HEIGHT` pixels. Deze constructor wordt automatisch gebruikt wanneer een `std::vector<frame>(size)` geinitialiseerd wordt.
- `void set_colour(unsigned int x, unsigned int y, pixel const& c)` - Zet de kleur van pixel $(x,y)$ op basis van een `pixel` a.k.a. `std::array<byte, 3>`
- `byte& operator[](std::size_t x, std::size_t y, std::size_t c)` - Geeft toegang tot 1 byte aan kleurinformatie op coordinaat $(x, y)$. De channel wordt aangeduidt met $c$ - dit dient een waarde in $\{0,1,2\}$ te zijn, en staat voor R, G, B respectievelijk. Hiervoor zijn `RED`, `GREEN` en `BLUE` beschikbaar gesteld.

Om controle te houden over welke `frame`-vectoren wel en niet geïnitialiseerd worden (en daarmee ruimte innemen) wordt de wrapper class `animation` gebruikt. Deze kan op twee manieren worden aangemaakt:

- `animation()` zonder argumenten maakt een lege animatie, zonder ruimte te alloceren.

- `animation(unsigned int size)` maakt een geïnitialiseerde vector waarvoor ruimte gealloceerd wordt.

Verder bevat de class de methode `initialise(unsigned int size)` om een animation alsnog te initialiseren (enkel in de root node), een `data()` methode die de animatie in een MPI-geschikt formaat aanlevert, en een `operator[]` waarmee een frame geindexeerd kan worden. Hiermee zou het niet nodig zijn met de onderliggende data te werken, behalve in de MPI collectives waar `.data()` nodig is.

De `cImg` library gebruikt onder water `ffmpeg`, om de animatie als videobestand op te slaan. Deze dependency wordt via de Makefile in Linux/Mac/WSL automatisch gedownload. 

Voor Windows zonder WSL kun je deze handmatig downloaden ([link](https://www.gyan.dev/ffmpeg/builds/)) en de .exe van ffmpeg in de map naast je eigen gecompileerde .exe zetten. 

Als het niet lukt om de gegeven code aan de praat te krijgen en je geen video kan schrijven, mag je als alternatief de [`save_png()`](http://www.cimg.eu/reference/structcimg__library_1_1CImg.html#ab77d79ff964a8d2edbb9a022e347810a) methode gebruiken om losse plaatjes op te slaan. Deze moeten nog steeds in de root node verzameld worden - andere nodes schrijven zelf geen image files.

## Opdracht

1. We raden voor deze opdracht een ontwerp aan waarbij OpenMP gebruikt wordt om de rendering van een frame te parallelliseren, en de frames met MPI over meerdere nodes verdeeld worden. 
   
   - Is dit het beste ontwerp? 
     - Waarom wel (of)
     - wat zou een beter ontwerp zijn (en onderbouw dit)?

2. Maak een ontwerp en voeg MPI toe:
   
   - Alle frames worden in root gegathered
   - Denk na over de werkverdeling over nodes.
     - Makkelijkste optie: homogeen, met collectives en een root node die naderhand alles samenvoegt
     - Uitdagender (ivm handmatig senden van data) met $n-1$ render nodes en een root node die ten alle tijde nieuwe frames verzamelt
       - In hoeverre verwacht je dat dit effect heeft op de snelheid? In hoeverre verwacht je dat dit effect heeft op het geheugengebruik?
   - Denk na over striping vs chunking
     - Wat is de invloed op gathering?
     - Wat is de invloed op performance?
       - Is iedere frame even zwaar om te renderen?

3. Maak een ontwerp en voeg OpenMP toe om de rendering binnen een frame te parallelliseren.
   
   - Welke loops kun je wel/niet optimaliseren?

4. Beschrijf in je verslag de gemaakte keuzes en afwegingen. 
   
   - Onderbouw dit met timing-data waarbij je een verschillend aantal nodes / OpenMP threads gebruikt voor het renderen van dezelfde animatie.

5. Laat met behulp van afbeeldingen in het verslag het resultaat van je code zien.

6. **Extra:** In de opdrachtbeschrijving en template code gaan we uit van een specifieke Julia set, gebaseerd op een vaste waarde $c$. Voel je vrij deze aan te passen, alsmede de kleuren en de scalingfunctie $(x, y) \to \mathbb{C}$. Het belangrijkste aspect is dat er in het eindresultaat een verandering over tijd is, met een roterend (= herhalend) en schalend/translerend (= niet herhalend) aspect.
