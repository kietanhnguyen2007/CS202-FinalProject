#include "View/AssetManager.h"
#include <iostream>

namespace View {

AssetManager& AssetManager::GetInstance() {
    static AssetManager instance;
    return instance;
}

AssetManager::AssetManager() {
    m_isRunning = true;
    m_workerThread = std::thread(&AssetManager::WorkerLoop, this);
}

AssetManager::~AssetManager() {
    m_isRunning = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void AssetManager::StartLoading(const std::vector<std::string>& jsonPaths) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (const auto& path : jsonPaths) {
        // Skip if already loaded
        std::lock_guard<std::mutex> atlasLock(m_atlasMutex);
        if (m_atlases.find(path) != m_atlases.end()) continue;
        
        m_pendingPaths.push(path);
    }
    m_totalToLoad = m_pendingPaths.size();
    m_loadedCount = 0;
}

void AssetManager::WorkerLoop() {
    while (m_isRunning) {
        std::string path;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_pendingPaths.empty()) {
                path = m_pendingPaths.front();
                m_pendingPaths.pop();
                
                std::lock_guard<std::mutex> lock(m_currentNameMutex);
                // Extract just the filename from the path
                size_t pos = path.find_last_of("/\\\\");
                m_currentAssetName = (pos != std::string::npos) ? path.substr(pos + 1) : path;
            }
        }

        if (!path.empty()) {
            auto atlas = Animations::TextureAtlas::LoadFromJSON(path);
            if (atlas) {
                atlas->LoadImageAsync();
            }
            
            {
                std::lock_guard<std::mutex> lock(m_uploadMutex);
                m_uploadQueue.push({path, std::move(atlas)});
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void AssetManager::UpdateMainThread() {
    std::lock_guard<std::mutex> lock(m_uploadMutex);
    while (!m_uploadQueue.empty()) {
        auto [path, atlas] = m_uploadQueue.front();
        m_uploadQueue.pop();

        if (atlas) {
            atlas->UploadTextureFromImage();
        }

        {
            std::lock_guard<std::mutex> atlasLock(m_atlasMutex);
            m_atlases[path] = std::move(atlas);
        }
        
        m_loadedCount++;
    }
    
    float target = GetProgress();
    float current = m_displayProgress.load();
    float next = current + (target - current) * 0.15f;
    m_displayProgress.store(next);
}

bool AssetManager::IsLoadingComplete() const {
    return m_totalToLoad == 0 || m_loadedCount >= m_totalToLoad;
}

float AssetManager::GetProgress() const {
    if (m_totalToLoad == 0) return 1.0f;
    return static_cast<float>(m_loadedCount) / static_cast<float>(m_totalToLoad);
}

std::string AssetManager::GetCurrentAssetName() const {
    std::lock_guard<std::mutex> lock(m_currentNameMutex);
    return m_currentAssetName;
}

std::shared_ptr<Animations::TextureAtlas> AssetManager::GetAtlas(const std::string& jsonPath) {
    {
        std::lock_guard<std::mutex> lock(m_atlasMutex);
        auto it = m_atlases.find(jsonPath);
        if (it != m_atlases.end()) {
            return it->second;
        }
    }

    // Fallback: synchronous load if not preloaded
    auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
    if (atlas) {
        atlas->LoadTexture(); // sync GPU upload
    }
    
    std::lock_guard<std::mutex> lock(m_atlasMutex);
    m_atlases[jsonPath] = atlas;
    return atlas;
}

} // namespace View
