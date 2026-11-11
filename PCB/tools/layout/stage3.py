"""Étape 3 : importe pistes/vias, crée les plans, sérigraphie, remplit et sauve."""
import json, sys
from common import *

out = sys.argv[1] if len(sys.argv) > 1 else SCR + '/stage3.kicad_pcb'
b = pcbnew.LoadBoard(SCR + '/stage2.kicad_pcb')
r = json.load(open(SCR + '/routes.json'))
LAY = {'F': pcbnew.F_Cu, 'B': pcbnew.B_Cu}
for t in r['tracks']:
    tr = pcbnew.PCB_TRACK(b)
    tr.SetStart(P(t['x1'], t['y1'])); tr.SetEnd(P(t['x2'], t['y2']))
    tr.SetWidth(mm(t['w'])); tr.SetLayer(LAY[t['layer']])
    tr.SetNet(b.FindNet(t['net']))
    b.Add(tr)
for v in r['vias']:
    vi = pcbnew.PCB_VIA(b)
    vi.SetPosition(P(v['x'], v['y']))
    vi.SetViaType(pcbnew.VIATYPE_THROUGH)
    vi.SetWidth(mm(0.6))
    vi.SetDrill(mm(0.3))
    vi.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    vi.SetNet(b.FindNet(v['net']))
    b.Add(vi)


def zone(net, layer, prio, clearance=0.2, name=''):
    z = pcbnew.ZONE(b)
    z.SetLayer(layer)
    z.SetNet(b.FindNet(net))
    z.SetZoneName(name)
    z.SetAssignedPriority(prio)
    ol = z.Outline()
    ol.NewOutline()
    for x, y in ((0, 0), (W, 0), (W, H), (0, H)):
        ol.Append(mm(X0 + x), mm(Y0 + y))
    z.SetLocalClearance(mm(clearance))
    z.SetMinThickness(mm(0.2))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(mm(0.25))
    z.SetThermalReliefSpokeWidth(mm(0.3))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    z.SetIsFilled(False)
    b.Add(z)
    return z


zone('GND', pcbnew.In1_Cu, 0, name='GND_PLANE')
zone('+3V3', pcbnew.In2_Cu, 0, name='3V3_PLANE')
for lay, nm in ((pcbnew.F_Cu, 'GND_F'), (pcbnew.B_Cu, 'GND_B')):
    zz = zone('GND', lay, 0, 0.25, name=nm)
    zz.SetPadConnection(pcbnew.ZONE_CONNECTION_THT_THERMAL)

# sérigraphie : références visibles seulement pour les gros composants
for f in b.GetFootprints():
    ref = f.GetReference()
    pre = ref.rstrip('0123456789')
    if ref not in ('U3', 'U5', 'J1', 'J2', 'J3', 'J4', 'J6'):
        f.Reference().SetVisible(False)
    for fld in f.GetFields():
        if fld.GetName() not in ('Reference', 'Value'):
            fld.SetVisible(False)


def text(s, x, y, layer, size=1.0, mirror=False, ang=0):
    t = pcbnew.PCB_TEXT(b)
    t.SetText(s); t.SetPosition(P(x, y)); t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    t.SetTextThickness(mm(max(0.12, size * 0.15)))
    t.SetMirrored(mirror)
    if ang:
        t.SetTextAngleDegrees(ang)
    b.Add(t)


FS, BS = pcbnew.F_SilkS, pcbnew.B_SilkS
text('GameGirl', 37, 74.5, FS, 2.2)
text('A', 62, 82.6, FS, 1.5)
text('B', 50, 90.6, FS, 1.5)
text('SELECT', 28, 113.4, FS, 1.0)
text('START', 46, 113.4, FS, 1.0)
text('GameGirl PCB v1', 50, 78, BS, 1.4, True)
text('+', 5.9, 109.5, BS, 1.2, True)
text('-', 5.9, 111.6, BS, 1.2, True)
text('BAT', 12, 105.2, BS, 1.0, True)
text('ON/OFF', 10.5, 79, BS, 1.0, True)
text('SWD: 3V3 CLK GND DIO RST SWO', 9.2, 18.4, BS, 0.8, True, 90)
text('UART: TX RX GND', 65.6, 32.6, BS, 0.8, True, 90)

filler = pcbnew.ZONE_FILLER(b)
filler.Fill(b.Zones())
b.Save(out)
print('sauvé', out, len(b.GetTracks()), 'pistes+vias')
