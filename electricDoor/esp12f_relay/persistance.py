import json
import ubinascii

WIFI_SSID = 'wifiSsid'
WIFI_PASSWORD = 'wifiPassword'
ACTIVE = 'active'
NAME = 'name'
ID = 'id'
HOME_ID = 'homeId'
MQTT_BROKER = 'mqttBrokerId'
MQTT_USER = 'mqttUser'
MQTT_PASSWORD = 'mqttPassword'

fileName = 'persistance.json'

def read():
    with open(fileName,'r') as f:
        value = f.read()
        f.close()
        return value

def write(content):
    with open(fileName,'w') as f:
        f.write(content)
        f.close()

def clear():
    write('{}')

def init():
    try:
       read()
    except:
        clear()

def get_json():
    try:
        value = read()
        return json.loads(value)
    except:
        print('could not read file')
        return {};

def get(key):
    try:
        return get_json()[key]
    except:
        print('cound not get for key: ', key)

def persist(key, value):
    response = get_json()
    response.update({key: value})
    write(json.dumps(response))

def getToken(initial=False):
    try:
        name = get(HOME_ID)
        if not initial:
            name+='/'+get(ID)
        return ubinascii.b2a_base64(f'{name}/user:keylock'.encode()).decode().strip()
    except Exception as e:
        print('Error:',e)
        return ''

init()
