# clear && unbuffer make debug 2>&1 | head -n 40
# make debug && valgrind --track-origins=yes ./subparcollider

lintedrender5.cpp: render5.cpp lintcpp.py
	cat render5.cpp | ./lintcpp.py > lintedrender5.cpp

rt: gl4.cpp
	#clang++-14 -march=native -O2 gl4.cpp -o rt -stdlib=libc++ -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include
	g++ -march=native -O2 gl4.cpp -o rt -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

rtdebug: gl4.cpp
	g++ -g3 -DDEBUG gl4.cpp -o rtdebug -lGL -lGLEW -lglut -lGLU -lm -L/usr/local/lib -I/usr/local/include

NEW_SRC := physics.h client.cpp terragen.cpp
CPP_SRC := lintedrender5.cpp sdlwrapper.cpp
SWIFT_SRC := main.swift gptphysics.swift spatialtypes.swift boxoid.swift gamelogic.swift
OBJC_HEADERS := subparcollider-Bridging-Header.h
LIBS := -lglfw -lGL -lGLEW -lm -lSDL2
NEW_LIBS := -lGL -lGLEW -lglut -lGLU -lm -lglfw -lbacktrace libFastNoise.a
LIBDIR := 
#LIBDIR := -L/usr/local/lib
INCDIR := -I/usr/local/include -IFastNoise2/include
CXXFLAGS := -O2 -march=native
#TAKEOFF_FLAGS := -std=c++20 -O2 -march=native -pthread
TAKEOFF_FLAGS := -std=c++23 -O3 -march=native -pthread -Wno-deprecated
TAKEOFF_DEBUG_FLAGS := -std=c++23 -ggdb3 -D DEBUG -march=native -pthread
COMPILER := clang++-18
#COMPILER := g++
COMPILER_DEBUG := g++

SHA-Intrinsics/sha256-x86.c:
	git clone https://github.com/noloader/SHA-Intrinsics.git

FastNoise2/include/FastNoise/FastNoise.h:
	git clone https://github.com/Auburn/FastNoise2.git

terragen.o: terragen.cpp FastNoise2/include/FastNoise/FastNoise.h
	$(COMPILER) $(TAKEOFF_FLAGS) -pg -c terragen.cpp -o terragen.o $(LIBDIR) $(INCDIR)

sha256.o: SHA-Intrinsics/sha256-x86.c
	$(COMPILER) $(TAKEOFF_FLAGS) -pg -c SHA-Intrinsics/sha256-x86.c -o sha256.o

sha256_debug.o: SHA-Intrinsics/sha256.c
	$(COMPILER_DEBUG) $(TAKEOFF_DEBUG_FLAGS) -c SHA-Intrinsics/sha256.c -o sha256_debug.o

terragen_debug.o: terragen.cpp
	$(COMPILER_DEBUG) $(TAKEOFF_DEBUG_FLAGS) -c terragen.cpp -o terragen_debug.o $(LIBDIR) $(INCDIR)

takeoff: $(NEW_SRC) terragen.o sha256.o
	$(COMPILER) $(TAKEOFF_FLAGS) -pg terragen.o sha256.o client.cpp -o takeoff $(LIBDIR) $(INCDIR) $(NEW_LIBS)

takeoff_debug: $(NEW_SRC) terragen_debug.o sha256_debug.o
	$(COMPILER_DEBUG) $(TAKEOFF_DEBUG_FLAGS) terragen_debug.o sha256_debug.o client.cpp -o takeoff_debug $(LIBDIR) $(INCDIR) $(NEW_LIBS)

#takeoff_debug: $(NEW_SRC)
#	$(COMPILER) -g3 -D DEBUG -march=native -c $(NEW_SRC) $(NEW_LIBS)
#	$(COMPILER) -g3 -D DEBUG -march=native $(NEW_SRC:.cpp=.o) -o takeoff_debug $(LIBDIR) $(INCDIR) $(NEW_LIBS)

takeoff_release: $(NEW_SRC)
	rm *.o
	$(COMPILER) -O2 -flto -march=native -c $(NEW_SRC) $(NEW_LIBS)
	$(COMPILER) -O2 -flto -march=native $(NEW_SRC:.cpp=.o) -o takeoff_release $(LIBDIR) $(INCDIR) $(NEW_LIBS)

old_optimized: $(CPP_SRC) $(SWIFT_SRC)
	g++ -O3 -march=native -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -O -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

old_debug: $(CPP_SRC) $(SWIFT_SRC)
	g++ -g -c $(CPP_SRC) -fpermissive -fPIC $(LIBS)
	swiftc -j8 -g -D DEBUG -import-objc-header $(OBJC_HEADERS) $(SWIFT_SRC) $(CPP_SRC:.cpp=.o) -o subparcollider $(LIBDIR) $(INCDIR) $(LIBS)

test_uid_profile: test_uid.cpp uid.cpp uid.h sha256.o
	$(COMPILER) $(TAKEOFF_FLAGS) -pg test_uid.cpp terragen.o sha256.o libFastNoise.a -o test_uid -fPIC $(LIBDIR) $(INCDIR)
	time ./test_uid && gprof --brief test_uid; rm test_uid

test_uid: test_uid.cpp uid.cpp uid.h sha256.o
	$(COMPILER_DEBUG) $(TAKEOFF_DEBUG_FLAGS) slow_test_uid.cpp terragen.o sha256.o libFastNoise.a -o test_uid -fPIC $(LIBDIR) $(INCDIR)
	time ./test_uid; rm test_uid

test_uid_valgrind: test_uid.cpp uid.cpp uid.h sha256.o
	$(COMPILER_DEBUG) $(TAKEOFF_DEBUG_FLAGS) test_uid.cpp terragen.o sha256.o libFastNoise.a -o test_uid -fPIC $(LIBDIR) $(INCDIR)
	valgrind --track-origins=yes ./test_uid; rm test_uid

quick: takeoff
debug: takeoff_debug
release: takeoff_release
old: old_optimized
test: test_uid
testprof: test_uid_profile
testvalgrind: test_uid_valgrind

.PHONY: quick debug release test

.DEFAULT_GOAL := quick

clean:
	rm -f *.o lintedrender5.cpp subparcollider rt rtdebug takeoff takeoff_debug takeoff_release test_uid

