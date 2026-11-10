// The tuning panel (F1): Dear ImGui over the game, for inspecting a run and
// trying balance numbers live. Not part of the game's own interface.
#include <imgui.h>

#include "data/tables_json.h"
#include "game.h"

namespace spin {

void debug_panel(Game& g) {
    ImGui::SetNextWindowSize(ImVec2(430, 560), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20, 120), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Tuning (F1)", &g.debug_open)) {
        ImGui::End();
        return;
    }
    ImGui::Text("%.1f fps, %d cues queued", ImGui::GetIO().Framerate, static_cast<int>(g.cues.size()));
    if (!g.has_run) {
        ImGui::TextWrapped("Start a run to inspect it. Changes here apply to the run in play only; assets/data/tables.json holds the real numbers.");
        if (ImGui::Button("Unlock everything")) {
            for (auto& id : g.tables.locked) g.profile.unlock(id);
            g.profile.wins = std::max(g.profile.wins, 5);
            for (auto& rig : g.tables.rigs) g.profile.best_stake[rig.id] = std::max(g.profile.best_stake[rig.id], 7);
            save_profile(g);
        }
        ImGui::End();
        return;
    }
    Run& r = g.run;
    static const char* phases[] = {"trip", "rescue", "rod", "shop", "won", "lost"};
    ImGui::Text("Seed %llu, %s rig, stake %d", static_cast<unsigned long long>(r.config.seed), r.config.rig.c_str(), r.config.stake);
    ImGui::Text("Round %d, phase %s, score %lld / %lld", r.round + 1, phases[static_cast<int>(r.phase)], r.round_score, r.target());
    ImGui::Text("Bag %d wedges, %d spins, %d deliveries, %d spoiled", static_cast<int>(r.bag.size()), r.m.spins, r.m.deliveries, r.m.spoiled);
    ImGui::Separator();
    if (ImGui::Button("+$10")) {
        r.money += 10;
        g.shown_money = r.money;
    }
    ImGui::SameLine();
    if (ImGui::Button("Refill tank") && r.phase == Phase::Trip) {
        r.load.spins = 0;
        sync_shown(g);
    }
    ImGui::SameLine();
    if (ImGui::Button("+1 trip") && r.phase == Phase::Trip) {
        ++r.trips_left;
        sync_shown(g);
    }
    if (ImGui::CollapsingHeader("This run's numbers", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Quota (this round)", &r.quota, 1, 80);
        ImGui::SliderInt("Tank (this round)", &r.tank, 1, 20);
        ImGui::SliderInt("Truck size", &r.t.scoring.truck_size, 3, 9);
        float quota_bonus = static_cast<float>(r.t.scoring.quota_bonus), crate = static_cast<float>(r.t.scoring.crate_mult),
              full = static_cast<float>(r.t.scoring.full_bonus);
        if (ImGui::SliderFloat("Quota pays x", &quota_bonus, 1.0f, 4.0f, "%.1f")) r.t.scoring.quota_bonus = quota_bonus;
        if (ImGui::SliderFloat("Crate x", &crate, 1.0f, 3.0f, "%.1f")) r.t.scoring.crate_mult = crate;
        if (ImGui::SliderFloat("Full truck x", &full, 1.0f, 4.0f, "%.1f")) r.t.scoring.full_bonus = full;
        ImGui::SliderInt("Freight $ per cargo", &r.t.income.freight_bonus, 0, 5);
        if (r.round < static_cast<int>(r.t.targets.size())) {
            int target = static_cast<int>(r.t.targets[r.round]);
            if (ImGui::InputInt("Target (this round)", &target, 10, 100)) r.t.targets[r.round] = std::max(1, target);
        }
    }
    if (ImGui::CollapsingHeader("Give")) {
        static int item = 0, wheel = 0, cons = 0;
        auto item_name = [](void* data, int i) { return static_cast<Tables*>(data)->items[i].name.c_str(); };
        ImGui::Combo("Item", &item, item_name, &r.t, static_cast<int>(r.t.items.size()));
        ImGui::SameLine();
        if (ImGui::Button("Add##item") && static_cast<int>(r.items.size()) < 8) r.items.push_back({r.t.items[item].id, 0, 0});
        auto wheel_name = [](void* data, int i) { return static_cast<Tables*>(data)->wheels[i].name.c_str(); };
        ImGui::Combo("Wheel", &wheel, wheel_name, &r.t, static_cast<int>(r.t.wheels.size()));
        ImGui::SameLine();
        if (ImGui::Button("Add##wheel") && static_cast<int>(r.wheels.size()) < r.t.run.max_wheels && r.phase == Phase::Shop)
            r.wheels.push_back({r.t.wheels[wheel].id, 0});
        ImGui::TextDisabled("Wheels join at the next deal: add them in the shop.");
        auto cons_name = [](void* data, int i) { return static_cast<Tables*>(data)->consumables[i].name.c_str(); };
        ImGui::Combo("Consumable", &cons, cons_name, &r.t, static_cast<int>(r.t.consumables.size()));
        ImGui::SameLine();
        if (ImGui::Button("Add##cons") && static_cast<int>(r.consumables.size()) < r.t.run.max_consumables)
            r.consumables.push_back(r.t.consumables[cons].id);
    }
    if (ImGui::CollapsingHeader("Tables file")) {
        ImGui::TextWrapped("%s/data/tables.json", g.asset_dir.c_str());
        if (ImGui::Button("Reload for the next run")) {
            try {
                g.tables_text = read_file(g.asset_dir + "/data/tables.json");
                g.tables = tables_from_json(g.tables_text);
                toast(g, "Tables reloaded", "They apply from the next run.");
            } catch (const std::exception& e) {
                toast(g, "Tables not reloaded", e.what());
            }
        }
    }
    ImGui::End();
}

} // namespace spin
