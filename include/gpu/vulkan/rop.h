#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>

namespace ps5 {

// ROP (Raster Order Pipeline) unit
class ROP {
public:
    ROP();
    ~ROP();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Render target management
    bool setRenderTarget(uint32_t index, uint32_t width, uint32_t height,
                        VkFormat format, VkImage image, VkImageView view);
    bool setDepthTarget(uint32_t width, uint32_t height, VkFormat format,
                       VkImage image, VkImageView view);
    bool clearRenderTarget(uint32_t index, float r, float g, float b, float a);
    bool clearDepthTarget(float depth, uint32_t stencil);
    
    // Blend state
    void setBlendState(uint32_t index, bool enabled, VkBlendFactor src_color,
                      VkBlendFactor dst_color, VkBlendOp color_op,
                      VkBlendFactor src_alpha, VkBlendFactor dst_alpha,
                      VkBlendOp alpha_op);
    void setBlendConstant(float r, float g, float b, float a);
    
    // Depth/stencil state
    void setDepthState(bool test_enabled, bool write_enabled,
                      VkCompareOp compare_op);
    void setStencilState(bool enabled, VkStencilOpState front,
                        VkStencilOpState back);
    
    // Output merger
    void mergeOutputs(VkCommandBuffer cmd, uint32_t width, uint32_t height,
                     uint32_t sample_count);
    
    // Statistics
    uint64_t getPixelsProcessed() const { return pixels_processed_; }
    uint64_t getSamplesProcessed() const { return samples_processed_; }
    
private:
    bool initialized_;
    
    struct RenderTarget {
        uint32_t width;
        uint32_t height;
        VkFormat format;
        VkImage image;
        VkImageView view;
    };
    
    std::vector<RenderTarget> render_targets_;
    RenderTarget depth_target_;
    
    struct BlendState {
        bool enabled;
        VkBlendFactor src_color;
        VkBlendFactor dst_color;
        VkBlendOp color_op;
        VkBlendFactor src_alpha;
        VkBlendFactor dst_alpha;
        VkBlendOp alpha_op;
        float constant[4];
    };
    
    std::vector<BlendState> blend_states_;
    
    struct DepthState {
        bool test_enabled;
        bool write_enabled;
        VkCompareOp compare_op;
    };
    
    DepthState depth_state_;
    
    struct StencilState {
        bool enabled;
        VkStencilOpState front;
        VkStencilOpState back;
    };
    
    StencilState stencil_state_;
    
    uint64_t pixels_processed_;
    uint64_t samples_processed_;
    
    std::mutex mutex_;
};

} // namespace ps5