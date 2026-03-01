#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace ps5 {

// DualSense HID reports
enum class DualSenseReportID : uint8_t {
    INPUT = 0x01,
    OUTPUT = 0x02,
    FEATURE = 0x03
};

// DualSense input report
struct DualSenseInputReport {
    uint8_t report_id;
    uint16_t timestamp;
    uint8_t battery;
    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t dpad : 4;
    uint8_t square : 1;
    uint8_t cross : 1;
    uint8_t circle : 1;
    uint8_t triangle : 1;
    uint8_t l1 : 1;
    uint8_t r1 : 1;
    uint8_t l2 : 1;
    uint8_t r2 : 1;
    uint8_t share : 1;
    uint8_t options : 1;
    uint8_t l3 : 1;
    uint8_t r3 : 1;
    uint8_t ps_button : 1;
    uint8_t touchpad_button : 1;
    uint8_t mic_button : 1;
    uint8_t l2_button : 1;
    uint8_t r2_button : 1;
    uint8_t left_stick_button : 1;
    uint8_t right_stick_button : 1;
    uint8_t trackpad_pressed : 1;
    uint8_t trackpad_touch1 : 1;
    uint8_t trackpad_touch2 : 1;
    uint8_t trackpad_touch3 : 1;
    uint8_t trackpad_x1;
    uint8_t trackpad_y1;
    uint8_t trackpad_x2;
    uint8_t trackpad_y2;
    uint8_t accel_x;
    uint8_t accel_y;
    uint8_t accel_z;
    uint8_t gyro_x;
    uint8_t gyro_y;
    uint8_t gyro_z;
    uint8_t power_mode : 4;
    uint8_t usb_connected : 1;
    uint8_t headphones_connected : 1;
    uint8_t microphone_connected : 1;
    uint8_t reserved : 1;
    uint8_t mute_button : 1;
    uint8_t reserved2 : 7;
};

// DualSense output report
struct DualSenseOutputReport {
    uint8_t report_id;
    uint8_t enable_flags1;
    uint8_t enable_flags2;
    uint8_t enable_flags3;
    uint8_t enable_flags4;
    uint8_t enable_flags5;
    uint8_t reserved1;
    uint8_t motor_left;
    uint8_t motor_right;
    uint8_t audio_volume;
    uint8_t mute_button_led;
    uint8_t power_save_control;
    uint8_t reserved2[6];
    uint8_t mic_led;
    uint8_t reserved3[2];
    uint8_t player_leds;
    uint8_t reserved4[3];
    uint8_t led_brightness;
    uint8_t reserved5;
    uint8_t audio_control;
    uint8_t reserved6[2];
    uint8_t haptics_strength;
    uint8_t reserved7[2];
    uint8_t trigger_left_mode;
    uint8_t trigger_left_param1;
    uint8_t trigger_left_param2;
    uint8_t trigger_left_param3;
    uint8_t trigger_left_param4;
    uint8_t trigger_left_param5;
    uint8_t trigger_left_param6;
    uint8_t trigger_left_param7;
    uint8_t trigger_left_param8;
    uint8_t trigger_left_param9;
    uint8_t trigger_left_param10;
    uint8_t trigger_left_param11;
    uint8_t trigger_left_param12;
    uint8_t trigger_left_param13;
    uint8_t trigger_left_param14;
    uint8_t trigger_left_param15;
    uint8_t trigger_left_param16;
    uint8_t trigger_left_param17;
    uint8_t trigger_left_param18;
    uint8_t trigger_left_param19;
    uint8_t trigger_left_param20;
    uint8_t trigger_left_param21;
    uint8_t trigger_left_param22;
    uint8_t trigger_left_param23;
    uint8_t trigger_left_param24;
    uint8_t trigger_left_param25;
    uint8_t trigger_left_param26;
    uint8_t trigger_left_param27;
    uint8_t trigger_left_param28;
    uint8_t trigger_left_param29;
    uint8_t trigger_left_param30;
    uint8_t trigger_left_param31;
    uint8_t trigger_left_param32;
    uint8_t trigger_right_mode;
    uint8_t trigger_right_param1;
    uint8_t trigger_right_param2;
    uint8_t trigger_right_param3;
    uint8_t trigger_right_param4;
    uint8_t trigger_right_param5;
    uint8_t trigger_right_param6;
    uint8_t trigger_right_param7;
    uint8_t trigger_right_param8;
    uint8_t trigger_right_param9;
    uint8_t trigger_right_param10;
    uint8_t trigger_right_param11;
    uint8_t trigger_right_param12;
    uint8_t trigger_right_param13;
    uint8_t trigger_right_param14;
    uint8_t trigger_right_param15;
    uint8_t trigger_right_param16;
    uint8_t trigger_right_param17;
    uint8_t trigger_right_param18;
    uint8_t trigger_right_param19;
    uint8_t trigger_right_param20;
    uint8_t trigger_right_param21;
    uint8_t trigger_right_param22;
    uint8_t trigger_right_param23;
    uint8_t trigger_right_param24;
    uint8_t trigger_right_param25;
    uint8_t trigger_right_param26;
    uint8_t trigger_right_param27;
    uint8_t trigger_right_param28;
    uint8_t trigger_right_param29;
    uint8_t trigger_right_param30;
    uint8_t trigger_right_param31;
    uint8_t trigger_right_param32;
    uint8_t reserved8[13];
};

// DualSense state
struct DualSenseState {
    bool connected;
    bool charging;
    bool headphones_connected;
    bool microphone_connected;
    uint8_t battery_level;
    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;
    bool square;
    bool cross;
    bool circle;
    bool triangle;
    bool l1;
    bool r1;
    bool l2;
    bool r2;
    bool share;
    bool options;
    bool l3;
    bool r3;
    bool ps_button;
    bool touchpad_button;
    bool mic_button;
    bool l2_button;
    bool r2_button;
    bool left_stick_button;
    bool right_stick_button;
    bool trackpad_pressed;
    uint16_t trackpad_x;
    uint16_t trackpad_y;
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    uint8_t motor_left;
    uint8_t motor_right;
    uint8_t audio_volume;
    uint8_t mute_button_led;
    uint8_t player_leds;
    uint8_t led_brightness;
    uint8_t trigger_left_mode;
    uint8_t trigger_right_mode;
};

// DualSense controller
class DualSense {
public:
    DualSense();
    ~DualSense();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Connection
    void connect();
    void disconnect();
    bool isConnected() const { return state_.connected; }
    
    // Input handling
    void setInputCallback(std::function<void(DualSenseInputReport&)> callback);
    void processInput(const DualSenseInputReport& report);
    
    // Output handling
    void processOutput(const DualSenseOutputReport& report);
    void setMotorLeft(uint8_t value);
    void setMotorRight(uint8_t value);
    void setAudioVolume(uint8_t value);
    void setMuteButtonLED(uint8_t value);
    void setPlayerLEDs(uint8_t value);
    void setLEDBrightness(uint8_t value);
    void setTriggerLeftMode(uint8_t mode);
    void setTriggerRightMode(uint8_t mode);
    
    // State
    const DualSenseState& getState() const { return state_; }
    
private:
    bool initialized_;
    DualSenseState state_;
    
    std::function<void(DualSenseInputReport&)> input_callback_;
    
    std::mutex mutex_;
    
    // Helper functions
    void updateMotor();
    void updateLED();
    void updateAudio();
};

} // namespace ps5