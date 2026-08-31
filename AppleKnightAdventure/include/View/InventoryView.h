#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

class Inventory;

namespace View {

struct ItemIconInfo {
    std::shared_ptr<Animations::TextureAtlas> atlas;
    Animations::Animator anim;
    bool animated = false;
};

class InventoryView {
public:
    static InventoryView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Open();
    void Close();
    bool IsOpen() const;

    void Update(float dt);
    void Render();

    void SetInventorySnapshot(const Inventory& snapshot);
    void SetSelectionIndex(int index);

    void RegisterOnRequestUseItem(std::function<void(int)> cb);

private:
    InventoryView() = default;

    void LoadItemAtlases();
    void DrawSlot(float x, float y, float size, bool highlighted);

    bool m_open = false;
    int m_selection = -1;
    std::function<void(int)> m_onRequestUse;
    std::vector<std::pair<std::string,int>> m_items;
    bool m_loaded = false;

    std::unordered_map<std::string, ItemIconInfo> m_itemIcons;

};

} // namespace View
