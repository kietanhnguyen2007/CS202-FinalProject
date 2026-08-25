#include "View/AssetManager.h"
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace {

std::string NormalizeAssetPath(const std::string& path) {
    // Cache keys must be independent of the slash style used by the caller.
    // recursive_directory_iterator yields backslashes on Windows while most
    // gameplay code uses forward slashes.
    return std::filesystem::path(path).lexically_normal().generic_string();
}

} // namespace

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
    Shutdown();
}

void AssetManager::Shutdown() {
    // Only the first caller owns the teardown. main() invokes this before
    // CloseWindow(); the destructor is a harmless fallback.
    if (m_shutdownComplete.exchange(true)) return;

    m_isRunning = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    // Destroy queued and cached atlases now, on the main thread and while the
    // OpenGL context is valid. This avoids hundreds of UnloadTexture calls
    // being deferred until static destruction after CloseWindow().
    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        decltype(m_uploadQueue) empty;
        m_uploadQueue.swap(empty);
    }
    {
        std::lock_guard<std::mutex> lock(m_atlasMutex);
        m_atlases.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        decltype(m_pendingPaths) empty;
        m_pendingPaths.swap(empty);
    }
    {
        std::lock_guard<std::mutex> lock(m_currentNameMutex);
        m_currentAssetName.clear();
    }

    m_totalToLoad = 0;
    m_loadedCount = 0;
    m_displayProgress = 1.0f;
}

void AssetManager::StartLoading(const std::vector<std::string>& jsonPaths) {
    if (m_shutdownComplete.load()) return;
    std::lock_guard<std::mutex> lock(m_queueMutex);
    for (const auto& path : jsonPaths) {
        const std::string normalizedPath = NormalizeAssetPath(path);
        // Skip if already loaded
        std::lock_guard<std::mutex> atlasLock(m_atlasMutex);
        if (m_atlases.find(normalizedPath) != m_atlases.end()) continue;
        
        m_pendingPaths.push(normalizedPath);
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
                std::string friendly = path;
                std::replace(friendly.begin(), friendly.end(), '\\', '/');
                const std::string root = "assets/textures/";
                const size_t rootPos = friendly.find(root);
                if (rootPos != std::string::npos) friendly.erase(0, rootPos + root.size());
                const size_t extension = friendly.rfind(".json");
                if (extension != std::string::npos) friendly.erase(extension);
                std::string breadcrumb;
                for (char c : friendly) breadcrumb += c == '/' ? "  >  " : std::string(1, c);
                m_currentAssetName = breadcrumb;
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
    // Upload only a small batch per frame. GPU uploads must happen on the main
    // thread, but holding the queue mutex (and draining an unbounded queue) made
    // the loading screen freeze while also blocking the worker.
    constexpr std::size_t maxUploadsPerFrame = 4;
    for (std::size_t uploaded = 0; uploaded < maxUploadsPerFrame; ++uploaded) {
        std::pair<std::string, std::shared_ptr<Animations::TextureAtlas>> item;
        {
            std::lock_guard<std::mutex> lock(m_uploadMutex);
            if (m_uploadQueue.empty()) break;
            item = std::move(m_uploadQueue.front());
            m_uploadQueue.pop();
        }

        auto& [path, atlas] = item;

        // GetAtlas() may have loaded this path synchronously while the worker
        // was still decoding it. In that case the cached GPU atlas wins and we
        // discard this CPU-side duplicate before doing another GPU upload.
        {
            std::lock_guard<std::mutex> atlasLock(m_atlasMutex);
            auto cached = m_atlases.find(path);
            if (cached != m_atlases.end() && cached->second) {
                m_loadedCount++;
                continue;
            }
        }

        const bool ready = atlas && atlas->UploadTextureFromImage();
        if (ready) {
            std::lock_guard<std::mutex> atlasLock(m_atlasMutex);
            m_atlases[path] = std::move(atlas);
        } else {
            std::cerr << "[AssetManager] Failed to upload atlas: " << path << "\n";
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
    if (m_shutdownComplete.load()) return nullptr;

    const std::string normalizedPath = NormalizeAssetPath(jsonPath);
    {
        std::lock_guard<std::mutex> lock(m_atlasMutex);
        auto it = m_atlases.find(normalizedPath);
        if (it != m_atlases.end()) {
            return it->second;
        }
    }

    // Fallback: synchronous load if not preloaded
    auto atlas = Animations::TextureAtlas::LoadFromJSON(normalizedPath);
    if (atlas && !atlas->LoadTexture()) {
        atlas.reset();
    }

    if (!atlas) return nullptr;

    std::lock_guard<std::mutex> lock(m_atlasMutex);
    auto [it, inserted] = m_atlases.emplace(normalizedPath, atlas);
    return inserted ? std::move(atlas) : it->second;
}

} // namespace View
