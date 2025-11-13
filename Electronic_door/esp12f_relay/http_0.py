try:
    import usocket as socket
except:
    import socket
from uselect import select
import json
import ssl

CONTENT_TYPE='content-type'
JSON_CONTENT_TYPE='application/json'

def init():
    global s
    s = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    s.bind(('',80))
    s.listen(5)

def is_connected():
    try:
        socket.getaddrinfo('www.google.com',80)
        return True
    except:
        print("network disconnected...")
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

def get_request_body(conn,addr):
    body = ''
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
                if line.lower().startswith('content-length'):
                        content_length = int(line.split(':')[1].strip())
        else:
            body += chunk
    return body
    
def handle_request(conn,addr):
    resp = get_request_body(conn,addr)
    resp = json.dumps(callBack(addr,json.loads(resp)))
    if not resp:
        return error_respond(conn)
    send(conn,'200',resp,JSON_CONTENT_TYPE)
    
def error_respond(conn):
    send(conn,'400',json.dumps({'success':False}),JSON_CONTENT_TYPE)

def get_request():
    r,w,err = select((s,),(),(),1)
    if r:
        for readable in r:
            conn,addr = s.accept()
            try:
                handle_request(conn,str(addr))
            except:
                error_respond(conn)

def send_notification(homeId,title,body):
    try:
        host = 'apps.bitkulcha.co.za'
        port = 80
        addr = socket.getaddrinfo(host, port)[0][-1]
        
        sock = socket.socket()
        sock.connect(addr)
        data = json.dumps({'topic':homeId,'title':title,'body':body})

        request_headers = [
            'POST /notification/index.php HTTP/1.1',
            f'Host: {host}',
            f'{CONTENT_TYPE}: {JSON_CONTENT_TYPE}',
            f'Content-Length: {len(data)}',
            'Connection: close',
            '',
        ]
        request = '\r\n'.join(request_headers) + '\r\n' + data
        
        sock.send(request)
        
        body = get_request_body(sock,addr)
        sock.close()
        
    except OSError as e:
        print('could not send notification:',e)
