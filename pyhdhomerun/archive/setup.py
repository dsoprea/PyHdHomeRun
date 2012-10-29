from distutils.core import setup, Extension

setup(
    ext_modules=[Extension("hdhr", ["hdhr.c"])],
    include_dirs=['/usr/lib/libhdhomerun']
)

