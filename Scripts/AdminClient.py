import socket

ADMIN_PORT = 7777

ips = input("Enter IPs, seperated by spaces, of modules to send to:\n").split(" ")
command = input("Enter command to send to modules:\n")

for ip in ips:
    print(f"Connecting to {ip}")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((ip, ADMIN_PORT))
        s.sendall(str.encode(command))
        print(f"Sent command to {ip}")