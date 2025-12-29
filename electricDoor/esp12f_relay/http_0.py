try:
    import usocket as socket
except:
    import socket
import uselect
import persistance as ps
import json
import ssl

CONTENT_TYPE='content-type'
JSON_CONTENT_TYPE='application/json'

def init():
    global s,udp,poller
    s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    s.bind(('',80))
    s.listen(5)
    udp = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    udp.bind(('', 37020))
    poller = uselect.poll()
    poller.register(s,uselect.POLLIN)
    poller.register(udp,uselect.POLLIN)

def is_connected():
    try:
        socket.getaddrinfo('www.google.com',80)
        return True
    except:
        print('network disconnected...')
        return False
    
def set_callBack(func):
    global callBack
    callBack = func

def send(conn,respCode,content,contentType):
    try:
        conn.send('HTTP/1.1 '+respCode+' OK\n')
        conn.send(CONTENT_TYPE+': '+contentType+'\n')
        conn.send('Connection: close\n\n')
        conn.sendall(content)
        conn.close()
    except Exception as e:
        print('could not send:',e)

def get_request_data(conn):
    body = ''
    token = ''
    content_length = 1
    headers = None
    while len(body) < content_length:
        chunk = conn.recv(1024)
        if not chunk:
            break
        chunk = chunk.decode()
        if headers is None:
            headers,body = chunk.split('\r\n\r\n',1)
            for line in headers.split('\r\n'):
                if 'authorization: basic' in line.lower():
                    token = line.split(' ')[2]
                if 'content-length' in line.lower():
                        content_length = int(line.split(':')[1].strip())
        else:
            body += chunk
    return body,token

def error_respond(conn):
    send(conn,'400',json.dumps({'success':False}),JSON_CONTENT_TYPE)
    
def handle_request():
    conn,addr=s.accept()
    try:
        resp,token = get_request_data(conn)
        resp = json.dumps(callBack(json.loads(resp),token))
        if not resp:
            return error_respond(conn)
        send(conn,'200',resp,JSON_CONTENT_TYPE)
    except:
        error_respond(conn)

def broadcast():
    try:
        data,addr=udp.recvfrom(1024)
        data = data.decode().split('/')
        if data[0] == ps.getToken(True) and data[1] == 'DISCOVER_DEVICES':
            udp.sendto(ps.get(ps.ID),addr)
    except:
        pass

def get_request():
    for sock,event in poller.poll(200):
        if sock is s:
            handle_request()
        elif sock is udp:
            broadcast()