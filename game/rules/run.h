// A run of Spintacular: the bag, the deal, spins, loads, deliveries, the shop.
// Rules: docs/design/scoring.md, rounds.md, economy.md, progression.md.
//
// The rules core depends on the C++ standard library only. It is deterministic:
// a seed and the player's choices give the same run on every machine. A Run is
// plain data, so the data layer can save one at any shop or spin and resume it.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rng.h"
#include "tables.h"

namespace spin {

constexpr int kWild = -1; // a wedge that counts as any color

struct Wedge {
    int uid = 0; // names the wedge for the whole run; 0 for an empty wedge
    WedgeKind kind = WedgeKind::Blank;
    int color = 0; // 0..3, or kWild
    int value = 0;
    int bonus = 0; // extra points from Stamps
};

struct Cargo {
    int color = 0, value = 0, bonus = 0, uid = 0;
};

// What is in the truck.
struct Load {
    std::vector<int> used; // uids of the wedges that have landed this trip
    std::vector<Cargo> cargo;
    int crates = 0, tarps = 0, risky = 0, spins = 0;
    bool covered = false;     // a tarp saved this load
    bool clutch_used = false; // Double Clutch has thrown out its one repeat
};

struct OwnedItem {
    std::string id;
    int price = 0;
    double state = 0; // what a scaling item has gained so far
};

struct OwnedWheel {
    std::string id;
    int price = 0;
};

enum class OfferKind { Item, Wheel, Upgrade, Wedge, Consumable, Zap };

struct Offer {
    OfferKind kind = OfferKind::Item;
    std::string id, name;
    int price = 0;
    Rarity rarity = Rarity::Common;
    WedgeKind wedge = WedgeKind::Cargo; // for a wedge offer
    int color = 0, value = 0;
    bool sold = false;
};

// One line of a delivery's payout, in the order the rules apply.
struct Step {
    enum class Kind { Cargo, Size, ColorSet, RunSet, Item, Crates, Quota, FullTruck, Total };
    Kind kind = Kind::Total;
    std::string label; // "+4 mult from Color Guard"
    int index = -1;    // the cargo or the item this step is about
    double points = 0, mult = 0, factor = 1; // the running payout after this step
};

struct Score {
    double points = 0, mult = 0, factor = 1;
    long long total = 0;
    bool quota_hit = false, full = false;
    int flush = 0, straight = 0;
};

enum class Landing {
    Cargo, Tarp, Crate, Blank, // landed safely
    Padded,                    // a repeat, thrown out by a Padded Wheel
    Clutch,                    // a repeat, thrown out by Double Clutch
    TarpSave,                  // a repeat, covered by a tarp
    Rescue,                    // a repeat: the player may use a Nudge or a Respin
    Spoiled,                   // a repeat, and the load is lost
};

struct SpinResult {
    bool ok = false; // false if the spin wasn't allowed
    int wheel = 0, index = 0;
    double chance = 0; // the wheel's chance to spoil the load before this spin
    bool free = false; // an Express wheel's free spin
    bool band = false; // an Outer Band was used
    Landing landing = Landing::Blank;
    bool near_miss = false; // stopped safely next to a wedge that would have spoiled the load
    // When the load spoiled:
    long long lost_value = 0;    // what the load would have paid
    long long insurance_pay = 0; // what Insurance paid for it
    int spoil_cost = 0;
};

enum class Rescue { NudgeLeft, NudgeRight, Respin, Decline };

struct Delivery {
    bool ok = false;
    bool empty = false; // the trip ended with no cargo: nothing to pay
    Load load;
    std::vector<Step> steps;
    Score score;
    int freight = 0; // $ for cargo past the third
    int tips = 0;    // $ from Tip Jars
    bool rod_choice = false; // The Rod: the player may now remove one delivered wedge
};

struct Income {
    int round = 0, unused = 0, interest = 0, piggy = 0, spendthrift = 0, total = 0;
};

enum class Phase {
    Trip,   // spin a wheel, or deliver
    Rescue, // a repeat landed: Nudge, Respin, or let the load spoil
    Rod,    // after a delivery: remove one delivered wedge, or not
    Shop,   // between rounds
    Won,    // cleared the last round; the run may go on, endless
    Lost,   // missed a round's target
};

struct RunConfig {
    std::uint64_t seed = 1;
    std::string rig = "standard";
    int stake = 1;
    std::vector<std::string> locked; // content this run's shop never offers
    bool daily = false;
};

// Counters for the whole run: the collection, unlocks and the simulation read them.
struct Metrics {
    int spins = 0, deliveries = 0, delivered_cargo = 0, spoiled = 0, spoiled_cargo = 0, quota_hits = 0;
    int freight = 0, full_trucks = 0, saves = 0, zaps = 0, rerolls = 0;
    long long best_delivery = 0;
};

class Run {
public:
    Run() = default;
    Run(const Tables& base, const RunConfig& config);

    // ---- state (plain data; saved and loaded as is)
    Tables t; // the tables with the rig and stake applied
    RunConfig config;
    Rng rng_shop, rng_deal, rng_spin, rng_route;
    Phase phase = Phase::Trip;
    int money = 0;
    int round = 0; // 0-based
    bool endless = false;
    int next_uid = 1;
    std::vector<Wedge> bag;
    std::vector<OwnedWheel> wheels;
    std::vector<OwnedItem> items;
    std::vector<std::string> consumables;
    std::vector<std::string> bought; // every content id bought this run
    std::vector<Inspection> route;   // the inspection of each round so far and ahead
    // The round in play
    std::vector<std::vector<Wedge>> deal; // what each wheel holds
    Load load;
    long long round_score = 0;
    int trips_left = 0, tank = 0, quota = 0, spare_tarps = 0, rods_left = 0;
    std::vector<int> free_spun; // Express wheels that have had their free spin this trip
    int rescue_wheel = 0, rescue_index = 0;
    std::vector<int> delivered_uids; // the last delivery's wedges, for The Rod
    // The shop
    std::vector<Offer> offers;
    int reroll_price = 0;
    Income last_income;
    Metrics m;

    // ---- reading the round
    long long target() const { return target_of(round); }
    long long target_of(int round) const;
    int quota_of(int round) const;
    Inspection inspection() const { return inspection_of(round); }
    Inspection inspection_of(int round) const;
    // The next round, from this one on, that is an inspection; -1 if none is known.
    int next_inspection_round() const;
    const WheelDef& wheel_def(int wheel) const;
    int truck_values() const; // different cargo values in the truck
    bool truck_full() const;
    int spins_left() const { return tank - load.spins; }
    bool can_spin() const;
    bool can_deliver() const; // the player may choose to deliver
    bool must_deliver() const; // a full truck or an empty tank: the trip ends now
    bool has(ItemEffect e) const;
    double sum_amount(ItemEffect e) const;
    int count_consumable(const std::string& id) const;

    // No repeats: would this wedge, landed on this wheel, spoil the load?
    bool clashes(int wheel, const Wedge& w) const { return clashes(wheel, w, load); }
    bool clashes(int wheel, const Wedge& w, const Load& l) const;
    // Would a repeat on this wheel be absorbed (Padded, Double Clutch, a tarp)?
    bool covered(int wheel) const { return covered(wheel, load); }
    bool covered(int wheel, const Load& l) const;
    // The chance that spinning this wheel now spoils the load, as shown to the player.
    double spoil_chance(int wheel, bool band = false) const;
    int landed_value(int wheel, const Wedge& w) const;
    Cargo cargo_from(int wheel, const Wedge& w) const;

    // What a load pays. commit is for a real delivery: scaling items grow.
    Score score(const Load& l, std::vector<Step>* steps = nullptr, bool commit = false);
    Score score_now() { return score(load); }

    // ---- playing the round
    SpinResult spin(int wheel, bool use_band = false);
    bool rescue_allowed(Rescue r) const;
    SpinResult rescue(Rescue r);
    Delivery deliver();
    bool rod_remove(int uid); // uid 0: keep them all

    // ---- the shop
    bool can_take(const Offer& o) const;
    // wedge_uid: the wedge an upgrade or a Zap applies to. choice: Recut's value, Paint's color.
    bool upgrade_fits(const Offer& o, const Wedge& w) const;
    bool buy(int offer, int wedge_uid = 0, int choice = 0);
    int sell_value(const OwnedItem& it) const;
    bool sell_item(int index);
    bool move_item(int from, int to);
    bool reroll();
    bool leave_shop(); // starts the next round
    bool go_endless(); // from Won: carry on past the last round

    Wedge* find_wedge(int uid);
    std::string wedge_label(const Wedge& w) const;

private:
    void start_round();
    void begin_trip();
    void end_trip();
    void end_round(bool won);
    void roll_offers();
    Offer roll_offer();
    Rarity pick_rarity(bool has[3]);
    bool is_locked(const std::string& id) const;
    void extend_route(int upto);
    SpinResult land(int wheel, int index, double chance, SpinResult r);
    void spoil(SpinResult& r);
    Income income(int unused_trips) const;
    bool rescue_possible() const;
};

extern const char* const kColorNames[4];   // "Red", ...
extern const char kColorLetters[5];        // "RGBY"

} // namespace spin
