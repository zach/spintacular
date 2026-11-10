#include "run.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace spin {

const char* const kColorNames[4] = {"Red", "Green", "Blue", "Yellow"};
const char kColorLetters[5] = "RGBY";

namespace {

// 1.5 -> "1.5", 2.0 -> "2"
std::string num(double v) {
    char buf[32];
    if (std::fabs(v - std::round(v)) < 1e-9)
        std::snprintf(buf, sizeof buf, "%.0f", v);
    else
        std::snprintf(buf, sizeof buf, "%.2g", v);
    if (std::fabs(v) >= 100 && std::fabs(v - std::round(v)) >= 1e-9) std::snprintf(buf, sizeof buf, "%.1f", v);
    return buf;
}

int set_bonus(const std::vector<int>& table, int size) {
    if (size < 3 || table.empty()) return 0;
    int i = std::min(size, static_cast<int>(table.size()) + 2) - 3;
    return table[i];
}

bool contains(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }

} // namespace

// ---------------------------------------------------------------- setup

Run::Run(const Tables& base, const RunConfig& cfg) : t(apply_setup(base, cfg.rig, cfg.stake)), config(cfg) {
    // One seed, four streams: rerolling the shop never changes the deal or the spins.
    Rng seeder(cfg.seed);
    rng_shop = Rng(seeder.next());
    rng_deal = Rng(seeder.next());
    rng_spin = Rng(seeder.next());
    rng_route = Rng(seeder.next());
    money = t.run.start_money;
    for (auto& spec : t.start.bag) {
        Wedge w;
        w.uid = next_uid++;
        w.kind = spec.kind;
        w.color = spec.color;
        w.value = spec.value;
        bag.push_back(w);
    }
    for (auto& id : t.start.wheels) wheels.push_back({id, 0});
    round = 0;
    start_round();
}

void Run::extend_route(int upto) {
    while (static_cast<int>(route.size()) <= upto) {
        int r = static_cast<int>(route.size());
        Inspection k = Inspection::None;
        if (t.round.inspections && t.round.inspection_every > 0 && (r + 1) % t.round.inspection_every == 0 &&
            !t.inspections.empty())
            k = t.inspections[rng_route.below(static_cast<int>(t.inspections.size()))].kind;
        route.push_back(k);
    }
}

long long Run::target_of(int r) const {
    int n = static_cast<int>(t.targets.size());
    if (n == 0) return 0;
    if (r < n) return t.targets[r];
    // Endless: targets keep rising at the last round's rate.
    double ratio = n >= 2 && t.targets[n - 2] > 0 ? static_cast<double>(t.targets[n - 1]) / t.targets[n - 2] : 1.6;
    double v = static_cast<double>(t.targets[n - 1]) * std::pow(ratio, r - (n - 1));
    return static_cast<long long>(std::min(v, 9.0e17));
}

int Run::quota_of(int r) const {
    int n = static_cast<int>(t.scoring.quotas.size());
    if (n == 0) return 0;
    int q = r < n ? t.scoring.quotas[r]
                  : t.scoring.quotas[n - 1] + (r - (n - 1)) * (n >= 2 ? t.scoring.quotas[n - 1] - t.scoring.quotas[n - 2] : 4);
    if (inspection_of(r) == Inspection::QuotaUp) q = static_cast<int>(std::ceil(q * 1.25 - 1e-9));
    return q;
}

Inspection Run::inspection_of(int r) const {
    return r >= 0 && r < static_cast<int>(route.size()) ? route[r] : Inspection::None;
}

int Run::next_inspection_round() const {
    for (int r = round; r < static_cast<int>(route.size()); ++r)
        if (route[r] != Inspection::None) return r;
    return -1;
}

const WheelDef& Run::wheel_def(int wheel) const {
    static const WheelDef plain;
    const WheelDef* d = t.wheel(wheels[wheel].id);
    return d ? *d : plain;
}

bool Run::has(ItemEffect e) const {
    for (auto& it : items)
        if (const ItemDef* d = t.item(it.id); d && d->effect == e) return true;
    return false;
}

double Run::sum_amount(ItemEffect e) const {
    double s = 0;
    for (auto& it : items)
        if (const ItemDef* d = t.item(it.id); d && d->effect == e) s += d->amount;
    return s;
}

int Run::count_consumable(const std::string& id) const {
    return static_cast<int>(std::count(consumables.begin(), consumables.end(), id));
}

void Run::start_round() {
    extend_route(round + 2 * std::max(1, t.round.inspection_every));
    Inspection insp = inspection();
    tank = t.round.tank + static_cast<int>(sum_amount(ItemEffect::Fuel)) - (insp == Inspection::FuelDown ? 2 : 0);
    tank = std::max(1, tank);
    trips_left = t.round.trips;
    quota = quota_of(round);
    spare_tarps = insp == Inspection::NoTarps ? 0 : static_cast<int>(sum_amount(ItemEffect::SpareTarp));
    rods_left = static_cast<int>(sum_amount(ItemEffect::Rod));
    round_score = 0;
    load = Load();
    free_spun.clear();
    delivered_uids.clear();

    // The deal: the bag, shuffled, goes onto the wheels one wedge at a time, face up.
    std::vector<Wedge> shuffled = bag;
    rng_deal.shuffle(shuffled);
    deal.assign(wheels.size(), {});
    std::size_t k = 0;
    int widest = 0;
    for (std::size_t wi = 0; wi < wheels.size(); ++wi) widest = std::max(widest, wheel_def(static_cast<int>(wi)).size);
    for (int pass = 0; pass < widest; ++pass)
        for (std::size_t wi = 0; wi < wheels.size(); ++wi)
            if (static_cast<int>(deal[wi].size()) < wheel_def(static_cast<int>(wi)).size && k < shuffled.size())
                deal[wi].push_back(shuffled[k++]);
    // Slots the bag can't fill hold empty wedges. Each is still its own wedge.
    for (std::size_t wi = 0; wi < wheels.size(); ++wi)
        while (static_cast<int>(deal[wi].size()) < wheel_def(static_cast<int>(wi)).size) {
            Wedge blank;
            blank.uid = -(static_cast<int>(wi) * 100 + static_cast<int>(deal[wi].size()) + 1);
            deal[wi].push_back(blank);
        }
    phase = Phase::Trip;
}

// ---------------------------------------------------------------- reading the round

int Run::truck_values() const {
    std::vector<int> vals;
    for (auto& c : load.cargo)
        if (!contains(vals, c.value)) vals.push_back(c.value);
    return static_cast<int>(vals.size());
}

bool Run::truck_full() const { return truck_values() >= t.scoring.truck_size; }
bool Run::can_spin() const { return phase == Phase::Trip && load.spins < tank && !truck_full(); }
bool Run::can_deliver() const { return phase == Phase::Trip && !load.cargo.empty(); }
bool Run::must_deliver() const { return phase == Phase::Trip && (truck_full() || load.spins >= tank); }

int Run::landed_value(int wheel, const Wedge& w) const {
    return wheel_def(wheel).effect == WheelEffect::Bump ? std::min(t.run.max_value, w.value + 1) : w.value;
}

Cargo Run::cargo_from(int wheel, const Wedge& w) const {
    const WheelDef& d = wheel_def(wheel);
    return Cargo{w.color, landed_value(wheel, w), w.bonus + (d.effect == WheelEffect::Gilded ? d.amount : 0), w.uid};
}

bool Run::clashes(int wheel, const Wedge& w, const Load& l) const {
    if (contains(l.used, w.uid)) return true;
    if (w.kind != WedgeKind::Cargo) return false;
    int v = landed_value(wheel, w);
    for (auto& c : l.cargo)
        if (c.value == v) return true;
    return false;
}

bool Run::covered(int wheel, const Load& l) const {
    if (wheel_def(wheel).effect == WheelEffect::Padded) return true;
    if (has(ItemEffect::DoubleClutch) && !l.clutch_used) return true;
    return inspection() != Inspection::NoTarps && (l.tarps > 0 || spare_tarps > 0);
}

double Run::spoil_chance(int wheel, bool band) const {
    if (wheel < 0 || wheel >= static_cast<int>(deal.size()) || deal[wheel].empty()) return 0;
    if (covered(wheel)) return 0;
    double bad = 0, all = 0;
    for (auto& w : deal[wheel]) {
        bool c = clashes(wheel, w);
        double width = band && c ? 0.5 : 1.0;
        all += width;
        if (c) bad += width;
    }
    return bad / all;
}

// ---------------------------------------------------------------- scoring

Score Run::score(const Load& l, std::vector<Step>* steps, bool commit) {
    Score s;
    const int n = static_cast<int>(l.cargo.size());
    if (n == 0) return s;
    const auto& sc = t.scoring;
    double points = 0, mult = 0;
    int by_color[4] = {0, 0, 0, 0};
    int wild = 0;
    auto push = [&](Step::Kind kind, std::string label, int index = -1, double factor = 1) {
        if (steps) steps->push_back(Step{kind, std::move(label), index, points, mult, factor});
    };
    for (int i = 0; i < n; ++i) {
        const Cargo& c = l.cargo[i];
        points += c.value + c.bonus;
        if (c.color == kWild)
            ++wild;
        else if (c.color >= 0 && c.color < 4)
            ++by_color[c.color];
        std::string name = c.color == kWild ? "Wild" : kColorNames[c.color];
        push(Step::Kind::Cargo, name + " " + std::to_string(c.value) + ": +" + std::to_string(c.value + c.bonus) + " points", i);
    }
    mult = n * sc.mult_per_cargo;
    push(Step::Kind::Size, std::to_string(n) + " cargo: mult " + num(mult));
    int flush = *std::max_element(by_color, by_color + 4) + wild;
    std::vector<int> vals;
    for (auto& c : l.cargo)
        if (!contains(vals, c.value)) vals.push_back(c.value);
    std::sort(vals.begin(), vals.end());
    int straight = 1, best = 1;
    for (std::size_t i = 1; i < vals.size(); ++i) {
        best = vals[i] == vals[i - 1] + 1 ? best + 1 : 1;
        straight = std::max(straight, best);
    }
    s.flush = flush;
    s.straight = straight;
    if (flush >= 3) {
        int b = set_bonus(sc.flush, flush);
        mult += b;
        push(Step::Kind::ColorSet, std::to_string(std::min(flush, 6)) + " of a color: +" + std::to_string(b) + " mult");
    }
    if (straight >= 3) {
        int b = set_bonus(sc.straight, straight);
        mult += b;
        push(Step::Kind::RunSet, "Run of " + std::to_string(std::min(straight, 6)) + " values: +" + std::to_string(b) + " mult");
    }
    double quota_bonus = sc.quota_bonus, full_bonus = sc.full_bonus;
    for (std::size_t ii = 0; ii < items.size(); ++ii) {
        OwnedItem& it = items[ii];
        const ItemDef* d = t.item(it.id);
        if (!d) continue;
        const double a = d->amount;
        const double p0 = points, m0 = mult;
        switch (d->effect) {
        case ItemEffect::FlatPoints: points += a; break;
        case ItemEffect::FlatMult: mult += a; break;
        case ItemEffect::MultPerColor: mult += a * (by_color[d->color & 3] + wild); break;
        case ItemEffect::PointsPerHigh: {
            int k = 0;
            for (auto& c : l.cargo) k += c.value >= d->threshold;
            points += a * k;
            break;
        }
        case ItemEffect::MultPerParity: {
            int k = 0;
            for (auto& c : l.cargo) k += c.value % 2 == d->parity;
            mult += a * k;
            break;
        }
        case ItemEffect::MultIfFlush:
            if (flush >= d->min) mult += a;
            break;
        case ItemEffect::MultIfStraight:
            if (straight >= d->min) mult += a;
            break;
        case ItemEffect::MultPerCargoPast: mult += a * std::max(0, n - d->past); break;
        case ItemEffect::MultPerRisky: mult += a * l.risky; break;
        case ItemEffect::XmultIfSize:
            if (n >= d->min) mult *= a;
            break;
        case ItemEffect::XmultIfFlush:
            if (flush >= d->min) mult *= a;
            break;
        case ItemEffect::XmultIfStraight:
            if (straight >= d->min) mult *= a;
            break;
        case ItemEffect::XmultPerFlushPast:
            if (flush >= 3) mult *= 1 + a * (flush - 2);
            break;
        case ItemEffect::XmultIfCovered:
            if (l.covered) mult *= a;
            break;
        case ItemEffect::ScalingMultSize:
            if (commit && n >= d->min) it.state += a;
            mult += it.state;
            break;
        case ItemEffect::ScalingXmultQuota: mult *= 1.0 + it.state; break;
        case ItemEffect::QuotaBonus:
            if (sc.quota_bonus > 1) quota_bonus = std::max(quota_bonus, a);
            break;
        case ItemEffect::FullBonus: full_bonus = std::max(full_bonus, a); break;
        default: break;
        }
        if (steps && (points != p0 || mult != m0)) {
            std::string label;
            if (points != p0)
                label = "+" + num(points - p0) + " points from " + d->name;
            else if (is_additive(d->effect))
                label = "+" + num(mult - m0) + " mult from " + d->name;
            else
                label = "x" + num(m0 != 0 ? mult / m0 : 1) + " mult from " + d->name;
            push(Step::Kind::Item, label, static_cast<int>(ii));
        }
    }
    if (l.crates > 0) {
        double x = std::pow(sc.crate_mult, l.crates);
        mult *= x;
        push(Step::Kind::Crates, std::to_string(l.crates) + (l.crates == 1 ? " crate: x" : " crates: x") + num(x) + " mult");
    }
    double factor = 1;
    s.quota_hit = points >= quota && quota_bonus > 1;
    if (s.quota_hit) {
        factor *= quota_bonus;
        push(Step::Kind::Quota, "Quota " + std::to_string(quota) + " reached: x" + num(quota_bonus), -1, factor);
    }
    s.full = static_cast<int>(vals.size()) >= sc.truck_size;
    if (s.full) {
        factor *= full_bonus;
        push(Step::Kind::FullTruck, "Full truck: x" + num(full_bonus), -1, factor);
    }
    s.points = points;
    s.mult = mult;
    s.factor = factor;
    s.total = static_cast<long long>(std::floor(points * mult * factor + 1e-9));
    push(Step::Kind::Total, "Delivered: " + std::to_string(s.total), -1, factor);
    if (commit && s.quota_hit)
        for (auto& it : items)
            if (const ItemDef* d = t.item(it.id); d && d->effect == ItemEffect::ScalingXmultQuota) it.state += d->amount;
    return s;
}

// ---------------------------------------------------------------- a trip

SpinResult Run::spin(int wheel, bool use_band) {
    SpinResult r;
    if (!can_spin() || wheel < 0 || wheel >= static_cast<int>(deal.size())) return r;
    r.ok = true;
    r.wheel = wheel;
    if (wheel_def(wheel).effect == WheelEffect::Express && !contains(free_spun, wheel)) {
        free_spun.push_back(wheel);
        r.free = true;
    } else {
        ++load.spins;
    }
    ++m.spins;
    r.chance = spoil_chance(wheel);
    std::vector<double> weights(deal[wheel].size(), 1.0);
    if (use_band && r.chance > 0) {
        auto it = std::find(consumables.begin(), consumables.end(), "band");
        if (it != consumables.end()) {
            consumables.erase(it);
            r.band = true;
            for (std::size_t i = 0; i < weights.size(); ++i)
                if (clashes(wheel, deal[wheel][i])) weights[i] = 0.5;
        }
    }
    int index = rng_spin.weighted(weights);
    return land(wheel, index, r.chance, r);
}

bool Run::rescue_possible() const {
    return rescue_allowed(Rescue::NudgeLeft) || rescue_allowed(Rescue::NudgeRight) || rescue_allowed(Rescue::Respin);
}

bool Run::rescue_allowed(Rescue r) const {
    if (r == Rescue::Decline) return phase == Phase::Rescue;
    if (r == Rescue::Respin) return count_consumable("respin") > 0;
    if (count_consumable("nudge") == 0) return false;
    const auto& wedges = deal[rescue_wheel];
    int n = static_cast<int>(wedges.size());
    if (n < 2) return false;
    int j = (rescue_index + (r == Rescue::NudgeLeft ? n - 1 : 1)) % n;
    return !clashes(rescue_wheel, wedges[j]);
}

SpinResult Run::land(int wheel, int index, double chance, SpinResult r) {
    r.wheel = wheel;
    r.index = index;
    const Wedge w = deal[wheel][index];
    const auto& wedges = deal[wheel];
    const int n = static_cast<int>(wedges.size());
    if (!clashes(wheel, w)) {
        r.near_miss = n >= 2 && !covered(wheel) &&
                      (clashes(wheel, wedges[(index + 1) % n]) || clashes(wheel, wedges[(index + n - 1) % n]));
        load.used.push_back(w.uid);
        switch (w.kind) {
        case WedgeKind::Crate:
            ++load.crates;
            r.landing = Landing::Crate;
            break;
        case WedgeKind::Tarp:
            if (inspection() != Inspection::NoTarps) ++load.tarps;
            r.landing = Landing::Tarp;
            break;
        case WedgeKind::Cargo:
            load.cargo.push_back(cargo_from(wheel, w));
            if (chance >= t.scoring.risky_spin) ++load.risky;
            r.landing = Landing::Cargo;
            break;
        default: r.landing = Landing::Blank; break;
        }
        return r;
    }
    // A repeat. What can save the load, in order.
    if (wheel_def(wheel).effect == WheelEffect::Padded) {
        r.landing = Landing::Padded;
        ++m.saves;
        return r;
    }
    if (has(ItemEffect::DoubleClutch) && !load.clutch_used) {
        load.clutch_used = true;
        r.landing = Landing::Clutch;
        ++m.saves;
        return r;
    }
    if (inspection() != Inspection::NoTarps && (load.tarps > 0 || spare_tarps > 0)) {
        if (load.tarps > 0)
            --load.tarps;
        else
            --spare_tarps;
        load.covered = true;
        r.landing = Landing::TarpSave;
        ++m.saves;
        return r;
    }
    rescue_wheel = wheel;
    rescue_index = index;
    if (rescue_possible()) {
        phase = Phase::Rescue;
        r.landing = Landing::Rescue;
        return r;
    }
    spoil(r);
    return r;
}

SpinResult Run::rescue(Rescue how) {
    SpinResult r;
    if (phase != Phase::Rescue || !rescue_allowed(how)) return r;
    r.ok = true;
    r.wheel = rescue_wheel;
    r.index = rescue_index;
    phase = Phase::Trip;
    const int n = static_cast<int>(deal[rescue_wheel].size());
    if (how == Rescue::Decline) {
        spoil(r);
        return r;
    }
    if (how == Rescue::Respin) {
        consumables.erase(std::find(consumables.begin(), consumables.end(), "respin"));
        int j = rng_spin.below(n);
        r = land(rescue_wheel, j, 0, r);
    } else {
        consumables.erase(std::find(consumables.begin(), consumables.end(), "nudge"));
        int j = (rescue_index + (how == Rescue::NudgeLeft ? n - 1 : 1)) % n;
        r = land(rescue_wheel, j, 0, r);
    }
    if (r.landing <= Landing::Blank) ++m.saves;
    return r;
}

void Run::spoil(SpinResult& r) {
    r.landing = Landing::Spoiled;
    r.lost_value = score(load).total;
    double ins = sum_amount(ItemEffect::Insurance);
    r.insurance_pay = ins > 0 ? static_cast<long long>(std::floor(ins * static_cast<double>(r.lost_value) + 1e-9)) : 0;
    round_score += r.insurance_pay;
    r.spoil_cost = std::min(money, t.round.spoil_cost);
    money -= r.spoil_cost;
    ++m.spoiled;
    m.spoiled_cargo += static_cast<int>(load.cargo.size());
    end_trip();
}

Delivery Run::deliver() {
    Delivery d;
    if (phase != Phase::Trip || (load.cargo.empty() && !must_deliver())) return d;
    d.ok = true;
    d.load = load;
    if (load.cargo.empty()) {
        d.empty = true;
        end_trip();
        return d;
    }
    d.score = score(load, &d.steps, true);
    round_score += d.score.total;
    int n = static_cast<int>(load.cargo.size());
    d.freight = t.income.freight_bonus * std::max(0, n - t.income.freight_past);
    if (d.score.quota_hit) {
        ++m.quota_hits;
        d.tips = static_cast<int>(sum_amount(ItemEffect::Tip));
    }
    money += d.freight + d.tips;
    m.freight += d.freight;
    ++m.deliveries;
    m.delivered_cargo += n;
    if (d.score.full) ++m.full_trucks;
    m.best_delivery = std::max(m.best_delivery, d.score.total);
    delivered_uids.clear();
    for (auto& c : load.cargo)
        if (find_wedge(c.uid)) delivered_uids.push_back(c.uid);
    if (rods_left > 0 && !delivered_uids.empty() && bag.size() > 1) {
        d.rod_choice = true;
        phase = Phase::Rod;
        load = Load();
        return d;
    }
    end_trip();
    return d;
}

bool Run::rod_remove(int uid) {
    if (phase != Phase::Rod) return false;
    if (uid != 0) {
        if (!contains(delivered_uids, uid) || bag.size() <= 1) return false;
        bag.erase(std::remove_if(bag.begin(), bag.end(), [&](const Wedge& w) { return w.uid == uid; }), bag.end());
        for (std::size_t wi = 0; wi < deal.size(); ++wi)
            for (std::size_t k = 0; k < deal[wi].size(); ++k)
                if (deal[wi][k].uid == uid) {
                    Wedge blank;
                    blank.uid = -(static_cast<int>(wi) * 100 + static_cast<int>(k) + 1) - 10000;
                    deal[wi][k] = blank;
                }
        --rods_left;
        ++m.zaps;
    }
    phase = Phase::Trip;
    end_trip();
    return true;
}

void Run::end_trip() {
    load = Load();
    free_spun.clear();
    --trips_left;
    if (round_score >= target())
        end_round(true);
    else if (trips_left <= 0)
        end_round(false);
    else
        phase = Phase::Trip;
}

Income Run::income(int unused_trips) const {
    Income in;
    const auto& inc = t.income;
    in.round = inc.per_round;
    in.unused = std::min(inc.unused_cap, unused_trips * inc.per_unused_trip);
    in.interest = inc.interest_per > 0 ? std::min(inc.interest_cap, money / inc.interest_per) : 0;
    for (auto& it : items) {
        const ItemDef* d = t.item(it.id);
        if (!d) continue;
        if (d->effect == ItemEffect::Piggy) in.piggy += std::min(d->cap, static_cast<int>(money * d->rate + 1e-9));
        if (d->effect == ItemEffect::Spendthrift) in.spendthrift += static_cast<int>(money * d->rate + 1e-9);
    }
    in.total = in.round + in.unused + in.interest + in.piggy + in.spendthrift;
    return in;
}

void Run::end_round(bool won) {
    if (!won) {
        phase = Phase::Lost;
        return;
    }
    if (round == t.run.rounds - 1 && !endless) {
        phase = Phase::Won;
        return;
    }
    last_income = income(std::max(0, trips_left));
    money += last_income.total;
    reroll_price = t.shop.reroll_base;
    roll_offers();
    phase = Phase::Shop;
}

bool Run::go_endless() {
    if (phase != Phase::Won) return false;
    endless = true;
    end_round(true);
    return true;
}

// ---------------------------------------------------------------- the shop

bool Run::is_locked(const std::string& id) const {
    return std::find(config.locked.begin(), config.locked.end(), id) != config.locked.end();
}

Rarity Run::pick_rarity(bool present[3]) {
    const RarityRow* row = t.shop.rarity_by_round.empty() ? nullptr : &t.shop.rarity_by_round[0];
    for (auto& r : t.shop.rarity_by_round)
        if (round + 1 >= r.from_round) row = &r;
    double w[3] = {row ? row->common : 1, row ? row->uncommon : 0, row ? row->rare : 0};
    std::vector<double> weights;
    std::vector<int> which;
    for (int i = 0; i < 3; ++i)
        if (w[i] > 0 && present[i]) {
            weights.push_back(w[i]);
            which.push_back(i);
        }
    if (which.empty()) {
        for (int i = 0; i < 3; ++i)
            if (present[i]) return static_cast<Rarity>(i);
        return Rarity::Common;
    }
    return static_cast<Rarity>(which[rng_shop.weighted(weights)]);
}

Offer Run::roll_offer() {
    const auto& sh = t.shop;
    Offer o;
    int kind = rng_shop.weighted({sh.w_item, sh.w_wheel, sh.w_upgrade, sh.w_wedge, sh.w_consumable, sh.w_zap});
    // Pick from a content pool: a rarity first, then one of that rarity.
    auto pick = [&](auto& defs, const int* prices, OfferKind k) {
        bool present[3] = {false, false, false};
        for (auto& d : defs)
            if (!is_locked(d.id)) present[static_cast<int>(d.rarity)] = true;
        Rarity rar = pick_rarity(present);
        std::vector<const std::string*> ids, names;
        for (auto& d : defs)
            if (!is_locked(d.id) && d.rarity == rar) {
                ids.push_back(&d.id);
                names.push_back(&d.name);
            }
        o.kind = k;
        if (ids.empty()) return false;
        int i = rng_shop.below(static_cast<int>(ids.size()));
        o.id = *ids[i];
        o.name = *names[i];
        o.rarity = rar;
        o.price = prices[static_cast<int>(rar)];
        return true;
    };
    bool ok = true;
    switch (kind) {
    case 0: ok = pick(t.items, sh.price_item, OfferKind::Item); break;
    case 1: ok = pick(t.wheels, sh.price_wheel, OfferKind::Wheel); break;
    case 2: ok = pick(t.upgrades, sh.price_upgrade, OfferKind::Upgrade); break;
    case 4: ok = pick(t.consumables, sh.price_consumable, OfferKind::Consumable); break;
    case 3:
        o.kind = OfferKind::Wedge;
        if (rng_shop.unit() < sh.special_wedge_share) {
            bool tarp = rng_shop.below(3) < 2;
            o.wedge = tarp ? WedgeKind::Tarp : WedgeKind::Crate;
            o.id = tarp ? "tarp_wedge" : "crate_wedge";
            o.name = tarp ? "Tarp wedge" : "Crate wedge";
            o.price = tarp ? sh.price_tarp : sh.price_crate;
        } else {
            o.wedge = WedgeKind::Cargo;
            o.color = rng_shop.below(4);
            o.value = 1 + rng_shop.below(6);
            o.id = "cargo_wedge";
            o.name = std::string(kColorNames[o.color]) + " " + std::to_string(o.value) + " cargo wedge";
            o.price = sh.price_wedge;
        }
        break;
    default: ok = false; break;
    }
    if (!ok) {
        o = Offer();
        o.kind = OfferKind::Zap;
        o.id = "zap";
        o.name = "Zap";
        o.price = sh.price_zap;
    }
    return o;
}

void Run::roll_offers() {
    offers.clear();
    for (int i = 0; i < t.shop.offers; ++i) offers.push_back(roll_offer());
}

bool Run::can_take(const Offer& o) const {
    switch (o.kind) {
    case OfferKind::Item: return static_cast<int>(items.size()) < t.run.max_items;
    case OfferKind::Wheel: return static_cast<int>(wheels.size()) < t.run.max_wheels;
    case OfferKind::Consumable: return static_cast<int>(consumables.size()) < t.run.max_consumables;
    case OfferKind::Zap: return bag.size() > 1;
    default: return true;
    }
}

Wedge* Run::find_wedge(int uid) {
    for (auto& w : bag)
        if (w.uid == uid) return &w;
    return nullptr;
}

bool Run::upgrade_fits(const Offer& o, const Wedge& w) const {
    if (o.kind == OfferKind::Zap) return bag.size() > 1;
    if (o.kind != OfferKind::Upgrade || w.kind != WedgeKind::Cargo) return false;
    if (o.id == "premium") return w.value < t.run.max_value;
    if (o.id == "wild") return w.color != kWild;
    return true;
}

bool Run::buy(int index, int wedge_uid, int choice) {
    if (phase != Phase::Shop || index < 0 || index >= static_cast<int>(offers.size())) return false;
    Offer& o = offers[index];
    if (o.sold || o.price > money || !can_take(o)) return false;
    switch (o.kind) {
    case OfferKind::Item: items.push_back({o.id, o.price, 0}); break;
    case OfferKind::Wheel: wheels.push_back({o.id, o.price}); break;
    case OfferKind::Consumable: consumables.push_back(o.id); break;
    case OfferKind::Wedge: {
        Wedge w;
        w.uid = next_uid++;
        w.kind = o.wedge;
        w.color = o.color;
        w.value = o.value;
        bag.push_back(w);
        break;
    }
    case OfferKind::Zap: {
        Wedge* w = find_wedge(wedge_uid);
        if (!w || bag.size() <= 1) return false;
        bag.erase(bag.begin() + (w - bag.data()));
        ++m.zaps;
        break;
    }
    case OfferKind::Upgrade: {
        Wedge* w = find_wedge(wedge_uid);
        if (!w || !upgrade_fits(o, *w)) return false;
        if (o.id == "recut") {
            if (choice < 1 || choice > 6) return false;
            w->value = choice;
        } else if (o.id == "paint") {
            if (choice < 0 || choice > 3) return false;
            w->color = choice;
        } else if (o.id == "premium") {
            w->value = std::min(t.run.max_value, w->value + 1);
        } else if (o.id == "wild") {
            w->color = kWild;
        } else if (o.id == "stamp") {
            w->bonus += 3;
        } else {
            return false;
        }
        break;
    }
    }
    money -= o.price;
    o.sold = true;
    if (std::find(bought.begin(), bought.end(), o.id) == bought.end()) bought.push_back(o.id);
    return true;
}

int Run::sell_value(const OwnedItem& it) const { return static_cast<int>(it.price * t.shop.sell_fraction + 1e-9); }

bool Run::sell_item(int index) {
    if (phase != Phase::Shop || index < 0 || index >= static_cast<int>(items.size())) return false;
    money += sell_value(items[index]);
    items.erase(items.begin() + index);
    return true;
}

bool Run::move_item(int from, int to) {
    int n = static_cast<int>(items.size());
    if (from < 0 || from >= n || to < 0 || to >= n) return false;
    OwnedItem it = items[from];
    items.erase(items.begin() + from);
    items.insert(items.begin() + to, it);
    return true;
}

bool Run::reroll() {
    if (phase != Phase::Shop || money < reroll_price) return false;
    money -= reroll_price;
    reroll_price += t.shop.reroll_step;
    ++m.rerolls;
    roll_offers();
    return true;
}

bool Run::leave_shop() {
    if (phase != Phase::Shop) return false;
    if (has(ItemEffect::Spendthrift)) money = 0; // money left after the shop is lost
    ++round;
    start_round();
    return true;
}

std::string Run::wedge_label(const Wedge& w) const {
    switch (w.kind) {
    case WedgeKind::Tarp: return "TARP";
    case WedgeKind::Crate: return "CRATE";
    case WedgeKind::Blank: return "--";
    default: break;
    }
    std::string s(1, w.color == kWild ? '*' : kColorLetters[w.color & 3]);
    s += std::to_string(w.value);
    if (w.bonus) s += "+";
    return s;
}

} // namespace spin
