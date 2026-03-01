#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace ps5 {

// Shader compiler
class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();
    
    // Compilation
    bool initialize();
    void shutdown();
    
    // HLSL to SPIR-V
    bool compileHLSL(const std::string& source, const std::string& entry_point,
                    VkShaderStageFlagBits stage, std::vector<uint32_t>& spirv);
    bool compileHLSLFile(const std::string& path, const std::string& entry_point,
                        VkShaderStageFlagBits stage, std::vector<uint32_t>& spirv);
    
    // GLSL to SPIR-V
    bool compileGLSL(const std::string& source, VkShaderStageFlagBits stage,
                    std::vector<uint32_t>& spirv);
    bool compileGLSLFile(const std::string& path, VkShaderStageFlagBits stage,
                        std::vector<uint32_t>& spirv);
    
    // Validation
    bool validateSPIRV(const std::vector<uint32_t>& spirv);
    
    // Optimization
    bool optimizeSPIRV(std::vector<uint32_t>& spirv);
    
private:
    bool initialized_;
    
    // Helper functions
    std::string preprocessHLSL(const std::string& source);
    std::vector<uint32_t> compileToSPIRV(const std::string& source,
                                        VkShaderStageFlagBits stage);
    bool linkSPIRV(std::vector<uint32_t>& spirv);
};

} // namespace ps5