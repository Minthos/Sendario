#!/bin/bash
g++ -c render5.cpp -o renderer.o -fPIC -lglfw -lGL -lGLEW -lm && swiftc -import-objc-header subparcollider-Bridging-Header.h main.swift gptphysics.swift renderer.o -o main -L/usr/local/lib -I/usr/local/include -lGLEW -lglfw -lGL -lm
