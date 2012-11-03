from setuptools import setup, find_packages
import sys, os

version = '2.2.3'

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
      zip_safe=False,
      install_requires=[
          'setuptools'
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
