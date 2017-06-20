appname := TeamSpeakHasher

CXX := g++
CXXFLAGS := -std=c++11 -Wall -Iinclude

LDLIBS=-lOpenCL -lpthread

srcfiles = sha1.cpp IdentityProgress.cpp TunedParameters.cpp Config.cpp DeviceContext.cpp TSHasherContext.cpp main.cpp
objects := $(patsubst %.cpp, %.o, $(srcfiles))


all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(objects)

dist-clean: clean
	rm -f *~ .depend

include .depend