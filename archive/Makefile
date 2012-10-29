all: module

module: setup.py hdhr.c
	CFLAGS="${CFLAGS} -lhdhomerun" python setup.py build_ext --inplace

clean:
	rm -fr hdhr.so


