import hdhr
from pprint import pprint

print("Devices found:\n")

devices = hdhr.find_devices()

pprint(devices)

first_ip = devices[0]['IP']

print("\nStatus for tuner [%s]-0:\n" % (first_ip))

pprint(hdhr.get_tuner_status(first_ip, 0))

print

