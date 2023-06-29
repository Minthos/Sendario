#!/bin/bash

# optimized build
#g++ -O3 -c render5.cpp -o renderer.o -fPIC -lglfw -lGL -lGLEW -lm && swiftc -O -import-objc-header subparcollider-Bridging-Header.h main.swift gptphysics.swift renderer.o -o main -L/usr/local/lib -I/usr/local/include -lGLEW -lglfw -lGL -lm

# ./build5.sh && valgrind --track-origins=yes ./main
# debug build
g++ -g -c render5.cpp sdlwrapper.cpp -fpermissive -fPIC -lglfw -lGL -lGLEW -lm -lSDL2 && swiftc -g -D DEBUG -import-objc-header subparcollider-Bridging-Header.h main.swift gptphysics.swift render5.o sdlwrapper.o -o main -L/usr/local/lib -I/usr/local/include -lGLEW -lglfw -lGL -lm -lSDL2
