#include "gpu/rasterizer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace ps5 {

Rasterizer::Rasterizer() : initialized_(false), triangles_processed_(0), fragments_processed_(0) {
}

Rasterizer::~Rasterizer() {
    shutdown();
}

bool Rasterizer::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Set default state
    state_.cull_enabled = true;
    state_.cull_mode = VK_CULL_MODE_BACK_BIT;
    state_.front_face = VK_FRONT_FACE_CLOCKWISE;
    state_.depth_bias = 0.0f;
    state_.slope_scaled_depth_bias = 0.0f;
    state_.line_width = 1.0f;
    
    // Set default viewports
    Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 1920.0f;
    viewport.height = 1080.0f;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    viewports_.push_back(viewport);
    
    // Set default scissors
    Scissor scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = 1920;
    scissor.height = 1080;
    scissors_.push_back(scissor);
    
    initialized_ = true;
    
    return true;
}

void Rasterizer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
}

void Rasterizer::setRasterizerState(const RasterizerState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_ = state;
}

void Rasterizer::setViewports(const std::vector<Viewport>& viewports) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    viewports_ = viewports;
}

void Rasterizer::setScissors(const std::vector<Scissor>& scissors) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    scissors_ = scissors;
}

void Rasterizer::rasterizeTriangles(const std::vector<Triangle>& triangles,
                                   std::vector<uint32_t>& depth_buffer,
                                   std::vector<uint8_t>& color_buffer,
                                   uint32_t width, uint32_t height) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    triangles_processed_ += triangles.size();
    
    // Process each triangle
    for (const auto& tri : triangles) {
        // Clip triangle
        std::vector<Triangle> clipped;
        clipTriangle(tri, clipped);
        
        // Process each clipped triangle
        for (const auto& clipped_tri : clipped) {
            // Calculate bounding box
            float min_x = std::min({clipped_tri.v0.x, clipped_tri.v1.x, clipped_tri.v2.x});
            float max_x = std::max({clipped_tri.v0.x, clipped_tri.v1.x, clipped_tri.v2.x});
            float min_y = std::min({clipped_tri.v0.y, clipped_tri.v1.y, clipped_tri.v2.y});
            float max_y = std::max({clipped_tri.v0.y, clipped_tri.v1.y, clipped_tri.v2.y});
            
            // Clamp to viewport
            min_x = std::max(min_x, 0.0f);
            max_x = std::min(max_x, static_cast<float>(width - 1));
            min_y = std::max(min_y, 0.0f);
            max_y = std::min(max_y, static_cast<float>(height - 1));
            
            // Rasterize
            for (uint32_t y = static_cast<uint32_t>(min_y); y <= static_cast<uint32_t>(max_y); y++) {
                for (uint32_t x = static_cast<uint32_t>(min_x); x <= static_cast<uint32_t>(max_x); x++) {
                    // Check if inside triangle
                    if (insideTriangle(x + 0.5f, y + 0.5f, clipped_tri)) {
                        // Calculate barycentric coordinates
                        float b0 = computeBarycentricCoord(x + 0.5f, y + 0.5f, clipped_tri, 0);
                        float b1 = computeBarycentricCoord(x + 0.5f, y + 0.5f, clipped_tri, 1);
                        float b2 = computeBarycentricCoord(x + 0.5f, y + 0.5f, clipped_tri, 2);
                        
                        // Process fragment
                        processFragment(x, y, clipped_tri, b0, b1, b2,
                                       depth_buffer, color_buffer, width, height);
                    }
                }
            }
        }
    }
}

void Rasterizer::processFragment(uint32_t x, uint32_t y, const Triangle& tri,
                                float b0, float b1, float b2,
                                std::vector<uint32_t>& depth_buffer,
                                std::vector<uint8_t>& color_buffer,
                                uint32_t width, uint32_t height) {
    // Calculate depth
    float depth = b0 * tri.v0.z + b1 * tri.v1.z + b2 * tri.v2.z;
    
    // Check depth test
    uint32_t depth_index = y * width + x;
    if (depth_buffer[depth_index] != 0 && depth >= *reinterpret_cast<float*>(&depth_buffer[depth_index])) {
        return;
    }
    
    // Update depth buffer
    *reinterpret_cast<float*>(&depth_buffer[depth_index]) = depth;
    
    // Calculate color
    float r = b0 * tri.v0.r + b1 * tri.v1.r + b2 * tri.v2.r;
    float g = b0 * tri.v0.g + b1 * tri.v1.g + b2 * tri.v2.g;
    float b = b0 * tri.v0.b + b1 * tri.v1.b + b2 * tri.v2.b;
    float a = b0 * tri.v0.a + b1 * tri.v1.a + b2 * tri.v2.a;
    
    // Sample texture
    uint8_t tex_r, tex_g, tex_b, tex_a;
    sampleTexture(0, b0 * tri.v0.u + b1 * tri.v1.u + b2 * tri.v2.u,
                 b0 * tri.v0.v + b1 * tri.v1.v + b2 * tri.v2.v,
                 tex_r, tex_g, tex_b, tex_a);
    
    // Apply texture
    r *= tex_r / 255.0f;
    g *= tex_g / 255.0f;
    b *= tex_b / 255.0f;
    a *= tex_a / 255.0f;
    
    // Write color
    uint32_t color_index = (y * width + x) * 4;
    color_buffer[color_index + 0] = static_cast<uint8_t>(r * 255.0f);
    color_buffer[color_index + 1] = static_cast<uint8_t>(g * 255.0f);
    color_buffer[color_index + 2] = static_cast<uint8_t>(b * 255.0f);
    color_buffer[color_index + 3] = static_cast<uint8_t>(a * 255.0f);
    
    fragments_processed_++;
}

void Rasterizer::sampleTexture(uint32_t texture_id, float u, float v,
                              uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
    // Simplified: sample texture
    // In real implementation, this would sample from texture memory
    
    r = 255;
    g = 255;
    b = 255;
    a = 255;
}

bool Rasterizer::insideTriangle(float x, float y, const Triangle& tri) {
    // Calculate edge functions
    float e0 = (tri.v1.x - tri.v0.x) * (y - tri.v0.y) - (tri.v1.y - tri.v0.y) * (x - tri.v0.x);
    float e1 = (tri.v2.x - tri.v1.x) * (y - tri.v1.y) - (tri.v2.y - tri.v1.y) * (x - tri.v1.x);
    float e2 = (tri.v0.x - tri.v2.x) * (y - tri.v2.y) - (tri.v0.y - tri.v2.y) * (x - tri.v2.x);
    
    // Check if inside (with winding order)
    return (e0 >= 0 && e1 >= 0 && e2 >= 0) || (e0 <= 0 && e1 <= 0 && e2 <= 0);
}

float Rasterizer::computeBarycentricCoord(float x, float y, const Triangle& tri, int vertex) {
    float x0 = tri.v0.x, y0 = tri.v0.y;
    float x1 = tri.v1.x, y1 = tri.v1.y;
    float x2 = tri.v2.x, y2 = tri.v2.y;
    
    if (vertex == 0) {
        return ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) /
               ((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));
    } else if (vertex == 1) {
        return ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) /
               ((y2 - y0) * (x1 - x2) + (x0 - x2) * (y1 - y2));
    } else {
        return 1.0f - computeBarycentricCoord(x, y, tri, 0) - computeBarycentricCoord(x, y, tri, 1);
    }
}

void Rasterizer::clipTriangle(const Triangle& tri, std::vector<Triangle>& clipped) {
    // Simplified: clip triangle
    // In real implementation, this would clip against view frustum
    
    clipped.push_back(tri);
}

} // namespace ps5