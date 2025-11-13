from utime import sleep_ms

try:
    import usocket as socket
except:
    import socket

from machine import Pin, UART
import network

import esp
esp.osdebug(None)

import gc
gc.collect()

uart = UART(1, 9600)
uart.init(9600, bits=8, parity=None, stop=1)

#The Onboard LED, used to show connection status
led = Pin(2, Pin.OUT)
#The Relay that sends the impules to the Door
relay = Pin(0, Pin.OUT)

#Set both the LED and relay to OFF 
led.value(1)
relay.value(1)

# relOn = chr(0xA0) + chr(0x01) + chr(0x01) + chr(0xA2) # [0xA0,0x01,0x01,0xA2] # bytearray([160,1,1,162])
# relOff = chr(0xA0) + chr(0x01) + chr(0x00) + chr(0xA1) # [0xA0,0x01,0x00,0xA2] # bytearray([160,1,0,161])

relOn = b'\xa0\x01\x01\xa2'
relOff = b'\xa0\x01\x00\xa1'

#Replace with your Wifi SSID and Password
ssid = 'AllNet'
password = 'Brendan1pass#'
wifi = network.WLAN(network.STA_IF)

#Connect to WiFi
def connectWifi():
    global wifi
    global led
    wifi.active(True)
    wifi.connect(ssid, password)
    while wifi.isconnected() == False:
        pass
    print('Connection successful')
    print(wifi.ifconfig())
    #Indicate that Wifi is connected with the on-board LED
    led.value(0)
    sleep_ms(200)
    led.value(1)
    sleep_ms(100)
    led.value(0)
    sleep_ms(200)
    led.value(1)
    sleep_ms(100)
    led.value(0)
    sleep_ms(200)
    led.value(1)

connectWifi()
