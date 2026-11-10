// What carries over between runs: unlocks, the collection, the best stake won
// with each rig, and settings (docs/design/progression.md). Nothing in it
// makes a run stronger.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "rules/tables.h"

namespace spin {

struct Settings {
    float master = 0.8f, music = 0.6f, sfx = 0.9f;
    int speed = 2; // animation speed: 0 instant, 1 fast, 2 normal, 3 slow
    bool fullscreen = false;
};

struct Profile {
    std::vector<std::string> seen;     // every piece of content the player has met
    std::vector<std::string> unlocked; // locked content earned in play
    std::map<std::string, int> best_stake; // rig id -> highest stake won
    int runs = 0, wins = 0;
    long long best_delivery = 0;
    int best_round = 0;       // furthest round reached
    std::string daily_date;   // the last daily haul played, YYYYMMDD
    std::string daily_result; // and how it went
    Settings settings;

    bool has_seen(const std::string& id) const;
    bool is_unlocked(const std::string& id) const;
    void see(const std::string& id);
    // Returns true if this unlocked something new.
    bool unlock(const std::string& id);
    // Content a new run's shop must not offer: the tables' locked list, less what's unlocked.
    std::vector<std::string> locked_now(const Tables& t) const;
    // Rigs unlock by winning: each win opens the next one.
    bool rig_unlocked(const Tables& t, const std::string& rig) const;
    // Winning a stake with a rig opens the next stake for that rig.
    int max_stake(const Tables& t, const std::string& rig) const;
};

std::string profile_to_json(const Profile& p);
Profile profile_from_json(const std::string& text); // a bad or empty text gives a fresh profile

} // namespace spin
