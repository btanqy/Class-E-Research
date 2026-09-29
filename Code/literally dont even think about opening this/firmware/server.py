#!/usr/bin/python

import socket

# Server configuration
SERVER_ADDR = "0.0.0.0"
SERVER_PORT = 4242
BUF_SIZE = 2048

# Create and bind socket
sock = socket.socket()
sock.bind((SERVER_ADDR, SERVER_PORT))
sock.listen(1)
print("Server listening on", SERVER_ADDR, SERVER_PORT)

# Wait for a client
con, addr = sock.accept()
print("Client connected from", addr)

try:
    while True:
        # Get message from user
        message = input(">> ").encode('utf-8')

        if message.decode() == "DISCONNECT":
            exit()

        # Send message with end byte 0x04
        message_len = con.send(message + b'\x04')
        print(f'Written {message_len} bytes to client')

        # Read response from client
        total_size = message_len # adjust as needed
        read_buf = b''
        attempts = 0

        while total_size > 0 and attempts < 10:
            buf = con.recv(total_size)
            if not buf:
                break
            read_buf += buf
            total_size -= len(buf)
            attempts += 1

        print(f'Read {len(read_buf)} bytes from client')
        if read_buf:
            print("Ack:", read_buf.decode())

except KeyboardInterrupt:
    print("\nServer shutting down")

finally:
    con.close()
    sock.close()
