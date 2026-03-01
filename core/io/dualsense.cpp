#include "core/io/dualsense.h"
#include <cstring>
#include <iostream>

namespace ps5 {

DualSense::DualSense() : initialized_(false) {
    std::memset(&state_, 0, sizeof(state_));
    state_.connected = false;
}

DualSense::~DualSense() {
    shutdown();
}

bool DualSense::initialize() {
    if (initialized_) {
        return true;
    }
    
    std::memset(&state_, 0, sizeof(state_));
    state_.connected = false;
    state_.charging = false;
    state_.headphones_connected = false;
    state_.microphone_connected = false;
    state_.battery_level = 100;
    state_.left_stick_x = 128;
    state_.left_stick_y = 128;
    state_.right_stick_x = 128;
    state_.right_stick_y = 128;
    state_.dpad_up = false;
    state_.dpad_down = false;
    state_.dpad_left = false;
    state_.dpad_right = false;
    state_.square = false;
    state_.cross = false;
    state_.circle = false;
    state_.triangle = false;
    state_.l1 = false;
    state_.r1 = false;
    state_.l2 = false;
    state_.r2 = false;
    state_.share = false;
    state_.options = false;
    state_.l3 = false;
    state_.r3 = false;
    state_.ps_button = false;
    state_.touchpad_button = false;
    state_.mic_button = false;
    state_.l2_button = false;
    state_.r2_button = false;
    state_.left_stick_button = false;
    state_.right_stick_button = false;
    state_.trackpad_pressed = false;
    state_.trackpad_x = 0;
    state_.trackpad_y = 0;
    state_.accel_x = 0.0f;
    state_.accel_y = 0.0f;
    state_.accel_z = 0.0f;
    state_.gyro_x = 0.0f;
    state_.gyro_y = 0.0f;
    state_.gyro_z = 0.0f;
    state_.motor_left = 0;
    state_.motor_right = 0;
    state_.audio_volume = 50;
    state_.mute_button_led = 0;
    state_.player_leds = 0;
    state_.led_brightness = 100;
    state_.trigger_left_mode = 0;
    state_.trigger_right_mode = 0;
    
    initialized_ = true;
    
    return true;
}

void DualSense::shutdown() {
    if (!initialized_) {
        return;
    }
    
    disconnect();
    initialized_ = false;
}

void DualSense::connect() {
    if (!initialized_) {
        return;
    }
    
    state_.connected = true;
}

void DualSense::disconnect() {
    if (!initialized_) {
        return;
    }
    
    state_.connected = false;
    state_.charging = false;
    state_.headphones_connected = false;
    state_.microphone_connected = false;
    state_.battery_level = 0;
}

void DualSense::setInputCallback(std::function<void(DualSenseInputReport&)> callback) {
    input_callback_ = callback;
}

void DualSense::processInput(const DualSenseInputReport& report) {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update state from input report
    state_.charging = (report.power_mode & 0x08) != 0;
    state_.headphones_connected = report.headphones_connected != 0;
    state_.microphone_connected = report.microphone_connected != 0;
    state_.battery_level = report.battery;
    state_.left_stick_x = report.left_stick_x;
    state_.left_stick_y = report.left_stick_y;
    state_.right_stick_x = report.right_stick_x;
    state_.right_stick_y = report.right_stick_y;
    state_.dpad_up = (report.dpad == 0) || (report.dpad == 1) || (report.dpad == 7);
    state_.dpad_down = (report.dpad == 3) || (report.dpad == 4) || (report.dpad == 5);
    state_.dpad_left = (report.dpad == 5) || (report.dpad == 6) || (report.dpad == 7);
    state_.dpad_right = (report.dpad == 1) || (report.dpad == 2) || (report.dpad == 3);
    state_.square = report.square != 0;
    state_.cross = report.cross != 0;
    state_.circle = report.circle != 0;
    state_.triangle = report.triangle != 0;
    state_.l1 = report.l1 != 0;
    state_.r1 = report.r1 != 0;
    state_.l2 = report.l2 != 0;
    state_.r2 = report.r2 != 0;
    state_.share = report.share != 0;
    state_.options = report.options != 0;
    state_.l3 = report.l3 != 0;
    state_.r3 = report.r3 != 0;
    state_.ps_button = report.ps_button != 0;
    state_.touchpad_button = report.touchpad_button != 0;
    state_.mic_button = report.mic_button != 0;
    state_.l2_button = report.l2_button != 0;
    state_.r2_button = report.r2_button != 0;
    state_.left_stick_button = report.left_stick_button != 0;
    state_.right_stick_button = report.right_stick_button != 0;
    state_.trackpad_pressed = report.trackpad_pressed != 0;
    state_.trackpad_x = report.trackpad_x1 | (static_cast<uint16_t>(report.trackpad_x2 & 0x0F) << 8);
    state_.trackpad_y = report.trackpad_y1 | (static_cast<uint16_t>(report.trackpad_y2 & 0x0F) << 8);
    state_.accel_x = static_cast<float>(report.accel_x - 128) / 128.0f;
    state_.accel_y = static_cast<float>(report.accel_y - 128) / 128.0f;
    state_.accel_z = static_cast<float>(report.accel_z - 128) / 128.0f;
    state_.gyro_x = static_cast<float>(report.gyro_x - 128) / 128.0f;
    state_.gyro_y = static_cast<float>(report.gyro_y - 128) / 128.0f;
    state_.gyro_z = static_cast<float>(report.gyro_z - 128) / 128.0f;
    
    // Call input callback
    if (input_callback_) {
        DualSenseInputReport modified_report = report;
        input_callback_(modified_report);
    }
}

void DualSense::processOutput(const DualSenseOutputReport& report) {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update state from output report
    if (report.enable_flags1 & 0x01) {
        state_.motor_left = report.motor_left;
    }
    if (report.enable_flags1 & 0x02) {
        state_.motor_right = report.motor_right;
    }
    if (report.enable_flags2 & 0x01) {
        state_.audio_volume = report.audio_volume;
    }
    if (report.enable_flags2 & 0x02) {
        state_.mute_button_led = report.mute_button_led;
    }
    if (report.enable_flags3 & 0x01) {
        state_.player_leds = report.player_leds;
    }
    if (report.enable_flags3 & 0x02) {
        state_.led_brightness = report.led_brightness;
    }
    if (report.enable_flags4 & 0x01) {
        state_.trigger_left_mode = report.trigger_left_mode;
    }
    if (report.enable_flags4 & 0x02) {
        state_.trigger_right_mode = report.trigger_right_mode;
    }
    
    // Update hardware
    updateMotor();
    updateLED();
    updateAudio();
}

void DualSense::setMotorLeft(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.motor_left = value;
    updateMotor();
}

void DualSense::setMotorRight(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.motor_right = value;
    updateMotor();
}

void DualSense::setAudioVolume(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.audio_volume = value;
    updateAudio();
}

void DualSense::setMuteButtonLED(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.mute_button_led = value;
    updateLED();
}

void DualSense::setPlayerLEDs(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.player_leds = value;
    updateLED();
}

void DualSense::setLEDBrightness(uint8_t value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.led_brightness = value;
    updateLED();
}

void DualSense::setTriggerLeftMode(uint8_t mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.trigger_left_mode = mode;
}

void DualSense::setTriggerRightMode(uint8_t mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    state_.trigger_right_mode = mode;
}

void DualSense::updateMotor() {
    // Update motor hardware
    // In real implementation, this would send commands to the controller
}

void DualSense::updateLED() {
    // Update LED hardware
    // In real implementation, this would send commands to the controller
}

void DualSense::updateAudio() {
    // Update audio hardware
    // In real implementation, this would send commands to the controller
}

} // namespace ps5