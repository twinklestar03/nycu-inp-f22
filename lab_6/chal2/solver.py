from ctypes import CDLL
from pwn import *


context.log_level = 'debug'

# p = process('./chal2')
p = remote('inp111.zoolab.org', 10007)
p.sendline('AAA')

libc = CDLL("libc.so.6")

time = libc.time(0)
libc.srand(time)
time_rand = libc.rand()
print('[+] time_rand = ', time_rand)
key_aaa = 172048705

def gather_seed(p):
    for _ in range(3):
        p.sendline(b'1')
        p.sendline('AAA')
        p.recvline()
    p.sendline(b'1')
    p.recvuntil(b'(from seed ')
    return int(p.recvuntil(')', drop=True), 16)


seed = gather_seed(p)
print('[+] Get seed:', hex(seed))

libc.srand(seed)

for i in range(4):
    libc.rand()

next_rand = libc.rand()
print(f'[+] Next rand: {next_rand & 0x7fffffff}')

# Guessing pid
target_seed = gather_seed(p)
print('[+] Get seed:', hex(target_seed))

guess_pid = 1
for i in range(1000, 900000):
    guess_pid = i
    libc.srand(guess_pid ^ time)

    guess_magic = libc.rand()
    oseed = guess_magic ^ next_rand ^ guess_pid ^ key_aaa

    if oseed == target_seed:
        print('[+] Found pid:', guess_pid)
        break

print('[+] PID should be:', guess_pid)

libc.srand(target_seed)
for i in range(4):
    libc.rand()

answer_rand = libc.rand()
print(f'[+] Answer rand: {answer_rand & 0x7fffffff}')

oseed = answer_rand ^ guess_magic ^ guess_pid ^ key_aaa
print(f'[+] oseed: {oseed}')
# build secret
'''
len  = snprintf(buf,     10, "%x", rand());
len += snprintf(buf+len, 10, "%x", rand());
len += snprintf(buf+len, 10, "%x", rand());
len += snprintf(buf+len, 10, "%x", rand());
'''
libc.srand(oseed)
secret = ''
for i in range(4):
    a = libc.rand()
    secret += hex(a)[2:]
print('[+] Secret:', secret)

p.sendline(b'1')
p.sendline(b'AAA')
p.sendline(b'1')
p.sendline(secret.encode())

p.interactive()


# oseed = (magic ^ next_rand ^ key_aaa) & 0x7fffffff;
# fun(time(0) ^ pid) = magic
# ( f(time(0) ^ pid) ^ next_rand ^ pid ^ key_aaa) = seed