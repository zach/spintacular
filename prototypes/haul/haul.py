"""PROTOTYPE: Spintacular as a press-your-luck haul. Throwaway code for tuning the design.

    python3 prototypes/haul/haul.py sim [runs] [key=value ...]    # bots play; prints the rubric report
    python3 prototypes/haul/haul.py play [seed]                   # play a run yourself
    python3 prototypes/haul/haul.py capacity [runs]               # how fast each bot's score grows
    python3 prototypes/haul/haul.py calibrate [runs] [key=value ...]  # scale targets to a 2% wall, then sim
    python3 prototypes/haul/haul.py experiments [runs]            # compare the open design choices

Every number and every piece of content is in tables.json; key=value overrides one for a run,
for example round.mode=trips or scoring.quota_bonus=1. Rules: docs/design/.
"""
import copy
import json
import math
import os
import random
import sys
import zlib
from collections import Counter, defaultdict
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
COLORS = "RGBY"
ADDITIVE = {"flat_points", "flat_mult", "mult_per_color", "points_per_high", "mult_per_parity",
            "mult_if_flush", "mult_if_straight", "mult_per_cargo_past", "mult_per_risky", "scaling_mult_size"}


def load_tables(overrides=()):
    with open(os.path.join(HERE, "tables.json")) as f:
        t = json.load(f)
    for ov in overrides:
        key, val = ov.split("=", 1)
        d = t
        path = key.split(".")
        for p in path[:-1]:
            d = d[p]
        try:
            d[path[-1]] = json.loads(val)
        except ValueError:
            d[path[-1]] = val
    return t


# ---------------------------------------------------------------- model

class Wedge:
    __slots__ = ("kind", "color", "value", "bonus")

    def __init__(self, kind="cargo", color=None, value=0):
        self.kind, self.color, self.value, self.bonus = kind, color, value, 0

    def label(self):
        if self.kind == "blank":
            return "__"
        if self.kind != "cargo":
            return self.kind.upper()
        return ("*" if self.color is None else COLORS[self.color]) + str(self.value) + ("+" if self.bonus else "")


class Load:
    """What is in the truck."""
    __slots__ = ("cargo", "crates", "tarps", "risky", "covered", "clutch_used", "spins", "used")

    def __init__(self):
        self.used = set()        # ids of the wedges that have landed this trip
        self.cargo = []          # (color or None, value, bonus points)
        self.crates = self.tarps = self.risky = self.spins = 0
        self.covered = self.clutch_used = False

    def plus(self, cargo=None, crate=False):
        n = Load()
        n.cargo = self.cargo + [cargo] if cargo else self.cargo
        n.crates = self.crates + (1 if crate else 0)
        n.tarps, n.risky, n.covered, n.clutch_used, n.spins = self.tarps, self.risky, self.covered, self.clutch_used, self.spins
        n.used = self.used
        return n

    def label(self):
        return " ".join(("*" if c is None else COLORS[c]) + str(v) for c, v, _ in self.cargo) or "empty"


class Run:
    def __init__(self, t, rng):
        self.t, self.rng = t, rng
        self.money = t["run"]["start_money"]
        self.bag = [Wedge("cargo", COLORS.index(w[0]), int(w[1])) for w in t["start"]["bag"]]
        self.wheels = [dict(id=w, **t["wheels"][w]) for w in t["start"]["wheels"]]
        self.items, self.consumables, self.bought = [], [], set()
        self.round = 0
        self.inspection = None    # this round's inspection id, if any
        self.quota = 0
        self.spare = 0            # spare tarps left this round
        self.m = Counter()        # run metrics

    def has(self, effect):
        return [i for i in self.items if i["effect"] == effect]

    def slots(self):
        return sum(w["size"] for w in self.wheels)


# ---------------------------------------------------------------- scoring

def score(run, L, commit=False, trace=None):
    """A delivered load: points x mult, then the quota and full-truck bonuses."""
    cargo = L.cargo
    n = len(cargo)
    if n == 0:
        return 0
    s = run.t["scoring"]
    points = 0
    by_color = [0, 0, 0, 0]
    wild = 0
    for c, v, b in cargo:
        points += v + b
        if c is None:
            wild += 1
        else:
            by_color[c] += 1
    mult = n * s["mult_per_cargo"]
    flush = max(by_color) + wild
    vals = sorted({v for _, v, _ in cargo})
    straight = best = 1
    for a, b in zip(vals, vals[1:]):
        best = best + 1 if b == a + 1 else 1
        straight = max(straight, best)
    if trace is not None:
        trace.append(f"{L.label()}: {points} points x {mult:g} for {n} cargo")
    if flush >= 3:
        m = s["flush"][min(flush, 6) - 3]
        mult += m
        if trace is not None:
            trace.append(f"{min(flush, 6)} of a color: +{m} mult")
    if straight >= 3:
        m = s["straight"][min(straight, 6) - 3]
        mult += m
        if trace is not None:
            trace.append(f"run of {min(straight, 6)} values: +{m} mult")
    quota_bonus, full_bonus = s["quota_bonus"], s["full_bonus"]
    for it in run.items:
        e, a = it["effect"], it.get("amount")
        before = (points, mult)
        if e == "flat_points":
            points += a
        elif e == "flat_mult":
            mult += a
        elif e == "mult_per_color":
            mult += a * (by_color[it["color"]] + wild)
        elif e == "points_per_high":
            points += a * sum(1 for _, v, _ in cargo if v >= it["threshold"])
        elif e == "mult_per_parity":
            mult += a * sum(1 for _, v, _ in cargo if v % 2 == it["parity"])
        elif e == "mult_if_flush":
            if flush >= it["min"]:
                mult += a
        elif e == "mult_if_straight":
            if straight >= it["min"]:
                mult += a
        elif e == "mult_per_cargo_past":
            mult += a * max(0, n - it["past"])
        elif e == "mult_per_risky":
            mult += a * L.risky
        elif e == "xmult_if_size":
            if n >= it["min"]:
                mult *= a
        elif e == "xmult_if_flush":
            if flush >= it["min"]:
                mult *= a
        elif e == "xmult_if_straight":
            if straight >= it["min"]:
                mult *= a
        elif e == "xmult_per_flush_past":
            if flush >= 3:
                mult *= 1 + a * (flush - 2)
        elif e == "xmult_if_covered":
            if L.covered:
                mult *= a
        elif e == "scaling_mult_size":
            if commit and n >= it["min"]:
                it["state"] = it.get("state", 0) + a
            mult += it.get("state", 0)
        elif e == "scaling_xmult_quota":
            mult *= it.get("state", 1.0)
        elif e == "quota_bonus":
            quota_bonus = max(quota_bonus, a) if s["quota_bonus"] > 1 else quota_bonus
        elif e == "full_bonus":
            full_bonus = max(full_bonus, a)
        if trace is not None and (points, mult) != before:
            trace.append(f"{it['name']}: {before[0]:g} x {before[1]:g} -> {points:g} x {mult:g}")
    if L.crates:
        mult *= s["crate_mult"] ** L.crates
        if trace is not None:
            trace.append(f"{L.crates} crate(s): x{s['crate_mult'] ** L.crates:g}")
    factor = 1.0
    hit = points >= run.quota and quota_bonus > 1
    if hit:
        factor *= quota_bonus
        if trace is not None:
            trace.append(f"quota {run.quota} reached: x{quota_bonus:g}")
    if len(vals) >= s["truck_size"]:
        factor *= full_bonus
        if trace is not None:
            trace.append(f"full truck: x{full_bonus:g}")
    total = int(points * mult * factor)
    if commit:
        if hit:
            run.m["quota_hits"] += 1
            for it in run.has("scaling_xmult_quota"):
                it["state"] = it.get("state", 1.0) + it["amount"]
            run.money += sum(it["amount"] for it in run.has("tip"))
    if trace is not None:
        trace.append(f"= {total}")
    return total


# ---------------------------------------------------------------- a round

def deal(run):
    """Fill the wheels from the bag. Slots the bag can't fill hold empty wedges."""
    bag = run.bag[:]
    run.rng.shuffle(bag)
    if run.t["round"]["deal"] == "assign":
        # Experiment: the player places wedges freely. A careful player groups each value on one wheel.
        bag.sort(key=lambda w: (w.kind != "cargo", w.value))
        loads, k = [], 0
        for w in run.wheels:
            loads.append(bag[k:k + w["size"]])
            k += w["size"]
    else:
        loads = [[] for _ in run.wheels]
        # Sorter wheels take their pick first: the lowest or the highest cargo in the bag.
        for wi, w in enumerate(run.wheels):
            if w["effect"] in ("low", "high"):
                cargo = sorted((x for x in bag if x.kind == "cargo"), key=lambda x: x.value,
                               reverse=w["effect"] == "high")[:w["size"]]
                loads[wi] = cargo
                picked = set(map(id, cargo))
                bag = [x for x in bag if id(x) not in picked]
        k = 0
        for _ in range(max(w["size"] for w in run.wheels)):
            for wi, w in enumerate(run.wheels):
                if len(loads[wi]) < w["size"] and k < len(bag):
                    loads[wi].append(bag[k])
                    k += 1
    for wi, w in enumerate(run.wheels):
        loads[wi] += [Wedge("blank") for _ in range(w["size"] - len(loads[wi]))]
    return loads


def landed_value(run, wheel, w):
    return min(run.t["run"]["max_value"], w.value + 1) if wheel["effect"] == "bump" else w.value


def clashes(run, wheel, w, L):
    """No repeats: a wedge that already landed this trip, or cargo whose value is already in the truck."""
    if id(w) in L.used:
        return True
    return w.kind == "cargo" and any(v == landed_value(run, wheel, w) for _, v, _ in L.cargo)


def covered(run, wheel, L):
    """Whether a duplicate landed now would be absorbed."""
    if wheel["effect"] == "padded" or (run.has("double_clutch") and not L.clutch_used):
        return True
    return run.inspection != "no_tarps" and (L.tarps > 0 or run.spare > 0)


def spoil_chance(run, wheel, wedges, L):
    if covered(run, wheel, L):
        return 0.0
    return sum(1 for w in wedges if clashes(run, wheel, w, L)) / len(wedges)


def play_round(run, player, log=None, endless=False):
    t, rc = run.t, run.t["round"]
    target = math.inf if endless else t["targets"][run.round]
    fuel_mode = rc["mode"] == "fuel"
    extra = sum(i["amount"] for i in run.has("fuel")) - (2 if run.inspection == "fuel_down" else 0)
    fuel = rc["fuel"] + extra          # fuel mode: one pool for the round
    tank = rc["tank"] + extra          # trips mode: fuel for each trip
    trips_left = rc["trips"]
    quota = t["scoring"]["quotas"][min(run.round, len(t["scoring"]["quotas"]) - 1)]
    run.quota = math.ceil(quota * 1.25) if run.inspection == "quota_up" else quota
    run.spare = 0 if run.inspection == "no_tarps" else sum(i["amount"] for i in run.has("spare_tarp"))
    loads = deal(run)
    rods = sum(i["amount"] for i in run.has("rod"))
    insurance = sum(i["amount"] for i in run.has("insurance"))
    risky = t["scoring"]["risky_spin"]
    free_used = set()
    total = 0
    L = Load()
    landed = []          # wedges in the current load, for The Rod

    def spoil():
        nonlocal total, L, landed, trips_left
        pay = int(insurance * score(run, L)) if insurance else 0
        total += pay
        run.m["spoiled"] += 1
        run.m["spoiled_cargo"] += len(L.cargo)
        if log:
            log(f"    SPOILED: lost {L.label()}" + (f", insurance pays {pay}" if pay else ""))
        L, landed = Load(), []
        trips_left -= 1

    def land(wheel, wedges, idx, p):
        """Resolve the wedge at idx. Returns False if the load spoiled."""
        w = wedges[idx]
        if not clashes(run, wheel, w, L):
            L.used.add(id(w))
            if w.kind == "crate":
                L.crates += 1
            elif w.kind == "tarp":
                if run.inspection != "no_tarps":
                    L.tarps += 1
            elif w.kind == "cargo":
                L.cargo.append((w.color, landed_value(run, wheel, w),
                                w.bonus + (wheel["amount"] if wheel["effect"] == "gilded" else 0)))
                landed.append(w)
            if p >= risky and w.kind == "cargo":
                L.risky += 1
            return True
        # A repeat. Protections, in order.
        if wheel["effect"] == "padded":
            return True
        if run.has("double_clutch") and not L.clutch_used:
            L.clutch_used = True
            return True
        if run.inspection != "no_tarps" and (L.tarps or run.spare):
            if L.tarps:
                L.tarps -= 1
            else:
                run.spare -= 1
            L.covered = True
            if log:
                log("    a tarp covers the duplicate")
            return True
        if L.cargo and player.use_fix(run, L):
            if "nudge" in run.consumables:
                for j in ((idx - 1) % len(wedges), (idx + 1) % len(wedges)):
                    if not clashes(run, wheel, wedges[j], L):
                        run.consumables.remove("nudge")
                        if log:
                            log("    nudge saves the load")
                        return land(wheel, wedges, j, 0)
            if "respin" in run.consumables:
                run.consumables.remove("respin")
                j = run.rng.randrange(len(wedges))
                if not clashes(run, wheel, wedges[j], L):
                    if log:
                        log("    respin saves the load")
                    return land(wheel, wedges, j, 0)
        spoil()
        return False

    while True:
        full = len({v for _, v, _ in L.cargo}) >= t["scoring"]["truck_size"]
        can_spin = (fuel > 0 if fuel_mode else L.spins < tank) and not full
        run.hud = f"fuel {fuel}" if fuel_mode else f"trip {rc['trips'] - trips_left + 1} of {rc['trips']}, tank {tank - L.spins}"
        act = player.choose(run, loads, L, total, target, fuel if fuel_mode else None) if can_spin else None
        if act is None:
            had = bool(L.cargo)
            if had:
                trace = [] if log else None
                value = score(run, L, commit=True, trace=trace)
                total += value
                run.m["deliveries"] += 1
                run.m["delivered_cargo"] += len(L.cargo)
                if log:
                    log(f"  DELIVER {' | '.join(trace)}   (round {total}/{target})")
                if rods > 0:
                    victim = player.zap_choice(run, [w for w in landed if w in run.bag])
                    if victim is not None:
                        rods -= 1
                        run.bag.remove(victim)
                        for wedges in loads:
                            while victim in wedges:
                                wedges[wedges.index(victim)] = Wedge("blank")
            L, landed = Load(), []
            trips_left -= 1
            if total >= target and not endless:
                break
            if (fuel_mode and fuel <= 0) or (not fuel_mode and trips_left <= 0) or (not had and can_spin):
                break
            continue
        wheel, wedges = run.wheels[act], loads[act]
        if wheel["effect"] == "express" and (act, trips_left if not fuel_mode else 0) not in free_used:
            free_used.add((act, trips_left if not fuel_mode else 0))
        elif fuel_mode:
            fuel -= 1
        else:
            L.spins += 1
        run.m["spins"] += 1
        p = spoil_chance(run, wheel, wedges, L)
        weights = None
        if p > 0 and "band" in run.consumables and player.use_band(run, L, p):
            run.consumables.remove("band")
            weights = [0.5 if clashes(run, wheel, w, L) else 1 for w in wedges]
        idx = run.rng.choices(range(len(wedges)), weights)[0]
        if log:
            w = wedges[idx]
            log(f"    wheel {act} ({p:.0%} to spoil): {w.label()}")
        if not land(wheel, wedges, idx, p):
            if not fuel_mode and trips_left <= 0:
                break
    return total >= target, max(0, fuel) if fuel_mode else max(0, trips_left) * 2, total


def round_income(run, unused):
    inc = run.t["income"]
    gain = inc["per_round"] + min(inc["unused_cap"], unused * inc["per_unused_fuel"])
    gain += min(inc["interest_cap"], run.money // inc["interest_per"])
    for it in run.has("piggy"):
        gain += min(it["cap"], int(run.money * it["rate"]))
    for it in run.has("spendthrift"):
        gain += int(run.money * it["rate"])
    run.money += gain
    return gain


# ---------------------------------------------------------------- shop

def pick_rarity(t, rng, rnd, pool):
    w = t["shop"]["rarity_by_round"][0]["weights"]
    for row in t["shop"]["rarity_by_round"]:
        if rnd + 1 >= row["from_round"]:
            w = row["weights"]
    present = {v["rarity"] for v in pool.values()}
    names = [r for r in w if w[r] > 0 and r in present]
    return rng.choices(names, [w[r] for r in names])[0]


def roll_offer(run):
    t, rng = run.t, run.rng
    kw = t["shop"]["kind_weights"]
    kind = rng.choices(list(kw), list(kw.values()))[0]
    prices = t["shop"]["prices"]
    if kind in ("item", "wheel", "upgrade", "consumable"):
        pool = t[kind + "s"]
        rar = pick_rarity(t, rng, run.round, pool)
        cid = rng.choice([k for k, v in pool.items() if v["rarity"] == rar])
        return dict(kind=kind, id=cid, name=pool[cid]["name"], price=prices[kind][rar], rarity=rar)
    if kind == "wedge":
        if rng.random() < t["shop"]["special_wedge_share"]:
            sp = rng.choice(["tarp", "tarp", "crate"])
            return dict(kind="wedge", id=sp + "_wedge", name=f"{sp.title()} wedge", special=sp,
                        price=prices[sp], rarity="common")
        c, v = rng.randrange(4), rng.randint(1, 6)
        return dict(kind="wedge", id="cargo_wedge", name=f"Cargo wedge {COLORS[c]}{v}", color=c, value=v,
                    price=prices["wedge"], rarity="common")
    return dict(kind="zap", id="zap", name="Zap (remove a wedge)", price=prices["zap"], rarity="common")


def can_take(run, o):
    r = run.t["run"]
    if o["kind"] == "item":
        return len(run.items) < r["max_items"]
    if o["kind"] == "wheel":
        return len(run.wheels) < r["max_wheels"]
    if o["kind"] == "consumable":
        return len(run.consumables) < r["max_consumables"]
    return True


def apply_offer(run, o, wedge=None, choice=None):
    k = o["kind"]
    if k == "item":
        it = copy.deepcopy(run.t["items"][o["id"]])
        it.update(id=o["id"], price=o["price"])
        run.items.append(it)
    elif k == "wheel":
        run.wheels.append(dict(id=o["id"], price=o["price"], **run.t["wheels"][o["id"]]))
    elif k == "consumable":
        run.consumables.append(o["id"])
    elif k == "wedge":
        run.bag.append(Wedge(o["special"]) if o.get("special") else Wedge("cargo", o["color"], o["value"]))
    elif k == "zap":
        if wedge is not None and len(run.bag) > 1:
            run.bag.remove(wedge)
    elif k == "upgrade" and wedge is not None:
        if o["id"] == "recut":
            wedge.value = choice
        elif o["id"] == "paint":
            wedge.color = choice
        elif o["id"] == "premium":
            wedge.value = min(run.t["run"]["max_value"], wedge.value + 1)
        elif o["id"] == "wild":
            wedge.color = None
        elif o["id"] == "stamp":
            wedge.bonus += 3
    run.money -= o["price"]
    run.bought.add(o["id"])


def sell_value(run, thing):
    return int(thing.get("price", 0) * run.t["shop"]["sell_fraction"])


# ---------------------------------------------------------------- players

class Player:
    """Buys nothing. Presses its luck by expected value, one spin ahead."""
    name = "passive"
    caution = 1.0

    def __init__(self, rng):
        self.rng = rng
        self._fresh = {}

    def shop(self, run, offers, stats):
        pass

    def use_fix(self, run, L):
        return len(L.cargo) >= 2

    def use_band(self, run, L, p):
        return p >= 0.25 and len(L.cargo) >= 3

    def zap_choice(self, run, landed):
        return None

    def fresh(self, run, loads):
        """Rough value of one spin spent on a new load instead: a single-cargo delivery."""
        key = (run.round, len(run.items), run.quota)
        if key not in self._fresh:
            tot = cnt = 0
            for wheel, wedges in zip(run.wheels, loads):
                for w in wedges:
                    cnt += 1
                    if w.kind == "cargo":
                        tot += score(run, Load().plus((w.color, landed_value(run, wheel, w), w.bonus)))
            self._fresh[key] = tot / max(1, cnt)
        return self._fresh[key]

    def choose(self, run, loads, L, total, target, fuel):
        """Returns the wheel to spin next, or None to deliver."""
        now = score(run, L) if L.cargo else 0
        need = target - total
        if L.cargo and now >= need and self.caution > 0:
            return None
        ins = sum(i["amount"] for i in run.has("insurance"))
        best, best_ev, best_p = None, -1.0, 0.0
        for wi, (wheel, wedges) in enumerate(zip(run.wheels, loads)):
            safe = covered(run, wheel, L)
            ev = p = 0.0
            for w in wedges:
                if clashes(run, wheel, w, L):
                    if safe:
                        ev += now
                    else:
                        p += 1
                        ev += ins * now
                elif w.kind in ("blank", "tarp"):
                    ev += now
                elif w.kind == "crate":
                    ev += score(run, L.plus(crate=True)) if L.cargo else 0
                else:
                    ev += score(run, L.plus((w.color, landed_value(run, wheel, w),
                                             w.bonus + (wheel["amount"] if wheel["effect"] == "gilded" else 0))))
            ev /= len(wedges)
            p /= len(wedges)
            if ev > best_ev + 1e-9 or (abs(ev - best_ev) <= 1e-9 and p < best_p):
                best, best_ev, best_p = wi, ev, p
        if not L.cargo or self.caution == 0:
            return best
        if fuel is None:                      # trips: delivering doesn't free up spins
            return best if best_ev > self.caution * now else None
        fresh = self.fresh(run, loads)
        if best_p == 0:
            return best if best_ev > now else None
        if need == math.inf:
            return best if best_ev > now + self.caution * fresh else None
        # Play to the target. Safe play is worth roughly `fresh` per fuel, more for bigger loads.
        safe = fresh * fuel * 2.0
        if need - now > safe * 1.5:           # safe play can't get there: gamble
            return best if best_p < 0.7 else None
        if need - now <= safe * 0.6:          # comfortably on pace: bank it
            return best if best_p <= 0.2 and best_ev > (now + fresh) * 1.3 else None
        return best if best_ev > now + self.caution * fresh else None


class RandomPlayer(Player):
    name = "random"

    def shop(self, run, offers, stats):
        for o in offers:
            if self.rng.random() < 0.5 and o["price"] <= run.money and can_take(run, o):
                apply_offer(run, o, self.rng.choice(run.bag),
                            self.rng.randrange(4) if o["id"] == "paint" else self.rng.randint(1, 6))
                stats.picked[o["id"]] += 1
        self.rng.shuffle(run.items)


FIT = {
    # item effect -> fit by focus kind; "*" is the default for engine focuses
    "flat_points": {"flat": 1.5, "*": 1.0}, "flat_mult": {"flat": 1.5, "*": 1.0},
    "mult_per_parity": {"flat": 1.2, "*": 0.8}, "points_per_high": {"flat": 1.2, "quota": 3.0, "*": 1.0},
    "mult_per_color": {"flat": 1.2, "*": 0.5},
    "mult_if_flush": {"color": 3.0, "*": 0.8}, "xmult_if_flush": {"color": 4.5, "*": 0.5},
    "xmult_per_flush_past": {"color": 5.0, "*": 0.5},
    "mult_if_straight": {"straight": 3.0, "*": 1.0}, "xmult_if_straight": {"straight": 4.5, "*": 0.8},
    "spare_tarp": {"safety": 4.5, "*": 3.0}, "double_clutch": {"safety": 5.0, "*": 4.0},
    "mult_per_cargo_past": {"safety": 4.5, "straight": 3.5, "*": 2.5},
    "xmult_if_size": {"safety": 4.5, "straight": 3.5, "*": 2.5},
    "scaling_mult_size": {"safety": 5.0, "straight": 3.5, "*": 2.5},
    "insurance": {"safety": 2.5, "*": 2.0}, "xmult_if_covered": {"safety": 4.0, "*": 1.5},
    "mult_per_risky": {"safety": 2.0, "*": 2.5},
    "quota_bonus": {"quota": 5.0, "*": 3.0}, "scaling_xmult_quota": {"quota": 5.0, "*": 3.5},
    "tip": {"quota": 2.5, "*": 1.0}, "full_bonus": {"safety": 4.0, "straight": 4.0, "*": 2.0},
    "fuel": {"*": 3.5}, "rod": {"*": 2.0},
}
FLAT_OK = {"flat_points", "flat_mult", "mult_per_parity", "points_per_high", "mult_per_color", "fuel"}


class Builder(Player):
    """Shops toward a focus with an economy style.

    focus: color (one color) | straight | safety | quota | adaptive | flat (no engine)
    style: spender | saver | reroller | bargain
    """

    def __init__(self, rng, focus, style, color=None):
        super().__init__(rng)
        self.focus, self.style = focus, style
        self.color = rng.randrange(4) if color is None else color
        self.name = f"{focus}/{style}"

    def kind(self, run):
        if self.focus != "adaptive":
            return self.focus
        sc = Counter()
        for it in run.items:
            for k in ("color", "straight", "safety", "quota"):
                sc[k] += self.item_fit(it, k)
        return sc.most_common(1)[0][0] if sc and max(sc.values()) >= 3 else "adaptive"

    def item_fit(self, it, kind):
        e = it["effect"]
        if kind == "flat":
            return FIT.get(e, {}).get("flat", 1.0 if e in FLAT_OK else 0)
        if e == "piggy":
            return 2.5 if self.style in ("saver", "bargain") else 0.5
        if e == "spendthrift":
            return 2.5 if self.style in ("spender", "reroller") else 0.3
        if e == "mult_per_color":
            return 3.0 if kind == "color" and it["color"] == self.color else 0.5
        row = FIT.get(e, {})
        return row.get(kind, row.get("*", 1.0))

    def wedge_ok(self, run, w, kind):
        """Whether a wedge belongs in this build's bag."""
        if w.kind != "cargo":
            return True
        if kind == "color":
            return w.color in (None, self.color)
        if kind == "flat":
            return w.value >= 3
        counts = Counter(x.value for x in run.bag if x.kind == "cargo")
        return counts[w.value] <= max(1, min(counts.values()))

    def fit(self, run, o, kind):
        if not can_take(run, o):
            return 0
        k = o["kind"]
        engine = kind != "flat"
        if k == "item":
            return self.item_fit(run.t["items"][o["id"]], kind)
        if k == "wheel":
            eff = run.t["wheels"][o["id"]]["effect"]
            return {"none": 2.0, "gilded": 2.5 if kind in ("quota", "flat") else 1.5,
                    "bump": 3.0 if kind == "quota" else 1.5, "express": 3.0,
                    "low": 4.0 if kind in ("safety", "straight") else 3.0,
                    "high": 4.0 if kind in ("safety", "quota") else 3.0,
                    "padded": 4.5 if engine else 0}[eff] * (1 if engine or eff in ("none", "gilded") else 0)
        if k == "upgrade":
            if not engine:
                return 0
            return {"recut": 2.0 if kind in ("straight", "safety", "adaptive") else 0.8,
                    "paint": 3.0 if kind == "color" else 0.2,
                    "premium": 3.5 if kind in ("quota", "safety", "straight") else 2.5,
                    "wild": 3.0 if kind == "color" else 0.3,
                    "stamp": 3.5 if kind == "quota" else 2.0}[o["id"]]
        if k == "wedge":
            if o.get("special"):
                return 0 if not engine else (3.5 if o["special"] == "tarp" else 3.0)
            if kind == "color":
                return 2.5 if o["color"] == self.color else 0
            return 1.0 if o["value"] >= 4 else 0.3
        if k == "zap":
            off = [w for w in run.bag if not self.wedge_ok(run, w, kind)]
            return 2.5 if engine and off and len(run.bag) > run.slots() and kind != "adaptive" else 0
        return 1.5 if k == "consumable" else 0

    def target_for(self, run, o, kind):
        cargo = [w for w in run.bag if w.kind == "cargo"]
        off = [w for w in cargo if not self.wedge_ok(run, w, kind)]
        counts = Counter(w.value for w in cargo)
        if o["kind"] == "zap":
            return (max(off, key=lambda w: counts[w.value]) if off else None), None
        oid = o["id"]
        if oid == "paint":
            pool = [w for w in cargo if w.color not in (None, self.color)]
            return (self.rng.choice(pool) if pool else None), self.color
        if oid == "wild":
            pool = [w for w in cargo if w.color not in (None, self.color)] or [w for w in cargo if w.color is not None]
            return (self.rng.choice(pool) if pool else None), None
        if oid == "recut":
            pool = cargo
            if not pool:
                return None, None
            w = max(pool, key=lambda x: counts[x.value])
            return w, min(range(1, 7), key=lambda v: (counts[v], -v))
        if oid == "premium":
            mx = run.t["run"]["max_value"]
            pool = [w for w in cargo if w.value < mx]
            if not pool:
                return None, None
            return max(pool, key=lambda x: (counts[x.value] - counts[x.value + 1], x.value)), None
        if oid == "stamp":
            return (max(cargo, key=lambda x: (-x.bonus, x.value)) if cargo else None), None
        return None, None

    def wants(self, run, o, f):
        left = run.money - o["price"]
        if self.style == "saver":
            return f >= 3.0 or left >= min(25, 5 * run.round)
        if self.style == "bargain":
            return o["price"] <= 3 or f / o["price"] >= 0.5
        return True

    def shop(self, run, offers, stats):
        kind = self.kind(run)
        reroll, rerolls = run.t["shop"]["reroll_base"], 0
        while True:
            scored = sorted(((self.fit(run, o, kind), i, o) for i, o in enumerate(offers)), key=lambda p: -p[0])
            good = sum(1 for f, _, _ in scored if f >= 2.0)
            stats.shops += 1
            stats.shops_with_fit += good >= 1
            stats.shops_with_choice += good >= 2
            for f, _, o in scored:
                if o.get("sold") or f < 1.0 or not self.wants(run, o, f):
                    continue
                if o["kind"] == "item" and len(run.items) >= run.t["run"]["max_items"]:
                    weakest = min(run.items, key=lambda it: self.item_fit(it, kind))
                    if self.item_fit(weakest, kind) < f - 1.5 and run.money + sell_value(run, weakest) >= o["price"]:
                        run.money += sell_value(run, weakest)
                        run.items.remove(weakest)
                if o["price"] > run.money or not can_take(run, o):
                    continue
                wedge, choice = self.target_for(run, o, kind) if o["kind"] in ("upgrade", "zap") else (None, None)
                if o["kind"] in ("upgrade", "zap") and wedge is None:
                    continue
                apply_offer(run, o, wedge, choice)
                o["sold"] = True
                stats.picked[o["id"]] += 1
            if self.style == "reroller" and rerolls < 3 and run.money >= reroll + 4 \
                    and not any(f >= 3 and not o.get("sold") for f, _, o in scored):
                run.money -= reroll
                reroll += run.t["shop"]["reroll_step"]
                rerolls += 1
                offers = [roll_offer(run) for _ in range(run.t["shop"]["offers"])]
                for o in offers:
                    stats.offered[o["id"]] += 1
                continue
            break
        run.items.sort(key=lambda it: 0 if it["effect"] in ADDITIVE else 1)   # additive before multipliers

    def zap_choice(self, run, landed):
        kind = self.kind(run)
        if len(run.bag) <= run.slots() or kind in ("flat", "adaptive"):
            return None
        off = [w for w in landed if not self.wedge_ok(run, w, kind)]
        return off[0] if off else None


class Reckless(Builder):
    """An insane risk: never delivers by choice. Only a full truck or an empty tank ends a load."""
    caution = 0

    def __init__(self, rng):
        super().__init__(rng, "safety", "spender")
        self.name = "reckless"


# ---------------------------------------------------------------- simulation

class Stats:
    def __init__(self):
        self.offered, self.picked = Counter(), Counter()
        self.shops = self.shops_with_fit = self.shops_with_choice = 0

    def merge(self, o):
        self.offered += o.offered
        self.picked += o.picked
        self.shops += o.shops
        self.shops_with_fit += o.shops_with_fit
        self.shops_with_choice += o.shops_with_choice


def pick_inspection(run):
    rc = run.t["round"]
    if rc["inspections"] and (run.round + 1) % rc["inspection_every"] == 0:
        return run.rng.choice(list(run.t["inspections"]))
    return None


def play_run(t, seed, make_player, stats, log=None, totals=None):
    rng = random.Random(seed)
    run = Run(t, rng)
    player = make_player(random.Random(seed + 1))
    rounds = t["run"]["rounds"]
    for r in range(rounds):
        run.round = r
        run.inspection = pick_inspection(run)
        if log:
            insp = f", inspection: {t['inspections'][run.inspection]['name']}" if run.inspection else ""
            log(f"Round {r + 1}: target {t['targets'][r]}, ${run.money}{insp}")
        won, unused, total = play_round(run, player, log, endless=totals is not None)
        if totals is not None:
            totals.append(total)
            won, unused = True, 1
        if not won:
            return dict(won=False, lost_round=r + 1, bought=run.bought, m=run.m)
        if r == rounds - 1:
            break
        round_income(run, unused)
        offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        for o in offers:
            stats.offered[o["id"]] += 1
        player.shop(run, offers, stats)
        if run.has("spendthrift"):
            run.money = 0
    return dict(won=True, lost_round=None, bought=run.bought, m=run.m)


STYLES = ("spender", "saver", "reroller", "bargain")
FOCUSES = ("color", "straight", "safety", "quota", "adaptive")
NO_ENGINE = ("passive", "random", "flat/spender")


def make_bot(name):
    if name == "passive":
        return Player
    if name == "random":
        return RandomPlayer
    if name == "reckless":
        return Reckless
    focus, style = name.split("/")
    return lambda r: Builder(r, focus, style)


def roster():
    return list(NO_ENGINE) + ["reckless"] + [f"{f}/{s}" for s in STYLES for f in FOCUSES]


def _work(args):
    t, name, runs, totals_mode = args
    stats, out = Stats(), []
    base = 10_000 * (zlib.crc32(name.encode()) % 1000)
    for i in range(runs):
        if totals_mode:
            tot = []
            play_run(t, 7_000 + i, make_bot(name), Stats(), totals=tot)
            out.append(tot)
        else:
            res = play_run(t, base + i, make_bot(name), stats)
            out.append((res["won"], res["lost_round"], res["bought"], res["m"]))
    return name, out, stats


def run_bots(t, names, runs, totals_mode=False):
    with Pool() as pool:
        return pool.map(_work, [(t, n, runs, totals_mode) for n in names])


def report(t, runs):
    res = {name: (out, st) for name, out, st in run_bots(t, roster(), runs)}
    rounds = t["run"]["rounds"]
    lines = [f"{runs} runs per bot, {rounds} rounds, targets {t['targets']}", "",
             f"{'bot':<20}{'win':>7}{'lose r1':>9}  losses by round"]
    win, r1 = {}, {}
    stats, presence, wins_total = Stats(), Counter(), 0
    m = Counter()
    for name, (out, st) in res.items():
        win[name] = sum(1 for o in out if o[0]) / runs
        r1[name] = sum(1 for o in out if o[1] == 1) / runs
        by = Counter(o[1] for o in out if not o[0])
        lines.append(f"{name:<20}{win[name]:>7.1%}{r1[name]:>9.1%}  " + " ".join(f"{by.get(i, 0):>3}" for i in range(1, rounds + 1)))
        if name not in NO_ENGINE and name != "reckless":
            stats.merge(st)
            for o in out:
                m.update(o[3])
                if o[0]:
                    wins_total += 1
                    presence.update(o[2])
    lines.append("")
    engines = {n: w for n, w in win.items() if n not in NO_ENGINE and n != "reckless"}
    checks = []

    def check(cid, label, ok, detail):
        checks.append((cid, label, ok, detail))

    worst_r1 = max(v for n, v in r1.items() if n != "reckless")
    check("A1", "Round 1 is almost impossible to lose (every non-reckless bot <= 1%)", worst_r1 <= 0.01,
          f"worst bot loses round 1 {worst_r1:.1%}")
    check("A2", "An insane risk can lose round 1 (reckless >= 50%)", r1["reckless"] >= 0.5,
          f"reckless loses round 1 {r1['reckless']:.1%}")
    ne = max(win[n] for n in NO_ENGINE)
    check("B1", "Winning without a point factory is almost impossible (no-engine bots <= 3%)", ne <= 0.03,
          f"best no-engine bot wins {ne:.1%}")
    best = max(engines.values())
    check("B2", "A point factory can win (best engine bot >= 40%)", best >= 0.4,
          f"best engine bot {max(engines, key=engines.get)} wins {best:.1%}")
    check("B3", "A point factory is still risky (no bot wins >= 90%)", best < 0.9, f"best engine bot wins {best:.1%}")
    fams = defaultdict(list)
    styles = defaultdict(list)
    for n, w in engines.items():
        fams[n.split("/")[0]].append(w)
        styles[n.split("/")[1]].append(w)
    viable = [f for f, ws in fams.items() if max(ws) >= 0.5 * best and best > 0]
    check("C1", "More than one kind of point factory is viable (>= 3 focuses within half the best)", len(viable) >= 3,
          ", ".join(f"{f} {max(ws):.0%}" for f, ws in fams.items()))
    smean = {s: sum(ws) / len(ws) for s, ws in styles.items()}
    top, median = max(smean.values()), sorted(smean.values())[len(smean) // 2]
    check("C2", "No spending strategy dominates (best style's mean <= 1.5x the median style's)",
          top > 0 and top <= 1.5 * median, ", ".join(f"{s} {v:.0%}" for s, v in smean.items()))
    losses = Counter()
    for n in engines:
        losses.update(o[1] for o in res[n][0] if not o[0])
    worst_round, worst = losses.most_common(1)[0] if losses else (None, 0)
    share = worst / max(1, sum(losses.values()))
    check("D1", "No cliff: no single round causes > 35% of engine-bot losses", share <= 0.35,
          f"round {worst_round} causes {share:.0%} of losses")
    choice = stats.shops_with_choice / max(1, stats.shops_with_fit)
    check("E1", "Shopping isn't forced: of shops with a good fit, >= 50% have two or more", choice >= 0.5, f"{choice:.0%}")
    held = Counter({c: n for c, n in presence.items() if c in t["items"] or c in t["wheels"]})
    top_id, top_n = held.most_common(1)[0] if held else ("-", 0)
    check("E2", "No must-have: no item or wheel is bought in > 80% of winning runs", top_n / max(1, wins_total) <= 0.8,
          f"{top_id} in {top_n / max(1, wins_total):.0%} of {wins_total} wins")
    lines.append("Rubric (docs/design/session-rubric.md)")
    for cid, label, ok, detail in checks:
        lines.append(f"  [{'PASS' if ok else 'FAIL'}] {cid} {label}: {detail}")
    loads = m["deliveries"] + m["spoiled"]
    press = dict(cargo_per_delivery=m["delivered_cargo"] / max(1, m["deliveries"]),
                 spoiled=m["spoiled"] / max(1, loads), quota=m["quota_hits"] / max(1, m["deliveries"]))
    lines += ["", f"Engine bots' loads: {press['cargo_per_delivery']:.1f} cargo per delivery, "
                  f"{press['spoiled']:.0%} of loads spoiled, {press['quota']:.0%} of deliveries reach the quota", "",
              "Telemetry, engine bots: pick rate = picked / offered; win presence = share of wins that bought it",
              f"  {'content':<16}{'offered':>8}{'pick':>7}{'win pres':>10}"]
    for cid in sorted(stats.offered, key=lambda c: -presence[c]):
        off = stats.offered[cid]
        lines.append(f"  {cid:<16}{off:>8}{stats.picked[cid] / off:>7.0%}{presence[cid] / max(1, wins_total):>10.0%}")
    lines += ["", f"{sum(ok for _, _, ok, _ in checks)}/{len(checks)} rubric checks pass"]
    return "\n".join(lines), checks, dict(win=win, r1=r1, press=press, best=best, ne=ne)


def scaled_targets(t, scale):
    n = len(t["targets"])
    return [int(v * scale ** (r / (n - 1))) for r, v in enumerate(t["targets"])]


def calibrate(t, runs, wall=0.02):
    """Scale the target curve (not round 1) until the best no-engine bot wins about `wall` of runs."""
    lo, hi = 0.05, 40.0
    for _ in range(9):
        mid = (lo * hi) ** 0.5
        tt = copy.deepcopy(t)
        tt["targets"] = scaled_targets(t, mid)
        best = 0
        for name, out, _ in run_bots(tt, ["flat/spender", "random", "passive"], runs):
            best = max(best, sum(1 for o in out if o[0]) / runs)
        if best > wall:
            lo = mid
        else:
            hi = mid
    return hi


def capacity(t, runs):
    names = ["passive", "random", "flat/spender"] + [f"{f}/spender" for f in FOCUSES] + ["safety/saver", "quota/reroller"]
    out = ["Median round total when no one can lose (all fuel spent), by round",
           f"{'bot':<18}" + "".join(f"{r + 1:>8}" for r in range(t["run"]["rounds"]))]
    for name, tots, _ in run_bots(t, names, runs, totals_mode=True):
        cols = list(zip(*tots))
        for q, tag in ((0.5, "p50"), (0.9, "p90")):
            out.append(f"{name + ' ' + tag:<18}" + "".join(f"{sorted(c)[int(q * (len(c) - 1))]:>8}" for c in cols))
    out.append(f"{'targets':<18}" + "".join(f"{v:>8}" for v in t["targets"]))
    return "\n".join(out)


EXPERIMENTS = [
    ("baseline: 4 trips, tank 10, quota x2", []),
    ("3 trips", ["round.trips=3"]),
    ("5 trips", ["round.trips=5"]),
    ("tank 7", ["round.tank=7"]),
    ("tank 14", ["round.tank=14"]),
    ("one pool of 10 fuel, no trips", ["round.mode=fuel"]),
    ("deal: player places wedges", ["round.deal=assign"]),
    ("no quota bonus", ["scoring.quota_bonus=1"]),
    ("quotas 25% higher", ["scoring.quotas=[12,15,16,19,21,24,26,29,32,36,40,45]"]),
    ("quotas 50% higher", ["scoring.quotas=[15,18,20,22,26,28,32,34,39,44,48,54]"]),
    ("quota bonus x3", ["scoring.quota_bonus=3"]),
    ("truck holds 5 values", ["scoring.truck_size=5"]),
    ("truck holds 7 values", ["scoring.truck_size=7"]),
    ("no full-truck bonus", ["scoring.full_bonus=1"]),
    ("no inspections", ["round.inspections=false"]),
]


def experiments(runs):
    print(f"Each variant's targets are scaled so no-engine bots win about 2%; then {runs} runs per bot.\n")
    print(f"{'variant':<40}{'scale':>6}{'pass':>5}{'engine':>8}{'no-eng':>7}{'reck r1':>8}{'worst r1':>9}"
          f"{'cargo':>6}{'spoil':>6}{'quota':>6}  fails")
    for label, ov in EXPERIMENTS:
        t = load_tables(ov)
        scale = calibrate(t, max(100, runs // 2))
        t["targets"] = scaled_targets(t, scale)
        _, checks, d = report(t, runs)
        worst = max(v for n, v in d["r1"].items() if n != "reckless")
        fails = ",".join(c for c, _, ok, _ in checks if not ok)
        print(f"{label:<40}{scale:>6.2f}{sum(ok for _, _, ok, _ in checks):>5}{d['best']:>8.0%}{d['ne']:>7.1%}"
              f"{d['r1']['reckless']:>8.0%}{worst:>9.1%}{d['press']['cargo_per_delivery']:>6.1f}"
              f"{d['press']['spoiled']:>6.0%}{d['press']['quota']:>6.0%}  {fails}", flush=True)


# ---------------------------------------------------------------- play by hand

def describe(it):
    e, a = it["effect"], it.get("amount")
    return {
        "flat_points": f"+{a} points", "flat_mult": f"+{a} mult",
        "mult_per_color": f"+{a} mult per {COLORS[it.get('color', 0)]} cargo",
        "points_per_high": f"+{a} points per cargo worth {it.get('threshold')}+",
        "mult_per_parity": f"+{a} mult per {'odd' if it.get('parity') else 'even'} cargo",
        "mult_if_flush": f"+{a} mult with {it.get('min')}+ of a color",
        "mult_if_straight": f"+{a} mult with a run of {it.get('min')}+ values",
        "xmult_if_flush": f"x{a} with {it.get('min')}+ of a color",
        "xmult_if_straight": f"x{a} with a run of {it.get('min')}+ values",
        "mult_per_cargo_past": f"+{a} mult per cargo past the {it.get('past')}rd",
        "xmult_if_size": f"x{a} for loads of {it.get('min')}+ cargo",
        "mult_per_risky": f"+{a} mult per spin taken at 25%+ to spoil",
        "xmult_per_flush_past": f"x(1 + {a} per cargo past 2 in your biggest color group)",
        "xmult_if_covered": f"x{a} if a tarp saved this load",
        "scaling_mult_size": f"gains +{a} mult for good per delivery of {it.get('min')}+ cargo",
        "scaling_xmult_quota": f"gains x{a} for good per delivery that reaches the quota",
        "quota_bonus": f"the quota pays x{a}", "full_bonus": f"a full truck pays x{a}",
        "spare_tarp": "one free tarp each round", "double_clutch": "the first duplicate in each load is thrown out",
        "fuel": f"+{a} fuel each round", "insurance": f"a spoiled load still pays {int(a * 100)}%",
        "tip": f"+${a} per delivery that reaches the quota",
        "rod": "after a delivery, may remove one delivered wedge from the bag",
        "piggy": f"+{int(it.get('rate', 0) * 100)}% interest on money held (max ${it.get('cap')})",
        "spendthrift": f"+{int(it.get('rate', 0) * 100)}% of money held each round; unspent money is lost after the shop",
    }.get(e, e)


class Human(Player):
    name = "human"

    def choose(self, run, loads, L, total, target, fuel):
        now = score(run, L) if L.cargo else 0
        print(f"\n  truck: {L.label()}  tarps {L.tarps + run.spare}  crates {L.crates}  worth {now} now"
              f" | round {total}/{target} | quota {run.quota} | {run.hud}")
        for wi, (wheel, wedges) in enumerate(zip(run.wheels, loads)):
            p = spoil_chance(run, wheel, wedges, L)
            print(f"   {wi}: {wheel['name']:<14} {p:>4.0%} to spoil   "
                  + " ".join(f"[{w.label()}]" if clashes(run, wheel, w, L) else f" {w.label()} " for w in wedges))
        while True:
            s = input("  spin which wheel, or d to deliver? ").strip().lower()
            if s == "d" and L.cargo:
                return None
            if s.isdigit() and int(s) < len(run.wheels):
                return int(s)

    def use_fix(self, run, L):
        return input(f"  Duplicate! Use {' or '.join(c for c in run.consumables if c != 'band')} to save the load? y/n ").strip().lower() == "y" \
            if any(c != "band" for c in run.consumables) else False

    def use_band(self, run, L, p):
        return input(f"  Use the Outer Band on this spin ({p:.0%} to spoil)? y/n ").strip().lower() == "y"

    def zap_choice(self, run, landed):
        if not landed:
            return None
        print("  The Rod can remove: " + " ".join(f"{i}:{w.label()}" for i, w in enumerate(landed)))
        s = input("  remove which (Enter to skip)? ").strip()
        return landed[int(s)] if s.isdigit() and int(s) < len(landed) else None


def show(run):
    print(f"  ${run.money} | wheels: " + ", ".join(f"{i}:{w['name']}({w['size']})" for i, w in enumerate(run.wheels)))
    print("  items: " + (", ".join(f"{i}:{it['name']} ({describe(it)})" for i, it in enumerate(run.items)) or "none")
          + " | consumables: " + (", ".join(run.consumables) or "none"))
    bag = Counter(w.label() for w in run.bag)
    print(f"  bag ({len(run.bag)} wedges, wheels hold {run.slots()}): " + " ".join(f"{k}x{v}" if v > 1 else k for k, v in sorted(bag.items())))


def play(seed):
    t = load_tables()
    rng = random.Random(seed)
    run = Run(t, rng)
    human = Human(rng)
    print(f"Spintacular haul prototype, seed {seed}. Cargo is color+value; * is wild, + is stamped, __ is blank.")
    print("No repeats: a wedge that lands twice in a trip, or a cargo value already in the truck, spoils the load.")
    print("Wedges in [brackets] would spoil it. Deliver before that happens.")
    for r in range(t["run"]["rounds"]):
        run.round = r
        run.inspection = pick_inspection(run)
        print(f"\n== Round {r + 1}/{t['run']['rounds']}: score {t['targets'][r]}"
              + (f" | INSPECTION {t['inspections'][run.inspection]['name']}: {t['inspections'][run.inspection]['text']}" if run.inspection else ""))
        show(run)
        won, unused, total = play_round(run, human, log=print)
        if not won:
            print(f"\nRun over: {total} of {t['targets'][r]} in round {r + 1}.")
            return
        if r == t["run"]["rounds"] - 1:
            print("\nYou win the run!")
            return
        print(f"  Round won; +${round_income(run, unused)}")
        offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        reroll = t["shop"]["reroll_base"]
        while True:
            print("\n  SHOP")
            show(run)
            for i, o in enumerate(offers):
                if not o.get("sold"):
                    d = t.get(o["kind"] + "s", {}).get(o["id"], {})
                    print(f"   {i}: {o['name']} ({o['rarity']}) ${o['price']}  {describe(d) if o['kind'] == 'item' else d.get('text', '')}")
            cmd = input(f"  buy N | sell N | order 2,0,1 | reroll (${reroll}) | done: ").strip().split()
            if not cmd or cmd[0] == "done":
                break
            if cmd[0] == "buy" and len(cmd) > 1 and cmd[1].isdigit() and int(cmd[1]) < len(offers):
                o = offers[int(cmd[1])]
                if o.get("sold") or o["price"] > run.money or not can_take(run, o):
                    print("  can't buy that")
                    continue
                wedge = choice = None
                if o["kind"] in ("upgrade", "zap"):
                    print("   " + "  ".join(f"{i}:{w.label()}" for i, w in enumerate(run.bag)))
                    s = input("  which wedge? ").strip()
                    if not s.isdigit() or int(s) >= len(run.bag):
                        continue
                    wedge = run.bag[int(s)]
                    if o["id"] == "paint":
                        c = input("  which color R/G/B/Y? ").strip().upper()
                        if c not in COLORS or not c:
                            continue
                        choice = COLORS.index(c)
                    elif o["id"] == "recut":
                        v = input("  which value 1-6? ").strip()
                        if v not in list("123456"):
                            continue
                        choice = int(v)
                apply_offer(run, o, wedge, choice)
                o["sold"] = True
            elif cmd[0] == "sell" and len(cmd) > 1 and cmd[1].isdigit() and int(cmd[1]) < len(run.items):
                run.money += sell_value(run, run.items.pop(int(cmd[1])))
            elif cmd[0] == "order" and len(cmd) > 1:
                idx = [int(x) for x in cmd[1].split(",")]
                if sorted(idx) == list(range(len(run.items))):
                    run.items = [run.items[i] for i in idx]
            elif cmd[0] == "reroll" and run.money >= reroll:
                run.money -= reroll
                reroll += t["shop"]["reroll_step"]
                offers = [roll_offer(run) for _ in range(t["shop"]["offers"])]
        if run.has("spendthrift"):
            run.money = 0


if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "sim"
    nums = [a for a in sys.argv[2:] if a.isdigit()]
    ovs = [a for a in sys.argv[2:] if "=" in a]
    n = int(nums[0]) if nums else None
    if mode == "play":
        play(n if n is not None else random.randrange(10 ** 6))
    elif mode == "capacity":
        print(capacity(load_tables(ovs), n or 200))
    elif mode == "experiments":
        experiments(n or 300)
    elif mode == "calibrate":
        tt = load_tables(ovs)
        sc = calibrate(tt, max(100, (n or 300) // 2))
        tt["targets"] = scaled_targets(tt, sc)
        print(f"target scale {sc:.2f}")
        print(report(tt, n or 300)[0])
    else:
        text, checks, _ = report(load_tables(ovs), n or 300)
        print(text)
        sys.exit(0 if all(ok for _, _, ok, _ in checks) else 1)
