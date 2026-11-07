ACTION = 'action'
STATUS = 'status'
OPERATE = 'operate'
INDEX = 'index'
ABOUT = 'about'

def start_door(num):
    print('operating door...')
    control.operate_relay(num)
    print('done')
    return door_status(num)

def door_status(num):
    json = {STATUS:'closed','index':num}
    if(control.reed_switch_status(num)):
        json = {STATUS:'open', 'index':num}
    return json

def handleRequest(source,msg):
    try:
        print('got resquest:',source,msg)
        if msg[ACTION] == STATUS:
            msg.update(door_status(int(msg[INDEX])))
        elif msg[ACTION] == OPERATE:
            msg.update(start_door(int(msg[INDEX])))
        elif msg[ACTION] == ABOUT:
            msg.update({ps.ID:ps.get(ps.ID),ps.NAME:ps.get(ps.NAME),'ip':wifi.ifconfig()[0]})
        else:
            raise Exception('invalid input')
        msg.update({'success':True})
    except Exception as e:
        msg.update({'success':False})
    return msg

def start():
    ping_timer=ticks_ms()
    ping_interval=3*60*1000 #min*s*ms
    http.init()
    mqtt.connect()
    http.set_callBack(handleRequest)
    mqtt.set_callBack(handleRequest)
    mqtt.subscribe()
    prev_statuses=[door_status(x) for x in control.get_list()]
    [mqtt.send_msg(x) for x in prev_statuses]
    while True:
        mqtt.get_request()
        http.get_request()
        if ticks_diff(ticks_ms(), ping_timer) >= ping_interval:
            mqtt.ping()
            ping_timer = ticks_ms()
        for num in control.get_list():
            status = door_status(num)
            if prev_statuses[num] != status:
                http.send_notification(
                    ps.get(ps.HOME_ID),
                    ps.get(ps.NAME),
                    ps.get(ps.NAME)+' is '+status[STATUS])
                mqtt.send_msg(status)
                prev_statuses[num] = status
        control.led_blink(1,500,4000)

start()
