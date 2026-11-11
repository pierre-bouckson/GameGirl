"""Routeur sur grille 0.1 mm, 2 couches signal (F/B) + vias traversants.
GND et +3V3 : simple descente par via vers les plans internes (In1 = GND, In2 = +3V3).
Entrée : board.json (export.py). Sortie : routes.json (pistes + vias).
"""
import json, heapq, math, sys, time, os

SCR = os.path.dirname(os.path.abspath(__file__))
G = 0.1
W_MM, H_MM = 74.0, 122.0
NX, NY = int(round(W_MM / G)) + 1, int(round(H_MM / G)) + 1
N = NX * NY
C = 0.15                  # isolement routé
SIGW = 0.15               # largeur signal
VIA_R = 0.3               # via 0.6 / 0.3
VIA_COST = 30.0
HW = 1.6                  # pondération de l'heuristique A*
PLANES = {'GND', '+3V3'}
WIDE = {'/Alimentation/VBUS': 0.5, '/Alimentation/VBAT': 0.5, '/Alimentation/VSYS': 0.5, 'VSW': 0.5,
        '/Ecran/BL_SW': 0.5, '/Ecran/BL_A': 0.3, '/Ecran/BL_FB': 0.3, '/MCU/VDDA': 0.3, '/MCU/VCAP1': 0.3,
        '/MCU/VCAP2': 0.3, 'Net-(Q2-D)': 0.3}
PLANE_W = 0.3
SLOT = (18.5, 65.75, 55.5, 68.25)
HOLES = [(4, 4), (70, 4), (4, 70), (70, 70), (4, 118), (70, 118)]
EDGE = 0.6                # axe de piste >= 0.6 mm du bord
R_CORNER = 4.0

board = json.load(open(SCR + '/board.json'))
pads = board['pads']
netid = {}
for p in pads:
    if p['net'] and p['net'] not in netid:
        netid[p['net']] = len(netid) + 1
idnet = {v: k for k, v in netid.items()}

trk = [[0] * N, [0] * N]      # 0 libre, n>0 réservé au net n, -1 interdit
via = [[0] * N, [0] * N]
novia = [bytearray(N), bytearray(N)]
fan = [bytearray(N), bytearray(N)]   # zones d'échappement des CI à pas fin (pénalité via de plan)


def cell_range(a, b):
    lo = int(math.floor(a / G + 1e-6)) + 1
    hi = int(math.ceil(b / G - 1e-6)) - 1
    return lo, hi


def mark_rect(m, x1, y1, x2, y2, infl, net):
    xa, xb = cell_range(x1 - infl, x2 + infl)
    ya, yb = cell_range(y1 - infl, y2 + infl)
    xa = max(xa, 0); ya = max(ya, 0); xb = min(xb, NX - 1); yb = min(yb, NY - 1)
    for y in range(ya, yb + 1):
        base = y * NX
        for i in range(base + xa, base + xb + 1):
            v = m[i]
            if v == 0:
                m[i] = net
            elif v != net:
                m[i] = -1


_disc = {}


def disc(r_mm):
    k = round(r_mm, 4)
    if k not in _disc:
        rc = r_mm / G
        n = int(rc) + 1
        _disc[k] = [(dx, dy) for dx in range(-n, n + 1) for dy in range(-n, n + 1) if dx * dx + dy * dy < rc * rc - 1e-6]
    return _disc[k]


def mark_disc(m, cx, cy, r_mm, net):
    for dx, dy in disc(r_mm):
        x = cx + dx; y = cy + dy
        if 0 <= x < NX and 0 <= y < NY:
            i = y * NX + x
            v = m[i]
            if v == 0:
                m[i] = net
            elif v != net:
                m[i] = -1


def set_rect(m, x1, y1, x2, y2, val=1):
    xa, xb = cell_range(x1, x2); ya, yb = cell_range(y1, y2)
    xa = max(xa, 0); ya = max(ya, 0); xb = min(xb, NX - 1); yb = min(yb, NY - 1)
    for y in range(ya, yb + 1):
        for i in range(y * NX + xa, y * NX + xb + 1):
            m[i] = val


# ---------------------------------------------------------------- obstacles fixes
def inside_board(x, y, m):
    """True si (x,y) mm est à plus de m du contour arrondi."""
    if x < m or y < m or x > W_MM - m or y > H_MM - m:
        return False
    r = R_CORNER
    for cx, cy in ((r, r), (W_MM - r, r), (r, H_MM - r), (W_MM - r, H_MM - r)):
        if (x < r and cx == r or x > W_MM - r and cx != r) and (y < r and cy == r or y > H_MM - r and cy != r):
            if math.hypot(x - cx, y - cy) > r - m:
                return False
    return True


for y in range(NY):
    for x in range(NX):
        xm, ym = x * G, y * G
        near_edge = xm < 4.7 or ym < 4.7 or xm > W_MM - 4.7 or ym > H_MM - 4.7
        if near_edge:
            i = y * NX + x
            if not inside_board(xm, ym, EDGE):
                trk[0][i] = trk[1][i] = -1
            if not inside_board(xm, ym, 0.5 + VIA_R + 0.05):
                via[0][i] = via[1][i] = -1
for l in (0, 1):
    mark_rect(trk[l], *SLOT, EDGE, -1)
    mark_rect(via[l], *SLOT, 0.5 + VIA_R + 0.05, -1)
    for hx, hy in HOLES:
        mark_disc(trk[l], int(round(hx / G)), int(round(hy / G)), 3.1, -1)
        mark_disc(via[l], int(round(hx / G)), int(round(hy / G)), 3.4, -1)

FINE = {'U3', 'J6', 'U5'}
for p in pads:
    n = netid.get(p['net'], -1)
    ls = [0, 1] if p['tht'] else [0 if p['layers'] == ['F'] else 1]
    for l in ls:
        if p['npth']:
            r = p['drill'] / 2
            cx, cy = int(round(p['cx'] / G)), int(round(p['cy'] / G))
            mark_disc(trk[l], cx, cy, r + 0.25 + SIGW / 2 + 0.05, -1)
            mark_disc(via[l], cx, cy, r + 0.25 + VIA_R + 0.05, -1)
            continue
        mark_rect(trk[l], p['x1'], p['y1'], p['x2'], p['y2'], C + SIGW / 2, n)
        mark_rect(via[l], p['x1'], p['y1'], p['x2'], p['y2'], C + VIA_R, n)
        set_rect(novia[l], p['x1'] - VIA_R - 0.06, p['y1'] - VIA_R - 0.06, p['x2'] + VIA_R + 0.06, p['y2'] + VIA_R + 0.06)
        if p['ref'] in FINE and not p['net'] in PLANES and not p['net'].startswith('unconnected'):
            w, h = p['x2'] - p['x1'], p['y2'] - p['y1']
            ex = 0.3 if w < h else 1.6
            ey = 1.6 if w < h else 0.3
            set_rect(fan[l], p['x1'] - ex, p['y1'] - ey, p['x2'] + ex, p['y2'] + ey)

hard_trk = trk
hard_via = via

# ---------------------------------------------------------------- état du routage
hist = [[0.0] * N, [0.0] * N]     # coût d'historique (cellules disputées)
pathcell = [dict(), dict()]      # cellule -> (net, e)
viacell = dict()                 # cellule -> net
routes = {}                      # net -> liste de {'path': [(l,c)...], 'w':, 'vias': [c...]}
tree = {}                        # net -> set d'états connectés (l*N + c)


def hw_of(e):
    return {0: SIGW / 2, 1: 0.15, 2: 0.25}[e]


def lay_cell(l, c, net, e):
    hw = hw_of(e)
    mark_disc(trk[l], c % NX, c // NX, hw + C + SIGW / 2, net)
    mark_disc(via[l], c % NX, c // NX, hw + C + VIA_R, net)


def lay_via(c, net):
    for l in (0, 1):
        mark_disc(trk[l], c % NX, c // NX, VIA_R + C + SIGW / 2, net)
        mark_disc(via[l], c % NX, c // NX, VIA_R + C + VIA_R, net)


def commit(net, path, e):
    """path : liste d'états (l*N+c) contigus."""
    vias = []
    for k, s in enumerate(path):
        l, c = divmod(s, N)
        if k and path[k - 1] // N != l:
            vias.append(c)
            viacell[c] = net
            lay_via(c, net)
        pathcell[l][c] = (net, e)
        lay_cell(l, c, net, e)
    routes.setdefault(net, []).append(dict(path=path, e=e, vias=vias))
    tree.setdefault(net, set()).update(path)


def rip(net):
    """Retire toutes les pistes du net et reconstruit localement les cartes."""
    region = [set(), set()]
    for r in routes.pop(net, []):
        for s in r['path']:
            l, c = divmod(s, N)
            if pathcell[l].get(c, (None,))[0] == net:
                del pathcell[l][c]
            x, y = c % NX, c // NX
            for dx, dy in disc(0.25 + C + VIA_R):
                xx, yy = x + dx, y + dy
                if 0 <= xx < NX and 0 <= yy < NY:
                    region[l].add(yy * NX + xx)
        for c in r['vias']:
            viacell.pop(c, None)
            x, y = c % NX, c // NX
            for dx, dy in disc(VIA_R + C + VIA_R):
                xx, yy = x + dx, y + dy
                if 0 <= xx < NX and 0 <= yy < NY:
                    region[0].add(yy * NX + xx); region[1].add(yy * NX + xx)
    tree.pop(net, None)
    for l in (0, 1):
        for c in region[l]:
            trk[l][c] = hard_trk[l][c]
            via[l][c] = hard_via[l][c]
    rv = disc(VIA_R + C + VIA_R + 0.05)
    for l in (0, 1):
        ext = set()
        for c in region[l]:
            x, y = c % NX, c // NX
            for dx, dy in rv:
                if 0 <= x + dx < NX and 0 <= y + dy < NY:
                    ext.add((y + dy) * NX + x + dx)
        for c in ext:
            v = pathcell[l].get(c)
            if v:
                lay_cell(l, c, v[0], v[1])
            n2 = viacell.get(c)
            if n2:
                lay_via(c, n2)


def pad_states(p, margin=0.04):
    ls = [0, 1] if p['tht'] else [0 if p['layers'] == ['F'] else 1]
    xa, xb = cell_range(p['x1'] + margin, p['x2'] - margin)
    ya, yb = cell_range(p['y1'] + margin, p['y2'] - margin)
    out = []
    w = p['x2'] - p['x1']; h = p['y2'] - p['y1']
    rr = min(w, h) / 2
    round_pad = p['tht'] or p['circ'] or p['ref'].startswith('TP')
    ax = max(0.0, w / 2 - rr); ay = max(0.0, h / 2 - rr)
    mx = (p['x1'] + p['x2']) / 2; my = (p['y1'] + p['y2']) / 2
    for l in ls:
        for y in range(ya, yb + 1):
            for x in range(xa, xb + 1):
                if round_pad:
                    dx = max(0.0, abs(x * G - mx) - ax); dy = max(0.0, abs(y * G - my) - ay)
                    if dx * dx + dy * dy > (rr - 0.08) ** 2:
                        continue
                out.append(l * N + y * NX + x)
    if not out:
        cx, cy = int(round(p['cx'] / G)), int(round(p['cy'] / G))
        out = [l * N + cy * NX + cx for l in ls]
    return out


MOVES = [(1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0), (1, 1, 1.4142), (1, -1, 1.4142), (-1, 1, 1.4142), (-1, -1, 1.4142)]
MOVE_D = [(dx + dy * NX, cost, k, dx, dy) for k, (dx, dy, cost) in enumerate(MOVES)]
E_OFF = {1: [(1, 0), (-1, 0), (0, 1), (0, -1)],
         2: [(dx, dy) for dx in range(-2, 3) for dy in range(-2, 3) if 0 < dx * dx + dy * dy <= 4]}


def via_ok(c, net):
    v0 = via[0][c]; v1 = via[1][c]
    return (v0 == 0 or v0 == net) and (v1 == 0 or v1 == net) and not novia[0][c] and not novia[1][c]


def search(net, starts, goals, e=0, soft=False, plane_goal=False, maxexp=3000000, layers=(0, 1)):
    """A*. starts : états (coût 0). goals : set d'états. plane_goal : s'arrête sur un site de via libre.
    soft : autorise à traverser les pistes d'autres nets (coût élevé) pour l'arrachage."""
    if not plane_goal:
        gx = [(s % N) % NX for s in goals]; gy = [(s % N) // NX for s in goals]
        gx1, gx2, gy1, gy2 = min(gx), max(gx), min(gy), max(gy)
    g = {}
    par = {}
    pdir = {}
    heap = []
    for s in starts:
        g[s] = 0.0
        c = s % N
        if plane_goal:
            h = 0
        else:
            x, y = c % NX, c // NX
            dx = gx1 - x if x < gx1 else (x - gx2 if x > gx2 else 0)
            dy = gy1 - y if y < gy1 else (y - gy2 if y > gy2 else 0)
            h = max(dx, dy) + 0.4142 * min(dx, dy)
        heap.append((h, 0.0, s))
    heapq.heapify(heap)
    eoff = E_OFF.get(e)
    nexp = 0
    best_plane = None
    while heap:
        f, gs, s = heapq.heappop(heap)
        if gs > g.get(s, 1e18):
            continue
        if plane_goal:
            if best_plane is not None and gs >= best_plane[0]:
                break
            if s in goals and gs > 0:
                best_plane = (gs, s, False)
                break
        elif s in goals:
            path = [s]
            while s in par:
                s = par[s]; path.append(s)
            return path[::-1], gs
        nexp += 1
        if nexp > maxexp:
            return None, None
        l, c = divmod(s, N)
        x, y = c % NX, c // NX
        tm = trk[l]
        hm = hard_trk[l]
        hm2 = hist[l]
        d0 = pdir.get(s, -1)
        if plane_goal and via_ok(c, net) and gs > 0:
            cost = gs + 6.0 + (40.0 if (fan[0][c] or fan[1][c]) else 0.0)
            if best_plane is None or cost < best_plane[0]:
                best_plane = (cost, s, True)
        for dc, mc, k, dx, dy in MOVE_D:
            xx = x + dx; yy = y + dy
            if xx < 0 or yy < 0 or xx >= NX or yy >= NY:
                continue
            c2 = c + dc
            v = tm[c2]
            pen = 0.0
            if v != 0 and v != net:
                if not soft:
                    continue
                hv = hm[c2]
                if hv != 0 and hv != net:
                    continue
                pen = 10.0
            if eoff:
                ok = True
                for ex, ey in eoff:
                    x3 = xx + ex; y3 = yy + ey
                    if x3 < 0 or y3 < 0 or x3 >= NX or y3 >= NY:
                        ok = False; break
                    v3 = tm[y3 * NX + x3]
                    if v3 != 0 and v3 != net:
                        if soft:
                            h3 = hm[y3 * NX + x3]
                            if h3 == 0 or h3 == net:
                                pen = 10.0
                                continue
                        ok = False; break
                if not ok:
                    continue
            elif k >= 4:
                # diagonale : les deux cellules orthogonales doivent être libres
                va = tm[c + dx]; vb = tm[c + dy * NX]
                if (va != 0 and va != net) or (vb != 0 and vb != net):
                    if not soft:
                        continue
                    ha = hm[c + dx]; hb = hm[c + dy * NX]
                    if (ha != 0 and ha != net) or (hb != 0 and hb != net):
                        continue
                    pen = 10.0
            ng = gs + mc + pen + hm2[c2] + (0.25 if (d0 != k and d0 != -1) else 0.0)
            s2 = l * N + c2
            if ng < g.get(s2, 1e18):
                g[s2] = ng; par[s2] = s; pdir[s2] = k
                if plane_goal:
                    h = 0
                else:
                    ddx = gx1 - xx if xx < gx1 else (xx - gx2 if xx > gx2 else 0)
                    ddy = gy1 - yy if yy < gy1 else (yy - gy2 if yy > gy2 else 0)
                    h = max(ddx, ddy) + 0.4142 * min(ddx, ddy)
                heapq.heappush(heap, (ng + HW * h, ng, s2))
        # via
        if not plane_goal and len(layers) == 2:
            okv = via_ok(c, net)
            pen = 0.0
            if not okv and soft:
                hv0 = hard_via[0][c]; hv1 = hard_via[1][c]
                if (hv0 == 0 or hv0 == net) and (hv1 == 0 or hv1 == net) and not novia[0][c] and not novia[1][c]:
                    okv = True; pen = 40.0
            if okv:
                l2 = 1 - l
                v = trk[l2][c]
                if v == 0 or v == net or (soft and (hard_trk[l2][c] in (0, net))):
                    if v != 0 and v != net:
                        pen += 10.0
                    s2 = l2 * N + c
                    ng = gs + VIA_COST + pen
                    if ng < g.get(s2, 1e18):
                        g[s2] = ng; par[s2] = s; pdir[s2] = -1
                        dx_ = gx1 - x if x < gx1 else (x - gx2 if x > gx2 else 0)
                        dy_ = gy1 - y if y < gy1 else (y - gy2 if y > gy2 else 0)
                        heapq.heappush(heap, (ng + HW * (max(dx_, dy_) + 0.4142 * min(dx_, dy_)), ng, s2))
    if plane_goal and best_plane:
        cost, s, need_via = best_plane
        path = [s]
        while s in par:
            s = par[s]; path.append(s)
        return path[::-1], need_via
    return None, None


def conflicts(path, net, e):
    """Nets dont les pistes/vias gênent ce chemin."""
    out = set()
    hw = hw_of(e)
    rt = disc(hw + C + 0.25 + 0.01)
    rvia = disc(hw + C + VIA_R + 0.01)
    rvt = disc(VIA_R + C + 0.25 + 0.01)
    rvv = disc(VIA_R + C + VIA_R + 0.01)
    for k, s in enumerate(path):
        l, c = divmod(s, N)
        x, y = c % NX, c // NX
        isvia = k and path[k - 1] // N != l
        for dx, dy in rt:
            v = pathcell[l].get((y + dy) * NX + x + dx)
            if v and v[0] != net and dx * dx + dy * dy < ((hw + C + hw_of(v[1])) / G) ** 2 - 1e-6:
                out.add(v[0])
        for dx, dy in rvia:
            n2 = viacell.get((y + dy) * NX + x + dx)
            if n2 and n2 != net:
                out.add(n2)
        if isvia:
            for ll in (0, 1):
                for dx, dy in rvt:
                    v = pathcell[ll].get((y + dy) * NX + x + dx)
                    if v and v[0] != net and dx * dx + dy * dy < ((VIA_R + C + hw_of(v[1])) / G) ** 2 - 1e-6:
                        out.add(v[0])
            for dx, dy in rvv:
                n2 = viacell.get((y + dy) * NX + x + dx)
                if n2 and n2 != net:
                    out.add(n2)
    return out


# ---------------------------------------------------------------- nets
netpads = {}
for p in pads:
    if p['net'] and not p['net'].startswith('unconnected') and not p['npth']:
        netpads.setdefault(p['net'], []).append(p)

t0 = time.time()
# 1) plans : chaque pad CMS GND/+3V3 descend par un via
plane_fail = []
for pname in ('+3V3', 'GND'):
    n = netid[pname]
    conn = set()
    for p in netpads[pname]:
        if p['tht']:
            conn.update(pad_states(p))
    tree[n] = conn
    smd = [p for p in netpads[pname] if not p['tht']]
    # d'abord les passifs (leurs vias servent ensuite aux CI), puis le reste
    smd.sort(key=lambda p: (0 if p['ref'][0] in 'CRLDF' else 1, p['ref'], p['num']))
    for p in smd:
        st = pad_states(p)
        if any(s in conn for s in st):
            conn.update(st)
            continue
        l0 = st[0] // N
        w = min(p['x2'] - p['x1'], p['y2'] - p['y1'])
        e = 1 if w >= 0.24 else 0
        path, need_via = search(n, st, conn, e=e, plane_goal=True, maxexp=60000)
        if path is None and e:
            e = 0
            path, need_via = search(n, st, conn, e=0, plane_goal=True, maxexp=60000)
        if path is None:
            plane_fail.append((pname, p['ref'], p['num']))
            continue
        if need_via:
            s = path[-1]
            path = path + [(1 - s // N) * N + s % N]
        commit(n, path, e)
        conn.update(st)
        conn.update(path)
hard_trk = [list(trk[0]), list(trk[1])]   # les descentes de plan deviennent des obstacles fixes
hard_via = [list(via[0]), list(via[1])]
print('plans: %d échecs %s (%.1fs)' % (len(plane_fail), plane_fail, time.time() - t0)); sys.stdout.flush()


# 2) signaux
def net_conns(name):
    return netpads[name]


def hpwl(name):
    ps = netpads[name]
    return (max(p['cx'] for p in ps) - min(p['cx'] for p in ps)) + (max(p['cy'] for p in ps) - min(p['cy'] for p in ps))


signals = [n for n in netpads if n not in PLANES and len(netpads[n]) >= 2]
PRIO = lambda n: (0 if n in WIDE else 1, hpwl(n))
signals.sort(key=PRIO)
failed = []
ripcount = {}
queue = list(signals)
done_pads = {}        # net -> set(index de pad connectés)
FIX = os.environ.get('FIXUP')
if FIX:
    prev = json.load(open(SCR + '/' + FIX))
    EOFW = {0.15: 0, 0.3: 1, 0.5: 2}
    for t in prev['tracks']:
        if t['net'] in PLANES:
            continue
        n = netid[t['net']]
        x1, y1, x2, y2 = (int(round(t[k] / G)) for k in ('x1', 'y1', 'x2', 'y2'))
        sx = (x2 > x1) - (x2 < x1); sy = (y2 > y1) - (y2 < y1)
        l = 'FB'.index(t['layer'])
        st = []
        x, y = x1, y1
        while True:
            st.append(l * N + y * NX + x)
            if x == x2 and y == y2:
                break
            x += sx; y += sy
        commit(n, st, EOFW[round(t['w'], 2)])
    for v in prev['vias']:
        if v['net'] in PLANES:
            continue
        c = int(round(v['y'] / G)) * NX + int(round(v['x'] / G))
        commit(netid[v['net']], [c, N + c], 0)
    queue = []
    for name in signals:
        n = netid[name]
        ps = netpads[name]
        if n not in tree:
            queue.append(name); continue
        dp = set()
        for i, p in enumerate(ps):
            stt = pad_states(p)
            if any(s_ in tree[n] for s_ in stt):
                dp.add(i); tree[n].update(stt)
        done_pads[n] = dp
        if len(dp) < len(ps):
            queue.append(name)
            print('à reprendre:', name, [(ps[i]['ref'], ps[i]['num']) for i in range(len(ps)) if i not in dp])
total_rip = 0
while queue:
    name = queue.pop(0)
    tn = time.time()
    n = netid[name]
    ps = netpads[name]
    e = {0.5: 2, 0.3: 1}.get(WIDE.get(name), 0)
    if n not in tree:
        tree[n] = set(pad_states(ps[0]))
        done_pads[n] = {0}
    ok = True
    while len(done_pads[n]) < len(ps):
        # pad restant le plus proche de l'arbre (approx : du barycentre des pads connectés)
        rest = [i for i in range(len(ps)) if i not in done_pads[n]]
        cxs = [ps[i]['cx'] for i in done_pads[n]]; cys = [ps[i]['cy'] for i in done_pads[n]]
        def dmin(i):
            return min(abs(ps[i]['cx'] - a) + abs(ps[i]['cy'] - b) for a, b in zip(cxs, cys))
        i = min(rest, key=dmin)
        goal = set(pad_states(ps[i]))
        if goal & tree[n]:
            done_pads[n].add(i); tree[n].update(goal); continue
        # on part du pad isolé vers l'arbre : heuristique = boîte de l'arbre (admissible)
        path, cost = search(n, list(goal), tree[n], e=e)
        ee = e
        if path is None and e:
            ee = max(0, e - 1)
            path, cost = search(n, list(goal), tree[n], e=ee)
        if path is None:
            # arrachage
            if ripcount.get(name, 0) < 14 and total_rip < 1200:
                path, cost = search(n, list(goal), tree[n], e=ee, soft=True, maxexp=12000000)
                if FIX and path is None: print('   soft: aucun chemin pour', name, ps[i]['ref'], ps[i]['num'])
                if path is not None:
                    cf = conflicts(path, n, ee)
                    for k_, s_ in enumerate(path):
                        l_, c_ = divmod(s_, N)
                        v_ = trk[l_][c_]
                        if v_ > 0 and v_ != n:
                            cf.add(v_)
                        if k_ and path[k_ - 1] // N != l_:
                            for m_ in (via[0], via[1], trk[1 - l_]):
                                if m_[c_] > 0 and m_[c_] != n:
                                    cf.add(m_[c_])
                        if k_ and path[k_ - 1] // N == l_:
                            cp_ = path[k_ - 1] % N
                            d_ = c_ - cp_
                            if abs(d_) in (NX + 1, NX - 1):
                                dxx = 1 if d_ in (NX + 1, -NX + 1) else -1
                                for cc_ in (cp_ + dxx, c_ - dxx):
                                    w_ = trk[l_][cc_]
                                    if w_ > 0 and w_ != n:
                                        cf.add(w_)
                        if v_ == -1 and FIX:
                            print('     cellule -1 sur le chemin', 'FB'[l_], c_ % NX / 10, c_ // NX / 10)
                    cf = {c for c in cf if idnet[c] not in PLANES}
                    bad = [c for c in conflicts(path, n, ee) if idnet[c] in PLANES]
                    if FIX: print('   soft: cout %.0f, conflits %s, plans %s' % (cost, sorted(idnet[c] for c in cf), bad))
                    if cf and not bad:
                        for s_ in path:
                            l_, c_ = divmod(s_, N)
                            v_ = trk[l_][c_]
                            if v_ != 0 and v_ != n:
                                for dx_, dy_ in disc(0.25):
                                    hist[l_][c_ + dx_ + dy_ * NX] += 6.0
                        for c in cf:
                            rip(c)
                            done_pads.pop(c, None)
                            if idnet[c] not in queue:
                                queue.append(idnet[c])
                        ripcount[name] = ripcount.get(name, 0) + 1
                        total_rip += len(cf)
                        # revérifier que le chemin est maintenant libre
                        path2, cost2 = search(n, list(goal), tree[n], e=ee)
                        if path2 is not None:
                            path = path2
                        else:
                            path = None
                    else:
                        path = None
            if path is None:
                ok = False
                failed.append((name, ps[i]['ref'], ps[i]['num']))
                done_pads[n].add(i)      # on abandonne ce pad
                continue
        commit(n, path, ee)
        tree[n].update(goal)
        done_pads[n].add(i)
    print('%s %-28s %5.1fs q=%d' % ('ok ' if ok else 'ECHEC', name, time.time() - tn, len(queue))); sys.stdout.flush()
print('\nsignaux: %d nets, échecs: %s, arrachages: %d (%.0fs)' % (len(signals), failed, total_rip, time.time() - t0))

# ---------------------------------------------------------------- sortie
tracks = []; vias_out = []
for n, rl in routes.items():
    for r in rl:
        w = {0: SIGW, 1: 0.3, 2: 0.5}[r['e']]
        path = r['path']
        k = 0
        while k < len(path) - 1:
            l, c = divmod(path[k], N)
            l2, c2 = divmod(path[k + 1], N)
            if l2 != l:
                vias_out.append(dict(net=idnet[n], x=round((c % NX) * G, 3), y=round((c // NX) * G, 3)))
                k += 1
                continue
            d = c2 - c
            j = k + 1
            while j + 1 < len(path) and path[j + 1] // N == l and (path[j + 1] % N) - (path[j] % N) == d:
                j += 1
            ce = path[j] % N
            tracks.append(dict(net=idnet[n], layer='FB'[l], w=w,
                               x1=round((c % NX) * G, 3), y1=round((c // NX) * G, 3),
                               x2=round((ce % NX) * G, 3), y2=round((ce // NX) * G, 3)))
            k = j
json.dump(dict(tracks=tracks, vias=vias_out, failed=failed, plane_fail=plane_fail), open(SCR + '/routes.json', 'w'))
print(len(tracks), 'segments,', len(vias_out), 'vias, longueur %.0f mm' % sum(math.hypot(t['x2'] - t['x1'], t['y2'] - t['y1']) for t in tracks))

with open(SCR + '/diag.txt', 'w') as fo:
    for name, ref, num in failed:
        n = netid[name]
        for p in netpads[name]:
            fo.write('%s %s.%s (%.2f,%.2f) %s\n' % (name, p['ref'], p['num'], p['cx'], p['cy'], p['layers']))
            cx, cy = int(round(p['cx'] / G)), int(round(p['cy'] / G))
            for l in (0, 1):
                fo.write(' couche %s\n' % 'FB'[l])
                for y in range(cy - 14, cy + 15):
                    row = ''
                    for x in range(cx - 22, cx + 23):
                        if not (0 <= x < NX and 0 <= y < NY):
                            row += ' '; continue
                        i = y * NX + x
                        v = trk[l][i]
                        ch = '.' if v == 0 else ('o' if v == n else ('#' if v == -1 else 'x'))
                        if ch == '.' and not via_ok(i, n):
                            ch = ','
                        row += ch
                    fo.write('  ' + row + '\n')
