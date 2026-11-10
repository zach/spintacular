#include "tables_json.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace spin {

using json = nlohmann::json;

std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool write_file(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << text;
    return static_cast<bool>(f);
}

namespace {

Rarity rarity_of(const json& j) {
    std::string r = j.value("rarity", "common");
    return r == "rare" ? Rarity::Rare : r == "uncommon" ? Rarity::Uncommon : Rarity::Common;
}

WedgeSpec wedge_spec(const std::string& s) {
    WedgeSpec w;
    if (s == "TARP") {
        w.kind = WedgeKind::Tarp;
    } else if (s == "CRATE") {
        w.kind = WedgeKind::Crate;
    } else {
        static const std::string letters = "RGBY";
        auto c = s.empty() ? std::string::npos : letters.find(s[0]);
        if (c == std::string::npos || s.size() < 2) throw std::runtime_error("bad wedge \"" + s + "\"");
        w.kind = WedgeKind::Cargo;
        w.color = static_cast<int>(c);
        w.value = std::stoi(s.substr(1));
    }
    return w;
}

ItemEffect item_effect(const std::string& e) {
    static const std::pair<const char*, ItemEffect> names[] = {
        {"flat_points", ItemEffect::FlatPoints},
        {"flat_mult", ItemEffect::FlatMult},
        {"mult_per_color", ItemEffect::MultPerColor},
        {"points_per_high", ItemEffect::PointsPerHigh},
        {"mult_per_parity", ItemEffect::MultPerParity},
        {"mult_if_flush", ItemEffect::MultIfFlush},
        {"mult_if_straight", ItemEffect::MultIfStraight},
        {"mult_per_cargo_past", ItemEffect::MultPerCargoPast},
        {"mult_per_risky", ItemEffect::MultPerRisky},
        {"scaling_mult_size", ItemEffect::ScalingMultSize},
        {"xmult_if_size", ItemEffect::XmultIfSize},
        {"xmult_if_flush", ItemEffect::XmultIfFlush},
        {"xmult_if_straight", ItemEffect::XmultIfStraight},
        {"xmult_per_flush_past", ItemEffect::XmultPerFlushPast},
        {"xmult_if_covered", ItemEffect::XmultIfCovered},
        {"scaling_xmult_quota", ItemEffect::ScalingXmultQuota},
        {"quota_bonus", ItemEffect::QuotaBonus},
        {"full_bonus", ItemEffect::FullBonus},
        {"tip", ItemEffect::Tip},
        {"spare_tarp", ItemEffect::SpareTarp},
        {"fuel", ItemEffect::Fuel},
        {"insurance", ItemEffect::Insurance},
        {"rod", ItemEffect::Rod},
        {"piggy", ItemEffect::Piggy},
        {"spendthrift", ItemEffect::Spendthrift},
        {"double_clutch", ItemEffect::DoubleClutch},
    };
    for (auto& [name, effect] : names)
        if (e == name) return effect;
    throw std::runtime_error("unknown item effect \"" + e + "\"");
}

WheelEffect wheel_effect(const std::string& e) {
    if (e == "none") return WheelEffect::None;
    if (e == "gilded") return WheelEffect::Gilded;
    if (e == "bump") return WheelEffect::Bump;
    if (e == "express") return WheelEffect::Express;
    if (e == "padded") return WheelEffect::Padded;
    throw std::runtime_error("unknown wheel effect \"" + e + "\"");
}

Mods mods_of(const std::string& id, const json& j) {
    Mods m;
    m.id = id;
    m.name = j.value("name", id);
    m.text = j.value("text", "");
    if (j.contains("bag")) {
        m.has_bag = true;
        for (auto& w : j["bag"]) m.bag.push_back(wedge_spec(w.get<std::string>()));
    }
    if (j.contains("add_wedges"))
        for (auto& w : j["add_wedges"]) m.add_wedges.push_back(wedge_spec(w.get<std::string>()));
    if (j.contains("money")) {
        m.has_money = true;
        m.money = j["money"].get<int>();
    }
    m.tank = j.value("tank", 0);
    m.item_slots = j.value("item_slots", 0);
    m.truck_size = j.value("truck_size", 0);
    m.price_up = j.value("price_up", 0);
    m.inspection_every = j.value("inspection_every", 0);
    m.spoil_cost = j.value("spoil_cost", 0);
    m.quota_scale = j.value("quota_scale", 0.0);
    m.target_scale = j.value("target_scale", 0.0);
    return m;
}

void prices(const json& j, int out[3]) {
    out[0] = j.at("common").get<int>();
    out[1] = j.at("uncommon").get<int>();
    out[2] = j.at("rare").get<int>();
}

} // namespace

Tables tables_from_json(const std::string& text) {
    Tables t;
    try {
        json j = json::parse(text);
        const json& run = j.at("run");
        t.run.rounds = run.at("rounds").get<int>();
        t.run.start_money = run.at("start_money").get<int>();
        t.run.max_items = run.at("max_items").get<int>();
        t.run.max_wheels = run.at("max_wheels").get<int>();
        t.run.max_consumables = run.at("max_consumables").get<int>();
        t.run.max_value = run.at("max_value").get<int>();
        const json& rd = j.at("round");
        t.round.trips = rd.at("trips").get<int>();
        t.round.tank = rd.at("tank").get<int>();
        t.round.inspections = rd.value("inspections", true);
        t.round.inspection_every = rd.at("inspection_every").get<int>();
        t.round.spoil_cost = rd.value("spoil_cost", 0);
        const json& sc = j.at("scoring");
        t.scoring.mult_per_cargo = sc.at("mult_per_cargo").get<double>();
        t.scoring.truck_size = sc.at("truck_size").get<int>();
        t.scoring.full_bonus = sc.at("full_bonus").get<double>();
        t.scoring.quota_bonus = sc.at("quota_bonus").get<double>();
        t.scoring.crate_mult = sc.at("crate_mult").get<double>();
        t.scoring.risky_spin = sc.at("risky_spin").get<double>();
        t.scoring.quotas = sc.at("quotas").get<std::vector<int>>();
        t.scoring.flush = sc.at("flush").get<std::vector<int>>();
        t.scoring.straight = sc.at("straight").get<std::vector<int>>();
        t.targets = j.at("targets").get<std::vector<long long>>();
        const json& inc = j.at("income");
        t.income.per_round = inc.at("per_round").get<int>();
        t.income.per_unused_trip = inc.at("per_unused_trip").get<int>();
        t.income.unused_cap = inc.at("unused_cap").get<int>();
        t.income.interest_per = inc.at("interest_per").get<int>();
        t.income.interest_cap = inc.at("interest_cap").get<int>();
        t.income.freight_bonus = inc.value("freight_bonus", 0);
        t.income.freight_past = inc.value("freight_past", 3);
        for (auto& w : j.at("start").at("bag")) t.start.bag.push_back(wedge_spec(w.get<std::string>()));
        t.start.wheels = j.at("start").at("wheels").get<std::vector<std::string>>();
        const json& sh = j.at("shop");
        t.shop.offers = sh.at("offers").get<int>();
        t.shop.reroll_base = sh.at("reroll_base").get<int>();
        t.shop.reroll_step = sh.at("reroll_step").get<int>();
        t.shop.sell_fraction = sh.at("sell_fraction").get<double>();
        t.shop.special_wedge_share = sh.at("special_wedge_share").get<double>();
        const json& kw = sh.at("kind_weights");
        t.shop.w_item = kw.at("item").get<double>();
        t.shop.w_wheel = kw.at("wheel").get<double>();
        t.shop.w_upgrade = kw.at("upgrade").get<double>();
        t.shop.w_wedge = kw.at("wedge").get<double>();
        t.shop.w_consumable = kw.at("consumable").get<double>();
        t.shop.w_zap = kw.at("zap").get<double>();
        for (auto& r : sh.at("rarity_by_round")) {
            RarityRow row;
            row.from_round = r.at("from_round").get<int>();
            row.common = r.at("weights").at("common").get<double>();
            row.uncommon = r.at("weights").at("uncommon").get<double>();
            row.rare = r.at("weights").at("rare").get<double>();
            t.shop.rarity_by_round.push_back(row);
        }
        const json& pr = sh.at("prices");
        prices(pr.at("item"), t.shop.price_item);
        prices(pr.at("wheel"), t.shop.price_wheel);
        prices(pr.at("upgrade"), t.shop.price_upgrade);
        prices(pr.at("consumable"), t.shop.price_consumable);
        t.shop.price_wedge = pr.at("wedge").get<int>();
        t.shop.price_tarp = pr.at("tarp").get<int>();
        t.shop.price_crate = pr.at("crate").get<int>();
        t.shop.price_zap = pr.at("zap").get<int>();

        // nlohmann's ordered_json would keep table order; ids are listed in "order" where it matters.
        nlohmann::ordered_json oj = nlohmann::ordered_json::parse(text);
        for (auto& [id, v] : oj.at("items").items()) {
            json d = json::parse(v.dump());
            ItemDef it;
            it.id = id;
            it.name = d.at("name").get<std::string>();
            it.text = d.value("text", "");
            it.rarity = rarity_of(d);
            it.effect = item_effect(d.at("effect").get<std::string>());
            it.amount = d.value("amount", 0.0);
            it.color = d.value("color", 0);
            it.threshold = d.value("threshold", 0);
            it.parity = d.value("parity", 0);
            it.min = d.value("min", 0);
            it.past = d.value("past", 0);
            it.cap = d.value("cap", 0);
            it.rate = d.value("rate", 0.0);
            t.items.push_back(it);
        }
        for (auto& [id, v] : oj.at("wheels").items()) {
            json d = json::parse(v.dump());
            WheelDef w;
            w.id = id;
            w.name = d.at("name").get<std::string>();
            w.text = d.value("text", "");
            w.rarity = rarity_of(d);
            w.size = d.at("size").get<int>();
            w.effect = wheel_effect(d.at("effect").get<std::string>());
            w.amount = d.value("amount", 0);
            t.wheels.push_back(w);
        }
        auto simple = [&](const char* key, std::vector<SimpleDef>& out) {
            for (auto& [id, v] : oj.at(key).items()) {
                json d = json::parse(v.dump());
                out.push_back({id, d.at("name").get<std::string>(), d.value("text", ""), rarity_of(d)});
            }
        };
        simple("upgrades", t.upgrades);
        simple("consumables", t.consumables);
        for (auto& [id, v] : oj.at("inspections").items()) {
            json d = json::parse(v.dump());
            InspectionDef in;
            in.id = id;
            in.kind = id == "quota_up" ? Inspection::QuotaUp : id == "fuel_down" ? Inspection::FuelDown : id == "no_tarps" ? Inspection::NoTarps : Inspection::None;
            if (in.kind == Inspection::None) throw std::runtime_error("unknown inspection \"" + id + "\"");
            in.name = d.at("name").get<std::string>();
            in.text = d.value("text", "");
            t.inspections.push_back(in);
        }
        for (auto& [id, v] : oj.at("rigs").items()) t.rigs.push_back(mods_of(id, json::parse(v.dump())));
        int n = 2;
        for (auto& v : j.at("stakes")) t.stakes.push_back(mods_of("stake" + std::to_string(n++), v));
        if (j.contains("locked")) t.locked = j["locked"].get<std::vector<std::string>>();
        if (oj.contains("unlocks"))
            for (auto& [id, v] : oj["unlocks"].items()) {
                json d = json::parse(v.dump());
                t.unlocks.push_back({id, d.at("when").get<std::string>(), d.value("text", ""), d.value("n", 0)});
            }
    } catch (const json::exception& e) {
        throw std::runtime_error(std::string("tables: ") + e.what());
    }
    if (t.rigs.empty() || t.rigs[0].id != "standard") throw std::runtime_error("tables: the first rig must be \"standard\"");
    if (static_cast<int>(t.targets.size()) < t.run.rounds || static_cast<int>(t.scoring.quotas.size()) < t.run.rounds)
        throw std::runtime_error("tables: targets and quotas need one entry per round");
    for (auto& w : t.start.wheels)
        if (!t.wheel(w)) throw std::runtime_error("tables: unknown starting wheel \"" + w + "\"");
    return t;
}

Tables load_tables(const std::string& path) {
    std::string text = read_file(path);
    if (text.empty()) throw std::runtime_error("tables: can't read " + path);
    return tables_from_json(text);
}

std::string override_tables_json(const std::string& text, const std::string& assignment) {
    auto eq = assignment.find('=');
    if (eq == std::string::npos) throw std::runtime_error("override needs key=value: " + assignment);
    nlohmann::ordered_json j = nlohmann::ordered_json::parse(text);
    std::string path = "/" + assignment.substr(0, eq);
    for (auto& c : path)
        if (c == '.') c = '/';
    nlohmann::ordered_json value;
    try {
        value = nlohmann::ordered_json::parse(assignment.substr(eq + 1));
    } catch (const json::exception&) {
        value = assignment.substr(eq + 1);
    }
    j[nlohmann::ordered_json::json_pointer(path)] = value;
    return j.dump(1);
}

} // namespace spin
