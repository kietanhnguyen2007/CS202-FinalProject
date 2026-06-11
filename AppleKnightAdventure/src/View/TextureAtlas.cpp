#include "View/TextureAtlas.h"
#include "Utils/Constants.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace View::Animations {

TextureAtlas::~TextureAtlas() {
    if (m_texture.id != 0) ::UnloadTexture(m_texture);
}

std::unique_ptr<TextureAtlas> TextureAtlas::LoadFromJSON(const std::string& jsonPath) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) return nullptr;

    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();

    std::unique_ptr<TextureAtlas> atlas(new TextureAtlas());

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
                    for (const auto& fname : clipObj["frames"]) {
                        if (!fname.is_string()) continue;
                        std::string fnameS = fname.get<std::string>();
                        auto fit = atlas->m_frames.find(fnameS);
                        if (fit != atlas->m_frames.end()) {
                            AnimationFrame af;
                            af.src = fit->second;
                            af.duration = 0.1f;
                            af.origin = {0,0};
                            af.name = fnameS;

                            // read metadata from top-level frames JSON if available
                            if (j.contains("frames") && j["frames"].is_object() && j["frames"].contains(fnameS)) {
                                const auto& frameJson = j["frames"][fnameS];
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

                atlas->m_clips.emplace(clipName, clip);
            }
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
    m_texture = ::LoadTexture(m_texturePath.c_str());
    if (m_texture.id == 0) {
        std::cerr << "TextureAtlas: failed to load texture: " << m_texturePath << "\n";
        return false;
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
