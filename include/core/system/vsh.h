#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

namespace ps5 {

// VSH (Visual Shell) - PS5's system UI
class VSH {
public:
    VSH();
    ~VSH();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Boot sequence
    void boot();
    void startServices();
    
    // Service management
    bool registerService(const std::string& name, uint64_t service_id);
    bool unregisterService(const std::string& name);
    uint64_t getServiceId(const std::string& name);
    
    // Application management
    uint64_t launchApplication(const std::string& pkg_path);
    KernelResult terminateApplication(uint64_t app_id);
    void suspendApplication(uint64_t app_id);
    void resumeApplication(uint64_t app_id);
    
    // System services
    void updateSystemTime();
    void handlePowerButton();
    void handleHomeButton();
    void handleTouchpadInput(uint32_t x, uint32_t y, bool pressed);
    
    // UI rendering
    void render();
    void present();
    
    // Configuration
    void setLanguage(const std::string& lang);
    void setRegion(const std::string& region);
    void setTimeZone(const std::string& tz);
    
    // State
    enum class State {
        BOOTING,
        READY,
        RUNNING,
        SUSPENDED,
        SHUTDOWN
    };
    
    State getState() const { return state_; }
    bool isBooted() const { return state_ == State::RUNNING || state_ == State::READY; }
    
private:
    State state_;
    bool initialized_;
    
    std::unordered_map<std::string, uint64_t> services_;
    std::unordered_map<uint64_t, std::string> service_names_;
    
    std::unordered_map<uint64_t, std::string> applications_;
    uint64_t next_app_id_{0};
    
    uint64_t system_time_{0};
    std::string language_{"en"};
    std::string region_{"US"};
    std::string timezone_{"UTC"};
    
    std::mutex mutex_;
    
    // Service handlers
    void handleGetSystemInfo(uint64_t arg0, uint64_t arg1);
    void handleGetFirmwareVersion(uint64_t arg0, uint64_t arg1);
    void handleGetHardwareInfo(uint64_t arg0, uint64_t arg1);
    void handleGetSecurityInfo(uint64_t arg0, uint64_t arg1);
};

} // namespace ps5