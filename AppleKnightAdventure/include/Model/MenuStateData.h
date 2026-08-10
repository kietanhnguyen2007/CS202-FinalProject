#pragma once
#include <string>
#include "Model/SaveManager.h"

struct MenuStateData {
    std::string playerName = "Player";
    int coins = 0;
    bool isFirstTimePlaying = true;
    int currentLevel = 1;
    int totalLevels = 3;
    
    // Populated from SaveManager by MenuController
    static MenuStateData FromSave() {
        MenuStateData data;
        auto& save = SaveManager::GetInstance();
        data.playerName = save.GetPlayerName();
        data.coins = save.GetCoins();
        data.isFirstTimePlaying = save.IsFirstTimePlaying();
        return data;
    }
};
