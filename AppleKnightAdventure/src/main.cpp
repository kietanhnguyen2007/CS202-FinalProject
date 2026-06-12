#include "View/Animator.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <functional>

using namespace View::Animations;

static int testsPassed = 0;
static int testsFailed = 0;

static void CHECK(bool cond, const char* name) {
    if (cond) {
        testsPassed++;
    } else {
        testsFailed++;
        std::cerr << "FAIL: " << name << "\n";
    }
}

static std::shared_ptr<AnimationClip> MakeClip(const std::string& name, int frameCount,
                                                float frameDuration, bool loop = true) {
    auto clip = std::make_shared<AnimationClip>();
    clip->name = name;
    clip->loop = loop;
    for (int i = 0; i < frameCount; ++i) {
        AnimationFrame f;
        f.src = {(float)i * 32.0f, 0, 32.0f, 32.0f};
        f.duration = frameDuration;
        f.origin = {0, 0};
        clip->frames.push_back(f);
    }
    clip->totalDuration = frameCount * frameDuration;
    return clip;
}

// ============================================================
// Test 1: Normal loop — playhead wraps via fmod
// ============================================================
static void Test_NormalLoop() {
    Animator a;
    a.AddClip(MakeClip("run", 4, 0.25f, true));
    a.Play("run", 1.0f, true);

    CHECK(a.IsPlaying(), "NormalLoop: should be playing");
    CHECK(a.GetCurrentFrameIndex() == 0, "NormalLoop: starts at frame 0");

    // 1.0 sec total: 4 frames * 0.25s = 1.0s clip length
    // Advance 1.0 sec → wraps back to start
    for (int i = 0; i < 40; ++i) a.Update(0.025f); // 40*0.025 = 1.0s
    CHECK(a.IsPlaying(), "NormalLoop: still playing after full loop");
    float ph = a.GetPlayheadTime();
    CHECK(ph < 0.01f || (1.0f - ph) < 0.01f, "NormalLoop: playhead near 0 after wrap");
}

// ============================================================
// Test 2: Normal non-loop — stops at end
// ============================================================
static void Test_NormalNonLoop() {
    Animator a;
    a.AddClip(MakeClip("jump", 3, 0.2f, false));
    bool finished = false;
    a.OnClipFinished = [&](const std::string&) { finished = true; };
    a.Play("jump", 1.0f, true);

    CHECK(a.GetCurrentFrameIndex() == 0, "NonLoop: starts at frame 0");

    // 3 frames * 0.2s = 0.6s clip length
    // Advance 0.8s → should stop at end
    for (int i = 0; i < 32; ++i) a.Update(0.025f); // 32*0.025 = 0.8s
    CHECK(!a.IsPlaying(), "NonLoop: stopped");
    CHECK(finished, "NonLoop: OnClipFinished fired");
    CHECK(a.GetCurrentFrameIndex() == 2, "NonLoop: ended on last frame");
}

// ============================================================
// Test 3: Reverse non-loop — stops at start
// ============================================================
static void Test_ReverseNonLoop() {
    Animator a;
    a.SetPlaybackMode(Animator::PlaybackMode::Reverse);
    a.AddClip(MakeClip("atk", 4, 0.25f, false));
    bool finished = false;
    a.OnClipFinished = [&](const std::string&) { finished = true; };
    a.Play("atk", 1.0f, true);

    // Play starts at clipLength for Reverse mode
    CHECK(a.GetPlayheadTime() > 0.9f, "ReverseNonLoop: starts near clip end");

    // Advance 1.0s → playhead goes below 0, stops
    for (int i = 0; i < 40; ++i) a.Update(0.025f);
    CHECK(!a.IsPlaying(), "ReverseNonLoop: stopped");
    CHECK(finished, "ReverseNonLoop: OnClipFinished fired");
}

// ============================================================
// Test 4: PingPong loop — infinite forward+reverse
// ============================================================
static void Test_PingPongLoop() {
    Animator a;
    a.SetPlaybackMode(Animator::PlaybackMode::PingPong);
    a.AddClip(MakeClip("idle", 4, 0.25f, true)); // 1.0s clip, period = 2.0s
    a.Play("idle", 1.0f, true);

    // Advance 2.5s (past one full cycle + some)
    for (int i = 0; i < 100; ++i) a.Update(0.025f);
    CHECK(a.IsPlaying(), "PingPongLoop: still playing after multiple cycles");
}

// ============================================================
// Test 5: PingPong non-loop — single cycle then stop
// ============================================================
static void Test_PingPongNonLoop() {
    Animator a;
    a.SetPlaybackMode(Animator::PlaybackMode::PingPong);
    a.AddClip(MakeClip("glow", 4, 0.25f, false)); // 1.0s clip, period = 2.0s
    bool finished = false;
    a.OnClipFinished = [&](const std::string&) { finished = true; };
    a.Play("glow", 1.0f, true);

    // Advance 2.2s (full forward+reverse + small overshoot)
    for (int i = 0; i < 88; ++i) a.Update(0.025f); // 88*0.025 = 2.2s
    CHECK(!a.IsPlaying(), "PingPongNonLoop: stopped after one cycle");
    CHECK(finished, "PingPongNonLoop: OnClipFinished fired");
    CHECK(a.GetPlayheadTime() < 0.001f, "PingPongNonLoop: playhead at 0");
}

// ============================================================
// Test 6: PingPong non-loop — frame mapping during forward and reverse
// ============================================================
static void Test_PingPongNonLoop_FrameMapping() {
    Animator a;
    a.SetPlaybackMode(Animator::PlaybackMode::PingPong);
    a.AddClip(MakeClip("test", 4, 0.25f, false)); // 1.0s clip
    a.Play("test", 1.0f, true);

    // At start: frame 0
    CHECK(a.GetCurrentFrameIndex() == 0, "PingPongFrameMap: frame 0 at start");

    // Advance 0.3s → frame 1 (0.0-0.25 is frame 0, 0.25-0.5 is frame 1)
    a.Update(0.3f);
    CHECK(a.GetCurrentFrameIndex() == 1, "PingPongFrameMap: frame 1 at 0.3s");

    // Advance to 1.3s → still forward but near reflection point
    // At 1.3s: playhead went past 1.0, reflects to 0.7s (reverse phase)
    a.Update(1.0f); // total = 1.3s
    CHECK(a.GetCurrentFrameIndex() >= 2, "PingPongFrameMap: frame >= 2 during reverse");
    CHECK(a.GetCurrentFrameIndex() <= 3, "PingPongFrameMap: frame <= 3 during reverse");
}

// ============================================================
// Test 7: Seek — Normal mode
// ============================================================
static void Test_Seek_Normal() {
    Animator a;
    a.AddClip(MakeClip("walk", 4, 0.25f, true));
    a.Play("walk", 1.0f, true);

    a.Seek(0.5f);
    CHECK(a.GetCurrentFrameIndex() == 2, "SeekNormal: 0.5s → frame 2");

    a.Seek(0.0f);
    CHECK(a.GetCurrentFrameIndex() == 0, "SeekNormal: 0.0s → frame 0");

    a.Seek(0.9f);
    CHECK(a.GetCurrentFrameIndex() == 3, "SeekNormal: 0.9s → frame 3");

    // Out of range → clamps
    a.Seek(5.0f);
    CHECK(a.GetPlayheadTime() < 0.001f, "SeekNormal: 5.0s wraps to ~0 (loop)");
}

// ============================================================
// Test 8: Seek — PingPong mode
// ============================================================
static void Test_Seek_PingPong() {
    Animator a;
    a.SetPlaybackMode(Animator::PlaybackMode::PingPong);
    a.AddClip(MakeClip("pp", 4, 0.25f, false)); // 1.0s clip, period = 2.0s
    a.Play("pp", 1.0f, true);

    a.Seek(0.5f);
    CHECK(a.GetCurrentFrameIndex() == 2, "SeekPingPong: 0.5s → frame 2 (forward)");

    a.Seek(1.5f); // 1.5s is in reverse phase (1.0 → 2.0 maps to 0.5 from end)
    CHECK(a.GetCurrentFrameIndex() == 2, "SeekPingPong: 1.5s → frame 2 (reverse)");

    a.Seek(2.5f); // out of range → clamps to period (2.0)
    CHECK(a.GetPlayheadTime() <= 2.001f, "SeekPingPong: 2.5s clamps to ~2.0");
}

// ============================================================
// Test 9: Play(reset=false) preserves playhead
// ============================================================
static void Test_PlayNoReset() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));
    a.AddClip(MakeClip("b", 4, 0.25f, true));

    a.Play("a", 1.0f, true);
    a.Update(0.5f); // advance to ~frame 2
    int frameBefore = a.GetCurrentFrameIndex();

    // Play same clip with reset=false → should keep playhead
    a.Play("a", 1.0f, false);
    CHECK(a.IsPlaying(), "PlayNoReset: still playing");
    // playhead should be preserved (approximately)
}

// ============================================================
// Test 10: Play(reset=true) resets playhead
// ============================================================
static void Test_PlayReset() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));

    a.Play("a", 1.0f, true);
    a.Update(0.5f);
    CHECK(a.GetCurrentFrameIndex() > 0, "PlayReset: advanced before reset");

    a.Play("a", 1.0f, true);
    CHECK(a.GetCurrentFrameIndex() == 0, "PlayReset: frame reset to 0");
    CHECK(a.GetPlayheadTime() < 0.001f, "PlayReset: playhead reset to ~0");
}

// ============================================================
// Test 11: Stop resets playhead
// ============================================================
static void Test_Stop() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));
    a.Play("a", 1.0f, true);
    a.Update(0.5f);

    a.Stop();
    CHECK(!a.IsPlaying(), "Stop: not playing");
    CHECK(a.GetPlayheadTime() < 0.001f, "Stop: playhead at 0");
    CHECK(a.GetCurrentFrameIndex() == 0, "Stop: frame index at 0");
}

// ============================================================
// Test 12: OnFrameChanged callback
// ============================================================
static void Test_OnFrameChanged() {
    Animator a;
    a.AddClip(MakeClip("run", 4, 0.25f, true));
    int lastFrame = -1;
    a.OnFrameChanged = [&](const std::string&, int frame) { lastFrame = frame; };
    a.Play("run", 1.0f, true);

    a.Update(0.3f); // should advance to frame 1
    CHECK(lastFrame >= 0, "OnFrameChanged: callback fired");
}

// ============================================================
// Test 13: GetTotalFrames / GetCurrentSrcRect
// ============================================================
static void Test_QueryMethods() {
    Animator a;
    a.AddClip(MakeClip("x", 3, 0.2f, true));
    a.Play("x", 1.0f, true);

    CHECK(a.GetTotalFrames() == 3, "Query: total frames = 3");
    Rectangle src = a.GetCurrentSrcRect();
    CHECK(src.width > 0.0f, "Query: src rect has width");
}

// ============================================================
// Test 14: Pause / Resume
// ============================================================
static void Test_PauseResume() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));
    a.Play("a", 1.0f, true);
    a.Update(0.3f);
    float playheadBefore = a.GetPlayheadTime();

    a.Pause();
    CHECK(!a.IsPlaying(), "Pause: not playing");
    a.Update(0.5f); // should not advance
    CHECK(a.GetPlayheadTime() == playheadBefore, "Pause: playhead unchanged after Update");

    a.Resume();
    CHECK(a.IsPlaying(), "Resume: playing again");
    a.Update(0.1f);
    CHECK(a.GetPlayheadTime() > playheadBefore, "Resume: playhead advanced");
}

// ============================================================
// Test 15: HasClip / nonexistent clip
// ============================================================
static void Test_HasClip() {
    Animator a;
    CHECK(!a.HasClip("nope"), "HasClip: nonexistent returns false");
    a.AddClip(MakeClip("yes", 2, 0.1f, true));
    CHECK(a.HasClip("yes"), "HasClip: existing returns true");
    CHECK(!a.Play("nope"), "Play: nonexistent returns false");
}

// ============================================================
// Test 16: Play nonexistent clip returns false
// ============================================================
static void Test_PlayNonexistent() {
    Animator a;
    CHECK(!a.Play("ghost"), "PlayNonexistent: returns false");
    CHECK(!a.IsPlaying(), "PlayNonexistent: not playing");
}

// ============================================================
// Test 17: SetFrame
// ============================================================
static void Test_SetFrame() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));
    a.Play("a", 1.0f, true);

    a.SetFrame(2);
    CHECK(a.GetCurrentFrameIndex() == 2, "SetFrame: jumped to frame 2");

    a.SetFrame(0);
    CHECK(a.GetCurrentFrameIndex() == 0, "SetFrame: jumped to frame 0");

    a.SetFrame(99); // out of range → clamps
    CHECK(a.GetCurrentFrameIndex() == 3, "SetFrame: clamped to last frame");
}

// ============================================================
// Test 18: Empty clip guard
// ============================================================
static void Test_EmptyClip() {
    Animator a;
    auto empty = std::make_shared<AnimationClip>();
    empty->name = "empty";
    empty->loop = true;
    empty->totalDuration = 0.0f;
    a.AddClip(empty);

    // Should not crash
    a.Play("empty", 1.0f, true);
    a.Update(0.1f);
    a.Seek(0.5f);
    a.SetFrame(0);

    CHECK(!a.IsPlaying() || a.GetCurrentFrameIndex() == 0, "EmptyClip: safe");
}

// ============================================================
// Test 19: Speed > 1 fast-forward
// ============================================================
static void Test_SpeedFastForward() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true)); // 1.0s
    a.Play("a", 2.0f, true); // 2x speed

    a.Update(0.6f); // at 2x → 1.2s elapsed → wraps to ~0.2s → frame 0 or 1
    CHECK(a.IsPlaying(), "SpeedFF: still playing");
    CHECK(a.GetPlayheadTime() < 1.0f, "SpeedFF: playhead wrapped");
}

// ============================================================
// Test 20: Double-click Play same clip no-op
// ============================================================
static void Test_PlaySameClipNoReset() {
    Animator a;
    a.AddClip(MakeClip("a", 4, 0.25f, true));
    a.Play("a", 1.0f, true);
    a.Update(0.5f);
    float ph1 = a.GetPlayheadTime();

    a.Play("a", 1.0f, false); // same clip, no reset → no-op
    float ph2 = a.GetPlayheadTime();
    CHECK(std::abs(ph1 - ph2) < 0.001f, "PlaySameClipNoReset: playhead preserved");
}

// ============================================================
int main() {
    Test_NormalLoop();
    Test_NormalNonLoop();
    Test_ReverseNonLoop();
    Test_PingPongLoop();
    Test_PingPongNonLoop();
    Test_PingPongNonLoop_FrameMapping();
    Test_Seek_Normal();
    Test_Seek_PingPong();
    Test_PlayNoReset();
    Test_PlayReset();
    Test_Stop();
    Test_OnFrameChanged();
    Test_QueryMethods();
    Test_PauseResume();
    Test_HasClip();
    Test_PlayNonexistent();
    Test_SetFrame();
    Test_EmptyClip();
    Test_SpeedFastForward();
    Test_PlaySameClipNoReset();

    std::cout << "\n=== Animator Tests ===\n";
    std::cout << "Passed: " << testsPassed << "\n";
    std::cout << "Failed: " << testsFailed << "\n";

    return testsFailed > 0 ? 1 : 0;
}
