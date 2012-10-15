import hdhr
from pprint import pprint

get_supported_channelmaps = lambda ip: \
        hdhr.get_supported(ip, 'channelmap: ').split(' ')

print("Devices found:\n")

devices = hdhr.find_devices()

pprint(devices)

print

first_ip = devices[0]['IP']

print("Status for tuner [%s]-0:\n" % (first_ip))

pprint(hdhr.get_tuner_status(first_ip))

print

supported = get_supported_channelmaps(first_ip)

print("Supported: %s" % (supported))

#print("\nChannel list:\n")
#
#pprint(hdhr.get_channel_list(first_ip, 'us-cable'))
#
#print

print("\nVStatus:\n")

pprint(hdhr.get_tuner_vstatus(first_ip))

print

