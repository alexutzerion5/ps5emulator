#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <functional>

#ifdef __linux__
#include <vulkan/vulkan.h>
#elif _WIN32
#include <vulkan/vulkan.h>
#endif

namespace ps5 {

// Vulkan renderer
class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Window
    bool createWindow(uint32_t width, uint32_t height, const std::string& title);
    void resizeWindow(uint32_t width, uint32_t height);
    
    // Rendering
    bool beginFrame();
    bool endFrame();
    void present();
    
    // Shader compilation
    bool compileShader(const std::string& path, VkShaderModule& shader);
    bool compileComputeShader(const std::string& path, VkShaderModule& shader);
    bool compileVertexShader(const std::string& path, VkShaderModule& shader);
    bool compileFragmentShader(const std::string& path, VkShaderModule& shader);
    
    // Pipeline creation
    bool createGraphicsPipeline(const std::string& vertex_shader,
                               const std::string& fragment_shader,
                               VkPipeline& pipeline);
    bool createComputePipeline(const std::string& compute_shader,
                              VkPipeline& pipeline);
    
    // Buffer creation
    bool createBuffer(VkBuffer& buffer, VkDeviceMemory& memory,
                     VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties);
    bool createVertexBuffer(VkBuffer& buffer, VkDeviceMemory& memory,
                           const void* data, VkDeviceSize size);
    bool createIndexBuffer(VkBuffer& buffer, VkDeviceMemory& memory,
                          const void* data, VkDeviceSize size);
    
    // Texture creation
    bool createTexture(VkImage& image, VkDeviceMemory& memory,
                      VkImageView& view, uint32_t width, uint32_t height,
                      VkFormat format, const void* data = nullptr);
    bool createDepthBuffer(VkImage& image, VkDeviceMemory& memory,
                          VkImageView& view, uint32_t width, uint32_t height);
    
    // Command buffer
    bool beginCommandBuffer(VkCommandBuffer cmd);
    bool endCommandBuffer(VkCommandBuffer cmd);
    bool submitCommandBuffer(VkCommandBuffer cmd, VkQueue queue);
    
    // Rendering
    void beginRenderPass(VkCommandBuffer cmd, VkFramebuffer framebuffer,
                        VkClearValue* clear_values, uint32_t clear_count);
    void endRenderPass(VkCommandBuffer cmd);
    void bindPipeline(VkCommandBuffer cmd, VkPipeline pipeline);
    void bindVertexBuffer(VkCommandBuffer cmd, VkBuffer buffer,
                         VkDeviceSize offset, uint32_t binding);
    void bindIndexBuffer(VkCommandBuffer cmd, VkBuffer buffer,
                        VkDeviceSize offset, VkIndexType type);
    void bindDescriptorSet(VkCommandBuffer cmd, VkPipelineLayout layout,
                          VkDescriptorSet set, uint32_t set_index);
    void draw(VkCommandBuffer cmd, uint32_t vertex_count, uint32_t instance_count,
             uint32_t first_vertex, uint32_t first_instance);
    void drawIndexed(VkCommandBuffer cmd, uint32_t index_count,
                    uint32_t instance_count, uint32_t first_index,
                    int32_t vertex_offset, uint32_t first_instance);
    void dispatch(VkCommandBuffer cmd, uint32_t group_count_x,
                 uint32_t group_count_y, uint32_t group_count_z);
    
    // State
    bool isReady() const { return initialized_; }
    uint32_t getWidth() const { return width_; }
    uint32_t getHeight() const { return height_; }
    
private:
    bool initialized_;
    uint32_t width_;
    uint32_t height_;
    
    VkInstance instance_;
    VkPhysicalDevice physical_device_;
    VkDevice device_;
    VkQueue queue_;
    VkSurfaceKHR surface_;
    VkSwapchainKHR swapchain_;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_views_;
    VkFormat swapchain_format_;
    VkExtent2D swapchain_extent_;
    
    VkRenderPass render_pass_;
    std::vector<VkFramebuffer> framebuffers_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    VkCommandPool command_pool_;
    VkCommandBuffer command_buffer_;
    VkFence fence_;
    
    VkPhysicalDeviceProperties device_properties_;
    VkPhysicalDeviceFeatures device_features_;
    VkPhysicalDeviceMemoryProperties memory_properties_;
    
    // Helper functions
    bool createInstance();
    bool createDevice();
    bool createSurface();
    bool createSwapchain();
    bool createRenderPass();
    bool createFramebuffers();
    bool createPipelineLayout();
    bool createDescriptorSetLayout();
    bool createCommandPool();
    bool createCommandBuffer();
    bool createFence();
    bool findQueueFamily();
    bool findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
    
    // Validation layers
    std::vector<const char*> validation_layers_{
        "VK_LAYER_KHRONOS_validation"
    };
    
    std::vector<const char*> device_extensions_{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};

} // namespace ps5