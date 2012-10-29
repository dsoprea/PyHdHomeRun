
import logging

from ctypes import *
from sys import exit
from socket import inet_aton

from pyhdhomerun.externals import *
from pyhdhomerun.types import *
from pyhdhomerun.constants import MAX_DEVICES, HDHOMERUN_DEVICE_TYPE_TUNER, \
                                  HDHOMERUN_DEVICE_ID_WILDCARD
from pyhdhomerun.utility import ip_ascii_to_int

class HdhrUtility(object):
    """Calls that don't require a device entity."""

    @staticmethod
    def discover_find_devices_custom(ip=None):
        """Discover tuner devices."""

        try:
            ip_int = ip_ascii_to_int(ip) if ip else 0
        except:
            logging.exception("Could not convert IP [%s] to integer." % (ip))
            raise

        logging.info("Discovering devices.  MAX= (%d)  IP= [%s]" % 
                     (MAX_DEVICES, ip))

        devices = (TYPE_hdhomerun_discover_device_t * MAX_DEVICES)()

        try:
            num_found = CFUNC_hdhomerun_discover_find_devices_custom(
                            0, 
                            HDHOMERUN_DEVICE_TYPE_TUNER, 
                            HDHOMERUN_DEVICE_ID_WILDCARD, 
                            devices, 
                            MAX_DEVICES
                        )
        except:
            logging.exception("Library call to discover devices failed.")
            raise

        if num_found == -1:
            message = ("Device discovery failed (%d)." % (num_found))

            logging.error(message)
            raise Exception(message)

        logging.info("(%d) devices found." % (num_found))

        return devices[0:num_found]

    @staticmethod
    def device_create_from_str(device_str):
        """Create a device-object to manipulate a specific device with."""

        logging.info("Creating device-entity for device [%s]." % (device_str))
    
        try:
            device = CFUNC_hdhomerun_device_create_from_str(device_str, 
                                                            None
                                                           )
        except:
            logging.exception("Library call to create device entity failed.")
            raise

        if not device:
            message = "Could not build device entity."
            
            logging.exception(message)
            raise Exception(message)

        return device.contents

    @staticmethod
    def get_channels_in_range(channel_map):
        """Determine the maximum number of channels available."""

        logging.info("Calculating channels count.")
    
        try:
            channel_list = CFUNC_hdhomerun_channel_list_create(channel_map)
        except:
            logging.exception("Could not create channel-list entity for range "
                              "check.")
            raise

        if not channel_list:
            message = "Could not build channel-list."
            
            logging.error(message)
            raise Exception(message)

        try:
            count = CFUNC_hdhomerun_channel_list_total_count(channel_list)
        except:
            logging.exception("Could not produce channel count.")
            raise
        else:
            return count
        finally:
            try:
                CFUNC_hdhomerun_channel_list_destroy(channel_list)
            except:
                logging.exception("Could not destroy channel-list entity.")
                raise

class HdhrDeviceQuery(object):
    """Device-specific queries."""

    device_str = None
    hd = None

    def __init__(self, device_str):
        self.device_str = device_str

        logging.info("Building device-query object for device [%s]." % 
                     (self.device_str))
    
        try:
            self.hd = HdhrUtility.device_create_from_str(self.device_str)
        except:
            logging.exception("Could not create device entity from string "
                              "[%s]." % (device_str))
            raise

    def __del__(self):
        try:
            CFUNC_hdhomerun_device_destroy(self.hd)
        except:
            logging.exception("Could not destroy device-entity object.")
            raise

    def get_tuner_vstatus(self):
        """Get the current state of the tuner using virtual channels (familiar 
        channel number).
        """

        logging.info("Doing device_get_tuner_vstatus call for device [%s]." % 
                     (self.device_str))

        raw_data = c_char_p()
        vstatus = TYPE_hdhomerun_tuner_vstatus_t()

        try:
            result = CFUNC_hdhomerun_device_get_tuner_vstatus(
                            self.hd, 
                            raw_data, 
                            vstatus
                        )
        except:
            logging.exception("Tuner vstatus failed.")
            raise

        if result != 1:
            message = ("Could not get tuner vstatus (%d)." % (result))
            
            logging.error(message)
            raise Exception(message)

        return (vstatus, raw_data.value)

    def set_tuner_vchannel(self, vchannel):
        """Set the current vchannel (familiar channel numbering)."""
        
        logging.info("Doing device_set_tuner_vstatus call for device [%s] with"
                     " vchannel [%s]." % (self.device_str, vchannel))
        
        try:
            result = CFUNC_hdhomerun_device_set_tuner_vchannel(self.hd, 
                                                               str(vchannel))
        except:
            logging.exception("Could not se vchannel.")
            raise

        if result != 1:
            message = "Failed to set vchannel."
            
            logging.error(message)
            raise Exception(message)
        
    def get_supported(self, prefix=None):
        """Get supported features as a dictionary of lists."""
    
        raw_str = c_char_p()

        logging.info("Doing device_get_supported call for device [%s]." % 
                     (self.device_str))

        try:
            result = CFUNC_hdhomerun_device_get_supported(
                            self.hd, 
                            None, 
                            raw_str
                        )
        except:
            logging.exception("Could not do get-supported request.")
            raise

        if result != 1:
            message = ("Could not get supported features (%d)." % (result))
            
            logging.error(message)
            raise Exception(message)

        raw_rows = raw_str.value.strip().split("\n")
        rows = { }
        for row in raw_rows:
            (key, value) = row.split(': ', 1)
            rows[key] = value.split(' ')

        return rows

    def scan_channels(self, channel_map, found_cb=None, incremental_cb=None):
        """Determine which channels can be locked. This is a long process and 
        the callbacks should be used in order to present progress to the user.
        """

        logging.info("Doing channel scan with map [%s]." % (channel_map))

        logging.debug("Determining range of channel scan.")

        try:
            num_channels = HdhrUtility.get_channels_in_range(channel_map)
        except:
            logging.exception("Could not calculate the maximum number of "
                              "channels to be scanned.")
            raise

        logging.debug("Building channel-scan object.")

        try:
            scan = CFUNC_channelscan_create(self.hd, channel_map)
        except:
            logging.exception("Could not initialize channel-scan object.")
            raise

        if not scan:
            message = "Could not build channel-scan object."
            
            logging.error(message)
            raise Exception(message)

        logging.debug("Doing actual scan.")

        try:
            found = self.__do_scan(scan, num_channels, found_cb, 
                                   incremental_cb)
        except:
            logging.exception("Could not do actual channel scan.")
            raise
        else:
            logging.info("Found programs on (%d) channels." % (len(found)))
            return found
        finally:
            try:
                CFUNC_channelscan_destroy(scan)
            except:
                logging.exception("Could not destroy channel-scan entity.")
                raise

    def __do_scan(self, scan, num_channels, found_cb=None, 
                  incremental_cb=None):
        """Do the actual scan (looping over channel numbers)."""
    
        i = 0
        num_channels = float(num_channels)
        found = []
        while 1:
            result = TYPE_hdhomerun_channelscan_result_t()

            if CFUNC_channelscan_advance(scan.contents, result) != 1:
                break

            elif CFUNC_channelscan_detect(scan.contents, result) == 1:
                if result.program_count > 0:
                    found.append(result)
                
                    if found_cb:
                        logging.info("Invoking 'found' callback.")

                        try:
                            found_cb(result, scan.contents)
                        except:
                            logging.exception("The 'found' callback threw an "
                                              "exception.")
                            raise
            i += 1

            progress = (float(i) / num_channels * 100.0)

            if incremental_cb:
                logging.info("Invoking 'incremental' callback.")
            
                try:
                    incremental_cb(scan.contents, progress)
                except:
                    logging.exception("The 'incremental' callback threw an "
                                      "exception.")
                    raise

            logging.info("Channel scan progress is (%.2f)%%." % (progress))

        return found

    def set_tuner_target(self, target_uri):
        """Start sending video to the given URI."""
# If doesn't work: int hdhomerun_device_stream_start(struct hdhomerun_device_t *hd)

        logging.info("Setting target to [%s]." % (target_uri))

        try:
            result = CFUNC_hdhomerun_device_set_tuner_target(self.hd, 
                                                             target_uri)
        except:
            logging.exception("There was an exception while setting the tuner "
                              "target.")
            raise

        if result != 1:
            message = ("Could not set tuner target to [%s]." % (target))
            
            logging.error(message)
            raise Exception(message)

