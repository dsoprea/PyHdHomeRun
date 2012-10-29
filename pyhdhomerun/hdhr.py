import logging

from ctypes import *

from pyhdhomerun import constants

def get_hdhr():
    """Get an instance of the library."""

    try:
        return get_hdhr.instance
    except:
        pass

    try:
        get_hdhr.instance = cdll.LoadLibrary(constants.HDHR_FILEPATH)
    except:
        logging.exception("Could not load HDHR library from [%s]." % 
                          (constants.HDHR_FILEPATH))
        raise

    return get_hdhr.instance

