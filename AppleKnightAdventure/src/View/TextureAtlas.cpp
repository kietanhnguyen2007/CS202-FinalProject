#include "View/TextureAtlas.h"
#include "Utils/Constants.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace View::Animations {

TextureAtlas::~TextureAtlas() {
    if (m_isImageLoaded && m_image.data != nullptr) {
        ::UnloadImage(m_image);
        m_image = {};
        m_isImageLoaded = false;
    }
    if (m_texture.id != 0) ::UnloadTexture(m_texture);
    m_texture = {};
}

std::shared_ptr<TextureAtlas> TextureAtlas::LoadFromJSON(const std::string& jsonPath) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) return nullptr;

    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();

    auto atlas = std::make_shared<TextureAtlas>();

    std::string baseDir;
    size_t slash = jsonPath.find_last_of("/\\");
    if (slash != std::string::npos) baseDir = jsonPath.substr(0, slash+1);

    // Parse JSON using nlohmann::json
    try {
        auto j = nlohmann::json::parse(s);

        // --- Extract image path robustly ---
        std::string imagePath;
        if (j.contains("image") && j["image"].is_string()) {
            imagePath = j["image"].get<std::string>();
        } else if (j.contains("meta") && j["meta"].contains("image") && j["meta"]["image"].is_string()) {
            imagePath = j["meta"]["image"].get<std::string>();
        } else {
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it->is_string()) {
                    std::string v = it->get<std::string>();
                    if (v.size() >= 4) {
                        std::string ext = v.substr(v.size()-4);
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == ".png") {
                            imagePath = v;
                            break;
                        }
                    }
                }
            }
        }

        if (!imagePath.empty()) {
            atlas->m_texturePath = baseDir + imagePath;
        } else {
            std::string fallbackPath = jsonPath;
            size_t extPos = fallbackPath.find_last_of('.');
            if (extPos != std::string::npos) {
                fallbackPath = fallbackPath.substr(0, extPos) + ".png";
            } else {
                fallbackPath += ".png";
            }
            atlas->m_texturePath = fallbackPath;
        }

        // --- Parse frames ---
        if (j.contains("frames")) {
            auto frames = j["frames"];
            if (frames.is_object()) {
                for (auto it = frames.begin(); it != frames.end(); ++it) {
                    const std::string key = it.key();
                    const auto& val = it.value();

                    // Support both "frame": {x,y,w,h} and direct x,y,w,h
                    int x=0,y=0,w=0,h=0;
                    if (val.contains("frame") && val["frame"].is_object()) {
                        x = val["frame"].value("x", 0);
                        y = val["frame"].value("y", 0);
                        w = val["frame"].value("w", 0);
                        h = val["frame"].value("h", 0);
                    } else if (val.is_object()) {
                        x = val.value("x", 0);
                        y = val.value("y", 0);
                        w = val.value("w", 0);
                        h = val.value("h", 0);
                    }
                    atlas->m_frames.emplace(key, Rectangle{(float)x,(float)y,(float)w,(float)h});
                }
            }
        }

        // --- Parse clips ---
        if (j.contains("clips") && j["clips"].is_object()) {
            for (auto it = j["clips"].begin(); it != j["clips"].end(); ++it) {
                const std::string clipName = it.key();
                const auto& clipObj = it.value();
                auto clip = std::make_shared<AnimationClip>();
                clip->name = clipName;
                if (clipObj.contains("frames") && clipObj["frames"].is_array()) {
                    size_t inlineFrameIndex = 0;
                    for (const auto& frameEntry : clipObj["frames"]) {
                        AnimationFrame af;
                        af.duration = 0.1f;
                        af.origin = {0,0};
                        const nlohmann::json* frameMetadata = nullptr;
                        bool hasValidRect = false;

                        // TexturePacker-style atlases reference a frame declared in
                        // the top-level "frames" object by name.
                        if (frameEntry.is_string()) {
                            const std::string frameName = frameEntry.get<std::string>();
                            auto fit = atlas->m_frames.find(frameName);
                            if (fit != atlas->m_frames.end()) {
                                af.src = fit->second;
                                af.name = frameName;
                                hasValidRect = af.src.width > 0.0f && af.src.height > 0.0f;
                                if (j.contains("frames") && j["frames"].is_object() && j["frames"].contains(frameName)) {
                                    frameMetadata = &j["frames"][frameName];
                                }
                            }
                        }
                        // The generated V2 character atlases store each rectangle
                        // directly in the clip: { "rect": [x, y, w, h], ... }.
                        else if (frameEntry.is_object()) {
                            int x = 0, y = 0, w = 0, h = 0;
                            if (frameEntry.contains("rect") && frameEntry["rect"].is_array() &&
                                frameEntry["rect"].size() >= 4) {
                                const auto& rect = frameEntry["rect"];
                                x = rect[0].get<int>();
                                y = rect[1].get<int>();
                                w = rect[2].get<int>();
                                h = rect[3].get<int>();
                            } else if (frameEntry.contains("frame") && frameEntry["frame"].is_object()) {
                                const auto& rect = frameEntry["frame"];
                                x = rect.value("x", 0);
                                y = rect.value("y", 0);
                                w = rect.value("w", 0);
                                h = rect.value("h", 0);
                            } else {
                                x = frameEntry.value("x", 0);
                                y = frameEntry.value("y", 0);
                                w = frameEntry.value("w", 0);
                                h = frameEntry.value("h", 0);
                            }

                            af.src = Rectangle{(float)x, (float)y, (float)w, (float)h};
                            af.duration = frameEntry.value("duration", 0.1f);
                            af.name = frameEntry.value(
                                "name", clipName + "_" + std::to_string(inlineFrameIndex));
                            hasValidRect = w > 0 && h > 0;
                            frameMetadata = &frameEntry;
                        }

                        if (hasValidRect && frameMetadata != nullptr) {
                            const auto& frameJson = *frameMetadata;
                            if (frameJson.contains("rotated")) af.rotated = frameJson["rotated"].get<bool>();
                            if (frameJson.contains("trimmed")) af.trimmed = frameJson["trimmed"].get<bool>();
                            if (frameJson.contains("spriteSourceSize") && frameJson["spriteSourceSize"].is_object()) {
                                af.spriteSourceSize.x = (float)frameJson["spriteSourceSize"].value("x", 0);
                                af.spriteSourceSize.y = (float)frameJson["spriteSourceSize"].value("y", 0);
                            }
                            if (frameJson.contains("sourceSize") && frameJson["sourceSize"].is_object()) {
                                af.originalSize.x = (float)frameJson["sourceSize"].value("w", 0);
                                af.originalSize.y = (float)frameJson["sourceSize"].value("h", 0);
                            }
                            if (af.originalSize.x > 0 && af.originalSize.y > 0) {
                                af.origin = {af.spriteSourceSize.x, af.spriteSourceSize.y};
                            }
                        }

                        if (hasValidRect) clip->frames.push_back(af);
                        ++inlineFrameIndex;
                    }
                }
                if (clipObj.contains("durations") && clipObj["durations"].is_array()) {
                    size_t i = 0;
                    for (const auto& d : clipObj["durations"]) {
                        if (i >= clip->frames.size()) break;
                        clip->frames[i].duration = d.get<float>();
                        ++i;
                    }
                }
                if (clipObj.contains("loop")) clip->loop = clipObj["loop"].get<bool>();

                // cache total duration
                float total = 0.0f;
                for (const auto& f : clip->frames) total += f.duration;
                clip->totalDuration = total;

                if (clip->frames.empty()) {
                    std::cerr << "TextureAtlas: clip '" << clipName
                              << "' has no valid frames in " << jsonPath << "\n";
                }

                atlas->m_clips.emplace(clipName, clip);
            }
        }

        // --- Auto-generate clip from frames if no clips exist ---
        if (atlas->m_clips.empty() && !atlas->m_frames.empty()) {
            auto clip = std::make_shared<AnimationClip>();
            clip->name = "default";
            for (auto const& [fname, rect] : atlas->m_frames) {
                AnimationFrame af;
                af.src = rect;
                af.duration = 0.1f;
                af.origin = {0,0};
                af.name = fname;
                
                // Read metadata if available
                if (j.contains("frames") && j["frames"].is_object() && j["frames"].contains(fname)) {
                    const auto& frameJson = j["frames"][fname];
                    if (frameJson.contains("rotated")) af.rotated = frameJson["rotated"].get<bool>();
                    if (frameJson.contains("trimmed")) af.trimmed = frameJson["trimmed"].get<bool>();
                    if (frameJson.contains("spriteSourceSize") && frameJson["spriteSourceSize"].is_object()) {
                        af.spriteSourceSize.x = (float)frameJson["spriteSourceSize"].value("x", 0);
                        af.spriteSourceSize.y = (float)frameJson["spriteSourceSize"].value("y", 0);
                    }
                    if (frameJson.contains("sourceSize") && frameJson["sourceSize"].is_object()) {
                        af.originalSize.x = (float)frameJson["sourceSize"].value("w", 0);
                        af.originalSize.y = (float)frameJson["sourceSize"].value("h", 0);
                    }
                    if (af.originalSize.x > 0 && af.originalSize.y > 0) {
                        af.origin = { af.spriteSourceSize.x, af.spriteSourceSize.y };
                    }
                }
                
                clip->frames.push_back(af);
            }
            clip->totalDuration = clip->frames.size() * 0.1f;
            atlas->m_clips.emplace("default", clip);
        }

        // metadata is attached when building clip frames above

    } catch (const std::exception& ex) {
        std::cerr << "TextureAtlas: JSON parse error: " << ex.what() << "\n";
    }

    return atlas;
}

bool TextureAtlas::LoadTexture() {
    if (m_texture.id != 0) return true; // already loaded
    if (m_texturePath.empty()) return false;
    if (!LoadImageAsync() || !UploadTextureFromImage()) {
        std::cerr << "TextureAtlas: failed to load texture: " << m_texturePath << "\n";
        return false;
    }
    return true;
}

void TextureAtlas::ComputeBossGroundAnchors() {
    if (!m_isImageLoaded || m_image.data == nullptr || m_image.width <= 0 || m_image.height <= 0) {
        return;
    }

    std::string normalizedPath = m_texturePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    if (normalizedPath.find("/boss/") == std::string::npos ||
        normalizedPath.find("/projectiles/") != std::string::npos ||
        normalizedPath.find("/ground_animate/") != std::string::npos) {
        return;
    }

    Color* pixels = ::LoadImageColors(m_image);
    if (!pixels) return;

    for (auto& [clipName, clip] : m_clips) {
        (void)clipName;
        if (!clip) continue;
        for (auto& frame : clip->frames) {
            const int x0 = std::clamp(static_cast<int>(frame.src.x), 0, m_image.width);
            const int y0 = std::clamp(static_cast<int>(frame.src.y), 0, m_image.height);
            const int x1 = std::clamp(static_cast<int>(frame.src.x + frame.src.width), 0, m_image.width);
            const int y1 = std::clamp(static_cast<int>(frame.src.y + frame.src.height), 0, m_image.height);

            int opaqueBottom = -1;
            for (int y = y1 - 1; y >= y0 && opaqueBottom < 0; --y) {
                const Color* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(m_image.width);
                for (int x = x0; x < x1; ++x) {
                    if (row[x].a > 8) {
                        opaqueBottom = y + 1;
                        break;
                    }
                }
            }

            if (opaqueBottom > y0) {
                frame.groundAnchorY = static_cast<float>(opaqueBottom - y0);
            }
        }
    }

    ::UnloadImageColors(pixels);
}

bool TextureAtlas::LoadImageAsync() {
    if (m_isImageLoaded) return true;
    if (m_texturePath.empty()) return false;
    m_image = ::LoadImage(m_texturePath.c_str());
    if (m_image.data != nullptr) {
        m_isImageLoaded = true;
        ComputeBossGroundAnchors();
        return true;
    }
    return false;
}

bool TextureAtlas::UploadTextureFromImage() {
    if (m_texture.id != 0) return true;
    if (!m_isImageLoaded) return false;
    
    m_texture = ::LoadTextureFromImage(m_image);
    ::UnloadImage(m_image);
    m_image.data = nullptr;
    m_isImageLoaded = false;
    
    if (m_texture.id == 0) return false;

    for (auto& pair : m_clips) {
        if (pair.second) {
            for (auto& frame : pair.second->frames) {
                frame.texture = &m_texture;
            }
        }
    }
    return true;
}

Texture2D* TextureAtlas::GetTexture() { return &m_texture; }

bool TextureAtlas::HasFrame(const std::string& name) const {
    return m_frames.find(name) != m_frames.end();
}

Rectangle TextureAtlas::GetFrameRect(const std::string& name) const {
    auto it = m_frames.find(name);
    if (it == m_frames.end()) return {0,0,0,0};
    return it->second;
}

bool TextureAtlas::HasClip(const std::string& clipName) const {
    return m_clips.find(clipName) != m_clips.end();
}

std::shared_ptr<AnimationClip> TextureAtlas::GetClip(const std::string& clipName) const {
    auto it = m_clips.find(clipName);
    if (it == m_clips.end()) return nullptr;
    return it->second;
}

std::vector<std::string> TextureAtlas::GetClipNames() const {
    std::vector<std::string> names;
    names.reserve(m_clips.size());
    for (const auto& pair : m_clips) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace View::Animations
