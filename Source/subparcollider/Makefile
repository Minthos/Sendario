# clear && unbuffer make debug 2>&1 | head -n 40
# make debug && valgrind --track-origins=yes ./subparcollider

lintedrender5.cpp: render5.cpp lintcpp.py
	cat render5.cpp | ./lintcpp.py > lintedrender5.cpp

rt: gl4.cpp
	#clang++-14 -march=native -O2 gl4.cpp -o rt -stdlib=libc++ -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include
	g++ -march=native -O2 gl4.cpp -o rt -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

rtdebug: gl4.cpp
	g++ -g3 -DDEBUG gl4.cpp -o rtdebug -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

NEW_SRC := physics.cpp client.cpp
CPP_SRC := lintedrender5.cpp sdlwrapper.cpp
SWIFT_SRC := main.swift gptphysics.swift spatialtypes.swift boxoid.swift gamelogic.swift
OBJC_HEADERS := subparcollider-Bridging-Header.h
LIBS := -lglfw -lGL -lGLEW -lm -lSDL2
NEW_LIBS := -lGL -lGLEW -lglut -lGLU -lm -lglfw libFastNoise.a
LIBDIR := -L/usr/local/lib
INCDIR := -I/usr/local/include -IFastNoise2/include
CXXFLAGS := -O2 -march=native


takeoff: $(NEW_SRC)
	g++ -O2 -march=native client.cpp -o takeoff $(LIBDIR) $(INCDIR) $(NEW_LIBS)

takeoff_debug: $(NEW_SRC)
	g++ -g3 -D DEBUG -march=native client.cpp -o takeoff_debug $(LIBDIR) $(INCDIR) $(NEW_LIBS)

#takeoff_debug: $(NEW_SRC)
#	g++ -g3 -D DEBUG -march=native -c $(NEW_SRC) $(NEW_LIBS)
#	g++ -g3 -D DEBUG -march=native $(NEW_SRC:.cpp=.o) -o takeoff_debug $(LIBDIR) $(INCDIR) $(NEW_LIBS)

takeoff_release: $(NEW_SRC)
	rm *.o
	g++ -O2 -flto -march=native -c $(NEW_SRC) $(NEW_LIBS)
	g++ -O2 -flto -march=native $(NEW_SRC:.cpp=.o) -o takeoff_release $(LIBDIR) $(INCDIR) $(NEW_LIBS)

old_optimized: $(CPP_SRC) $(SWIFT_SRC)
	g++ -O3 -march=native -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -O -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

old_debug: $(CPP_SRC) $(SWIFT_SRC)
	g++ -g -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -g -D DEBUG -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

quick: takeoff
debug: takeoff_debug
release: takeoff_release
old: old_optimized

.PHONY: quick debug release

.DEFAULT_GOAL := quick

clean:
	rm *.o lintedrender5.cpp subparcollider rt rtdebug takeoff takeoff_debug takeoff_release

