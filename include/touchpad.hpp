#ifndef __TOUCHPAD_HPP
#define __TOUCHPAD_HPP

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <stddef.h>

// I2C Configuration
#define TOUCHPAD_I2C_PORT i2c0
#define TOUCHPAD_SDA_PIN 0
#define TOUCHPAD_SCL_PIN 1
#define TOUCHPAD_I2C_ADDR 0x56
#define TOUCHPAD_I2C_SPEED 100000

// Control Pins
#define TOUCHPAD_RDY_PIN 17
#define TOUCHPAD_RST_PIN 18

// IQS7211A Register Definitions
#define TOUCHPAD_REG_INFO_FLAGS 0x10
#define TOUCHPAD_REG_GESTURES 0x11
#define TOUCHPAD_REG_FINGER1_X 0x14
#define TOUCHPAD_REG_FINGER1_Y 0x15
#define TOUCHPAD_REG_FINGER1_TOUCH_STR 0x16
#define TOUCHPAD_REG_FINGER1_AREA 0x17
#define TOUCHPAD_REG_SYSTEM_CONTROL 0x50

#define TOUCHPAD_BOOT_COMPLETE_MASK 0x0080

// Touch data structure
struct TouchData {
    uint16_t x;
    uint16_t y;
    uint8_t strength;
    uint8_t area;
    bool is_touching;
};

class Touchpad {
private:
    bool event_mode_enabled;
    bool has_cached_touch;
    TouchData cached_touch;
    // Fixed-point smoothed coordinates (value × 256) for EMA filter
    uint32_t smooth_x;
    uint32_t smooth_y;
    // I2C helper functions
    bool write_register(uint8_t reg, uint16_t value);
    bool read_register(uint8_t reg, uint16_t &value);
    bool read_registers_burst(uint8_t start_reg, uint8_t *buf, size_t len);
    bool wait_for_comm_window(uint32_t timeout_ms);
    bool reset_and_wait_for_boot();
    bool apply_default_config();

public:
    Touchpad();
    
    // Initialize the touchpad
    bool init();
    
    // Get touch coordinates scaled to 0-255 range
    TouchData get_touch_data();
};

#endif
