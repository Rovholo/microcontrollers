from machine import Pin, reset as _reset, unique_id as _unique_id, Timer
from utime import sleep_ms

led=Pin(5,Pin.OUT)
relays=[Pin(14,Pin.OUT)]
reed_switches=[Pin(4,Pin.IN, Pin.PULL_UP)]

def led_on():
    led.off()

def led_off():
    led.on()
    
def relay_on(idx):
    led_on()
    relays[idx].on()

def relay_off(idx):
    relays[idx].off()
    led_off()

def operate_relay(idx):
    relay_on(idx)
    Timer(-1).init(period=500, mode=Timer.ONE_SHOT, callback=lambda timer:relay_off(idx))
    
    
def reed_switch_status(idx):
    return reed_switches[idx].value()

def led_blink(on_ms,off_ms):
    Timer(-1).init(period=off_ms, mode=Timer.ONE_SHOT, callback=lambda timer:led_on())
    Timer(-1).init(period=on_ms+off_ms, mode=Timer.ONE_SHOT, callback=lambda timer:led_off())
        
def reset():
    return _reset()
    
def unique_id():
    return _unique_id()

def getSize():
    return len(relays)

led_off()
[relay_off(idx) for idx in range(getSize())]