#ifndef mesh_h
#define mesh_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "globals.h"
#include "physfs.h"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <array>
#include <string>

class vec3 {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  vec3(int fx, int fy, int fz);
};

//a 3d vertex
class vertex3d {
public:
  float x = 0;
  float y = 0;
  float z = 0;
  SDL_Color color = {0,255,0,255};
  float u = 0;
  float v = 0;

  float lu;
  float lv;

  array<float, 3> normal = {0,0,0};
};

class face {
public:
  unsigned int a = 0;
  unsigned int b = 0;
  unsigned int c = 0;
};

enum meshtype {
  FLOOR,
  COLLISION,
  LIGHTING,
};

class mesh {
public:
  mesh* lighting = nullptr;
  vec3 origin = {0,0,0};
  SDL_Texture* texture = NULL;

  SDL_Vertex* vertex = NULL;
  int numVertices = 0;

  float sleepRadius = 0;

  vector<face> faces; //for 3d data, for determining z of entities ontop.
  vector<vertex3d> vertices;

  meshtype mtype = meshtype::FLOOR;

  bool visible = 1;

  mesh();

  ~mesh();
};

mesh* loadMeshFromPly(string faddress, vec3 forigin, float scale, meshtype fmtype);

#endif
