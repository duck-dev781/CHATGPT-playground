#include "game_manager.h"

#include <cstring>

namespace {
constexpr std::size_t MAX_GAMES = 32;
const GameDefinition* games[MAX_GAMES] = {};
std::size_t game_count = 0;
const GameDefinition* active_game = nullptr;
}

void game_manager_register(const GameDefinition* game) {
    if (!game || !game->id || game_count >= MAX_GAMES) return;
    for (std::size_t i = 0; i < game_count; ++i) {
        if (games[i] == game || std::strcmp(games[i]->id, game->id) == 0) return;
    }
    games[game_count++] = game;
}

void game_manager_init() {
    active_game = nullptr;
}

void game_manager_tick() {
    if (active_game && active_game->tick) active_game->tick();
}

std::size_t game_manager_count() { return game_count; }

const GameDefinition* game_manager_get(std::size_t index) {
    return index < game_count ? games[index] : nullptr;
}

const GameDefinition* game_manager_find(const char* id) {
    if (!id) return nullptr;
    for (std::size_t i = 0; i < game_count; ++i) {
        if (std::strcmp(games[i]->id, id) == 0) return games[i];
    }
    return nullptr;
}
