#include "profile.h"

#include <algorithm>

#include <nlohmann/json.hpp>

namespace spin {

using json = nlohmann::json;

namespace {
bool in(const std::vector<std::string>& v, const std::string& id) { return std::find(v.begin(), v.end(), id) != v.end(); }
} // namespace

bool Profile::has_seen(const std::string& id) const { return in(seen, id); }
bool Profile::is_unlocked(const std::string& id) const { return in(unlocked, id); }
void Profile::see(const std::string& id) {
    if (!in(seen, id)) seen.push_back(id);
}
bool Profile::unlock(const std::string& id) {
    if (in(unlocked, id)) return false;
    unlocked.push_back(id);
    return true;
}

std::vector<std::string> Profile::locked_now(const Tables& t) const {
    std::vector<std::string> out;
    for (auto& id : t.locked)
        if (!in(unlocked, id)) out.push_back(id);
    return out;
}

bool Profile::rig_unlocked(const Tables& t, const std::string& rig) const {
    for (std::size_t i = 0; i < t.rigs.size(); ++i)
        if (t.rigs[i].id == rig) return static_cast<int>(i) <= wins;
    return false;
}

int Profile::max_stake(const Tables& t, const std::string& rig) const {
    auto it = best_stake.find(rig);
    int best = it == best_stake.end() ? 0 : it->second;
    return std::min(best + 1, static_cast<int>(t.stakes.size()) + 1);
}

std::string profile_to_json(const Profile& p) {
    json j;
    j["format"] = "spintacular-profile/1";
    j["seen"] = p.seen;
    j["unlocked"] = p.unlocked;
    j["best_stake"] = p.best_stake;
    j["runs"] = p.runs;
    j["wins"] = p.wins;
    j["best_delivery"] = p.best_delivery;
    j["best_round"] = p.best_round;
    j["daily_date"] = p.daily_date;
    j["daily_result"] = p.daily_result;
    j["settings"] = {{"master", p.settings.master}, {"music", p.settings.music}, {"sfx", p.settings.sfx},
                     {"speed", p.settings.speed}, {"fullscreen", p.settings.fullscreen}};
    return j.dump(1);
}

Profile profile_from_json(const std::string& text) {
    Profile p;
    try {
        json j = json::parse(text);
        if (j.value("format", "") != "spintacular-profile/1") return p;
        p.seen = j.value("seen", p.seen);
        p.unlocked = j.value("unlocked", p.unlocked);
        p.best_stake = j.value("best_stake", p.best_stake);
        p.runs = j.value("runs", 0);
        p.wins = j.value("wins", 0);
        p.best_delivery = j.value("best_delivery", 0LL);
        p.best_round = j.value("best_round", 0);
        p.daily_date = j.value("daily_date", "");
        p.daily_result = j.value("daily_result", "");
        if (j.contains("settings")) {
            const json& s = j["settings"];
            p.settings.master = s.value("master", p.settings.master);
            p.settings.music = s.value("music", p.settings.music);
            p.settings.sfx = s.value("sfx", p.settings.sfx);
            p.settings.speed = s.value("speed", p.settings.speed);
            p.settings.fullscreen = s.value("fullscreen", p.settings.fullscreen);
        }
    } catch (const json::exception&) {
        return Profile{};
    }
    return p;
}

} // namespace spin
