# Define default flags (include your source tree for example

#CC = gcc-7
CXX       := g++-7


CXXFLAGS = `wx-config --cxxflags` -I/usr/local/Cellar/boost/1.64.0_1/include
LDLIBS   = `wx-config --libs` -L/usr/local/Cellar/boost/1.64.0_1/lib -lboost_locale-mt

ifdef FINAL
    EXTRAFLAGS = -MD -O2 -fno-rtti -fno-exceptions -fomit-frame-pointer
else
    EXTRAFLAGS = -MD -g
endif

# Define our sources, calculate the dependecy files and object files

KEPLER_SOURCES := src/main.cpp 
KEPLER_OBJECTS := $(patsubst %.cpp, %.o, ${KEPLER_SOURCES})
KEPLER_DEPS := $(patsubst %.cpp, %.d, ${KEPLER_SOURCES})

#include our project dependecy files

-include $(KEPLER_DEPS)

all: kepler

kepler: $(KEPLER_OBJECTS)
	$(CXX) -o kepler $(KEPLER_OBJECTS) $(LDLIBS)
ifdef FINAL
	strip kepler
endif

clean:
	rm -rf kepler.exe kepler $(KEPLER_OBJECTS) $(KEPLER_DEPS)