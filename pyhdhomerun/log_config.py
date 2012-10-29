import logging

from getpass import getuser

current_user = getuser()

if current_user == 'root':
    log_filepath = 'hdhomerun.log'

else:
    log_filepath = 'hdhomerun.log'

format = '%(asctime)s  %(levelname)s %(message)s'

logging.basicConfig(
        level       = logging.DEBUG,
        format      = format,
        filename    = log_filepath
    )

# Hook console logging.
log_console = logging.StreamHandler()
log_console.setLevel(logging.DEBUG)
log_console.setFormatter(logging.Formatter(format))

logging.getLogger('').addHandler(log_console)

