#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include "View/TextureAtlas.h"

namespace View {

class AssetManager {
public:
    static AssetManager& GetInstance();

    // Start loading a list of JSON paths on a background thread.
    void StartLoading(const std::vector<std::string>& jsonPaths);

    // Update main thread: upload loaded images to GPU
    void UpdateMainThread();

    bool IsLoadingComplete() const;
    float GetProgress() const;

    // Get loaded atlas. If not loaded asynchronously, it falls back to synchronous load!
    std::shared_ptr<Animations::TextureAtlas> GetAtlas(const std::string& jsonPath);

private:
    AssetManager();
    ~AssetManager();

    std::unordered_map<std::string, std::shared_ptr<Animations::TextureAtlas>> m_atlases;
    std::mutex m_atlasMutex;

    // Threading
    std::thread m_workerThread;
    std::atomic<bool> m_isRunning{false};
    
    std::queue<std::string> m_pendingPaths;
    std::mutex m_queueMutex;

    // Items loaded by worker, waiting for GPU upload on main thread
    std::queue<std::pair<std::string, std::shared_ptr<Animations::TextureAtlas>>> m_uploadQueue;
    std::mutex m_uploadMutex;

    std::atomic<int> m_totalToLoad{0};
    std::atomic<int> m_loadedCount{0};

    void WorkerLoop();
};

} // namespace View
