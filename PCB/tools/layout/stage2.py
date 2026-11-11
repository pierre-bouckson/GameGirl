"""Étape 2 : contour, fente FPC, trous, placement, repères mécaniques."""
import math, json
from common import *

b = pcbnew.LoadBoard(SCR + '/stage1.kicad_pcb')
FPS = {f.GetReference(): f for f in b.GetFootprints()}
LR = pcbnew.FLIP_DIRECTION_LEFT_RIGHT


def place(ref, x, y, rot=0, side='B'):
    f = FPS[ref]
    f.SetPosition(P(x, y))
    if (side == 'B') != f.IsFlipped():
        f.Flip(P(x, y), LR)
    f.SetOrientationDegrees(rot)
    return f


def padxy(ref, num):
    for p in FPS[ref].Pads():
        if p.GetNumber() == str(num):
            return bxy(p.GetPosition())
    raise KeyError((ref, num))


def aim(ref, x, y, pad, d, side='B'):
    """Place ref en (x,y) avec la rotation qui met le pad `pad` dans la direction d=(dx,dy)."""
    best = None
    for rot in (0, 90, 180, 270):
        place(ref, x, y, rot, side)
        px, py = padxy(ref, pad)
        s = (px - x) * d[0] + (py - y) * d[1]
        if best is None or s > best[0]:
            best = (s, rot)
    return place(ref, x, y, best[1], side)


def two(ref, p1, p2, side='B'):
    """Composant 2 pads : pad1 vers p1, pad2 vers p2 (centre au milieu)."""
    x, y = (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2
    return aim(ref, x, y, 1, (p1[0] - p2[0], p1[1] - p2[1]), side)


def V(ref, x, y, up=1, side='B'):
    """2 pads vertical, pad1 en haut si up=1."""
    return two(ref, (x, y - 0.5 * up), (x, y + 0.5 * up), side)


def Hh(ref, x, y, left=1, side='B'):
    """2 pads horizontal, pad1 à gauche si left=1."""
    return two(ref, (x - 0.5 * left, y), (x + 0.5 * left, y), side)


# ------------------------------------------------------------------ face avant
place('SW6', 17.5, 93.0, 0, 'F')     # joystick
place('SW2', 62.0, 87.0, 0, 'F')     # A
place('SW3', 50.0, 95.0, 0, 'F')     # B
place('SW4', 28.0, 109.0, 0, 'F')    # SELECT
place('SW5', 46.0, 109.0, 0, 'F')    # START

# ------------------------------------------------------------------ zone haute (dos)
UX, UY = 37.05, 34.55        # +0.05 : pads du LQFP sur la grille de 0.1 mm
place('U3', UX, UY, 0)
place('U5', 37.0, 10.3, 180)
aim('J6', 37.0, 54.0, 1, (1, -1))
assert abs(padxy('J6', 1)[0] - 48.0) < 0.01 and padxy('J6', 1)[1] < 54

# condensateurs de découplage MCU : tangentiels, juste à l'extérieur des pads
u3net = {p.GetNumber(): p.GetNetname() for p in FPS['U3'].Pads()}
R_CAP = 12.55


def side_of(num):
    x, y = padxy('U3', num)
    dx, dy = x - UX, y - UY
    if abs(dx) > abs(dy):
        return ('L' if dx < 0 else 'R'), y
    return ('T' if dy < 0 else 'Bo'), x


def mcu_cap(ref, vdd, toward, r=R_CAP, shift=0.0):
    """cap pour la broche vdd ; pad2 décalé vers la broche `toward` (numéro) ou sens +1/-1 en coord."""
    s, c = side_of(vdd)
    if isinstance(toward, str):
        c2 = side_of(toward)[1]
        sgn = 1 if c2 > c else -1
    else:
        sgn = toward
    a = c - 0.25 * sgn + shift  # pad1
    bb = c + 0.75 * sgn + shift # pad2
    if s == 'L':
        return two(ref, (UX - r, a), (UX - r, bb))
    if s == 'R':
        return two(ref, (UX + r, a), (UX + r, bb))
    if s == 'T':
        return two(ref, (a, UY - r), (bb, UY - r))
    return two(ref, (a, UY + r), (bb, UY + r))


mcu_cap('C6', '6', -1)        # pin 6, pas de GND voisin
mcu_cap('C7', '17', '16')
mcu_cap('C8', '30', '29', shift=0.5)     # GND 31 pris par C24
mcu_cap('C9', '39', '38')
mcu_cap('C10', '52', '51')
mcu_cap('C11', '62', '61')
mcu_cap('C12', '72', '73' if False else 1)
mcu_cap('C13', '84', '83')
mcu_cap('C14', '95', '94')
mcu_cap('C15', '108', '107')
mcu_cap('C16', '121', '120')
mcu_cap('C17', '131', '130')
mcu_cap('C20', '144', -1)
mcu_cap('C24', '32', '31', shift=-0.5)    # VDDA / VSSA
# VCAP
x71 = padxy('U3', 71)[0]
two('C21', (x71 - 0.3, UY - 14.2), (x71 - 1.9, UY - 14.2))       # VCAP1 pin 71
y106 = padxy('U3', 106)[1]
two('C22', (UX + 14.3, y106 - 0.2), (UX + 14.3, y106 + 1.4))     # VCAP2 pin 106
# VDDA : FB1 + C19, C18 bulk
y32 = padxy('U3', 32)[1]
two('C19', (UX - 14.5, y32 - 0.2), (UX - 14.5, y32 + 1.4))
two('FB1', (UX - 16.3, y32 + 1.4), (UX - 16.3, y32 - 0.2))
place('TP7', UX - 18.6, y32 - 0.6, 0)
two('C18', (UX - 13.0, UY - 13.6), (UX - 14.6, UY - 13.6))
y25 = padxy('U3', 25)[1]
two('C23', (UX - 14.2, y25 + 0.6), (UX - 15.2, y25 + 0.6))       # NRST
x138 = padxy('U3', 138)[0]
two('R9', (x138, UY + 12.6), (x138, UY + 13.6))                  # BOOT0

# SDRAM : découplage (condensateurs verticaux, pad1 juste sous/sur la broche +3V3)
def u5x(n):
    return padxy('U5', n)[0]
yb = padxy('U5', 1)[1] + 0.9 + 0.75     # rangée du bas (vers le MCU)
yt = padxy('U5', 28)[1] - 0.9 - 0.75    # rangée du haut
for ref, pin in (('C40', 1), ('C46', 3), ('C41', 9), ('C42', 14), ('C43', 27)):
    two(ref, (u5x(pin), yb), (u5x(pin), yb + 1.0))
for ref, pin in (('C44', 43), ('C45', 49)):
    two(ref, (u5x(pin), yt), (u5x(pin), yt - 1.0))
two('C47', (22.6, 9.5), (22.6, 11.1))

# connecteurs debug
aim('J3', 5.0, 12.0, 6, (0, 1))       # SWD 1x6
aim('J4', 69.0, 30.0, 3, (0, 1))      # UART 1x3

# écran : découplage + pull-up
V('R43', 33.0, 49.9, up=1)                       # pad2 = RDX vers J6.31
two('C50', (padxy('J6', 3)[0] + 0.5, 50.0), (padxy('J6', 3)[0] + 1.5, 50.0))
two('C51', (51.3, 48.2), (52.9, 48.2))
# boost rétroéclairage (en bas à gauche de la zone haute)
aim('U6', 11.5, 50.5, 3, (-1, -1))
aim('L1', 5.5, 48.0, 1, (0, -1))
aim('D4', 11.5, 46.2, 2, (-1, 0))
two('C53', (16.0, 45.2), (16.0, 47.2))
two('C52', (5.5, 53.3), (7.1, 53.3))
two('R44', (16.5, 51.5), (16.5, 49.9))
two('C54', (14.3, 54.0), (13.3, 54.0)) if False else None
V('C54', 14.6, 53.6, up=1)
place('TP8', 19.5, 45.5, 0)
place('TP9', 19.5, 49.0, 0)

# ------------------------------------------------------------------ zone basse (dos)
aim('SW1', 2.5, 79.0, 2, (1, 0))                 # levier vers la gauche (bord x=0)
aim('J1', 37.05, 117.0, 'A5', (0, -1))            # USB-C, ouverture vers le bas
aim('J2', 12.0, 110.5, 1, (-1, 0))               # JST batterie, prise vers +x
# joystick : filtres
V('R20', 17.5, 105.3); V('C31', 17.5, 107.5)
V('R21', 19.5, 105.3); V('C32', 19.5, 107.5)
V('R22', 21.5, 105.3, up=-1)
V('C30', 14.0, 104.6)
# boutons : pull-down + anti-rebond
V('R23', 56.0, 91.6); V('C33', 57.6, 91.6)
V('R24', 44.0, 99.6); V('C34', 45.6, 99.6)
V('R25', 23.0, 112.6); V('C35', 24.6, 112.6)
V('R26', 43.2, 105.0); V('C36', 44.8, 105.0)
# USB / charge
V('R1', 34.4, 111.4, up=-1)
V('R2', 39.6, 111.4, up=-1)
two('C1', (42.4, 111.6), (42.4, 110.0))
place('U1', 52.3, 103.6, 0)
V('R3', 49.6, 108.6)
two('C2', (54.0, 108.6), (52.4, 108.6))
Hh('D2', 46.5, 120.3)
Hh('D3', 51.0, 120.3)
V('R4', 46.5, 117.6, up=-1)
V('R5', 51.0, 117.6, up=-1)
Hh('R6', 48.4, 100.6)
aim('Q1', 58.0, 103.6, 3, (-1, 0))
aim('D1', 57.6, 99.9, 2, (-1, 0))
aim('Q3', 58.0, 107.6, 3, (1, 0))
V('R10', 55.6, 106.6)
aim('U2', 59.0, 112.6, 5, (1, 1))
two('C3', (56.0, 112.0), (56.0, 113.6))
two('C4', (62.0, 113.6), (62.0, 112.0))
V('R7', 56.6, 116.6); V('R8', 58.2, 116.6); V('C5', 59.8, 116.6)
# buzzer
place('BZ1', 66.0, 103.5, 0)
aim('Q2', 64.4, 110.8, 3, (0, -1))
aim('D5', 69.4, 110.4, 2, (-1, 0))
V('R37', 63.8, 114.2); V('R38', 65.4, 114.2)
# points de test alimentation
for i, tp in enumerate(['TP1', 'TP2', 'TP3', 'TP4', 'TP5', 'TP6']):
    place(tp, 33.0 + 3.2 * i, 99.0 if False else 96.5, 0)

# ------------------------------------------------------------------ contour + fente + trous
EC = pcbnew.Edge_Cuts
R = 4.0


def seg(x1, y1, x2, y2, layer=EC, w=0.1):
    s = pcbnew.PCB_SHAPE(b)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(P(x1, y1)); s.SetEnd(P(x2, y2))
    s.SetLayer(layer); s.SetWidth(mm(w)); b.Add(s)
    return s


def arc(cx, cy, sx, sy, ang, layer=EC, w=0.1):
    s = pcbnew.PCB_SHAPE(b)
    s.SetShape(pcbnew.SHAPE_T_ARC)
    s.SetCenter(P(cx, cy)); s.SetStart(P(sx, sy))
    s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(ang, pcbnew.DEGREES_T))
    s.SetLayer(layer); s.SetWidth(mm(w)); b.Add(s)
    return s


def rrect(x1, y1, x2, y2, r, layer=EC, w=0.1):
    seg(x1 + r, y1, x2 - r, y1, layer, w); seg(x2, y1 + r, x2, y2 - r, layer, w)
    seg(x2 - r, y2, x1 + r, y2, layer, w); seg(x1, y2 - r, x1, y1 + r, layer, w)
    arc(x1 + r, y1 + r, x1, y1 + r, 90, layer, w)
    arc(x2 - r, y1 + r, x2 - r, y1, 90, layer, w)
    arc(x2 - r, y2 - r, x2, y2 - r, 90, layer, w)
    arc(x1 + r, y2 - r, x1 + r, y2, 90, layer, w)


def rect(x1, y1, x2, y2, layer, w=0.1):
    seg(x1, y1, x2, y1, layer, w); seg(x2, y1, x2, y2, layer, w)
    seg(x2, y2, x1, y2, layer, w); seg(x1, y2, x1, y1, layer, w)


def text(s, x, y, layer, size=1.0, mirror=False):
    t = pcbnew.PCB_TEXT(b)
    t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    t.SetTextThickness(mm(size * 0.15))
    t.SetMirrored(mirror)
    b.Add(t)
    return t


rrect(0, 0, W, H, R)
SLOT = (18.5, 65.75, 55.5, 68.25)        # fente FPC 37 x 2.5
rrect(*SLOT, 1.0)

HOLES = [(4, 4), (70, 4), (4, 70), (70, 70), (4, 118), (70, 118)]
for i, (hx, hy) in enumerate(HOLES):
    fp = load_fp('MountingHole:MountingHole_2.7mm_M2.5')
    b.Add(fp)
    fp.SetReference(f'H{i + 1}')
    fp.SetValue('M2.5')
    fp.SetPosition(P(hx, hy))
    fp.SetAttributes(fp.GetAttributes() | pcbnew.FP_BOARD_ONLY | pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES)
    fp.Reference().SetVisible(False)
    fp.Value().SetVisible(False)

# repères mécaniques (Dwgs.User = face avant, Cmts.User = dos)
DW = pcbnew.Dwgs_User
CM = pcbnew.Cmts_User
DISP = (37 - 21.36, 6.0, 37 + 21.36, 65.46)          # contour module (BL outline)
AA = (37 - 18.36, 9.0, 37 + 18.36, 57.96)            # zone active
rect(*DISP, DW, 0.15)
rect(*AA, DW, 0.1)
text('ECRAN DT024CTFT 42.72x59.46 (dans la coque avant)', 37, 4.6, DW, 0.9)
text('zone active 36.72x48.96', 37, 33.5, DW, 1.0)
rect(19.0, 56.5, 55.0, 65.3, CM, 0.1)
text('nappe FPC repliee (dos) - zone sans composants', 37, 61, CM, 0.8, True)
rect(28.0, 71.0, 72.0, 95.0, CM, 0.1)
text('zone batterie conseillee 44x24 (composants <= 0.6 mm)', 50, 83, CM, 0.8, True)

b.GetDesignSettings().SetAuxOrigin(P(0, 0))
b.GetDesignSettings().SetGridOrigin(P(0, 0)) if hasattr(b.GetDesignSettings(), 'SetGridOrigin') else None

# ------------------------------------------------------------------ contrôle des chevauchements
items = []
for f in b.GetFootprints():
    bb = f.GetBoundingBox(False)
    x1, y1 = bxy(pcbnew.VECTOR2I(bb.GetX(), bb.GetY()))
    items.append((f.GetReference(), f.IsFlipped(), x1, y1, x1 + pcbnew.ToMM(bb.GetWidth()), y1 + pcbnew.ToMM(bb.GetHeight()), f))
bad = 0
for i in range(len(items)):
    for j in range(i + 1, len(items)):
        a, c = items[i], items[j]
        tht = lambda f: any(p.GetAttribute() in (pcbnew.PAD_ATTRIB_PTH, pcbnew.PAD_ATTRIB_NPTH) for p in f.Pads())
        if a[1] != c[1] and not (tht(a[6]) or tht(c[6])):
            continue
        m = 0.0
        if a[2] < c[4] + m and c[2] < a[4] + m and a[3] < c[5] + m and c[3] < a[5] + m:
            print('CHEVAUCHEMENT', a[0], c[0], [round(v, 2) for v in a[2:6]], [round(v, 2) for v in c[2:6]])
            bad += 1
for it in items:
    if it[2] < 0.3 or it[3] < 0.3 or it[4] > W - 0.3 or it[5] > H - 0.3:
        print('HORS CARTE / BORD', it[0], [round(v, 2) for v in it[2:6]])
print('chevauchements:', bad)
b.Save(SCR + '/stage2.kicad_pcb')
