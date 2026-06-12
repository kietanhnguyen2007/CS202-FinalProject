#pragma once

#include "raylib.h"
#include "Systems/ObservableList.h"
#include <vector>
#include <functional>
#include <string>

class Inventory;
class Item;

namespace View {

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

    // Passive snapshot API
    void SetInventorySnapshot(const Inventory& snapshot);
    // Controller-owned selection index (row-major)
    void SetSelectionIndex(int index);

    // Callback registration for controller/model integration
    void RegisterInventoryChangedCallback(std::function<void()> cb);
    void UnregisterInventoryChangedCallback();

    // Bind to an ObservableList<const Item*> so view auto-updates on add/remove
    void AttachObservable(ObservableList<const Item*>* observable);
    void DetachObservable();

    // Optional view-initiated event when user requests to use an item
    void RegisterOnRequestUseItem(std::function<void(int)> cb);

private:
    InventoryView() = default;
    bool m_open = false;
    int m_selection = -1;
    std::function<void()> m_onInventoryChanged;
    std::function<void(int)> m_onRequestUse;
    // local snapshot cache (simple: store item ids/counts as strings for rendering)
    std::vector<std::pair<std::string,int>> m_items;
    bool m_loaded = false;

    ObservableList<const Item*>* m_attachedObservable = nullptr;
};

} // namespace View
