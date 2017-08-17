import socket
import sys
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.bind(("", 46835))
s.listen()

def normal_protocol(conn):
    msg = conn.recv(4)
    size = int.from_bytes(msg, 'little')
    if size != 0:
        msg = conn.recv(size)
        print("  [R] ", size, msg)
    else:
        raise Exception("no data!")

for i in range(10000):
    try:
        conn, addr = s.accept()
        print("Client connected!")
        while conn:
            normal_protocol(conn)
    except Exception:
        pass
    finally:
        conn.close()

