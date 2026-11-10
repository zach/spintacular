// The balance tables: every number and every piece of content in the game.
// The data layer fills a Tables from assets/data/tables.json; the rules read it
// and never parse anything themselves.
#pragma once

#include <map>
#include <string>
#include <vector>

namespace spin {

enum class Rarity { Common, Uncommon, Rare };
enum class WedgeKind { Cargo, Tarp, Crate, Blank };
enum class WheelEffect { None, Gilded, Bump, Express, Padded };
enum class Inspection { None, QuotaUp, FuelDown, NoTarps };

enum class ItemEffect {
    FlatPoints, FlatMult, MultPerColor, PointsPerHigh, MultPerParity, MultIfFlush, MultIfStraight,
    MultPerCargoPast, MultPerRisky, ScalingMultSize,                       // additive
    XmultIfSize, XmultIfFlush, XmultIfStraight, XmultPerFlushPast, XmultIfCovered, ScalingXmultQuota,
    QuotaBonus, FullBonus,                                                 // multiplying
    Tip, SpareTarp, Fuel, Insurance, Rod, Piggy, Spendthrift, DoubleClutch // outside the payout
};

inline bool is_additive(ItemEffect e) { return e <= ItemEffect::ScalingMultSize; }

struct ItemDef {
    std::string id, name, text;
    Rarity rarity = Rarity::Common;
    ItemEffect effect = ItemEffect::FlatPoints;
    double amount = 0;
    int color = 0, threshold = 0, parity = 0, min = 0, past = 0, cap = 0;
    double rate = 0;
};

struct WheelDef {
    std::string id, name, text;
    Rarity rarity = Rarity::Common;
    int size = 3;
    WheelEffect effect = WheelEffect::None;
    int amount = 0;
};

struct SimpleDef {
    std::string id, name, text;
    Rarity rarity = Rarity::Common;
};

struct InspectionDef {
    Inspection kind = Inspection::None;
    std::string id, name, text;
};

// One starting wedge, as written in the tables: "R1", "TARP", "CRATE".
struct WedgeSpec {
    WedgeKind kind = WedgeKind::Cargo;
    int color = 0, value = 0;
};

// What a rig or a stake changes.
struct Mods {
    std::string id, name, text;
    bool has_bag = false;
    std::vector<WedgeSpec> bag, add_wedges;
    bool has_money = false;
    int money = 0;
    int tank = 0, item_slots = 0, truck_size = 0, price_up = 0, inspection_every = 0, spoil_cost = 0;
    double quota_scale = 0, target_scale = 0;
};

// What unlocks a piece of locked content (docs/design/progression.md).
struct UnlockDef {
    std::string id, when, text;
    int n = 0;
};

struct RarityRow {
    int from_round = 1;
    double common = 1, uncommon = 0, rare = 0;
};

struct Tables {
    struct {
        int rounds = 12, start_money = 4, max_items = 5, max_wheels = 6, max_consumables = 2, max_value = 9;
    } run;
    struct {
        int trips = 4, tank = 10, inspection_every = 3, spoil_cost = 0;
        bool inspections = true;
    } round;
    struct {
        double mult_per_cargo = 1, full_bonus = 1.5, quota_bonus = 2, crate_mult = 1.5, risky_spin = 0.25;
        int truck_size = 6;
        std::vector<int> quotas, flush, straight;
    } scoring;
    std::vector<long long> targets;
    struct {
        int per_round = 5, per_unused_trip = 2, unused_cap = 4, interest_per = 5, interest_cap = 5;
        int freight_bonus = 1, freight_past = 3;
    } income;
    struct {
        std::vector<WedgeSpec> bag;
        std::vector<std::string> wheels;
    } start;
    struct {
        int offers = 5, reroll_base = 4, reroll_step = 1;
        double sell_fraction = 0.5, special_wedge_share = 0.4;
        double w_item = 36, w_wheel = 12, w_upgrade = 20, w_wedge = 16, w_consumable = 10, w_zap = 6;
        std::vector<RarityRow> rarity_by_round;
        int price_item[3] = {4, 6, 9}, price_wheel[3] = {5, 7, 10}, price_upgrade[3] = {3, 5, 7},
            price_consumable[3] = {2, 3, 4};
        int price_wedge = 2, price_tarp = 3, price_crate = 4, price_zap = 2;
    } shop;

    // Content, in table order.
    std::vector<ItemDef> items;
    std::vector<WheelDef> wheels;
    std::vector<SimpleDef> upgrades, consumables;
    std::vector<InspectionDef> inspections;
    std::vector<Mods> rigs;   // rigs[0] is the Standard rig
    std::vector<Mods> stakes; // stakes[i] is what stake i + 2 adds
    std::vector<std::string> locked; // content the first runs don't offer
    std::vector<UnlockDef> unlocks;  // how each locked piece is unlocked

    const ItemDef* item(const std::string& id) const;
    const WheelDef* wheel(const std::string& id) const;
    const SimpleDef* upgrade(const std::string& id) const;
    const SimpleDef* consumable(const std::string& id) const;
    const InspectionDef* inspection(Inspection k) const;
    const Mods* rig(const std::string& id) const;
};

// The tables a run plays on: the base tables with a rig and a stake applied.
// Stake 1 is the base game; stake n keeps every rule below it.
Tables apply_setup(const Tables& base, const std::string& rig, int stake);

} // namespace spin
