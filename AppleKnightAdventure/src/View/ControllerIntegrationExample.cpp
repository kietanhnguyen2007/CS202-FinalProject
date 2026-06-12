#include "View/HUDView.h"
#include "View/MenuView.h"
#include "View/InventoryView.h"
#include "View/FloatingText.h"
#include "View/EnemyStatusRenderer.h"
#include "View/ElementalFX.h"
#include "View/EntityRenderer.h"

// Example pseudocode showing how a controller should call view APIs
// This file is for documentation / developer reference and is not used at runtime.

void ExampleIntegration() {
    using namespace View;

    // After raylib InitWindow()
    MenuView::GetInstance().Init();
    MenuView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

    HUDView::GetInstance().Init();
    HUDView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

    InventoryView::GetInstance().Init();
    InventoryView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

    // Per-frame (controller/game loop): update + render
    // HUDView::Update(dt, playerPtr);
    // GameView::Render(camera, particles, dt);
    // HUDView::Render(); InventoryView::Render(); MenuView::Render();

    // When elemental reaction occurs in model / elemental system:
    // EnemyStatusRenderer::GetInstance().SetStatus(enemyId, enemyPos, true, false, false);
    // FloatingTextManager::GetInstance().Emit(enemyPos, "VAPORIZE!", GOLD, 1.0f);
    // ElementalFX::GetInstance().SetElementTint(playerWeaponEntityId, DamageType::Fire);

    // On chest opened:
    // EntityRenderer::GetInstance().UpdateSpriteRect(chestId, atlas->GetFrameRect("chest/open"));

    // On fake wall destroyed:
    // EntityRenderer::GetInstance().SetEntityVisible(wallId, false);
    // ParticleRenderer::GetInstance().EmitBurst(wallPos, 12);
}
