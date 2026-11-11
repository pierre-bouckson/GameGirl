"""Exporte pads/trous de la carte en JSON pour le routeur."""
import json, sys
from common import *
src = sys.argv[1]
b = pcbnew.LoadBoard(src)
pads = []
for f in b.GetFootprints():
    for p in f.Pads():
        bb = p.GetBoundingBox()
        x1, y1 = bxy(pcbnew.VECTOR2I(bb.GetX(), bb.GetY()))
        cx, cy = bxy(p.GetPosition())
        at = p.GetAttribute()
        ls = p.GetLayerSet()
        layers = [n for n, l in (('F', pcbnew.F_Cu), ('B', pcbnew.B_Cu)) if ls.Contains(l)]
        pads.append(dict(ref=f.GetReference(), num=p.GetNumber(), net=p.GetNetname(),
                         x1=round(x1, 4), y1=round(y1, 4), x2=round(x1 + pcbnew.ToMM(bb.GetWidth()), 4),
                         y2=round(y1 + pcbnew.ToMM(bb.GetHeight()), 4), cx=round(cx, 4), cy=round(cy, 4),
                         tht=at in (pcbnew.PAD_ATTRIB_PTH, pcbnew.PAD_ATTRIB_NPTH), npth=at == pcbnew.PAD_ATTRIB_NPTH,
                         drill=pcbnew.ToMM(p.GetDrillSizeX()), drilly=pcbnew.ToMM(p.GetDrillSizeY()), layers=layers,
                         circ=p.GetShape() == pcbnew.PAD_SHAPE_CIRCLE))
json.dump(dict(pads=pads), open(SCR + '/board.json', 'w'))
print(len(pads), 'pads')
