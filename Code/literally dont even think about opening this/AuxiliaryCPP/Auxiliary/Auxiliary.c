#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

int main() {
    const uint pwm_freq = 1500000; // 100 kHz
    const float duty_cycle = 0.5f;

    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t wrap = clock_freq / pwm_freq;
    uint32_t level = wrap * duty_cycle;

    // === GPIO 0 → PWM output ===
    gpio_set_function(0, GPIO_FUNC_PWM);
    uint slice0 = pwm_gpio_to_slice_num(0);
    pwm_set_wrap(slice0, wrap);
    pwm_set_chan_level(slice0, PWM_CHAN_A, level);
    pwm_set_enabled(slice0, true);

    // === GPIO 1 → Input pin ===
    gpio_init(1);
    gpio_set_dir(1, GPIO_IN);
    gpio_pull_down(1); // optional, adds default low

    // === GPIO 2 → Output pin (manual control) ===
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_put(2, 1); // start high

    while (1) {
        if (gpio_get(1)) {
            // GPIO 1 is high → turn off GPIO 2
            gpio_put(2, 0);
        }
        // (Optional) else keep GPIO 2 high:
         else gpio_put(2, 1);
    }
}
