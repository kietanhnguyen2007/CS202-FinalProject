#include "Controller/PrepareController.h"
#include "View/PrepareView.h"
#include "Model/SaveManager.h"
#include "Systems/SoundManager.h"
#include <raylib.h>
#include <algorithm>

PrepareController& PrepareController::GetInstance() {
    static PrepareController instance;
    return instance;
}

bool PrepareController::Init() {
    m_charItems = {
        {"knight", "Knight", "assets/textures/player/knight/idle.json", true},
        {"fighter", "Fighter", "assets/textures/player/fighter/idle.json", false},
        {"magic_caster", "Mage", "assets/textures/player/magic_caster/idle.json", false},
        {"ninja", "Ninja", "assets/textures/player/ninja/idle.json", false}
    };
    
    m_petItems = {
        {"skull", "Skull", "assets/textures/pets/skull/idle.json", false},
        {"ghost", "Ghost", "assets/textures/pets/ghost/idle.json", false},
        {"baby_dragon", "Dragon", "assets/textures/pets/baby_dragon/idle.json", false},
        {"fairy", "Fairy", "assets/textures/pets/fairy/idle.json", false}
    };
    
    return View::PrepareView::GetInstance().Init();
}

void PrepareController::Open(int levelId) {
    m_levelId = levelId;
    m_wantsBack = false;
    m_wantsStart = false;
    m_focusColumn = 0;
    m_inputCooldown = 0.0f;
    
    auto& saveMgr = SaveManager::GetInstance();
    
    // Update unlock status
    for (auto& item : m_charItems) {
        item.isUnlocked = saveMgr.IsCharUnlocked(item.id);
    }
    // Knight is always unlocked just in case
    if (!m_charItems.empty()) m_charItems[0].isUnlocked = true;
    
    for (auto& item : m_petItems) {
        item.isUnlocked = saveMgr.IsCharUnlocked("pet_" + item.id);
    }
    
    std::string savedChar = saveMgr.GetSelectedChar();
    std::string savedPet = saveMgr.GetSelectedPet();
    
    m_selectedCharIdx = 0; // Default to knight
    for (size_t i = 0; i < m_charItems.size(); ++i) {
        if (m_charItems[i].id == savedChar && m_charItems[i].isUnlocked) {
            m_selectedCharIdx = (int)i;
            break;
        }
    }
    
    m_selectedPetIdx = -1; // Default to none
    for (size_t i = 0; i < m_petItems.size(); ++i) {
        if (m_petItems[i].id == savedPet && m_petItems[i].isUnlocked) {
            m_selectedPetIdx = (int)i;
            break;
        }
    }
    
    View::PrepareView::GetInstance().Show();
}

void PrepareController::Update(float dt) {
    View::PrepareView::GetInstance().Update(dt);
    
    if (m_inputCooldown > 0.f) {
        m_inputCooldown -= dt;
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        SetWantsBack();
        return;
    }
    
    // Keyboard navigation
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        m_focusColumn = (m_focusColumn + 1) % 3;
        SoundManager::GetInstance().PlaySound("click");
    }
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        m_focusColumn = (m_focusColumn + 2) % 3;
        SoundManager::GetInstance().PlaySound("click");
    }
    
    if (m_focusColumn == 0) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            MoveCharacterSelection(-1);
            SoundManager::GetInstance().PlaySound("hover");
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            MoveCharacterSelection(1);
            SoundManager::GetInstance().PlaySound("hover");
        }
    }
    else if (m_focusColumn == 1) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            MovePetSelection(-1);
            SoundManager::GetInstance().PlaySound("hover");
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            MovePetSelection(1);
            SoundManager::GetInstance().PlaySound("hover");
        }
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (m_focusColumn == 2) {
            TryStartGame();
        } else {
            // Selecting an item in columns 0 or 1
            SoundManager::GetInstance().PlaySound("click");
        }
    }
}

bool PrepareController::SetSelectedCharIdx(int idx) {
    if (idx >= 0 && idx < (int)m_charItems.size() && m_charItems[idx].isUnlocked) {
        m_selectedCharIdx = idx;
        return true;
    }
    return false;
}

bool PrepareController::SetSelectedPetIdx(int idx) {
    if (idx >= -1 && idx < (int)m_petItems.size()) {
        if (idx == -1 || m_petItems[idx].isUnlocked) {
            m_selectedPetIdx = idx;
            return true;
        }
    }
    return false;
}

void PrepareController::MoveCharacterSelection(int direction) {
    if (m_charItems.empty() || direction == 0) return;
    int idx = m_selectedCharIdx;
    for (int step = 0; step < (int)m_charItems.size(); ++step) {
        idx = std::clamp(idx + direction, 0, (int)m_charItems.size() - 1);
        if (m_charItems[idx].isUnlocked) {
            m_selectedCharIdx = idx;
            return;
        }
        if (idx == 0 || idx == (int)m_charItems.size() - 1) return;
    }
}

void PrepareController::MovePetSelection(int direction) {
    if (direction == 0) return;
    int idx = m_selectedPetIdx;
    for (int step = 0; step <= (int)m_petItems.size(); ++step) {
        idx = std::clamp(idx + direction, -1, (int)m_petItems.size() - 1);
        if (idx == -1 || m_petItems[idx].isUnlocked) {
            m_selectedPetIdx = idx;
            return;
        }
        if (idx == -1 || idx == (int)m_petItems.size() - 1) return;
    }
}

void PrepareController::SetFocusColumn(int col) {
    m_focusColumn = std::clamp(col, 0, 2);
}

void PrepareController::TryStartGame() {
    if (m_selectedCharIdx >= 0 && m_selectedCharIdx < (int)m_charItems.size()) {
        const auto& charItem = m_charItems[m_selectedCharIdx];
        if (charItem.isUnlocked) {
            SaveManager::GetInstance().SetSelectedChar(charItem.id);
            
            if (m_selectedPetIdx >= 0 && m_selectedPetIdx < (int)m_petItems.size()) {
                const auto& petItem = m_petItems[m_selectedPetIdx];
                if (petItem.isUnlocked) {
                    SaveManager::GetInstance().SetSelectedPet(petItem.id);
                } else {
                    SaveManager::GetInstance().SetSelectedPet("");
                }
            } else {
                SaveManager::GetInstance().SetSelectedPet("");
            }
            // Persist the loadout before entering gameplay, not only on a
            // graceful application shutdown.
            SaveManager::GetInstance().Save();
            m_wantsStart = true;
            SoundManager::GetInstance().PlaySound("start");
        }
    }
}
