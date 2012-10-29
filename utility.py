
from socket import inet_ntoa

def ip_int_to_ascii(ip_int):
    return inet_ntoa(hex(ip_int)[2:-1].zfill(8).decode('hex'))

