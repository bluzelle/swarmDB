# On OSX, if you want static WX Widgets, install it with: brew install wxmac --with-static
# Once installed, this makefile will automatically link against the static WX Widgets libraries

# Idiotic but it seems not obvious how to get this to statically link. wx was re-installed only to have static libraries, so that
# part linked statically but all its damn dependencies are still dynamic. How do you tell the linked ONLY to use the static version of
# some library it is referring to?

# Here is the MANUAL command to do that on OSX. Use carefully as it might need to get tweaked.

# c++ -o kepler  src/main.o -std=c++11 -L/usr/local/lib   -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL /usr/local/lib/libwx_osx_cocoau_xrc-3.0.a /usr/local/lib/libwx_osx_cocoau_webview-3.0.a /usr/local/lib/libwx_osx_cocoau_qa-3.0.a /usr/local/lib/libwx_baseu_net-3.0.a /usr/local/lib/libwx_osx_cocoau_html-3.0.a /usr/local/lib/libwx_osx_cocoau_adv-3.0.a /usr/local/lib/libwx_osx_cocoau_core-3.0.a /usr/local/lib/libwx_baseu_xml-3.0.a /usr/local/lib/libwx_baseu-3.0.a /usr/local/opt/libpng/lib/libpng16.a /usr/local/opt/jpeg/lib/libjpeg.a /usr/local/opt/libtiff/lib/libtiff.a -framework WebKit -lexpat -lwxregexu-3.0 -lz -lpthread -liconv -L/usr/local/Cellar/boost/1.64.0_1/lib /usr/local/opt/boost/lib/libboost_locale-mt.a /usr/local/opt/boost/lib/libboost_system.a

# Define default flags (include your source tree for example

CXXFLAGS = -std=c++14 `wx-config --cxxflags` -I/usr/local/Cellar/boost/1.64.0_1/include
LDLIBS   = -std=c++14 `wx-config --libs` -L/usr/local/Cellar/boost/1.64.0_1/lib -lboost_locale-mt

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

# Here is the MANUAL command to do that on OSX. Use carefully as it might need to get tweaked. It requires the program to have been successfully built the dynamic way first
# so that it has access to the compiled .o file.

# c++ -o kepler  src/main.o -std=c++11 -L/usr/local/lib   -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL /usr/local/lib/libwx_osx_cocoau_xrc-3.0.a /usr/local/lib/libwx_osx_cocoau_webview-3.0.a /usr/local/lib/libwx_osx_cocoau_qa-3.0.a /usr/local/lib/libwx_baseu_net-3.0.a /usr/local/lib/libwx_osx_cocoau_html-3.0.a /usr/local/lib/libwx_osx_cocoau_adv-3.0.a /usr/local/lib/libwx_osx_cocoau_core-3.0.a /usr/local/lib/libwx_baseu_xml-3.0.a /usr/local/lib/libwx_baseu-3.0.a /usr/local/opt/libpng/lib/libpng16.a /usr/local/opt/jpeg/lib/libjpeg.a /usr/local/opt/libtiff/lib/libtiff.a -framework WebKit -lexpat -lwxregexu-3.0 -lz -lpthread -liconv -L/usr/local/Cellar/boost/1.64.0_1/lib /usr/local/opt/boost/lib/libboost_locale-mt.a /usr/local/opt/boost/lib/libboost_system.a

hack_static: 
	c++ -o kepler_static src/main.o -std=c++14 -L/usr/local/lib   -framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL /usr/local/lib/libwx_osx_cocoau_xrc-3.0.a /usr/local/lib/libwx_osx_cocoau_webview-3.0.a /usr/local/lib/libwx_osx_cocoau_qa-3.0.a /usr/local/lib/libwx_baseu_net-3.0.a /usr/local/lib/libwx_osx_cocoau_html-3.0.a /usr/local/lib/libwx_osx_cocoau_adv-3.0.a /usr/local/lib/libwx_osx_cocoau_core-3.0.a /usr/local/lib/libwx_baseu_xml-3.0.a /usr/local/lib/libwx_baseu-3.0.a /usr/local/opt/libpng/lib/libpng16.a /usr/local/opt/jpeg/lib/libjpeg.a /usr/local/opt/libtiff/lib/libtiff.a -framework WebKit -lexpat -lwxregexu-3.0 -lz -lpthread -liconv -L/usr/local/Cellar/boost/1.64.0_1/lib /usr/local/opt/boost/lib/libboost_locale-mt.a /usr/local/opt/boost/lib/libboost_system.a

clean:
	rm -rf kepler.exe kepler kepler_static $(KEPLER_OBJECTS) $(KEPLER_DEPS)