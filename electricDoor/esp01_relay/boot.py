import network
import control_0 as control
import http_0 as http
import mqtt_0 as mqtt
import gc
import persistance as ps
from utime import sleep_ms, ticks_ms, ticks_diff
import uasyncio as asyncio

gc.collect()

hotspot = network.WLAN(network.AP_IF)
wifi = network.WLAN(network.STA_IF)

def handle_credential(source, msg):
    print('got:', source, msg)
    ssid = msg[ps.WIFI_SSID];
    password = msg[ps.WIFI_PASSWORD]
    if connect_wifi(ssid, password):
        ps.persist(ps.WIFI_SSID, ssid)
        ps.persist(ps.WIFI_PASSWORD, password)
        ps.persist(ps.NAME, msg[ps.NAME])
        ps.persist(ps.ID, msg[ps.ID])
        ps.persist(ps.HOME_ID, msg[ps.HOME_ID])
        ps.persist(ps.MQTT_BROKER, msg[ps.MQTT_BROKER])
        ps.persist(ps.MQTT_USER, msg[ps.MQTT_USER])
        ps.persist(ps.MQTT_PASSWORD, msg[ps.MQTT_PASSWORD])
        ps.persist(ps.ACTIVE, True)
        return {'success':True,'ip':wifi.ifconfig()[0]}
    else:
        return {'success':False}
    
def create_hotspot(ssid, password):
    hotspot.active(False)
    hotspot.config(ssid=ssid, password=password)
    hotspot.active(True)
    print('hotspot', hotspot.ifconfig())

def connect_wifi(ssid, password):
    connection_timeout=10*1000#s*ms
    connection_start=ticks_ms()
    wifi.active(True)
    wifi.connect(ssid, password)
    while not wifi.isconnected():
        if ticks_diff(ticks_ms(),connection_start)>=connection_timeout:
            wifi.active(False)
            return False
    print('Connection successful')
    print(wifi.ifconfig())
    control.led_blink(3, 200, 100)
    return True

def boot():
    if ps.get(ps.ACTIVE) == None:
        ps.persist(ps.ACTIVE, False)
    if ps.get(ps.ACTIVE):
        connect_wifi(ps.get(ps.WIFI_SSID), ps.get(ps.WIFI_PASSWORD))
    else:
        create_hotspot('homeQ','abcd1234')
        http.init()
        http.set_callBack(handle_credential)
        while not ps.get(ps.ACTIVE):
            http.get_request()
            control.led_blink(1, 100, 100)
        control.led_blink(3, 1000, 100)
        hotspot.active(False)
        control.reset()

boot()
