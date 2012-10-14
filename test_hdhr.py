import hdhr
from pprint import pprint

get_supported_channelmaps = lambda ip: \
    hdhr.get_supported(ip, 'channelmap')[2:].split(' ')

print("Devices found:\n")

devices = hdhr.find_devices()

pprint(devices)

first_ip = devices[0]['IP']

print("\nStatus for tuner [%s]-0:\n" % (first_ip))

pprint(hdhr.get_tuner_status(first_ip, 0))

supported = get_supported_channelmaps(first_ip)

print("\nSupported: %s" % (supported))

