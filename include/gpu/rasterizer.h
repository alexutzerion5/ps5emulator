#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace ps5 {

// Vertex structure
struct Vertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b, a;
};

// Triangle
struct Triangle {
    Vertex v0, v1, v2;
    uint32_t material_id;
};

// Rasterizer state
struct RasterizerState {
    bool cull_enabled;
    VkCullModeFlags cull_mode;
    VkFrontFace front_face;
    float depth_bias;
    float slope_scaled_depth_bias;
    float line_width;
};

// Viewport
struct Viewport {
    float x, y;
    float width, height;
    float min_depth, max_depth;
};

// Scissor
struct Scissor {
    int32_t x, y;
    uint32_t width, height;
};

// Rasterizer
class Rasterizer {
public:
    Rasterizer();
    ~Rasterizer();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // State management
    void setRasterizerState(const RasterizerState& state);
    void setViewports(const std::vector<Viewport>& viewports);
    void setScissors(const std::vector<Scissor>& scissors);
    
    // Rasterization
    void rasterizeTriangles(const std::vector<Triangle>& triangles,
                           std::vector<uint32_t>& depth_buffer,
                           std::vector<uint8_t>& color_buffer,
                           uint32_t width, uint32_t height);
    
    // Fragment processing
    void processFragment(uint32_t x, uint32_t y, const Triangle& tri,
                        float b0, float b1, float b2,
                        std::vector<uint32_t>& depth_buffer,
                        std::vector<uint8_t>& color_buffer,
                        uint32_t width, uint32_t height);
    
    // Texturing
    void sampleTexture(uint32_t texture_id, float u, float v,
                      uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);
    
    // Statistics
    uint64_t getTrianglesProcessed() const { return triangles_processed_; }
    uint64_t getFragmentsProcessed() const { return fragments_processed_; }
    
private:
    bool initialized_;
    RasterizerState state_;
    std::vector<Viewport> viewports_;
    std::vector<Scissor> scissors_;
    
    std::vector<std::vector<uint8_t>> textures_;
    std::vector<uint32_t> texture_sizes_;
    
    uint64_t triangles_processed_;
    uint64_t fragments_processed_;
    
    std::mutex mutex_;
    
    // Helper functions
    bool insideTriangle(float x, float y, const Triangle& tri);
    float computeBarycentricCoord(float x, float y, const Triangle& tri, int vertex);
    void clipTriangle(const Triangle& tri, std::vector<Triangle>& clipped);
};

} // namespace ps5