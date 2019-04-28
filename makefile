CXX = g++
CXXFLAGS = -std=gnu++11 -fopenmp

# These options disable the command line
SDLCOMPILE = -Wl,-subsystem,windows

# SDL compilation flags
SDLFLAGS = -IC:\SDL\i686-w64-mingw32\include -LC:\SDL\i686-w64-mingw32\lib -w -lmingw32 -lSDL2main -lSDL2
# SDLFLAGS = `sdl2-config --cflags --libs`						Why doesn't this work?

# GLM compilation flags
GLMFLAGS = -IC:\glm

# CImg compilation flags
CIMGFLAGS = -IC:\CImg -lgdi32 

# RapidJson compilation flags
RJFLAGS = -IC:\rapidjson\include

OBJS = main.cpp

OBJ_NAME = test

main: main.cpp
	export OMP_NUM_THREADS=4
	$(CXX) $(CXXFLAGS) -o $(OBJ_NAME) $(OBJS) $(SDLFLAGS) $(GLMFLAGS) $(CIMGFLAGS) $(RJFLAGS)

# 	$(CXX) $(CXXFLAGS) $(SDLFLAGS) -o $(OBJ_NAME) $(OBJS)		Why doesn't this work?