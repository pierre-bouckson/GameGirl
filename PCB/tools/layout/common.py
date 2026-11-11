"""Helpers partagés : parseur de netlist KiCad + création de carte."""
import re, os
import pcbnew

PRJ = '/Users/pierro/projet/GameGirl/PCB'
SCR = os.path.dirname(os.path.abspath(__file__))
STD = '/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints'
LIBS = {
    'PCM_JLCPCB': PRJ + '/libs/JLCPCB/footprints/JLCPCB.pretty',
    'EasyEDA': PRJ + '/libs/EasyEDA/EasyEDA.pretty',
    'GameGirl': PRJ + '/libs/GameGirl/GameGirl.pretty',
}
X0, Y0 = 100.0, 40.0          # coin haut-gauche de la carte dans la page KiCad
W, H = 74.0, 122.0            # dimensions de la carte


def mm(v):
    return pcbnew.FromMM(v)


def P(x, y):
    """Coordonnées carte (origine coin haut-gauche, vue de face) -> VECTOR2I page."""
    return pcbnew.VECTOR2I(mm(X0 + x), mm(Y0 + y))


def bxy(v):
    """VECTOR2I page -> coordonnées carte en mm."""
    return (pcbnew.ToMM(v.x) - X0, pcbnew.ToMM(v.y) - Y0)


def parse_sexpr(text):
    tok = re.compile(r'"((?:[^"\\]|\\.)*)"|([()])|([^\s()"]+)')
    stack = [[]]
    for m in tok.finditer(text):
        s, par, atom = m.groups()
        if par == '(':
            stack.append([])
        elif par == ')':
            l = stack.pop()
            stack[-1].append(l)
        elif s is not None:
            stack[-1].append(s.replace('\\"', '"'))
        else:
            stack[-1].append(atom)
    return stack[0][0]


def find(l, key):
    return [x for x in l if isinstance(x, list) and x and x[0] == key]


def first(l, key, default=None):
    r = find(l, key)
    return r[0] if r else default


def load_netlist(path):
    root = parse_sexpr(open(path, encoding='utf-8').read())
    comps = {}
    for c in find(first(root, 'components'), 'comp'):
        ref = first(c, 'ref')[1]
        props = {}
        flags = set()
        for p in find(c, 'property'):
            name = first(p, 'name')[1]
            val = first(p, 'value')
            if val is None:
                flags.add(name)
            else:
                props[name] = val[1] if len(val) > 1 else ''
        fields = {}
        for f in find(first(c, 'fields', []), 'field'):
            fields[first(f, 'name')[1]] = f[2] if len(f) > 2 else ''
        sp = first(c, 'sheetpath')
        ds = first(c, 'datasheet')
        comps[ref] = dict(
            ref=ref, value=first(c, 'value')[1], footprint=first(c, 'footprint')[1],
            datasheet=ds[1] if ds and len(ds) > 1 else '',
            props=props, flags=flags, fields=fields,
            sheet=first(sp, 'tstamps')[1], sheetname=first(sp, 'names')[1],
            uuid=first(c, 'tstamps')[1],
            desc=(first(first(c, 'libsource'), 'description') or [0, ''])[1] if first(c, 'libsource') else '')
    nets = {}
    pinfunc = {}
    for n in find(first(root, 'nets'), 'net'):
        name = first(n, 'name')[1]
        nodes = []
        for nd in find(n, 'node'):
            r, p = first(nd, 'ref')[1], first(nd, 'pin')[1]
            nodes.append((r, p))
            pf = first(nd, 'pinfunction')
            pt = first(nd, 'pintype')
            pinfunc[(r, p)] = (pf[1] if pf else '', pt[1] if pt else '')
        nets[name] = nodes
    return comps, nets, pinfunc


def load_fp(fpid):
    lib, name = fpid.split(':')
    path = LIBS.get(lib, f'{STD}/{lib}.pretty')
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise RuntimeError('footprint introuvable ' + fpid)
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp
