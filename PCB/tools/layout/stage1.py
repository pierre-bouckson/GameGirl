"""Étape 1 : carte vierge 4 couches + toutes les empreintes du schéma, nets affectés."""
import sys
from common import *

comps, nets, pinfunc = load_netlist(SCR + '/net.net')
b = pcbnew.CreateEmptyBoard()
b.SetCopperLayerCount(4)
b.SetLayerName(pcbnew.In1_Cu, 'In1.Cu')
b.SetLayerName(pcbnew.In2_Cu, 'In2.Cu')
b.SetLayerType(pcbnew.In1_Cu, pcbnew.LT_POWER)
b.SetLayerType(pcbnew.In2_Cu, pcbnew.LT_POWER)
ls = b.GetEnabledLayers()
for l in (pcbnew.In1_Cu, pcbnew.In2_Cu):
    ls.AddLayer(l)
b.SetEnabledLayers(ls)
b.SetVisibleLayers(ls)

# nets
netinfo = {}
for name in nets:
    ni = b.FindNet(name)
    if ni is None:
        ni = pcbnew.NETINFO_ITEM(b, name)
        b.Add(ni)
    netinfo[name] = ni
padnet = {}
for name, nodes in nets.items():
    for r, p in nodes:
        padnet[(r, p)] = name

seen_nc = {}
SKIP = {'Sheetname', 'Sheetfile', 'ki_keywords', 'ki_fp_filters', 'ki_description'}
for ref, c in sorted(comps.items()):
    fp = load_fp(c['footprint'])
    b.Add(fp)
    fp.SetReference(ref)
    fp.SetValue(c['value'])
    fp.SetPath(pcbnew.KIID_PATH(c['sheet'] + c['uuid']))
    fp.SetSheetname(c['sheetname'])
    fp.SetSheetfile(c['props'].get('Sheetfile', ''))
    fp.SetLibDescription(c['desc'])
    d = fp.GetFieldByName('Datasheet')
    if d:
        d.SetText(c['datasheet'])
    for k, v in c['fields'].items():
        if k in ('Footprint', 'Datasheet', 'Description', 'Reference', 'Value'):
            continue
        fp.SetField(k, v)
        fld = fp.GetFieldByName(k)
        fld.SetVisible(False)
    attr = fp.GetAttributes()
    if 'exclude_from_bom' in c['flags']:
        attr |= pcbnew.FP_EXCLUDE_FROM_BOM
    if 'exclude_from_board' in c['flags']:
        pass
    if 'dnp' in c['flags']:
        attr |= pcbnew.FP_DNP
    fp.SetAttributes(attr)
    for pad in fp.Pads():
        n = padnet.get((ref, pad.GetNumber()))
        if not n and pad.GetDrillSizeX() > 0 and pad.GetDrillSizeX() >= pad.GetSizeX():
            pad.SetAttribute(pcbnew.PAD_ATTRIB_NPTH)
        if n and n.startswith('unconnected-'):
            k = seen_nc.get(n, 0)
            seen_nc[n] = k + 1
            if k:
                nn = '%s_%d' % (n, k)
                ni = pcbnew.NETINFO_ITEM(b, nn)
                b.Add(ni)
                netinfo[nn] = ni
                n = nn
        if n:
            pad.SetNet(netinfo[n])
            pf = pinfunc.get((ref, pad.GetNumber()))
            if pf:
                pad.SetPinFunction(pf[0])
                pad.SetPinType(pf[1])
    fp.SetPosition(P(-30, 0))

b.Save(SCR + '/stage1.kicad_pcb')
print('ok', len(b.GetFootprints()))
