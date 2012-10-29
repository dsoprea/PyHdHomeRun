#!/usr/bin/python

from sys import exit
from pprint import pprint

from pyhdhomerun.adapter import HdhrUtility, HdhrDeviceQuery
from pyhdhomerun.constants import MAP_US_CABLE

def find_devices():
    (num_found, devices) = HdhrUtility.discover_find_devices_custom()

    for i in xrange(num_found):
        print(devices[i])

    return (num_found, devices)

def get_tuner_vstatus(device_adapter):
    
    (vstatus, raw_data) = device_adapter.get_tuner_vstatus()
    print(vstatus)

def set_tuner_vstatus(device_adapter, vchannel):
    
    device_adapter.set_tuner_vchannel(vchannel)

    (vstatus, raw_data) = device_adapter.get_tuner_vstatus()
    print(vstatus)

def get_supported(device_adapter):

    pprint(device_adapter.get_supported())

def scan(device_adapter):

    def found(result, scan_info):
        result.dump_programs()

    def progress_update(scan_info, progress):
        print("Progress: %.2f  Scanned= (%d)" % (progress, 
                                                 scan_info.scanned_channels))

    device_adapter.scan_channels(MAP_US_CABLE, found_cb=found, 
                                 incremental_cb=progress_update)

def get_count():

    return HdhrUtility.get_channels_in_range(MAP_US_CABLE)

(num_found, devices) = find_devices()

if not devices:
    print("Could not find any devices.")
    exit()

first_device_str = ("%s-%d" % (devices[0].nice_device_id, 1))

#create_device(first_device_str)

device_adapter = HdhrDeviceQuery(first_device_str)

get_tuner_vstatus(device_adapter)

set_tuner_vstatus(device_adapter, 66)

#get_supported(device_adapter)

#print get_count()

#scan(device_adapter)


