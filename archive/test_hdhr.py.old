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

# We want to use our third tuner.
tuner = ("%s-%d" % (first_ip, 2))

print("Status for tuner [%s]:\n" % (tuner))

pprint(hdhr.get_tuner_status(tuner))

print("\nSupported features:\n")

print(hdhr.get_supported(tuner))

supported = get_supported_channelmaps(tuner)

print("Supported channelmaps: %s" % (supported))

#print("\nChannel list:\n")
#
#pprint(hdhr.get_channel_list(tuner, 'us-cable'))
#
#print

print("\nVStatus:\n")

pprint(hdhr.get_tuner_vstatus(tuner))

print

#print("LockKey:\n")

#lock_key = hdhr.acquire_lockkey(tuner)
#pprint(lock_key)

#print

#print("Scanning channels.")
#
#def progress_callback(channel_info, scan_progress):
#    print("Processed channel (%d)." % (scan_progress))
#
#    pprint(channel_info)
#
#    print
#
#    return True
#
#hdhr.scan_channels(tuner, 'us-cable', progress_callback)
#
#exit()
#
#print

#hdhr.clear_target(tuner)
#exit()

vchannel = '67'
print("Acquire (%s).\n" % (vchannel))

hdhr.set_vchannel(tuner, vchannel)
is_locked = hdhr.wait_for_lock(tuner);

if not is_locked:
    print("Could not lock on channel [%s]." % (vchannel))
    exit()

print("Channel changed. Receiving video.")

hdhr.set_target(tuner, '192.168.5.112:9999')

print

