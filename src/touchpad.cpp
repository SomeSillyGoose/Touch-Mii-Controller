#include "touchpad.hpp"

#include <cstdio>

namespace {
struct TouchpadRegInit {
    uint8_t reg;
    uint16_t value;
};

constexpr TouchpadRegInit kDefaultConfig[] = {
    {0x30, 0x3BE1}, {0x31, 0x000F}, {0x32, 0x00FA}, {0x33, 0x0032},
    {0x34, 0x0032}, {0x35, 0x0005}, {0x36, 0x2882}, {0x37, 0x0005},
    {0x38, 0x00FA}, {0x39, 0x0014}, {0x3A, 0x01C4}, {0x3B, 0x01E8},
    {0x40, 0x000A}, {0x41, 0x0032}, {0x42, 0x0032}, {0x43, 0x0032},
    {0x44, 0x0064}, {0x45, 0x000A}, {0x46, 0x003C}, {0x47, 0x0014},
    {0x48, 0x000A}, {0x49, 0x0008}, {0x4A, 0x0064}, {0x50, 0x0000},
    {0x51, 0x063C}, {0x52, 0xFF20}, {0x53, 0x141A}, {0x54, 0x0008},
    {0x55, 0xFFFF}, {0x56, 0x0402}, {0x57, 0xFFFF}, {0x58, 0x7F05},
    {0x59, 0x7F05}, {0x5A, 0x0D01}, {0x5B, 0x1D65}, {0x60, 0x0428},
    {0x61, 0x0105}, {0x62, 0x0300}, {0x63, 0x0400}, {0x64, 0x0006},
    {0x65, 0x007C}, {0x66, 0x8007}, {0x67, 0x0014}, {0x68, 0x0014},
    {0x69, 0x0014}, {0x70, 0x00B4}, {0x71, 0x0406}, {0x72, 0x0333},
    {0x73, 0x0540}, {0x74, 0x0100}, {0x80, 0x0F3F}, {0x81, 0x0096},
    {0x82, 0x0032}, {0x83, 0x012C}, {0x84, 0x0096}, {0x85, 0x00C8},
    {0x86, 0x00C8}, {0x87, 0x0017}, {0x90, 0x0405}, {0x91, 0x0001},
    {0x92, 0x090A}, {0x93, 0x0708}, {0x94, 0x0A06}, {0x95, 0x0809},
    {0x96, 0x0000}, {0xA0, 0x0005}, {0xA1, 0x0502}, {0xA2, 0x0301},
    {0xA3, 0x0405}, {0xA4, 0x0506}, {0xA5, 0x0705}, {0xA6, 0x0805},
    {0xA7, 0x050A}, {0xA8, 0x0B09}, {0xA9, 0x0C05}, {0xAA, 0x050E},
    {0xAB, 0x0F0D}, {0xAC, 0x1005}, {0xAD, 0x0512}, {0xAE, 0x1311},
    {0xB0, 0xFF05}, {0xB1, 0x05FF}, {0xB2, 0xFFFF}, {0xB3, 0xFF05},
    {0xB4, 0x05FF}, {0xB5, 0xFFFF}, {0xB6, 0xFF05}, {0xB7, 0x05FF},
    {0xB8, 0xFFFF}, {0xB9, 0xFF05}, {0xBA, 0x05FF}, {0xBB, 0xFFFF},
};

constexpr uint16_t kXResolution = 0x0300;
constexpr uint16_t kYResolution = 0x0400;
}  // namespace

Touchpad::Touchpad()
    : event_mode_enabled(false), has_cached_touch(false), cached_touch{128, 128, 0, 0, false},
      smooth_x(128u << 8), smooth_y(128u << 8) {
}

bool Touchpad::wait_for_comm_window(uint32_t timeout_ms) {
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);

    while (gpio_get(TOUCHPAD_RDY_PIN) != 0) {
        if (time_reached(deadline)) {
            return false;
        }
        sleep_ms(1);
    }

    sleep_ms(1);
    return true;
}

bool Touchpad::write_register(uint8_t reg, uint16_t value) {
    uint8_t buffer[3] = {
        reg,
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
    };

    if (event_mode_enabled && !wait_for_comm_window(250)) {
        return false;
    }

    int written = i2c_write_blocking(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_ADDR, buffer, 3, false);
    if (written != 3) {
        return false;
    }

    sleep_ms(20);
    return true;
}

bool Touchpad::read_register(uint8_t reg, uint16_t &value) {
    uint8_t buffer[2] = {0, 0};

    if (event_mode_enabled && !wait_for_comm_window(250)) {
        return false;
    }

    int written = i2c_write_blocking(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_ADDR, &reg, 1, true);
    if (written != 1) {
        return false;
    }

    int read = i2c_read_blocking(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_ADDR, buffer, 2, false);
    if (read != 2) {
        return false;
    }

    value = static_cast<uint16_t>(buffer[0]) | (static_cast<uint16_t>(buffer[1]) << 8);
    sleep_ms(20);
    return true;
}

bool Touchpad::read_registers_burst(uint8_t start_reg, uint8_t *buf, size_t len) {
    if (event_mode_enabled && !wait_for_comm_window(250)) {
        return false;
    }

    int written = i2c_write_blocking(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_ADDR, &start_reg, 1, true);
    if (written != 1) {
        return false;
    }

    int read = i2c_read_blocking(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_ADDR, buf, len, false);
    if (read != static_cast<int>(len)) {
        return false;
    }

    sleep_ms(40);
    return true;
}

bool Touchpad::reset_and_wait_for_boot() {
    uint16_t info_flags = 0;

    if (!write_register(TOUCHPAD_REG_GESTURES, 0x0000)) {
        return false;
    }

    if (!write_register(TOUCHPAD_REG_SYSTEM_CONTROL, 0x0200)) {
        return false;
    }

    for (uint32_t attempt = 0; attempt < 500; ++attempt) {
        if (!read_register(TOUCHPAD_REG_INFO_FLAGS, info_flags)) {
            return false;
        }

        if ((info_flags & TOUCHPAD_BOOT_COMPLETE_MASK) != 0u) {
            event_mode_enabled = true;
            return true;
        }

        sleep_ms(10);
    }

    return false;
}

bool Touchpad::apply_default_config() {
    for (const TouchpadRegInit &entry : kDefaultConfig) {
        if (!write_register(entry.reg, entry.value)) {
            return false;
        }
    }

    sleep_ms(3000);
    return true;
}

bool Touchpad::init() {
    i2c_init(TOUCHPAD_I2C_PORT, TOUCHPAD_I2C_SPEED);
    gpio_set_function(TOUCHPAD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(TOUCHPAD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(TOUCHPAD_SDA_PIN);
    gpio_pull_up(TOUCHPAD_SCL_PIN);

    gpio_init(TOUCHPAD_RDY_PIN);
    gpio_set_dir(TOUCHPAD_RDY_PIN, GPIO_IN);
    gpio_pull_up(TOUCHPAD_RDY_PIN);

    gpio_init(TOUCHPAD_RST_PIN);
    gpio_set_dir(TOUCHPAD_RST_PIN, GPIO_OUT);
    gpio_put(TOUCHPAD_RST_PIN, 1);

    sleep_ms(100);

    gpio_put(TOUCHPAD_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(TOUCHPAD_RST_PIN, 1);
    sleep_ms(100);

    event_mode_enabled = false;
    has_cached_touch = false;
    cached_touch = {128, 128, 0, 0, false};
    smooth_x = 128u << 8;
    smooth_y = 128u << 8;

    if (!reset_and_wait_for_boot()) {
        return false;
    }

    if (!apply_default_config()) {
        return false;
    }

    return true;
}

TouchData Touchpad::get_touch_data() {
    TouchData result = {0, 0, 0, 0, false};

    if (gpio_get(TOUCHPAD_RDY_PIN) != 0) {
        return has_cached_touch ? cached_touch : result;
    }

    uint16_t info_flags = 0;
    if (!read_register(TOUCHPAD_REG_INFO_FLAGS, info_flags)) {
        return result;
    }

    uint8_t number_of_touches = static_cast<uint8_t>((info_flags >> 8) & 0x03u);
    if (number_of_touches == 0) {
        has_cached_touch = false;
        cached_touch = {128, 128, 0, 0, false};
        return result;
    }

    // Read all 4 coordinate registers in one burst (x, y, strength, area = 8 bytes)
    uint8_t coord_buf[8] = {0};
    if (!read_registers_burst(TOUCHPAD_REG_FINGER1_X, coord_buf, 8)) {
        return has_cached_touch ? cached_touch : result;
    }

    uint16_t x        = static_cast<uint16_t>(coord_buf[0]) | (static_cast<uint16_t>(coord_buf[1]) << 8);
    uint16_t y        = static_cast<uint16_t>(coord_buf[2]) | (static_cast<uint16_t>(coord_buf[3]) << 8);
    uint16_t strength = static_cast<uint16_t>(coord_buf[4]) | (static_cast<uint16_t>(coord_buf[5]) << 8);
    uint16_t area     = static_cast<uint16_t>(coord_buf[6]) | (static_cast<uint16_t>(coord_buf[7]) << 8);

    if (x > kXResolution) {
        x = kXResolution;
    }
    if (y > kYResolution) {
        y = kYResolution;
    }

    // Scale raw coordinates to 0-255 range with center at 128 and invert axes to match expected orientation
    uint16_t scaled_x = static_cast<uint16_t>((0u - ((static_cast<uint32_t>(x) * 255u) / kXResolution - 128u)) + 127u);
    uint16_t scaled_y = static_cast<uint16_t>((0u - ((static_cast<uint32_t>(y) * 255u) / kYResolution - 128u)) + 127u);

    // Exponential moving average: smooth = 0.75*smooth + 0.25*new (fixed-point ×256)
    // Alpha = 64/256 = 0.25. Increase alpha for more responsiveness, decrease for smoother.
    constexpr uint32_t kAlpha = 128u;
    smooth_x = (smooth_x * (256u - kAlpha) + (static_cast<uint32_t>(scaled_x) << 8) * kAlpha) >> 8;
    smooth_y = (smooth_y * (256u - kAlpha) + (static_cast<uint32_t>(scaled_y) << 8) * kAlpha) >> 8;

    result.x = static_cast<uint16_t>(smooth_y >> 8);
    result.y = static_cast<uint16_t>(smooth_x >> 8);
    result.strength = static_cast<uint8_t>(strength & 0xFFu);
    result.area = static_cast<uint8_t>(area & 0xFFu);
    result.is_touching = true;
    cached_touch = result;
    has_cached_touch = true;
    return result;
}