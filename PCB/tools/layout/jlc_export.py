"""Génère les fichiers de fabrication JLCPCB (gerbers + perçage zippés, BOM, CPL)
dans jlcpcb/, au même format que le plugin kicad-jlcpcb-tools (Bouni).

  <python de KiCad> tools/layout/jlc_export.py

Écarts volontaires par rapport au plugin :
- les corrections de rotation ne s'appliquent pas aux empreintes de la bibliothèque EasyEDA
  (déjà dans l'orientation JLC) : seul le LQFP-144 standard KiCad reçoit +270°.
- en plus des fichiers complets, une variante « dos seul » (BOM/CPL-PCB-dos.csv) est écrite.
"""
import csv, os, re, shutil, zipfile
import pcbnew

PRJ = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
OUT = PRJ + '/jlcpcb/production_files'
GERB = PRJ + '/jlcpcb/gerber/PCB'
CORRECTIONS = [(r'^LQFP-', 270), (r'^TQFP-', 270), (r'^SOT-23', -90), (r'^SOT-223', 180), (r'^SOIC-', 270),
               (r'^TSSOP-', 270), (r'^SOP-(?!18_)', 270)]

b = pcbnew.LoadBoard(PRJ + '/PCB.kicad_pcb')
pcbnew.ZONE_FILLER(b).Fill(b.Zones())
aux = b.GetDesignSettings().GetAuxOrigin()

# ---------------------------------------------------------------- gerbers + perçage
os.makedirs(GERB, exist_ok=True)
for f in os.listdir(GERB):
    os.remove(os.path.join(GERB, f))
pctl = pcbnew.PLOT_CONTROLLER(b)
popt = pctl.GetPlotOptions()
popt.SetOutputDirectory(GERB)
popt.SetFormat(1)
popt.SetPlotValue(True)
popt.SetPlotReference(True)
popt.SetSketchPadsOnFabLayers(False)
popt.SetUseGerberProtelExtensions(False)
popt.SetCreateGerberJobFile(False)
popt.SetSubtractMaskFromSilk(True)
popt.SetUseAuxOrigin(True)
popt.SetUseGerberX2format(True)
popt.SetIncludeGerberNetlistInfo(True)
popt.SetDisableGerberMacros(False)
popt.SetDrillMarksType(pcbnew.DRILL_MARKS_NO_DRILL_SHAPE)
popt.SetPlotFrameRef(False)
plan = [('CuTop', pcbnew.F_Cu), ('SilkTop', pcbnew.F_SilkS), ('MaskTop', pcbnew.F_Mask), ('PasteTop', pcbnew.F_Paste)]
plan += [('CuIn%d' % i, getattr(pcbnew, 'In%d_Cu' % i)) for i in range(1, b.GetCopperLayerCount() - 1)]
plan += [('CuBottom', pcbnew.B_Cu), ('SilkBottom', pcbnew.B_SilkS), ('MaskBottom', pcbnew.B_Mask),
         ('PasteBottom', pcbnew.B_Paste), ('EdgeCuts', pcbnew.Edge_Cuts)]
for name, layer in plan:
    popt.SetSkipPlotNPTH_Pads(pcbnew.IsCopperLayer(layer))
    pctl.SetLayer(layer)
    assert pctl.OpenPlotfile(name, pcbnew.PLOT_FORMAT_GERBER, name)
    assert pctl.PlotLayer()
pctl.ClosePlot()
drl = pcbnew.EXCELLON_WRITER(b)
drl.SetOptions(False, False, aux, False)
drl.SetFormat(True)
drl.CreateDrillandMapFilesSet(GERB, True, True)
os.makedirs(OUT, exist_ok=True)
with zipfile.ZipFile(OUT + '/GERBER-PCB.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for f in sorted(os.listdir(GERB)):
        z.write(os.path.join(GERB, f), f)

# ---------------------------------------------------------------- BOM + CPL
def lcsc_of(fp):
    for k in ('LCSC', 'LCSC Part', 'JLC', 'JLCPCB'):
        f = fp.GetFieldByName(k)
        if f and f.GetText().strip():
            return f.GetText().strip()
    return ''


def rotation(fp):
    rot = fp.GetOrientation().AsDegrees()
    if fp.GetLayer() != pcbnew.F_Cu:
        rot = (180 - rot) % 360
    name = str(fp.GetFPID().GetLibItemName())
    if str(fp.GetFPID().GetLibNickname()) != 'EasyEDA':
        for rx, corr in CORRECTIONS:
            if re.search(rx, name):
                rot = (rot + corr % 360) % 360
                break
    return rot % 360


def center(fp):
    pads = list(fp.Pads())
    bb = pads[0].GetBoundingBox()
    for p in pads:
        bb.Merge(p.GetBoundingBox())
    return bb.GetCenter()


parts = []
skipped = []
for fp in sorted(b.GetFootprints(), key=lambda f: (re.sub(r'\d', '', f.GetReference()), int(re.sub(r'\D', '', f.GetReference()) or 0))):
    a = fp.GetAttributes()
    l = lcsc_of(fp)
    if a & (pcbnew.FP_DNP | pcbnew.FP_EXCLUDE_FROM_BOM | pcbnew.FP_EXCLUDE_FROM_POS_FILES | pcbnew.FP_BOARD_ONLY) or not l:
        skipped.append(fp.GetReference())
        continue
    c = center(fp)
    parts.append(dict(ref=fp.GetReference(), val=fp.GetValue(), pkg=str(fp.GetFPID().GetLibItemName()), lcsc=l,
                      x=pcbnew.ToMM(c.x - aux.x), y=-pcbnew.ToMM(c.y - aux.y), rot=rotation(fp),
                      layer='top' if fp.GetLayer() == pcbnew.F_Cu else 'bottom'))


def write(suffix, sel):
    groups = {}
    for p in sel:
        groups.setdefault((p['val'], p['pkg'], p['lcsc']), []).append(p['ref'])
    with open('%s/BOM-PCB%s.csv' % (OUT, suffix), 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['Comment', 'Designator', 'Footprint', 'LCSC', 'Quantity'])
        for (val, pkg, l), refs in sorted(groups.items()):
            w.writerow([val, ','.join(refs), pkg, l, len(refs)])
    with open('%s/CPL-PCB%s.csv' % (OUT, suffix), 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow(['Designator', 'Val', 'Package', 'Mid X', 'Mid Y', 'Rotation', 'Layer'])
        for p in sel:
            w.writerow([p['ref'], p['val'], p['pkg'], '%.6f' % p['x'], '%.6f' % p['y'], p['rot'], p['layer']])
    return len(sel), len(groups)


print('complet :', write('', parts))
print('dos seul :', write('-dos', [p for p in parts if p['layer'] == 'bottom']))
print('face avant :', [p['ref'] for p in parts if p['layer'] == 'top'])
print('exclus (sans LCSC, DNP ou hors BOM) :', skipped)
print('gerbers :', sorted(os.listdir(GERB)))
