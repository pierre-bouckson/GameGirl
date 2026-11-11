# Boîtier GameGirl

Boîtier imprimable en 3D pour le PCB 74 × 122 mm (`../PCB`), généré par `boitier.py` (CadQuery).
Encombrement : 79,6 × 127,6 × 22,5 mm, hors capuchon de joystick.

Les cotes viennent de `../PCB/MECANIQUE_BOITIER.md` (même repère : origine au coin haut-gauche du PCB vu de face, Z = 0 sur la face avant du PCB).

## Pièces à imprimer

| Fichier | Qté | Rôle | Sens d'impression |
|---|---|---|---|
| `coque_avant.stl` | 1 | Face avant, lunette d'écran en retrait, colonnettes de vissage | face avant sur le plateau |
| `coque_arriere.stl` | 1 | Dos, logement batterie, appuis sous le PCB | dos sur le plateau |
| `cale_ecran.stl` | 1 | Cadre en U de 4,5 mm posé sur le PCB, porte le dos de l'écran | à plat |
| `capuchon_AB.stl` | 2 | Boutons A et B (Ø 10) | tête sur le plateau |
| `capuchon_start_select.stl` | 2 | Boutons SELECT et START (9 × 4,5) | tête sur le plateau |
| `capuchon_joystick.stl` | 1 | Champignon emmanché sur la tige du joystick | cuvette vers le haut |

Chaque pièce existe aussi en `.step`. `assemblage.step` contient tout, en position, avec le contour du PCB.

Réglages conseillés : PLA ou PETG, couches de 0,2 mm (0,12 pour les capuchons), 3 périmètres, pas de supports nécessaires pour les coques. Les jeux sont prévus pour une imprimante correctement réglée : 0,4 mm autour du PCB, 0,2 mm autour des capuchons.

## Visserie et batterie

- 6 vis **M2.5 × 16** autotaraudeuses pour plastique (ou M2.5 × 14), tête Ø ≤ 5 mm. Elles entrent par le dos, traversent le PCB et se vissent dans les colonnettes de la coque avant (avant-trou Ø 2,1).
- Batterie : LiPo **503450, 3,7 V, environ 1000 mAh, avec circuit de protection et connecteur JST-PH 2,0 mm**. Logement de 53,8 × 36 × 6,2 mm, derrière la zone des commandes. Les fils sortent par l'encoche en bas à gauche, vers J2.
  - **Vérifier la polarité du connecteur avant de brancher** : + côté Y = 109,5 (repère « + » de la sérigraphie au dos). Elle varie selon les vendeurs.
  - Dimensions de référence : 50 × 34 × 5 nominal, jusqu'à 53 × 34,8 × 5,8 selon les fabricants.

## Montage

1. Poser les 4 capuchons de boutons dans la coque avant retournée.
2. Poser l'écran, vitre contre la lunette, nappe vers le bas.
3. Poser la cale d'écran sur le dos de l'écran (ouverture du U vers le bas).
4. Brancher la nappe sur J6 (au dos du PCB) en la passant par la fente, puis poser le PCB.
5. Brancher la batterie, la loger dans la coque arrière, refermer et visser.
6. Emmancher le capuchon sur la tige du joystick.

## Ouvertures

USB-C en bas, interrupteur à gauche (cuvette pour atteindre le levier), ouïes face au buzzer, deux trous Ø 2 face aux LED de charge, deux fentes au dos pour atteindre les pastilles SWD (J3) et UART (J4) avec des pointes.

## À valider sur les pièces réelles

Ces cotes viennent des modèles 3D et non de mesures. Les paramètres correspondants sont en tête de `boitier.py`.

- **Joystick** : hauteur du corps (8 mm supposés, la face intérieure est à 8,5), section de la tige (3 × 4 mm supposés, logement de 3,15 × 4,15), axe à (17,25 ; 91,55).
- **Écran** : épaisseur du module (2,8 mm) et longueur de nappe. Le dos de l'écran est à 4,7 mm au-dessus du PCB ; si la nappe est trop courte, réduire `Z_DISP_IN` et la cale.
- **Interrupteur SW1** : dépassement du levier (0,85 mm supposés), accessible au fond de la cuvette.
- **Batterie** : épaisseur réelle (6,2 mm disponibles).

## Régénérer

```sh
python3 -m venv venv && ./venv/bin/pip install cadquery
./venv/bin/python boitier.py
```

Le script recalcule les interférences (coques, PCB, composants principaux, capuchons) et les affiche : toutes sont à zéro.
