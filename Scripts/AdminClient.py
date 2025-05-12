import socket
import sys

"""
Utility script to send one-off commands to modules without requiring connection to server.
Modules will listen for these "admin" while attemping to connect to the server.
Example usage:

python AdminClient.py 192.168.1.10,192.168.1.11,192.168.1.12 SET_EEPROM_VALUE SERVERIP 192.168.1.100
"""

ADMIN_PORT = 7777

ips = sys.argv[1].split(',')
command = ' '.join(sys.argv[2:])

print(f"Attemping to issue command '{command}' to modules at following ips:")
print(' | '.join(ips))

for ip in ips:
    print(f"Connecting to {ip}... ", end='')
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        try:
            s.settimeout(5)
            s.connect((ip, ADMIN_PORT))
            s.sendall(str.encode(command))
            print(f"Success")
        except TimeoutError:
            print("FAILED due to timeout!")
