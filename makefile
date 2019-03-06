CXX = g++
CXXFLAGS = -std=c++11

# These options disable the command line
SDLCOMPILE = -Wl,-subsystem,windows

# SDL compilation flags
SDLFLAGS = -IC:\SDL\i686-w64-mingw32\include -LC:\SDL\i686-w64-mingw32\lib -w -lmingw32 -lSDL2main -lSDL2

# GLM compilation flags
GLMFLAGS = -IC:\glm
# SDLFLAGS = `sdl2-config --cflags --libs`						Why doesn't this work?

OBJS = main.cpp

OBJ_NAME = test

main: main.cpp
	$(CXX) $(CXXFLAGS) -o $(OBJ_NAME) $(OBJS) $(SDLFLAGS) $(GLMFLAGS)

# 	$(CXX) $(CXXFLAGS) $(SDLFLAGS) -o $(OBJ_NAME) $(OBJS)		Why doesn't this work?