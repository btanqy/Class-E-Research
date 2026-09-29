from machine import Pin, PWM
from utime import sleep

s1 = Pin(22, Pin.OUT)
s2 = Pin(21, Pin.OUT)
s3 = Pin(20, Pin.OUT)
s4 = Pin(19, Pin.OUT)
s5 = Pin(18, Pin.OUT)
s6 = Pin(17, Pin.OUT)
drain = Pin(16, Pin.OUT)
pwm = PWM(Pin(16))
pwm.freq(200000)

def swon():
    s1.on()
    s2.on()
    s3.on()
    s4.on()
    s5.on()
    s6.on()
    drain.off()

def stage(num):
    # region stages
    if num == 1:
        s1.on()
        s2.off()
        s3.off()
        s4.off()
        s5.off()
        s6.off()
        drain.off()
    if num == 2:
        s1.off()
        s2.on()
        s3.off()
        s4.off()
        s5.off()
        s6.off()
        drain.off()
    if num == 3:
        s1.off()
        s2.off()
        s3.on()
        s4.off()
        s5.off()
        s6.off()
        drain.off()
    if num == 4:
        s1.off()
        s2.off()
        s3.off()
        s4.on()
        s5.off()
        s6.off()
        drain.off()
    if num == 5:
        s1.off()
        s2.off()
        s3.off()
        s4.off()
        s5.on()
        s6.off()
        drain.off()
    if num == 6:
        s1.off()
        s2.off()
        s3.off()
        s4.off()
        s5.off()
        s6.on()
        drain.off()
    if num == "drain":
        s1.off()
        s2.off()
        s3.off()
        s4.off()
        s5.off()
        s6.off()
        drain.on()
    #endregion

def flyback(duty):
    pwm.duty_u16(int(duty * 655.35))

while True:
    data = input().split(" ")
    if data[0] == "swon":
        swon()
        print("swon OK")
    elif data[0] == "stage":
        try:
            stage(data[1])
            print(f"stage {data[0]} OK")
        except Exception as e:
            print("stage" + str(e))
    elif data[0] == "flyback":
        flyback(float(data[1]))
        print(f"flyback {data[1]} OK")
    else:
        print("Invalid")

    