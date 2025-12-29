from umqtt.simple import MQTTClient
import ubinascii
from control_0 import unique_id, reset
import ssl
import json
import persistance as ps
import http_0 as http

class Mqtt:
    home_id=ps.get(ps.HOME_ID);
    device_id=ps.get(ps.ID);
    if home_id is None: home_id='';
    if device_id is None: device_id='';

    CLIENT_ID=ubinascii.hexlify(unique_id())
    HOME='home/'+home_id
    ACTION='action'
    REACTION='reaction'
    PING_TOPIC=HOME+'/ping/'+device_id
    initialised=False
    connected=False

    def init(self):
        self.mqttClient = MQTTClient(client_id=self.CLIENT_ID,
                            server=ps.get(ps.MQTT_BROKER),
                            user=ps.get(ps.MQTT_USER),
                            password=ps.get(ps.MQTT_PASSWORD),
                            keepalive=7200,
                            ssl=ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT))
        self.initialised=True

    def connect(self):
        try:
            print(f'Begin connection with MQTT Broker :: {ps.get(ps.MQTT_BROKER)}')
            if not self.initialised:
                self.init()
            self.mqttClient.connect()
            self.connected=True
            print('MQTT connected')
        except OSError as e:
            print('Error:',e)

    def send_msg(self,msg):
        try:
            print('mqtt sending...',msg)
            self.mqttClient.publish(self.HOME+'/'+self.REACTION+'/'+self.device_id,json.dumps(msg))
        except:
            print('mqtt could not send msg')

    def set_callBack(self,func):
        self.callBack = func

    def handle_request(self,source,msg):
        self.connected=True
        source = source.decode()
        msg = msg.decode()
        if self.PING_TOPIC not in source:
            self.send_msg(self.callBack(json.loads(msg),ps.getToken()))
    
    def get_request(self):
        try:
            self.mqttClient.check_msg()
        except:
            pass
    
    def ping(self):
        if self.connected:
            print('pinged')
            self.connected=False
            self.mqttClient.publish(self.PING_TOPIC,'{}')
        elif http.is_connected():
            reset()
        else:
            print('http not connected')

    def subscribe(self):
        if self.connected:
            self.mqttClient.set_callback(self.handle_request)
            self.mqttClient.subscribe(self.PING_TOPIC)
            self.mqttClient.subscribe(self.HOME+'/'+self.ACTION+'/'+self.device_id)
