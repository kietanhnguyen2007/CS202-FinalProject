#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "raylib.h"
#include "View/Animator.h"

namespace View::Animations {

// Simple texture atlas that loads JSON metadata and a texture
class TextureAtlas {
public:
    TextureAtlas() = default;
    ~TextureAtlas();

    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    // Load from json path. Returns nullptr on failure.
    // Parses JSON only; call LoadTexture() separately after InitWindow().
    static std::unique_ptr<TextureAtlas> LoadFromJSON(const std::string& jsonPath);

    // Call after InitWindow() to load the texture into GPU.
    // No-op if already loaded. Returns true on success.
    bool LoadTexture();
    bool IsTextureLoaded() const { return m_texture.id != 0; }

    Texture2D* GetTexture();
    bool HasFrame(const std::string& name) const;
    Rectangle GetFrameRect(const std::string& name) const;

    // If JSON contains clips, return clip
    bool HasClip(const std::string& clipName) const;
    std::shared_ptr<AnimationClip> GetClip(const std::string& clipName) const;

private:
    Texture2D m_texture{};
    std::string m_texturePath; // resolved image path, set by LoadFromJSON
    std::unordered_map<std::string, Rectangle> m_frames;
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> m_clips;
};

} // namespace View::Animations
