#include "game_manager.h"
#include "0.h"
#include "1.h"

namespace {
void start() {}
void tick() {}

GameDefinition definition{
    "test_game",
    "Test Game",
    start,
    tick,
};

struct Registrar {
    Registrar() { game_manager_register(&definition); }
} registrar;
}
