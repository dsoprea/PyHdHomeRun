from setuptools import setup, find_packages
import sys, os

from pyhdhomerun.hdhr import get_hdhr

try:
    get_hdhr()
except OSError as e:
    print("Could not load HDHomeRun library: %s" % (e))
    sys.exit(1)
else:
    print("HDHomeRun libraries verified.")
    
version = '2.3.3'

setup(name='pyhdhomerun',
      version=version,
      description="HDHomeRun interface library.",
      long_description="""\
HDHomeRun interface library. Supports device discovery, channel-scanning, streaming, status inquiries, channel changes, etc..""",
      classifiers=['Development Status :: 4 - Beta',
                   'License :: OSI Approved :: BSD License',
                   'Natural Language :: English',
                   'Programming Language :: Python :: 2.7',
                   'Topic :: Software Development :: Libraries :: Python Modules',
                   'Topic :: Multimedia :: Video :: Capture'
                  ],
      keywords='tv television tuner tvtuner hdhomerun',
      author='Dustin Oprea',
      author_email='myselfasunder@gmail.com',
      url='https://github.com/dsoprea/PyHdHomeRun',
      license='New BSD',
      packages=['pyhdhomerun'],
      include_package_data=True,
      zip_safe=True,
      install_requires=[
          'setuptools',
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
