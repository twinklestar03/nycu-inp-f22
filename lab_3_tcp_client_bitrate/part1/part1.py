import socket
    

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

def recvline(s):
    data = b''
    while not data.endswith(b'\n'):
        data += s.recv(1)
    return data

def recvall(s, delim):
    data = b''
    while not data.endswith(delim):
        data += s.recv(1024)
    return data

s.connect(('inp111.zoolab.org', 10002))

recv = recvline(s)
print(recv)
recv = recvline(s)
print(recv)

s.send('GO\n'.encode())

# ==== BEGIN DATA ====\n
recv = recvline(s)

# Data body
data = recvall(s, b'\n')
print(data)

answer = len(data) - len('\n==== END DATA ====\n** How many bytes of data have you received?\n')
print('[+] Answer: {}'.format(answer))
s.send('{}\n'.format(answer).encode())

recv = s.recv(1024)
print(recv.decode())


