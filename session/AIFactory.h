#pragma once
#include "IPlayer.h"
#include <memory>

// Создаёт ИИ по уровню сложности: 1 = лёгкий, 2 = средний, 3 = сложный.
// Любое другое значение даёт лёгкий уровень.
std::unique_ptr<IPlayer> createAI(int difficulty);