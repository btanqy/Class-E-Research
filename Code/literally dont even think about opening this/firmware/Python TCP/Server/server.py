import network
import usocket as socket
import time

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect("UCLA_PEEC","PEEC53133")

# Wait for connection
while not wlan.isconnected():
    time.sleep(1)
print("Connection Completed")
print('WiFi connected')
print(wlan.ifconfig())

# Set up UDP server
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((wlan.ifconfig()[0], 12345))
print("Server is Up and Listening")
print(wlan.ifconfig()[0])

while True:
    print('Waiting for a request from the client...')
    # Receive request from client
    request, client_address = server_socket.recvfrom(1024)
    print("Client Request:",request.decode())
    print("FROM CLIENT",client_address)
    
    # String to send
    data = "225,128,64"
    
    # Send data to client
    server_socket.sendto(data.encode(), client_address)
    print(f'Sent data to {client_address}')
    
    # Optional: Pause for a short period to prevent overwhelming the client
    time.sleep(1)