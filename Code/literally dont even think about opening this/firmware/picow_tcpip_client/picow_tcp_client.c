/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "hardware/clocks.h"


#if !defined(TEST_TCP_SERVER_IP)
#error TEST_TCP_SERVER_IP not defined
#endif

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 2048

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5

#define PICO_CYW43_ARCH_POLL 1

#if 0
static void dump_bytes(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0;

    printf("dump_bytes %d", len);
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            printf("\n");
        } else if ((i & 0x07) == 0) {
            printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}
#define DUMP_BYTES dump_bytes
#else
#define DUMP_BYTES(A,B)
#endif


//PINS
#include "hardware/pwm.h"
uint PIN = 22;          // GPIO pin
uint32_t FREQ_HZ = 1000; // Frequency
float DUTY = 0.5f;      // Duty cycle (0.0-1.0)
int pin, freq, duty;

uint freq_khz = 150000;


//METHODS

uint32_t pwm_set_freq_duty(uint slice_num,
       uint chan,uint32_t f, int d)
{
 uint32_t clock = 150000000;
 uint32_t divider16 = clock / f / 4096 + 
                           (clock % (f * 4096) != 0);
 if (divider16 / 16 == 0)
 divider16 = 16;
 uint32_t wrap = clock * 16 / divider16 / f - 1;
 pwm_set_clkdiv_int_frac(slice_num, divider16/16,
                                     divider16 & 0xF);
 pwm_set_wrap(slice_num, wrap);
 pwm_set_chan_level(slice_num, chan, wrap * d / 100);
 return wrap;
}




typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    int buffer_len;
    int sent_len;
    bool complete;
    int run_count;
    bool connected;
} TCP_CLIENT_T;

static err_t tcp_client_close(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    err_t err = ERR_OK;
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        state->tcp_pcb = NULL;
        free(state);
    }
    return err;
}

// Called with results of operation
static err_t tcp_result(void *arg, int status) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
            // struct tcp_pcb *tcp_pcb;
            // ip_addr_t remote_addr;
            // uint8_t buffer[BUF_SIZE];
            // int buffer_len;
            // int sent_len;
            // bool complete;
            // int run_count;
            // bool connected;
            // DEBUG_printf("");
    }
    state->complete = true;
    return tcp_client_close(arg);
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("tcp_client_sent %u\n", len);
    state->sent_len += len;

    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    state->connected = true;
    DEBUG_printf("Waiting for buffer from server\n");
    return ERR_OK;
}

static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    DEBUG_printf("tcp_client_poll\n");
    return tcp_result(arg, -1); // no response is an error?
}

static void tcp_client_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    while (!p) {
        return tcp_result(arg, -1);
    }

    cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        uint16_t copied = pbuf_copy_partial(p, state->buffer + state->buffer_len,
                                            p->tot_len, 0);

        // Check newly received bytes for end byte
        for (int i = state->buffer_len; i < state->buffer_len + copied; i++) {
            if (state->buffer[i] == 0x04) {  // End byte
                state->buffer[i] = '\0'; // terminate string

                // Disconnect command
                if (strcmp((char *)state->buffer, "DISCONNECT") == 0) {
                    state->complete = true;
                    gpio_put(16, 0);
                    gpio_put(15, 0);
                    gpio_put(15, 0);
                    char msg[64];
                    int n = snprintf(msg, sizeof(msg), "EN 0 OK");
                    msg[n++] = 0x04;
                    tcp_write(tpcb, msg, n, TCP_WRITE_FLAG_COPY);
                    tcp_output(tpcb);
                    cyw43_arch_deinit();
                    return 0;
                }

                // Enable PWM
                if (strcmp((char *)state->buffer, "EN 1") == 0) {
                    pwm_set_enabled(pwm_gpio_to_slice_num(16), true);
                    pwm_set_enabled(pwm_gpio_to_slice_num(14), true);
                    gpio_set_function(16, GPIO_FUNC_PWM);
                    gpio_set_function(15, GPIO_FUNC_PWM);
                    gpio_set_function(14, GPIO_FUNC_PWM);
                    char msg[64];
                    int n = snprintf(msg, sizeof(msg), "EN 1 OK");
                    msg[n++] = 0x04;
                    tcp_write(tpcb, msg, n, TCP_WRITE_FLAG_COPY);
                    tcp_output(tpcb);
                }

                // Disable PWM
                if (strcmp((char *)state->buffer, "EN 0") == 0) {
                    pwm_set_enabled(pwm_gpio_to_slice_num(16), false);
                    pwm_set_enabled(pwm_gpio_to_slice_num(14), false);
                    gpio_set_function(16, GPIO_OUT);
                    gpio_set_function(15, GPIO_OUT);
                    gpio_set_function(14, GPIO_OUT);
                    gpio_put(16, 0);
                    gpio_put(15, 0);
                    gpio_put(14, 0);
                    char msg[64];
                    int n = snprintf(msg, sizeof(msg), "EN 0 OK");
                    msg[n++] = 0x04;
                    tcp_write(tpcb, msg, n, TCP_WRITE_FLAG_COPY);
                    tcp_output(tpcb);
                }

                // Set PIN frequency & duty
                if (sscanf((char *)state->buffer, "PINFREQDUTY %d %d %d", &pin, &freq, &duty) == 3) {
                    uint slice_num = pwm_gpio_to_slice_num(pin);
                    uint chan      = pwm_gpio_to_channel(pin);
                    pwm_set_freq_duty(slice_num, chan, freq, duty);
                    char msg[128];  // Make sure it's large enough for your string
                    int n = snprintf(msg, sizeof(msg), "Pin: %d F: %d D: %d OK.\n", pin, freq, duty);
                    msg[n++] = 0x04;
                    tcp_write(tpcb, msg, n, TCP_WRITE_FLAG_COPY);
                    tcp_output(tpcb);
                }

                // Set system clock frequency
            if (sscanf((char *)state->buffer, "CLK? %d", &freq_khz) == 1) {
                uint vco_freq, post_div1, post_div2;
                if (check_sys_clock_khz(freq_khz, &vco_freq, &post_div1, &post_div2)) {
                    set_sys_clock_pll(vco_freq, post_div1, post_div2);
                    printf("System clock set via PLL\n");
                    set_sys_clock_khz (freq_khz,true);
                    char msg[128];
                    int len = snprintf(msg, sizeof(msg),
                        "Frequency valid! VCO: %u kHz, PostDiv1: %u, PostDiv2: %u",
                        vco_freq, post_div1, post_div2);
                    msg[len++] = 0x04;
                    tcp_write(tpcb, msg, len, TCP_WRITE_FLAG_COPY);
                } else {
                    char msg[] = "Frequency not achievable.";
                    int len = sizeof(msg)-1; // exclude null terminator
                    msg[len++] = 0x04;       // append end byte
                    tcp_write(tpcb, msg, len, TCP_WRITE_FLAG_COPY);
                    tcp_output(tpcb);
                }
            }


                // Reset buffer
                state->buffer_len = 0;
                state->buffer[state->buffer_len] = '\0';
                state->complete = true;
                break;
            }
        }

        state->buffer_len += copied;
        tcp_recved(tpcb, p->tot_len);
    }

    pbuf_free(p);
    return ERR_OK;
}



static bool tcp_client_open(void *arg) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr), TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, 1);//POLL_TIME_S * 2);
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);

    state->buffer_len = 0;

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT, tcp_client_connected);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

// Perform initialisation
static TCP_CLIENT_T* tcp_client_init(void) {
    TCP_CLIENT_T *state = calloc(1, sizeof(TCP_CLIENT_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    ip4addr_aton(TEST_TCP_SERVER_IP, &state->remote_addr);
    return state;
}

void run_tcp_client_test(void) {
    TCP_CLIENT_T *state = tcp_client_init();
    if (!state) {
        return;
    }
    if (!tcp_client_open(state)) {
        tcp_client_close(state);
        tcp_result(state, -1);
        free(state);
        return;
    }
    while(!state->complete) {
        // the following #ifdef is only here so this same example can be used in multiple modes;
        // you do not need it in your code
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    tcp_client_close(state);
}

int main() {
    stdio_init_all();
    sleep_ms(5000);

    uint32_t sys_hz = clock_get_hz(clk_sys);
    printf("System clock: %u Hz\n", sys_hz);

    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return 1;
    } else {
        printf("Connected.\n"); 
    }
    while (true){
        run_tcp_client_test();
        sleep_ms(100);
    }
    cyw43_arch_deinit();
    return 0;
}