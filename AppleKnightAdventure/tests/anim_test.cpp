#include <iostream>
#include <memory>
#include <fstream>
#include "../../AppleKnightAdventure/include/Systems/AnimationSystem.h"
#include "../../AppleKnightAdventure/include/Systems/TextureAtlas.h"

using namespace Systems;

static void assert_true(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << std::endl;
        std::exit(1);
    }
}

int main() {
    Animator anim;
    auto clip = std::make_shared<AnimationClip>();
    clip->name = "test";
    // 3 frames, 0.1s each
    for (int i = 0; i < 3; ++i) {
        AnimationFrame f; f.src = {0,0,16,16}; f.duration = 0.1f; clip->frames.push_back(f);
    }
    clip->loop = true;
    anim.AddClip(clip);
    bool ok = anim.Play("test", 1.0f, true);
    assert_true(ok, "Play returned false for existing clip");

    // Advance 0.25s: should advance 2 frames (0->1->2) remaining time 0.05
    anim.Update(0.25f);
    assert_true(anim.GetCurrentFrameIndex() == 2, "Expected frame index 2 after 0.25s");
    float t = anim.GetPlayheadTime();
    assert_true(t > 0.049f && t < 0.06f, "Playhead time mismatch after update");

    // Seek to 0.05 should go to frame 0
    anim.Seek(0.05f);
    assert_true(anim.GetCurrentFrameIndex() == 0, "Seek to 0.05 should yield frame 0");

    // Large dt advance
    anim.Seek(0.0f);
    anim.Update(1.0f); // should loop many times but end on frame (1s / 0.1 = 10 -> frame  (10 % 3)=1 )
    assert_true(anim.GetCurrentFrameIndex() == 1, "Large dt should wrap correctly");

    // Test atlas JSON parsing (metadata) without loading a texture file
    // Create a small JSON file in the build/test directory
    const char* jsonPath = "test_atlas.json";
    std::ofstream jf(jsonPath);
    jf << R"({
      "frames": {
        "player_0": {
          "frame": {"x": 0, "y": 0, "w": 16, "h": 16},
          "rotated": false,
          "trimmed": true,
          "spriteSourceSize": {"x": 1, "y": 2, "w": 14, "h": 14},
          "sourceSize": {"w": 16, "h": 16}
        }
      },
      "clips": {
        "walk": { "frames": ["player_0"], "durations": [0.1], "loop": true }
      }
    })";
    jf.close();

    auto atlas = Systems::TextureAtlas::LoadFromJSON(jsonPath);
    assert_true(atlas != nullptr, "Atlas should parse and return a TextureAtlas (even without texture)");
    assert_true(atlas->HasFrame("player_0"), "Atlas should contain player_0 frame");
    auto clip2 = atlas->GetClip("walk");
    assert_true(clip2 != nullptr, "Atlas should contain walk clip");
    assert_true(!clip2->frames.empty(), "Clip frames not empty");
    assert_true(clip2->frames[0].trimmed == true, "Trimmed flag should be true");
    assert_true((int)clip2->frames[0].originalSize.x == 16, "Original width should be 16");
    assert_true((int)clip2->frames[0].spriteSourceSize.x == 1, "spriteSourceSize.x should be 1");

    std::cout << "PASS: anim tests" << std::endl;
    return 0;
}
