#pragma once

#include <cstddef>

struct GameDefinition {
    const char* id;
    const char* name;
    void (*start)();
    void (*tick)();
};

void game_manager_register(const GameDefinition* game);
void game_manager_init();
void game_manager_tick();
std::size_t game_manager_count();
const GameDefinition* game_manager_get(std::size_t index);
const GameDefinition* game_manager_find(const char* id);
