#include "bots.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace spin::sim {

namespace {

int slots(const Run& run) {
    int n = 0;
    for (int wi = 0; wi < static_cast<int>(run.wheels.size()); ++wi) n += run.wheel_def(wi).size;
    return n;
}

const Wedge* bag_wedge(const Run& run, int uid) {
    for (auto& w : run.bag)
        if (w.uid == uid) return &w;
    return nullptr;
}

// How many cargo wedges in the bag have each value.
std::map<int, int> value_counts(const Run& run) {
    std::map<int, int> counts;
    for (auto& w : run.bag)
        if (w.kind == WedgeKind::Cargo) ++counts[w.value];
    return counts;
}

int count_of(const std::map<int, int>& counts, int value) {
    auto it = counts.find(value);
    return it == counts.end() ? 0 : it->second;
}

// How well an item's effect fits each build; `other` is for every build not named.
// A negative entry means "use other".
struct FitRow {
    double color = -1, straight = -1, safety = -1, quota = -1, other = 1.0;
};

FitRow fit_row(ItemEffect e) {
    FitRow r;
    switch (e) {
    case ItemEffect::FlatPoints:
    case ItemEffect::FlatMult: r.other = 1.0; break;
    case ItemEffect::MultPerParity: r.other = 0.8; break;
    case ItemEffect::PointsPerHigh: r.quota = 3.0, r.other = 1.0; break;
    case ItemEffect::MultPerColor: r.other = 0.5; break;
    case ItemEffect::MultIfFlush: r.color = 3.0, r.other = 0.8; break;
    case ItemEffect::XmultIfFlush: r.color = 4.5, r.other = 0.5; break;
    case ItemEffect::XmultPerFlushPast: r.color = 5.0, r.other = 0.5; break;
    case ItemEffect::MultIfStraight: r.straight = 3.0, r.other = 1.0; break;
    case ItemEffect::XmultIfStraight: r.straight = 4.5, r.other = 0.8; break;
    case ItemEffect::SpareTarp: r.safety = 4.5, r.other = 3.0; break;
    case ItemEffect::DoubleClutch: r.safety = 5.0, r.other = 4.0; break;
    case ItemEffect::MultPerCargoPast:
    case ItemEffect::XmultIfSize: r.safety = 4.5, r.straight = 3.5, r.other = 2.5; break;
    case ItemEffect::ScalingMultSize: r.safety = 5.0, r.straight = 3.5, r.other = 2.5; break;
    case ItemEffect::Insurance: r.safety = 2.5, r.other = 2.0; break;
    case ItemEffect::XmultIfCovered: r.safety = 4.0, r.other = 1.5; break;
    case ItemEffect::MultPerRisky: r.safety = 2.0, r.other = 2.5; break;
    case ItemEffect::QuotaBonus: r.quota = 5.0, r.other = 3.0; break;
    case ItemEffect::ScalingXmultQuota: r.quota = 5.0, r.other = 3.5; break;
    case ItemEffect::Tip: r.quota = 2.5, r.other = 1.0; break;
    case ItemEffect::FullBonus: r.safety = 4.0, r.straight = 4.0, r.other = 2.0; break;
    case ItemEffect::Fuel: r.other = 3.5; break;
    case ItemEffect::Rod: r.other = 2.0; break;
    default: break;
    }
    return r;
}

// What an item is worth to a bot with no engine: only what adds points now.
double flat_fit(ItemEffect e) {
    switch (e) {
    case ItemEffect::FlatPoints:
    case ItemEffect::FlatMult: return 1.5;
    case ItemEffect::MultPerParity:
    case ItemEffect::PointsPerHigh:
    case ItemEffect::MultPerColor: return 1.2;
    case ItemEffect::Fuel: return 1.0;
    default: return 0;
    }
}

} // namespace

// ---------------------------------------------------------------- stats

void Stats::merge(const Stats& o) {
    for (auto& [id, n] : o.offered) offered[id] += n;
    for (auto& [id, n] : o.picked) picked[id] += n;
    shops += o.shops;
    shops_with_fit += o.shops_with_fit;
    shops_with_choice += o.shops_with_choice;
}

void Stats::count_offers(const Run& run) {
    for (auto& o : run.offers) ++offered[o.id];
}

// ---------------------------------------------------------------- the passive bot

bool Bot::use_fix(const Run& run) const { return run.load.cargo.size() >= 2; }

bool Bot::use_band(const Run& run, double p) const { return p >= 0.25 && run.load.cargo.size() >= 3; }

int Bot::zap_choice(Run&, const std::vector<int>&) { return 0; }

void Bot::shop(Run&, Stats&) {}

int Bot::choose(Run& run) {
    const bool loaded = !run.load.cargo.empty();
    const double now = loaded ? static_cast<double>(run.score_now().total) : 0;
    const double need = static_cast<double>(run.target() - run.round_score);
    const auto& inc = run.t.income;
    const bool free_money = inc.freight_bonus > 0 && static_cast<int>(run.load.cargo.size()) >= inc.freight_past;
    if (loaded && now >= need && caution > 0 && !free_money) return -1;
    const double ins = run.sum_amount(ItemEffect::Insurance);
    // The expected value of the load after one more spin of each wheel.
    Load next = run.load;
    int best = -1;
    double best_ev = -1.0, best_p = 0.0;
    for (int wi = 0; wi < static_cast<int>(run.deal.size()); ++wi) {
        const auto& wedges = run.deal[wi];
        if (wedges.empty()) continue;
        const bool safe = run.covered(wi);
        double ev = 0, p = 0;
        for (auto& w : wedges) {
            if (run.clashes(wi, w)) {
                if (safe) {
                    ev += now;
                } else {
                    p += 1;
                    ev += ins * now;
                }
            } else if (w.kind == WedgeKind::Blank || w.kind == WedgeKind::Tarp) {
                ev += now;
            } else if (w.kind == WedgeKind::Crate) {
                if (loaded) {
                    ++next.crates;
                    ev += static_cast<double>(run.score(next).total);
                    --next.crates;
                }
            } else {
                next.cargo.push_back(run.cargo_from(wi, w));
                ev += static_cast<double>(run.score(next).total);
                next.cargo.pop_back();
            }
        }
        ev /= static_cast<double>(wedges.size());
        p /= static_cast<double>(wedges.size());
        if (ev > best_ev + 1e-9 || (std::fabs(ev - best_ev) <= 1e-9 && p < best_p)) {
            best = wi;
            best_ev = ev;
            best_p = p;
        }
    }
    if (!loaded || caution == 0) return best;
    if (now >= need) // the round is won on delivery: press on only for the freight bonus
        return best_p <= freight_risk ? best : -1;
    // Delivering doesn't free up spins, so the load is all there is to weigh.
    return best_ev > caution * now ? best : -1;
}

void Bot::act(Run& run) {
    switch (run.phase) {
    case Phase::Trip: {
        int wheel = run.can_spin() ? choose(run) : -1;
        if (wheel < 0 && run.deliver().ok) return;
        if (wheel < 0) wheel = 0; // nothing to deliver yet: the trip goes on
        const double p = run.spoil_chance(wheel);
        const bool band = p > 0 && run.count_consumable("band") > 0 && use_band(run, p);
        run.spin(wheel, band);
        return;
    }
    case Phase::Rescue: {
        // A Nudge first, when a neighbor is safe; else a Respin.
        Rescue how = Rescue::Decline;
        if (!run.load.cargo.empty() && use_fix(run)) {
            if (run.rescue_allowed(Rescue::NudgeLeft))
                how = Rescue::NudgeLeft;
            else if (run.rescue_allowed(Rescue::NudgeRight))
                how = Rescue::NudgeRight;
            else if (run.rescue_allowed(Rescue::Respin))
                how = Rescue::Respin;
        }
        run.rescue(how);
        // One try for each repeat, as in the prototype: a Respin that lands on another repeat loses the load.
        if (how == Rescue::Respin && run.phase == Phase::Rescue) run.rescue(Rescue::Decline);
        return;
    }
    case Phase::Rod: {
        int uid = zap_choice(run, run.delivered_uids);
        if (!run.rod_remove(uid)) run.rod_remove(0);
        return;
    }
    default: return;
    }
}

// ---------------------------------------------------------------- the random bot

RandomBot::RandomBot(Rng rng) : Bot(rng) { name = "random"; }

void RandomBot::shop(Run& run, Stats& stats) {
    for (int i = 0; i < static_cast<int>(run.offers.size()); ++i) {
        const Offer& o = run.offers[i];
        if (rng.unit() < 0.5 && o.price <= run.money && run.can_take(o)) {
            const int uid = run.bag[rng.below(static_cast<int>(run.bag.size()))].uid;
            const int choice = o.id == "paint" ? rng.below(4) : 1 + rng.below(6);
            const std::string id = o.id;
            if (run.buy(i, uid, choice)) ++stats.picked[id];
        }
    }
    // Shuffle the items: the order they are applied in is part of what it gets wrong.
    for (int i = static_cast<int>(run.items.size()) - 1; i > 0; --i) {
        int j = rng.below(i + 1);
        if (i == j) continue;
        run.move_item(i, j);     // i's item to j; j's old item is now at j + 1
        run.move_item(j + 1, i); // and that one goes to i
    }
}

// ---------------------------------------------------------------- the builders

const char* focus_name(Focus f) {
    switch (f) {
    case Focus::Color: return "color";
    case Focus::Straight: return "straight";
    case Focus::Safety: return "safety";
    case Focus::Quota: return "quota";
    case Focus::Adaptive: return "adaptive";
    default: return "flat";
    }
}

const char* style_name(Style s) {
    switch (s) {
    case Style::Spender: return "spender";
    case Style::Saver: return "saver";
    case Style::Reroller: return "reroller";
    default: return "bargain";
    }
}

Builder::Builder(Rng r, Focus focus, Style style) : Bot(r), focus(focus), style(style), color(0) {
    color = rng.below(4);
    name = std::string(focus_name(focus)) + "/" + style_name(style);
}

Focus Builder::kind(const Run& run) const {
    if (focus != Focus::Adaptive) return focus;
    // Follow what the shop has offered: the build the items held fit best, once they fit one at all.
    static const Focus kinds[] = {Focus::Color, Focus::Straight, Focus::Safety, Focus::Quota};
    Focus best = Focus::Adaptive;
    double best_fit = 0;
    for (Focus k : kinds) {
        double sum = 0;
        for (auto& it : run.items)
            if (const ItemDef* d = run.t.item(it.id)) sum += item_fit(*d, k);
        if (sum > best_fit) {
            best = k;
            best_fit = sum;
        }
    }
    return !run.items.empty() && best_fit >= 3 ? best : Focus::Adaptive;
}

double Builder::item_fit(const ItemDef& d, Focus kind) const {
    const ItemEffect e = d.effect;
    if (kind == Focus::Flat) return flat_fit(e);
    if (e == ItemEffect::Piggy) return style == Style::Saver || style == Style::Bargain ? 2.5 : 0.5;
    if (e == ItemEffect::Spendthrift) return style == Style::Spender || style == Style::Reroller ? 2.5 : 0.3;
    if (e == ItemEffect::MultPerColor) return kind == Focus::Color && d.color == color ? 3.0 : 0.5;
    const FitRow row = fit_row(e);
    double v = -1;
    switch (kind) {
    case Focus::Color: v = row.color; break;
    case Focus::Straight: v = row.straight; break;
    case Focus::Safety: v = row.safety; break;
    case Focus::Quota: v = row.quota; break;
    default: break;
    }
    return v >= 0 ? v : row.other;
}

bool Builder::wedge_ok(const Run& run, const Wedge& w, Focus kind) const {
    if (w.kind != WedgeKind::Cargo) return true;
    if (kind == Focus::Color) return w.color == kWild || w.color == color;
    if (kind == Focus::Flat) return w.value >= 3;
    // The other builds want every value equally often: the fewer repeats, the longer the load.
    const auto counts = value_counts(run);
    int fewest = 0;
    for (auto& [value, n] : counts) fewest = fewest == 0 ? n : std::min(fewest, n);
    return count_of(counts, w.value) <= std::max(1, fewest);
}

double Builder::fit(const Run& run, const Offer& o, Focus kind) const {
    if (!run.can_take(o)) return 0;
    const bool engine = kind != Focus::Flat;
    switch (o.kind) {
    case OfferKind::Item: {
        const ItemDef* d = run.t.item(o.id);
        return d ? item_fit(*d, kind) : 0;
    }
    case OfferKind::Wheel: {
        const WheelDef* d = run.t.wheel(o.id);
        if (!d) return 0;
        switch (d->effect) {
        case WheelEffect::None: return 2.0;
        case WheelEffect::Gilded: return kind == Focus::Quota || kind == Focus::Flat ? 2.5 : 1.5;
        case WheelEffect::Bump: return !engine ? 0 : kind == Focus::Quota ? 3.0 : 1.5;
        case WheelEffect::Express: return engine ? 3.0 : 0;
        case WheelEffect::Padded: return engine ? 4.5 : 0;
        }
        return 0;
    }
    case OfferKind::Upgrade:
        if (!engine) return 0;
        if (o.id == "recut") return kind == Focus::Straight || kind == Focus::Safety || kind == Focus::Adaptive ? 2.0 : 0.8;
        if (o.id == "paint") return kind == Focus::Color ? 3.0 : 0.2;
        if (o.id == "premium") return kind == Focus::Quota || kind == Focus::Safety || kind == Focus::Straight ? 3.5 : 2.5;
        if (o.id == "wild") return kind == Focus::Color ? 3.0 : 0.3;
        if (o.id == "stamp") return kind == Focus::Quota ? 3.5 : 2.0;
        return 0;
    case OfferKind::Wedge:
        if (o.wedge != WedgeKind::Cargo) return !engine ? 0 : o.wedge == WedgeKind::Tarp ? 3.5 : 3.0;
        if (kind == Focus::Color) return o.color == color ? 2.5 : 0;
        return o.value >= 4 ? 1.0 : 0.3;
    case OfferKind::Zap: {
        // Worth it only to thin a bag that has more wedges than the wheels hold.
        if (!engine || kind == Focus::Adaptive || static_cast<int>(run.bag.size()) <= slots(run)) return 0;
        for (auto& w : run.bag)
            if (!wedge_ok(run, w, kind)) return 2.5;
        return 0;
    }
    case OfferKind::Consumable: return 1.5;
    }
    return 0;
}

void Builder::target_for(const Run& run, const Offer& o, Focus kind, int& wedge_uid, int& choice) {
    wedge_uid = 0;
    choice = 0;
    std::vector<const Wedge*> cargo;
    for (auto& w : run.bag)
        if (w.kind == WedgeKind::Cargo) cargo.push_back(&w);
    const auto counts = value_counts(run);
    // The first wedge with the highest key, as Python's max() picks.
    auto first_max = [](const std::vector<const Wedge*>& pool, auto key) {
        const Wedge* best = nullptr;
        for (const Wedge* w : pool)
            if (!best || key(*best) < key(*w)) best = w;
        return best;
    };
    auto pick = [&](const std::vector<const Wedge*>& pool) {
        return pool.empty() ? nullptr : pool[rng.below(static_cast<int>(pool.size()))];
    };
    const Wedge* w = nullptr;
    if (o.kind == OfferKind::Zap) {
        // The off-build wedge whose value repeats most.
        std::vector<const Wedge*> off;
        for (const Wedge* c : cargo)
            if (!wedge_ok(run, *c, kind)) off.push_back(c);
        w = first_max(off, [&](const Wedge& x) { return count_of(counts, x.value); });
    } else if (o.id == "paint" || o.id == "wild") {
        std::vector<const Wedge*> pool;
        for (const Wedge* c : cargo)
            if (c->color != kWild && c->color != color) pool.push_back(c);
        if (o.id == "wild" && pool.empty())
            for (const Wedge* c : cargo)
                if (c->color != kWild) pool.push_back(c);
        w = pick(pool);
        if (o.id == "paint") choice = color;
    } else if (o.id == "recut") {
        // The most repeated value becomes the rarest one; the higher on a tie.
        w = first_max(cargo, [&](const Wedge& x) { return count_of(counts, x.value); });
        choice = 1;
        for (int v = 2; v <= 6; ++v)
            if (count_of(counts, v) <= count_of(counts, choice)) choice = v;
    } else if (o.id == "premium") {
        // Raise a value that repeats into one that doesn't.
        std::vector<const Wedge*> pool;
        for (const Wedge* c : cargo)
            if (c->value < run.t.run.max_value) pool.push_back(c);
        w = first_max(pool, [&](const Wedge& x) {
            return std::make_pair(count_of(counts, x.value) - count_of(counts, x.value + 1), x.value);
        });
    } else if (o.id == "stamp") {
        w = first_max(cargo, [](const Wedge& x) { return std::make_pair(-x.bonus, x.value); });
    }
    wedge_uid = w ? w->uid : 0;
}

bool Builder::wants(const Run& run, const Offer& o, double f) const {
    const int left = run.money - o.price;
    if (style == Style::Saver) return f >= 3.0 || left >= std::min(25, 5 * run.round);
    if (style == Style::Bargain) return o.price <= 3 || f / o.price >= 0.5;
    return true;
}

void Builder::shop(Run& run, Stats& stats) {
    const Focus kind = this->kind(run);
    int rerolls = 0;
    for (;;) {
        // The offers, best fit first.
        std::vector<std::pair<double, int>> scored;
        for (int i = 0; i < static_cast<int>(run.offers.size()); ++i) scored.push_back({fit(run, run.offers[i], kind), i});
        std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        int good = 0;
        for (auto& s : scored) good += s.first >= 2.0;
        ++stats.shops;
        stats.shops_with_fit += good >= 1;
        stats.shops_with_choice += good >= 2;
        for (auto& [f, i] : scored) {
            const Offer o = run.offers[i];
            if (o.sold || f < 1.0 || !wants(run, o, f)) continue;
            if (o.kind == OfferKind::Item && static_cast<int>(run.items.size()) >= run.t.run.max_items) {
                // No room: sell the weakest item if this one is clearly better.
                auto fit_of = [&](const OwnedItem& it) {
                    const ItemDef* d = run.t.item(it.id);
                    return d ? item_fit(*d, kind) : 0.0;
                };
                int weakest = 0;
                for (int k = 1; k < static_cast<int>(run.items.size()); ++k)
                    if (fit_of(run.items[k]) < fit_of(run.items[weakest])) weakest = k;
                if (!run.items.empty() && fit_of(run.items[weakest]) < f - 1.5 &&
                    run.money + run.sell_value(run.items[weakest]) >= o.price)
                    run.sell_item(weakest);
            }
            if (o.price > run.money || !run.can_take(o)) continue;
            int uid = 0, choice = 0;
            if (o.kind == OfferKind::Upgrade || o.kind == OfferKind::Zap) {
                target_for(run, o, kind, uid, choice);
                if (uid == 0) continue;
            }
            if (run.buy(i, uid, choice)) ++stats.picked[o.id];
        }
        // A reroller pays for a new shop when nothing great is left in this one.
        bool find_left = false;
        for (auto& [f, i] : scored) find_left = find_left || (f >= 3 && !run.offers[i].sold);
        if (style == Style::Reroller && rerolls < 3 && run.money >= run.reroll_price + 4 && !find_left && run.reroll()) {
            ++rerolls;
            stats.count_offers(run);
            continue;
        }
        break;
    }
    // Additive items before multipliers: the order that pays most.
    int front = 0;
    for (int i = 0; i < static_cast<int>(run.items.size()); ++i) {
        const ItemDef* d = run.t.item(run.items[i].id);
        if (d && is_additive(d->effect)) run.move_item(i, front++);
    }
}

int Builder::zap_choice(Run& run, const std::vector<int>& delivered_uids) {
    const Focus kind = this->kind(run);
    if (static_cast<int>(run.bag.size()) <= slots(run) || kind == Focus::Flat || kind == Focus::Adaptive) return 0;
    for (int uid : delivered_uids)
        if (const Wedge* w = bag_wedge(run, uid); w && !wedge_ok(run, *w, kind)) return uid;
    return 0;
}

Reckless::Reckless(Rng rng) : Builder(rng, Focus::Safety, Style::Spender) {
    caution = 0;
    name = "reckless";
}

// ---------------------------------------------------------------- the roster

std::unique_ptr<Bot> make_bot(const std::string& name, Rng rng) {
    if (name == "passive") return std::make_unique<Bot>(rng);
    if (name == "random") return std::make_unique<RandomBot>(rng);
    if (name == "reckless") return std::make_unique<Reckless>(rng);
    static const Focus focuses[] = {Focus::Color, Focus::Straight, Focus::Safety, Focus::Quota, Focus::Adaptive, Focus::Flat};
    static const Style styles[] = {Style::Spender, Style::Saver, Style::Reroller, Style::Bargain};
    for (Focus f : focuses)
        for (Style s : styles)
            if (name == std::string(focus_name(f)) + "/" + style_name(s)) return std::make_unique<Builder>(rng, f, s);
    throw std::runtime_error("unknown bot \"" + name + "\"");
}

} // namespace spin::sim
