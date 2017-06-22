LIB_FLAGS = -L/usr/local/Cellar/boost/1.64.0_1/lib
CPP_FLAGS = -I/usr/local/Cellar/boost/1.64.0_1/include
ALL_OBJECTS = src/main.o

all: $(ALL_OBJECTS)
	g++ -o kepler $(ALL_OBJECTS) $(LIB_FLAGS) -lboost_locale-mt

src/main.o: src/main.cpp
	g++ -o src/main.o -c src/main.cpp -W -Wall -ansi $(CPP_FLAGS)

clean:
	rm -rf src/*.o kepler
