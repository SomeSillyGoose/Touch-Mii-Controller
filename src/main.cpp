#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "joybus.hpp"
#include "gcReport.hpp"
#include "touchpad.hpp"
#include <cstdio>

// GPIO pin assignments for buttons (active low, pulled up)
const int PIN_A = 2;
const int PIN_B = 3;
const int PIN_X = 4;
const int PIN_Y = 5;
const int PIN_START = 6;
const int PIN_DUP = 7;
const int PIN_DDOWN = 8;
const int PIN_DLEFT = 9;
const int PIN_DRIGHT = 10;
const int PIN_Z = 11;
const int PIN_R = 13;
const int PIN_L = 12;
const int PIN_R_TRIGGER = 15;
const int PIN_L_TRIGGER = 14;

// Global touchpad instance
Touchpad touchpad;

// Input state structure
struct InputState {
    bool a, b, x, y, start;
    bool dUp, dDown, dLeft, dRight;
    bool z, l, r;
    uint8_t analogL;
    uint8_t analogR;
    uint8_t cstick_x;
    uint8_t cstick_y;
};

static volatile InputState inputState = {
    false, false, false, false, false,
    false, false, false, false,
    false, false, false,
    0, 0,
    128, 128
};

static __time_critical_func(GCReport) buildGcReport() {
    GCReport report = defaultGcReport;

    // Button states
    report.a = inputState.a;
    report.b = inputState.b;
    report.x = inputState.x;
    report.y = inputState.y;
    report.start = inputState.start;

    report.dUp = inputState.dUp;
    report.dDown = inputState.dDown;
    report.dLeft = inputState.dLeft;
    report.dRight = inputState.dRight;

    report.z = inputState.z;
    report.l = inputState.l;
    report.r = inputState.r;

    report.analogL = inputState.analogL;
    report.analogR = inputState.analogR;

    // C-stick from touchpad
    report.cxStick = inputState.cstick_x;
    report.cyStick = inputState.cstick_y;

    return report;
}

void updateButtons() {
    // Read button states (active low)
    inputState.a = !gpio_get(PIN_A);
    inputState.b = !gpio_get(PIN_B);
    inputState.x = !gpio_get(PIN_X);
    inputState.y = !gpio_get(PIN_Y);
    inputState.start = !gpio_get(PIN_START);

    inputState.dUp = !gpio_get(PIN_DUP);
    inputState.dDown = !gpio_get(PIN_DDOWN);
    inputState.dLeft = !gpio_get(PIN_DLEFT);
    inputState.dRight = !gpio_get(PIN_DRIGHT);

    inputState.z = !gpio_get(PIN_Z);
    inputState.l = !gpio_get(PIN_L);
    inputState.r = !gpio_get(PIN_R);

    inputState.analogL = !gpio_get(PIN_L_TRIGGER) ? 255 : 0;
    inputState.analogR = !gpio_get(PIN_R_TRIGGER) ? 255 : 0;
}

void updateTouchpad() {
    TouchData touch = touchpad.get_touch_data();
    
    if (touch.is_touching) {
        inputState.cstick_x = touch.x;
        inputState.cstick_y = touch.y;
    } else {
        // Return to center when not touching
        inputState.cstick_x = 128;
        inputState.cstick_y = 128;
    }
}

void initButtons() {
    const int pins[] = {
        PIN_A, PIN_B, PIN_X, PIN_Y, PIN_START,
        PIN_DUP, PIN_DDOWN, PIN_DLEFT, PIN_DRIGHT,
        PIN_Z, PIN_L, PIN_R,
        PIN_L_TRIGGER, PIN_R_TRIGGER
    };

    for (int pin : pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }
}

void core1_entry() {
    const int dataPin = 22;
    enterMode(dataPin, buildGcReport);
}

int main() {
    stdio_init_all();

    // Initialize buttons
    initButtons();
    
    // Initialize touchpad
    if (!touchpad.init()) {
        printf("Touchpad initialization failed!\n");
        return 1;
    }
    printf("Touchpad initialized successfully\n");

    // Start joybus on core1
    multicore_launch_core1(core1_entry);

    // Main loop on core0: poll buttons and touchpad
    while (true) {
        updateButtons();
        updateTouchpad();
        
        sleep_us(1000);  // 1ms polling loop
    }

    return 0;
}
