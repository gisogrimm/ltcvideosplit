BINFILES = ltcvideosplit

EXTERNALS += libavutil libavformat libavcodec libswscale

CXXFLAGS += -fPIC -Wall -msse -msse2 -mfpmath=sse -ffast-math	\
-fomit-frame-pointer -fno-finite-math-only -L./

all:
	mkdir -p build
	$(MAKE) -C build -f ../Makefile $(BINFILES)

VPATH = ../src

include $(wildcard *.mk)

%: %.o
	$(CXX) $(CXXFLAGS) $(LDLIBS) $^ -o $@

%.o: %.cc
	$(CPP) $(CPPFLAGS) -MM -MF $(@:.o=.mk) $<
	$(CXX) $(CXXFLAGS) -c $< -o $@



CXXFLAGS += `pkg-config --cflags $(EXTERNALS)`
LDLIBS += `pkg-config --libs $(EXTERNALS)`

