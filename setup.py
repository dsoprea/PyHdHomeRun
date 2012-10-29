from setuptools import setup, find_packages
import sys, os

version = '2.1'

setup(name='pyhdhomerun',
      version=version,
      description="HDHomeRun interface library.",
      long_description="""\
HDHomeRun interface library. Supports device discovery, channel-scanning, streaming, status inquiries, channel changes, etc..""",
      classifiers=[], # Get strings from http://pypi.python.org/pypi?%3Aaction=list_classifiers
      keywords='tv television tuner tvtuner hdhomerun',
      author='Dustin Oprea',
      author_email='myselfasunder@gmail.com',
      url='https://github.com/dsoprea/PyHdHomeRun',
      license='New BSD',
      packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
      include_package_data=True,
      zip_safe=False,
      install_requires=[
          # -*- Extra requirements: -*-
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
