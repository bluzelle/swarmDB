LIB_FLAGS =
ALL_OBJECTS = src/main.o

all: $(ALL_OBJECTS)
	g++ -o kepler $(ALL_OBJECTS) $(LIB_FLAGS)

clean:
	rm -rf *.o kepler
