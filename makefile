CXX = g++
CXXFLAGS = -std=c++11
SDLFLAGS = -IC:\SDL\i686-w64-mingw32\include -LC:\SDL\i686-w64-mingw32\lib -w -Wl,-subsystem,windows -lmingw32 -lSDL2main -lSDL2
# SDLFLAGS = `sdl2-config --cflags --libs`						Why doesn't this work?

OBJS = main.cpp

OBJ_NAME = test

main: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OBJ_NAME) $(OBJS) $(SDLFLAGS)

# 	$(CXX) $(CXXFLAGS) $(SDLFLAGS) -o $(OBJ_NAME) $(OBJS)		Why doesn't this work?