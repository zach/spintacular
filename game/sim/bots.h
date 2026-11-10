// The balance simulation's bots: stand-ins for kinds of players
// (docs/design/session-rubric.md). They are ports of the prototype's bots
// (prototypes/haul/haul.py) and play a spin::Run only through its actions.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rules/run.h"

namespace spin::sim {

// What the engine bots saw and bought in the shop, for the rubric and the telemetry.
struct Stats {
    std::map<std::string, long long> offered, picked; // by content id
    long long shops = 0, shops_with_fit = 0, shops_with_choice = 0;

    void merge(const Stats& o);
    void count_offers(const Run& run); // call when a shop opens or is rerolled
};

// The passive bot: buys nothing. Presses its luck by expected value, one spin ahead.
class Bot {
public:
    explicit Bot(Rng rng) : rng(rng) {}
    virtual ~Bot() = default;

    std::string name = "passive";
    double caution = 1.0;      // 0: never delivers by choice
    double freight_risk = 0.0; // the most risk it takes for a freight bonus once the round is already won

    // One decision in a Trip, Rescue or Rod phase: spin or deliver, rescue or not, zap or not.
    void act(Run& run);
    // The whole visit to the shop, except leaving it.
    virtual void shop(Run& run, Stats& stats);

    // The wheel to spin next, or -1 to deliver.
    int choose(Run& run);
    virtual bool use_fix(const Run& run) const;            // spend a Nudge or a Respin on this repeat?
    virtual bool use_band(const Run& run, double p) const; // spend an Outer Band on a spin with chance p to spoil?
    // The Rod: the uid of the delivered wedge to remove, or 0 to keep them all.
    virtual int zap_choice(Run& run, const std::vector<int>& delivered_uids);

protected:
    Rng rng; // the bot's own choices never touch the run's streams
};

// Buys each offer on a coin flip, with random targets.
class RandomBot : public Bot {
public:
    explicit RandomBot(Rng rng);
    void shop(Run& run, Stats& stats) override;
};

enum class Focus { Color, Straight, Safety, Quota, Adaptive, Flat };
enum class Style { Spender, Saver, Reroller, Bargain };

const char* focus_name(Focus f);
const char* style_name(Style s);

// Shops toward a focus with an economy style.
//   focus: color (one color) | straight | safety | quota | adaptive (follows the shop) | flat (no engine)
//   style: spender | saver | reroller | bargain (docs/design/economy.md)
class Builder : public Bot {
public:
    Builder(Rng rng, Focus focus, Style style);

    Focus focus;
    Style style;
    int color; // the color a color build collects

    void shop(Run& run, Stats& stats) override;
    int zap_choice(Run& run, const std::vector<int>& delivered_uids) override;

    // The build the bot is on now. An adaptive bot commits once its items lean one way.
    Focus kind(const Run& run) const;
    double item_fit(const ItemDef& d, Focus kind) const;
    // Whether a wedge belongs in this build's bag.
    bool wedge_ok(const Run& run, const Wedge& w, Focus kind) const;
    // How well an offer fits the build: 2 is good, 3 and up is a find, 0 is never.
    double fit(const Run& run, const Offer& o, Focus kind) const;
    // The wedge (uid, 0 for none) and the choice an upgrade or a Zap should take.
    void target_for(const Run& run, const Offer& o, Focus kind, int& wedge_uid, int& choice);
    // Whether the style spends on an offer of this fit.
    bool wants(const Run& run, const Offer& o, double fit) const;
};

// An insane risk: never delivers by choice. Only a full truck or an empty tank ends a load.
class Reckless : public Builder {
public:
    explicit Reckless(Rng rng);
};

// "passive", "random", "reckless" or "<focus>/<style>". Throws std::runtime_error on another name.
std::unique_ptr<Bot> make_bot(const std::string& name, Rng rng);

} // namespace spin::sim
