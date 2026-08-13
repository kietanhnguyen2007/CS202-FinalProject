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

    // Stop the loader thread and release every cached atlas while the graphics
    // context is still alive. Safe to call more than once.
    void Shutdown();

    // Update main thread: upload loaded images to GPU
    void UpdateMainThread();

    bool IsLoadingComplete() const;
    float GetProgress() const;

    // Returns the filename (basename only) of the asset currently being loaded.
    // Thread-safe. Returns empty string if nothing is loading.
    std::string GetCurrentAssetName() const;

    // Smooth display progress (lerps toward real progress, updated by UpdateMainThread)
    float GetDisplayProgress() const { return m_displayProgress.load(); }

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
    std::atomic<bool> m_shutdownComplete{false};
    
    std::queue<std::string> m_pendingPaths;
    std::mutex m_queueMutex;

    // Items loaded by worker, waiting for GPU upload on main thread
    std::queue<std::pair<std::string, std::shared_ptr<Animations::TextureAtlas>>> m_uploadQueue;
    std::mutex m_uploadMutex;

    std::atomic<int> m_totalToLoad{0};
    std::atomic<int> m_loadedCount{0};

    void WorkerLoop();

    std::atomic<float> m_displayProgress{0.0f};
    mutable std::mutex m_currentNameMutex;
    std::string m_currentAssetName;  // set by worker thread when picking a file
};

} // namespace View
