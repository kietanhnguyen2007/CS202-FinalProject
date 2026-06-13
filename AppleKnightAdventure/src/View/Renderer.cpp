#include "View/Renderer.h"
#include "View/RenderTypes.h"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <thread>
#include <cstdint>
#include "rlgl.h"
#include "raymath.h"

namespace View {

static const int LAYER_COUNT = 4;

struct LayerBuffer {
    RenderCommand* commands = nullptr;
    uint32_t* indexOrder = nullptr;
    size_t count = 0;
    size_t capacity = 0;
};

static LayerBuffer* s_layers = nullptr;

// GPU batching resources (shared)
static Mesh s_mesh = {0};
static Material s_material = {0};
static float* s_vertices = nullptr;
static float* s_texcoords = nullptr;
static unsigned char* s_colors = nullptr;
static unsigned short* s_indices = nullptr;
static size_t s_maxQuads = 0;

// Lock-free MPSC bounded ring buffer
struct EnqueuedCommand {
    RenderCommand cmd;
    Layer layer;
};

struct SubmitCell {
    std::atomic<int> ready; // 0 = empty, 1 = full
    EnqueuedCommand item;
};

static SubmitCell* s_submitBuffer = nullptr;
static size_t s_submitMask = 0;
static size_t s_submitCapacity = 0;
static std::atomic<size_t> s_enqueuePos{0};
static std::atomic<size_t> s_dequeuePos{0};
static std::thread::id s_mainThreadId;
static std::atomic<size_t> s_pendingDropped{0};

Renderer& Renderer::GetInstance() {
    static Renderer inst;
    return inst;
}

bool Renderer::Init(size_t cmdCapacityPerLayer, size_t /*textureSlotsPerLayer*/) {
    if (m_initialized) return true;
    m_cmdCapacityPerLayer = cmdCapacityPerLayer;
    assert(m_cmdCapacityPerLayer <= 16383 && "Max 16383 quads per layer for 16-bit indices");

    s_layers = (LayerBuffer*)malloc(sizeof(LayerBuffer) * LAYER_COUNT);
    memset(s_layers, 0, sizeof(LayerBuffer) * LAYER_COUNT);
    for (int i = 0; i < LAYER_COUNT; ++i) {
        s_layers[i].commands = (RenderCommand*)malloc(sizeof(RenderCommand) * m_cmdCapacityPerLayer);
        s_layers[i].indexOrder = (uint32_t*)malloc(sizeof(uint32_t) * m_cmdCapacityPerLayer);
        s_layers[i].capacity = m_cmdCapacityPerLayer;
    }

    s_maxQuads = m_cmdCapacityPerLayer;
    size_t maxVerts = s_maxQuads * 4;
    size_t maxIndices = s_maxQuads * 6;
    s_vertices = (float*)malloc(sizeof(float) * 3 * maxVerts);
    s_texcoords = (float*)malloc(sizeof(float) * 2 * maxVerts);
    s_colors = (unsigned char*)malloc(sizeof(unsigned char) * 4 * maxVerts);
    s_indices = (unsigned short*)malloc(sizeof(unsigned short) * maxIndices);

    for (size_t q = 0; q < s_maxQuads; ++q) {
        unsigned short vbase = (unsigned short)(q * 4);
        size_t ibase = q * 6;
        s_indices[ibase + 0] = vbase + 0;
        s_indices[ibase + 1] = vbase + 1;
        s_indices[ibase + 2] = vbase + 2;
        s_indices[ibase + 3] = vbase + 0;
        s_indices[ibase + 4] = vbase + 2;
        s_indices[ibase + 5] = vbase + 3;
    }

    s_mesh.vertexCount = (int)maxVerts;
    s_mesh.triangleCount = (int)(s_maxQuads * 2);
    s_mesh.vertices = s_vertices;
    s_mesh.texcoords = s_texcoords;
    s_mesh.colors = s_colors;
    s_mesh.indices = s_indices;
    UploadMesh(&s_mesh, true);

    s_material = LoadMaterialDefault();

    m_windowWidth = GetScreenWidth();
    m_windowHeight = GetScreenHeight();
    m_initialized = true;
    m_drawCalls = 0;
    m_totalSubmitted = 0;

    s_mainThreadId = std::this_thread::get_id();
    size_t wanted = m_cmdCapacityPerLayer * (size_t)LAYER_COUNT * 2;
    size_t cap = 1;
    while (cap < wanted) cap <<= 1;
    s_submitCapacity = cap;
    s_submitMask = cap - 1;
    s_submitBuffer = (SubmitCell*)malloc(sizeof(SubmitCell) * cap);
    for (size_t i = 0; i < cap; ++i) {
        s_submitBuffer[i].ready.store(0);
    }
    return true;
}

void Renderer::Shutdown() {
    if (!m_initialized) return;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        free(s_layers[i].commands);
        free(s_layers[i].indexOrder);
    }
    free(s_layers);
    s_layers = nullptr;
    if (s_mesh.vboId) UnloadMesh(s_mesh);
    if (s_material.shader.id) UnloadMaterial(s_material);
    free(s_vertices); s_vertices = nullptr;
    free(s_texcoords); s_texcoords = nullptr;
    free(s_colors); s_colors = nullptr;
    free(s_indices); s_indices = nullptr;
    s_mesh = {0};
    if (s_submitBuffer) {
        free(s_submitBuffer);
        s_submitBuffer = nullptr;
    }
    s_submitCapacity = 0;
    s_submitMask = 0;
    s_enqueuePos.store(0);
    s_dequeuePos.store(0);
    if (m_whitePixelReady) {
        UnloadTexture(m_whitePixel);
        m_whitePixelReady = false;
    }
    m_initialized = false;
}

void Renderer::BeginFrame() {
    for (int i = 0; i < LAYER_COUNT; ++i) {
        s_layers[i].count = 0;
    }
    m_frameSubmitted = 0;
    m_droppedSubmissions = s_pendingDropped.exchange(0, std::memory_order_acquire);

    if (s_submitBuffer && s_submitCapacity > 0) {
        size_t localDeq = s_dequeuePos.load(std::memory_order_relaxed);
        size_t enq = s_enqueuePos.load(std::memory_order_acquire);
        while (localDeq < enq) {
            size_t idx = localDeq & s_submitMask;
            SubmitCell& cell = s_submitBuffer[idx];
            int ready = cell.ready.load(std::memory_order_acquire);
            if (ready != 1) break;

            EnqueuedCommand ec = cell.item;
            cell.ready.store(0, std::memory_order_release);

            LayerBuffer& lb = s_layers[static_cast<int>(ec.layer)];
            if (lb.count < lb.capacity) {
                lb.commands[lb.count] = ec.cmd;
                lb.indexOrder[lb.count] = (uint32_t)lb.count;
                lb.count++;
                m_totalSubmitted++;
                m_frameSubmitted++;
            } else {
                s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
            }

            ++localDeq;
        }
        s_dequeuePos.store(localDeq, std::memory_order_release);
    }
}

bool Renderer::SubmitSprite(Texture2D* texture,
                            const Rectangle& src,
                            Vector2 pos,
                            Vector2 scale,
                            float rotation,
                            Vector2 origin,
                            Color tint,
                            Layer layer,
                            float z,
                            bool flipX,
                            uint32_t entityId) {
    if (!m_initialized) return false;
    if (!texture) {
        s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    if (std::this_thread::get_id() == s_mainThreadId) {
        LayerBuffer& lb = s_layers[static_cast<int>(layer)];
        if (lb.count >= lb.capacity) {
            assert(false && "Renderer command capacity exceeded");
            s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        RenderCommand& cmd = lb.commands[lb.count];
        cmd.m_texture = texture;
        cmd.m_src = src;
        cmd.m_x = pos.x;
        cmd.m_y = pos.y;
        cmd.m_scaleX = scale.x;
        cmd.m_scaleY = scale.y;
        cmd.m_rotation = rotation;
        cmd.m_originX = origin.x;
        cmd.m_originY = origin.y;
        cmd.m_tint = tint;
        cmd.m_flipX = flipX;
        cmd.m_entityId = entityId;
        cmd.m_z = z;
        lb.indexOrder[lb.count] = (uint32_t)lb.count;
        lb.count++;
        m_totalSubmitted++;
        m_frameSubmitted++;
        return true;
    }

    if (!s_submitBuffer || s_submitCapacity == 0) {
        s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    EnqueuedCommand ec;
    ec.layer = layer;
    ec.cmd.m_texture = texture;
    ec.cmd.m_src = src;
    ec.cmd.m_x = pos.x;
    ec.cmd.m_y = pos.y;
    ec.cmd.m_scaleX = scale.x;
    ec.cmd.m_scaleY = scale.y;
    ec.cmd.m_rotation = rotation;
    ec.cmd.m_originX = origin.x;
    ec.cmd.m_originY = origin.y;
    ec.cmd.m_tint = tint;
    ec.cmd.m_flipX = flipX;
    ec.cmd.m_entityId = entityId;
    ec.cmd.m_z = z;

    while (true) {
        size_t curEnq = s_enqueuePos.load(std::memory_order_relaxed);
        size_t curDeq = s_dequeuePos.load(std::memory_order_acquire);
        if (curEnq - curDeq >= s_submitCapacity) {
            s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        if (s_enqueuePos.compare_exchange_weak(curEnq, curEnq + 1, std::memory_order_acq_rel)) {
            size_t idx = curEnq & s_submitMask;
            SubmitCell& cell = s_submitBuffer[idx];
            cell.item = ec;
            cell.ready.store(1, std::memory_order_release);
            return true;
        }
    }
}

void Renderer::EndFrameAndFlush() {
    if (!m_initialized) return;
    m_drawCalls = 0;

    for (int li = 0; li < LAYER_COUNT; ++li) {
        LayerBuffer& lb = s_layers[li];
        if (lb.count == 0) continue;

        std::stable_sort(lb.indexOrder, lb.indexOrder + lb.count, [&lb](uint32_t a, uint32_t b){
            return lb.commands[a].m_z < lb.commands[b].m_z;
        });

        for (size_t i = 0; i < lb.count; ++i) {
            RenderCommand& cmd = lb.commands[lb.indexOrder[i]];
            if (!cmd.m_texture) continue;

            Rectangle src = cmd.m_src;
            if (cmd.m_flipX) src.width = -src.width; // Flip horizontally

            Rectangle dest = { cmd.m_x, cmd.m_y, std::abs(src.width) * cmd.m_scaleX, std::abs(src.height) * cmd.m_scaleY };
            Vector2 origin = { cmd.m_originX, cmd.m_originY };

            ::DrawTexturePro(*cmd.m_texture, src, dest, origin, cmd.m_rotation, cmd.m_tint);
            m_drawCalls++;
        }
    }
}

void Renderer::ResizeWindow(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

void Renderer::EnsureWhitePixel() {
    if (m_whitePixelReady) return;
    Image img = GenImageColor(1, 1, WHITE);
    m_whitePixel = LoadTextureFromImage(img);
    UnloadImage(img);
    m_whitePixelReady = true;
}

void Renderer::DrawRectangle(Vector2 pos, Vector2 wsize, Color color,
                              Layer layer, float z) {
    EnsureWhitePixel();
    Rectangle src = {0, 0, 1, 1};
    SubmitSprite(&m_whitePixel, src, pos,
                 {wsize.x, wsize.y}, 0.0f, {0, 0}, color,
                 layer, z, false, 0);
}

void Renderer::DrawText(const char* text, Vector2 pos, int fontSize, Color color) {
    ::DrawText(text, (int)pos.x, (int)pos.y, fontSize, color);
}

} // namespace View
