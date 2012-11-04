import logging

from os import environ
from getpass import getuser

import pyhdhomerun
app_path = dirname(pyhdhomerun.__file__)

log_paths = [('%s/logs' % (app_path)), 
             '/var/log/pyhdhr', 
            ]

log_filename = 'pyhdhr.log'

format = '%(asctime)s  %(levelname)s %(message)s'

for log_path in log_paths:
    if exists(log_path):
        logging.basicConfig(
                level       = logging.DEBUG,
                format      = format,
                filename    = ('%s/%s' % (log_path, log_filename))
            )

        break

# Hook console logging.
#if ('HDHR_DEBUG' in environ and environ['HDHR_DEBUG'] or
#    'RI_DEBUG' in environ and environ['RI_DEBUG']
#   ) and 'RI_CONSOLE_LOG_ACTIVE' not in environ:
#    log_console = logging.StreamHandler()
#    log_console.setLevel(logging.DEBUG)
#    log_console.setFormatter(logging.Formatter(format))
#
#    logging.getLogger('').addHandler(log_console)
#
#    environ['RI_CONSOLE_LOG_ACTIVE'] = '1'

