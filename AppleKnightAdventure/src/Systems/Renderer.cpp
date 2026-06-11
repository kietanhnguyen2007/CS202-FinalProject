#include "Systems/Renderer.h"
#include "Systems/RenderTypes.h"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cmath>
#include "rlgl.h"
#include "raymath.h"
#include "raylib.h" // for Mesh/Material functions

namespace Systems {

static const int LAYER_COUNT = 4;

struct LayerBuffer {
    RenderCommand* commands = nullptr;
    uint32_t* indexOrder = nullptr; // indices into commands for sorting
    size_t count = 0;
    size_t capacity = 0;
    Texture2D** textureSlots = nullptr;
    size_t textureSlotCount = 0;
    size_t textureSlotCapacity = 0;
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

Renderer& Renderer::GetInstance() {
    static Renderer inst;
    return inst;
}

bool Renderer::Init(size_t cmdCapacityPerLayer, size_t textureSlotsPerLayer) {
    if (m_initialized) return true;
    m_cmdCapacityPerLayer = cmdCapacityPerLayer;
    m_textureSlotsPerLayer = textureSlotsPerLayer;

    s_layers = (LayerBuffer*)malloc(sizeof(LayerBuffer) * LAYER_COUNT);
    memset(s_layers, 0, sizeof(LayerBuffer) * LAYER_COUNT);
    for (int i = 0; i < LAYER_COUNT; ++i) {
        s_layers[i].commands = (RenderCommand*)malloc(sizeof(RenderCommand) * m_cmdCapacityPerLayer);
        s_layers[i].indexOrder = (uint32_t*)malloc(sizeof(uint32_t) * m_cmdCapacityPerLayer);
        s_layers[i].capacity = m_cmdCapacityPerLayer;
        s_layers[i].textureSlots = (Texture2D**)malloc(sizeof(Texture2D*) * m_textureSlotsPerLayer);
        s_layers[i].textureSlotCapacity = m_textureSlotsPerLayer;
        memset(s_layers[i].textureSlots, 0, sizeof(Texture2D*) * m_textureSlotsPerLayer);
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

    m_initialized = true;
    m_drawCalls = 0;
    m_totalSubmitted = 0;
    return true;
}

void Renderer::Shutdown() {
    if (!m_initialized) return;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        free(s_layers[i].commands);
        free(s_layers[i].indexOrder);
        free(s_layers[i].textureSlots);
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
    m_initialized = false;
}

void Renderer::BeginFrame(const Camera2D* camera) {
    (void)camera;
    for (int i = 0; i < LAYER_COUNT; ++i) {
        s_layers[i].count = 0;
        s_layers[i].textureSlotCount = 0;
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
    LayerBuffer& lb = s_layers[static_cast<int>(layer)];
    if (lb.count >= lb.capacity) {
#ifndef NDEBUG
        assert(false && "Renderer command capacity exceeded");
#endif
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
    return true;
}

// helper to find or add texture slot (linear scan)
static int FindOrAddTextureSlot(LayerBuffer& lb, Texture2D* tex) {
    for (size_t i = 0; i < lb.textureSlotCount; ++i) if (lb.textureSlots[i] == tex) return (int)i;
    if (lb.textureSlotCount < lb.textureSlotCapacity) {
        size_t idx = lb.textureSlotCount++;
        lb.textureSlots[idx] = tex;
        return (int)idx;
    }
    return -1;
}

// Emit a quad using rlgl immediate-mode calls for current bound texture
static void EmitQuad(const RenderCommand& cmd) {
    Rectangle src = cmd.m_src;
    // compute dest rectangle
    float w = src.width * cmd.m_scaleX;
    float h = src.height * cmd.m_scaleY;
    float ox = cmd.m_originX;
    float oy = cmd.m_originY;
    // compute corners centered at pos - origin
    // top-left
    float x0 = cmd.m_x - ox;
    float y0 = cmd.m_y - oy;
    float x1 = x0 + w;
    float y1 = y0 + h;

    // texture coords
    float u0 = src.x / (float)cmd.m_texture->width;
    float v0 = src.y / (float)cmd.m_texture->height;
    float u1 = (src.x + src.width) / (float)cmd.m_texture->width;
    float v1 = (src.y + src.height) / (float)cmd.m_texture->height;

    // color
    rlColor4ub(cmd.m_tint.r, cmd.m_tint.g, cmd.m_tint.b, cmd.m_tint.a);

    // If rotation is zero and flipX false, we can emit simple quad
    if (cmd.m_rotation == 0.0f && !cmd.m_flipX) {
        rlTexCoord2f(u0, v0); rlVertex3f(x0, y0, cmd.m_z);
        rlTexCoord2f(u1, v0); rlVertex3f(x1, y0, cmd.m_z);
        rlTexCoord2f(u1, v1); rlVertex3f(x1, y1, cmd.m_z);
        rlTexCoord2f(u0, v1); rlVertex3f(x0, y1, cmd.m_z);
        return;
    }

    // Fallback: compute rotated corners
    float cx = cmd.m_x;
    float cy = cmd.m_y;
    float hw = w;
    float hh = h;
    float ang = cmd.m_rotation * (3.14159265f/180.0f);
    float ca = cosf(ang), sa = sinf(ang);

    // local corners relative to center
    float rx0 = -ox;
    float ry0 = -oy;
    float rx1 = -ox + w;
    float ry1 = -oy + h;

    auto rot = [&](float rx, float ry, float &oxr, float &oyr){
        oxr = cx + rx * ca - ry * sa;
        oyr = cy + rx * sa + ry * ca;
    };

    float ax, ay, bx, by, cx2, cy2, dx, dy;
    rot(rx0, ry0, ax, ay);
    rot(rx1, ry0, bx, by);
    rot(rx1, ry1, cx2, cy2);
    rot(rx0, ry1, dx, dy);

    if (cmd.m_flipX) {
        // swap u coordinates
        std::swap(u0, u1);
    }

    rlTexCoord2f(u0, v0); rlVertex3f(ax, ay, cmd.m_z);
    rlTexCoord2f(u1, v0); rlVertex3f(bx, by, cmd.m_z);
    rlTexCoord2f(u1, v1); rlVertex3f(cx2, cy2, cmd.m_z);
    rlTexCoord2f(u0, v1); rlVertex3f(dx, dy, cmd.m_z);
}

void Renderer::EndFrameAndFlush() {
    if (!m_initialized) return;
    m_drawCalls = 0;

    for (int li = 0; li < LAYER_COUNT; ++li) {
        LayerBuffer& lb = s_layers[li];
        if (lb.count == 0) continue;

        // stable sort indexOrder by z using lambda
        std::stable_sort(lb.indexOrder, lb.indexOrder + lb.count, [&lb](uint32_t a, uint32_t b){
            return lb.commands[a].m_z < lb.commands[b].m_z;
        });

        // build texture slots in sorted order and form runs of same texture
        size_t i = 0;
        while (i < lb.count) {
            Texture2D* tex = lb.commands[lb.indexOrder[i]].m_texture;
            // collect run length
            size_t j = i;
            while (j < lb.count && lb.commands[lb.indexOrder[j]].m_texture == tex) ++j;
            size_t runSize = j - i;

            // split run into batches of at most s_maxQuads
            size_t processed = 0;
            while (processed < runSize) {
                size_t batchCount = runSize - processed;
                if (batchCount > s_maxQuads) batchCount = s_maxQuads;

                // fill CPU buffers for this batch
                size_t vertBase = 0;
                size_t idxBase = 0;
                for (size_t b = 0; b < batchCount; ++b) {
                    RenderCommand& cmd = lb.commands[lb.indexOrder[i + processed + b]];
                    // compute quad vertices
                    Rectangle src = cmd.m_src;
                    float w = src.width * cmd.m_scaleX;
                    float h = src.height * cmd.m_scaleY;
                    float ox = cmd.m_originX;
                    float oy = cmd.m_originY;
                    float x0 = cmd.m_x - ox;
                    float y0 = cmd.m_y - oy;
                    float x1 = x0 + w;
                    float y1 = y0 + h;
                    float u0 = src.x / (float)cmd.m_texture->width;
                    float v0 = src.y / (float)cmd.m_texture->height;
                    float u1 = (src.x + src.width) / (float)cmd.m_texture->width;
                    float v1 = (src.y + src.height) / (float)cmd.m_texture->height;

                    // vertex 0
                    s_vertices[vertBase + 0] = x0;
                    s_vertices[vertBase + 1] = y0;
                    s_vertices[vertBase + 2] = cmd.m_z;
                    s_texcoords[(vertBase/3)*2 + 0] = u0;
                    s_texcoords[(vertBase/3)*2 + 1] = v0;
                    size_t colorIdx = (vertBase/3)*4;
                    s_colors[colorIdx+0] = cmd.m_tint.r;
                    s_colors[colorIdx+1] = cmd.m_tint.g;
                    s_colors[colorIdx+2] = cmd.m_tint.b;
                    s_colors[colorIdx+3] = cmd.m_tint.a;

                    // vertex 1
                    s_vertices[vertBase + 3] = x1;
                    s_vertices[vertBase + 4] = y0;
                    s_vertices[vertBase + 5] = cmd.m_z;
                    s_texcoords[(vertBase/3)*2 + 2] = u1;
                    s_texcoords[(vertBase/3)*2 + 3] = v0;
                    s_colors[colorIdx+4] = cmd.m_tint.r;
                    s_colors[colorIdx+5] = cmd.m_tint.g;
                    s_colors[colorIdx+6] = cmd.m_tint.b;
                    s_colors[colorIdx+7] = cmd.m_tint.a;

                    // vertex 2
                    s_vertices[vertBase + 6] = x1;
                    s_vertices[vertBase + 7] = y1;
                    s_vertices[vertBase + 8] = cmd.m_z;
                    s_texcoords[(vertBase/3)*2 + 4] = u1;
                    s_texcoords[(vertBase/3)*2 + 5] = v1;
                    s_colors[colorIdx+8] = cmd.m_tint.r;
                    s_colors[colorIdx+9] = cmd.m_tint.g;
                    s_colors[colorIdx+10] = cmd.m_tint.b;
                    s_colors[colorIdx+11] = cmd.m_tint.a;

                    // vertex 3
                    s_vertices[vertBase + 9] = x0;
                    s_vertices[vertBase + 10] = y1;
                    s_vertices[vertBase + 11] = cmd.m_z;
                    s_texcoords[(vertBase/3)*2 + 6] = u0;
                    s_texcoords[(vertBase/3)*2 + 7] = v1;
                    s_colors[colorIdx+12] = cmd.m_tint.r;
                    s_colors[colorIdx+13] = cmd.m_tint.g;
                    s_colors[colorIdx+14] = cmd.m_tint.b;
                    s_colors[colorIdx+15] = cmd.m_tint.a;

                    vertBase += 12; // 4 verts * 3 floats
                    idxBase += 6;    // 6 indices
                }

                // Update GPU buffers for this batch (use UpdateMeshBuffer)
                // vertices buffer index = 0, texcoords = 1, colors = 5, indices = 6
                int vertsCount = (int)(batchCount * 4);
                int indicesCount = (int)(batchCount * 6);
                // Update positions
                UpdateMeshBuffer(s_mesh, 0, s_vertices, sizeof(float) * 3 * vertsCount, 0);
                // Update texcoords
                UpdateMeshBuffer(s_mesh, 1, s_texcoords, sizeof(float) * 2 * vertsCount, 0);
                // Update colors
                UpdateMeshBuffer(s_mesh, 5, s_colors, sizeof(unsigned char) * 4 * vertsCount, 0);
                // Update indices
                UpdateMeshBuffer(s_mesh, 6, s_indices, sizeof(unsigned short) * indicesCount, 0);

                // Set mesh counts for draw
                s_mesh.vertexCount = vertsCount;
                s_mesh.triangleCount = (int)batchCount * 2;

                // bind texture on material and draw
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
    (void)width; (void)height;
}

} // namespace Systems
