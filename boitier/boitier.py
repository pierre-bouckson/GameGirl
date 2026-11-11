"""Boîtier GameGirl imprimable en 3D (CadQuery).

    <python avec cadquery> boitier.py          -> STL + STEP dans ce dossier

Repère : celui de PCB/MECANIQUE_BOITIER.md (origine coin haut-gauche du PCB, vu de face,
X à droite, Y vers le bas, Z = 0 face avant du PCB, Z > 0 vers le joueur).
En interne, Y_cao = -Y_carte pour garder un repère direct.
"""
import os, math
import cadquery as cq

OUT = os.path.dirname(os.path.abspath(__file__))

# ------------------------------------------------------------------ paramètres
PCB_W, PCB_H, PCB_T, PCB_R = 74.0, 122.0, 1.6, 4.0
GAP = 0.4                       # jeu PCB / paroi
WALL = 2.4                      # épaisseur des parois latérales
Z_FACE = 10.5                   # face avant extérieure
T_FACE = 2.0                    # épaisseur de la face avant
Z_IN = Z_FACE - T_FACE          # 8.5 : face avant intérieure (au-dessus du corps du joystick)
Z_PART = -PCB_T                 # plan de joint (dos du PCB)
LAP = 1.5                       # recouvrement du joint en feuillure
Z_BACK_IN = -10.0               # fond intérieur de la coque arrière
T_BACK = 2.0
Z_BACK = Z_BACK_IN - T_BACK     # -12.0
R_TOP, R_BOT = 6.5, 9.0         # rayons extérieurs (haut / bas)

HOLES = [(4, 4), (70, 4), (4, 70), (70, 70), (4, 118), (70, 118)]
# écran DT024CTFT
DISP = (15.64, 6.00, 58.36, 65.46)       # contour du module
AA_C = (37.0, 33.48)                     # centre de la zone active
WIN = (37.5, 49.8)                       # fenêtre
DISP_T = 2.8
RECESS = 1.5                             # enfoncement de la lunette d'écran
Z_DISP_IN = Z_IN - 1.0                   # 7.5 : appui de la vitre de l'écran
H_DISP = Z_DISP_IN - DISP_T              # 4.7 : dos de l'écran au-dessus du PCB
# commandes
JOY = (17.25, 91.55)                     # axe de la tige (centre des pattes du blindage)
JOY_HOLE = 17.0
BTN_A, BTN_B = (62.0, 87.0), (50.0, 95.0)
BTN_SEL, BTN_STA = (28.0, 109.0), (46.0, 109.0)
SW_TOP = 1.5                             # sommet des poussoirs
# dos
USB_X = 37.05
SW1_Y = 79.0
BUZ = (66.0, 103.5)
LEDS = [(46.5, 120.3), (51.0, 120.3)]
BAT = (7.2, 69.5, 61.0, 105.5)           # logement batterie 53,8 x 36 (cellule 503450 : 53 x 34,8 x 5,8 max)
BAT_T = 6.2
Z_BAT_TOP = Z_BACK_IN + BAT_T            # -3.8 : sommet des nervures (2,2 mm derrière le PCB)


# ------------------------------------------------------------------ outils
def box(x1, y1, x2, y2, z1, z2):
    return (cq.Workplane('XY').workplane(offset=z1)
            .center((x1 + x2) / 2, -(y1 + y2) / 2).rect(x2 - x1, y2 - y1).extrude(z2 - z1))


def cyl(x, y, d, z1, z2):
    return cq.Workplane('XY').workplane(offset=z1).center(x, -y).circle(d / 2).extrude(z2 - z1)


def stadium(x, y, L, w, z1, z2):
    return cq.Workplane('XY').workplane(offset=z1).center(x, -y).slot2D(L, w).extrude(z2 - z1)


def rrect_wire(x1, y1, x2, y2, rtl, rtr, rbr, rbl):
    """Rectangle (coordonnées carte) à rayons de coin indépendants -> Workplane avec un fil fermé."""
    k = 1 - math.sqrt(0.5)
    w = cq.Workplane('XY').moveTo(x1 + rtl, -y1).lineTo(x2 - rtr, -y1)
    w = w.threePointArc((x2 - rtr * k, -(y1 + rtr * k)), (x2, -(y1 + rtr)))
    w = w.lineTo(x2, -(y2 - rbr)).threePointArc((x2 - rbr * k, -(y2 - rbr * k)), (x2 - rbr, -y2))
    w = w.lineTo(x1 + rbl, -y2).threePointArc((x1 + rbl * k, -(y2 - rbl * k)), (x1, -(y2 - rbl)))
    w = w.lineTo(x1, -(y1 + rtl)).threePointArc((x1 + rtl * k, -(y1 + rtl * k)), (x1 + rtl, -y1))
    return w.close()


def prism(off, z1, z2):
    """Contour extérieur du boîtier décalé de `off` vers l'intérieur, extrudé de z1 à z2."""
    m = GAP + WALL - off
    w = rrect_wire(-m, -m, PCB_W + m, PCB_H + m, R_TOP - off, R_TOP - off, R_BOT - off, R_BOT - off)
    return w.extrude(z2 - z1).translate((0, 0, z1))


def cavity(z1, z2):
    """Logement du PCB : contour du PCB + jeu."""
    r = PCB_R + GAP
    w = rrect_wire(-GAP, -GAP, PCB_W + GAP, PCB_H + GAP, r, r, r, r)
    return w.extrude(z2 - z1).translate((0, 0, z1))


# ================================================================== COQUE AVANT
front = prism(0, Z_PART, Z_FACE)
front = front.faces('>Z').edges().fillet(2.5)
front = front.cut(cavity(Z_PART - 1, Z_IN))
# jupe du joint en feuillure : moitié extérieure de la paroi, descend de LAP sous le plan de joint
front = front.union(prism(0, Z_PART - LAP, Z_PART).cut(prism(1.2, Z_PART - LAP - 1, Z_PART + 1)))

# --- lunette d'écran enfoncée (chanfrein à 45°) + logement du module
mx1, my1, mx2, my2 = DISP
PK = 0.3                                                   # jeu autour du module
rx1, ry1, rx2, ry2 = mx1 - 1.6, my1 - 1.7, mx2 + 1.6, my2 + 1.7     # fond de la lunette
rec = (cq.Workplane('XY').workplane(offset=Z_FACE)
       .center((rx1 + rx2) / 2, -(ry1 + ry2) / 2)
       .rect(rx2 - rx1 + 2 * RECESS, ry2 - ry1 + 2 * RECESS).extrude(-RECESS, taper=45))
front = front.cut(rec)
# surépaisseur intérieure sous la lunette : la face d'appui de la vitre descend à Z_DISP_IN
front = front.union(box(rx1 - 1.2, ry1 - 1.2, rx2 + 1.2, ry2 + 1.2, Z_DISP_IN, Z_IN + 0.01))
# nervure de centrage du module (ouverte en bas pour la nappe)
rib = box(mx1 - PK - 1.2, my1 - PK - 1.2, mx2 + PK + 1.2, my2 + PK + 1.2, 1.0, Z_DISP_IN + 0.01)
rib = rib.cut(box(mx1 - PK, my1 - PK, mx2 + PK, my2 + PK, 0, Z_DISP_IN + 1))
rib = rib.cut(box(37 - 19.5, my2 - 2, 37 + 19.5, my2 + 5, 0, Z_DISP_IN - 0.0))
front = front.union(rib)
# fenêtre
front = front.cut(box(AA_C[0] - WIN[0] / 2, AA_C[1] - WIN[1] / 2, AA_C[0] + WIN[0] / 2, AA_C[1] + WIN[1] / 2, 0, Z_FACE + 1))

# --- colonnettes de fixation (vis M2.5 auto-taraudeuse, avant-trou Ø2,1)
for hx, hy in HOLES:
    front = front.union(cyl(hx, hy, 6.0, 0.0, Z_IN + 0.01))
for hx, hy in HOLES:
    front = front.cut(cyl(hx, hy, 2.1, -0.1, Z_IN - 0.8))

# --- joystick
front = front.cut(cyl(JOY[0], JOY[1], JOY_HOLE, Z_IN - 1, Z_FACE + 1))
# --- boutons A / B : trou guidé par un fût
COL_Z = 5.5
for bx, by in (BTN_A, BTN_B):
    front = front.union(cyl(bx, by, 13.0, COL_Z, Z_IN + 0.01))
for bx, by in (BTN_A, BTN_B):
    front = front.cut(cyl(bx, by, 10.4, COL_Z - 1, Z_FACE + 1))
# --- START / SELECT : lumières oblongues guidées
for bx, by in (BTN_SEL, BTN_STA):
    front = front.union(stadium(bx, by, 12.0, 7.5, COL_Z, Z_IN + 0.01))
for bx, by in (BTN_SEL, BTN_STA):
    front = front.cut(stadium(bx, by, 9.4, 4.9, COL_Z - 1, Z_FACE + 1))

# --- marquage gravé
try:
    txt = (cq.Workplane('XY').workplane(offset=Z_FACE - 0.5).center(37.0, -75.3)
           .text('GameGirl', 5.6, 1.0, kind='bold', halign='center', valign='center'))
    front = front.cut(txt)
except Exception as e:                                       # police indisponible : on s'en passe
    print('texte ignoré :', e)

# ================================================================== COQUE ARRIÈRE
back = prism(0, Z_BACK, Z_PART - LAP)
back = back.faces('<Z').edges().fillet(3.5)
back = back.cut(prism(WALL, Z_BACK_IN, Z_PART))
# languette du joint : moitié intérieure de la paroi, monte jusque sous le PCB
tongue = prism(1.3, Z_PART - LAP - 0.01, Z_PART - 0.1).cut(prism(WALL, Z_PART - LAP - 1, Z_PART + 1))
back = back.union(tongue)

# --- colonnettes d'appui sous le PCB + passage de vis et lamage pour la tête
for hx, hy in HOLES:
    back = back.union(cyl(hx, hy, 6.0, Z_BACK_IN - 0.01, Z_PART))
for hx, hy in HOLES:
    back = back.cut(cyl(hx, hy, 2.9, Z_BACK - 1, Z_PART + 1))
    back = back.cut(cyl(hx, hy, 5.4, Z_BACK - 1, Z_BACK + 2.5))

# --- logement batterie : cadre de nervures
bx1, by1, bx2, by2 = BAT
frame = box(bx1 - 1.2, by1 - 1.2, bx2 + 1.2, by2 + 1.2, Z_BACK_IN - 0.01, Z_BAT_TOP)
frame = frame.cut(box(bx1, by1, bx2, by2, Z_BACK_IN - 1, Z_BAT_TOP + 1))
frame = frame.cut(box(6.5, by2 - 1, 18.0, by2 + 2.5, Z_BACK_IN + 1.5, Z_BAT_TOP + 1))     # sortie des fils vers J2
frame = frame.cut(box(60.9, 98.4, 71.1, 108.6, -4.9, 0))                                  # dégagement du buzzer
back = back.union(frame)

# --- buzzer : ouïes
for i in range(4):
    s = stadium(BUZ[0] - 2.7 + 2.2 * i, BUZ[1], 8.0, 1.2, Z_BACK - 1, Z_BACK_IN + 1)
    back = back.cut(s.rotate((BUZ[0] - 2.7 + 2.2 * i, -BUZ[1], 0), (BUZ[0] - 2.7 + 2.2 * i, -BUZ[1], 1), 90))
# --- LED de charge : conduits de lumière
for lx, ly in LEDS:
    back = back.cut(cyl(lx, ly, 2.0, Z_BACK - 1, Z_BACK_IN + 1))
# --- accès SWD (J3) et UART (J4) par pointes de touche
back = back.cut(box(3.3, 10.3, 6.7, 26.4, Z_BACK - 1, Z_BACK_IN + 1))
back = back.cut(box(67.3, 28.3, 70.7, 36.8, Z_BACK - 1, Z_BACK_IN + 1))

# ================================================================== OUVERTURES COMMUNES AUX DEUX COQUES
yb = PCB_H + GAP + WALL
# USB-C : passage de l'embase + dégagement extérieur pour le surmoulage de la prise
usb = box(USB_X - 4.9, PCB_H - 1.0, USB_X + 4.9, yb + 1, -PCB_T - 3.5, -PCB_T + 0.05)
usb_out = (cq.Workplane('XZ').workplane(offset=yb - 1.7).center(USB_X, -PCB_T - 1.7)
           .rect(13.0, 7.0).extrude(-3.0).edges('|Y').fillet(2.0))
# interrupteur : lumière traversante + cuvette extérieure
sw = box(-GAP - WALL - 1, SW1_Y - 4.5, 1.0, SW1_Y + 4.5, -PCB_T - 2.0, -PCB_T + 0.05)
sw_out = box(-GAP - WALL - 1, SW1_Y - 7.0, -GAP - 0.9, SW1_Y + 7.0, -PCB_T - 3.2, -PCB_T + 1.2)
for c in (usb, usb_out, sw, sw_out):
    front = front.cut(c)
    back = back.cut(c)

# ================================================================== PIÈCES RAPPORTÉES
# cale d'écran : cadre en U posé sur le PCB, porte le dos de l'écran
sp = box(mx1 + 0.2, my1 + 0.2, mx2 - 0.2, my2 - 0.2, 0, H_DISP - 0.2)
sp = sp.cut(box(mx1 + 3.2, my1 + 3.2, mx2 - 3.2, my2 + 1, -1, H_DISP + 1))
spacer = sp


def round_cap(d, top):
    """Capuchon rond (A/B) : tête Ø d, collerette de retenue, pied qui appuie sur le poussoir."""
    z0 = SW_TOP + 0.15
    c = cyl(0, 0, 5.0, z0, 4.3)
    c = c.union(cyl(0, 0, d + 2.4, 4.3, 5.3))
    head = cyl(0, 0, d, 5.3, Z_FACE + top).faces('>Z').edges().fillet(1.0)
    return c.union(head)


def pill_cap(L, w, top):
    z0 = SW_TOP + 0.15
    c = cyl(0, 0, 4.0, z0, 4.3)
    c = c.union(stadium(0, 0, L + 2.0, w + 2.0, 4.3, 5.3))
    head = stadium(0, 0, L, w, 5.3, Z_FACE + top).faces('>Z').edges().fillet(0.8)
    return c.union(head)


cap_ab = round_cap(10.0, 1.6)
cap_ss = pill_cap(9.0, 4.5, 1.0)
# capuchon de joystick : champignon emmanché sur la tige (section mesurée sur le modèle 3D : 3,0 x 4,0)
joy_cap = cyl(0, 0, 7.0, 11.6, 17.0).union(cyl(0, 0, 19.0, 16.0, 20.5).faces('>Z').edges().fillet(1.2))
joy_cap = joy_cap.cut(cq.Workplane('XY').workplane(offset=20.5 + 28.5).sphere(29.3))          # cuvette pour le pouce (fond à Z 19,7)
joy_cap = joy_cap.cut(cq.Workplane('XY').workplane(offset=11.5).rect(3.15, 4.15).extrude(7.2))   # tige jusqu'à Z 18,7

# ================================================================== contrôles + export
def vol(s):
    return s.val().Volume()


def place(shape, xy):
    return shape.translate((xy[0], -xy[1], 0))


pcb = cavity(-PCB_T, 0).cut(box(18.5, 65.75, 55.5, 68.25, -2, 1))          # approx. (contour + jeu)
pcb_exact = rrect_wire(0, 0, PCB_W, PCB_H, 4, 4, 4, 4).extrude(PCB_T).translate((0, 0, -PCB_T))
keep = {   # volumes des composants principaux (x1, y1, x2, y2, z1, z2)
    'joystick corps': (9.0, 83.1, 25.5, 100.8, 0, 8.0),
    'bouton A': (59.4, 84.4, 64.6, 89.6, 0, 1.5), 'bouton B': (47.4, 92.4, 52.6, 97.6, 0, 1.5),
    'SELECT': (25.4, 106.4, 30.6, 111.6, 0, 1.5), 'START': (43.4, 106.4, 48.6, 111.6, 0, 1.5),
    'USB-C': (32.6, 113.8, 41.5, 122.3, -4.8, -1.6), 'SW1': (-0.85, 75.6, 4.0, 82.4, -3.1, -1.6),
    'J2': (7.2, 106.4, 16.6, 114.6, -7.1, -1.6), 'buzzer': (61.2, 98.7, 70.8, 108.3, -4.6, -1.6),
    'J6': (23.2, 51.7, 50.8, 58.7, -4.0, -1.6), 'U3': (25.4, 22.9, 48.7, 46.2, -3.2, -1.6),
    'U5': (25.7, 3.8, 48.4, 16.8, -2.8, -1.6), 'L1': (3.5, 46.0, 7.5, 50.0, -3.5, -1.6),
    'batterie': (BAT[0] + 0.5, BAT[1] + 0.6, BAT[0] + 53.5, BAT[1] + 35.4, Z_BACK_IN + 0.1, Z_BACK_IN + 5.9),
    'ecran': (DISP[0], DISP[1], DISP[2], DISP[3], H_DISP, H_DISP + DISP_T),
}
print('volumes (cm3) : avant %.1f, arrière %.1f, cale %.1f' % (vol(front) / 1000, vol(back) / 1000, vol(spacer) / 1000))
print('interférence avant/arrière : %.3f mm3' % vol(front.intersect(back)))
for name, shell in (('avant', front), ('arrière', back)):
    print('interférence %s / PCB : %.3f mm3' % (name, vol(shell.intersect(pcb_exact))))
    for k, b in keep.items():
        v = vol(shell.intersect(box(*b[:4], b[4], b[5])))
        if v > 0.01:
            print('  !! %s touche %s : %.2f mm3' % (name, k, v))
for k in ('ecran',):
    b = keep[k]
    print('cale / écran : %.3f mm3' % vol(spacer.intersect(box(*b[:4], b[4], b[5]))))
caps = [(cap_ab, BTN_A, 'A'), (cap_ab, BTN_B, 'B'), (cap_ss, BTN_SEL, 'SELECT'), (cap_ss, BTN_STA, 'START')]
for c, xy, n in caps:
    print('capuchon %s / coque avant : %.3f mm3' % (n, vol(place(c, xy).intersect(front))))
print('capuchon joystick / coque avant : %.3f mm3' % vol(place(joy_cap, JOY).intersect(front)))
bb = front.union(back).val().BoundingBox()
print('encombrement : %.1f x %.1f x %.1f mm' % (bb.xlen, bb.ylen, bb.zlen))

parts = {'coque_avant': front, 'coque_arriere': back, 'cale_ecran': spacer,
         'capuchon_AB': cap_ab, 'capuchon_start_select': cap_ss, 'capuchon_joystick': joy_cap}
for n, s in parts.items():
    cq.exporters.export(s, '%s/%s.stl' % (OUT, n), tolerance=0.02, angularTolerance=0.15)
    cq.exporters.export(s, '%s/%s.step' % (OUT, n))
asm = cq.Assembly()
asm.add(front, name='coque_avant', color=cq.Color(0.80, 0.80, 0.78))
asm.add(back, name='coque_arriere', color=cq.Color(0.80, 0.80, 0.78))
asm.add(spacer, name='cale_ecran', color=cq.Color(0.2, 0.2, 0.2))
asm.add(pcb_exact, name='pcb', color=cq.Color(0.1, 0.4, 0.2))
for c, xy, n in caps:
    asm.add(place(c, xy), name='capuchon_' + n, color=cq.Color(0.55, 0.1, 0.3))
asm.add(place(joy_cap, JOY), name='capuchon_joystick', color=cq.Color(0.2, 0.2, 0.2))
asm.save(OUT + '/assemblage.step')
# vues de contrôle
for n, s, d in (('avant_face', front, (0, 0, 1)), ('avant_interieur', front, (0, 0, -1)),
                ('arriere_interieur', back, (0, 0, 1)), ('arriere_dos', back, (0, 0, -1)),
                ('iso', front.union(back), (1, -1.2, 1.4))):
    cq.exporters.export(s, '%s/vue_%s.svg' % (OUT, n), opt=dict(width=700, height=1000, projectionDir=d, showAxes=False,
                                                                 showHidden=False, strokeWidth=0.3))
print('export terminé')

# contrôle des ouvertures latérales
for n, bx in (('USB paroi ext.', (31, 123.3, 43, 124.7, -5.0, -2.0)), ('USB traversant', (33, 122.5, 41, 123.0, -4.8, -1.8)),
              ('SW1 traversant', (-2.7, 76, -0.5, 82, -3.4, -1.8))):
    print(n, ': matière restante %.2f mm3' % (vol(front.union(back).intersect(box(*bx[:4], bx[4], bx[5])))))
