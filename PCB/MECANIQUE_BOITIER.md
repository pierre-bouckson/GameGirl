# GameGirl — cotes mécaniques du PCB pour la conception du boîtier

Document destiné à l'agent (ou à la personne) qui conçoit le boîtier imprimé en 3D, style Game Boy.
Source de vérité : `PCB.kicad_pcb`. Modèle 3D de la carte équipée : `PCB.step` (même dossier).
En cas d'écart entre ce document et le fichier KiCad, c'est le fichier KiCad qui a raison.
Dans `PCB.step`, l'origine en X/Y est le coin haut-gauche du PCB, comme dans ce document. Le sens de l'axe Y et la position du zéro en Z suivent les conventions d'export de KiCad et n'ont pas été contrôlés : **vérifier l'orientation à l'import** en se repérant sur le joystick (face avant, en bas à gauche).

## 1. Repère et conventions

- Toutes les cotes sont en **millimètres**.
- **Origine (0 ; 0)** : coin **haut-gauche** du PCB, carte **vue de face** (côté joueur).
- **X** vers la droite, **Y** vers le bas (comme dans KiCad).
- **Z = 0** : face avant du PCB. Z positif vers le joueur. Le PCB fait **1,6 mm** : sa face arrière est à Z = −1,6.
- « Avant » = face joueur (F.Cu). « Dos » = face arrière (B.Cu).
- Les positions des composants du dos sont données **dans le même repère vu de face** (X non inversé).
  Vu depuis l'arrière du boîtier, un point d'abscisse X se trouve donc à `74 − X` du bord gauche.
- Dans KiCad, l'origine de ce repère est au point page (100 ; 40) mm : `X_kicad = X + 100`, `Y_kicad = Y + 40`.
  L'origine auxiliaire (perçage/placement) est posée sur ce coin.

## 2. Contour du PCB

| Élément | Valeur |
|---|---|
| Largeur × hauteur | **74,0 × 122,0** |
| Épaisseur | 1,6 (4 couches) |
| Rayon des 4 coins | 4,0 |
| Format visé | Game Boy Pocket (boîtier ≈ 79 × 127 avec parois de 2 mm et 0,5 mm de jeu) |

### Trous de fixation

6 trous **Ø 2,7 mm non métallisés** (vis M2.5). Zone sans piste ni composant de Ø 6,2 autour de chaque centre (tête de vis ou colonnette Ø ≤ 6).

| Trou | X | Y |
|---|---|---|
| H1 | 4,0 | 4,0 |
| H2 | 70,0 | 4,0 |
| H3 | 4,0 | 70,0 |
| H4 | 70,0 | 70,0 |
| H5 | 4,0 | 118,0 |
| H6 | 70,0 | 118,0 |

### Fente pour la nappe de l'écran

Fente débouchante, coins R1 : **X de 18,5 à 55,5 ; Y de 65,75 à 68,25** (37 × 2,5 mm, centrée sur X = 37).
La nappe de l'écran la traverse de l'avant vers le dos. Rien ne doit l'obstruer dans le boîtier.

## 3. Écran (face avant, moitié haute)

Module **Displaytech DT024CTFT**, 2,4" 240 × 320, en **portrait**, nappe en bas. Datasheet : `../docs/DT024CTFT.pdf` (page 4).
L'écran n'est **pas soudé** au PCB : il est tenu par le boîtier (logement dans la coque avant), relié par sa nappe.

| Élément | X min | X max | Y min | Y max | Remarque |
|---|---|---|---|---|---|
| Contour du module (rétroéclairage) | 15,64 | 58,36 | 6,00 | 65,46 | 42,72 × 59,46, tolérance ±0,15 |
| Zone active (image) | 18,64 | 55,36 | 9,00 | 57,96 | 36,72 × 48,96, centre (37,00 ; 33,48) |

- Épaisseur du module : **2,8 mm max**.
- La zone active n'est pas centrée en hauteur dans le module : 3,0 mm de marge en haut, 7,5 mm en bas ; 3,0 mm à gauche et à droite.
- Fenêtre conseillée dans la coque : 37,5 × 49,8 centrée sur (37,00 ; 33,48).
- Sens de vision optimal : « 6 heures » (depuis le bas), ce qui convient à une console tenue en main.

### Nappe et hauteur de l'écran

- La nappe sort du bord bas du module (Y = 65,46), fait **22 ± 0,5 mm** de long, 35,4 mm de large au plus (23 mm à l'extrémité).
- Elle descend dans la fente, se replie à 180° et remonte **contre le dos du PCB** jusqu'au connecteur J6.
- Entrée de J6 (dos) : bord du connecteur à **Y ≈ 58,7**, centré sur X = 37 ; la nappe s'y enfonce d'environ 4 mm vers le haut.
- **Hauteur du dos de l'écran au-dessus du PCB (h)** : la longueur de nappe disponible impose **h ≤ 5,5 mm**. Valeur conseillée : **h = 3 à 5 mm**. Plus h est petit, plus la nappe a de mou (elle fait une boucle au dos).
  Ces valeurs reposent sur une profondeur d'insertion estimée : **à valider avec l'écran réel avant d'imprimer la coque définitive.**
- Zone du dos réservée à la nappe repliée : **X de 19 à 55, Y de 56,5 à 65,3**, sur environ 1,5 mm d'épaisseur. Aucun composant à cet endroit ; le boîtier ne doit pas y appuyer.
- Le levier de verrouillage de J6 doit rester accessible au montage (accès par le dos avant fermeture).

## 4. Commandes (face avant, moitié basse)

| Repère | Fonction | Centre X | Centre Y | Emprise au sol | Hauteur au-dessus du PCB |
|---|---|---|---|---|---|
| SW6 | Joystick analogique YHE-YG16-001 (avec clic) | **17,5** | **93,0** | 16,5 × 18,4 (X de 9,25 à 25,75 ; Y de 83,8 à 102,2) | **18,7** au sommet de la tige |
| SW2 | Bouton **A** | **62,0** | **87,0** | 6,5 × 5,2 | 1,5 |
| SW3 | Bouton **B** | **50,0** | **95,0** | 6,5 × 5,2 | 1,5 |
| SW4 | **SELECT** | **28,0** | **109,0** | 6,5 × 5,2 | 1,5 |
| SW5 | **START** | **46,0** | **109,0** | 6,5 × 5,2 | 1,5 |

- **Boutons A, B, SELECT, START** : poussoirs CMS 5,1 × 5,1 mm (LCSC C318884), poussoir central Ø ≈ 3 mm, course ≈ 0,25 mm, effort ≈ 160 gf. Le boîtier doit fournir des **capuchons** qui appuient sur le centre de chaque poussoir (sommet du poussoir à Z = 1,5).
  Entraxe A–B : 12,0 en X, 8,0 en Y (A en haut à droite, B en bas à gauche, comme sur une Game Boy).
- **Joystick** : composant traversant, origine de l'empreinte à (17,5 ; 93,0), mais **axe de la tige décalé, à environ (17,25 ; 91,55)** d'après le modèle 3D (corps jusqu'à Z ≈ 8, dôme jusqu'à ≈ 10,75, tige ≈ 3 × 4 mm). Le modèle 3D est `libs/EasyEDA/EasyEDA.3dshapes/SW-TH_YHE-YG16-001-SH1B166A1.step` : s'y référer pour la hauteur du corps, le diamètre de la tige et le débattement, que je n'ai pas pu confirmer par une datasheet (LCSC C42411044). **À mesurer sur la pièce réelle.**
  Prévoir une ouverture circulaire centrée sur (17,5 ; 93,0), assez large pour le débattement du capuchon.
- La hauteur du joystick fixe la distance entre le PCB et la face avant du boîtier (de l'ordre de 9 à 12 mm selon le capuchon). C'est pour cela que l'écran est surélevé (voir § 3) et que les boutons demandent des capuchons longs.
- Sérigraphie avant : « A », « B », « SELECT », « START » près des boutons, « GameGirl » centré vers Y = 74,5.

## 5. Connecteurs et éléments sur les bords (au dos)

| Repère | Élément | Position | Détail pour le boîtier |
|---|---|---|---|
| J1 | **USB-C** (charge) | bord **bas**, axe **X = 37,05** | Embase 8,94 de large × ≈ 3,2 de haut, posée sur le dos du PCB (de Z = −1,6 à Z ≈ −4,8). Le nez dépasse le bord du PCB de **0,3 mm** (Y = 122,3). Ouverture conseillée dans la coque : 9,6 × 3,8, centrée sur X = 37,05 et Z ≈ −3,2. |
| SW1 | **Interrupteur ON/OFF** à glissière MSK12C02 | bord **gauche**, centre **Y = 79,0** | Corps 6,7 × 2,7, hauteur 1,5 (de Z = −1,6 à −3,1). Le levier dépasse le bord gauche du PCB d'environ **0,85 mm** (X ≈ −0,85), course ≈ ±0,75 mm le long de Y. Avec une paroi de 2 mm, le levier reste en retrait : prévoir une lumière (≈ 5 × 2,5) et un curseur imprimé, ou un amincissement local de la paroi. |
| D2 | LED rouge « en charge » | (46,5 ; 120,3) | Au dos, à 1,7 mm du bord bas. Prévoir un trou Ø 1,5 ou un conduit de lumière dans la coque arrière. |
| D3 | LED verte « chargée » | (51,0 ; 120,3) | Idem. |

## 6. Composants du dos : hauteurs et zones à dégager

Hauteurs mesurées depuis la face arrière du PCB (ajouter 1,6 pour les avoir depuis Z = 0).

| Repère | Élément | Centre (X ; Y) | Emprise (X min–max ; Y min–max) | Hauteur |
|---|---|---|---|---|
| J2 | Connecteur batterie JST-PH 2 broches, entrée latérale | (12,0 ; 110,5) | 7,2–16,6 ; 106,4–114,6 | **5,5** |
| BZ1 | Buzzer MLT-8530 | (66,0 ; 103,5) | 61,2–70,8 ; 98,7–108,3 | **3,0** |
| J1 | USB-C | (37,05 ; 117,0) | 32,1–42,0 ; 113,8–122,3 | 3,2 |
| J6 | Connecteur de nappe FH12-45S | (37,0 ; 54,0) | 23,2–50,8 ; 51,7–58,7 | 2,4 |
| L1 | Inductance | (5,5 ; 48,0) | 3,5–7,5 ; 46,0–50,0 | 1,9 |
| U3 | STM32F429 LQFP-144 | (37,05 ; 34,55) | 25,4–48,7 ; 22,9–46,2 | 1,6 |
| U5 | SDRAM TSOP-54 | (37,0 ; 10,3) | 25,7–48,4 ; 3,8–16,8 | 1,2 |
| SW1 | Interrupteur | (2,5 ; 79,0) | bord gauche ; 74,8–83,2 | 1,5 |
| — | Autres CMS (SOT-23, ESOP-8, diodes, 0603, 0402) | partout | — | ≤ 1,7 |
| SW6 | Pattes du joystick (traversantes) | 8 points entre X 10–25, Y 84–102 | — | dépassent d'environ 1,4 |
| J3 | SWD, barrette 1×6 au pas de 2,54 (non montée par défaut) | X = 5,0 ; Y de 12,0 à 24,7 | — | 0 si non montée, ≈ 8,5 si montée |
| J4 | UART, barrette 1×3 (non montée par défaut) | X = 69,0 ; Y de 30,0 à 35,1 | — | 0 si non montée, ≈ 8,5 si montée |

- **Dégagement minimal au dos** : 2 mm sur toute la surface, 3,5 mm sous le buzzer et l'USB, **6 mm au-dessus de J2** (plus la place du câble).
- **J2 (batterie)** : la prise mâle s'insère **depuis la droite** (vers X décroissant) ; laisser libre X de 16,6 à ≈ 30 à Y ≈ 110,5 pour la prise et ses fils. Broche **+** à Y = 109,5, broche **−** à Y = 111,5 (sérigraphiés). La polarité des câbles JST-PH du commerce n'est pas normalisée : vérifier avant de brancher.
- **Buzzer** : la sortie du son est sur le dessus du composant (côté arrière). Prévoir quelques trous ou fentes dans la coque arrière en face de (66,0 ; 103,5).
- **J3 (SWD) et J4 (UART)** : accès de programmation/débogage. Soit des trous d'accès dans la coque arrière, soit rien si la carte est programmée boîtier ouvert. Brochage J3 de haut en bas : 3V3, SWCLK, GND, SWDIO, NRST, SWO. J4 de haut en bas : TX, RX, GND.
- Points de test TP1 à TP6 (VBUS, VBAT, VSYS, VSW, +3V3, GND) : pastilles Ø 1,5 au dos, alignées à Y = 96,5, de X = 33,0 à 49,0 par pas de 3,2.

## 7. Batterie

- Li-ion/LiPo 1 cellule 3,7 V avec circuit de protection, connecteur JST-PH 2 mm.
- **Zone conseillée au dos : X de 28 à 72, Y de 71 à 95 (44 × 24 mm)**. Dans cette zone, le PCB ne porte que des composants de 0,6 mm de haut au plus. Une cellule type 403040 ou 503040 (≈ 40 × 30 × 4 à 5 mm) y tient, à plat contre le PCB avec une cale ou un adhésif mousse d'au moins 1 mm.
- Le boîtier peut aussi loger la batterie dans un compartiment séparé derrière le PCB ; dans ce cas respecter les dégagements du § 6.
- Courant de charge ≈ 780 mA : choisir une cellule d'au moins 800 mAh.

## 8. Empilement en épaisseur (résumé)

| Z | Élément |
|---|---|
| ≈ +18,7 | sommet de la tige du joystick (sans capuchon) |
| +h à +h+2,8 | écran (h = 3 à 5 mm conseillé, voir § 3) |
| +1,5 | sommet des poussoirs A, B, SELECT, START |
| 0 | face avant du PCB |
| −1,6 | face arrière du PCB |
| −3,1 à −3,5 | la plupart des composants du dos |
| −4,6 / −4,8 | buzzer / USB-C |
| −7,1 | sommet du connecteur batterie J2 |
| au-delà | batterie (4 à 6 mm) puis coque arrière |

Épaisseur totale indicative du boîtier : 11 à 12 mm devant le PCB (imposée par le joystick), 1,6 de PCB, 8 à 10 mm derrière (batterie), soit **environ 22 à 25 mm** hors capuchon de joystick.

## 9. Fixation et appuis

- Fixer le PCB par les 6 trous M2.5 sur des colonnettes venant de la coque arrière ou avant (Ø extérieur ≤ 6).
- Les 4 poussoirs et le joystick reçoivent des efforts d'appui : les trous H3/H4 (Y = 70) et H5/H6 (Y = 118) encadrent la zone des commandes. Un appui supplémentaire de la coque arrière sous la zone des boutons A/B est possible dans la zone batterie si la batterie est ailleurs.
- Ne pas appuyer sur le PCB entre Y = 56 et Y = 69 au dos (nappe) ni sur les composants.
- Jeu conseillé entre le chant du PCB et les parois : 0,3 à 0,5 mm.

## 10. Points à valider sur les pièces réelles

1. Longueur utile de la nappe et hauteur h de l'écran (§ 3).
2. Dimensions réelles du joystick : hauteur du corps, tige, débattement (§ 4).
3. Dépassement et course du levier de SW1 (§ 5).
4. Hauteur exacte de l'embase USB-C et de J2 avec sa prise.
