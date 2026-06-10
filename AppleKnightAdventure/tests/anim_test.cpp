#include <iostream>
#include <memory>
#include <fstream>
#include <cmath>
#include "Systems/AnimationSystem.h"
#include "Systems/TextureAtlas.h"

using namespace Systems;

static int g_failures = 0;
static int g_tests = 0;

static void assert_true(bool cond, const char* msg) {
    g_tests++;
    if (!cond) { std::cerr << "FAIL: " << msg << std::endl; g_failures++; }
}

static void assert_int_eq(int expected, int actual, const char* msg) {
    g_tests++;
    if (expected != actual) {
        std::cerr << "FAIL: " << msg << " (expected " << expected << " got " << actual << ")" << std::endl;
        g_failures++;
    }
}

static void assert_float_approx(float expected, float actual, float eps, const char* msg) {
    g_tests++;
    if (std::abs(expected - actual) > eps) {
        std::cerr << "FAIL: " << msg << " (expected " << expected << " got " << actual << ")" << std::endl;
        g_failures++;
    }
}

// Write content to a temp file and return its path.
static std::string writeTempFile(const std::string& content) {
    std::string path = "_anim_test_temp.json";
    std::ofstream f(path);
    f << content;
    return path;
}

int main() {
    // ------------------------------------------------------------------
    // Test 1: Animator core (multi-frame catch-up, Seek, SetFrame, events)
    // ------------------------------------------------------------------
    {
        Animator anim;
        auto clip = std::make_shared<AnimationClip>();
        clip->name = "test";
        for (int i = 0; i < 3; ++i) {
            AnimationFrame f; f.src = {0,0,16,16}; f.duration = 0.1f; clip->frames.push_back(f);
        }
        clip->loop = true;
        anim.AddClip(clip);

        // Play returns true for existing clip
        bool ok = anim.Play("test", 1.0f, true);
        assert_true(ok, "Play returned true for existing clip");

        // Play returns false for missing clip
        ok = anim.Play("nonexistent", 1.0f, true);
        assert_true(!ok, "Play returned false for missing clip");

        // Multi-frame catch-up: 0.25s -> advance 2 frames (idx 0->1->2), remaining timer ~0.05
        anim.Update(0.25f);
        assert_int_eq(2, anim.GetCurrentFrameIndex(), "Frame index 2 after 0.25s (multi-frame catch-up)");
        float t = anim.GetPlayheadTime();
        assert_float_approx(0.25f, t, 0.001f, "Playhead time 0.25s after update");

        // Seek to 0.05 should land on frame 0 with timer=0.05
        anim.Seek(0.05f);
        assert_int_eq(0, anim.GetCurrentFrameIndex(), "Seek(0.05) -> frame 0");
        assert_float_approx(0.05f, anim.GetPlayheadTime(), 0.001f, "Playhead time after seek");

        // SetFrame
        anim.SetFrame(1);
        assert_int_eq(1, anim.GetCurrentFrameIndex(), "SetFrame(1) -> frame 1");
        assert_float_approx(0.1f, anim.GetPlayheadTime(), 0.001f, "Playhead time after SetFrame(1)");

        // Large dt wrapping: 1.0s over 0.1s/frame = 10 frames, loop 3 -> (10 % 3) = 1
        anim.Seek(0.0f);
        anim.Update(1.0f);
        assert_int_eq(1, anim.GetCurrentFrameIndex(), "Large dt 1.0s -> frame 1 (10 mod 3)");

        // Events
        int changeCount = 0;
        std::string lastChangeClip;
        anim.Stop();
        anim.Seek(0.0f);
        anim.OnFrameChanged = [&](const std::string& name, int idx) {
            changeCount++;
            lastChangeClip = name;
        };
        bool finished = false;
        anim.OnClipFinished = [&](const std::string& name) { finished = true; };

        // Non-looping clip
        auto noclip = std::make_shared<AnimationClip>();
        noclip->name = "noloop";
        noclip->loop = false;
        for (int i = 0; i < 3; ++i) {
            AnimationFrame f; f.src = {0,0,16,16}; f.duration = 0.1f; noclip->frames.push_back(f);
        }
        anim.AddClip(noclip);
        anim.Play("noloop", 1.0f, true);
        // dt = 0.25 should advance 2 frames (changeCount=2), frame index=2 (not finished yet)
        anim.Update(0.25f);
        assert_int_eq(2, anim.GetCurrentFrameIndex(), "Non-looping frame index after 0.25s");
        assert_true(changeCount >= 1, "OnFrameChanged called at least once");
        assert_true(!finished, "OnClipFinished not yet called");

        // Advance remaining 0.1s -> hits end, OnClipFinished fires
        anim.Update(0.1f);
        assert_true(anim.GetCurrentFrameIndex() == 2, "Non-looping stays on last frame");
        assert_true(finished, "OnClipFinished called after clip ends");

        // Pause / Resume
        anim.Play("test", 1.0f, true);
        anim.Pause();
        assert_true(!anim.IsPlaying(), "Paused: not playing");
        anim.Resume();
        assert_true(anim.IsPlaying(), "Resumed: playing");
    }

    // ------------------------------------------------------------------
    // Test 3: Playback modes (Reverse, PingPong)
    // ------------------------------------------------------------------
    {
        Animator anim;
        auto clip = std::make_shared<AnimationClip>();
        clip->name = "modes";
        for (int i = 0; i < 3; ++i) { AnimationFrame f; f.src = {0,0,16,16}; f.duration = 0.1f; clip->frames.push_back(f); }
        clip->loop = true;
        anim.AddClip(clip);

        // Reverse mode: start at end when reset
        anim.SetPlaybackMode(Animator::PlaybackMode::Reverse);
        bool ok = anim.Play("modes", 1.0f, true);
        assert_true(ok, "Play modes reverse");
        // advance 0.15s backwards -> from ~0.3 to ~0.15 -> should be frame index 1
        anim.Update(0.15f);
        assert_int_eq(1, anim.GetCurrentFrameIndex(), "Reverse mode: frame index after 0.15s");

        // PingPong: start forward, then bounce back after passing end
        anim.SetPlaybackMode(Animator::PlaybackMode::PingPong);
        anim.Play("modes", 1.0f, true);
        // advance 0.35s: forward to end (0.3) then reflected to 0.25 -> frame index 2
        anim.Update(0.35f);
        assert_true(anim.GetCurrentFrameIndex() == 2, "PingPong mode: frame index after 0.35s should be 2");
    }

    // ------------------------------------------------------------------
    // Test 2: Atlas JSON parsing w/ trimmed/rotated metadata
    // ------------------------------------------------------------------
    {
        const char* jsonContent = R"({
            "image": "",
            "frames": {
                "player_0": {
                    "frame": {"x":0,"y":0,"w":16,"h":16},
                    "rotated": false,
                    "trimmed": true,
                    "spriteSourceSize": {"x":1,"y":2,"w":14,"h":14},
                    "sourceSize": {"w":16,"h":16}
                },
                "player_1": {
                    "frame": {"x":16,"y":0,"w":16,"h":16},
                    "rotated": true,
                    "trimmed": false,
                    "spriteSourceSize": {"x":0,"y":0,"w":16,"h":16},
                    "sourceSize": {"w":16,"h":16}
                },
                "player_2": {
                    "frame": {"x":32,"y":0,"w":16,"h":16}
                }
            },
            "clips": {
                "walk": {
                    "frames": ["player_0","player_1"],
                    "durations": [0.15, 0.2],
                    "loop": true
                }
            }
        })";

        std::string tmpFile = writeTempFile(jsonContent);
        auto atlas = TextureAtlas::LoadFromJSON(tmpFile);
        std::remove(tmpFile.c_str());

        assert_true(atlas != nullptr, "Atlas loaded from JSON");
        assert_true(atlas->HasFrame("player_0"), "HasFrame player_0");
        assert_true(atlas->HasFrame("player_1"), "HasFrame player_1");
        assert_true(atlas->HasFrame("player_2"), "HasFrame player_2");
        assert_true(atlas->HasClip("walk"), "HasClip walk");

        // Frame rects
        Rectangle r = atlas->GetFrameRect("player_0");
        assert_int_eq(0, (int)r.x, "player_0 rect x");
        assert_int_eq(0, (int)r.y, "player_0 rect y");
        assert_int_eq(16, (int)r.width, "player_0 rect w");
        assert_int_eq(16, (int)r.height, "player_0 rect h");

        // Clip duration parsing
        auto clip = atlas->GetClip("walk");
        assert_true(clip != nullptr, "GetClip walk non-null");
        assert_int_eq(2, (int)clip->frames.size(), "walk has 2 frames");
        assert_float_approx(0.15f, clip->frames[0].duration, 0.001f, "walk frame 0 duration");
        assert_float_approx(0.20f, clip->frames[1].duration, 0.001f, "walk frame 1 duration");
        assert_true(clip->loop, "walk loop = true");

        // Frame metadata populated from frames object
        if (clip->frames.size() >= 2) {
            auto& f0 = clip->frames[0];
            assert_true(f0.trimmed, "player_0 trimmed");
            assert_true(!f0.rotated, "player_0 not rotated");
            assert_float_approx(1.0f, f0.spriteSourceSize.x, 0.01f, "player_0 ss.x");
            assert_float_approx(2.0f, f0.spriteSourceSize.y, 0.01f, "player_0 ss.y");
            assert_float_approx(16.0f, f0.originalSize.x, 0.01f, "player_0 os.w");
            assert_float_approx(16.0f, f0.originalSize.y, 0.01f, "player_0 os.h");
            // origin from spriteSourceSize when originalSize is present
            assert_float_approx(1.0f, f0.origin.x, 0.01f, "player_0 origin.x from ss");
            assert_float_approx(2.0f, f0.origin.y, 0.01f, "player_0 origin.y from ss");

            auto& f1 = clip->frames[1];
            assert_true(!f1.trimmed, "player_1 not trimmed");
            assert_true(f1.rotated, "player_1 rotated");
        }
    }

    // ------------------------------------------------------------------
    // Summary
    // ------------------------------------------------------------------
    if (g_failures == 0) {
        std::cout << "PASS: all " << g_tests << " tests passed" << std::endl;
        return 0;
    } else {
        std::cerr << "FAIL: " << g_failures << " of " << g_tests << " tests failed" << std::endl;
        return 1;
    }
}
