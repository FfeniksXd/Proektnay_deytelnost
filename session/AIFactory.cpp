#include "AIFactory.h"
#include "EasyAI.h"
#include "MediumAI.h"
#include "HardAI.h"

std::unique_ptr<IPlayer> createAI(int difficulty) {
    switch (difficulty) {
        case 2:  return std::make_unique<MediumAI>();
        case 3:  return std::make_unique<HardAI>();
        case 1:
        default: return std::make_unique<EasyAI>();
    }
}