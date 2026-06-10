# Player Assets — Note

Current spritesheets from **Kenney Pixel Platformer** (CC0). Each tile in
`characters.png` is a different character variant — there are no actual animation
frames per character.

All 4 JSON files currently reference a **single static tile** for every state
(idle/run/jump/attack/hurt). This means the character appears motionless on screen.

**To get real animation**, replace with multi-frame spritesheets (e.g. Kenney
Platformer Characters pack or custom art) and update the JSON files with proper
frame rects and clips.

Character → tile mapping (24×24 tiles with 1px padding in characters.png):
  - Knight:    tile 0,0      → [0,0,24,24]
  - Assassin:  tile 0,25     → [0,25,24,24]
  - Berserker: tile 0,50     → [0,50,24,24]
  - Mage:      tile 200,0    → [200,0,24,24]
