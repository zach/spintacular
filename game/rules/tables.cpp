#include "tables.h"

#include <cmath>

namespace spin {

namespace {
template <class V>
auto find_id(const V& v, const std::string& id) -> decltype(&v[0]) {
    for (auto& d : v)
        if (d.id == id) return &d;
    return nullptr;
}
} // namespace

const ItemDef* Tables::item(const std::string& id) const { return find_id(items, id); }
const WheelDef* Tables::wheel(const std::string& id) const { return find_id(wheels, id); }
const SimpleDef* Tables::upgrade(const std::string& id) const { return find_id(upgrades, id); }
const SimpleDef* Tables::consumable(const std::string& id) const { return find_id(consumables, id); }
const Mods* Tables::rig(const std::string& id) const { return find_id(rigs, id); }
const InspectionDef* Tables::inspection(Inspection k) const {
    for (auto& d : inspections)
        if (d.kind == k) return &d;
    return nullptr;
}

Tables apply_setup(const Tables& base, const std::string& rig, int stake) {
    Tables t = base;
    std::vector<const Mods*> mods;
    if (const Mods* r = base.rig(rig)) mods.push_back(r);
    for (int i = 0; i < stake - 1 && i < static_cast<int>(base.stakes.size()); ++i) mods.push_back(&base.stakes[i]);
    for (const Mods* m : mods) {
        if (m->has_bag) t.start.bag = m->bag;
        for (auto& w : m->add_wedges) t.start.bag.push_back(w);
        if (m->has_money) t.run.start_money = m->money;
        t.round.tank += m->tank;
        t.run.max_items += m->item_slots;
        if (m->truck_size) t.scoring.truck_size = m->truck_size;
        if (m->quota_scale > 0)
            for (auto& q : t.scoring.quotas) q = static_cast<int>(std::ceil(q * m->quota_scale - 1e-9));
        if (m->target_scale > 0)
            for (std::size_t i = 1; i < t.targets.size(); ++i)
                t.targets[i] = static_cast<long long>(t.targets[i] * m->target_scale + 1e-9);
        if (m->price_up) {
            for (int r = 0; r < 3; ++r) {
                t.shop.price_item[r] += m->price_up;
                t.shop.price_wheel[r] += m->price_up;
                t.shop.price_upgrade[r] += m->price_up;
                t.shop.price_consumable[r] += m->price_up;
            }
            t.shop.price_wedge += m->price_up;
            t.shop.price_tarp += m->price_up;
            t.shop.price_crate += m->price_up;
            t.shop.price_zap += m->price_up;
        }
        if (m->inspection_every) t.round.inspection_every = m->inspection_every;
        t.round.spoil_cost += m->spoil_cost;
    }
    return t;
}

} // namespace spin
