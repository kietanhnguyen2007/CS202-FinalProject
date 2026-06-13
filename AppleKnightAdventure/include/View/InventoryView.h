#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include "Systems/ObservableList.h"
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

class Inventory;
class Item;

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

    void RegisterInventoryChangedCallback(std::function<void()> cb);
    void UnregisterInventoryChangedCallback();

    void AttachObservable(ObservableList<const Item*>* observable);
    void DetachObservable();

    void RegisterOnRequestUseItem(std::function<void(int)> cb);

private:
    InventoryView() = default;

    void LoadItemAtlases();
    std::string AtlasPathForItem(const std::string& itemName) const;

    bool m_open = false;
    int m_selection = -1;
    std::function<void()> m_onInventoryChanged;
    std::function<void(int)> m_onRequestUse;
    std::vector<std::pair<std::string,int>> m_items;
    bool m_loaded = false;

    ObservableList<const Item*>* m_attachedObservable = nullptr;

    std::unordered_map<std::string, ItemIconInfo> m_itemIcons;
};

} // namespace View
