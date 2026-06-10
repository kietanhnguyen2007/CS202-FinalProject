#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "raylib.h"
#include "AnimationSystem.h"

namespace Systems {

// Simple texture atlas that loads JSON metadata and a texture
class TextureAtlas {
public:
    TextureAtlas() = default;
    ~TextureAtlas();

    // Load from json path (relative to project root). Returns nullptr on failure.
    static std::unique_ptr<TextureAtlas> LoadFromJSON(const std::string& jsonPath);

    Texture2D* GetTexture();
    bool HasFrame(const std::string& name) const;
    Rectangle GetFrameRect(const std::string& name) const;

    // If JSON contains clips, return clip
    bool HasClip(const std::string& clipName) const;
    std::shared_ptr<AnimationClip> GetClip(const std::string& clipName) const;

private:
    Texture2D m_texture{};
    std::unordered_map<std::string, Rectangle> m_frames;
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> m_clips;
};

} // namespace Systems
