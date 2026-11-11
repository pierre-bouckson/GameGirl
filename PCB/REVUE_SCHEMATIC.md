# Revue du schéma GameGirl — 2026-11-08

Périmètre : `PCB.kicad_sch` et ses 5 feuilles (Alimentation, MCU, Boutons, SDRAM, Écran), comparées à `../firmware/GameGirl.ioc`, au firmware (`../firmware/Src/main.c`, BSP Discovery), au schéma MB1075 de la DISC1 et aux datasheets (DT024CTFT/ILI9341, TPS61165, TP4056, AP2112K, IS42S16400J).
Méthode : ERC `kicad-cli`, export du netlist et vérification net par net (3 relectures indépendantes en parallèle). Chaque point critique a été recoupé à la main sur le netlist.

## Synthèse

| Sévérité | # | Sujet |
|---|---|---|
| 🔴 CRITIQUE | F1 | **VREF+ (U3.32) flottant** : l'ADC ne marche pas (joystick, mesure batterie) |
| 🔴 CRITIQUE | B1 | **PCB désynchronisé du schéma** : 7 empreintes orphelines et un C20 en double, donc une BOM/CPL fausse |
| 🟠 MAJEUR | F2 | USART1 (printf) non sorti : labels `UART_TX`/`UART_RX` en l'air |
| 🟠 MAJEUR | F3 | Buzzer annoncé dans le README mais absent du schéma |
| 🟠 MAJEUR | F4 | AP2112K (SOT-23-5) : dissipation thermique limite sur USB |
| 🟠 MAJEUR | B2 | J3 et BG1 sans LCSC et non exclus de la BOM/CPL |
| 🟠 MAJEUR | B3 | Rotation CPL de U6 (TPS61165) probablement fausse de 90° |
| 🟠 MAJEUR | B4 | Stock du STM32F429ZIT6 (C84808) très bas (36 pièces au 26/09) |
| 🟡 MINEUR | F5–F13, B5–B8 | Voir le détail |

Le reste est conforme. FMC→SDRAM, LTDC→dalle, SPI5/CS/WRX, SWD, chargeur, power-path et boost du rétroéclairage ont été vérifiés broche par broche (section « Vérifié OK »).

---

## Partie 1 : fonctionnel

### ✅ F1 — VREF+ non connecté (CRITIQUE) — CORRIGÉ le 2026-11-08
> **Correction appliquée dans `mcu.kicad_sch` :**
> - VDDA (U3.33) est retiré du bus +3V3.
> - Le filtre est remis : +3V3 → **FB1** (600 Ω@100 MHz, C1002) → net `VDDA` = U3.32 VREF+ + U3.33 VDDA, découplé par **C19** (1 µF 0603, C15849) et **C24** (100 nF 0402, C1525) vers GND.
> - Un PWR_FLAG est ajouté sur `VDDA`.
> - L'ERC n'a plus d'erreur VDDA/VREF+.
> - Côté PCB : FB1, C19 et C24 sont à placer près des broches 31-33. Lors du F8, cocher « re-lier les empreintes par référence » pour réutiliser les anciennes empreintes FB1 et C19.


- **Constat :**
  - Le net `/MCU/VDDA` ne contient qu'un nœud, `U3.32 (VREF+)`.
  - La broche VDDA (`U3.33`) est directement sur `+3V3`.
  - Le label local `VDDA` n'existe qu'une fois dans `mcu.kicad_sch`, en (180.34, 78.74) mm.
  - L'ERC le signale : `label_dangling 'VDDA'` et `pin_not_driven U3 Pin 32 [VREF+]`.
- **Cause probable :** la ferrite FB1 et C19 ont été supprimées du schéma (on les retrouve encore dans le PCB, voir B1) sans recâbler VREF+.
- **Conséquence :** conversions ADC sans valeur, donc joystick X/Y (PA5/PC3) et `VBAT_SENSE` (PA1) inutilisables.
- **Correctif :** relier VREF+ à VDDA, et idéalement filtrer VDDA comme le recommande ST et comme le fait la DISC1. Par exemple : ferrite ou 10–47 Ω depuis +3V3 → nœud VDDA/VREF+ → 1 µF + 100 nF vers VSSA, au plus près des broches 31-33. On peut aussi simplement remettre FB1 (C1002) et C19.

### ✅ F2 — USART1 non exposé (MAJEUR) — CORRIGÉ le 2026-11-08
> **Correction appliquée dans `mcu.kicad_sch` :**
> - Ajout de **J4 « UART »** : barrette 1×3 au pas de 2,54 mm (`PinHeader_1x03_P2.54mm_Vertical`), en (345,44 ; 134,62), sous J3. Brochage : 1 = UART_TX (PA9, sortie MCU), 2 = UART_RX (PA10), 3 = GND, avec une note 115200 8N1.
> - L'ERC n'a plus de `label_dangling`.
> - J4 n'a pas de LCSC : comme J3, à souder à la main et à exclure de la BOM et du CPL JLCPCB (voir B2).


- **Constat :** `/MCU/UART_TX` (U3.101 = PA9) et `/MCU/UART_RX` (U3.102 = PA10) n'ont chacun qu'un seul nœud (ERC `label_dangling`). L'ancien J4 (UART) n'existe plus que sur le PCB.
- **Conséquence :** sans le VCP du ST-LINK, il n'y a plus de sortie `printf`, ce que CLAUDE.md exige pourtant.
- **Correctif :** remettre un connecteur 3 broches (TX, RX, GND) ou des pastilles de test, exclu de la BOM. Autre option : passer J3 en 1×8.

### ✅ F3 — Buzzer absent (MAJEUR) — CORRIGÉ le 2026-11-08
> **Ajouté dans la feuille Boutons (`inputs.kicad_sch`, section « 4. Buzzer ») :**
> - **BZ1** MLT-8530 (C94599, magnétique passif CMS 8,5 mm), broche + sur +3V3.
> - **Q2** AO3400A (C20917, Basic) en bas de charge, sur la broche − du buzzer.
> - **D5** 1N4148W (C81598) en roue libre, cathode sur +3V3.
> - **R37** 100 Ω en série dans la grille et **R38** 100 kΩ de grille vers GND : le buzzer reste muet au reset.
> - Commande par le label global `BUZZER` sur **PB14** (U3.75) = **TIM12_CH1, AF9**. TIM12 est libre, ainsi que les alternatives TIM1/TIM8_CH2N.
> - ERC propre.
>
> **À faire :**
> - Confirmer sur la datasheet MLT-8530 que les pads 1/2 sont bien les bornes +/− et que 3/4 sont des pads de fixation.
> - Ajouter TIM12_CH1 sur PB14 au `.ioc` : PWM ~2,7 kHz, rapport cyclique 50 % au maximum.
> - Le buzzer est une pièce **Extended** de plus (11 au total).
> - Il ajoute jusqu'à ~80 mA intermittents sur le 3V3 (voir F4).


- **Constat :** le README liste un buzzer, mais il n'y a ni buzzer, ni transistor, ni diode dans le netlist, et rien non plus dans le firmware.
- **Correctif s'il est voulu :**
  - Buzzer magnétique passif (type MLT-8530) piloté par un N-MOS AO3400, avec une diode de roue libre.
  - Commande PWM depuis une broche libre, par exemple PB14 (TIM12_CH1) ou PA2 (TIM2_CH3).
  - Éviter PA8/PC9 (I2C3 dans le `.ioc`) et PA15 (JTDI).
  - Mettre le `.ioc` à jour.

### 🟢 F4 — Dissipation de l'AP2112K-3.3 sur USB — ATTÉNUÉ le 2026-11-08
> **Corrections appliquées :**
> - **Boost U6 alimenté par VSW au lieu de +3V3**, soit environ −75 mA sur le LDO : la dissipation passe de ~0,4 W à ~0,3 W sur USB.
>   - Concerne VIN de U6, l'entrée de L1 et le condensateur d'entrée C52 (`lcd.kicad_sch`).
>   - `VSW` devient un label global (`power.kicad_sch`).
>   - C51 reste sur +3V3 (découplage de la dalle).
> - **Note de routage à côté de U2** : grande zone de cuivre sur GND/VIN/VOUT et vias thermiques vers le plan de masse.
>
> À valider sur le prototype : mesurer la température de U2 en jouant branché en USB. Si c'est encore trop chaud, passer à un buck-boost.


- **Charge estimée sur le 3V3 :**

  | Poste | Courant |
  |---|---|
  | MCU à 128 MHz (horloge actuelle du firmware) | ~80–110 mA |
  | SDRAM | ~50–100 mA |
  | Logique LCD | ~10 mA |
  | Joystick Hall | ~5–10 mA |
  | Bargraph (maximum) | ~30 mA |
  | **Boost du rétroéclairage, alimenté depuis +3V3** (U6.1 VIN sur `+3V3`) : 9,6 V × 20 mA / 0,8 / 3,3 V | ≈ 75 mA |
  | **Total** | **≈ 270–350 mA** |

- **Constat :** sur USB, VSYS vaut environ 4,65 V. Le LDO dissipe alors (4,65 − 3,3) × 0,3 ≈ **0,4 W** dans un SOT-23-5 (θJA ~ 200 °C/W), soit un échauffement d'environ +80 °C. C'est proche de la limite.
- **Correctif :**
  - Le plus simple : alimenter U6 (VIN 3–18 V) depuis **VSW** au lieu de +3V3. Cela retire environ 75 mA du LDO.
  - En plus : un grand plan de cuivre sous U2.
  - Alternative : un buck-boost (TPS63001…), qui règle aussi F5.

### 🟡 F5 — Dropout du LDO en fin de batterie (MINEUR)
- **Constat :**
  - Avec ~250 mV de dropout, le 3V3 n'est plus régulé sous VBAT ≈ 3,5 V.
  - La SDRAM exige VDD ≥ 3,0 V, ce qui correspond à VBAT ≈ 3,2 V, alors que le PCM de la batterie coupe plus bas.
  - De plus, VBAT_SENSE est mesuré contre VREF+ = 3V3 : en dropout, la mesure devient fausse.
- **Correctif firmware :** lire VREFINT (ADC1_IN17) pour corriger la mesure, et éteindre proprement vers 3,4 V.

### 🟡 F6 — Rétroéclairage allumé dès le reset (MINEUR)
- **Constat :**
  - La broche CTRL de U6 est sur PB4 (`LCD_BL`), absente du `.ioc` et du code.
  - Au reset, PB4 = NJTRST avec un pull-up interne, ce qui l'emporte sur le pull-down interne de 400 kΩ de CTRL. Le rétroéclairage démarre donc à 100 %.
  - La note de `lcd.kicad_sch` (~l.8063) affirme l'inverse.
- **Correctif :**
  - Configurer PB4 en TIM3_CH1 (AF2), en PWM de 6,5 à 100 kHz.
  - Respecter la fenêtre EasyScale : pas de niveau bas de plus de 160 µs dans la première ms.
  - Corriger la note, ou ajouter un pull-down de 10 kΩ si l'on veut l'écran éteint au boot.

### 🟡 F7 — `.ioc` désynchronisé du schéma (MINEUR)
- **Constat :**
  - Absents du `.ioc` : PE2/PE3/PE4/PE5 (B, SELECT, START, JOY_SW), PA1 (VBAT_SENSE), PB4 (TIM3_CH1) et les 10 broches du bargraph (PD2, PD4–PD7, PD11, PD12, PG2, PG3, PG9).
  - Conflit firmware : la BSP Discovery (`stm32f429i_discovery.c:768-773`, `LCD_IO_Init`) pilote toujours **PD12 = RDX** et PD11 (TE). Sur cette carte, ces broches deviennent **BAR7 et BAR6** du bargraph. Côté matériel, RDX de la dalle est tiré à 3V3 par R43, donc correct.
- **Correctif :** mettre à jour le `.ioc` et retirer RDX/TE de la BSP.

### 🟡 F8 — PWR_FLAG manquants (MINEUR, ERC)
- **Constat :** ERC `power_pin_not_driven` sur #PWR001 (GND) et #PWR016 (+3V3). Aucun PWR_FLAG dans le projet.
- **Correctif :** placer des PWR_FLAG sur GND et +3V3, et éventuellement sur VBUS, VBAT et VSW.

### 🟡 F9 — Courant USB total (MINEUR)
- **Constat :**
  - Console allumée et en charge, il faut environ 780 mA (R_PROG = 1,5 kΩ) + ~300 mA, soit **≈ 1,1 A**. Un port PC USB 2.0 ne le garantit pas.
  - Le TP4056 dissipe alors environ 1 W, donc il passe en régulation thermique.
- **Correctif :** chargeur ≥ 1,5 A, pad exposé soudé sur un grand plan GND. Ou bien R_PROG = 2 kΩ, soit environ 580 mA.

### 🟡 F10 — SW1 (MSK12C02) traversé par tout le courant système (MINEUR, à vérifier)
- **Constat :** environ 0,3–0,45 A passe dans un interrupteur à glissière miniature dont le calibre est souvent de 0,3–0,5 A.
- **Correctif :** vérifier le calibre de C431540. Si c'est juste, faire commander l'EN de U2 par SW1 au lieu de couper la puissance.

### 🟡 F11 — Protections (MINEUR)
- **Constat :** pas de TVS/ESD sur VBUS et CC. Pas de protection contre l'inversion de batterie sur J2, alors que la polarité JST-PH n'est pas normalisée.
- **Correctif :**
  - TVS 5 V (SMF5.0A) sur VBUS.
  - Sérigraphie « + » et « − » sur J2, et vérification de la polarité sur le câble réel.

### 🟡 F12 — Joystick SW6 (MINEUR, à vérifier)
- **Constat :**
  - Le brochage (1 VCC, 2 OUT1, 3 OUT2, 4 SW, 5 GND, 6 SHELL) ne provient que du symbole EasyEDA, sans datasheet chez LCSC.
  - Il faut confirmer le fonctionnement Hall en 3,3 V et le niveau actif du bouton.
  - JOY_SW (PE5) n'a pas de condensateur anti-rebond, contrairement aux 4 boutons.
- **Correctif :** valider sur un échantillon avant routage, et prévoir l'emplacement d'un 100 nF.

### 🟡 F13 — BOOT0 sans accès (MINEUR)
- **Constat :** R9 tire BOOT0 à GND, sans pad vers 3V3. C'est une sécurité qui manque si le SWD est verrouillé par un firmware.
- **Correctif :** ajouter un pad ou un cavalier BOOT0 → 3V3.

### ✅ Ajout 2026-11-08 — Points de test des tensions
Pastilles CMS `TestPoint:TestPoint_Pad_D1.5mm`, déjà exclues de la BOM et du CPL par leur empreinte, donc sans coût JLCPCB :

| TP | Net | Feuille | Valeur attendue |
|---|---|---|---|
| TP1 | VBUS | Alimentation | 5 V (USB) |
| TP2 | VBAT | Alimentation | 3,0–4,2 V |
| TP3 | VSYS | Alimentation | ~4,65 V sur USB, sinon VBAT |
| TP4 | VSW | Alimentation | VSYS après SW1 (0 V à l'arrêt) |
| TP5 | +3V3 | Alimentation | 3,3 V |
| TP6 | GND | Alimentation | référence |
| TP7 | VDDA | MCU | 3,3 V filtré (après FB1) |
| TP8 | BL_A | Écran | ~9,6–10 V (sortie du boost) |
| TP9 | BL_FB | Écran | ~200 mV, soit 20 mA dans les LED |

### ℹ️ Infos (pas d'action requise)
- **Pas de quartz HSE (PH0/PH1 NC) : c'est OK.** Le firmware tourne sur HSI (`main.c:228-236`, PLL HSI /12 ×192 /2 = 128 MHz), et l'horloge pixel LTDC vient de PLLSAI, elle aussi sur HSI. Un HSE ne deviendra nécessaire qu'en cas d'USB.
- **I2C3 (PA8/PC9) n'est pas câblé.** La DT024CTFT n'a pas de tactile et `MX_I2C3_Init` ne fait aucune transaction. On peut retirer I2C3 du `.ioc`.
- **PG13 (LED de la DISC1)** est piloté par le firmware mais il n'est pas câblé. Sans effet.
- **J6 (FPC) :** la numérotation 1:1 est conforme au datasheet DT024CTFT (pages 6-7). Le FH12-45S-0.5SH est à **contacts en bas**, alors que le datasheet suggère FH12S ou FH19SC. Il faut vérifier sur le plan mécanique que la face contacts de la nappe correspond dans l'orientation de montage, sinon on a une inversion 1↔45.
- **Boutons A/B/SELECT/START :** actifs haut, avec pull-down 10 kΩ et 100 nF sans résistance série (copie du B1 de la DISC1). Ça fonctionne ; une série de 100 Ω à 1 kΩ est optionnelle.
- **USB D+/D− non connectés :** c'est inévitable, puisque PA11/PA12 servent au LTDC (R4/R5).
- **Bargraph BG1 :** 10 LED via 470 Ω, ≈ 3 mA par segment, 30 mA au maximum. Électriquement cohérent.
- **Erreurs ERC « library/footprint not found » :** problème de chemin d'environnement de `kicad-cli`, pas un défaut du design.

### ✅ Vérifié OK
- **Alimentation MCU :**
  - 12 VDD + VBAT + PDR_ON sur +3V3 ; 9 VSS + VSSA sur GND.
  - VCAP_1 et VCAP_2 avec 2,2 µF chacun (C21, C22).
  - 13 × 100 nF + 4,7 µF de découplage.
  - NRST avec 100 nF, partagé avec le RESET de la dalle comme sur la DISC1.
  - BOOT0 à 0 ; PB2 libre.
- **SWD J3 :** 3V3 / SWCLK PA14 / GND / SWDIO PA13 / NRST / SWO PB3.
- **SDRAM U5 :**
  - Brochage TSOP-54 vérifié (alimentations, DQ0-15, A0-11, BA0/1, DQML/DQMH, WE/CAS/RAS/CS/CKE/CLK).
  - Mapping FMC identique au `.ioc` et à la DISC1 (SDCKE1 = PB5, SDNE1 = PB6, banque 2).
  - Configuration FMC cohérente : 8 colonnes, 12 lignes, 16 bits, 4 banques, CAS 3, SDCLK 64 MHz, refresh 980.
  - Découplage 7 × 100 nF + 10 µF.
- **Dalle J6 :**
  - IM[3:0] = 0110 (SPI 4 fils + RGB, comme la DISC1).
  - Mapping RGB565 LTDC conforme au `.ioc` ; R2/B2 à GND.
  - Synchro : HSYNC PC6, VSYNC PA4, DOTCLK PG7, DE PF10.
  - SPI5 : SCK PF7, MOSI PF9 ; CS = PC2, WRX = PD13 ; RDX tiré haut par R43.
  - VCC 3V3 découplé.
- **Rétroéclairage :**
  - TPS61165 DBV : 1 VIN, 2 CTRL, 3 SW, 4 GND, 5 COMP, 6 FB, conforme au symbole.
  - R44 = 10 Ω → 20 mA. L1 10 µH, D4 40 V, C53 1 µF 50 V (couvre l'OVP de 38 V), COMP 220 nF.
- **USB-C J1 :** CC1/CC2 avec 5,1 kΩ séparées, VBUS et GND doublés, blindage à GND.
- **TP4056 :** TEMP→GND, PROG 1,5 kΩ (≈ 780 mA), CE→VCC, EP→GND, LED CHRG/STDBY bien orientées, 10 µF en entrée et en sortie.
- **Power-path :**
  - Q1 AO3401A : G = VBUS avec pulldown R6, S = VSYS, D = VBAT, diode interne dans le bon sens.
  - D1 empêche le retour de la batterie vers l'USB.
  - La charge reste possible console éteinte.
  - Le diviseur VBAT (100k/100k) est en aval de SW1, donc aucune fuite à l'arrêt.
- **AP2112K :** brochage conforme, EN sur VIN, 10 µF en entrée et en sortie.
- **Entrées :** BTN_A = PA0 actif haut, conforme à l'EXTI0 en front montant. Joystick X = PA5, Y = PC3 via 1 kΩ / 100 nF, alimenté en 3V3.
- **Labels :** pas de label global mal orthographié entre feuilles, aucune broche MCU à double usage. Les seuls nets à un nœud sont VDDA, UART_TX et UART_RX.

---

## Partie 2 : BOM / JLCPCB

L'outillage est **Bouni kicad-jlcpcb-tools** (dossier `jlcpcb/`). Il lit les champs des **empreintes du PCB** dont le nom correspond à `^(lcsc|jlc)`. Les LCSC ont été contrôlés sur la base pièces locale du plugin (instantané du 26/09/2026). Les stocks n'ont pas été revérifiés en ligne, sauf la référence de C84808.

### Tableau
| Refs | Valeur | LCSC | Statut | Cat. | Remarque |
|---|---|---|---|---|---|
| C1-C4, C47 | 10 µF 0603 | C19702 | ✅ | Basic | 10 V X5R ; voir B6 pour C1 sur VBUS |
| 30 × 100 nF | 100 nF 0402 | C1525 | ✅ | Basic | 16 V X7R |
| C18, C51, C52 | 4,7 µF 0603 | C19666 | ✅ | Basic | 16 V |
| C21, C22 | 2,2 µF 0603 | C23630 | ✅ | Basic | VCAP |
| C53 | 1 µF 0805 | C28323 | ✅ | Basic | 50 V X7R |
| C54 | 220 nF 0402 | C16772 | ✅ | Basic | |
| D1, D4 | B5819W | C64885 | ✅ | Preferred | |
| D2 | LED rouge 0603 | C2286 | ⚠️ | Basic | Valeur « Rouge : en charge » |
| D3 | LED verte 0805 | C2297 | ⚠️ | Basic | Valeur « Vert : chargée » ; boîtier différent de D2 |
| J1 | USB-C 16P | C2927039 | ✅ | Ext | |
| J2 | JST-PH SMD | C295747 | ✅ | Ext | |
| J3 | SWD 1×6 THT | — | ❌ | — | À exclure de la BOM et du CPL |
| J6 | FH12-45S-0.5SH(55) | C3168432 | ⚠️ | Ext | Attribut `through_hole` sur une empreinte 100 % CMS |
| L1 | SWPA4030S100MT | C38117 | ⚠️ | Ext | Champ « LCSC Part » |
| Q1 | AO3401A | C15127 | ✅ | Basic | |
| R1, R2 | 5,1 kΩ | C25905 | ✅ | Basic | |
| R3 | 1,5 kΩ | C25867 | ✅ | Basic | |
| R4, R5, R20, R21 | 1 kΩ | C11702 | ✅ | Basic | |
| R6-R8 | 100 kΩ | C25741 | ✅ | Basic | |
| R9, R22-R26, R43 | 10 kΩ | C25744 | ✅ | Basic | |
| R27-R36 | 470 Ω | C25117 | ✅ | Basic | |
| R44 | 10 Ω 0603 1 % | C22859 | ✅ | Basic | |
| SW1 | MSK12C02 | C431540 | ✅ | Ext | Pièce SMT malgré le nom d'empreinte « SW-TH » |
| SW2-SW5 | Tactile 5,1 mm | C318884 | ✅ | Basic | |
| SW6 | Joystick YHE-YG16-001 | C42411044 | ⚠️ | Ext | **THT, stock 201**, pas de datasheet |
| U1 | TP4056 | C16581 | ✅ | Preferred | |
| U2 | AP2112K-3.3TRG1 | C51118 | ✅ | Ext | |
| U3 | STM32F429ZIT6 | C84808 | ⚠️ | Ext | **Stock 36**, ~11 $ |
| U5 | IS42S16400J-7TLI | C2979137 | ✅ | Ext | Stock 871, ~5,7 $ |
| U6 | TPS61165DBVR | C58756 | ⚠️ | Ext | Champ « LCSC Part » ; rotation (B3) |
| BG1 | « BAS10251A » bargraph DIP-20 | — | ❌ | — | Référence introuvable, THT, à exclure |

Les 33 lignes CMS ont un LCSC cohérent en valeur et en boîtier : **aucune erreur de référence LCSC**. On compte **10 pièces Extended** (J1, J2, J6, L1, SW1, SW6, U2, U3, U5, U6), soit **environ 30 $ de frais fixes** par commande. Les pièces Extended coûtent environ 21 $ par carte.

### 🔴 B1 — PCB désynchronisé du schéma (CRITIQUE)
- **Constat (vérifié) :**
  - Le PCB contient des empreintes absentes du schéma : **C19, FB1, J4, Q2, R40, R41, R42**, plus **un second C20** à (98,31 ; 82,52).
  - Les fichiers de `jlcpcb/production_files/` (5 oct.) sont périmés : J6 y est encore l'ancien C22751607 (48P), et L1, U6, C52-C54, D4 et R44 y manquent.
- **Correctif :**
  1. Dans Pcbnew, faire *Mettre à jour le PCB depuis le schéma* (F8) en cochant la suppression des empreintes orphelines.
  2. Rouvrir le plugin pour qu'il resynchronise `project.db`.
  3. Régénérer BOM, CPL et Gerber.

### 🟠 B2 — J3 et BG1 non exclus (MAJEUR)
- **Constat :** les deux n'ont pas de LCSC, et `exclude_from_bom` / `exclude_from_pos` valent 0 dans `project.db`.
- **Correctif :** cocher « Exclure de la BOM » et « Exclure des fichiers de position » (ou DNP) sur les deux. Si l'on veut BG1 assemblé, le remplacer par 10 LED 0603 (C2286).

### 🟠 B3 — Rotation CPL de U6 (MAJEUR)
- **Constat :**
  - La règle de correction du plugin `^SOT-23 → -90°` s'applique à l'empreinte EasyEDA `SOT-23-6_L2.9-…`, alors que les empreintes EasyEDA sont déjà dans l'orientation JLC. U6 sortira probablement tourné de 90°.
  - U3 (LQFP standard KiCad) reçoit 270°, ce qui est la correction habituelle.
- **Correctif :** ajouter une correction 0° pour C58756. Contrôler U3, U6, J1, J6 et U1 dans l'aperçu de placement JLC avant de valider.

### 🟠 B4 — Stock du MCU (MAJEUR)
- **Constat :** C84808 n'avait que **36 pièces** au 26/09 ; la variante -TR (C2053976) n'en avait que 9.
- **Correctif :** vérifier juste avant la commande. Sinon, passer par le pre-order ou le global sourcing JLC, ou envoyer les puces en consigne.

### 🟡 B5 — Joystick SW6 (MINEUR)
- **Constat :** composant THT (assemblage THT payant) avec un stock de 201.
- **Correctif :** soit le faire assembler, soit l'exclure de la BOM et le souder à la main.

### 🟡 B6 — C1 sur VBUS (MINEUR)
- **Constat :** 10 µF **10 V** X5R 0603 ; il perd environ 50 % de sa capacité à 5 V et a peu de marge face aux surtensions au branchement à chaud.
- **Correctif :** 10 µF 25 V en 0805.

### 🟡 B7 — Champs et valeurs (MINEUR)
- **Constat :**
  - L1 et U6 utilisent le champ « LCSC Part ». Le plugin le lit, mais pas `kicad-cli` ni la BOM KiCad.
  - D2 et D3 ont des valeurs descriptives (« Rouge : en charge », « Vert : chargée »).
- **Correctif :**
  - Renommer le champ en « LCSC ».
  - Mettre les valeurs D2/D3 à « Red » / « Green » et déplacer le texte dans Description. Option : uniformiser D3 en 0603.

### 🟡 B8 — Attributs d'empreinte (MINEUR)
- **Constat :**
  - J6 est en `through_hole` alors que ses pads sont tous CMS.
  - Les `R_0402` de la bibliothèque CDFER n'ont pas d'attribut SMD : un export de position natif de KiCad en mode « SMD seulement » les oublierait (le plugin les inclut).
- **Correctif :** passer J6 en SMD.

---

## Check-list avant commande
1. [ ] Relier **VREF+** à VDDA, avec filtrage (F1)
2. [ ] Réexposer **USART1** : connecteur ou pastilles de test (F2)
3. [ ] Décider pour le **buzzer** (F3)
4. [ ] Alimenter **U6 depuis VSW** et soigner le cuivre sous U2 (F4)
5. [ ] Ajouter les **PWR_FLAG**, puis relancer un ERC propre (F8)
6. [ ] Valider le brochage du **joystick** et l'orientation de la **nappe FPC** (F12, J6)
7. [ ] **F8 dans Pcbnew** : supprimer les orphelins C19/FB1/J4/Q2/R40-42 et le C20 en double (B1)
8. [ ] **Exclure J3 et BG1** (et SW6 si soudé à la main) de la BOM et du CPL (B2, B5)
9. [ ] Corriger la **rotation de U6** et contrôler l'aperçu de placement JLC (B3)
10. [ ] Vérifier le **stock du STM32** (B4)
11. [ ] Régénérer les fichiers de production
12. [ ] Mettre à jour le **`.ioc`** et la BSP (PD11/PD12) (F7)
