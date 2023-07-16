#!/bin/bash

cat render5.cpp | ./lintcpp.py > lintedrender5.cpp

# optimized build
#g++ -O3 -c render5.cpp sdlwrapper.cpp -fpermissive -fPIC -lglfw -lGL -lGLEW -lm -lSDL2 && swiftc -O -import-objc-header subparcollider-Bridging-Header.h main.swift gptphysics.swift render5.o sdlwrapper.o -o main -L/usr/local/lib -I/usr/local/include -lGLEW -lglfw -lGL -lm -lSDL2

# ./build5.sh && valgrind --track-origins=yes ./main
# debug build
g++ -g -c lintedrender5.cpp sdlwrapper.cpp -fpermissive -fPIC -lglfw -lGL -lGLEW -lm -lSDL2 && swiftc -g -D DEBUG -import-objc-header subparcollider-Bridging-Header.h main.swift gptphysics.swift lintedrender5.o sdlwrapper.o -o main -L/usr/local/lib -I/usr/local/include -lGLEW -lglfw -lGL -lm -lSDL2
