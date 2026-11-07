class Main:
    
    ACTION='action'
    STATUS='status'
    OPERATE='operate'
    INDEX='index'
    ABOUT='about'
    
    blink_timer=ticks_ms()
    ping_timer=ticks_ms()
    status_timer=ticks_ms()
    offline_timer=ticks_ms()
    offline_interval=4*60*60*1000 #min*s*ms
    ping_interval=2*60*1000
    status_interval=5*1000
    
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

    def handleRequest(self,msg,token):
        try:
            print('got:',msg)
            if token != ps.getToken():
                raise Exception('auth failed')
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
        if ticks_diff(ticks_ms(),self.blink_timer) >= self.status_interval:
            control.led_blink(500,500)
            self.blink_timer=ticks_ms()
        if ticks_diff(ticks_ms(),self.ping_timer) >= self.ping_interval:
            self.mqtt.ping()
            self.ping_timer=ticks_ms()
        if ticks_diff(ticks_ms(),self.offline_timer) >= self.offline_interval:
            self.offline_timer = ticks_ms()
            if not http.is_connected():
                control.reset()
    
    def handle_status(self,prev_statuses):
        if ticks_diff(ticks_ms(),self.status_timer) >= self.status_interval:
            self.status_timer=ticks_ms()
            for i in range(len(prev_statuses)):
                status = self.door_status(i)
                if prev_statuses[i][self.STATUS] != status[self.STATUS]:
                    status.update({'old':prev_statuses[i][self.STATUS],ps.NAME:ps.get(ps.NAME)})
                    self.mqtt.send_msg(status)
                    prev_statuses[i] = status

    def start(self):
        http.init()
        self.mqtt.connect()
        http.set_callBack(self.handleRequest)
        self.mqtt.set_callBack(self.handleRequest)
        self.mqtt.subscribe()
        prev_statuses=[self.door_status(i) for i in range(control.getSize())]
        [self.mqtt.send_msg(x) for x in prev_statuses]
        while True:
            self.mqtt.get_request()
            http.get_request()
            self.handle_recovery()
            self.handle_status(prev_statuses)
            

Main().start()
