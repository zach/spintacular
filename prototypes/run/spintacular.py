"""PROTOTYPE: a text-only Spintacular run, for tuning numbers.

Throwaway code. Every balance number lives in tables.json.

    python3 prototypes/run/spintacular.py sim [runs]   # bots play; prints the rubric report
    python3 prototypes/run/spintacular.py play [seed]  # play a run yourself
    python3 prototypes/run/spintacular.py compare [runs]  # compare scoring layers at the same wall

Rules follow docs/design/. Details the design leaves open are listed in
prototypes/run/README.md under "Prototype defaults".
"""
import copy
import json
import os
import random
import sys
import zlib
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
COLOR_NAMES = ["R", "G", "B", "Y"]
FAMILIES = ["color", "value", "straight", "color_straight"]
ADDITIVE = {"flat_points", "flat_mult", "mult_per_color", "mult_per_value", "points_per_high",
            "mult_per_parity", "mult_per_pattern", "scaling_mult"}


def load_tables(path=None):
    with open(path or os.path.join(HERE, "tables.json")) as f:
        return json.load(f)


# ---------------------------------------------------------------- model

class Wedge:
    _next = 0

    def __init__(self, color, value):
        Wedge._next += 1
        self.uid = Wedge._next
        self.color = color          # 0..3, or None when wild
        self.value = value
        self.ranged = False         # value rolls value-1..value+1
        self.dice = 1

    def label(self):
        c = "*" if self.color is None else COLOR_NAMES[self.color]
        v = f"{self.value}~" if self.ranged else str(self.value)
        return f"{c}{v}" + ("x2" if self.dice == 2 else "")

    def roll_value(self, rng):
        if not self.ranged:
            return self.value
        return min(6, max(1, self.value + rng.choice((-1, 0, 1))))


class Die:
    __slots__ = ("color", "value", "weight", "points", "wedge")

    def __init__(self, color, value, wedge):
        self.color, self.value, self.wedge = color, value, wedge
        self.weight, self.points = 1, 0

    def label(self):
        return ("*" if self.color is None else COLOR_NAMES[self.color]) + str(self.value)


class Run:
    def __init__(self, t, rng):
        self.t, self.rng = t, rng
        self.money = t["run"]["start_money"]
        self.bag = [Wedge(c, v) for c in range(4) for v in t["start"]["bag_values"]]
        self.wheels = [dict(id=w, **t["wheels"][w]) for w in t["start"]["wheels"]]
        self.items = []        # dicts copied from tables, plus "id" and "state"
        self.consumables = []
        self.bought = set()
        self.round = 0

    def has(self, effect):
        return [i for i in self.items if i["effect"] == effect]

    def slots(self):
        return sum(w["size"] for w in self.wheels)


# ---------------------------------------------------------------- scoring

def runs_of(seq, ok_pair_start, extend):
    """Maximal windows of adjacent dice. extend(state, die) -> new state or None."""
    out, n, best_end = [], len(seq), -1
    for i in range(n):
        state, j = ok_pair_start(seq[i]), i
        while j + 1 < n:
            nxt = extend(state, seq[j + 1])
            if nxt is None:
                break
            state, j = nxt, j + 1
        if j > best_end:
            out.append((i, j))
            best_end = j
    return out


def find_patterns(seq):
    """Returns [(family, size)] for every maximal run of 3+ adjacent dice. A blank (None) breaks runs."""
    if any(d is None for d in seq):
        pats, part = [], []
        for d in seq + [None]:
            if d is None:
                pats += find_patterns(part) if part else []
                part = []
            else:
                part.append(d)
        return pats
    pats = []

    def colors(d):
        return {0, 1, 2, 3} if d.color is None else {d.color}

    def color_ext(state, d):
        s = state & colors(d)
        return s or None

    for i, j in runs_of(seq, colors, color_ext):
        length = sum(d.weight for d in seq[i:j + 1])
        if length >= 3:
            pats.append(("color", min(length, 5)))

    for i, j in runs_of(seq, lambda d: d.value, lambda s, d: s if d.value == s else None):
        if j - i + 1 >= 3:
            pats.append(("value", min(j - i + 1, 5)))

    for i, j in runs_of(seq, lambda d: d.value, lambda s, d: d.value if d.value == s + 1 else None):
        if j - i + 1 >= 3:
            pats.append(("straight", min(j - i + 1, 5)))

    def cs_start(d):
        return (colors(d), d.value)

    def cs_ext(s, d):
        cs = s[0] & colors(d)
        return (cs, d.value) if cs and d.value == s[1] + 1 else None

    for i, j in runs_of(seq, cs_start, cs_ext):
        if j - i + 1 >= 3:
            pats.append(("color_straight", min(j - i + 1, 5)))
    return pats


def longest(pats, family):
    return max([s for f, s in pats if f == family], default=0)


def score_sequence(run, seq, commit, trace=None):
    """Scores one spin's sequence: dice, then the scoring layer's patterns, then items in order."""
    t = run.t
    full = seq
    seq = [d for d in full if d is not None]
    points = sum(d.value + d.points for d in seq)
    mult = 1.0
    if trace is not None:
        trace.append(f"dice {' '.join(d.label() if d else '__' for d in full)}: {points} points")
    layer = getattr(run, "layer", None)
    if layer is None:
        pats, penalty = find_patterns(full), 0
    else:
        pats, penalty = layer.patterns(full, commit, trace)
    if penalty:
        points = max(0, points - penalty)
        if trace is not None:
            trace.append(f"floor: -{penalty} points")
    for fam, size in pats:
        if fam == "wall_passive":
            m = size * t["scoring"].get("wall_passive_mult_per_marker", 0)
            if m:
                mult += m
                if trace is not None:
                    trace.append(f"wall holds {size} markers: +{m} mult")
            continue
        if fam == "wall":
            m = size * t["scoring"]["wall_mult_per_point"]
            mult += m
            if trace is not None:
                trace.append(f"wall placement scores {size}: +{m} mult")
            continue
        p, m = t["patterns"][fam][str(size)]
        points += p
        mult += m
        if trace is not None:
            trace.append(f"{fam} run of {size}: +{p} points, +{m} mult")
    for it in run.items:
        e, before = it["effect"], (points, mult)
        if e == "flat_points":
            points += it["amount"]
        elif e == "flat_mult":
            mult += it["amount"]
        elif e == "mult_per_color":
            mult += it["amount"] * sum(1 for d in seq if d.color in (None, it["color"]))
        elif e == "mult_per_value":
            mult += it["amount"] * sum(1 for d in seq if d.value in it["values"])
        elif e == "points_per_high":
            points += it["amount"] * sum(1 for d in seq if d.value >= it["threshold"])
        elif e == "mult_per_parity":
            mult += it["amount"] * sum(1 for d in seq if d.value % 2 == it["parity"])
        elif e == "mult_per_pattern":
            mult += it["amount"] * sum(1 for f, _ in pats if f == it["family"])
        elif e == "xmult_if_pattern":
            if longest(pats, it["family"]) >= it["min"]:
                mult *= it["amount"]
        elif e == "xmult_longest":
            n = longest(pats, it["family"])
            if n >= 3:
                mult *= 1 + it["amount"] * (n - 2)
        elif e == "scaling_xmult":
            if commit and longest(pats, it["family"]) >= it["min"]:
                it["state"] = it.get("state", 1.0) + it["amount"]
            mult *= it.get("state", 1.0)
        elif e == "scaling_mult":
            if commit:
                it["state"] = it.get("state", 0) + it["amount"] * sum(1 for f, _ in pats if f == it["family"])
            mult += it.get("state", 0)
        if trace is not None and (points, mult) != before:
            trace.append(f"{it['name']}: {before[0]:.0f}x{before[1]:.2f} -> {points:.0f}x{mult:.2f}")
    total = int(points * mult)
    if trace is not None:
        trace.append(f"= {points:.0f} x {mult:.2f} = {total}")
    return total, pats


# ---------------------------------------------------------------- scoring layers
#
# The dice layer is swappable (docs/design/scoring.md). tables.json "scoring.layer" picks one:
#   adjacent: runs of adjacent dice in the order the wheels add them (the current design)
#   path:     the player traces one run through a grid of recent spins (reading order)
#   holders:  the player places each die into fixed-size lines (Azul-style)

def path_families(dice):
    """Families the whole path forms, read in path order (either direction), with sizes."""
    out = []
    cs = {0, 1, 2, 3}
    for d in dice:
        cs &= {0, 1, 2, 3} if d.color is None else {d.color}
    if cs:
        n = sum(d.weight for d in dice)
        if n >= 3:
            out.append(("color", min(n, 5)))
    vals = [d.value for d in dice]
    n = min(len(dice), 5)
    if len(dice) >= 3:
        if len(set(vals)) == 1:
            out.append(("value", n))
        steps = {b - a for a, b in zip(vals, vals[1:])}
        if steps in ({1}, {-1}):
            out.append(("straight", n))
            if cs:
                out.append(("color_straight", n))
    return out


def path_can_extend(dice):
    """Whether a partial path can still become a run of some family."""
    cs = {0, 1, 2, 3}
    for d in dice:
        cs &= {0, 1, 2, 3} if d.color is None else {d.color}
    if cs:
        return True
    vals = [d.value for d in dice]
    if len(set(vals)) == 1:
        return True
    steps = {b - a for a, b in zip(vals, vals[1:])}
    return steps in ({1}, {-1})


class AdjacentLayer:
    def __init__(self, run, player):
        pass

    def patterns(self, seq, commit, trace):
        return find_patterns(seq), 0


class PathLayer:
    """Results stay on a grid for a few spins: rows are spins, columns are the order dice were added.
    Each spin the player traces one path of 3-5 orthogonally adjacent dice that touches the newest row
    and forms a run; its dice are then used up."""

    def __init__(self, run, player):
        self.run = run
        self.window = run.t["scoring"]["path_rows"]
        self.rows = []

    def patterns(self, seq, commit, trace):
        grid = [r[:] for r in self.rows] + [list(seq)]
        last = len(grid) - 1
        cells = [(r, c) for r in range(len(grid)) for c in range(len(grid[r])) if grid[r][c] is not None]
        best = (None, [], ())
        seen = {}

        def nbrs(r, c):
            for dr, dc in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                rr, cc = r + dr, c + dc
                if 0 <= rr < len(grid) and 0 <= cc < len(grid[rr]) and grid[rr][cc] is not None:
                    yield rr, cc

        def evaluate(path):
            nonlocal best
            pats = path_families([grid[r][c] for r, c in path])
            if not pats:
                return
            key = tuple(sorted(pats))
            if key not in seen:
                seen[key] = score_items_only(self.run, seq, pats)
            if best[0] is None or seen[key] > best[0]:
                best = (seen[key], pats, tuple(path))

        def dfs(path):
            if len(path) >= 3 and any(r == last for r, _ in path):
                evaluate(path)
            if len(path) == 5:
                return
            r, c = path[-1]
            for nb in nbrs(r, c):
                if nb not in path:
                    path.append(nb)
                    if path_can_extend([grid[x][y] for x, y in path]):
                        dfs(path)
                    path.pop()

        for cell in cells:
            dfs([cell])
        _, pats, path = best
        if trace is not None and path:
            trace.append("path " + "-".join(grid[r][c].label() for r, c in path))
        if commit:
            for r, c in path:
                grid[r][c] = None
            self.rows = grid[-(self.window - 1):] if self.window > 1 else []
        return pats, 0


class HoldersLayer:
    """Azul-style lines of fixed size. Each die goes into a line that accepts it, or the floor.
    A line becomes a color, value or straight line when started; a full line scores its pattern
    and empties. Lines carry over between spins within a round."""

    def __init__(self, run, player):
        self.run, self.player = run, player
        self.caps = run.t["scoring"]["holder_sizes"]
        self.floor = run.t["scoring"]["floor_penalties"]
        self.lines = [dict(fam=None, key=None, n=0, last=None, dir=0) for _ in self.caps]

    @staticmethod
    def accepts(line, d):
        if line["fam"] is None:
            return True
        if line["fam"] == "color":
            return d.color is None or line["key"] is None or d.color == line["key"]
        if line["fam"] == "value":
            return d.value == line["key"]
        if line["dir"] == 0:
            return abs(d.value - line["last"]) == 1
        return d.value == line["last"] + line["dir"]

    def place(self, lines, d, fam_pref):
        """Greedy placement: finish lines first, then extend the fullest, then start the smallest."""
        best, best_key = None, None
        for i, line in enumerate(lines):
            if line["fam"] is None or not self.accepts(line, d):
                continue
            w = d.weight if line["fam"] == "color" else 1
            key = (line["n"] + w >= self.caps[i], line["n"] / self.caps[i])
            if best_key is None or key > best_key:
                best, best_key = i, key
        if best is not None:
            return best, None
        empty = [i for i, l in enumerate(lines) if l["fam"] is None]
        if empty:
            return min(empty, key=lambda i: self.caps[i]), fam_pref or "color"
        return None, None

    def patterns(self, seq, commit, trace):
        lines = copy.deepcopy(self.lines)
        wall = set(getattr(self.run, "wall", set()))
        fam_pref = self.player.holder_family(self.run) if hasattr(self.player, "holder_family") else "color"
        pats, floor = [], 0
        for d in seq:
            if d is None:
                continue
            i, start = self.place(lines, d, fam_pref)
            if i is None:
                floor += 1
                continue
            line = lines[i]
            if start:
                line.update(fam=start, n=0, last=d.value, dir=0,
                            key=d.color if start == "color" else d.value)
            elif line["fam"] == "color" and line["key"] is None:
                line["key"] = d.color
            elif line["fam"] == "straight":
                line["dir"] = line["dir"] or (d.value - line["last"])
                line["last"] = d.value
            w = d.weight if line["fam"] == "color" else 1
            line["n"] += w
            if line["n"] >= self.caps[i]:
                floor += line["n"] - self.caps[i]
                pats.append((line["fam"], self.caps[i]))
                if trace is not None:
                    trace.append(f"{line['fam']} line of {self.caps[i]} filled")
                self.completed(line, d, wall, pats, trace)
                line.update(fam=None, key=None, n=0, last=None, dir=0)
        penalty = sum(self.floor[:floor]) + max(0, floor - len(self.floor)) * (self.floor[-1] if self.floor else 0)
        if self.run.t["scoring"]["layer"] == "wall" and wall:
            pats.append(("wall_passive", len(wall)))
        if commit:
            self.lines = lines
            self.run.wall = wall
        return pats, penalty

    def completed(self, line, d, wall, pats, trace):
        pass


def wall_score(wall, cell):
    """Azul scoring for a marker placed at cell (color, value) on a wall that already holds `wall`."""
    c, v = cell
    row = 1
    x = v - 1
    while (c, x) in wall:
        row, x = row + 1, x - 1
    x = v + 1
    while (c, x) in wall:
        row, x = row + 1, x + 1
    col = 1
    for step in (-1, 1):
        y = c + step
        while 0 <= y < 4 and (y, v) in wall:
            col, y = col + 1, y + step
    if row == 1 and col == 1:
        return 1
    return (row if row > 1 else 0) + (col if col > 1 else 0)


class WallLayer(HoldersLayer):
    """Holders plus an Azul-style wall that lasts the whole run. The wall is a 4 x 6 grid:
    a row per color, a column per value. When a line fills, the die that filled it places a
    marker on its (color, value) cell, if that cell is free. The marker scores like an Azul
    tile: 1 alone, or the length of its row chain plus the length of its column chain.
    Rows reward consecutive values in one color; columns reward one value in many colors."""

    def completed(self, line, d, wall, pats, trace):
        colors = [d.color] if d.color is not None else [0, 1, 2, 3]
        free = [(c, d.value) for c in colors if (c, d.value) not in wall]
        if not free:
            return
        cell = max(free, key=lambda cl: wall_score(wall, cl))
        k = wall_score(wall, cell)
        wall.add(cell)
        pats.append(("wall", k))
        if trace is not None:
            trace.append(f"marker on wall {COLOR_NAMES[cell[0]]}{cell[1]}")


LAYERS = {"adjacent": AdjacentLayer, "path": PathLayer, "holders": HoldersLayer, "wall": WallLayer}


def make_layer(run, player):
    return LAYERS[run.t.get("scoring", {}).get("layer", "adjacent")](run, player)


def score_items_only(run, seq, pats):
    """Score with fixed patterns and no state changes (for choosing among options)."""
    saved = run.layer
    run.layer = _Fixed(pats)
    try:
        return score_sequence(run, seq, commit=False)[0]
    finally:
        run.layer = saved


class _Fixed:
    def __init__(self, pats):
        self.pats = pats

    def patterns(self, seq, commit, trace):
        return self.pats, 0


# ---------------------------------------------------------------- spinning

def fill_wheels(run):
    bag = run.bag[:]
    run.rng.shuffle(bag)
    # Deal round-robin. Slots the bag can't fill hold blank wedges (None), which add no die
    # and break a run, so every wheel always holds its full number of wedges.
    loads = [[] for _ in run.wheels]
    k = 0
    for _ in range(max((w["size"] for w in run.wheels), default=0)):
        for wi, w in enumerate(run.wheels):
            if len(loads[wi]) < w["size"] and k < len(bag):
                loads[wi].append(bag[k])
                k += 1
    if run.t["run"].get("blank_wedges", True):
        for wi, w in enumerate(run.wheels):
            loads[wi] += [None] * (w["size"] - len(loads[wi]))
    return loads


def land(run, wedges, band_color):
    weights = [3 if band_color is not None and w is not None and w.color in (None, band_color) else 1
               for w in wedges]
    return run.rng.choices(range(len(wedges)), weights)[0]


def spin(run, loads, band_color=None):
    """Returns landings: per wheel, a list of indexes into its wedges."""
    out = []
    for w, wedges in zip(run.wheels, loads):
        if not wedges:
            out.append([])
            continue
        n = 2 if w["effect"] == "twin" else 1
        out.append([land(run, wedges, band_color) for _ in range(n)])
    return out


def build_sequence(run, loads, landings, values=None):
    """values: optional per-(wheel, landing, die) rolled values, so a nudge can re-evaluate."""
    seq, rolled = [], {}
    for wi, (w, wedges, lands) in enumerate(zip(run.wheels, loads, landings)):
        for li, idx in enumerate(lands):
            wedge = wedges[idx]
            if wedge is None:
                seq.append(None)
                continue
            for di in range(wedge.dice):
                key = (wi, li, di, wedge.uid)
                v = values.get(key) if values else None
                if v is None:
                    v = wedge.roll_value(run.rng)
                rolled[key] = v
                d = Die(wedge.color, v, wedge)
                if w["effect"] == "bump":
                    d.value = min(6, d.value + 1)
                elif w["effect"] == "heavy":
                    d.weight = 2
                elif w["effect"] == "gilded":
                    d.points = w["amount"]
                seq.append(d)
    return seq, rolled


# ---------------------------------------------------------------- round

def play_round(run, player, log=None, target=None):
    t = run.t
    target = t["targets"][run.round] if target is None else target
    spins = t["run"]["spins_per_round"]
    loads = fill_wheels(run)
    run.layer = make_layer(run, player)
    rods = sum(i["amount"] for i in run.has("rod"))
    total, used = 0, 0
    best_spin = 0
    while used < spins and total < target:
        used += 1
        band = player.use_band(run, total, target, used, spins)
        landings = spin(run, loads, band)
        seq, rolled = build_sequence(run, loads, landings)
        value, _ = score_sequence(run, seq, commit=False)
        last = used == spins
        # Nudge: move one wheel's pointer one wedge either way.
        if last and total + value < target and "nudge" in run.consumables:
            best = None
            for wi, lands in enumerate(landings):
                for li in range(len(lands)):
                    for step in (-1, 1):
                        trial = [l[:] for l in landings]
                        trial[wi][li] = (trial[wi][li] + step) % len(loads[wi])
                        s2, _ = build_sequence(run, loads, trial, rolled)
                        v2, _ = score_sequence(run, s2, commit=False)
                        if best is None or v2 > best[0]:
                            best = (v2, trial)
            if best and best[0] > value:
                run.consumables.remove("nudge")
                landings = best[1]
                seq, rolled = build_sequence(run, loads, landings, rolled)
                value, _ = score_sequence(run, seq, commit=False)
        if last and total + value < target and "respin" in run.consumables:
            run.consumables.remove("respin")
            landings = spin(run, loads)
            seq, rolled = build_sequence(run, loads, landings)
        trace = [] if log else None
        value, pats = score_sequence(run, seq, commit=True, trace=trace)
        total += value
        best_spin = max(best_spin, value)
        if log:
            log(f"  spin {used}: " + " | ".join(trace) + f"   (round {total}/{target})")
        # The Rod: take a landed wedge out of the bag for good.
        if rods > 0:
            landed = [loads[wi][i] for wi, lands in enumerate(landings) for i in lands if loads[wi][i] is not None]
            landed = list({w.uid: w for w in landed if w in run.bag}.values())
            victim = player.zap_choice(run, landed)
            if victim is not None:
                rods -= 1
                run.bag.remove(victim)
                for wedges in loads:
                    if victim in wedges:
                        wedges[wedges.index(victim)] = None
                if log:
                    log(f"  The Rod removes {victim.label()} from the bag")
    return total >= target, spins - used, best_spin, total


def round_income(run, unused):
    inc = run.t["income"]
    interest = min(inc["interest_cap"], run.money // inc["interest_per"])
    gain = inc["per_round"] + unused * inc["per_unused_spin"] + interest
    for it in run.has("piggy"):
        gain += min(it["cap"], int(run.money * it["rate"]))
    for it in run.has("spendthrift"):
        gain += int(run.money * it["rate"])
    run.money += gain
    return gain


# ---------------------------------------------------------------- shop

def rarity_weights(t, rnd):
    w = t["shop"]["rarity_by_round"][0]["weights"]
    for row in t["shop"]["rarity_by_round"]:
        if rnd + 1 >= row["from_round"]:
            w = row["weights"]
    return w


def pick_rarity(t, rng, rnd, pool):
    w = rarity_weights(t, rnd)
    present = {v["rarity"] for v in pool.values()}
    names = [r for r in w if w[r] > 0 and r in present]
    return rng.choices(names, [w[r] for r in names])[0]


def roll_offer(run):
    t, rng = run.t, run.rng
    kw = t["shop"]["kind_weights"]
    kind = rng.choices(list(kw), list(kw.values()))[0]
    prices = t["shop"]["prices"]
    if kind in ("item", "wheel", "upgrade", "consumable"):
        pool = t[{"item": "items", "wheel": "wheels", "upgrade": "upgrades", "consumable": "consumables"}[kind]]
        rar = pick_rarity(t, rng, run.round, pool)
        cid = rng.choice([k for k, v in pool.items() if v["rarity"] == rar])
        return dict(kind=kind, id=cid, name=pool[cid]["name"], price=prices[kind][rar], rarity=rar)
    if kind == "wedge":
        c, v = rng.randrange(4), rng.randint(1, 6)
        return dict(kind="wedge", id="wedge", name=f"Wedge {COLOR_NAMES[c]}{v}", price=prices["wedge"],
                    color=c, value=v, rarity="common")
    return dict(kind="zap", id="zap", name="Zap (remove a wedge)", price=prices["zap"], rarity="common")


def can_take(run, offer):
    t = run.t["run"]
    if offer["kind"] == "item":
        return len(run.items) < t["max_items"]
    if offer["kind"] == "wheel":
        return len(run.wheels) < t["max_wheels"]
    if offer["kind"] == "consumable":
        return len(run.consumables) < t["max_consumables"]
    return True


def apply_offer(run, offer, target_wedge=None, choice=None):
    """choice: the color (paint) or value (recut) the player picks."""
    k = offer["kind"]
    if k == "item":
        it = copy.deepcopy(run.t["items"][offer["id"]])
        it["id"] = offer["id"]
        it["price"] = offer["price"]
        run.items.append(it)
    elif k == "wheel":
        run.wheels.append(dict(id=offer["id"], price=offer["price"], **run.t["wheels"][offer["id"]]))
    elif k == "consumable":
        run.consumables.append(offer["id"])
    elif k == "wedge":
        run.bag.append(Wedge(offer["color"], offer["value"]))
    elif k == "zap":
        if target_wedge is not None and len(run.bag) > 1:
            run.bag.remove(target_wedge)
    elif k == "upgrade":
        w = target_wedge
        if offer["id"] == "range":
            w.ranged = True
        elif offer["id"] == "wild":
            w.color = None
        elif offer["id"] == "second_die":
            w.dice = 2
        elif offer["id"] == "paint":
            w.color = choice
        elif offer["id"] == "recut":
            w.value = choice
    run.money -= offer["price"]
    run.bought.add(offer["id"])


def sell_value(run, thing):
    return int(thing.get("price", 0) * run.t["shop"]["sell_fraction"])


# ---------------------------------------------------------------- players

class Player:
    """Base: buys nothing, uses nothing."""
    name = "passive"
    engine = False

    def __init__(self, rng):
        self.rng = rng

    def pre_run(self, run):
        pass

    def shop(self, run, offers, stats):
        pass

    def use_band(self, run, total, target, used, spins):
        return None

    def zap_choice(self, run, landed):
        return None


class RandomPlayer(Player):
    name = "random"

    def shop(self, run, offers, stats):
        for o in offers:
            if self.rng.random() < 0.5 and o["price"] <= run.money and can_take(run, o):
                apply_offer(run, o, self.rng.choice(run.bag),
                            self.rng.randrange(4) if o["id"] == "paint" else self.rng.randint(1, 6))
                stats.picked[o["id"]] += 1
        self.rng.shuffle(run.items)

    def zap_choice(self, run, landed):
        return self.rng.choice(landed) if landed and len(run.bag) > run.slots() else None


class Builder(Player):
    """Scores offers by fit to a focus, and spends by an economy style.

    focus: ("color", c) | ("value", v) | ("straight", None) | ("flat", None) | ("adaptive", None)
    style: spender | saver | reroller | bargain
    """

    def __init__(self, rng, focus, style):
        super().__init__(rng)
        self.focus, self.style = focus, style
        self.engine = focus[0] != "flat"
        self.name = f"{focus[0]}/{style}"
        self.adaptive = focus[0] == "adaptive"

    # --- what the build wants
    def current_focus(self, run):
        if not self.adaptive:
            return self.focus
        score = Counter()
        for it in run.items:
            for f in self.candidate_foci():
                score[f] += self.item_fit(it, f)
        if not score or max(score.values()) < 2:
            return ("adaptive", None)
        return score.most_common(1)[0][0]

    def candidate_foci(self):
        return [("color", c) for c in range(4)] + [("value", 5), ("value", 6), ("straight", None)]

    def wedge_fits(self, w, focus):
        kind, x = focus
        if kind == "color":
            return w.color in (None, x)
        if kind == "value":
            return w.value == x or (w.ranged and abs(w.value - x) == 1)
        if kind == "straight":
            return 2 <= w.value <= 5
        return w.value >= 4

    def item_fit(self, it, focus):
        kind, x = focus
        e, fam = it["effect"], it.get("family")
        if e in ("flat_points", "flat_mult"):
            return 1.5 if kind == "flat" else 1.0
        if e == "rod":
            return 0 if kind == "flat" else 3.5
        if e == "piggy":
            return 2.5 if self.style in ("saver", "bargain") else 0.5
        if e == "spendthrift":
            return 2.5 if self.style in ("spender", "reroller") else 0.3
        if kind in ("flat", "adaptive"):
            return 1.2 if e in ("points_per_high", "mult_per_parity", "mult_per_color", "mult_per_value") else (2.0 if kind == "adaptive" else 0.6)
        want = {"color": "color", "value": "value", "straight": "straight"}[kind]
        if e == "mult_per_color":
            return 3.0 if kind == "color" and it["color"] == x else 0.3
        if e == "points_per_high":
            return 2.0 if kind == "value" and x >= 5 else 0.6
        if e == "mult_per_value":
            return 3.0 if kind == "value" and x in it["values"] else 0.3
        if e == "mult_per_parity":
            return 1.5 if kind == "value" and x % 2 == it["parity"] else 0.4
        if fam == want:
            return {"mult_per_pattern": 3.0, "xmult_if_pattern": 4.0, "xmult_longest": 5.0,
                    "scaling_xmult": 5.0, "scaling_mult": 5.0}.get(e, 1.0)
        if fam == "color_straight":
            return 3.0 if kind == "straight" else 1.0
        return 0.3

    def fit(self, run, o, focus):
        kind, x = focus
        k = o["kind"]
        if not can_take(run, o):
            return 0
        if k == "item":
            return self.item_fit(run.t["items"][o["id"]], focus)
        if k == "wheel":
            eff = run.t["wheels"][o["id"]]["effect"]
            base = {"none": 2.5, "bump": 2.0 if kind in ("straight", "value") else 1.5,
                    "gilded": 2.0, "heavy": 4.0 if kind == "color" else 1.5, "twin": 4.0}[eff]
            return base
        if k == "upgrade":
            if o["id"] == "second_die":
                return 3.5
            if o["id"] == "wild":
                return 3.5 if kind in ("color", "adaptive") else 0.5
            if o["id"] == "paint":
                return 3.0 if kind == "color" else 0.3
            if o["id"] == "recut":
                return 3.0 if kind == "value" else (1.5 if kind == "straight" else 0.3)
            return 2.0 if kind in ("straight", "value") else 0.5
        if k == "wedge":
            fake = Wedge(o["color"], o["value"])
            return 2.5 if self.wedge_fits(fake, focus) and kind != "adaptive" else 0.2
        if k == "zap":
            off = [w for w in run.bag if not self.wedge_fits(w, focus)]
            return 2.5 if off and len(run.bag) > self.min_bag(run) and kind != "adaptive" else 0
        if k == "consumable":
            return 1.5
        return 0

    def upgrade_target(self, run, o, focus):
        fits = [w for w in run.bag if self.wedge_fits(w, focus)]
        off = [w for w in run.bag if not self.wedge_fits(w, focus)]
        if o["kind"] == "zap":
            return self.rng.choice(off) if off else None
        if o["id"] in ("wild", "paint", "recut"):
            pool = [w for w in off if w.color is not None] or [w for w in run.bag if w.color is not None]
        elif o["id"] == "second_die":
            pool = [w for w in fits if w.dice == 1] or [w for w in run.bag if w.dice == 1]
        else:
            pool = [w for w in run.bag if not w.ranged]
        return self.rng.choice(pool) if pool else None

    def holder_family(self, run):
        kind = self.current_focus(run)[0]
        return kind if kind in ("color", "value", "straight") else "color"

    def choice_for(self, o, focus):
        kind, x = focus
        if o["id"] == "paint":
            return x if kind == "color" else self.rng.randrange(4)
        if o["id"] == "recut":
            return x if kind == "value" else self.rng.randint(2, 5)
        return None

    def min_bag(self, run):
        # With blank wedges, thinning below the wheels' slots trades dice for blanks.
        return run.slots() if run.t["run"].get("blank_wedges", True) else len(run.wheels)

    # --- spending
    def shop(self, run, offers, stats):
        focus = self.current_focus(run)
        reroll_cost = run.t["shop"]["reroll_base"]
        rerolls = 0
        while True:
            scored = sorted(((self.fit(run, o, focus), o) for o in offers), key=lambda p: -p[0])
            good = sum(1 for f, _ in scored if f >= 2.0)
            stats.shops += 1
            stats.shops_with_fit += good >= 1
            stats.shops_with_choice += good >= 2
            for f, o in scored:
                if o.get("sold") or f < 1.0:
                    continue
                if not self.wants(run, o, f):
                    continue
                if o["price"] > run.money:
                    self.try_sell_to_afford(run, o, f)
                if o["price"] <= run.money and can_take(run, o):
                    tgt = self.upgrade_target(run, o, focus) if o["kind"] in ("upgrade", "zap") else None
                    if o["kind"] == "upgrade" and tgt is None:
                        continue
                    apply_offer(run, o, tgt, self.choice_for(o, focus))
                    o["sold"] = True
                    stats.picked[o["id"]] += 1
                elif o["kind"] == "item" and len(run.items) >= run.t["run"]["max_items"]:
                    self.replace_item(run, o, f, focus, stats)
            if self.style == "reroller" and rerolls < 3 and run.money >= reroll_cost + 4 \
                    and not any(f >= 3 and not o.get("sold") for f, o in scored):
                run.money -= reroll_cost
                reroll_cost += run.t["shop"]["reroll_step"]
                rerolls += 1
                offers = [roll_offer(run) for _ in range(run.t["shop"]["offers"])]
                for o in offers:
                    stats.offered[o["id"]] += 1
                continue
            break
        # Additive effects before multipliers.
        run.items.sort(key=lambda it: 0 if it["effect"] in ADDITIVE else 1)

    def wants(self, run, o, f):
        left = run.money - o["price"]
        if self.style == "saver":
            # Wait: buy essentials, and bank toward the interest cap as the run goes on.
            return f >= 3.0 or left >= min(25, 5 * run.round)
        if self.style == "bargain":
            return o["price"] <= 3 or f / o["price"] >= 0.5
        return True

    def try_sell_to_afford(self, run, o, f):
        # Big buy: sell the weakest item if that pays for a much better one.
        if self.style != "spender" or f < 4 or not run.items or o["kind"] != "item":
            return
        focus = self.current_focus(run)
        weakest = min(run.items, key=lambda it: self.item_fit(it, focus))
        if self.item_fit(weakest, focus) < f - 2 and run.money + sell_value(run, weakest) >= o["price"]:
            run.money += sell_value(run, weakest)
            run.items.remove(weakest)

    def replace_item(self, run, o, f, focus, stats):
        weakest = min(run.items, key=lambda it: self.item_fit(it, focus))
        if self.item_fit(weakest, focus) < f - 1.5 and run.money + sell_value(run, weakest) >= o["price"]:
            run.money += sell_value(run, weakest)
            run.items.remove(weakest)
            apply_offer(run, o)
            o["sold"] = True
            stats.picked[o["id"]] += 1

    # --- during a round
    def use_band(self, run, total, target, used, spins):
        focus = self.current_focus(run)
        if "band" in run.consumables and focus[0] == "color" and used == spins and total < target:
            run.consumables.remove("band")
            return focus[1]
        return None

    def zap_choice(self, run, landed):
        focus = self.current_focus(run)
        if len(run.bag) <= self.min_bag(run) or focus[0] in ("flat", "adaptive"):
            return None
        off = [w for w in landed if not self.wedge_fits(w, focus)]
        return off[0] if off else None


class RecklessPlayer(Builder):
    """An insane risk: sells three starting wheels before round 1 to bank cash."""

    def __init__(self, rng):
        super().__init__(rng, ("color", 0), "spender")
        self.name = "reckless"

    def pre_run(self, run):
        for _ in range(3):
            run.wheels.pop()
            run.money += int(run.t["shop"]["prices"]["wheel"]["common"] * run.t["shop"]["sell_fraction"])


# ---------------------------------------------------------------- simulation

class Stats:
    def __init__(self):
        self.offered, self.picked = Counter(), Counter()
        self.shops = self.shops_with_fit = self.shops_with_choice = 0


def play_run(t, seed, make_player, stats, log=None, totals=None):
    """totals: if a list, play every spin of every round without losing and record round totals."""
    rng = random.Random(seed)
    run = Run(t, rng)
    player = make_player(random.Random(seed + 1))
    player.pre_run(run)
    clutch = 0
    for r in range(t["run"]["rounds"]):
        run.round = r
        if log:
            log(f"Round {r + 1}: target {t['targets'][r]}, ${run.money}")
        won, unused, best, total = play_round(run, player, log, 10**15 if totals is not None else None)
        if totals is not None:
            totals.append(total)
            won, unused = True, 1
        if not won:
            return dict(won=False, lost_round=r + 1, bought=run.bought, clutch=clutch)
        clutch += unused == 0
        if r == t["run"]["rounds"] - 1:
            break
        round_income(run, unused)
        offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        for o in offers:
            stats.offered[o["id"]] += 1
        player.shop(run, offers, stats)
        if run.has("spendthrift"):
            run.money = 0
    return dict(won=True, lost_round=None, bought=run.bought, clutch=clutch)


def bot_roster():
    rost = {
        "passive": lambda r: Player(r),
        "random": lambda r: RandomPlayer(r),
        "flat/spender": lambda r: Builder(r, ("flat", None), "spender"),
        "reckless": lambda r: RecklessPlayer(r),
    }
    for style in ("spender", "saver", "reroller", "bargain"):
        rost[f"color/{style}"] = (lambda s: lambda r: Builder(r, ("color", r.randrange(4)), s))(style)
        rost[f"value/{style}"] = (lambda s: lambda r: Builder(r, ("value", r.choice((5, 6))), s))(style)
        rost[f"straight/{style}"] = (lambda s: lambda r: Builder(r, ("straight", None), s))(style)
        rost[f"adaptive/{style}"] = (lambda s: lambda r: Builder(r, ("adaptive", None), s))(style)
    return rost


NO_ENGINE = ("passive", "random", "flat/spender")


def simulate(t, runs):
    roster = bot_roster()
    results, stats = {}, Stats()
    win_presence, wins_total = Counter(), 0
    for name, make in roster.items():
        rs = []
        bot_stats = Stats() if name in NO_ENGINE or name == "reckless" else stats
        for i in range(runs):
            res = play_run(t, 10_000 * (zlib.crc32(name.encode()) % 1000) + i, make, bot_stats)
            rs.append(res)
            if res["won"] and name not in NO_ENGINE:
                wins_total += 1
                for cid in res["bought"]:
                    win_presence[cid] += 1
        results[name] = rs
    return results, stats, win_presence, wins_total


def rate(rs, pred):
    return sum(1 for r in rs if pred(r)) / len(rs)


def report(t, runs):
    results, stats, presence, wins_total = simulate(t, runs)
    rounds = t["run"]["rounds"]
    out = [f"{runs} runs per bot, {rounds} rounds, targets {t['targets']}", ""]
    out.append(f"{'bot':<20}{'win':>7}{'lose r1':>9}  losses by round")
    for name, rs in results.items():
        by = Counter(r["lost_round"] for r in rs if not r["won"])
        hist = " ".join(f"{by.get(i, 0):>3}" for i in range(1, rounds + 1))
        out.append(f"{name:<20}{rate(rs, lambda r: r['won']):>7.1%}{rate(rs, lambda r: r['lost_round'] == 1):>9.1%}  {hist}")
    out.append("")

    checks = []

    def check(cid, label, ok, detail):
        checks.append((cid, label, ok, detail))

    r1 = max(rate(results[n], lambda r: r["lost_round"] == 1) for n in NO_ENGINE)
    check("A1", "Round 1 is almost impossible to lose (every non-reckless bot <= 1%)",
          r1 <= 0.01 and all(rate(rs, lambda r: r["lost_round"] == 1) <= 0.01
                             for n, rs in results.items() if n != "reckless"),
          f"worst no-engine bot loses round 1 {r1:.1%}")
    rk = rate(results["reckless"], lambda r: r["lost_round"] == 1)
    check("A2", "An insane risk can lose round 1 (reckless >= 50%)", rk >= 0.5, f"reckless loses round 1 {rk:.1%}")
    ne = max(rate(results[n], lambda r: r["won"]) for n in NO_ENGINE)
    check("B1", "Winning without a point factory is almost impossible (no-engine bots <= 3%)",
          ne <= 0.03, f"best no-engine bot wins {ne:.1%}")
    engines = {n: rate(rs, lambda r: r["won"]) for n, rs in results.items()
               if n not in NO_ENGINE and n != "reckless"}
    best_engine = max(engines.values())
    check("B2", "A point factory can win (best engine bot >= 40%)", best_engine >= 0.4,
          f"best engine bot {max(engines, key=engines.get)} wins {best_engine:.1%}")
    check("B3", "A point factory is still risky (no bot wins >= 90%)", best_engine < 0.9,
          f"best engine bot wins {best_engine:.1%}")
    fams = defaultdict(list)
    for n, w in engines.items():
        fams[n.split("/")[0]].append(w)
    viable = [f for f, ws in fams.items() if max(ws) >= 0.5 * best_engine]
    check("C1", "More than one kind of point factory is viable (>= 3 focuses within half the best)",
          len(viable) >= 3, "viable: " + ", ".join(f"{f} {max(fams[f]):.0%}" for f in fams))
    styles = defaultdict(list)
    for n, w in engines.items():
        styles[n.split("/")[1]].append(w)
    smean = {s: sum(ws) / len(ws) for s, ws in styles.items()}
    top = max(smean.values())
    median = sorted(smean.values())[len(smean) // 2]
    check("C2", "No spending strategy dominates (best style's mean <= 1.5x the median style's)",
          top > 0 and top <= 1.5 * median,
          ", ".join(f"{s} {v:.0%}" for s, v in smean.items()))
    engine_runs = [r for n, rs in results.items() if n in engines for r in rs]
    losses = Counter(r["lost_round"] for r in engine_runs if not r["won"])
    worst_round, worst = (losses.most_common(1)[0] if losses else (None, 0))
    share = worst / max(1, sum(losses.values()))
    check("D1", "No cliff: no single round causes > 35% of engine-bot losses", share <= 0.35,
          f"round {worst_round} causes {share:.0%} of losses")
    choice = stats.shops_with_choice / max(1, stats.shops_with_fit)
    check("E1", "Shopping isn't forced: of shops with a good fit, >= 50% have two or more", choice >= 0.5,
          f"{choice:.0%}")
    held = Counter({c: n for c, n in presence.items() if c in t["items"] or c in t["wheels"]})
    top_presence = max(held.values(), default=0) / max(1, wins_total)
    top_id = held.most_common(1)[0][0] if held else "-"
    check("E2", "No must-have: no item or wheel is bought in > 80% of winning runs", top_presence <= 0.8,
          f"{top_id} in {top_presence:.0%} of {wins_total} wins")

    out.append("Rubric (docs/design/session-rubric.md)")
    for cid, label, ok, detail in checks:
        out.append(f"  [{'PASS' if ok else 'FAIL'}] {cid} {label}: {detail}")
    out.append("")
    out.append("Telemetry, engine bots: pick rate = picked / offered; win presence = share of wins that bought it")
    out.append(f"  {'content':<16}{'offered':>8}{'pick':>7}{'win pres':>10}")
    for cid in sorted(stats.offered, key=lambda c: -presence[c]):
        off = stats.offered[cid]
        out.append(f"  {cid:<16}{off:>8}{stats.picked[cid] / off:>7.0%}{presence[cid] / max(1, wins_total):>10.0%}")
    passed = sum(ok for _, _, ok, _ in checks)
    out.append("")
    out.append(f"{passed}/{len(checks)} rubric checks pass")
    return "\n".join(out), checks


# ---------------------------------------------------------------- play by hand

class Human(Player):
    name = "human"

    def zap_choice(self, run, landed):
        if not landed:
            return None
        print("  The Rod can remove one of: " + " ".join(f"{i}:{w.label()}" for i, w in enumerate(landed)))
        s = input("  remove which (Enter to skip)? ").strip()
        return landed[int(s)] if s.isdigit() and int(s) < len(landed) else None

    def use_band(self, run, total, target, used, spins):
        if "band" not in run.consumables:
            return None
        s = input(f"  Outer Band before spin {used}? color R/G/B/Y or Enter: ").strip().upper()
        if s in COLOR_NAMES:
            run.consumables.remove("band")
            return COLOR_NAMES.index(s)
        return None


def describe_item(it):
    e, a = it["effect"], it.get("amount")
    fam = {"color": "color run", "value": "value run", "straight": "straight",
           "color_straight": "color straight"}.get(it.get("family"), "")
    return {
        "flat_points": f"+{a} points",
        "flat_mult": f"+{a} mult",
        "mult_per_color": f"+{a} mult per {COLOR_NAMES[it.get('color', 0)]} die",
        "mult_per_value": f"+{a} mult per die showing {' or '.join(map(str, it.get('values', [])))}",
        "points_per_high": f"+{a} points per die of {it.get('threshold')} or more",
        "mult_per_parity": f"+{a} mult per {'odd' if it.get('parity') else 'even'} die",
        "mult_per_pattern": f"+{a} mult per {fam}",
        "xmult_if_pattern": f"x{a} mult if there is a {fam} of {it.get('min')}+",
        "xmult_longest": f"x(1 + {a} per die past 2) for the longest {fam}",
        "scaling_xmult": f"gains x{a} mult for good each spin with a {fam} of {it.get('min')}+",
        "scaling_mult": f"gains +{a} mult for good per {fam} scored",
        "rod": "after each spin, may remove one landed wedge from the bag",
        "piggy": f"+{int(it.get('rate', 0) * 100)}% interest on money held (max ${it.get('cap')})",
        "spendthrift": f"+{int(it.get('rate', 0) * 100)}% of money held each round; unspent money is lost after the shop",
    }.get(e, e)


def show_state(run):
    print(f"  ${run.money} | wheels: " + ", ".join(f"{i}:{w['name']}({w['size']})" for i, w in enumerate(run.wheels)))
    print("  items: " + (", ".join(f"{i}:{it['name']} ({describe_item(it)})" for i, it in enumerate(run.items)) or "none")
          + " | consumables: " + (", ".join(run.consumables) or "none"))
    bag = Counter(w.label() for w in run.bag)
    print(f"  bag ({len(run.bag)}, wheels hold {run.slots()}): " + " ".join(f"{k}x{v}" if v > 1 else k for k, v in sorted(bag.items())))


def play(seed):
    t = load_tables()
    rng = random.Random(seed)
    run = Run(t, rng)
    human = Human(rng)
    print(f"Spintacular prototype, seed {seed}. Dice are color+value; * is wild, ~ is a value range.")
    for r in range(t["run"]["rounds"]):
        run.round = r
        print(f"\n== Round {r + 1}/{t['run']['rounds']}: reach {t['targets'][r]} in {t['run']['spins_per_round']} spins")
        show_state(run)
        input("  Enter to spin... ")
        won, unused, _, total = play_round(run, human, log=print)
        if not won:
            print(f"\nRun over: {total} of {t['targets'][r]} in round {r + 1}.")
            return
        if r == t["run"]["rounds"] - 1:
            print("\nYou win the run!")
            return
        print(f"  Round won with {unused} spins left; +${round_income(run, unused)}")
        offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        reroll = t["shop"]["reroll_base"]
        while True:
            print("\n  SHOP")
            show_state(run)
            for i, o in enumerate(offers):
                if not o.get("sold"):
                    desc = t.get(o["kind"] + "s", {}).get(o["id"], {})
                    extra = describe_item(desc) if o["kind"] == "item" else desc.get("text", "")
                    print(f"   {i}: {o['name']} ({o['rarity']}) ${o['price']}  {extra}")
            cmd = input(f"  buy N | sell N | order 2,0,1 | wheels 1,0,2 | reroll (${reroll}) | done: ").strip().split()
            if not cmd or cmd[0] == "done":
                break
            if cmd[0] == "buy" and len(cmd) > 1 and cmd[1].isdigit() and int(cmd[1]) < len(offers):
                o = offers[int(cmd[1])]
                if o.get("sold") or o["price"] > run.money or not can_take(run, o):
                    print("  can't buy that")
                    continue
                tgt = None
                if o["kind"] in ("upgrade", "zap"):
                    for i, w in enumerate(run.bag):
                        print(f"   {i}:{w.label()}", end="")
                    s = input("\n  which wedge? ").strip()
                    if not s.isdigit() or int(s) >= len(run.bag):
                        continue
                    tgt = run.bag[int(s)]
                choice = None
                if o["id"] == "paint":
                    c = input("  which color R/G/B/Y? ").strip().upper()
                    if c not in COLOR_NAMES:
                        continue
                    choice = COLOR_NAMES.index(c)
                elif o["id"] == "recut":
                    v = input("  which value 1-6? ").strip()
                    if v not in list("123456"):
                        continue
                    choice = int(v)
                apply_offer(run, o, tgt, choice)
                o["sold"] = True
            elif cmd[0] == "sell" and len(cmd) > 1 and cmd[1].isdigit() and int(cmd[1]) < len(run.items):
                it = run.items.pop(int(cmd[1]))
                run.money += sell_value(run, it)
            elif cmd[0] == "order" and len(cmd) > 1:
                idx = [int(x) for x in cmd[1].split(",")]
                if sorted(idx) == list(range(len(run.items))):
                    run.items = [run.items[i] for i in idx]
            elif cmd[0] == "wheels" and len(cmd) > 1:
                idx = [int(x) for x in cmd[1].split(",")]
                if sorted(idx) == list(range(len(run.wheels))):
                    run.wheels = [run.wheels[i] for i in idx]
            elif cmd[0] == "reroll" and run.money >= reroll:
                run.money -= reroll
                reroll += t["shop"]["reroll_step"]
                offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        if run.has("spendthrift"):
            run.money = 0


def scaled_targets(t, scale):
    """Scale the target curve, fully at the last round and not at all at round 1."""
    n = len(t["targets"])
    return [int(v * scale ** (r / (n - 1))) for r, v in enumerate(t["targets"])]


def calibrate(t, runs, wall=0.02):
    """Find the target scale at which the best no-engine bot wins about `wall` of runs."""
    roster = bot_roster()
    lo, hi = 0.02, 50.0
    for _ in range(10):
        mid = (lo * hi) ** 0.5
        tt = copy.deepcopy(t)
        tt["targets"] = scaled_targets(t, mid)
        best = 0
        for name in ("flat/spender", "random"):
            wins = sum(play_run(tt, 50_000 + i, roster[name], Stats())["won"] for i in range(runs))
            best = max(best, wins / runs)
        if best > wall:
            lo = mid
        else:
            hi = mid
    return hi


def compare(t, runs):
    """Calibrate each scoring layer's wall to the same height, then run the rubric on each."""
    out = [f"Each layer's targets are scaled so the best no-engine bot wins about 2%, then {runs} runs per bot.", ""]
    rows = []
    for layer in LAYERS:
        tt = copy.deepcopy(t)
        tt["scoring"]["layer"] = layer
        scale = calibrate(tt, max(100, runs // 2))
        tt["targets"] = scaled_targets(tt, scale)
        text, checks = report(tt, runs)
        results = {c: (ok, det) for c, _, ok, det in checks}
        rows.append((layer, scale, tt["targets"], results, text))
    out.append(f"{'check':<6}" + "".join(f"{l:>34}" for l, *_ in rows))
    for cid in rows[0][3]:
        out.append(f"{cid:<6}" + "".join(
            f"{('PASS ' if r[3][cid][0] else 'FAIL ') + r[3][cid][1][-28:]:>34}" for r in rows))
    out.append(f"{'pass':<6}" + "".join(f"{sum(ok for ok, _ in r[3].values()):>34}" for r in rows))
    out.append("")
    for layer, scale, targets, _, text in rows:
        out.append(f"== {layer}: target scale {scale:.2f}, targets {targets}")
        out.extend(l for l in text.splitlines()[2:24])
        out.append("")
    return "\n".join(out)


def capacity(t, runs):
    """Median round total per round when no one can lose: how fast each bot's score grows."""
    out = ["Median round total (all spins played, no losing), by round"]
    out.append(f"{'bot':<20}" + "".join(f"{r + 1:>9}" for r in range(t["run"]["rounds"])))
    for name, make in bot_roster().items():
        per = [[] for _ in range(t["run"]["rounds"])]
        for i in range(runs):
            totals = []
            play_run(t, 7_000 + i, make, Stats(), totals=totals)
            for r, v in enumerate(totals):
                per[r].append(v)
        out.append(f"{name:<20}" + "".join(f"{sorted(v)[len(v) // 2]:>9}" for v in per))
    out.append(f"{'targets':<20}" + "".join(f"{v:>9}" for v in t["targets"]))
    return "\n".join(out)


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "sim"
    if mode == "compare":
        print(compare(load_tables(), int(sys.argv[2]) if len(sys.argv) > 2 else 300))
    elif mode == "capacity":
        print(capacity(load_tables(), int(sys.argv[2]) if len(sys.argv) > 2 else 100))
    elif mode == "play":
        play(int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(10**6))
    else:
        text, checks = report(load_tables(), int(sys.argv[2]) if len(sys.argv) > 2 else 200)
        print(text)
        sys.exit(0 if all(ok for _, _, ok, _ in checks) else 1)
