// =============================================================================
// ShopController.cpp — Apple Knight Adventure
// Owns ShopView. Reads/writes SaveManager for coin-gated purchases.
// =============================================================================
#include "Controller/ShopController.h"
#include "View/ShopView.h"
#include "Model/SaveManager.h"
#include "Systems/SoundManager.h"
#include "raylib.h"

// ─────────────────────────────────────────────────────────────────────────────
// Item definitions — tiers match the price proposal in requirements
// ─────────────────────────────────────────────────────────────────────────────
namespace {

struct CharDef {
    const char* id;
    const char* displayName;
    const char* idlePath;   // relative to project root
    int price;
    int hp, atk, speed;
    const char* desc;
};

static const CharDef kCharDefs[] = {
    {
        "knight",   "Knight",
        "assets/textures/player/knight/idle.json",
        0, 150, 70, 100,
        "Stalwart warrior. High defence, balanced offence."
    },
    {
        "fighter",  "Fighter",
        "assets/textures/player/fighter/idle.json",
        50, 120, 120, 130,
        "Aggressive brawler. Lower defence, very high damage."
    },
    {
        "magic_caster", "Magic Caster",
        "assets/textures/player/magic_caster/idle.json",
        100, 90, 160, 110,
        "Long-range mage. Fragile but devastating spells."
    },
    {
        "ninja",    "Ninja",
        "assets/textures/player/ninja/idle.json",
        150, 100, 130, 190,
        "Elusive assassin. Extreme speed and burst damage."
    },
};

struct PetDef {
    const char* id;
    const char* displayName;
    const char* idlePath;
    int price;
    int hp, atk, speed;
    const char* desc;
};

static const PetDef kPetDefs[] = {
    {
        "skull",       "Skull",
        "assets/textures/pets/skull/idle.json",
        0, 60, 50, 120,
        "Floats beside you, fires bone shards at enemies."
    },
    {
        "ghost",       "Ghost",
        "assets/textures/pets/ghost/idle.json",
        50, 80, 40, 140,
        "Heals you over time. Phases through walls."
    },
    {
        "baby_dragon", "Baby Dragon",
        "assets/textures/pets/baby_dragon/idle.json",
        100, 100, 90, 100,
        "Breathes fire. Loyal and ferocious."
    },
    {
        "fairy",       "Fairy",
        "assets/textures/pets/fairy/idle.json",
        150, 70, 60, 160,
        "Collects nearby items automatically. Very fast."
    },
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
ShopController& ShopController::GetInstance() {
    static ShopController inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ShopController::Init() {
    View::ShopView::GetInstance().Init();
    BuildItemLists();
    m_initialized  = true;
    m_returnToMenu = false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopController::Shutdown() {
    View::ShopView::GetInstance().Shutdown();
    m_initialized = false;
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopController::Open() {
    m_returnToMenu = false;
    BuildItemLists();   // re-sync unlock state from SaveManager
    int coins = SaveManager::GetInstance().GetCoins();
    View::ShopView::GetInstance().Show(coins);
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopController::BuildItemLists() {
    auto& save = SaveManager::GetInstance();
    auto& shop = View::ShopView::GetInstance();

    // Characters
    std::vector<View::ShopItemData> charItems;
    for (const auto& def : kCharDefs) {
        View::ShopItemData item;
        item.id           = def.id;
        item.displayName  = def.displayName;
        item.idleAtlasPath = def.idlePath;
        item.price        = def.price;
        item.isUnlocked   = (def.price == 0) || save.IsCharUnlocked(def.id);
        item.statHP       = def.hp;
        item.statATK      = def.atk;
        item.statSpeed    = def.speed;
        item.description  = def.desc;
        charItems.push_back(std::move(item));
    }
    shop.SetCharacterItems(charItems);

    // Pets — use a separate "pet" prefix so SaveManager keys don't clash
    std::vector<View::ShopItemData> petItems;
    for (const auto& def : kPetDefs) {
        View::ShopItemData item;
        std::string petKey = std::string("pet_") + def.id;
        item.id           = petKey;
        item.displayName  = def.displayName;
        item.idleAtlasPath = def.idlePath;
        item.price        = def.price;
        item.isUnlocked   = (def.price == 0) || save.IsCharUnlocked(petKey.c_str());
        item.statHP       = def.hp;
        item.statATK      = def.atk;
        item.statSpeed    = def.speed;
        item.description  = def.desc;
        petItems.push_back(std::move(item));
    }
    shop.SetPetItems(petItems);
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopController::Update(float dt) {
    auto& shop = View::ShopView::GetInstance();
    shop.Update(dt);
    shop.Render();

    // Back
    if (shop.WantsBack()) {
        shop.ClearWantsBack();
        shop.SetVisible(false);
        m_returnToMenu = true;
        SaveManager::GetInstance().Save();
        return;
    }

    // Buy
    if (shop.WantsBuy()) {
        shop.ClearWantsBuy();
        HandleBuyAttempt();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void ShopController::HandleBuyAttempt() {
    auto& shop = View::ShopView::GetInstance();
    auto& save = SaveManager::GetInstance();

    View::ShopTab tab = shop.GetActiveTab();
    int idx = shop.GetSelectedIndex();

    // Get the item list pointers via the controller's local definitions
    const char* itemId   = nullptr;
    int         price    = 0;
    bool        unlocked = false;

    if (tab == View::ShopTab::Characters) {
        int nChars = (int)(sizeof(kCharDefs) / sizeof(kCharDefs[0]));
        if (idx < 0 || idx >= nChars) return;
        itemId   = kCharDefs[idx].id;
        price    = kCharDefs[idx].price;
        unlocked = (price == 0) || save.IsCharUnlocked(itemId);
    } else {
        int nPets = (int)(sizeof(kPetDefs) / sizeof(kPetDefs[0]));
        if (idx < 0 || idx >= nPets) return;
        std::string petKey = std::string("pet_") + kPetDefs[idx].id;
        itemId   = nullptr; // handled below
        price    = kPetDefs[idx].price;
        std::string pk = std::string("pet_") + kPetDefs[idx].id;
        unlocked = (price == 0) || save.IsCharUnlocked(pk.c_str());

        if (!unlocked) {
            if (save.GetCoins() >= price) {
                save.SpendCoins(price);
                save.UnlockChar(pk.c_str());
                save.Save();
                shop.MarkSelectedUnlocked();
                shop.SetCurrentCoins(save.GetCoins());
                auto& snd = SoundManager::GetInstance();
                if (snd.IsAudioInitialized()) snd.PlaySound("ui_confirm");
            } else {
                shop.TriggerBuyShake();
                auto& snd = SoundManager::GetInstance();
                if (snd.IsAudioInitialized()) snd.PlaySound("ui_error");
            }
        }
        return;
    }

    if (unlocked) return; // already owned, nothing to do

    if (save.GetCoins() >= price) {
        save.SpendCoins(price);
        save.UnlockChar(itemId);
        save.Save();
        shop.MarkSelectedUnlocked();
        shop.SetCurrentCoins(save.GetCoins());
        auto& snd = SoundManager::GetInstance();
        if (snd.IsAudioInitialized()) snd.PlaySound("ui_confirm");
    } else {
        shop.TriggerBuyShake();
        auto& snd = SoundManager::GetInstance();
        if (snd.IsAudioInitialized()) snd.PlaySound("ui_error");
    }
}
