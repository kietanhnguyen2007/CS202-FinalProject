#include "../../include/Systems/TextureAtlas.h"
#include "../../include/Utils/Constants.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

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

TextureAtlas* TextureAtlas::LoadFromJSON(const std::string& jsonPath) {
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

    TextureAtlas* atlas = new TextureAtlas();
    atlas->m_texture = LoadTexture(finalImage.c_str());
    if (atlas->m_texture.id == 0) {
        std::cerr << "TextureAtlas: failed to load texture: " << finalImage << "\n";
    }

    // parse frames: look for "frames": { ... }
    size_t framesPos = s.find("\"frames\"");
    if (framesPos != std::string::npos) {
        size_t brace = s.find('{', framesPos);
        if (brace != std::string::npos) {
            size_t end = s.find('}', brace);
            if (end != std::string::npos) {
                std::string sub = s.substr(brace+1, end-brace-1);
                // crude split by '"'
                size_t cur = 0;
                while (true) {
                    size_t keyStart = sub.find('"', cur);
                    if (keyStart == std::string::npos) break;
                    size_t keyEnd = sub.find('"', keyStart+1);
                    if (keyEnd == std::string::npos) break;
                    std::string key = sub.substr(keyStart+1, keyEnd-keyStart-1);
                    size_t colon = sub.find(':', keyEnd);
                    if (colon == std::string::npos) break;
                    size_t arrStart = sub.find('[', colon);
                    if (arrStart == std::string::npos) break;
                    size_t arrEnd = sub.find(']', arrStart);
                    if (arrEnd == std::string::npos) break;
                    std::string nums = sub.substr(arrStart+1, arrEnd-arrStart-1);
                    // parse numbers x,y,w,h (ignore origin if present)
                    std::replace(nums.begin(), nums.end(), ',', ' ');
                    std::stringstream ns(nums);
                    int x=0,y=0,w=0,h=0;
                    ns >> x >> y >> w >> h;
                    atlas->m_frames.emplace(key, Rectangle{(float)x,(float)y,(float)w,(float)h});
                    cur = arrEnd+1;
                }
            }
        }
    }

    // parse clips if present
    size_t clipsPos = s.find("\"clips\"");
    if (clipsPos != std::string::npos) {
        size_t brace = s.find('{', clipsPos);
        if (brace != std::string::npos) {
            // depth-based matching to find closing } of clips block
            size_t end = brace;
            int depth = 1;
            while (depth > 0 && ++end < s.length()) {
                if (s[end] == '{') depth++;
                else if (s[end] == '}') depth--;
            }
            if (depth == 0) {
                std::string sub = s.substr(brace+1, end-brace-1);
                size_t cur = 0;
                while (true) {
                    size_t keyStart = sub.find('"', cur);
                    if (keyStart == std::string::npos) break;
                    size_t keyEnd = sub.find('"', keyStart+1);
                    if (keyEnd == std::string::npos) break;
                    std::string clipName = sub.substr(keyStart+1, keyEnd-keyStart-1);
                    size_t colon = sub.find(':', keyEnd);
                    if (colon == std::string::npos) break;
                    size_t clipStart = sub.find('{', colon);
                    if (clipStart == std::string::npos) break;
                    // depth-based matching for clip body
                    size_t clipEnd = clipStart;
                    int cdepth = 1;
                    while (cdepth > 0 && ++clipEnd < sub.length()) {
                        if (sub[clipEnd] == '{') cdepth++;
                        else if (sub[clipEnd] == '}') cdepth--;
                    }
                    if (cdepth != 0) break;
                    std::string clipBody = sub.substr(clipStart+1, clipEnd-clipStart-1);
                    // find frames array
                    size_t framesKey = clipBody.find('"' + std::string("frames") + '"');
                    AnimationClip clip;
                    clip.name = clipName;
                    if (framesKey != std::string::npos) {
                        size_t fColon = clipBody.find(':', framesKey);
                        size_t fArr = clipBody.find('[', fColon);
                        size_t fEnd = clipBody.find(']', fArr);
                        if (fArr != std::string::npos && fEnd != std::string::npos) {
                            std::string names = clipBody.substr(fArr+1, fEnd-fArr-1);
                            // split by quotes
                            size_t p = 0;
                            while (true) {
                                size_t q1 = names.find('"', p);
                                if (q1 == std::string::npos) break;
                                size_t q2 = names.find('"', q1+1);
                                if (q2 == std::string::npos) break;
                                std::string fname = names.substr(q1+1, q2-q1-1);
                                auto it = atlas->m_frames.find(fname);
                                if (it != atlas->m_frames.end()) {
                                    AnimationFrame af; af.src = it->second; af.duration = 0.1f; af.origin = {0,0};
                                    clip.frames.push_back(af);
                                }
                                p = q2+1;
                            }
                        }
                    }
                    // durations
                    size_t durKey = clipBody.find('"' + std::string("durations") + '"');
                    if (durKey != std::string::npos) {
                        size_t dColon = clipBody.find(':', durKey);
                        size_t dArr = clipBody.find('[', dColon);
                        size_t dEnd = clipBody.find(']', dArr);
                        if (dArr != std::string::npos && dEnd != std::string::npos) {
                            std::string nums = clipBody.substr(dArr+1, dEnd-dArr-1);
                            std::replace(nums.begin(), nums.end(), ',', ' ');
                            std::stringstream ns(nums);
                            for (size_t i = 0; i < clip.frames.size(); ++i) {
                                float v = 0.1f; ns >> v; clip.frames[i].duration = v;
                            }
                        }
                    }
                    // loop
                    size_t loopKey = clipBody.find('"' + std::string("loop") + '"');
                    if (loopKey != std::string::npos) {
                        size_t lColon = clipBody.find(':', loopKey);
                        if (lColon != std::string::npos) {
                            size_t t = clipBody.find_first_not_of(" \t\n", lColon+1);
                            if (t != std::string::npos && clipBody[t] == 'f') clip.loop = false; else clip.loop = true;
                        }
                    }
                    atlas->m_clips.emplace(clipName, clip);
                    cur = clipEnd+1;
                }
            }
        }
    }

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

AnimationClip TextureAtlas::GetClip(const std::string& clipName) const {
    auto it = m_clips.find(clipName);
    if (it == m_clips.end()) return AnimationClip();
    return it->second;
}

} // namespace Systems
