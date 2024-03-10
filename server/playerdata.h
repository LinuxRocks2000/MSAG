// player stats ONLY (no position or inventory or the like)
// I'm mainly writing this for note-taking purposes
#pragma once
#include <cstdint>

struct PlayerStats {
    float health;                    // the current health of the player. No larger than maxHealth, and if it goes below 0 you die (full sleep heals you to max).
    float heal;                      // in units of health/minute; base rate is 0.1 but this can be improved with potions. self-healing.
    float maxHealth;                 // starts at 10, increases slowly over the course of the game based on damage you take (taking hits makes you capable of taking more hits)

    float energy;                    // food, basically. eat to increase it. full sleep increases this to max (because I don't feel like adding a sleepiness stat)
    float energyBaseConsumption;     // resting caloric consumption. Decreases with exercise. you drain energy at 1.5x speed when health is below 3.0.
    // resting caloric burn is also increased if you're in the water, because you have to tread water, and increases with bad atmosphere effects.
    // if energy goes below 3.0, you can no longer sprint or yell. If your energy hits 0, you start taking damage and walk speed is significantly reduced.
    // if you get in the water, you drown; you can no longer tread water at this point. you *can* go to sleep on 0 energy, which is a recommended tactic.
    // another recommended tactic is to not run out of energy near any dangerous animals.

    float walkSpeed;                 // speed you walk at. Walking or slower drains very little energy. When health is below 3.0, this is cut in half.
    float sprintSpeed;               // speed you sprint at. Sprinting consumes a lot of energy. When health is below 3.0, you cannot sprint.
    // walking and sprinting apply to two-speed swimming as well, but their energy consumption is increased by 50% and their speed is decreased 50%. swimming is not recommended.

    float force;                     // how hard you can swing a sword, draw back an arrow, etc. This improves damage on most manual weapons. Automatic ones will
    // not be improved. If your health or energy goes below 3.0, force is ridiculously reduced. If energy goes to 0, force is set to 0 as well. Technically,
    // if health goes to 0 force is set to 0 as well, because you're dead.

    // craft skills. Craft skills control what you can craft, obviously; if you have insufficient skills to craft something in any area, you can't craft it.
    // Not all craftable items should be visible to the player! Clients should only show items within a few craft skill points of being craftable (so players
    // know what they need to improve in the near-term). Craft skills increase by 1 every time you craft something using that skill, and by one extra if the
    // required craft skill is more than half of your skill: so if you're crafting something with metalworking level 6 and woodworking level 6, and you're at
    // iron level 7 and wood level 20, you get two points towards metalworking and one point twowards woodworking. This means players will relatively quickly be
    // able to make all the stuff they need for survival, but it'll be harder as they progress to learn more. You can also get large skill boosts from some items.
    uint16_t craft_woodworking; // skill with wood, duh
    uint16_t craft_metalworking; // skill with metals, duh
};