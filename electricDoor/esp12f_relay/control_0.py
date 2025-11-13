from machine import Pin, reset as _reset, unique_id as _unique_id
from utime import sleep_ms

led = Pin(5, Pin.OUT)
relays = [Pin(14, Pin.OUT)]
reed_switches = [Pin(4, Pin.IN, Pin.PULL_UP)]

def led_on():
    led.off()

def led_off():
    led.on()
    
def relay_on(idx):
    relays[idx].on()

def relay_off(idx):
    relays[idx].off()

def operate_relay(idx):
    led_on()
    relay_on(idx)
    sleep_ms(500)
    relay_off(idx)
    led_off()
    
def reed_switch_status(idx):
    return reed_switches[idx].value()

def led_blink(num, on_ms, off_ms):
    for x in range(num):
        led_on()
        sleep_ms(on_ms)
        led_off()
        sleep_ms(off_ms)
        
def reset():
    return _reset()
    
def unique_id():
    return _unique_id()

def get_list():
    return range(len(relays))

led_off()
[relay_off(idx) for idx in get_list()]