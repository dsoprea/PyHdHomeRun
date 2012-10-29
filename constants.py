import os

HDHR_FILEPATH = os.environ['HDHOMERUN_PATH'] \
                    if 'HDHOMERUN_PATH' in os.environ \
                    else '/usr/lib/libhdhomerun.so'
                    
MAX_DEVICES = 16

HDHOMERUN_DEVICE_TYPE_TUNER             = 0x00000001
HDHOMERUN_DEVICE_ID_WILDCARD            = 0xFFFFFFFF
HDHOMERUN_CHANNELSCAN_MAX_PROGRAM_COUNT = 64

MAP_AU_BCAST = 'au-bcast'
MAP_AU_BCAST = 'au-cable'
MAP_EU_BCAST = 'eu-bcast'
MAP_EU_CABLE = 'eu-cable'
MAP_TW_BCAST = 'tw-bcast'
MAP_TW_CABLE = 'tw-cable'
MAP_KR_BCAST = 'kr-bcast'
MAP_KR_CABLE = 'kr-cable'
MAP_US_BCAST = 'us-bcast'
MAP_US_CABLE = 'us-cable'
MAP_US_HRC	 = 'us-hrc'
MAP_US_IRC   = 'us-irc'

