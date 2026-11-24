# Boîtier GameGirl

Boîtier imprimable en 3D pour le PCB 74 × 122 mm (`../PCB`), généré par `boitier.py` (CadQuery).
Encombrement : 79,6 × 127,6 × 22,5 mm, hors capuchon de joystick.

Les cotes viennent de `../PCB/MECANIQUE_BOITIER.md` (même repère : origine au coin haut-gauche du PCB vu de face, Z = 0 sur la face avant du PCB).

## Pièces à imprimer

| Fichier | Qté | Rôle | Sens d'impression |
|---|---|---|---|
| `coque_avant.stl` | 1 | Face avant plane (fenêtre d'écran chanfreinée, sans lunette en creux), colonnettes et lamages des vis | face avant sur le plateau |
| `coque_arriere.stl` | 1 | Dos, logement batterie, appuis sous le PCB, logements d'écrous | dos sur le plateau |
| `cale_ecran.stl` | 1 | Cadre en U de 4,5 mm posé sur le PCB, porte le dos de l'écran | à plat |
| `capuchon_AB.stl` | 2 | Boutons A et B (Ø 10) | tête sur le plateau |
| `capuchon_start_select.stl` | 2 | Boutons SELECT et START (9 × 4,5) | tête sur le plateau |
| `capuchon_joystick.stl` | 1 | Provisoire : champignon emmanché sur la tige du joystick, en attendant de valider un capuchon PS4/PS5 du commerce (voir plus bas) | cuvette vers le haut |

`prenoms/coque_avant_<Prénom>.stl` : la même coque avant avec un prénom gravé sous START / SELECT (Gaïa, Elsa, Claire, Agnès, Faustine), à imprimer à la place de `coque_avant.stl`. Toutes les coques avant portent « GameGirl » et la signature « by πR » sous l'écran. Les lettres sont en creux de 0,5 mm ; la liste `PRENOMS` et les positions sont dans `boitier.py`.

Chaque pièce existe aussi en `.step`. `assemblage.step` contient tout, en position, avec le contour du PCB.

Réglages conseillés : PLA ou PETG, couches de 0,2 mm (0,12 pour les capuchons), 3 périmètres, pas de supports nécessaires pour les coques. Les jeux sont prévus pour une imprimante correctement réglée : 0,4 mm autour du PCB, 0,2 mm autour des capuchons.

## Visserie et batterie

- 6 vis **M2.5 × 20 à tête bombée** (ISO 7045 ou ISO 7380), tête Ø ≤ 5,4 mm et hauteur ≤ 2 mm, et 6 **écrous M2.5** (ISO 4032, 5 mm sur plats, 2 mm d'épaisseur).
  Les vis entrent par la face avant (lamage Ø 5,6 × 2), traversent la colonnette avant, le PCB et la colonnette arrière, et se serrent dans un écrou logé dans un 6 pans au dos de la coque arrière (5,4 sur plats, 2,4 de profondeur). Les colonnettes arrière ne travaillent plus qu'en compression : Ø 6 au contact du PCB, Ø 8 en dessous.
- Batterie : LiPo **503450, 3,7 V, environ 1000 mAh, avec circuit de protection et connecteur JST-PH 2,0 mm**. Logement de 53,8 × 36 × 6,2 mm, derrière la zone des commandes. Les fils sortent par l'encoche en bas à gauche, vers J2.
  - **Vérifier la polarité du connecteur avant de brancher** : + côté Y = 109,5 (repère « + » de la sérigraphie au dos). Elle varie selon les vendeurs.
  - Dimensions de référence : 50 × 34 × 5 nominal, jusqu'à 53 × 34,8 × 5,8 selon les fabricants.

## Montage

1. Poser les 4 capuchons de boutons dans la coque avant retournée, puis engager les 6 vis par l'avant (elles tiennent les pièces alignées pendant le montage).
2. Poser l'écran, vitre contre la lunette, nappe vers le bas.
3. Poser la cale d'écran sur le dos de l'écran (ouverture du U vers le bas).
4. Brancher la nappe sur J6 (au dos du PCB) en la passant par la fente, puis poser le PCB.
5. Brancher la batterie, la loger dans la coque arrière, refermer.
6. Glisser les écrous dans les 6 pans du dos et serrer les vis depuis l'avant.
7. Emmancher le capuchon sur la tige du joystick.

## Ouvertures

USB-C en bas : la paroi est ouverte sur 13,5 × 7,5 (R 2,5) jusqu'au nez de l'embase, pour que le surmoulage de la prise vienne en butée. Interrupteur à gauche : lumière de 10 × 2,7 et cuvette évasée à 45° (16 × 5,2 au fond, ≈ 20 × 9 en surface) dont le fond affleure le bout du levier. Ouïes face au buzzer, deux trous Ø 2 face aux LED de charge, deux fentes au dos pour atteindre les pastilles SWD (J3) et UART (J4) avec des pointes.

## À valider sur les pièces réelles

Ces cotes viennent des modèles 3D et non de mesures. Les paramètres correspondants sont en tête de `boitier.py`.

- **Joystick** (datasheet `../docs/joy.pdf`, YHE-YG16-001) : corps de 7,5 mm (face intérieure à 8,5), tige ronde Ø 4,0 à deux méplats de 3,0 (trou central Ø 1,5), haut de tige à 18,95 mm du PCB, axe à (17,25 ; 91,55).
- **Capuchon PS4/PS5 du commerce** (fiche vendeur : jupe Ø 24, tête Ø 18, hauteur 15) : le trou de la face avant est passé à **Ø 26**. C'est le rayon balayé par la jupe à 30° autour du pivot de la tige (Z = 7,5 d'après la datasheet), plus 0,75 mm de jeu, quelle que soit la hauteur du capuchon sur la tige.
  Reste à vérifier à la réception : la **profondeur du logement de tige** (bas de jupe → fond du logement). Le bas de jupe est à Z = 18,95 − cette profondeur. S'il est en dessous de Z ≈ 11, la jupe risque de toucher le **corps du joystick** (et non le boîtier) avant 30°, ce que le boîtier ne peut pas corriger. Une fois le capuchon validé, retirer `capuchon_joystick.stl/.step` des pièces à imprimer.
- **Écran** : épaisseur du module (2,8 mm) et longueur de nappe. Le dos de l'écran est à 4,7 mm au-dessus du PCB ; si la nappe est trop courte, réduire `Z_DISP_IN` et la cale.
- **Interrupteur SW1** : dépassement du levier (0,85 mm supposés), accessible au fond de la cuvette.
- **Batterie** : épaisseur réelle (6,2 mm disponibles).

## Régénérer

```sh
python3.12 -m venv venv && ./venv/bin/pip install --only-binary=:all: "cadquery==2.5.2"   # pas de roues OCP/nlopt utilisables en 3.14 sur ce Mac
./venv/bin/python boitier.py
```

Le script recalcule les interférences (coques, PCB, composants principaux, capuchons) et contrôle les ouvertures USB / interrupteur et les passages de vis : tout est à zéro.
