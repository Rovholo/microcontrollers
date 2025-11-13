from umqtt.simple import MQTTClient
import ubinascii
from control_0 import unique_id, reset
import ssl
import json
import persistance as ps
import http_0 as http

home_id = ps.get(ps.HOME_ID);
device_id = ps.get(ps.ID);
if home_id is None: home_id='';
if device_id is None: device_id='';

CLIENT_ID = ubinascii.hexlify(unique_id())
HOME = 'home/'+home_id
ACTION = 'action'
REACTION = 'reaction'
PING_TOPIC = HOME+'/ping/'+device_id
initialised = False
connected = False

def is_init():
    return initialised

def is_connected():
    return connected

def set_connected(val):
    global connected;
    connected = val

def init():
    global mqttClient
    global initialised
    mqttClient = MQTTClient(client_id = CLIENT_ID,
                            server = ps.get(ps.MQTT_BROKER),
                            user = ps.get(ps.MQTT_USER),
                            password = ps.get(ps.MQTT_PASSWORD),
                            keepalive = 7200,
                            ssl = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT))
    initialised = True

def connect():
    try:
        print(f'Begin connection with MQTT Broker :: {ps.get(ps.MQTT_BROKER)}')
        if not is_init():
            init()
        mqttClient.connect()
        set_connected(True)
        print('MQTT connected')
    except OSError as e:
        print('Error:', e)

def send_msg(msg):
    try:
        print('mqtt sending...', msg)
        mqttClient.publish(HOME+'/'+REACTION+'/'+device_id,json.dumps(msg))
    except:
        print('mqtt could not send msg')

def set_callBack(func):
    global callBack
    callBack = func

def handle_request(source, msg):
    set_connected(True)
    source = source.decode()
    msg = msg.decode()
    if PING_TOPIC not in source:
        send_msg(callBack(source,json.loads(msg)))
    
def get_request():
    try:
        mqttClient.check_msg()
    except:
        pass
    
def ping():
    if is_connected():
        print('pinged')
        set_connected(False)
        mqttClient.publish(PING_TOPIC,'{}')
    elif http.is_connected():
        reset()
    else:
        print('http not connected')

def subscribe():
    if is_connected():
        mqttClient.set_callback(handle_request)
        mqttClient.subscribe(HOME+'/'+ACTION+'/'+device_id)
        mqttClient.subscribe(PING_TOPIC)