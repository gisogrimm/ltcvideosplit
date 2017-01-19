BINFILES = ltcvideosplit sndfile-bcastinfo
OBJECTS = error.o writevideo.o

EXTERNALS += libavutil libavformat libavcodec ltc

CXXFLAGS += -fPIC -Wall -msse -msse2 -mfpmath=sse -ffast-math	\
-fomit-frame-pointer -fno-finite-math-only -L./

VERSION = $(shell cat ../version)
VERSION_MAJOR = $(shell cat ../version|sed -e 's/\..*//1')
VERSION_MINOR = $(shell cat ../version|sed -e 's/[^\.]*\.//1')
CXXFLAGS += -DVERSION_MAJOR=$(VERSION_MAJOR) -DVERSION_MINOR=$(VERSION_MINOR)

CXXFLAGS += -g

all:
	mkdir -p build
	$(MAKE) -C build -f ../Makefile $(BINFILES)

install:
	$(MAKE) all
	(cd build && cp $(BINFILES) /usr/local/bin)

VPATH = ../src

.PHONY : clean

include $(wildcard *.mk)

%: %.o
	$(CXX) $(CXXFLAGS) $(LDLIBS) $^ -o $@

%.o: %.cc
	$(CPP) $(CPPFLAGS) -MM -MF $(@:.o=.mk) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BINFILES): $(OBJECTS)

clean:
	rm -Rf build

CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += `pkg-config --libs $(EXTERNALS)`

sndfile-bcastinfo: EXTERNALS+=sndfile
