from machine import Pin
import rp2
import time

# === PIO Program ===
@rp2.asm_pio(set_init=rp2.PIO.OUT_LOW)
def pwm_prog():
    # ON time
    set(pins, 1)
    nop() [on_delay]   # Delay while pin is high
    # OFF time
    set(pins, 0)
    nop() [off_delay]  # Delay while pin is low
    jmp(pwm_prog)

# === Parameters ===
frequency = 100_000        # Hz
duty_cycle = 0.4           # 40%
sys_clk = 125_000_000      # RP2040 default clock = 125 MHz
pwm_period_cycles = int(sys_clk / frequency)

on_cycles = int(pwm_period_cycles * duty_cycle)
off_cycles = pwm_period_cycles - on_cycles

# Delay per instruction = 1 cycle + delay (0-31)
# So we subtract 1 to get the [delay] part
on_delay = max(0, on_cycles - 1)
off_delay = max(0, off_cycles - 1)

# Limit to max delay of 31 (PIO instruction limit)
if on_delay > 31 or off_delay > 31:
    raise ValueError("Delay too long — lower frequency or use wraparound PIO")

# === Launch State Machine ===
from rp2 import StateMachine

sm = StateMachine(0, pwm_prog, freq=sys_clk, set_base=Pin(0))
sm.exec("set(pins, 0)")  # Reset pin to LOW
sm.active(1)

print(f"Running PWM on Pin 0 at {frequency} Hz, {duty_cycle*100}% duty.")