import hdhr
from pprint import pprint
from sys import exit

get_supported_channelmaps = lambda ip: \
        hdhr.get_supported(ip, 'channelmap: ').split(' ')

print("Devices found:\n")

devices = hdhr.find_devices()

pprint(devices)

print

first_ip = devices[0]['IP']

print("Status for tuner [%s]-0:\n" % (first_ip))

pprint(hdhr.get_tuner_status(first_ip))

print("\nSupported features:\n")

print(hdhr.get_supported(first_ip))

supported = get_supported_channelmaps(first_ip)

print("Supported channelmaps: %s" % (supported))

#print("\nChannel list:\n")
#
#pprint(hdhr.get_channel_list(first_ip, 'us-cable'))
#
#print

print("\nVStatus:\n")

pprint(hdhr.get_tuner_vstatus(first_ip))

print

#print("LockKey:\n")

#lock_key = hdhr.acquire_lockkey(first_ip)
#pprint(lock_key)

print

print("Scanning channels.")

def progress_callback(channel_info, scan_progress):
    print("Processed channel (%d)." % (scan_progress))

    pprint(channel_info)

    print

    return True

hdhr.scan_channels(first_ip + "-2", 'us-cable', progress_callback)

exit()

print

#hdhr.clear_target(first_ip)

vchannel = '66'
print("Acquire (%s).\n" % (vchannel))

hdhr.set_vchannel(first_ip, vchannel)
is_locked = hdhr.wait_for_lock(first_ip);

if not is_locked:
    print("Could not lock on channel [%s]." % (vchannel))
    exit()

print("Channel changed. Receiving video.")

hdhr.set_target(first_ip, '192.168.5.112:9999')

print

