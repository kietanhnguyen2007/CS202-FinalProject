#include "Survival3D/Animation/AnimationGraph.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace Survival3D::Animation;

void Check(bool condition, const char* message) {
    assert(condition);
    if (!condition) throw std::runtime_error(message);
}

bool Near(float left, float right, float epsilon = 0.0001f) {
    return std::abs(left - right) <= epsilon;
}

void TestDeterministicStateMachine() {
    StateMachine hero(ActorClass::Hero);
    Check(hero.Submit({State::BasicAttack, TransitionReason::Gameplay, 1}, 0.0f)
              .Accepted(),
          "hero must enter BasicAttack from Idle");
    Check(!hero.Submit({State::Dash, TransitionReason::Gameplay, 2}, 0.30f)
               .Accepted(),
          "dash must not erase BasicAttack windup");
    Check(hero.Current() == State::BasicAttack,
          "rejected cancel changed hero state");
    Check(hero.Submit({State::Dash, TransitionReason::Gameplay, 3}, 0.50f)
              .Accepted(),
          "dash cancel window did not open");
    Check(hero.Submit({State::Ultimate, TransitionReason::Gameplay, 4}, 0.90f)
              .Accepted(),
          "late dash recovery must cancel into ultimate");
    Check(hero.Submit({State::Hurt, TransitionReason::HitReaction, 5}, 0.10f)
              .Accepted(),
          "hit reaction must interrupt a hero action");
    Check(!hero.Submit({State::PhaseTransition, TransitionReason::PhaseChange, 6},
                       0.20f).Accepted(),
          "hero must reject boss-only phase transition");
    Check(hero.Submit({State::Death, TransitionReason::Death, 7}, 0.20f)
              .Accepted(),
          "death must interrupt every non-terminal state");
    Check(!hero.Submit({State::BasicAttack, TransitionReason::Gameplay, 8}, 1.0f)
               .Accepted(),
          "death state must be terminal");
    Check(hero.CompleteCurrent(false).status == TransitionStatus::RejectedTerminal,
          "death completion must remain terminal");

    StateMachine first(ActorClass::Hero);
    StateMachine second(ActorClass::Hero);
    std::vector<TransitionRequest> requests{
        {State::BasicAttack, TransitionReason::Gameplay, 10},
        {State::Ultimate, TransitionReason::Gameplay, 8},
        {State::SkillTwo, TransitionReason::Gameplay, 9}
    };
    std::vector<TransitionRequest> reversed = requests;
    std::reverse(reversed.begin(), reversed.end());
    const TransitionResult firstResult = first.SubmitBest(requests, 0.0f);
    const TransitionResult secondResult = second.SubmitBest(reversed, 0.0f);
    Check(firstResult.Accepted() && secondResult.Accepted(),
          "batched requests were not accepted");
    Check(first.Current() == State::Ultimate && second.Current() == State::Ultimate,
          "request append order changed priority result");
    Check(first.Submit({State::SkillOne, TransitionReason::Gameplay, 9}, 1.0f)
              .status == TransitionStatus::RejectedStale,
          "consumed request serial was accepted twice");

    StateMachine enemy(ActorClass::Enemy);
    Check(enemy.Submit({State::BasicAttack, TransitionReason::Gameplay, 1}, 0.0f)
              .Accepted(),
          "enemy attack did not start");
    Check(!enemy.Submit({State::SkillOne, TransitionReason::Gameplay, 2}, 0.70f)
               .Accepted(),
          "enemy attack was cancelled before recovery");
    Check(enemy.Submit({State::SkillOne, TransitionReason::Gameplay, 3}, 0.99f)
              .Accepted(),
          "enemy could not select action at completed recovery");
    Check(enemy.CompleteCurrent(true).Accepted()
              && enemy.Current() == State::Locomotion,
          "enemy natural completion did not resolve to locomotion");

    StateMachine boss(ActorClass::Boss);
    Check(boss.Submit({State::SkillTwo, TransitionReason::Gameplay, 1}, 0.0f)
              .Accepted(),
          "boss skill did not start");
    Check(boss.Submit({State::PhaseTransition, TransitionReason::PhaseChange, 2},
                      0.1f).Accepted(),
          "phase transition must interrupt a boss skill");
    Check(!boss.Submit({State::Hurt, TransitionReason::HitReaction, 3}, 0.5f)
               .Accepted(),
          "hurt incorrectly interrupted a boss phase transition");
    Check(boss.Submit({State::Death, TransitionReason::Death, 4}, 0.5f)
              .Accepted(),
          "death must interrupt phase transition");
}

void TestLocomotionBlendTree() {
    LocomotionWeights weights = SolveLocomotionBlend({0.0f, 5.0f}, 5.0f);
    Check(Near(weights.Sum(), 1.0f), "forward weights are not normalized");
    Check(Near(weights.forward, 1.0f) && Near(weights.idle, 0.0f),
          "full forward speed did not select Forward");

    weights = SolveLocomotionBlend({5.0f, 5.0f}, 5.0f);
    Check(Near(weights.Sum(), 1.0f), "diagonal weights are not normalized");
    Check(Near(weights.forward, 0.5f) && Near(weights.right, 0.5f),
          "diagonal did not blend equally between cardinal poses");

    weights = SolveLocomotionBlend({0.01f, 0.01f}, 5.0f, 0.05f);
    Check(Near(weights.idle, 1.0f), "dead zone did not select Idle");

    const LocalVelocity2D local = WorldToLocalVelocity(
        3.0f, 0.0f, 0.0f, 1.0f);
    Check(Near(local.right, 3.0f) && Near(local.forward, 0.0f),
          "world-to-local velocity axes are incorrect");
    const LocalVelocity2D rotated = WorldToLocalVelocity(
        0.0f, 2.0f, 1.0f, 0.0f);
    Check(Near(rotated.right, -2.0f) && Near(rotated.forward, 0.0f),
          "facing rotation did not produce local Left");
}

void TestUpperBodyLayerContract() {
    UpperBodyLayer layer = ResolveUpperBodyLayer(
        {State::Locomotion, true, false, 0.75f, 1.0f});
    Check(layer.mode == LayerMode::Additive && Near(layer.weight, 0.75f),
          "aim layer must be additive");
    Check(MaskContains(layer.mask, SemanticBone::Chest)
              && MaskContains(layer.mask, SemanticBone::HandRight)
              && MaskContains(layer.mask, SemanticBone::WeaponSocketRight),
          "aim layer misses upper-body aiming joints");
    Check(!MaskContains(layer.mask, SemanticBone::Root)
              && !MaskContains(layer.mask, SemanticBone::Hips)
              && !MaskContains(layer.mask, SemanticBone::ThighLeft)
              && !MaskContains(layer.mask, SemanticBone::ShinRight),
          "upper-body mask leaked into locomotion joints");

    layer = ResolveUpperBodyLayer(
        {State::Locomotion, true, true, 0.8f, 0.6f});
    Check(layer.mode == LayerMode::Override && layer.usesCastPose
              && Near(layer.weight, 0.6f),
          "cast layer must override aim layer");
    layer = ResolveUpperBodyLayer(
        {State::Death, true, true, 1.0f, 1.0f});
    Check(layer.mode == LayerMode::None && layer.mask == 0,
          "death must suppress upper-body overlay");
}

void TestRootMotionContract() {
    const RootMotionProfile dash{
        RootMotionMode::Dash, 4.0f, 0.0f, 0.20f, 0.80f, 0.5f, true
    };
    const RootMotionDelta first = SampleRootMotionDelta(
        dash, 0.0f, 0.35f, 0.0f, 1.0f);
    const RootMotionDelta second = SampleRootMotionDelta(
        dash, 0.35f, 0.60f, 0.0f, 1.0f);
    const RootMotionDelta third = SampleRootMotionDelta(
        dash, 0.60f, 1.0f, 0.0f, 1.0f);
    Check(Near(first.localForward + second.localForward + third.localForward, 4.0f),
          "partitioned dash sampling changed total root motion");
    Check(Near(first.worldX + second.worldX + third.worldX, 0.0f)
              && Near(first.worldZ + second.worldZ + third.worldZ, 4.0f),
          "dash was not transformed along actor facing");
    Check(first.requiresCollisionSweep && second.requiresCollisionSweep,
          "root motion must require controller collision sweep");

    const RootMotionProfile blink{
        RootMotionMode::Blink, 4.0f, 0.0f, 0.0f, 1.0f, 0.55f, true
    };
    const RootMotionDelta before = SampleRootMotionDelta(
        blink, 0.10f, 0.50f, 1.0f, 0.0f);
    const RootMotionDelta commit = SampleRootMotionDelta(
        blink, 0.50f, 0.60f, 1.0f, 0.0f);
    const RootMotionDelta after = SampleRootMotionDelta(
        blink, 0.60f, 0.90f, 1.0f, 0.0f);
    Check(Near(before.localForward, 0.0f) && Near(after.localForward, 0.0f),
          "blink emitted outside its trigger crossing");
    Check(commit.instantaneous && Near(commit.worldX, 4.0f)
              && Near(commit.worldZ, 0.0f),
          "blink did not emit one facing-relative instantaneous delta");
}

void TestDataDrivenClipsAndEvents() {
    const ClipLibrary knight = BuildStep6HeroClipLibrary(HeroStyle::Knight);
    const ClipLibrary mage = BuildStep6HeroClipLibrary(HeroStyle::MagicCaster);
    Check(knight.Clips().size() == 9 && mage.Clips().size() == 9,
          "Step 6 hero libraries must contain nine states");
    Check(knight.Validate().empty() && mage.Validate().empty(),
          "built-in hero clip data failed validation");

    const ClipDefinition* knightRush = knight.Find("knight.skill_two");
    const ClipDefinition* knightDash = knight.Find("knight.dash");
    const ClipDefinition* mageBlink = mage.Find("mage.dash");
    Check(knightRush != nullptr
              && knightRush->rootMotion.mode == RootMotionMode::Rush
              && Near(knightRush->rootMotion.forwardDistance, 7.0f),
          "Knight rush root-motion contract is missing");
    Check(knightDash != nullptr
              && knightDash->rootMotion.mode == RootMotionMode::Dash,
          "Knight dash root-motion contract is missing");
    Check(mageBlink != nullptr
              && mageBlink->rootMotion.mode == RootMotionMode::Blink
              && Near(mageBlink->rootMotion.forwardDistance, 4.0f),
          "Magic Caster blink root-motion contract is missing");

    const ClipDefinition* basic = knight.Find("knight.basic");
    Check(basic != nullptr, "Knight Basic clip was not found by id");
    std::vector<EventDefinition> events = CollectCrossedEvents(
        *basic, 180.0f, 188.0f);
    Check(events.size() == 1 && events.front().id == "sword_contact",
          "contact event was not emitted on exact frame crossing");
    events = CollectCrossedEvents(*basic, 188.0f, 200.0f);
    Check(events.empty(), "contact event fired twice after its frame");

    const ClipDefinition* run = knight.Find("knight.run");
    Check(run != nullptr, "Knight Run clip was not found by id");
    events = CollectCrossedEvents(*run, 150.0f, 105.0f, true);
    Check(events.size() == 2 && events[0].frame == 151 && events[1].frame == 104,
          "loop event order is not tail-then-head");

    const ClipLibrary boss = BuildStep6NonHeroClipLibrary(ActorClass::Boss);
    const ClipDefinition* bossUltimate = boss.Find("boss.ultimate");
    Check(bossUltimate != nullptr && bossUltimate->events.size() == 1
              && bossUltimate->events[0].type == ClipEventType::PhaseCommit,
          "boss library does not expose phase event data");

    ClipLibrary custom;
    ClipDefinition invalid;
    invalid.id = "invalid";
    invalid.firstFrame = 10;
    invalid.lastFrame = 20;
    invalid.events = {{"outside", ClipEventType::ActionContact, 21, 0.0f}};
    std::string error;
    Check(!custom.Add(invalid, &error) && !error.empty(),
          "invalid designer clip was accepted");

    ClipDefinition archer;
    archer.id = "hex_archer.basic";
    archer.state = State::BasicAttack;
    archer.firstFrame = 160;
    archer.lastFrame = 222;
    archer.events = {{"release_arrow", ClipEventType::ProjectileRelease, 202, 1.0f}};
    Check(custom.Add(archer, &error),
          "custom C++ clip/event definition was rejected");
    Check(custom.Find("hex_archer.basic") != nullptr,
          "custom data was not retrievable by id");
    Check(Near(archer.DurationSeconds(), 62.0f / 60.0f),
          "clip duration does not use sampled endpoint span");
}

} // namespace

int main() {
    try {
        TestDeterministicStateMachine();
        TestLocomotionBlendTree();
        TestUpperBodyLayerContract();
        TestRootMotionContract();
        TestDataDrivenClipsAndEvents();
        std::cout << "AnimationGraphTests: all checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "AnimationGraphTests: " << error.what() << '\n';
        return 1;
    }
}
