#include "../../include/Systems/TextureAtlas.h"
#include "../../include/Utils/Constants.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

// Minimal JSON parsing: this code expects the simple schema produced by our tools.
// It's intentionally small to avoid adding an external dependency.

namespace Systems {

TextureAtlas::~TextureAtlas() {
    if (m_texture.id) UnloadTexture(m_texture);
}

static std::string trimQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size()-2);
    return s;
}

std::unique_ptr<TextureAtlas> TextureAtlas::LoadFromJSON(const std::string& jsonPath) {
    std::ifstream f(jsonPath);
    if (!f.is_open()) return nullptr;

    // read entire file
    std::stringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();

    // Very naive parsing: find "image": "..." and frames/ clips blocks.
    auto findString = [&](const std::string& key) -> std::string {
        size_t pos = s.find('"' + key + '"');
        if (pos == std::string::npos) return std::string();
        size_t colon = s.find(':', pos);
        if (colon == std::string::npos) return std::string();
        size_t quote = s.find('"', colon);
        if (quote == std::string::npos) return std::string();
        size_t endq = s.find('"', quote+1);
        if (endq == std::string::npos) return std::string();
        return s.substr(quote+1, endq-quote-1);
    };

    std::string imagePath = findString("image");
    if (imagePath.empty()) {
        // fallback: try to find first .png
        size_t p = s.find(".png");
        if (p != std::string::npos) {
            // find opening quote before
            size_t q = s.rfind('"', p);
            if (q != std::string::npos) {
                size_t q0 = s.rfind('"', q-1);
                if (q0 != std::string::npos) imagePath = s.substr(q0+1, q-q0-1);
            }
        }
    }

    // Build base dir from jsonPath
    std::string baseDir;
    size_t slash = jsonPath.find_last_of("/\\");
    if (slash != std::string::npos) baseDir = jsonPath.substr(0, slash+1);

    std::string finalImage = baseDir + imagePath;

    std::unique_ptr<TextureAtlas> atlas(new TextureAtlas());
    atlas->m_texture = LoadTexture(finalImage.c_str());
    if (atlas->m_texture.id == 0) {
        std::cerr << "TextureAtlas: failed to load texture: " << finalImage << "\n";
    }

    // Parse using nlohmann::json for robustness
    try {
        auto j = nlohmann::json::parse(s);
        if (j.contains("frames")) {
            auto frames = j["frames"];
            if (frames.is_object()) {
                for (auto it = frames.begin(); it != frames.end(); ++it) {
                    const std::string key = it.key();
                    const auto& val = it.value();
                    // Support both "frame": {x,y,w,h} or direct x,y,w,h entries
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
        // parse clips if present
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
                            AnimationFrame af; af.src = fit->second; af.duration = 0.1f; af.origin = {0,0};
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
                atlas->m_clips.emplace(clipName, clip);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "TextureAtlas: JSON parse error: " << ex.what() << "\n";
    }

    // legacy manual parsing removed - nlohmann::json used above
    return atlas;
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

} // namespace Systems
