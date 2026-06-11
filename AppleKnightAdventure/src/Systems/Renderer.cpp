#include "Systems/Renderer.h"
#include "Systems/RenderTypes.h"
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
#include "raylib.h" // for Mesh/Material functions

namespace Systems {

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
static float* s_vertices = nullptr;    // positions (x,y,z) per vertex
static float* s_texcoords = nullptr;   // u,v per vertex
static unsigned char* s_colors = nullptr; // r,g,b,a per vertex
static unsigned short* s_indices = nullptr; // indices per triangle
static size_t s_maxQuads = 0;

// Lock-free MPSC bounded ring buffer for submissions (simple cell-based protocol).
// This is a constrained implementation optimized for single consumer (main thread)
// and multiple producers (worker threads) that call SubmitSprite().
struct EnqueuedCommand {
    RenderCommand cmd;
    Layer layer;
};

struct SubmitCell {
    std::atomic<int> ready; // 0 = empty, 1 = full
    EnqueuedCommand item;
};

static SubmitCell* s_submitBuffer = nullptr;
static size_t s_submitMask = 0; // capacity - 1 (power of two)
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

    // Prepare GPU mesh buffers for batching (max quads = cmdCapacityPerLayer)
    s_maxQuads = m_cmdCapacityPerLayer;
    // allocate CPU staging buffers (max vertices = 4 * maxQuads)
    size_t maxVerts = s_maxQuads * 4;
    size_t maxIndices = s_maxQuads * 6;
    s_vertices = (float*)malloc(sizeof(float) * 3 * maxVerts);
    s_texcoords = (float*)malloc(sizeof(float) * 2 * maxVerts);
    s_colors = (unsigned char*)malloc(sizeof(unsigned char) * 4 * maxVerts);
    s_indices = (unsigned short*)malloc(sizeof(unsigned short) * maxIndices);

    // prefill index buffer pattern
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

    // Setup Mesh struct and upload once (dynamic)
    s_mesh.vertexCount = (int)maxVerts;
    s_mesh.triangleCount = (int)(s_maxQuads * 2);
    s_mesh.vertices = s_vertices;
    s_mesh.texcoords = s_texcoords;
    s_mesh.colors = s_colors;
    s_mesh.indices = s_indices;
    UploadMesh(&s_mesh, true); // dynamic mesh

    // Load default material and keep it
    s_material = LoadMaterialDefault();

    m_windowWidth = GetScreenWidth();
    m_windowHeight = GetScreenHeight();
    m_initialized = true;
    m_drawCalls = 0;
    m_totalSubmitted = 0;
    // initialize lock-free submit buffer
    s_mainThreadId = std::this_thread::get_id();
    // capacity: choose next power-of-two from (cmdCapacityPerLayer * LAYER_COUNT * 2)
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
    // free GPU batching resources
    if (s_mesh.vboId) UnloadMesh(s_mesh);
    if (s_material.shader.id) UnloadMaterial(s_material);
    free(s_vertices); s_vertices = nullptr;
    free(s_texcoords); s_texcoords = nullptr;
    free(s_colors); s_colors = nullptr;
    free(s_indices); s_indices = nullptr;
    // clear dangling mesh pointers
    s_mesh = {0};
    // free submit buffer
    if (s_submitBuffer) {
        free(s_submitBuffer);
        s_submitBuffer = nullptr;
    }
    s_submitCapacity = 0;
    s_submitMask = 0;
    s_enqueuePos.store(0);
    s_dequeuePos.store(0);
    m_initialized = false;
}

void Renderer::BeginFrame() {
    for (int i = 0; i < LAYER_COUNT; ++i) {
        s_layers[i].count = 0;
    }
    m_frameSubmitted = 0;
    m_droppedSubmissions = s_pendingDropped.exchange(0, std::memory_order_acquire);

    // Drain lock-free submit buffer (commands pushed from worker threads)
    if (s_submitBuffer && s_submitCapacity > 0) {
        size_t localDeq = s_dequeuePos.load(std::memory_order_relaxed);
        size_t enq = s_enqueuePos.load(std::memory_order_acquire);
        while (localDeq < enq) {
            size_t idx = localDeq & s_submitMask;
            SubmitCell& cell = s_submitBuffer[idx];
            int ready = cell.ready.load(std::memory_order_acquire);
            if (ready != 1) break; // not yet written

            EnqueuedCommand ec = cell.item; // copy out
            cell.ready.store(0, std::memory_order_release);

            // push into layer buffer (same as in SubmitSprite main-thread path)
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

    // Fast path: if caller is main thread, write directly into LayerBuffer
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

    // Producer thread path: attempt to enqueue to lock-free ring buffer
    if (!s_submitBuffer || s_submitCapacity == 0) {
        // fallback to dropping if no buffer
        s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    // Prepare the enqueued command
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

    // Acquire slot using CAS to avoid overflow
    while (true) {
        size_t curEnq = s_enqueuePos.load(std::memory_order_relaxed);
        size_t curDeq = s_dequeuePos.load(std::memory_order_acquire);
        if (curEnq - curDeq >= s_submitCapacity) {
            // buffer full
            s_pendingDropped.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        if (s_enqueuePos.compare_exchange_weak(curEnq, curEnq + 1, std::memory_order_acq_rel)) {
            size_t idx = curEnq & s_submitMask;
            SubmitCell& cell = s_submitBuffer[idx];
            // write item
            cell.item = ec;
            // publish
            cell.ready.store(1, std::memory_order_release);
            return true;
        }
        // otherwise CAS failed, retry
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

        size_t i = 0;
        while (i < lb.count) {
            Texture2D* tex = lb.commands[lb.indexOrder[i]].m_texture;
            size_t j = i;
            while (j < lb.count && lb.commands[lb.indexOrder[j]].m_texture == tex) ++j;
            size_t runSize = j - i;

            size_t processed = 0;
            while (processed < runSize) {
                size_t batchCount = runSize - processed;
                if (batchCount > s_maxQuads) batchCount = s_maxQuads;

                size_t vertBase = 0;
                size_t colorIdx = 0;
                for (size_t b = 0; b < batchCount; ++b) {
                    RenderCommand& cmd = lb.commands[lb.indexOrder[i + processed + b]];
                    Rectangle src = cmd.m_src;
                    float w = src.width * cmd.m_scaleX;
                    float h = src.height * cmd.m_scaleY;

                    float u0 = src.x / (float)cmd.m_texture->width;
                    float v0 = src.y / (float)cmd.m_texture->height;
                    float u1 = (src.x + src.width) / (float)cmd.m_texture->width;
                    float v1 = (src.y + src.height) / (float)cmd.m_texture->height;
                    if (cmd.m_flipX) std::swap(u0, u1);

                    // local corners relative to origin
                    float lx[4] = { -cmd.m_originX, -cmd.m_originX + w, -cmd.m_originX + w, -cmd.m_originX };
                    float ly[4] = { -cmd.m_originY, -cmd.m_originY, -cmd.m_originY + h, -cmd.m_originY + h };

                    if (cmd.m_rotation != 0.0f) {
                        float ang = cmd.m_rotation * DEG2RAD;
                        float ca = cosf(ang), sa = sinf(ang);
                        float cx = cmd.m_x, cy = cmd.m_y;
                        for (int v = 0; v < 4; ++v) {
                            float rx = lx[v], ry = ly[v];
                            lx[v] = cx + rx * ca - ry * sa;
                            ly[v] = cy + rx * sa + ry * ca;
                        }
                    } else {
                        for (int v = 0; v < 4; ++v) {
                            lx[v] += cmd.m_x;
                            ly[v] += cmd.m_y;
                        }
                    }

                    for (int v = 0; v < 4; ++v) {
                        s_vertices[vertBase + v*3 + 0] = lx[v];
                        s_vertices[vertBase + v*3 + 1] = ly[v];
                        s_vertices[vertBase + v*3 + 2] = cmd.m_z;
                    }

                    float u[4] = {u0, u1, u1, u0};
                    float v[4] = {v0, v0, v1, v1};
                    for (int vi = 0; vi < 4; ++vi) {
                        s_texcoords[(vertBase/3)*2 + vi*2 + 0] = u[vi];
                        s_texcoords[(vertBase/3)*2 + vi*2 + 1] = v[vi];
                    }

                    for (int v = 0; v < 4; ++v) {
                        s_colors[colorIdx + v*4 + 0] = cmd.m_tint.r;
                        s_colors[colorIdx + v*4 + 1] = cmd.m_tint.g;
                        s_colors[colorIdx + v*4 + 2] = cmd.m_tint.b;
                        s_colors[colorIdx + v*4 + 3] = cmd.m_tint.a;
                    }

                    vertBase += 12;
                    colorIdx += 16;
                }

                int vertsCount = (int)(batchCount * 4);
                int indicesCount = (int)(batchCount * 6);
                UpdateMeshBuffer(s_mesh, 0, s_vertices, sizeof(float) * 3 * vertsCount, 0);
                UpdateMeshBuffer(s_mesh, 1, s_texcoords, sizeof(float) * 2 * vertsCount, 0);
                UpdateMeshBuffer(s_mesh, 3, s_colors, sizeof(unsigned char) * 4 * vertsCount, 0);
                UpdateMeshBuffer(s_mesh, 6, s_indices, sizeof(unsigned short) * indicesCount, 0);

                s_mesh.vertexCount = vertsCount;
                s_mesh.triangleCount = (int)batchCount * 2;

                SetMaterialTexture(&s_material, MATERIAL_MAP_DIFFUSE, *tex);
                DrawMesh(s_mesh, s_material, MatrixIdentity());
                m_drawCalls++;

                processed += batchCount;
            }
            i = j;
        }
    }
}

void Renderer::ResizeWindow(int width, int height) {
    m_windowWidth = width;
    m_windowHeight = height;
}

} // namespace Systems
