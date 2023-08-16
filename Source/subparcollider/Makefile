# clear && unbuffer make debug 2>&1 | head -n 40
# make debug && valgrind --track-origins=yes ./subparcollider

lintedrender5.cpp: render5.cpp lintcpp.py
	cat render5.cpp | ./lintcpp.py > lintedrender5.cpp

rt: gl4.cpp
	g++ -O2 gl4.cpp -o rt -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

rtdebug: gl4.cpp
	g++ -g3 -DDEBUG gl4.cpp -o rtdebug -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

CPP_SRC := lintedrender5.cpp sdlwrapper.cpp
SWIFT_SRC := main.swift gptphysics.swift spatialtypes.swift boxoid.swift gamelogic.swift
OBJC_HEADERS := subparcollider-Bridging-Header.h
LIBS := -lglfw -lGL -lGLEW -lm -lSDL2
LIBDIR := -L/usr/local/lib
INCDIR := -I/usr/local/include

main_optimized: $(CPP_SRC) $(SWIFT_SRC)
	g++ -O3 -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -O -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

main_debug: $(CPP_SRC) $(SWIFT_SRC)
	g++ -g -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -g -D DEBUG -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

optimized: main_optimized
debug: main_debug

.PHONY: optimized debug

.DEFAULT_GOAL := optimized

clean:
	rm *.o lintedrender5.cpp subparcollider rt rtdebug

