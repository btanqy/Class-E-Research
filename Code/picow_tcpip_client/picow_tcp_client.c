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

#include "lwip/timeouts.h"
#include "hardware/pwm.h"


#if !defined(TEST_TCP_SERVER_IP)
#error TEST_TCP_SERVER_IP not defined
#endif

#define TCP_PORT 4242
#define DEBUG_printf printf
#define BUF_SIZE 128

#define TEST_ITERATIONS 2
#define POLL_TIME_S 60  
#define LONG_TIMEOUT_MS (3600 * 1000)


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

uint32_t pwm_set_freq_duty(uint slice_num,
       uint chan,int f, int d)
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
    }
    state->complete = true;
    return tcp_client_close(arg);
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T *state = (TCP_CLIENT_T*)arg;
    DEBUG_printf("tcp_client_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        state->run_count++;
        // if (state->run_count >= TEST_ITERATIONS) {
        //     tcp_result(arg, 0);
        //     return ERR_OK;
        // }

        // We should receive a new buffer from the server
        state->buffer_len = 0;
        state->sent_len = 0;
        DEBUG_printf("Waiting for buffer from server\n");
    }

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
    if (!p) {
        return tcp_result(arg, -1);
    }

    cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DUMP_BYTES(q->payload, q->len);
        }

        const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
        int bytes_to_copy = p->tot_len > buffer_left ? buffer_left : p->tot_len;
        int copied = pbuf_copy_partial(p, state->buffer + state->buffer_len, bytes_to_copy, 0);
        state->buffer_len += copied;

        char temp_buffer[BUF_SIZE];
        int len = state->buffer_len < sizeof(temp_buffer) - 1 ? state->buffer_len : sizeof(temp_buffer) - 1;
        memcpy(temp_buffer, state->buffer, len);
        temp_buffer[len] = '\0'; // safe null-termination

        char *tokens[4];
        int count = 0;
        char *ptr = temp_buffer;

        while (*ptr != '\0' && count < 4) {
            while (*ptr == ' ' && *ptr != '\0') ptr++;
            if (*ptr == '\0') break;

            tokens[count++] = ptr;

            while (*ptr != ' ' && *ptr != '\0') ptr++;

            if (*ptr != '\0') {
                *ptr = '\0';
                ptr++;
            }
        }

        // DEFINITIONS

        printf(tokens[0]);

        if (strcmp(tokens[0], "exit") == 0) {
            printf("Exit\n");
            tcp_write(tpcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
            tcp_close(state->tcp_pcb);
            return tcp_result(arg, -1);
        }
        else if (strcmp(tokens[0], "freq") == 0){
            
        }
        else if (strcmp(tokens[0], "pwm") == 0){
            
        }
        else if (strcmp(tokens[0], "freq-pwm") == 0){
            const char *freq = tokens[1];
            const char *duty = tokens[2];
            uint slice_num = pwm_gpio_to_slice_num(atoi(tokens[3]));
            uint chan      = pwm_gpio_to_channel(atoi(tokens[3]));
            gpio_set_function(atoi(tokens[3]), GPIO_FUNC_PWM);
            pwm_set_freq_duty(slice_num, chan, atoi(freq), atoi(duty));
        }

        else if (strcmp(tokens[0], "EN") == 0){
            uint slice_num = pwm_gpio_to_slice_num(atoi(tokens[1]));
            uint en = atoi(tokens[2]);
            if(en == 0){
                gpio_set_function(14, GPIO_FUNC_PWM);
                gpio_set_function(15, GPIO_FUNC_PWM);
                gpio_put(15, 0);
                gpio_put(14, 0);
            } else if (en == 1){
                gpio_set_function(15, GPIO_FUNC_PWM);
                pwm_set_enabled(slice_num, en);
            }
            
        }

        else {
            state->buffer[state->buffer_len - 7] = ' ';
            state->buffer[state->buffer_len - 6] = 'N';
            state->buffer[state->buffer_len - 5] = 'O';
            state->buffer[state->buffer_len - 4] = 'T';
        }
    }

    pbuf_free(p);

    // --- Write back if buffer is full ---
    if (state->buffer_len == BUF_SIZE) {
        DEBUG_printf("Writing %d bytes to server\n", state->buffer_len);

        // Make sure you don't overwrite out-of-bounds
        if (state->buffer_len >= 3) {
            state->buffer[state->buffer_len - 3] = ' ';
            state->buffer[state->buffer_len - 2] = 'O';
            state->buffer[state->buffer_len - 1] = 'K';
        }

        err_t err = tcp_write(tpcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            DEBUG_printf("Failed to write data %d\n", err);
            return tcp_result(arg, -1);
        }
    }

    return ERR_OK;
}


// static void tcp_client_timer(void *arg) {
//     struct tcp_client_state *state = (struct tcp_client_state *)arg;

//     if (state && state->tcp_pcb) {
//         tcp_client_poll(state->tcp_pcb, state);
//     }
// }

static void long_timeout_cb(void *arg) {
    struct tcp_client_state *state = (struct tcp_client_state *)arg;

    sys_timeout(LONG_TIMEOUT_MS, long_timeout_cb, state);
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
    // tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
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
        tcp_result(state, -1);
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
    free(state);
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialise\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        // return 1;
    } //else {
        printf("Connected.\n");
    // }
    run_tcp_client_test();
    cyw43_arch_deinit();
    return 0;
}