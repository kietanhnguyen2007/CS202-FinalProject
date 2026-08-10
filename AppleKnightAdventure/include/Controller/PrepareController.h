#pragma once
#include <string>
#include <vector>

struct PrepareItemData {
    std::string id;
    std::string displayName;
    std::string idlePath;
    bool isUnlocked = false;
};

class PrepareController {
public:
    static PrepareController& GetInstance();
    bool Init();
    void Open(int levelId);
    void Update(float dt);
    
    bool ShouldReturnToMenu() const { return m_wantsBack; }
    bool ShouldStartGame() const { return m_wantsStart; }
    int GetLevelId() const { return m_levelId; }
    
    // View getters
    const std::vector<PrepareItemData>& GetCharItems() const { return m_charItems; }
    const std::vector<PrepareItemData>& GetPetItems() const { return m_petItems; }
    int GetSelectedCharIdx() const { return m_selectedCharIdx; }
    int GetSelectedPetIdx() const { return m_selectedPetIdx; }
    int GetFocusColumn() const { return m_focusColumn; } // 0=Chars, 1=Pets, 2=Start Btn

    // Setters for view clicks
    void SetSelectedCharIdx(int idx);
    void SetSelectedPetIdx(int idx);
    void SetFocusColumn(int col);
    void SetWantsBack() { m_wantsBack = true; }
    void TryStartGame();

private:
    PrepareController() = default;
    
    int m_levelId = 1;
    bool m_wantsBack = false;
    bool m_wantsStart = false;
    
    std::vector<PrepareItemData> m_charItems;
    std::vector<PrepareItemData> m_petItems;
    
    int m_selectedCharIdx = 0;
    int m_selectedPetIdx = -1;
    int m_focusColumn = 0;
    float m_inputCooldown = 0.f;
};
