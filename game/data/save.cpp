#include "save.h"

#include <nlohmann/json.hpp>

namespace spin {

using json = nlohmann::json;

namespace {

json rng_json(const Rng& r) { return json::array({r.s[0], r.s[1], r.s[2], r.s[3]}); }
void rng_from(const json& j, Rng& r) {
    for (int i = 0; i < 4; ++i) r.s[i] = j.at(i).get<std::uint64_t>();
}

json wedge_json(const Wedge& w) { return json::array({w.uid, static_cast<int>(w.kind), w.color, w.value, w.bonus}); }
Wedge wedge_from(const json& j) {
    Wedge w;
    w.uid = j.at(0).get<int>();
    w.kind = static_cast<WedgeKind>(j.at(1).get<int>());
    w.color = j.at(2).get<int>();
    w.value = j.at(3).get<int>();
    w.bonus = j.at(4).get<int>();
    return w;
}

json load_json(const Load& l) {
    json cargo = json::array();
    for (auto& c : l.cargo) cargo.push_back(json::array({c.color, c.value, c.bonus, c.uid}));
    return {{"used", l.used},   {"cargo", cargo},     {"crates", l.crates},   {"tarps", l.tarps},
            {"risky", l.risky}, {"spins", l.spins},   {"covered", l.covered}, {"clutch_used", l.clutch_used}};
}
Load load_from(const json& j) {
    Load l;
    l.used = j.at("used").get<std::vector<int>>();
    for (auto& c : j.at("cargo")) l.cargo.push_back({c.at(0).get<int>(), c.at(1).get<int>(), c.at(2).get<int>(), c.at(3).get<int>()});
    l.crates = j.at("crates").get<int>();
    l.tarps = j.at("tarps").get<int>();
    l.risky = j.at("risky").get<int>();
    l.spins = j.at("spins").get<int>();
    l.covered = j.at("covered").get<bool>();
    l.clutch_used = j.at("clutch_used").get<bool>();
    return l;
}

} // namespace

std::string run_to_json(const Run& r) {
    json j;
    j["format"] = "spintacular-run/1";
    j["config"] = {{"seed", r.config.seed}, {"rig", r.config.rig}, {"stake", r.config.stake}, {"locked", r.config.locked}, {"daily", r.config.daily}};
    j["rng"] = {{"shop", rng_json(r.rng_shop)}, {"deal", rng_json(r.rng_deal)}, {"spin", rng_json(r.rng_spin)}, {"route", rng_json(r.rng_route)}};
    j["phase"] = static_cast<int>(r.phase);
    j["money"] = r.money;
    j["round"] = r.round;
    j["endless"] = r.endless;
    j["next_uid"] = r.next_uid;
    json bag = json::array();
    for (auto& w : r.bag) bag.push_back(wedge_json(w));
    j["bag"] = bag;
    json wheels = json::array();
    for (auto& w : r.wheels) wheels.push_back(json::array({w.id, w.price}));
    j["wheels"] = wheels;
    json items = json::array();
    for (auto& it : r.items) items.push_back(json::array({it.id, it.price, it.state}));
    j["items"] = items;
    j["consumables"] = r.consumables;
    j["bought"] = r.bought;
    json route = json::array();
    for (auto k : r.route) route.push_back(static_cast<int>(k));
    j["route"] = route;
    json deal = json::array();
    for (auto& wheel : r.deal) {
        json ws = json::array();
        for (auto& w : wheel) ws.push_back(wedge_json(w));
        deal.push_back(ws);
    }
    j["deal"] = deal;
    j["load"] = load_json(r.load);
    j["round_score"] = r.round_score;
    j["trips_left"] = r.trips_left;
    j["tank"] = r.tank;
    j["quota"] = r.quota;
    j["spare_tarps"] = r.spare_tarps;
    j["rods_left"] = r.rods_left;
    j["free_spun"] = r.free_spun;
    j["rescue"] = json::array({r.rescue_wheel, r.rescue_index});
    j["delivered_uids"] = r.delivered_uids;
    json offers = json::array();
    for (auto& o : r.offers)
        offers.push_back({{"kind", static_cast<int>(o.kind)}, {"id", o.id}, {"name", o.name}, {"price", o.price},
                          {"rarity", static_cast<int>(o.rarity)}, {"wedge", static_cast<int>(o.wedge)},
                          {"color", o.color}, {"value", o.value}, {"sold", o.sold}});
    j["offers"] = offers;
    j["reroll_price"] = r.reroll_price;
    const Income& in = r.last_income;
    j["income"] = json::array({in.round, in.unused, in.interest, in.piggy, in.spendthrift, in.total});
    const Metrics& m = r.m;
    j["metrics"] = json::array({static_cast<long long>(m.spins), static_cast<long long>(m.deliveries),
                                static_cast<long long>(m.delivered_cargo), static_cast<long long>(m.spoiled),
                                static_cast<long long>(m.spoiled_cargo), static_cast<long long>(m.quota_hits),
                                static_cast<long long>(m.freight), static_cast<long long>(m.full_trucks),
                                static_cast<long long>(m.saves), static_cast<long long>(m.zaps),
                                static_cast<long long>(m.rerolls), m.best_delivery});
    return j.dump();
}

bool run_from_json(const Tables& base, const std::string& text, Run& out) {
    try {
        json j = json::parse(text);
        if (j.value("format", "") != "spintacular-run/1") return false;
        Run r;
        const json& c = j.at("config");
        r.config.seed = c.at("seed").get<std::uint64_t>();
        r.config.rig = c.at("rig").get<std::string>();
        r.config.stake = c.at("stake").get<int>();
        r.config.locked = c.at("locked").get<std::vector<std::string>>();
        r.config.daily = c.at("daily").get<bool>();
        r.t = apply_setup(base, r.config.rig, r.config.stake);
        rng_from(j.at("rng").at("shop"), r.rng_shop);
        rng_from(j.at("rng").at("deal"), r.rng_deal);
        rng_from(j.at("rng").at("spin"), r.rng_spin);
        rng_from(j.at("rng").at("route"), r.rng_route);
        r.phase = static_cast<Phase>(j.at("phase").get<int>());
        r.money = j.at("money").get<int>();
        r.round = j.at("round").get<int>();
        r.endless = j.at("endless").get<bool>();
        r.next_uid = j.at("next_uid").get<int>();
        for (auto& w : j.at("bag")) r.bag.push_back(wedge_from(w));
        for (auto& w : j.at("wheels")) {
            OwnedWheel ow{w.at(0).get<std::string>(), w.at(1).get<int>()};
            if (!r.t.wheel(ow.id)) return false;
            r.wheels.push_back(ow);
        }
        for (auto& it : j.at("items")) {
            OwnedItem oi{it.at(0).get<std::string>(), it.at(1).get<int>(), it.at(2).get<double>()};
            if (!r.t.item(oi.id)) return false;
            r.items.push_back(oi);
        }
        r.consumables = j.at("consumables").get<std::vector<std::string>>();
        r.bought = j.at("bought").get<std::vector<std::string>>();
        for (auto& k : j.at("route")) r.route.push_back(static_cast<Inspection>(k.get<int>()));
        for (auto& wheel : j.at("deal")) {
            std::vector<Wedge> ws;
            for (auto& w : wheel) ws.push_back(wedge_from(w));
            r.deal.push_back(ws);
        }
        if (r.deal.size() != r.wheels.size() && r.phase != Phase::Shop) return false;
        r.load = load_from(j.at("load"));
        r.round_score = j.at("round_score").get<long long>();
        r.trips_left = j.at("trips_left").get<int>();
        r.tank = j.at("tank").get<int>();
        r.quota = j.at("quota").get<int>();
        r.spare_tarps = j.at("spare_tarps").get<int>();
        r.rods_left = j.at("rods_left").get<int>();
        r.free_spun = j.at("free_spun").get<std::vector<int>>();
        r.rescue_wheel = j.at("rescue").at(0).get<int>();
        r.rescue_index = j.at("rescue").at(1).get<int>();
        r.delivered_uids = j.at("delivered_uids").get<std::vector<int>>();
        for (auto& o : j.at("offers")) {
            Offer of;
            of.kind = static_cast<OfferKind>(o.at("kind").get<int>());
            of.id = o.at("id").get<std::string>();
            of.name = o.at("name").get<std::string>();
            of.price = o.at("price").get<int>();
            of.rarity = static_cast<Rarity>(o.at("rarity").get<int>());
            of.wedge = static_cast<WedgeKind>(o.at("wedge").get<int>());
            of.color = o.at("color").get<int>();
            of.value = o.at("value").get<int>();
            of.sold = o.at("sold").get<bool>();
            r.offers.push_back(of);
        }
        r.reroll_price = j.at("reroll_price").get<int>();
        const json& in = j.at("income");
        r.last_income = {in.at(0).get<int>(), in.at(1).get<int>(), in.at(2).get<int>(), in.at(3).get<int>(), in.at(4).get<int>(), in.at(5).get<int>()};
        const json& m = j.at("metrics");
        r.m = {m.at(0).get<int>(), m.at(1).get<int>(), m.at(2).get<int>(), m.at(3).get<int>(), m.at(4).get<int>(), m.at(5).get<int>(),
               m.at(6).get<int>(), m.at(7).get<int>(), m.at(8).get<int>(), m.at(9).get<int>(), m.at(10).get<int>(), m.at(11).get<long long>()};
        out = std::move(r);
        return true;
    } catch (const json::exception&) {
        return false;
    }
}

} // namespace spin
