class Main:
    
    ACTION='action'
    STATUS='status'
    OPERATE='operate'
    INDEX='index'
    ABOUT='about'
    
    ping_timer=ticks_ms()
    offline_timer=ticks_ms()
    ping_interval=2*60*1000 #min*s*ms
    offline_interval=4*60*60*1000 #min*s*ms
    
    def __init__(self):
        self.mqtt=Mqtt()

    def start_door(self,num):
        print('operating door...')
        control.operate_relay(num)
        print('done')
        return self.door_status(num)

    def door_status(self,num):
        json = {self.STATUS:'closed',self.INDEX:num}
        if(control.reed_switch_status(num)):
            json = {self.STATUS:'open', self.INDEX:num}
        return json

    def handleRequest(self,source,msg):
        try:
            print('got resquest:',source,msg)
            if msg[self.ACTION] == self.STATUS:
                msg.update(self.door_status(int(msg[self.INDEX])))
            elif msg[self.ACTION] == self.OPERATE:
                msg.update(self.start_door(int(msg[self.INDEX])))
            elif msg[self.ACTION] == self.ABOUT:
                msg.update({ps.ID:ps.get(ps.ID),ps.NAME:ps.get(ps.NAME),'ip':wifi.ifconfig()[0]})
            else:
                raise Exception('invalid input')
            msg.update({'success':True})
        except Exception as e:
            print('caught exception: ',e)
            msg.update({'success':False})
        return msg
    
    def handle_recovery(self):
        if ticks_diff(ticks_ms(), self.ping_timer) >= self.ping_interval:
            self.mqtt.ping()
            self.ping_timer = ticks_ms()
        if ticks_diff(ticks_ms(), self.offline_timer) >= self.offline_interval*2:
            self.offline_timer = ticks_ms()
        if not http.is_connected():
            if ticks_diff(ticks_ms(), self.offline_timer) >= self.offline_interval:
                control.reset()

    def start(self):
        http.init()
        self.mqtt.connect()
        http.set_callBack(self.handleRequest)
        self.mqtt.set_callBack(self.handleRequest)
        self.mqtt.subscribe()
        prev_statuses=[self.door_status(x) for x in control.get_list()]
        [self.mqtt.send_msg(x) for x in prev_statuses]
        while True:
            self.mqtt.get_request()
            http.get_request()
            self.handle_recovery()
            for num in control.get_list():
                status = self.door_status(num)
                if prev_statuses[num] != status:
                    http.send_notification(ps.get(ps.HOME_ID),ps.get(ps.NAME),status[self.STATUS])
                    self.mqtt.send_msg(status)
                    prev_statuses[num] = status
            control.led_blink(1,500,4000)


Main().start()
