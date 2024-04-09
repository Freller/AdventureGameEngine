#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <cctype>
#include <limits>
#include <stdlib.h>

#include <filesystem> //checking if a usable dir exists

#include "globals.h"
#include "lightcookies.h"
#include "objects.h"
#include "map_editor.h"
#include "specialobjects.h"
#include "utils.h"

#include <utility>

#define PI 3.14159265

using namespace std;

class usable;

navNode* getNodeByPos(vector<navNode*> array, int x, int y) {
  float min_dist = 0;
  navNode* ret = nullptr;
  bool flag = 1;

  //todo check for boxs
  if(array.size() == 0) {return nullptr;}
  for (long long unsigned int i = 0; i < array.size(); i++) {
    float dist = Distance(x, y, array[i]->x, array[i]->y);
    if(dist < min_dist || flag) {
      min_dist = dist;
      ret = array[i];
      flag = 0;
    }
  }
  return ret;
}


/*
 *         pi/2 up
 *
 *
 *   pi left  *      0 right
 *    
 *
 *         3pi/4 down
 */
//returns the direction to turn from angle a to angle b
bool getTurningDirection(float a, float b) {
  b -= a;
  b = wrapAngle(b);
  if(b < M_PI) {
    //turn counterclockwise (positive)
    return 1;
  } else {
    return 0;
  }
}

float angleMod(float a, float n) {
  return a - floor(a/n) * n;
}

//get difference between angles
float angleDiff(float a, float b) {
  float c = b - a;
  c = angleMod((c + M_PI) , (M_PI * 2)) - (M_PI);
  return c;
}

void parseScriptForLabels(vector<string> &sayings) {
  //parse sayings for lables
  vector<pair<string, int>> symboltable;
  
  for(int i = 0; i < (int)sayings.size(); i++) {
    if(sayings[i][0] == '<') {
      pair<string, int> pushMeBack{ sayings[i].substr(1,sayings[i].length() - 2), i };
      symboltable.push_back(pushMeBack);
      continue;
    }
  }


  for(int i = 0; i < (int)sayings.size(); i++) {
    int pos = sayings[i].find(":");
    if(pos != (int)string::npos) {
      if(sayings[i][0] == '`') {break;} //this is a ':' in a bit of dialog or something
      bool good = 0;
      for(auto y: symboltable) {
        if(sayings[i].substr(pos+1, sayings[i].length()-(pos+1)) == y.first) {
          sayings[i].replace(pos, y.first.length() + 1, ":" + to_string(y.second + 3) );
          good = 1;
        }
      }
      if(!good) {
        E("Couldn't match labels from symboltable.");
        M(sayings[i]);
        quit = 1;
        break;
      }
    }
  }

}


heightmap::heightmap(string fname, string fbinding, float fmagnitude) {
  image = loadSurface(fbinding.c_str());
  name = fname;
  binding = fbinding;

  magnitude = fmagnitude;
  g_heightmaps.push_back(this);
}

heightmap::~heightmap() {
  SDL_FreeSurface(image);
  g_heightmaps.erase(remove(g_heightmaps.begin(), g_heightmaps.end(), this), g_heightmaps.end());
}

Uint32 heightmap::getpixel(SDL_Surface *surface, int x, int y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
    case 1:
      return *p;
      //break;

    case 2:
      return *(Uint16 *)p;
      //break;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      //break;

    case 4:
      return *(Uint32 *)p;
      //break;

    default:
      return 0;
  }
}


navNode::navNode(int fx, int fy, int fz) {
  //M("navNode()" );
  x = fx;
  y = fy;
  z = fz;
  g_navNodes.push_back(this);
  pair<int, int> pos;
  pos.first = fx; pos.second = fy;
  navNodeMap[pos] = this;
}

void navNode::Add_Friend(navNode* newFriend) {
  friends.push_back(newFriend);
  float cost = pow(pow((newFriend->x - this->x), 2) + pow((newFriend->y - this->y), 2), 0.5);
  costs.push_back(cost);
}

void navNode::Update_Costs() {
  for (int i = 0; i < (int)friends.size(); i++) {
    costs[i] = XYWorldDistance(x, y, friends[i]->x, friends[i]->y);
  }
}

void navNode::Render(int red, int green, int blue) {
  SDL_Rect obj = {(int)((this->x -g_camera.x - 20)* g_camera.zoom) , (int)(((this->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
  SDL_SetTextureColorMod(nodeDebug, red, green, blue);
  SDL_RenderCopy(renderer, nodeDebug, NULL, &obj);
}

navNode::~navNode() {
  //M("~navNode()");
  for (auto x : friends) {
    x->friends.erase(remove(x->friends.begin(), x->friends.end(), this), x->friends.end());
  }
  //M("got here");
  //changed if to while to try and solve a bug
  // !!! it's a crutch, find a way to remove it later
  while(count(g_navNodes.begin(), g_navNodes.end(), this)) {
    g_navNodes.erase(remove(g_navNodes.begin(), g_navNodes.end(), this), g_navNodes.end());
  }
}


navNode* getNodeByPosition(int fx, int fy) {
  //narrow down our search signifigantly

  //auto p = navNodeMap.equal_range(make_pair(fx, fy));
  //think of the below numbers as the maximum distance, in worldpixels
  //that the player, or any target-entity can be from the proper node
  //that they really are closest to.
  //it has to be "caught" in this "search" for a lower bound
  //otherwise, the entity will go somewhere diagonal that was caught
  int MaxDistanceFromNode = 3;
  auto lowerbound = navNodeMap.lower_bound(make_pair(fx - (64 * MaxDistanceFromNode), fy-(45 * MaxDistanceFromNode)));
  auto upperbound = navNodeMap.upper_bound(make_pair(fx+(64 * MaxDistanceFromNode), fy+(45 * MaxDistanceFromNode)));
  //return lowerbound->second;
  float min_dist = 0;
  navNode* ret = lowerbound->second;
  bool flag = 1;

  for(auto q = lowerbound; q != upperbound; q++) {
    //this is segfaulting, q->second->x ,y are not valid memory locations
    float dist = Distance(fx, fy, q->second->x, q->second->y);
    if( (dist < min_dist || flag) && q->second->enabled) {
      min_dist = dist;
      ret = q->second;
      flag = 0;
    }
  }

  //if every close node was disabled, I guess just take one of the disabled one :S
  if(flag) {
    for(auto q = lowerbound; q != upperbound; q++) {
      float dist = Distance(fx, fy, q->second->x, q->second->y);
      if( (dist < min_dist || flag) /*&& q->second->enabled*/) {
        min_dist = dist;
        ret = q->second;
        flag = 0;
      }
    }
  }

  return ret;
}

void RecursiveNavNodeDelete(navNode* a) {
  //copy friends array to new vector
  vector<navNode*> buffer;
  for (auto f : a->friends) {
    buffer.push_back(f);
  }

  //navNode* b = a->friends[0];
  delete a;
  for(auto f : buffer) {
    if(count(g_navNodes.begin(), g_navNodes.end(), f)) {
      RecursiveNavNodeDelete(f);
    }
  }
}

void Update_NavNode_Costs(vector<navNode*> fnodes) {
  //M("Update_NavNode_Costs()" );
  for (int i = 0; i < (int)fnodes.size(); i++) {
    fnodes[i]->Update_Costs();
  }
}


rect::rect() {
  x=0;
  y=0;
  width=45;
  height=45;
}

rect::rect(int a, int b, int c, int d) {
  x=a;
  y=b;
  width=c;
  height=d;
}

rect::rect(int fx, int fy, int fz, int fw, int fh, int fzh) {
  x=fx;
  y=fy;
  width=fw;
  height=fh;
  z = fz;
  zeight = fzh;
}

void rect::render(SDL_Renderer * renderer) {
  SDL_Rect rect = { this->x, this->y, this->width, this->height};
  SDL_RenderFillRect(renderer, &rect);
}

pointOfInterest::pointOfInterest(int fx, int fy, int findex) : x(fx), y(fy), index(findex) {
  if(findex > g_numberOfInterestSets) {index = 0;}
  g_setsOfInterest.at(index).push_back(this);

}

//probably best to not call this when unloading a level
pointOfInterest::~pointOfInterest() {
  g_setsOfInterest.at(index).erase(remove(g_setsOfInterest.at(index).begin(), g_setsOfInterest.at(index).end(), this), g_setsOfInterest.at(index).end());
}

mapCollision::mapCollision() {
  g_mapCollisions.push_back(this);
}

//copy constructor
mapCollision::mapCollision(const mapCollision & other) {
  this->walltexture = other.walltexture;
  this->captexture = other.captexture;
  this->capped = other.capped;
  this->children = other.children;
  g_mapCollisions.push_back(this);
}

//copy assignment
mapCollision& mapCollision::operator=(const mapCollision &other) {
  mapCollision*a;
  a->walltexture = other.walltexture;
  a->captexture = other.captexture;
  a->capped = other.capped;
  a->children = other.children;
  g_mapCollisions.push_back(a);
  return *a;
}

mapCollision::~mapCollision() {
  g_mapCollisions.erase(remove(g_mapCollisions.begin(), g_mapCollisions.end(), this), g_mapCollisions.end());
}

tri::tri(int fx1, int fy1, int fx2, int fy2, int flayer, string fwallt, string fcapt, bool fcapped, bool fshaded, int fstyle) {
  //M("tri()");
  x1=fx1; y1=fy1;
  x2=fx2; y2=fy2;
  layer = flayer;
  shaded = fshaded;
  style = fstyle;
  if(x2 < x1 && y2 > y1) {
    type = 0; //  :'
  }
  if(x2 < x1 && y2 < y1) {
    type = 1; //  :,
  }
  if(x2 > x1 && y2 < y1) {
    type = 2; //  ,:
  }
  if(x2 > x1 && y2 > y1) {
    type = 3; //  ':
  }
  m = float(y1 -y2) / float(x1 - x2);
  b = y1 - (m * x1);
  walltexture = fwallt;
  captexture = fcapt;
  capped = fcapped;

  x = min(x1, x2);
  y = min(y1, y2);
  width = abs(x1 - x2);
  height = abs(y1 - y2);

  g_triangles[layer].push_back(this);
}

tri::~tri() {
  //M("~tri()");
  g_triangles[layer].erase(remove(g_triangles[layer].begin(), g_triangles[layer].end(), this), g_triangles[layer].end());
}

void tri::render(SDL_Renderer* renderer) {

  int tx1 = g_camera.zoom * (x1-g_camera.x);
  int tx2 = g_camera.zoom * (x2-g_camera.x);


  int ty1 = g_camera.zoom * (y1-g_camera.y)- layer * 38;
  int ty2 = g_camera.zoom * (y2-g_camera.y)- layer * 38;


  SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty2);
  SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty1);
  SDL_RenderDrawLine(renderer,  tx2, ty2, tx2, ty1);
}

impliedSlopeTri::impliedSlopeTri(int fx1, int fy1, int fx2, int fy2, int flayer, int fstyle) {
  //M("tri()");
  x1=fx1; y1=fy1;
  x2=fx2; y2=fy2;
  layer = flayer;
  style = fstyle;
  if(x2 < x1 && y2 > y1) {
    type = 0; //  :'
  }
  if(x2 < x1 && y2 < y1) {
    type = 1; //  :,
  }
  if(x2 > x1 && y2 < y1) {
    type = 2; //  ,:
  }
  if(x2 > x1 && y2 > y1) {
    type = 3; //  ':
  }
  m = float(y1 -y2) / float(x1 - x2);
  b = y1 - (m * x1);

  x = min(x1, x2);
  y = min(y1, y2);
  width = abs(x1 - x2);
  height = abs(y1 - y2);

  g_impliedSlopeTris.push_back(this);
}

impliedSlopeTri::~impliedSlopeTri() {
  g_impliedSlopeTris.erase(remove(g_impliedSlopeTris.begin(), g_impliedSlopeTris.end(), this), g_impliedSlopeTris.end());
}

void impliedSlopeTri::render(SDL_Renderer* renderer) {
  int tx1 = g_camera.zoom * (x1-g_camera.x);
  int tx2 = g_camera.zoom * (x2-g_camera.x);
  int ty1 = g_camera.zoom * (y1-g_camera.y)- layer * 38;
  int ty2 = g_camera.zoom * (y2-g_camera.y)- layer * 38;
  SDL_SetRenderDrawColor(renderer, 10, 200, 150, 255);
  SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty2);
  SDL_RenderDrawLine(renderer,  tx1, ty1, tx2, ty1);
  SDL_RenderDrawLine(renderer,  tx2, ty2, tx2, ty1);
}

//sortingfunction for optimizing fog and triangular walls
//sort based on x and y
int trisort(tri* one, tri* two) {
  return one->x < two->x || (one->x==two->x && one->y < two->y);
}

ramp::ramp(int fx, int fy, int flayer, int ftype, string fwallt, string fcapt) {
  x = fx;
  y = fy;
  layer = flayer;
  type = ftype;
  walltexture = fwallt;
  captexture = fcapt;
  g_ramps[layer].push_back(this);
}

ramp::~ramp() {
  g_ramps[layer].erase(remove(g_ramps[layer].begin(), g_ramps[layer].end(), this), g_ramps[layer].end());
}


bool PointInsideRightTriangle(tri* t, int px, int py) {
  switch(t->type) {
    case(0):
      if(px >= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {


        return true;
      }
      break;

    case(1):
      if(px >= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {


        return true;
      }
      break;

    case(2):
      if(px <= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {


        return true;
      }
      break;

    case(3):
      if(px <= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {


        return true;
      }
      break;

  }
  return false;
}

bool IPointInsideRightTriangle(impliedSlopeTri* t, int px, int py) {
  switch(t->type) {
    case(0):
      if(px >= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {


        return true;
      }
      break;

    case(1):
      if(px >= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {


        return true;
      }
      break;

    case(2):
      if(px <= t->x2 && py <= t->y1 && py > ceil(t->m * px) + t->b) {


        return true;
      }
      break;

    case(3):
      if(px <= t->x2 && py >= t->y1 && py < floor(t->m * px)  + t->b) {


        return true;
      }
      break;

  }
  return false;
}

bool RectOverlap(rect a, rect b) {
  if (a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y) {
    return true;
  } else {
    return false;
  }
}

bool RectOverlap3d(rect a, rect b) {
  return ( RectOverlap(a, b) && a.z < b.z + b.zeight && a.z + a.zeight > b.z);
}

bool ElipseOverlap(rect a, rect b) {
  //get midpoints
  coord midpointA = {a.x + a.width/2, a.y + a.height/2};
  coord midpointB = {b.x + b.width/2, b.y + b.height/2};

  //"unfold" crumpled y axis by multiplying by 1/XtoY
  midpointA.y *= 1/XtoY;
  midpointB.y *= 1/XtoY;

  //circle collision test using widths of rectangles as radiusen
  return (Distance(midpointA.x, midpointA.y, midpointB.x, midpointB.y) < (a.width + b.width)/2);
}

bool CylinderOverlap(rect a, rect b, int skin) {
  //do an elipsecheck - then make sure the heights match up
  return (ElipseOverlap(a, b)) && ( (a.z >= b.z && a.z <= b.z + b.zeight) || (b.z >= a.z && b.z <= a.z + a.zeight));
}

bool RectOverlap(SDL_Rect a, SDL_Rect b) {
  if (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y) {
    return true;
  } else {
    return false;
  }
}

bool RectOverlap(SDL_FRect a, SDL_FRect b) {
  if (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y) {
    return true;
  } else {
    return false;
  }
}

//is a inside b?
bool RectWithin(rect a, rect b) {
  if (b.x < a.x && b.x + b.width > a.x + a.width && b.y < a.y && b.y + b.height > a.y + a.height) {
    return true;
  } else {
    return false;
  }
}

bool TriRectOverlap(tri* a, int x, int y, int width, int height) {
  if(PointInsideRightTriangle(a, x, y +  height)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x + width, y + height)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x + width, y)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x, y)) {
    return 1;
  }
  //also check if the points of the triangle are inside the rectangle
  //skin usage is possibly redundant here
  if(a->x1 > x + 0 && a->x1 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y2 > y + 0&& a->y2 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  return 0;
}

bool TriRectOverlap(tri* a, rect r) {
  int x = r.x;
  int y = r.y;
  int width = r.width;
  int height = r.height;
  if(PointInsideRightTriangle(a, x, y +  height)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x + width, y + height)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x + width, y)) {
    return 1;
  }
  if(PointInsideRightTriangle(a, x, y)) {
    return 1;
  }
  //also check if the points of the triangle are inside the rectangle
  //skin usage is possibly redundant here
  if(a->x1 > x + 0 && a->x1 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y2 > y + 0&& a->y2 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  return 0;
}

//for impliedSlopeTris
bool ITriRectOverlap(impliedSlopeTri* a, int x, int y, int width, int height) {
  if(IPointInsideRightTriangle(a, x, y +  height)) {
    return 1;
  }
  if(IPointInsideRightTriangle(a, x + width, y + height)) {
    return 1;
  }
  if(IPointInsideRightTriangle(a, x + width, y)) {
    return 1;
  }
  if(IPointInsideRightTriangle(a, x, y)) {
    return 1;
  }
  //also check if the points of the triangle are inside the rectangle
  //skin usage is possibly redundant here
  if(a->x1 > x + 0 && a->x1 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y2 > y + 0&& a->y2 < y + height - 0) {
    return 1;
  }
  if(a->x2 > x + 0 && a->x2 < x + width - 0 && a->y1 > y + 0 && a->y1 < y + height - 0) {
    return 1;
  }
  return 0;
}

SDL_Rect transformRect(SDL_Rect input) {
  SDL_Rect obj;
  obj.x = floor((input.x -g_camera.x) * g_camera.zoom);
  obj.y = floor((input.y - g_camera.y) * g_camera.zoom);
  obj.w = ceil(input.w * g_camera.zoom);
  obj.h = ceil(input.h * g_camera.zoom);
  return obj;
}

SDL_FRect transformRect(SDL_FRect input) {
  SDL_FRect obj;
  obj.x = ((input.x -g_camera.x) * g_camera.zoom);
  obj.y = ((input.y - g_camera.y) * g_camera.zoom);
  obj.w = (input.w * g_camera.zoom);
  obj.h = (input.h * g_camera.zoom);
  return obj;
}

rect transformRect(rect input) {
  rect obj;
  obj.x = floor((input.x -g_camera.x) * g_camera.zoom);
  obj.y = floor((input.y - g_camera.y) * g_camera.zoom);
  obj.width = ceil(input.width * g_camera.zoom);
  obj.height = ceil(input.height * g_camera.zoom);
  return obj;
}

box::box(int x1f, int y1f, int x2f, int y2f, int flayer, string &fwallt, string &fcapt, bool fcapped, bool fshineTop, bool fshineBot, const char* shading) {
  //M("box()");
  bounds.x = x1f;
  bounds.y = y1f;
  bounds.z = flayer * 32;
  bounds.width = x2f;
  bounds.height = y2f;
  bounds.zeight = 32;
  layer = flayer;
  walltexture = fwallt;
  captexture = fcapt;
  capped = fcapped;
  shineTop = fshineTop;
  shineBot = fshineBot;
  shadeTop = (shading[0] == '1');

  switch (shading[1])
  {
    case '0':
      shadeBot = 0;
      break;
    case '1':
      shadeBot = 1;
      break;
    case '2':
      shadeBot = 2;
      break;
  }
  shadeLeft = (shading[2] == '1');
  shadeRight = (shading[3] == '1');
  g_boxs[layer].push_back(this);
}

box::~box() {
  //M("~box()");
  //this line crashes during easybake
  g_boxs[layer].erase(remove(g_boxs[layer].begin(), g_boxs[layer].end(), this), g_boxs[layer].end());
}


//For walls which are not drawn, this object implies that they are sloped so that entities cannot hide behind them, which makes the world feel a bit easier to understand and feels very natural and "forgetable"
//always two blocks zeight, one block in y, and whatever in x
//support for layers?
impliedSlope::impliedSlope(int x1, int y1, int x2, int y2, int flayer, int fsleft, int fsright, int fShadedAtAll) {
  bounds.x = x1;
  bounds.y = y1;
  bounds.z = flayer * 32;
  bounds.width = x2;
  bounds.height = y2;
  bounds.zeight = 64;
  layer = flayer;
  shadeLeft = fsleft;
  shadeRight = fsright;
  shadedAtAll = fShadedAtAll;
  g_impliedSlopes.push_back(this);
}

impliedSlope::~impliedSlope() {
  g_impliedSlopes.erase(remove(g_impliedSlopes.begin(), g_impliedSlopes.end(), this), g_impliedSlopes.end());
}



// The idea of a collisionZone is to reduce overhead for large maps
// by having entities check if they are overlapping a collisionZone and only test for
// collision with other walls/projectiles/entities also overlapping that collisionZone
// They will be able to be placed manually or procedurally
collisionZone::collisionZone(int x, int y, int width, int height) {
  bounds = {x, y, width, height};
  g_collisionZones.push_back(this);
  for (int i = 0; i < g_layers; i++) {
    vector<box*> v = {};
    guests.push_back(v);
  }
}

collisionZone::~collisionZone() {
  for (int i = 0; i < g_layers; i++) {
    guests[i].clear();
  }
}

//ATM we will be doing this on mapload
void collisionZone::inviteAllGuests() {
  for(int i = 0; i < g_layers; i++) {
    for(int j = 0; j < (int)g_boxs[i].size(); j++){
      if(RectOverlap(this->bounds, g_boxs[i][j]->bounds)) {
        guests[i].push_back(g_boxs[i][j]);
      }
    }
  }
}

void collisionZone::debugRender(SDL_Renderer* renderer) {
  SDL_FRect rend = {(float)bounds.x, (float)bounds.y, (float)bounds.width, (float)bounds.height};
  rend = transformRect(rend);
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderDrawRectF(renderer, &rend);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}


//cast a ray from the sky at a xy position and returns a z position of an intersection with a block
int verticalRayCast(int fx, int fy) {

  rect trace = {fx-5, fy-5, 10, 10};
  for (int i = g_layers - 1; i >= 0; i--) {
    for(auto x : g_boxs[i]) {
      if(RectOverlap(trace, x->bounds )) {
        //we hit a block
        return (i + 1) * 64;
      }
    }
  }

  //no hit
  return 0;

}

door::door(SDL_Renderer * renderer, const char* fmap, string fto_point,  int fx, int fy, int fz, int fwidth, int fheight, int fzeight) {
  //M("door()");
  this->x = fx;
  this->y = fy;
  this->z = fz;

  this->bounds.x = fx;
  this->bounds.y = fy;

  to_map = fmap;
  to_point = fto_point;

  this->width = fwidth;
  this->height = fheight;
  this->zeight = fzeight;

  this->bounds.width = width;
  this->bounds.height = height;
  g_doors.push_back(this);

}


door::~door() {
  g_doors.erase(remove(g_doors.begin(), g_doors.end(), this), g_doors.end());
}

dungeonDoor::dungeonDoor(int fx, int fy, int fwidth, int fheight) 
{
  x = fx;
  y = fy;
  width = fwidth;
  height = fheight;
  g_dungeonDoors.push_back(this);
}

dungeonDoor::~dungeonDoor()
{
  g_dungeonDoors.erase(remove(g_dungeonDoors.begin(), g_dungeonDoors.end(), this), g_dungeonDoors.end());
}


tile::tile(SDL_Renderer * renderer, const char* filename, const char* mask_filename, int fx, int fy, int fwidth, int fheight, int flayer, bool fwrap, bool fwall, float fdxoffset, float fdyoffset) {
  this->x = fx;
  this->y = fy;
  this->z = flayer;
  this->width = fwidth;
  this->height = fheight;
  this->wall = fwall;
  this->wraptexture = fwrap;

  this->dxoffset = fdxoffset;
  this->dyoffset = fdyoffset;

  fileaddress = filename;

  bool cached = false;
  //has someone else already made a texture?
  for(int i=0; i < (int)g_tiles.size(); i++){
    if(g_tiles[i]->fileaddress == this->fileaddress && g_tiles[i]->mask_fileaddress == mask_filename) {
      //make sure both are walls or both aren't
      if(g_tiles[i]->wall == this->wall) {
        //M("sharing a texture" );
        cached = true;
        this->texture = g_tiles[i]->texture;
        SDL_QueryTexture(g_tiles[i]->texture, NULL, NULL, &texwidth, &texheight);
        this->asset_sharer = 1;
        break;
      }
    }
  }
  if(cached) {

  } else {
    image = loadSurface(filename);
    texture = SDL_CreateTextureFromSurface(renderer, image);
    if(wall) {
      SDL_SetTextureColorMod(texture, -65, -65, -65);
    }

    SDL_QueryTexture(texture, NULL, NULL, &texwidth, &texheight);
    if(g_waterAllocated == 0 && fileaddress.find("sp-water") != std::string::npos) {
      g_waterAllocated = 1;
      g_waterSurface = loadSurface(filename); // the values of this data will not be modified
      g_waterTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 512, 440);
      SDL_SetTextureBlendMode(g_waterTexture, SDL_BLENDMODE_BLEND);
      texture = g_waterTexture;
    }

    if(mask_filename[0] != '&') {
      SDL_DestroyTexture(texture);

      SDL_Surface* smask = loadSurface(mask_filename);
      SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
      SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);

      texture = MaskTexture(renderer, diffuse, mask);
      SDL_FreeSurface(smask);
      SDL_DestroyTexture(mask);
      SDL_DestroyTexture(diffuse);
    }
  }



  mask_fileaddress = mask_filename;


  this->xoffset = fmod(this->x, this->texwidth);
  this->yoffset = fmod(this->y, this->texheight);

  SDL_FreeSurface(image);
  g_tiles.push_back(this);
}

tile::~tile() {
  //M("~tile()" );
  g_tiles.erase(remove(g_tiles.begin(), g_tiles.end(), this), g_tiles.end());
  if(!asset_sharer) {
    SDL_DestroyTexture(texture);
  }
}

void tile::reloadTexture() {
  if(asset_sharer) {

  } else {
    image = loadSurface(fileaddress);
    texture = SDL_CreateTextureFromSurface(renderer, image);
    if(wall) {
      //make walls a bit darker
      SDL_SetTextureColorMod(texture, -65, -65, -65);
    } else {
      //SDL_SetTextureColorMod(texture, -20, -20, -20);
    }

    SDL_QueryTexture(texture, NULL, NULL, &texwidth, &texheight);
    if(fileaddress.find("OCCLUSION") != string::npos) {
      //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);

    }
    if(mask_fileaddress[0] != '&') {
      SDL_DestroyTexture(texture);

      SDL_Surface* smask = loadSurface(mask_fileaddress.c_str());
      SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
      SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);

      texture = MaskTexture(renderer, diffuse, mask);
      SDL_FreeSurface(smask);
      SDL_DestroyTexture(mask);
      SDL_DestroyTexture(diffuse);
    }
    SDL_FreeSurface(image);
  }
}

void tile::reassignTexture() {
  if(asset_sharer) {
    if(g_tiles.size() > 1) {
      for(unsigned int i=g_tiles.size() - 1; i > 0; i--){
        if(g_tiles[i]->mask_fileaddress == this->mask_fileaddress && g_tiles[i]->fileaddress == this->fileaddress && !g_tiles[i]->asset_sharer) {

          this->texture = g_tiles[i]->texture;

          this->asset_sharer = 1;
          this->texwidth = g_tiles[i]->texwidth;
          this->texheight = g_tiles[i]->texheight;
          break;

        }
      }
    }
  }
}

rect tile::getMovedBounds() {
  return rect(x, y, width, height);
}

void tile::render(SDL_Renderer * renderer, camera fcamera) {
  SDL_FPoint nowt = {0, 0};
  rect obj((x -fcamera.x)* fcamera.zoom, (y-fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom);
  rect cam(0, 0, fcamera.width, fcamera.height);

  //movement
  if((dxoffset !=0 || dyoffset != 0 ) && wraptexture) {
    xoffset += dxoffset;
    yoffset += dyoffset;
    if(xoffset < 0) {
      xoffset = texwidth;
    }
    if(xoffset > texwidth) {
      xoffset = 0;
    }
    if(yoffset < 0) {
      yoffset = texheight;
    }
    if(yoffset > texheight) {
      yoffset = 0;
    }
  }

  if(RectOverlap(obj, cam)) {

    if(this->wraptexture) {
      SDL_FRect srcrect;
      SDL_FRect dstrect;
      float ypos = 0;
      float xpos = 0;

      srcrect.x = xoffset;
      srcrect.y = yoffset;
      while(1) {
        if(srcrect.x == xoffset) {
          srcrect.w = (texwidth - xoffset);
          dstrect.w = (texwidth - xoffset);
        } else {
          srcrect.w = texwidth;
          dstrect.w = texwidth;
        }
        if(srcrect.y == yoffset) {

          srcrect.h = texheight - yoffset;
          dstrect.h = texheight - yoffset;
        } else {
          dstrect.h = texheight;
          srcrect.h = texheight;
        }






        dstrect.x = (x + xpos);
        dstrect.y = (y + ypos);




        //are we still inbounds?
        if(xpos + dstrect.w > this->width) {
          dstrect.w = this->width - xpos;
          if(dstrect.w + srcrect.x > texwidth) {
            dstrect.w = texwidth - srcrect.x;
          }
          srcrect.w = dstrect.w;
        }
        if(ypos + dstrect.h > this->height) {
          dstrect.h = this->height - ypos;
          if(dstrect.h + srcrect.y > texheight) {
            dstrect.h = texheight - srcrect.y;
          }
          srcrect.h = dstrect.h;

        }
        //are we still in the texture






        //transform

        dstrect.w = (dstrect.w * fcamera.zoom);
        dstrect.h = (dstrect.h * fcamera.zoom);

        dstrect.x = ((dstrect.x - fcamera.x)* fcamera.zoom);
        dstrect.y = ((dstrect.y - fcamera.y)* fcamera.zoom);

        SDL_Rect fsrcrect;
        fsrcrect.x = srcrect.x;
        fsrcrect.y = srcrect.y;
        fsrcrect.w = srcrect.w;
        fsrcrect.h = srcrect.h;

        SDL_RenderCopyExF(renderer, texture, &fsrcrect, &dstrect, 0, &nowt, SDL_FLIP_NONE);




        xpos += srcrect.w;
        srcrect.x = 0;

        if(xpos >= this->width) {
          xpos = 0;
          srcrect.x = xoffset;

          ypos += srcrect.h;
          srcrect.y = 0;
          if(ypos >= this->height)
            break;
        }
      }
    } else {
      SDL_FRect dstrect = { (x -fcamera.x)* fcamera.zoom, (y-fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};
      SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
    }

    //render specular over water
    if(texture == g_waterTexture) {
      SDL_FRect dstrect = { 0, 0, WIN_WIDTH, WIN_HEIGHT};
      SDL_Rect srcrect = { 0, 0, 256, 220};
      //minimize the shine if it wouldn't be drawn over the
      //tile
      float left, right;
      float top, bottom;
      transform3dPoint(x, y, 0, left, top);
      transform3dPoint(x+width, y+height, 0, right, bottom);

      float tclip = 0, bclip = 0, lclip = 0, rclip = 0;
      float tp = 0, lp = 0;
      if(top > 0) {
        tclip = top/WIN_HEIGHT;
      }
      if(bottom < WIN_HEIGHT) {
        bclip = (WIN_HEIGHT-bottom)/WIN_HEIGHT;
      }

      if(left > 0) {
        lclip = left/WIN_WIDTH;
      }     
      if(right < WIN_WIDTH) {
        rclip = (WIN_WIDTH-right)/WIN_WIDTH;
      }
      dstrect.y += tclip * WIN_HEIGHT;
      srcrect.y += (tclip)* 220;

      dstrect.x += lclip * WIN_WIDTH;
      srcrect.x += (lclip)* 256;

      dstrect.h -= bclip * WIN_HEIGHT;
      srcrect.h -= bclip * 220;

      dstrect.w -= rclip * WIN_WIDTH;
      srcrect.w -= rclip* 256;
      
      if(dstrect.w + dstrect.x > WIN_WIDTH) {
        dstrect.w += WIN_WIDTH - (dstrect.w + dstrect.x);
      }

      if(dstrect.h + dstrect.y > WIN_HEIGHT) {
        dstrect.h += WIN_HEIGHT - (dstrect.h + dstrect.y);
      }
      
      SDL_RenderCopyF(renderer, g_wSpec, &srcrect, &dstrect);
    }
  }
}


//i should update this with variables, which are expressions from an attack file
float attack::forward(float time) {
  return speed;
}

float attack::sideways(float time) {
  //return 10 * cos(time / 45);
  return 0;
}

//new param to attack()
//entities that are deleted on map closure
//can try to share attack graphics
//but not entities that could possibly join the party
attack::attack(string filename, bool tryToShareTextures) {
  //M("attack()");
  this->name = filename;
  string loadstr;
  loadstr = "resources/static/attacks/" + filename + ".atk";

  istringstream file(loadTextAsString(loadstr));

  file >> spritename;

  string temp;
  temp = "resources/static/sprites/" + spritename + ".qoi";


  //only try to share textures if this isnt an entity
  //that can ever be part of the party
//  if(tryToShareTextures) {
//    for(auto x : g_attacks){
//      if(x->spritename == this->spritename) {
//        this->texture = x->texture;
//        assetsharer = 1;
//
//      }
//    }
//  }

  file >> framewidth;
  file >> frameheight;
  if(!assetsharer) {
    //texture = loadTexture(renderer, temp);
  }

  file >> this->maxCooldown;
  float size;
  file >> size;

  //SDL_QueryTexture(texture, NULL, NULL, &width, &height);

//  xframes = width / framewidth;
//  yframes = height / frameheight;
//
//  for (int j = 0; j < height; j+=frameheight) {
//    for (int i = 0; i < width; i+= framewidth) {
//      coord a;
//      a.x = i;
//      a.y = j;
//      framespots.push_back(a);
//    }
//  }
//
//  width = framewidth * size;
//  height = frameheight * size;



  file >> this->damage;
  file >> this->speed;
  file >> this->shotLifetime;
  file >> this->spread;
  this->spread *= M_PI;
  file >> this->randomspread;
  randomspread *= M_PI;
  file >> this->numshots;
  file >> this->range;
  this->melee = 0;
  file >> this->melee; //should we just barrel towards the target no matter what
  file >> this->snake;

  g_attacks.push_back(this);
}

attack::~attack() {
  if(!assetsharer) {
    //SDL_DestroyTexture(texture);
  }
  g_attacks.erase(remove(g_attacks.begin(), g_attacks.end(), this), g_attacks.end());
}



weapon::weapon() {}

//add constructor and field on entity object
//second param should be 0 for entities
//that could join the party and 1 otherwise
weapon::weapon(string fname, bool tryToShareGraphics) {
  name = fname;

  string line;
  string address;

  //local
  address = "resources/static/weapons/" + name + ".wep";
  

  string field = "";
  string value = "";
  //file.open(address);
  istringstream file(loadTextAsString(address));


  while(getline(file, line)) {
    if(line == "&") { break; }
    field = line.substr(0, line.find(' '));
    attack* a = new attack(line, tryToShareGraphics);

    //a->faction = faction;
    attacks.push_back(a);
  }
  file >> maxComboResetMS;
  g_weapons.push_back(this);
}

weapon::~weapon() {
  for(auto x: attacks) {
    delete x;
  }
  g_weapons.erase(remove(g_weapons.begin(), g_weapons.end(), this), g_weapons.end());
}


//add entities and mapObjects to g_actors with dc
actor::actor() {
  //M("actor()");
  bounds.x = 0; bounds.y = 0; bounds.width = 10; bounds.height = 10;
  g_actors.push_back(this);
}

actor::~actor() {
  //M("~actor()");
  g_actors.erase(remove(g_actors.begin(), g_actors.end(), this), g_actors.end());
}

void actor::render(SDL_Renderer * renderer, camera fcamera) {

}


float actor::getOriginX() {
  return  x + bounds.x + bounds.width/2;
}

float actor::getOriginY() {
  return y + bounds.y + bounds.height/2;
}

//for moving the object by its origin
//this won't move the origin relative to the sprite or anything like that
void actor::setOriginX(float fx) {
  x = fx - bounds.x - bounds.width/2;
}

void actor::setOriginY(float fy) {
  y = fy - bounds.y - bounds.height/2;
}


inline int compare_ent (actor* a, actor* b) {
  return a->y + a->z + a->sortingOffset + a->bonusSortingOffset < b->y + b->z + b->sortingOffset + b->bonusSortingOffset;
}

void sort_by_y(vector<actor*> &g_entities) {
  stable_sort(g_entities.begin(), g_entities.end(), compare_ent);
}


particle::particle(effectIndex* ftype) {
  g_particles.push_back(this);
  type = ftype;
  lifetime = type->plifetime;
  width = type->pwidth;
  height = type->pheight;
  accelerationx = type->paccelerationx;
  accelerationy = type->paccelerationy;
  accelerationz = type->paccelerationz;
  deltasizex = type->pdeltasizex;
  deltasizey = type->pdeltasizey;
  texture = type->texture;
  yframes = type->yframes;
  framewidth = type->framewidth;
  frameheight = type->frameheight;
  msPerFrame = type->msPerFrame;
  killAfterAnim = type->killAfterAnim;
}

particle::~particle() {
  g_particles.erase(remove(g_particles.begin(), g_particles.end(), this), g_particles.end());
}

void particle::update(int elapsed, camera fcamera) {
  lifetime -= elapsed;
  x -= velocityx * -cos(angle) + velocityy * sin(angle);
  y -= velocityx * sin(angle) + velocityy * -cos(angle);
  z += velocityz * elapsed;

  velocityx -= accelerationx * elapsed;
  velocityy -= accelerationy * elapsed;
  velocityz += accelerationz * elapsed;
  width += deltasizex * elapsed;
  height += deltasizey * elapsed;
  curAlpha += deltaAlpha * elapsed;
  if(curAlpha < 0) {curAlpha = 0;}
  float zero = 0;
  width = max(zero,width);
  height = max(zero,height);
  z = max(zero,z);

  msTilNextFrame += elapsed;
  if(msPerFrame != 0 && msTilNextFrame > msPerFrame) {
    frame++;
    if(frame >= yframes) {
      if(killAfterAnim) {
        lifetime = -1;
      } else {
        frame = 0;

      }
    }
    msTilNextFrame = 0;
  }
}

// particle render
void particle::render(SDL_Renderer* renderer, camera fcamera) {

  SDL_FRect dstrect = { (x -fcamera.x -(width/2))* fcamera.zoom, (y-(z* XtoZ) - fcamera.y-(height/2)) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};

  //set opacity of texture
  SDL_SetTextureAlphaMod(texture, curAlpha / 100);

  //need to consider the frame
  if(yframes > 1) {
    SDL_Rect srcrect = {0 + frame * framewidth , 0,  framewidth, frameheight};
    SDL_RenderCopyF(renderer, texture, &srcrect, &dstrect);
  } else {
    SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
  }

}


effectIndex::effectIndex(string filename, SDL_Renderer* renderer) {
  name = filename;
  string existSTR;
  existSTR = "resources/static/effects/" + filename + ".eft";
  //ifstream file;
  //file.open(existSTR);

  istringstream file(loadTextAsString(existSTR));
  string line;

  //name of texture
  file >> line;
  file >> line;
  texname = line;

  //has anyone already loaded this texture?
  //for(auto x : g_effectIndexes) {
  //if(x->texname == this->texname) {
  //this->OwnsTexture = 0;
  //this->texture = x->texture;
  //break;
  //}
  //}



  if(1) {
    existSTR = "resources/static/sprites/" + texname + ".qoi";
    texture = loadTexture(renderer, existSTR);
  }

  //yframes;
  file >> line;
  file >> yframes;

  //calculate framewidth;
  int texW = 0;
  int texH = 0;

  SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
  framewidth = texW / yframes;
  frameheight = texH;

  //chose_frandom_frame;
  file >> line;
  file >> chooseRandomFrame;

  //spawnNumber
  file >> line;
  file >> spawnNumber;

  //spawnRadius
  file >> line;
  file >> spawnRadius;
  spawnRadius *= 64;

  //plifetime
  file >> line;
  file >> plifetime;

  //disappear method
  file >> line;
  file >> disappearMethod;

  //pwidth
  file >> line;
  file >> pwidth;

  //pheight
  file >> line;
  file >> pheight;

  //alpha
  file >> line;
  file >> alpha;

  //delta alpha
  file >> line;
  file >> deltaAlpha;

  //pvelocityx
  file >> line;
  file >> pvelocityx;

  //pvelocityy
  file >> line;
  file >> pvelocityy;

  file >> line;
  file >> pvelocityz;

  //paccelerationx
  file >> line;
  file >> paccelerationx;

  //paccelerationy
  file >> line;
  file >> paccelerationy;

  //paccelerationz
  file >> line;
  file >> paccelerationz;

  //pdeltasizex
  file >> line;
  file >> pdeltasizex;

  //pdeltasizey
  file >> line;
  file >> pdeltasizey;

  //parent
  file >> line;
  file >> line;
  spawnerParent = searchEntities(line);

  //spawner X offset
  file >> line;
  file >> spawnerXOffset;

  //spawner Y offset
  file >> line;
  file >> spawnerYOffset;

  //spawner Z offset
  file >> line;
  file >> spawnerZOffset;

  //interval ms
  file >> line;
  file >> spawnerIntervalMs;

  //msPerFrame
  file >> line;
  file >> msPerFrame;

  //killAfterAnim
  file >> line;
  file >> killAfterAnim;


  g_effectIndexes.push_back(this);

}

//add support effectIndexes to maps
//given coordinates, spawn particles in the level
void effectIndex::happen(int fx, int fy, int fz, float fangle) {
  for(int i = 0; i < spawnNumber; i++) {
    particle* a = new particle(this);
    g_lastParticleCreated = a;
    a->velocityx = this->pvelocityx;
    a->velocityy = this->pvelocityy;
    a->velocityz = this->pvelocityz;

    //spawn randomly in a sphere
    float tx = rand() % 10000 - 5000;
    float ty = rand() % 10000 - 5000;
    float tz = rand() % 10000 - 5000;
    float tr = fmod(rand(), spawnRadius);

    float net = tr / pow( pow(tx,2) + pow(ty,2) + pow(tz,2) , 0.5);
    tx *= net;
    ty *= net;
    tz *= net;

    a->angle = fangle;

    a->x = fx + tx;
    a->y = fy + ty;
    a->z = fz + tz;
      
    a->alpha = this->alpha;
    a->deltaAlpha = this->deltaAlpha; 
    a->curAlpha = this->alpha;

    if(chooseRandomFrame) {
      a->frame = rand() % this->yframes;
    }
  }
}

effectIndex::~effectIndex() {
  if(OwnsTexture) {
    SDL_DestroyTexture(texture);
  }

  g_effectIndexes.erase(remove(g_effectIndexes.begin(), g_effectIndexes.end(), this), g_effectIndexes.end());
}

emitter::emitter() {
  g_emitters.push_back(this);
}

emitter::~emitter() {
  g_emitters.erase(remove(g_emitters.begin(), g_emitters.end(), this), g_emitters.end());
}



cshadow::cshadow(SDL_Renderer * renderer, float fsize) {
  size = fsize;
  g_shadows.push_back(this);
}

cshadow::~cshadow() {
  //M("~cshadow()" );
  g_shadows.erase(remove(g_shadows.begin(), g_shadows.end(), this), g_shadows.end());
}

void cshadow::render(SDL_Renderer * renderer, camera fcamera) {
  if(!owner->tangible || !this->visible) {return;}
  if(!owner->visible) {
    //show the protags shadow whilst spinning
    if(!(owner == protag && g_spinning_duration > -16)) {
      return;
    }
  }

  if(alphamod <= 0) {return;}
  if(alphamod > 255) {alphamod = 255;}

  //update texture with proper alphamod
  SDL_SetTextureAlphaMod(this->texture, alphamod);

  SDL_FRect dstrect = { ((this->x)-fcamera.x) *fcamera.zoom, (( (this->y - (XtoZ * z) ) ) -fcamera.y) *fcamera.zoom, (float)(width * size), (float)((height * size)* (637 /640) * 0.9)};
  //SDL_Rect dstrect = {500, 500, 200, 200 };
  //dstrect.y += (owner->height -(height/2)) * fcamera.zoom;
  float temp;
  temp = width;
  temp*= fcamera.zoom;
  dstrect.w = temp;
  temp = height;
  temp*= fcamera.zoom;
  dstrect.h = temp;

  rect obj(dstrect.x, dstrect.y, dstrect.w, dstrect.h);

  rect cam(0, 0, fcamera.width, fcamera.height);

  if(RectOverlap(obj, cam)) {
    SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
  }
}


projectile::projectile(attack* fattack) {
  this->sortingOffset = 12;
  this->width = fattack->width;
  this->height = fattack->height;
  this->bounds.width = this->width;
  this->bounds.height = this->height * XtoY;
  this->bounds.y = this->height - this->bounds.height;

  this->gun = fattack;
  this->maxLifetime = fattack->shotLifetime;
  this->lifetime = fattack->shotLifetime;

  shadow = new cshadow(renderer, fattack->size);
  this->shadow->owner = this;
  shadow->width = width;
  shadow->height = bounds.height;


  gun = fattack;
  texture = gun->texture;
  asset_sharer = 1;

  animate = 0;
  curheight = height;
  curwidth = width;
  g_projectiles.push_back(this);
}

projectile::~projectile() {
  g_projectiles.erase(remove(g_projectiles.begin(), g_projectiles.end(), this), g_projectiles.end());
  delete shadow;
  //make recursive projectile
}

void projectile::update(float elapsed) {


  rect bounds = {(int)x, (int)y, (int)width, (int)height};
  layer = max(z /64, 0.0f);
  if(!gun->snake) {
    for(auto n : g_boxs[layer]) {
      if(RectOverlap(bounds, n->bounds)) {
        lifetime = 0;
        return;
      }
    }
  }

  if(lifetime <= 0) {
    return;
  }

  x += (sin(angle) * gun->sideways(maxLifetime - lifetime) + cos(angle) * gun->forward(maxLifetime - lifetime) + xvel)* (elapsed / 16);
  y += (XtoY * (sin(angle + M_PI / 2) * gun->sideways(maxLifetime - lifetime) + cos(angle + M_PI / 2) * gun->forward(maxLifetime - lifetime) + yvel)) * (elapsed / 16);
  lifetime -= elapsed;

  //move shadow to feet
  shadow->x = x;
  shadow->y = y;

  float floor = 0;
  int layer;
  layer = max(z /64, 0.0f);
  layer = min(layer, (int)g_boxs.size() - 1);
  if(layer > 0) {

    //this means that if the bullet is above a block that isnt right under it, the shadow won't be seen, but it's not worth it atm to change
    rect thisMovedBounds = rect(x,  y, width, height);
    for (auto n : g_boxs[layer - 1]) {
      if(RectOverlap(n->bounds, thisMovedBounds)) {
        floor = 64 * (layer);
        break;
      }
    }
  }
  shadow->z = floor;
}

void projectile::render(SDL_Renderer * renderer, camera fcamera) {
  SDL_FRect obj = {(x -fcamera.x)* fcamera.zoom, ((y- (z + zeight) * XtoZ) - fcamera.y) * fcamera.zoom, width * fcamera.zoom, height * fcamera.zoom};
  SDL_FRect cam = {0, 0, fcamera.width, fcamera.height};

  if(RectOverlap(obj, cam)) {
    if(gun->framespots.size() > 1) {
      frame = animation * gun->xframes + frameInAnimation;
      SDL_Rect srcrect = {gun->framespots[frame].x, gun->framespots[frame].y, gun->framewidth, gun->frameheight};
      const SDL_FPoint center = {0 ,0};
      if(texture != NULL) {
        SDL_RenderCopyExF(renderer, texture, &srcrect, &obj, 0, &center, flip);
      }
    } else {
      if(texture != NULL) {
        SDL_RenderCopyF(renderer, texture, NULL, &obj);
      }
    }
  }
}





mapObject::mapObject(SDL_Renderer * renderer, string imageadress, const char* mask_filename, float fx, float fy, float fz, float fwidth, float fheight, bool fwall, float extrayoffset) {
  //M("mapObject() fake");

  name = imageadress;
  mask_fileaddress = mask_filename;

  bool cached = false;
  wall = fwall;

  //this could further be improved by going thru g_boxs and g_triangles
  //as there are always less of those than mapobjects
  //heres an idea: could we copy textures if the wall field arent equal instead of loading them again
  //techincally, this could be a big problem, so there could be a finit number of mapObjects we check
  //in a huge map, loading a map object with a texture that hasnt been loaded yet could take forever
  //!!!
  //if you're having problems with huge maps, revisit this
  if(g_mapObjects.size()  > 1) {
    for(unsigned int i=g_mapObjects.size() - 1; i > 0; i--){
      if(g_mapObjects[i]->mask_fileaddress == mask_filename && g_mapObjects[i]->name == this->name) {
        if(g_mapObjects[i]->wall == this->wall) {
          cached = true;
          this->texture = g_mapObjects[i]->texture;
          this->alternative =g_mapObjects[i]->alternative;
          this->asset_sharer = 1;
          this->framewidth = g_mapObjects[i]->framewidth;
          this->frameheight = g_mapObjects[i]->frameheight;
          break;
        } else {
          cached = true;
          this->texture = g_mapObjects[i]->alternative;
          this->alternative = g_mapObjects[i]->texture;

          this->asset_sharer = 1;
          this->framewidth = g_mapObjects[i]->framewidth;
          this->frameheight = g_mapObjects[i]->frameheight;
          break;
        }
      }
    }
  }
  if(cached) {

  } else {
    const char* plik = imageadress.c_str();
    SDL_Surface* image = loadSurface(plik);
    texture = SDL_CreateTextureFromSurface(renderer, image);
    alternative = SDL_CreateTextureFromSurface(renderer, image);


    if(mask_filename[0] != '&') {

      //the SDL_SetHint() changes a flag from 3 to 0 to 3 again.
      //this effects texture interpolation, and for masked entities such as wallcaps, it should
      //be off.

      SDL_DestroyTexture(texture);
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
      SDL_Surface* smask = loadSurface(mask_filename);
      SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
      SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);
      //SDL_SetTextureColorMod(diffuse, -65, -65, -65);
      texture = MaskTexture(renderer, diffuse, mask);
      SDL_FreeSurface(smask);
      SDL_DestroyTexture(mask);
      SDL_DestroyTexture(diffuse);
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
    }
    SDL_FreeSurface(image);

    if(fwall) {
      wall = 1;
      SDL_SetTextureColorMod(texture, -g_walldarkness, -g_walldarkness, -g_walldarkness);
    } else {
      SDL_SetTextureColorMod(alternative, -g_walldarkness, -g_walldarkness, -g_walldarkness);
    }
    if(name.find("SHADING") != string::npos) {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
      diffuse = 0;
      //SDL_SetTextureAlphaMod(texture, 150);
    }
    if(name.find("OCCLUSION") != string::npos) {
      //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
      diffuse = 0;
    }
    SDL_QueryTexture(texture, NULL, NULL, &this->framewidth, &this->frameheight);
  }




  //used for tiling


  this->x = fx;
  this->y = fy;
  this->z = fz;
  this->width = fwidth;
  this->height = fheight;

  //crappy solution for porting to windows
  if(this->framewidth == 0) { this->framewidth = 64;}
  this->xoffset = int(this->x) % int(this->framewidth);
  this->bounds.y = -55; //added after the ORIGIN was used for ent sorting rather than the FOOT.
  //this essentially just gives the blocks an invisible hitbox starting from their "head" so that their origin is in the middle
  //of the box

  extraYOffset = extrayoffset;
  this->yoffset = ceil(fmod(this->y - this->height+ extrayoffset, this->frameheight));
  //this->yoffset = 33;
  if(fwall ) {
    //this->yoffset = int(this->y + this->height - (z * XtoZ) + extrayoffset) % int(this->frameheight);

    //most favorable approach- everything lines up, even diagonal walls.
    //why is the texture height multiplied by 4? It just has to be a positive number, so multiples of the frameheight
    //are added in because of the two negative terms
    this->yoffset = (int)(4 * this->frameheight - this->height - (z * XtoZ)) % this->frameheight;
  }

  g_mapObjects.push_back(this);

}

//There was a problem when I ported to windows, where mapobjects which used masks for their textures (e.g. the tops of triangular walls)
//would lose their textures. This function exists to reload them after the windowsize has been changed.
void mapObject::reloadTexture() {
  if(asset_sharer) {

  } else {
    //delete our existing texture
    SDL_DestroyTexture(texture);
    SDL_DestroyTexture(alternative);

    const char* plik = name.c_str();
    SDL_Surface* image = loadSurface(plik);
    texture = SDL_CreateTextureFromSurface(renderer, image);
    alternative = SDL_CreateTextureFromSurface(renderer, image);


    if(mask_fileaddress[0] != '&') {

      //the SDL_SetHint() changes a flag from 3 to 0 to 3 again.
      //this effects texture interpolation, and for masked entities such as wallcaps, it should
      //be off.

      SDL_DestroyTexture(texture);
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
      SDL_Surface* smask = loadSurface(mask_fileaddress.c_str());
      SDL_Texture* mask = SDL_CreateTextureFromSurface(renderer, smask);
      SDL_Texture* diffuse = SDL_CreateTextureFromSurface(renderer, image);
      //SDL_SetTextureColorMod(diffuse, -65, -65, -65);
      texture = MaskTexture(renderer, diffuse, mask);
      SDL_FreeSurface(smask);
      SDL_DestroyTexture(mask);
      SDL_DestroyTexture(diffuse);
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
    }
    SDL_FreeSurface(image);

    if(wall) {
      SDL_SetTextureColorMod(texture, -g_walldarkness, -g_walldarkness, -g_walldarkness);
    } else {
      SDL_SetTextureColorMod(alternative, -g_walldarkness, -g_walldarkness, -g_walldarkness);
    }
    if(name.find("SHADING") != string::npos) {
      SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
      //SDL_SetTextureAlphaMod(texture, 150);
    }
    if(name.find("OCCLUSION") != string::npos) {
      //SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_MOD);
    }
    //SDL_QueryTexture(texture, NULL, NULL, &this->framewidth, &this->frameheight);
  }
}

//this is used with reloadTexture for any mapObjects which are asset-sharers
void mapObject::reassignTexture() {
  if(asset_sharer) {
    if(g_mapObjects.size()  > 1) {
      for(unsigned int i=g_mapObjects.size() - 1; i > 0; i--){
        if(g_mapObjects[i]->mask_fileaddress == this->mask_fileaddress && g_mapObjects[i]->name == this->name && !g_mapObjects[i]->asset_sharer) {
          if(g_mapObjects[i]->wall == this->wall) {
            this->texture = g_mapObjects[i]->texture;
            this->alternative =g_mapObjects[i]->alternative;
            this->asset_sharer = 1;
            this->framewidth = g_mapObjects[i]->framewidth;
            this->frameheight = g_mapObjects[i]->frameheight;
            break;
          } else {
            this->texture = g_mapObjects[i]->alternative;
            this->alternative = g_mapObjects[i]->texture;

            this->asset_sharer = 1;
            this->framewidth = g_mapObjects[i]->framewidth;
            this->frameheight = g_mapObjects[i]->frameheight;
            break;
          }
        }
      }
    }
  }
}

mapObject::~mapObject() {
  if(!asset_sharer) {
    SDL_DestroyTexture(alternative);
  }
  g_mapObjects.erase(remove(g_mapObjects.begin(), g_mapObjects.end(), this), g_mapObjects.end());
}

rect mapObject::getMovedBounds() {
  return rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
}

void mapObject::render(SDL_Renderer * renderer, camera fcamera) {
  SDL_FPoint nowt = {0, 0};

  SDL_FRect obj; // = {(floor((x -fcamera.x)* fcamera.zoom) , floor((y-fcamera.y - height - XtoZ * z) * fcamera.zoom), ceil(width * fcamera.zoom), ceil(height * fcamera.zoom))};
  obj.x = (x -fcamera.x)* fcamera.zoom;
  obj.y = (y-fcamera.y - height - XtoZ * z) * fcamera.zoom;
  obj.w = width * fcamera.zoom;
  obj.h = height * fcamera.zoom;

  SDL_FRect cam;
  cam.x = 0;
  cam.y = 0;
  cam.w = fcamera.width;
  cam.h = fcamera.height;

  if(RectOverlap(obj, cam)) {
    SDL_Rect srcrect;
    SDL_Rect dstrect;
    int ypos = 0;
    int xpos = 0;
    srcrect.x = xoffset;
    srcrect.y = yoffset;

    while(1) {
      if(srcrect.x == xoffset) {
        srcrect.w = framewidth - xoffset;
        dstrect.w = framewidth - xoffset;
      } else {
        srcrect.w = framewidth;
        dstrect.w = framewidth;
      }
      if(srcrect.y == yoffset) {
        srcrect.h = frameheight - yoffset;
        dstrect.h = frameheight - yoffset;
      } else {
        dstrect.h = frameheight;
        srcrect.h = frameheight;
      }

      dstrect.x = (x + xpos);
      dstrect.y = (y+ ypos- z * XtoZ );




      //are we still inbounds?
      if(xpos + dstrect.w > this->width) {
        dstrect.w = this->width - xpos;
        if(dstrect.w + srcrect.x > framewidth) {
          dstrect.w = framewidth - srcrect.x;
        }
        srcrect.w = dstrect.w;
      }
      if(ypos + dstrect.h > this->height ) {

        dstrect.h = this->height - ypos;
        if(dstrect.h + srcrect.y > frameheight) {
          dstrect.h = frameheight - srcrect.y;
        }
        srcrect.h = dstrect.h;
        //dstrect.h ++;
      }

      dstrect.x = ((dstrect.x - fcamera.x)* fcamera.zoom);
      dstrect.y = ((dstrect.y - fcamera.y - height)* fcamera.zoom);
      dstrect.w = (dstrect.w * fcamera.zoom);
      dstrect.h = (dstrect.h * fcamera.zoom);

      SDL_RenderCopy(renderer, texture, &srcrect, &dstrect);
      xpos += srcrect.w;
      srcrect.x = 0;

      if(xpos >= this->width) {

        xpos = 0;
        srcrect.x = xoffset;

        ypos += srcrect.h;
        srcrect.y = 0;
        if(ypos >= this->height)
          break;
      }
    }
  }
}

indexItem::indexItem(string fname, bool fisKeyItem) : name(fname), isKeyItem(fisKeyItem) {

  //search worlditems for an item with the same texture
  string lstr;

  lstr = "resources/static/items/" + fname + "-inv.qoi";


  bool storeThis = true;
  for(auto x : g_indexItems) {
    //M(x->name);
    if(this->name == x->name) {
      storeThis = false;
    }
  }
  if(storeThis) {
    texture = loadTexture(renderer, lstr);
    g_indexItems.push_back(this);
  }

  //script
  ifstream stream;

  //check local dir
  string loadstr = "resources/static/items/" + fname + ".txt";

  vector<string> script = loadText(loadstr);

  parseScriptForLabels(script);
}

indexItem::~indexItem() {
  g_indexItems.erase(remove(g_indexItems.begin(), g_indexItems.end(), this), g_indexItems.end());
  SDL_DestroyTexture(texture);
}

void fancychar::setIndex(int findex) {
  auto entry = g_fancyAlphabet[findex];
  texture = entry.first;
  width = entry.second * 0.0007;
}

void fancychar::render(fancyword* parent) {
  if(!show) {return;}
  SDL_FRect dstrect = {x + xd + parent->x + 0.05, y + yd + parent->y + 0.7, width, 0.05};

  dstrect.w *= bonusWidth;
  dstrect.h *= bonusWidth;
  
  dstrect.x -= bonusSize/2;
  dstrect.y -= bonusSize/2;
  dstrect.w += bonusSize;
  dstrect.h += bonusSize;

  dstrect.x *= (float)WIN_WIDTH;
  dstrect.y *= (float)WIN_HEIGHT;
  dstrect.w *= (float)WIN_WIDTH;
  dstrect.h *= (float)WIN_WIDTH;

  float booshAmount = 2*(WIN_WIDTH / STANDARD_SCREENWIDTH);
  SDL_FRect shadowRect = dstrect;
  shadowRect.x += booshAmount;
  shadowRect.y += booshAmount;
  
  SDL_Color cc = adventureUIManager->textcolors[color].second;

  //SDL_SetTextureColorMod(texture, 55, 55, 55);
  SDL_SetTextureColorMod(texture, pow(cc.r,0.8),pow(cc.g,0.8),pow(cc.b,0.8));
  SDL_SetTextureAlphaMod(texture, opacity);

  SDL_RenderCopyF(renderer, texture, NULL, &shadowRect);

  SDL_SetTextureColorMod(texture, pow(cc.r,1.0013),pow(cc.g,1.0013),pow(cc.b,1.0013));
  
  SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
}

void fancychar::update(float elapsed) {
  accumulator += accSpeed;
  if(accumulator > 1) {
    accumulator -= 1;
  }

  switch (movement) {
    case '0': 
    {
      break;
    }
    case '1':
    {
      //frightful shaking
      xd = ((double)rand() / RAND_MAX) / 500;
      yd = ((double)rand() / RAND_MAX) / 500;
      break;
    }
    case '2':
    {
      //lightheaded wave
      accSpeed = 0.01;
  
      float indexOffset = index;
      indexOffset /= 5;
  
      yd = sin(accumulator*2*M_PI + indexOffset) / 120;
      xd = cos(accumulator*2*M_PI + indexOffset) / 120;
      
      break;
    }
    case '3':
    {
      //parade, lines dance united
      accSpeed = 0.02;
  
      float indexOffset = index;
      indexOffset /= 15;
  
      yd = sin(accumulator*2*M_PI + indexOffset) / 240;
      xd = cos(accumulator*2*M_PI + indexOffset) / 240;
      break;
    }
    case '4':
    {
      //light swinging
      accSpeed = 0.02;
  
      float indexOffset = index;
      indexOffset /= 15;
  
      yd = sin(accumulator*2*M_PI + indexOffset) / 240;
      xd = cos(accumulator*2*M_PI + indexOffset) / 240;
      break;
    }
    case '5':
    {
      //swinging
      accSpeed = 0.02;
  
      float indexOffset = index;
      indexOffset /= 5;
  
      yd = sin(accumulator*2*M_PI + indexOffset) / 240;
      xd = cos(accumulator*2*M_PI + indexOffset) / 240;
      break;
    }
    case '6':
    {
      //smashed
      if(xd == 0) {
        xd = ((double)rand() / RAND_MAX) / 200;
        yd = ((double)rand() / RAND_MAX) / 200;
      }
      break;
    }
    case '7':
    {
      //vertical smash
      if(yd == 0) {
        yd = ((double)rand() / RAND_MAX) / 200;
      }
      break;
    }
    case '8':
    {
      //drop in, used by nintendo for shouting, or announcements
      if(show) {
        opacity += 25;
        if(opacity > 255) {
          opacity = 255;
        }
        bonusSize -= 0.01;
        if(bonusSize <= 0) {
          bonusSize = 0;
        }
      }
      break;
    }
    case '9':
    {
      //drop in big text;
      if(show) {
        opacity += 25;
        if(opacity > 255) {
          opacity = 255;
        }
        bonusSize -= 0.01;
        if(bonusSize <= 0) {
          bonusSize = 0;
        }
      }
      break;
    }
    case 'a':
    {
      //little text
      break;
    }
    case 'b':
    {
      //swinging, with a swing tempo
     
      accSpeed = abs(xd)*2 + 0.01;
      float indexOffset = index;
      indexOffset /= 5;

  
      yd = sin(accumulator*2*M_PI + indexOffset) / 240;
      xd = cos(accumulator*2*M_PI + indexOffset) / 240;

      break;
    }

  }

}

void fancyword::render() { //render each char
  for(auto myChar : chars) {
    myChar.render(this);
  }
}

void fancyword::update(float elapsed) {
  for(int i = 0; i < chars.size(); i++) {
    chars[i].update(elapsed);
  }
}

void fancyword::append(int index, float fbw) {
  fancychar newChar;
  newChar.setIndex(index);
  newChar.bonusWidth = fbw;
  chars.push_back(newChar);
  if(chars.size() > 1) {
    chars.at(chars.size() -1).x += (chars.at(chars.size()-2).width * chars.at((chars.size()-2)).bonusWidth) + chars.at(chars.size()-2).x;
  }
  width += newChar.width * fbw;
}

fancybox::fancybox() {
  bounds.x = 0.05;
  bounds.y = 0.7;
  bounds.width = 0.95;
  bounds.height = 0.25;
}

void fancybox::render() {
  if(show) {
    for(auto word : words) {
      word.render();
    }
  }
}

void fancybox::update(float elapsed) {
  if(show) {
    for(int i = 0; i < words.size(); i++) {

      words[i].update(elapsed);
    }
  }
}

void fancybox::clear() {
  words.clear();
  wordProgress = 0;
  letterProgress = 0;
}

void fancybox::arrange(string fcontent) { 
  
  int i = 0;
  fancyword word;
  int runningIndex = 0;
  int color = 0;
  char movement = 0;
  for(int i = 1; i < fcontent.size(); i++) {
    runningIndex++;

    if(fcontent[i] == '\\') {
      //special encoding operation
      char first = fcontent[i+1];
      string second = "";
      second += fcontent[i+2];
      char sc = second[0];
      if(first == 'c') {
        switch(sc) {
          case 'a':
          {
            color = 10;
            break;
          }
          case 'b':
          {
            color = 11;
            break;
          }
          case 'c':
          {
            color = 12;
            break;
          }
          case 'd':
          {
            color = 13;
            break;
          }
          case 'e':
          {
            color = 14;
            break;
          }
          case 'f':
          {
            color = 15;
            break;
          }
          case 'g':
          {
            color = 16;
            break;
          }
          case 'h':
          {
            color = 17;
            break;
          }
          case 'i':
          {
            color = 18;
            break;
          }
          case 'j':
          {
            color = 19;
            break;
          }
          case 'k':
          {
            color = 20;
            break;
          }
          case 'l':
          {
            color = 21;
            break;
          }
          case 'm':
          {
            color = 22;
            break;
          }
          case 'n':
          {
            color = 23;
            break;
          }
          default:
          {
            color = stoi(second);
            break;
          }
        }
      } else if(first == 'm'){
        movement = second[0];
      }

      i++;
      i++;
      continue;
    }
    if(fcontent[i] == ' ') {
      float oldwidth = word.width + word.x;
      float oldy = word.y;
      words.push_back(word);
      word.chars.clear();
      word.width = 0;
      word.x = oldwidth + (0.01);
      word.y = oldy;
      if(word.x > 0.7) {
        word.x = 0;
        word.y += 0.08;
        if(movement == '9') {
          word.y += 0.08;
        }
        if(movement != '3') {
          //parade movement involves dancing lines
          runningIndex = 0;
        }

      }
      continue;
    }

    float bonusWidth = 1;
    if(movement == '9') { bonusWidth = 3;}
    if(movement == 'a') { bonusWidth = 0.5; word.y=0.025;}
    word.append(g_fancyCharLookup[fcontent[i]], bonusWidth);
    word.chars.at(word.chars.size() -1).index = runningIndex;
    word.chars.at(word.chars.size() -1).color = color;
    word.chars.at(word.chars.size() -1).movement = movement;
    if(movement == '8') {
      word.chars.at(word.chars.size() -1).opacity = 0;
      word.chars.at(word.chars.size() -1).bonusSize = 0.1;
    }
    if(movement == '9') {
      word.chars.at(word.chars.size() -1).opacity = 0;
      word.chars.at(word.chars.size() -1).bonusSize = 0.1;
    }
    if(movement == 'a') {
      word.chars.at(word.chars.size() -1).opacity = 170;
      word.chars.at(word.chars.size() -1).bonusSize = 0;
    }

    
    
  }

  words.push_back(word);

}

int fancybox::reveal() {
  if(words.size() == 0) { return -1;}
  // Play a clank
  if (adventureUIManager->typing)
  {
    Mix_HaltChannel(6);
    Mix_VolumeChunk(adventureUIManager->blip, 20);
    //playSound(6, adventureUIManager->blip, 0);
  }

  if(wordProgress < words.size()) {
    if(letterProgress < words[wordProgress].chars.size()) {
      words[wordProgress].chars[letterProgress].show = 1;
    }
    letterProgress++;
    if(letterProgress >= words[wordProgress].chars.size()) {
      letterProgress = 0;
      wordProgress ++;
    }
  } else {
    //all done!
    if(adventureUIManager->dialogProceedIndicator->show == 0) {
      adventureUIManager->dialogProceedIndicator->show = 1;
      adventureUIManager->dialogProceedIndicator->y = 0.9;
      adventureUIManager->c_dpiDesendMs = 0;
      adventureUIManager->c_dpiAsending = 0;
      adventureUIManager->typing = 0;
    }
    return -1;
  }

  return 0;
}

void fancybox::revealAll() {
  for(int i = 0; i <4000; i++) {
    if(reveal() == -1) {
      break;
    }
  }
}

worldsound::worldsound(string filename, int fx, int fy) {
  name = filename;
  //M("worldsound()" );

  string loadstr;
  //try to open from local map folder first

  loadstr = "resources/static/worldsounds/" + filename + ".ws";
  const char* plik = loadstr.c_str();

  istringstream file(loadTextAsString(plik));

  string temp;
  file >> temp;
  string existSTR;
  existSTR = "resources/static/sounds/" + temp + ".wav";
  


  blip = loadWav(existSTR.c_str());

  x = fx;
  y = fy;

  float tempFloat;
  file >> tempFloat;
  volumeFactor = tempFloat;

  file >> tempFloat;
  maxDistance = tempFloat;

  file >> tempFloat;
  minWait = tempFloat * 1000;

  file >> tempFloat;
  maxWait = tempFloat * 1000;
  g_worldsounds.push_back(this);
}

worldsound::~worldsound() {
  //M("~worldsound()" );
  Mix_FreeChunk(blip);
  g_worldsounds.erase(remove(g_worldsounds.begin(), g_worldsounds.end(), this), g_worldsounds.end());
}

void worldsound::update(float elapsed) {
  //!!! you can update this with the middle of the camera instead of the focused actor
  float dist = Distance(x, y, g_focus->getOriginX(), g_focus->getOriginY());
  //linear
  float cur_volume = ((maxDistance - dist)/maxDistance) * volumeFactor * 128;

  //logarithmic
  //float cur_volume = volume * 128 * (-log(pow(dist, 1/max_distance) + 2.718));
  //float cur_volume = (volume / (dist / max_distance)) * 128;


  if(cur_volume < 0) {
    cur_volume = 0;
  }
  Mix_VolumeChunk(blip, cur_volume);

  if(cooldown < 0) {
    //change volume
    //playSound(-1, blip, 0);
    cooldown = rand() % (int)maxWait + minWait;
  } else {
    cooldown -= elapsed;
  }
}

//manage an entity's statuseffects

void statusComponent::statusSet::addStatus(float ptime, float pfactor) {
  if(immunity) {return;}
  if(complex)
  {
    status newStatus;
    newStatus.lifetime = ptime * (1 - resistence);
    newStatus.factor = pfactor;
    statuses.push_back(newStatus);
  }
  else
  {
    if(statuses.size() > 0) {
      if(statuses.at(0).lifetime < ptime * (1-resistence)) {
        statuses.at(0).lifetime = ptime * (1-resistence);
      }
    } else {
      status newStatus;
      newStatus.lifetime = ptime * (1-resistence);
      statuses.push_back(newStatus);
    }
  }
}

void statusComponent::statusSet::clearStatuses() {
  statuses.clear();
}

float statusComponent::statusSet::updateStatuses(float elapsedMS) {
  float totalFactor = 0;
  for(int i = 0; i< statuses.size(); i++) {
    statuses.at(i).currentProckWaitMS += elapsedMS;
    statuses.at(i).lifetime -= elapsedMS;
    if(statuses.at(i).currentProckWaitMS > this->maxProckWaitMS) {
      statuses.at(i).currentProckWaitMS = 0;
      if(statuses.at(i).factor > totalFactor) {totalFactor = statuses.at(i).factor;}
    }
  }
  return totalFactor;
}

void statusComponent::statusSet::cleanUpStatuses() {
  for(int i = 0; i < statuses.size(); i++) {
    if(statuses.at(i).lifetime < 0) {statuses.erase(statuses.begin() + i); i--;}
  }
}

int statusComponent::statusSet::check() {
  return statuses.size() > 0;
}

statusComponent::statusComponent() {
  poisoned.complex = 1;
  poisoned.maxProckWaitMS = 1000;
  buffed.complex = 1;
  healen.complex = 1;
  healen.maxProckWaitMS = 1000;
  slown.complex = 1;
}



//default constructor is called automatically for children
entity::entity() {
  //M("entity()" );
}

//entity constructor
entity::entity(SDL_Renderer * renderer, string filename, float sizeForDefaults) {

  //bool using_default = 0;
  this->name = filename;

  string loadstr;
  loadstr = "resources/static/entities/" + filename + ".ent";

  istringstream file(loadTextAsString(loadstr));

  string temp;
  file >> temp;
  string spritefilevar;
  spritefilevar = "resources/static/sprites/" + temp + ".qoi";

  if(temp == "sp-no-texture") {
    spritefilevar = "resources/engine/sp-no-texture.qoi";
  }

  const char* spritefile = spritefilevar.c_str();
  float size;
  string comment;
  file >> comment;
  file >> size;

  //useNearestNeighbor
  file >> comment;
  file >> blurPixelsForScaling;

  file >> comment;
  file >> this->xagil;

  file >> comment; //turns_per_second
  file >> this->turningSpeed;


  file >> comment;
  file >> this->xmaxspeed;
  baseMaxSpeed = xmaxspeed;

  file >> comment;
  file >> this->friction;
  baseFriction = friction;

  file >> comment;
  float twidth, theight, tzeight;
  file >> twidth;
  file >> theight;
  file >> tzeight;
  bounds.width = twidth * 64;
  bounds.height = theight * 55;
  bounds.zeight = tzeight * 64;


  //bounds_offset
  file >> comment;
  file >> this->bounds.x;
  file >> this->bounds.y;

  file >> comment;
  file >> this->sortingOffset;
  this->baseSortingOffset = sortingOffset;
  file >> comment;
  file >> this->shadowSize;
  shadow = new cshadow(renderer, shadowSize);


  this->shadow->owner = this;


  file >> comment;
  file >> shadow->xoffset;
  file >> shadow->yoffset;

  int tempshadowSoffset;
  file >> comment;
  file >> tempshadowSoffset;

  file >> comment;
  file >> this->animspeed;
  file >> this->animlimit;

  file >> comment;
  file >> this->growFromFloor;

  file >> comment;
  file >> this->turnToFacePlayer;

  file >> comment;
  file >> this->framewidth;
  file >> this->frameheight;
  this->shadow->width = this->bounds.width * shadowSize;
  this->shadow->height = this->bounds.height * shadowSize;


  //bigger shadows have bigger sortingoffsets
  shadow->sortingOffset = 65 * (shadow->height / 44.4) + tempshadowSoffset;
  //sortingOffset += 8;

  file >> comment;
  file >> this->dynamic;
  int solidifyHim = 0;

  file >> comment;
  file >> solidifyHim;

  if(solidifyHim == 2) {
    //don't react to solid objects
    ignoreSolids = 1;
  }

  file >> comment;
  file >> semisolid;
  if(!g_loadingATM) {
    if(semisolid) {
      storedSemisolidValue = semisolid;
      semisolidwaittoenable = 1; //disabled semi
      semisolid = 0;
    }
  }

  file >> comment;
  file >> pushable;

  file >> comment;
  file >> navblock;

  file >> comment;
  file >> large;

  file >> comment;
  file >> boxy;

  if(large) {
    g_large_entities.push_back(this);
  }

  if(solidifyHim == 1) {
    this->solidify();
    this->canBeSolid = 1;
  }



  file >> comment;
  file >> this->rectangularshadow;
  if(rectangularshadow) {shadow->texture = g_shadowTextureAlternate;}

  file >> comment;
  file >> this->faction;
  
  if(faction != -1) {
    canFight = 1;
  } else {
    canFight = 0;
  }

  file >> comment;
  file >> this->weaponName;

  file >> comment;
  file >> this->agrod;

  file >> comment;
  file >> maxhp;
  hp = maxhp;

  file >> comment;
  file >> invincible;

  file >> comment;
  file >> cost;

  file >> comment;
  file >> essential;

  file >> comment;
  file >> this->persistentGeneral;

  file >> comment;
  file >> parentName;
  if(parentName != "null") {
    entity* hopeful = searchEntities(parentName);
    if(hopeful != nullptr) {
      this->isOrbital = 1;
      this->parent = hopeful;
      parent->children.push_back(this);

//      curwidth = width;
//      curheight = height;




    }
  }

  file >> comment;
  file >> orbitRange;

  file >> comment;
  file >> orbitOffset;


  if(canFight) {
    //check if someone else already made the attack
    //bool cached = 0;
    hisweapon = new weapon(weaponName, this->faction != 0);
  }



  //load dialogue file

  string txtfilename = "";
  //open from local folder first
  txtfilename = "resources/static/scripts/" + filename + ".txt";
  

  //load voice
  string voiceSTR = "resources/static/sounds/voice-normal.wav";
  voice = loadWav(voiceSTR.c_str());

  if(PHYSFS_exists(txtfilename.c_str())) {
    sayings = loadText(txtfilename);
    parseScriptForLabels(sayings);
  }

  //has another entity already loaded this texture
  for (auto x : g_entities) {
    if(x->name == this->name && !x->isWorlditem) {
      texture = x->texture;
      this->asset_sharer = 1;
    }
  }
  if(!asset_sharer) {

    //SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    //changing this in june 2023 to use linear interp on low res character sprites
    //this may have unwanted consequences for engine sprites, e.g. lighting
    if(!blurPixelsForScaling) {
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    } else {
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
    }

    texture = loadTexture(renderer, spritefile);
    if(texture == nullptr) {
      E("Error loading texture for entity " + name);
      D(spritefile);

    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
  }


  this->width = size * framewidth;
  this->height = size * frameheight;

  originalWidth = width;
  originalHeight = height;

  //move shadow to feet
  shadow->xoffset += width/2 - shadow->width/2;
  shadow->yoffset += height - shadow->height/2;



  this->bounds.x += width/2 - bounds.width/2;
  this->bounds.y += height - bounds.height/2;


  shadow->width += g_extraShadowSize;
  shadow->height += g_extraShadowSize * XtoY;

  shadow->xoffset -= 0.5 * g_extraShadowSize;
  shadow->yoffset -= 0.5 * g_extraShadowSize * XtoY;

  shadow->x = x + shadow->xoffset;
  shadow->y = y + shadow->yoffset;

  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);

  //make entities pop in unless this is a mapload
  if(!transition) {
    curwidth = 0;
    curheight = 0;
  } else {
    curwidth = width;
    curheight = height;
  }

  xframes = w / framewidth;
  yframes = h / frameheight;



  for (int j = 0; j < h; j+=frameheight) {
    for (int i = 0; i < w; i+= framewidth) {
      coord a;
      a.x = i;
      a.y = j;
      framespots.push_back(a);
    }
  }

  //if we don't have the frames just set anim to 0
  if(!(yframes > 3) ) {
    animation = 0;
    defaultAnimation = 0;
  }

  //disabled nodes underneath if we are set to (e.g. doors)
  


  g_entities.push_back(this);

  file >> comment;
  //spawn everything on spawnlist
  int overflow = 100;
  for(;;) {
    string line;
    file >> line;
    if(line == "}" || line == "") {break;}
    overflow--;
    if(overflow < 0) {E("Bad spawnlist."); break;}
    entity* a = new entity(renderer, line);
    a->dontSave = 1;
    spawnlist.push_back(a);
    if(a->parentName == this->name) {
      a->parent = this;
    }

    //trying to fix orbitals not appearing for the first frame of mapload
//    a->curheight = a->height;
//    a->curwidth = a->width;
  }

  //music and radius
  file >> comment;
  string musicname = "0";
  file >> musicname;
  string fileExistsSTR;
  if(musicname != "0") {
    fileExistsSTR = "resources/maps/" + g_mapdir + "/music/" + musicname + ".ogg";
    if(!fileExists(musicname)) {
      fileExistsSTR = "resources/static/music/" + musicname + ".ogg";
    }
    this->theme = Mix_LoadMUS(fileExistsSTR.c_str());
    g_musicalEntities.push_back(this);
  }
  file >> comment;
  file >> musicRadius;
  //convert blocks to worldpixels
  musicRadius *= 64;

  //worldsound
  file >> comment;
  //add worldsounds
  overflow = 100;
  for(;;) {

    string line;
    file >> line;
    if(line == "}" || line == "") {break;}
    overflow--;
    if(overflow < 0) {E("Bad soundlist."); break;}
    worldsound* a = new worldsound(line, 0, 0);
    this->mobilesounds.push_back(a);
    a->owner = this;
  }

  //script-on-contact
  file >> comment;
  string fieldScript = "0";
  file >> fieldScript;

  if(fieldScript != "0") {
    usesContactScript = 1;

    contactScriptCaller = new adventureUI(renderer, 1);
    contactScriptCaller->playersUI = 0;
    contactScriptCaller->talker = this;

    string txtfilename = "";
    txtfilename = "resources/static/scripts/" + fieldScript + ".txt";

    this->contactScript = loadText(txtfilename);
    parseScriptForLabels(contactScript);

    contactScriptCaller->ownScript = this->contactScript;
  }

  //script-on-contact-ms
  file >> comment;
  file >> contactScriptWaitMS;

  //animation config
  file >> comment;
  file >> animationconfig;

  //walk_frames
  file >> comment;
  file >> animWalkFrames;

  //walkMsPerSecond
  file >> comment;
  file >> walkAnimMsPerFrame;

  //identity
  file >> comment;
  file >> identity;

  //boardable?
  file >> comment;
  file >> isBoardable;

  //hidingspot
  file >> comment;
  file >> isHidingSpot;

  if(isBoardable) {
    g_boardableEntities.push_back(this);
  }

  //transport_ent
  file >> comment;
  file >> transportEnt;

  //transport_rate
  file >> comment;
  file >> transportRate;

  //script-on-boarding
  file >> comment;
  file >> boardingScriptName;

  if(boardingScriptName != "0") {
    usesBoardingScript = 1;
    string txtfilename = "";
    if (fileExists("resources/maps/" + g_mapdir + "/scripts/" + boardingScriptName + ".txt")) {
      txtfilename = "resources/maps/" + g_mapdir + "/scripts/" + boardingScriptName + ".txt";
    } else {
      txtfilename = "resources/static/scripts/" + boardingScriptName + ".txt";
    }
    D(name);
    D(txtfilename);

    boardingScript = loadText(txtfilename);
    parseScriptForLabels(boardingScript);

  }

  if(animationconfig == 0) {
    useAnimForWalking = 1;
  }
  
  if(animationconfig == 1) {
    useAnimForWalking = 0;
  }

  //load ai-data
  string AIloadstr;
  if(fileExists("resources/maps/" + g_mapdir + "/ai/" + filename + ".ai")) {
    this->isAI = 1;
    AIloadstr = "resources/maps/" + g_mapdir + "/ai/" + filename + ".ai";
  } else if(fileExists("resources/static/ai/" + filename + ".ai")) {
    this->isAI = 1;
    AIloadstr = "resources/static/ai/" + filename + ".ai";
  }
  if(this->isAI) {
    g_ai.push_back(this);
    //istringstream stream;
    //const char* plik = AIloadstr.c_str();
    //stream.open(plik);
    istringstream stream(loadTextAsString(AIloadstr));

    string line;
    string comment;

    stream >> comment; //abilities_and_radiuses_formate...
    stream >> comment; //abilities
    stream >> comment; // open curly brace character

    for(;;) {
      if(! (stream >> line) ) {break;}
      if(line[0] == '}') {break;}
      //line contains the name of an ability
      ability newAbility;
      newAbility.name = line;

      //they're stored as seconds in the configfile, but ms in the object
      float seconds = 0;
      stream >> seconds;
      newAbility.lowerRangeBound = seconds * 64;
      stream >> seconds;
      newAbility.upperRangeBound = seconds * 64;
      stream >> seconds;
      newAbility.lowerCooldownBound = seconds * 1000;
      stream >> seconds;
      newAbility.upperCooldownBound = seconds * 1000;


      char rsa;
      stream >> rsa;
      if(rsa == 'R') {
        newAbility.resetStableAccumulate = 0; //reset when out of range
      } else if (rsa == 'S') {
        newAbility.resetStableAccumulate = 1; //only charge when in range
      } else if (rsa == 'A') {
        newAbility.resetStableAccumulate = 2; //charge when out of range
      }


      newAbility.cooldownMS = (newAbility.lowerCooldownBound + newAbility.upperCooldownBound) / 2;
      this->myAbilities.push_back(newAbility);
    }

    //give us a way to call scripts if we have at least one script
//    if(hasAtleastOneScript) {
//      // !!! make another constructor that doesn't have a dialogbox
//      myScriptCaller = new adventureUI(renderer, 1);
//      myScriptCaller->playersUI = 0;
//      myScriptCaller->talker = this;
//    }


    stream >> comment; // states
    stream >> comment; // open curly brace character
    
    //a line might look like this 
    //  passive(1300, 3)
    //  duration, range in blocks
    //  the duration only decreases if the behemoth is within X blocks of the target
    //  if the range is zero, that means any range is suitable for decreasing the remaining duration
    for(;;) {
      if(! (stream >> line) ) {break;}
      if(line[0] == '}') {break;}
      //line contains the name of an ability
      state newState;
      string name;
      
      vector<string> x = splitString(line, '(');
      name = x[0];
      x[1].pop_back(); 

      string interval = x[1];
      
      stream >> line;
      line.pop_back();
      
      string blocks = line;

      newState.name = name;
      newState.interval = stoi(interval);
      newState.nextInterval = stoi(interval);
      newState.blocks = stoi(blocks) * 64;

      this->states.push_back(newState);
    }

    stream >> comment; // stateTransitions
    stream >> comment; // open curly brace character
    
    //a line might look like this 
    //  passive -> active (1)
    for(;;) {
      if(! (stream >> line) ) {break;}
      if(line[0] == '}') {break;}
      //line contains the name of an ability
      string fromState;
      string toState;
      string probability;
      
      fromState = line;
      stream >> comment; // "->"
      stream >> toState;
      stream >> probability;
      
      probability = probability.substr(1, probability.size()-2);

      int fromIndex = -1;
      int toIndex = -1;
      for(int i = 0; i < states.size(); i++) {
        if(fromState == states[i].name) {
          fromIndex = i;
        }
        if(toState == states[i].name) {
          toIndex = i;
        }
      }
      if(fromIndex == -1 || toIndex == -1) {
        E("Couldn't find state for stateTransition");
      } else {
        states[fromIndex].nextStates.push_back(toIndex);
        states[fromIndex].nextStateProbabilities.push_back(stof(probability));

      }
      
    }
    if(states.size() > 0) {useStateSystem = 1;}


    stream >> comment; //ai_index
    stream >> this->aiIndex;

    if(aiIndex == 0) {
      g_behemoth0 = this;
      g_behemoths.push_back(this);
    } else if(aiIndex == 1) {
      g_behemoth1 = this;
      g_behemoths.push_back(this);
    } else if(aiIndex == 2) {
      g_behemoth2 = this;
      g_behemoths.push_back(this);
    } else if(aiIndex == 3) {
      g_behemoth3 = this;
      g_behemoths.push_back(this);
    }


    if(!useAgro) {
      traveling = 1;
      readyForNextTravelInstruction = 1;
      poiIndex = aiIndex;
    }

    //ai index of -1 won't be a behemoth, for instance, but will still be ai (e.g. fleshpit, it can run a script to animate.)

    stream >> comment; //smell_radius
    stream >> smellAgroRadius;
    smellAgroRadius*= 64;

    stream >> comment; //smell_time
    stream >> maxSmellAgroMs;

    stream >> comment; //hearing_radius
    stream >> hearingRadius;
    hearingRadius *= 64;

    stream >> comment; //vision_radius
    stream >> visionRadius;
    visionRadius *= 64;

    stream >> comment; //vision_time
    stream >> visionTime;

    stream >> comment; //roam-rate
    stream >> roamRate;

    stream >> comment;
    stream >> minPatrolPerRoam;

    stream >> comment; //custom_movement
    stream >> customMovement;

    stream >> comment; //movement_type_switch_radius_blocks
    stream >> movementTypeSwitchRadius;
    movementTypeSwitchRadius *= 64;

    stream >> comment; //velocity_predictive_factor_for_navnodes
    stream >> velocityPredictiveFactor;

    stream >> comment; //bonus_speed
    stream >> bonusSpeed;

    stream >> comment; //target_faction
    stream >> targetFaction;

    stream >> comment; //useAgro_and_not_aggressiveness
    stream >> useAgro;


    stream >> comment; //aggressiveness_gain
    stream >> aggressivenessGain;
                       
    stream >> comment; //aggressiveness_loss
    stream >> aggressivenessLoss;

    stream >> comment; //aggressiveness_noise_gain
    stream >> aggressivenessNoiseGain;

    stream >> comment; //min_aggressiveness
    stream >> minAggressiveness;

    stream >> comment; //max_aggressiveness
    stream >> maxAggressiveness;

    //stream.close();

  }

  specialObjectsInit(this);

}

//copy constructor
//first intended for spawning in cannonballs
entity::entity(SDL_Renderer* renderer, entity* a) {
  this->growFromFloor = a->growFromFloor;
  this->texture = a->texture;
  this->asset_sharer = 1;
  this->missile = a->missile;
  this->xagil = a->xagil;
  this->turningSpeed = a->turningSpeed;
  this->xmaxspeed = a->xmaxspeed;
  this->grounded = a->grounded;
  this->baseMaxSpeed = a->baseMaxSpeed;
  this->friction = a->friction;
  this->baseFriction = a->baseFriction;
  this->animspeed = a->animspeed;
  this->animlimit = a->animlimit;
  this->framewidth = a->framewidth;
  this->frameheight = a->frameheight;
  this->xframes = a->xframes;
  this->yframes = a->yframes;
  this->framespots = a->framespots;
  this->dynamic = a->dynamic;
  this->canFight = a->canFight;
  this->usesContactScript = a->usesContactScript;
  this->contactScriptWaitMS = a->contactScriptWaitMS;
  this->contactReadyToProc = a->contactReadyToProc;
  this->contactScript = a->contactScript;
  this->faction = a->faction;
  this->targetFaction = a->targetFaction;
  this->shadowSize = a->shadowSize;
  this->hisweapon = a->hisweapon;
  this->visible = a->visible;
  this->name = a->name;
  this->tangible = a->tangible;
  this->width = a->width;
  this->height = a->height;
  this->zeight = a->zeight;
  this->sortingOffset = a->sortingOffset;
  this->bounds = a->bounds;
  cshadow* shadow = new cshadow(renderer, shadowSize);
  shadow->owner = this;
  this->shadow = shadow;
  this->shadow->width = a->shadow->width;
  this->shadow->height = a->shadow->height;
  this->shadow->sortingOffset = a->shadow->sortingOffset;
  this->shadow->xoffset = a->shadow->xoffset;
  this->shadow->yoffset = a->shadow->yoffset;
  this->shadow->x = a->shadow->x;
  this->shadow->y = a->shadow->y;

  

  g_entities.push_back(this);
}

      //for worlditems, load the ent file but use a texture by name
entity::entity(SDL_Renderer * renderer, int idk,  string texturename) {
  //M("entity()");
  sortingOffset = 16;
  ifstream file;
  //bool using_default = 0;
  this->name = texturename;
  isWorlditem = 1;
  this->faction = -1;
  string loadstr;

  //load from global folder
  loadstr = "resources/engine/worlditem.ent";
  const char* plik = loadstr.c_str();

  file.open(plik);



  string temp;
  file >> temp;
  string spritefilevar;

  //!!! do something else if there's none, use a generic image or smt
  spritefilevar = "resources/static/items/" + texturename + ".qoi";
  

  if(onionmode) {spritefilevar = "resources/engine/onion.qoi";}

  texture = loadTexture(renderer, spritefilevar);
  //M(spritefilevar );


  string comment;
  file >> comment;
  float size;
  file >> size;

  file >> comment;
  file >> this->xagil;


  file >> comment;
  file >> this->xmaxspeed;

  file >> comment;
  file >> this->friction;

  file >> comment;
  float twidth, theight, tzeight;
  file >> twidth;
  file >> theight;
  file >> tzeight;
  bounds.width = twidth * 64;
  bounds.height = theight * 55;
  bounds.zeight = tzeight * 32;


  file >> comment;
  file >> this->bounds.x;
  file >> this->bounds.y;

  file >> comment;
  file >> this->sortingOffset;

  file >> comment;
  float fsize;
  file >> fsize;

  shadow = new cshadow(renderer, fsize);
  if(rectangularshadow) {shadow->texture = g_shadowTextureAlternate;}

  this->shadow->owner = this;


  file >> comment;
  file >> shadow->xoffset;
  file >> shadow->yoffset;

  int tempshadowSoffset;
  file >> comment;
  file >> tempshadowSoffset;

  file >> comment;
  file >> this->animspeed;
  file >> this->animlimit;

  file >> comment;
  file >> growFromFloor;

  file >> comment;
  file >> this->turnToFacePlayer;

  file >> comment;
  file >> this->framewidth;
  file >> this->frameheight;
  this->shadow->width = framewidth * fsize;
  this->shadow->height = framewidth * fsize * (1/p_ratio);

  //bigger shadows have bigger sortingoffsets
  shadow->sortingOffset = 65 * (shadow->height / 44.4) + tempshadowSoffset;
  //sortingOffset = 8;


  file >> comment;
  file >> this->dynamic;
  int solidifyHim = 0;


  file >> comment;
  file >> solidifyHim;
  if(solidifyHim == 1) {
    this->solidify();
  }

  if(solidifyHim == 2) {
    //don't react to solid objects
    ignoreSolids = 1;
  }

  file >> comment;
  file >> semisolid;

  file >> comment;
  file >> navblock;

  file >> comment;
  file >> this->rectangularshadow;

  file >> comment;
  file >> this->faction;


  if(faction != 0) {
    canFight = 1;
  }

  file >> comment;
  file >> this->weaponName;

  file >> comment;
  file >> this->agrod;

  file >> comment;
  file >> maxhp;

  hp = maxhp;

  file >> comment;
  file >> invincible;

  file >> comment;
  file >> cost;

  file >> comment;
  file >> essential;
   
  this->width = size * framewidth;
  this->height = size * frameheight;

  //move shadow to feet
  shadow->xoffset += width/2 - shadow->width/2;
  shadow->yoffset += height - shadow->height/2;


  this->bounds.x += width/2 - bounds.width/2;
  this->bounds.y += height - bounds.height/2;


  shadow->width += g_extraShadowSize;
  shadow->height += g_extraShadowSize * XtoY;

  shadow->xoffset -= 0.5 * g_extraShadowSize;
  shadow->yoffset -= 0.5 * g_extraShadowSize * XtoY;



  int w, h;
  SDL_QueryTexture(texture, NULL, NULL, &w, &h);

  xframes = 1;
  yframes = 1;
  frame = 0;
  framewidth = w;
  frameheight = h;
  coord a;
  a.x = 0;
  a.y = 0;
  framespots.push_back(a);

  g_entities.push_back(this);
}


entity::~entity() {
  //M("~entity()" );
  if (!wallcap) {
    delete shadow;
  }

  if(!asset_sharer) {
    SDL_DestroyTexture(texture);
  }

  if(myScriptCaller!= nullptr){
    delete myScriptCaller;
  }

  for(auto ws : mobilesounds) {
    delete ws;
  }

  //delete hisweapon;
  //if this entity is talking or driving a script, a: the game is probably broken and b: we're about to crash

  if(this == g_talker && protag_is_talking) {
    g_forceEndDialogue = 1;
  }

  if(solid) {
    g_solid_entities.erase(remove(g_solid_entities.begin(), g_solid_entities.end(), this), g_solid_entities.end());
  }

  if(large) {
    g_large_entities.erase(remove(g_large_entities.begin(), g_large_entities.end(), this), g_large_entities.end());
  }

  if(identity == 1) {
    g_pellets.erase(remove(g_pellets.begin(), g_pellets.end(), this), g_pellets.end());
  }

  for(auto x : this->children) {
    x->tangible = 0;
  }

    //delete inventory
    for(auto x : inventory) {
      delete x.first;
    }
    inventory.clear();

  g_entities.erase(remove(g_entities.begin(), g_entities.end(), this), g_entities.end());

}


int entity::getOriginX() {
  if(!cachedOriginValsAreGood) {
    cachedOriginX = x + bounds.x + bounds.width/2;
    cachedOriginY = y + bounds.y + bounds.height/2;
    cachedOriginValsAreGood = 1;
  }
  return cachedOriginX;
  //return  x + bounds.x + bounds.width/2;
}

int entity::getOriginY() {
  if(!cachedOriginValsAreGood ) {
    cachedOriginX = x + bounds.x + bounds.width/2;
    cachedOriginY = y + bounds.y + bounds.height/2;
    cachedOriginValsAreGood = 1;
  }        
  return cachedOriginY;
  //return y + bounds.y + bounds.height/2;
}

void entity::setOriginX(float fx) {
  x = fx - bounds.x - bounds.width/2;
  cachedOriginValsAreGood = 0;
}

void entity::setOriginY(float fy) {
  y = fy - bounds.y - bounds.height/2;
  cachedOriginValsAreGood = 0;
}

rect entity::getMovedBounds() {
  return rect(bounds.x + x, bounds.y + y, z, bounds.width, bounds.height, bounds.zeight);
}

void entity::solidify() {
  //consider checking member field for solidness, and updating
  this->solid = 1;
  //shouldnt cause a crash anyway
  g_solid_entities.push_back(this);
}

void entity::unsolidify() {
    this->solid = 0;
  g_solid_entities.erase(remove(g_solid_entities.begin(), g_solid_entities.end(), this), g_solid_entities.end());
}

//entity render function
void entity::render(SDL_Renderer * renderer, camera fcamera) {
  
  opacity += opacity_delta;
  shadow->alphamod += opacity_delta;
  if(opacity_delta < 0 && opacity <= 0) {
    if(this->asset_sharer) {
      this->usingTimeToLive = 1;
      this->timeToLiveMs = -1;
    } else {
      this->tangible = 0;
    }
  } else if(opacity_delta > 0 && opacity >= 255) {
    this->opacity_delta = 0;
    this->opacity = 255;
    this->semisolid = 1; //for dungeon behemoths

  }

  if(opacity < 0) {
    SDL_SetTextureAlphaMod(texture, 0);
  } else {
    SDL_SetTextureAlphaMod(texture, opacity);
  }

  if(!tangible) {return;}
  if(!visible) {return;}

  if(this == protag) { g_protagHasBeenDrawnThisFrame = 1; }
  //if its a wallcap, tile the image just like a maptile
  
  rect obj;
  
    
  if(shrinking) {
    if(growFromFloor) {
      obj = rect(
          (floor(x) -fcamera.x + (originalWidth-floor(curwidth))/2)* 1 ,
          (floor(y) - ((floor(curheight) * (XtoY) + (originalHeight * (1-XtoY)))) - (floor(z) + floatheight) * XtoZ) - fcamera.y,
          floor(curwidth),
          floor(curheight)
          );
    } else {
      obj = rect(
          (floor(x) -fcamera.x + (originalWidth-floor(curwidth))/2)* 1 ,

          (floor(y) - ((floor(curheight) * (XtoY) + (originalHeight * (1-XtoY)))) 

           - ( 
               (originalHeight * (XtoY))
                -
               (curheight * XtoY)
             )*0.5

           - (floor(z) + floatheight) * XtoZ) - fcamera.y,

          floor(curwidth),
          floor(curheight)
          );
    }
  } else {

    if(growFromFloor) {
      obj = rect(
          (floor(x) -fcamera.x + (width-floor(curwidth))/2)* 1 ,
          (floor(y) - ((floor(curheight) * (XtoY) + (height * (1-XtoY)))) - (floor(z) + floatheight) * XtoZ) - fcamera.y,
          floor(curwidth),
          floor(curheight)
          );
    } else {
      obj = rect(
          (floor(x) -fcamera.x + (width-floor(curwidth))/2)* 1 ,

          (floor(y) - ((floor(curheight) * (XtoY) + (height * (1-XtoY)))) 

           - ( 
               (height * (XtoY))
                -
               (curheight * XtoY)
             )*0.5

           - (floor(z) + floatheight) * XtoZ) - fcamera.y,

          floor(curwidth),
          floor(curheight)
          );
    }
  }

  if(sizeRestoreMs > 0) {
    sizeRestoreMs -= elapsed;
    curwidth = (width + curwidth) / 2;
    curheight = (height + curheight) / 2;
  }


  rect cam(0, 0, fcamera.width, fcamera.height);


  if(RectOverlap(obj, cam)) {

    if(directionUpdateCooldownMs < 0) {

      //set visual direction
      if(
          (forwardsVelocity + forwardsPushVelocity > 0 && !this->wasPellet)
          ||
          this->forceAngularUpdate
          ) {
        animation = convertAngleToFrame(steeringAngle);
        flip = SDL_FLIP_NONE;
        if(yframes < 8) {
          if(animation == 5) {
            animation = 3;
            flip = SDL_FLIP_HORIZONTAL;
          } else if(animation == 6) {
            animation = 2;
            flip = SDL_FLIP_HORIZONTAL;
          } else if(animation == 7) {
            animation = 1;
            flip = SDL_FLIP_HORIZONTAL;
          }
  
          if(animation > 5 || animation < 0) {
            animation = 0;
          }
        }
  
        if(yframes < 2) {
          animation = 0;
        }
  
  
      }
      if(lastDirection != animation) {
        directionUpdateCooldownMs = maxDirectionUpdateCooldownMs;
      }
      lastDirection = animation;
    } else {
      directionUpdateCooldownMs -= elapsed;
    }

    hadInput = 0;



    frame = animation * xframes + frameInAnimation;
    SDL_FRect dstrect = { (float)obj.x, (float)obj.y, (float)obj.width, (float)obj.height};
    if(animationconfig == 2) {
      //this is used to share a texture between multiple sprites who really just use one frame of the texture, e.g. collectible familiars
      frame = animation * xframes + animWalkFrames;
    }
    //genericmode has just one frame
    if(isWorlditem) {frame = 0;}



    if(framespots.size() > 1) {
      //int spinOffset = 0;
      int framePlusSpinOffset = frame;


      if(framePlusSpinOffset >= framespots.size()) {
        framePlusSpinOffset = 0;
      } 

      SDL_Rect srcrect = {framespots[framePlusSpinOffset].x,framespots[framePlusSpinOffset].y, framewidth, frameheight};
      const SDL_FPoint center = {0 ,0};

      //color for statuseffects
      Uint8 rmod = 255; Uint8 gmod = 255; Uint8 bmod = 255; bool setColor = 0;
      if(flashingMS > 0) {
        gmod = 255 * (1-((float)flashingMS/g_flashtime));
        bmod = 255 * (1-((float)flashingMS/g_flashtime));
        //SDL_SetTextureColorMod(texture, 255, 255 * (1-((float)flashingMS/g_flashtime)), 255 * (1-((float)flashingMS/g_flashtime)));
      }

//      if(stunned && hisStatusComponent.stunned.statuses.size() > 0) {
//        rmod *= 0.5;
//        gmod *= 0.5;
//        bmod *= 0.5;
//      }

      if(marked) {
        rmod *= 0.9;
        gmod *= 0.8;
        bmod *= 1;
      }

      if(hisStatusComponent.poisoned.statuses.size() > 0) {
        rmod *= 0.83;
        gmod *= 1;
        bmod *= 0.89;
      }

      if(poisoned || poisonFlickerFrames > 0) {
        poisonFlickerFrames--;
        rmod *= 0.71;
        gmod *= 0.91;
        bmod *= 0.81;
      }

      Uint8 crmod = 0; Uint8 cgmod = 0; Uint8 cbmod = 0;
      SDL_GetTextureColorMod(texture, &crmod, &cgmod, &cbmod);
      if(crmod != rmod || cgmod != gmod || cbmod != bmod) {
        SDL_SetTextureColorMod(texture, rmod, gmod, bmod);
      }
      if(darkenMs > 0) {
        SDL_SetTextureColorMod(texture, darkenValue, darkenValue, darkenValue);
        darkenValue -= elapsed;
        if(darkenValue <0) {darkenValue = 0;}
      } else {
        if(darkenValue < 255) {
          darkenValue += 20;
          if(darkenValue > 255) {darkenValue = 255;}
          SDL_SetTextureColorMod(texture, darkenValue, darkenValue, darkenValue);
        } else {
          darkenValue = 255;
        }
      }

      if(texture != NULL) {
        SDL_RenderCopyExF(renderer, texture, &srcrect, &dstrect, 0, &center, flip);
      }
    } else {
      if(flashingMS > 0) {
        SDL_SetTextureColorMod(texture, 255, 255 * (1 - ((float)flashingMS/g_flashtime)), 255 * (1-((float)flashingMS/g_flashtime)));
      }

      if(darkenMs > 0) {
        SDL_SetTextureColorMod(texture, darkenValue, darkenValue, darkenValue);
        darkenValue -= elapsed;
        if(darkenValue <0) {darkenValue = 0;}
      } else {
        if(darkenValue < 255) {
          darkenValue += 20;
          if(darkenValue > 255) {darkenValue = 255;}
          SDL_SetTextureColorMod(texture, darkenValue, darkenValue, darkenValue);
        } else {
          darkenValue = 255;
        }
      }

      if(texture != NULL) {
        SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
      }
//      if(flashingMS > 0) {
//        SDL_SetTextureColorMod(texture, 255, 255, 255);
//      }
    }
  }

}

void entity::move_up() {
  //M("move_up()"); was having problems walking in devmode ;_;
  if(stunned) {return;}
  forwardsVelocity = (xagil * (100 - statusSlownPercent));
  //y-=xagil;
  //yaccel = -1* (xagil * (100 - statusSlownPercent));
  if(shooting) { return;}
  up = true;
  down = false;
  hadInput = 1;

  //M("move_up() completed");
}

void entity::stop_verti() {
  yaccel = 0;
  if(shooting) { return;}
  up = false;
  down = false;
}

void entity::move_down() {
  if(stunned) {return;}
  forwardsVelocity = (xagil * (100 - statusSlownPercent));
  //y+=xagil;
  //yaccel = (xagil * (100 - statusSlownPercent));
  if(shooting) { return;}
  down = true;
  up = false;
  hadInput = 1;
}

void entity::move_left() {
  if(stunned) {return;}
  forwardsVelocity = (xagil * (100 - statusSlownPercent));
  //x-=xagil;
  //xaccel = -1 * (xagil * (100 - statusSlownPercent));
  //x -= 3;
  if(shooting) { return;}
  left = true;
  right = false;
  hadInput = 1;
}

void entity::stop_hori() {
  xaccel = 0;
  if(shooting) { return;}
  left = false;
  right = false;
}

void entity::move_right() {
  if(stunned) {return;}
  forwardsVelocity = (xagil * (100 - statusSlownPercent));
  //x+=xagil;
  //xaccel = (xagil * (100 - statusSlownPercent));
  if(shooting) { return;}
  right = true;
  left = false;
  hadInput = 1;

}

void entity::shoot_up() {
  if(stunned) {return;}
  shooting = 1;
  up = true;
  down = false;
  hadInput = 1;
}

void entity::shoot_down() {
  if(stunned) {return;}
  shooting = 1;
  down = true;
  up = false;
  hadInput = 1;
}

void entity::shoot_left() {
  if(stunned) {return;}
  shooting = 1;
  left = true;
  right = false;
  hadInput = 1;
}

void entity::shoot_right() {
  if(stunned) {return;}
  shooting = 1;
  //xaccel = xagil;
  right = true;
  left = false;
  hadInput = 1;
}

// !!! horrible implementation, if you have problems with pathfinding efficiency try making this not O(n)
template <class T>
T* entity::Get_Closest_Node(vector<T*> array, int useVelocity) {
  float min_dist = 0;
  T* ret = nullptr;
  bool flag = 1;

  int cacheX = getOriginX();
  int cacheY = getOriginY();

  if(useVelocity) {
    cacheX += xvel * velocityPredictiveFactor; //this might need to be parametrized and unique to each behemoth
    cacheY += yvel * velocityPredictiveFactor;
  }

  //todo check for boxs
  if(array.size() == 0) {return nullptr;}
  for (long long unsigned int i = 0; i < array.size(); i++) {
    float dist = XYWorldDistanceSquared(cacheX, cacheY, array[i]->x, array[i]->y);
    if( (dist < min_dist || flag) && array[i]->enabled) {
      min_dist = dist;
      ret = array[i];
      flag = 0;
    }
  }
  return ret;
}

musicNode* entity::Get_Closest_Node(vector<musicNode*> array, int useVelocity) {
  float min_dist = 0;
  musicNode* ret = nullptr;
  bool flag = 1;

  int cacheX = getOriginX();
  int cacheY = getOriginY();

  if(useVelocity) {
    cacheX += xvel * velocityPredictiveFactor; //this might need to be parametrized and unique to each behemoth
    cacheY += yvel * velocityPredictiveFactor;
  }

  //todo check for boxs
  if(array.size() == 0) {return nullptr;}
  for (long long unsigned int i = 0; i < array.size(); i++) {
    float dist = XYWorldDistanceSquared(cacheX, cacheY, array[i]->x, array[i]->y);
    if( (dist < min_dist || flag) && array[i]->enabled) {
      min_dist = dist;
      ret = array[i];
      flag = 0;
    }
  }
  return ret;
}



//returns a pointer to a door that the player used
//entity update
door* entity::update(vector<door*> doors, float elapsed) {
        if(!tangible) {return nullptr;}

        if(target != nullptr) {
          this->distanceToTarget = XYWorldDistance(this->getOriginX(), this->getOriginY(), target->getOriginX(), target->getOriginY());
        }
        
        //unset these so they update
        cachedOriginValsAreGood = 0;
        
        if(usingTimeToLive) {
          timeToLiveMs -= elapsed;
        }

        for(auto t : mobilesounds) {
          t->x = getOriginX();
          t->y = getOriginY();
        }

        if(isOrbital) {
          this->z = parent->z -10 - (parent->height - parent->curheight);


          float angle = convertFrameToAngle(parent->animation, parent->flip == SDL_FLIP_HORIZONTAL);


          //orbitoffset is the number of frames, counter-clockwise from facing straight down
          float fangle = angle;
          fangle += (float)orbitOffset * (M_PI/4);
          fangle = fmod(fangle , (2* M_PI));

          this->setOriginX(parent->getOriginX() - cos(fangle) * orbitRange);
          this->setOriginY(parent->getOriginY() - sin(fangle) * orbitRange);

          this->sortingOffset = baseSortingOffset + sin(fangle) * 21 + 10 + (parent->height - parent->curheight);

          if(yframes == 8) {
            this->animation = convertAngleToFrame(fangle);
            this->flip = SDL_FLIP_NONE;
          } else {
            this->flip = parent->flip;
            this->animation = parent->animation;
            if(yframes == 1) {
              this->animation = 0;
            }
          }

          //I think I added this check for the ent's floor to let the protag walk up
          //slopes, but i'll disable it since i don't really use slopes (Nov 2023)
          //There's another one 1000 lines later

//          //update shadow
//          float heightfloor = 0;
//          layer = max(z /64, 0.0f);
//          layer = min(layer, (int)g_boxs.size() - 1);
//
//          //should we fall?
//          //bool should_fall = 1;
//          float floor = 0;
//          if(layer > 0) {
//            //!!!
//            rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
//            //rect thisMovedBounds = rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
//            for (auto n : g_boxs[layer - 1]) {
//              if(RectOverlap(n->bounds, thisMovedBounds)) {
//                floor = 64 * (layer);
//                break;
//              }
//            }
//            for (auto n : g_triangles[layer - 1]) {
//              if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
//                floor = 64 * (layer);
//                break;
//              }
//
//            }
//
//
//            float shadowFloor = floor;
//            floor = max(floor, heightfloor);
//
//            bool breakflag = 0;
//            for(int i = layer - 1; i >= 0; i--) {
//              for (auto n : g_boxs[i]) {
//                if(RectOverlap(n->bounds, thisMovedBounds)) {
//                  shadowFloor = 64 * (i + 1);
//                  breakflag = 1;
//                  break;
//                }
//              }
//              if(breakflag) {break;}
//              for (auto n : g_triangles[i]) {
//                if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
//                  shadowFloor = 64 * (i + 1);
//                  breakflag = 1;
//                  break;
//                }
//
//              }
//              if(breakflag) {break;}
//            }
//            if(breakflag == 0) {
//              //just use heightmap
//              shadowFloor = floor;
//            }
//            this->shadow->z = shadowFloor;
//          } else {
//            this->shadow->z = heightfloor;
//            floor = heightfloor;
//          }
          shadow->x = x + shadow->xoffset;
          shadow->y = y + shadow->yoffset;



          //return nullptr; 
          //i commented this out
          //to make biting work
        }


        if(msPerFrame != 0) {
          msTilNextFrame += elapsed;
          if(msTilNextFrame > msPerFrame && xframes > 1) {
            msTilNextFrame = 0; //should be msTilNextFrame - msPerFrame
           
            if(reverseAnimation) {
              frameInAnimation--;
              if(frameInAnimation < 0) {
                if(loopAnimation) {
                  if(scriptedAnimation) {
                    frameInAnimation = xframes - 1;
                  } else {
                    frameInAnimation = 0;
                  }
                } else {
                  frameInAnimation = 0;
                  msPerFrame = 0;
                  //!!! slightly ambiguous. open to review later
                  scriptedAnimation = 0;
                }
              }
            } else {
              frameInAnimation++;
              if(frameInAnimation == xframes || (useAnimForWalking && frameInAnimation == animWalkFrames + 1 && !scriptedAnimation)) {
                if(loopAnimation) {
                  if(scriptedAnimation) {
                    frameInAnimation = 0;
                  } else {
                    frameInAnimation = 1;
                  }
                } else {
                  frameInAnimation = xframes - 1;
                  msPerFrame = 0;
                  scriptedAnimation = 0;
                }
              }
            }
          }
        }
        if(animate && !transition && animlimit != 0) {
          curwidth = (curwidth * 0.8 + width * 0.2) * ((sin(animtime*animspeed))   + (1/animlimit)) * (animlimit);
          curheight = (curheight * 0.8 + height* 0.2) * ((sin(animtime*animspeed + PI))+ (1/animlimit)) * (animlimit);
          animtime += elapsed;
          if(this == protag && ( pow( pow(xvel,2) + pow(yvel, 2), 0.5) > 30 ) && (1 - sin(animtime * animspeed) < 0.01 || 1 - sin(animtime * animspeed + PI) < 0.01)) {
            if(footstep_reset && grounded) {
              footstep_reset = 0;
              if(1 - sin(animtime * animspeed) < 0.04) {
                //playSound(-1, g_footstep_a, 0);
              } else {
                //playSound(-1, g_footstep_b, 0);
              }


            }
          } else {
            footstep_reset = 1;
          }
        } else {
          animtime = 0;
          curwidth = curwidth * 0.8 + width * 0.2;
          curheight = curheight * 0.8 + height* 0.2;
        }


        //should we animate?
        if( (xaccel != 0 || yaccel != 0) || !grounded ) {
          animate = 1;
          if(useAnimForWalking) {
            if( (!scriptedAnimation) && grounded) {
              msPerFrame = walkAnimMsPerFrame;
            } else {
              msPerFrame = 0;
            }
          }
        } else {
          animate = 0;
          if(useAnimForWalking && !scriptedAnimation && this->useAnimForWalking) {
            msPerFrame = 0;
            frameInAnimation = 0;
          }
        }

        //should we enabled semisolidness? (for entities spawned after map-load far from protag)
        if(semisolidwaittoenable) {
          if(!CylinderOverlap(this->getMovedBounds(), protag->getMovedBounds())) {
            this->semisolid = storedSemisolidValue;
            this->semisolidwaittoenable = 0;
          }
        }

        if(this->usesContactScript) {
          this->curContactScriptWaitMS -= elapsed;
          if(curContactScriptWaitMS <= 0) {
            contactReadyToProc = 1;
            curContactScriptWaitMS = contactScriptWaitMS;
          } else {
            contactReadyToProc = 0;
          }
        }

        if(!dynamic && !CalcDynamicForOneFrame) { return nullptr; }
        //if(CalcDynamicForOneFrame) { M("Calct for one frame");}
        CalcDynamicForOneFrame = 0;

        //wait until turned to walk
        float thisTA = targetSteeringAngle;
        float thisSA = steeringAngle;
        float angularDiff = abs(thisTA - thisSA);
        if( angularDiff > M_PI/2) {
          //could it be a case of angle wrap causing problems?
          if(thisTA == 0) {
            thisTA = 2 * M_PI;
          }
          if(thisSA == 0) {
            thisSA = 2 * M_PI;
          }
          angularDiff = abs(thisTA - thisSA);
          if(angularDiff > M_PI/2) {
            forwardsVelocity = 0;
          }
        }

        forwardsPushVelocity *= 0.8;
        if(forwardsPushVelocity < 0) {
          forwardsPushVelocity = 0;
        }

        if(stunned) {forwardsVelocity = 0;}

        //set xaccel and yaccel from forwardsVelocity
        xaccel = cos(steeringAngle) * forwardsVelocity;
        yaccel = -sin(steeringAngle) * forwardsVelocity;

        //keep pellets moving
        if(this->wasPellet && this->usingTimeToLive) {
          this->steeringAngle = wrapAngle(atan2(protag->getOriginX() - this->getOriginX(), protag->getOriginY() - this->getOriginY()) - M_PI/2);
          this->targetSteeringAngle = this->steeringAngle;
        } else {
          forwardsVelocity = 0;
        }
 

        if(this == protag) {
          if(up) {
            if(left) {
              //up + left
              targetSteeringAngle = 3*M_PI/4;
  
            } else if(right) {
              //up + right
              targetSteeringAngle = M_PI/4;
  
            } else {
              //up
              targetSteeringAngle = M_PI/2;
  
            }
  
          } else if(down) {
            if(left) {
              //down + left
              targetSteeringAngle = 5 * M_PI/4;
  
            } else if(right) {
              //down + right
              targetSteeringAngle = 7 * M_PI/4;
  
            } else {
              //down
              targetSteeringAngle = 3 * M_PI/2;
  
            }
   
          } else if(left) {
            //left
            targetSteeringAngle = M_PI;
  
          } else if(right) {
            //right
            targetSteeringAngle = 0;
          }
        }

        if( (this != protag) || ((this == protag) && ((protag_can_move) && (up || down || left || right)))) {
          float amountToTurn = turningSpeed * elapsed/1000 * 2*M_PI;
          if(this == protag && g_spinning_duration > 0) {
            amountToTurn = 2 * M_PI;
          }
          if( abs(targetSteeringAngle - steeringAngle) < amountToTurn) {
            steeringAngle = targetSteeringAngle;
          } else {
            if(getTurningDirection(steeringAngle, targetSteeringAngle)) {
              steeringAngle += amountToTurn;
            } else {
              steeringAngle -= amountToTurn;
            }
          }
          steeringAngle = wrapAngle(steeringAngle);
        }

        if(1) {xmaxspeed = baseMaxSpeed + bonusSpeed;} else 
        {xmaxspeed = baseMaxSpeed;}

        //normalize accel vector
        float vectorlen = pow( pow(xaccel, 2) + pow(yaccel, 2), 0.5) /(xmaxspeed * (1 - statusSlownPercent));
        if(xaccel != 0) {
          xaccel /=vectorlen;
        }
        if(yaccel != 0) {
          yaccel /=vectorlen;
        }

        xaccel += cos(forwardsPushAngle) * forwardsPushVelocity;
        yaccel += -sin(forwardsPushAngle) * forwardsPushVelocity;

        if(yaccel != 0) {
          yaccel /= p_ratio;
        }

        if(xaccel > 0) {
          xvel += xaccel * ((double) elapsed / 256.0);
        }

        if(xaccel < 0) {
          xvel += xaccel * ((double) elapsed / 256.0);
        }

        if(yaccel > 0) {
          yvel += yaccel* ((double) elapsed / 256.0);
        }

        if(yaccel < 0) {
          yvel += yaccel* ((double) elapsed / 256.0);
        }

        //if this is the protag, and they're spinning, use saved xvel/yvel
        if(this == protag) {
          if( g_spinning_duration > 0) {
            xvel = g_spinning_xvel;
            yvel = g_spinning_yvel;
            protag->visible = 0;
            g_spin_entity->visible = 1;

          } else {
            protag->visible = 1;
            g_spin_entity->visible = 0;
            if(g_afterspin_duration > 0) {
              xvel = 0;
              yvel = 0;
              protag->visible = 1;
              g_spin_entity->visible = 0;
            }
          }
        }

        //we need to detect not if the entity will collide into solid objects with their current
        //velocity, but if they are within solid objects now
        //in that case, set velocity to zero and look for a way to "jiggle" the entity
        //to get them out
        vector<pair<int,int>> jiggleOptions = { 
                                                {0,0},
                                                {0,1},
                                                {0,-1},
                                                {1,0},
                                                {-1,0},
                                                {1,1},
                                                {-1,-1},
                                                {1,-1},
                                                {-1,1},
                                                {0,2},
                                                {0,-2},
                                                {2,0},
                                                {-2,0},
                                                {2,2},
                                                {-2,-2},
                                                {2,-2},
                                                {-2,2}
                                              };
        int jiggleOptionIndex = 0;
        if(boxsenabled && g_collisionResolverOn) {
          for(;;) {
            //does jiggleOptions[jiggleOptionIndex] work for us?
            int experimentalXOffset = jiggleOptions[jiggleOptionIndex].first;
            int experimentalYOffset = jiggleOptions[jiggleOptionIndex].second;
  
            bool inCollision = 0;
  
            //might be able to delete these two lines
            layer = max(z /64, 0.0f);
            layer = min(layer, (int)g_boxs.size() - 1);
  
            { //collision checking
              rect movedbounds = rect(bounds.x + x + experimentalXOffset, bounds.y + y + experimentalYOffset, bounds.width, bounds.height);
              movedbounds.z = z;
              movedbounds.zeight = bounds.zeight;
  
              //check for map collisions
              for (int i = 0; i < (int)g_boxs[layer].size(); i++) {
                //don't worry about boxes if we're not even close
                //no point for a sleepbox if we're only doing one check
                //rect sleepbox = rect(g_boxs[layer].at(i)->bounds.x - 150, g_boxs[layer].at(i)->bounds.y-150, g_boxs[layer].at(i)->bounds.width+300, g_boxs[layer].at(i)->bounds.height+300);
                //if(!RectOverlap(sleepbox, movedbounds)) {continue;}
                if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
                  inCollision = 1;
                  break;
                }
  
              }

              if(g_useSimpleImpliedGeometry) {
                for(auto x : g_impliedSlopes) {
                //rect sleepbox = rect(x->bounds.x - 150, x->bounds.y-150, x->bounds.width+300, x->bounds.height+300);
                //if(!RectOverlap(sleepbox, movedbounds)) {continue;}
                  if(RectOverlap(movedbounds, x->bounds)) {
                    inCollision = 1;
                    break;
                  }
                }

              }
  
              if(!inCollision) { //shortcut if already in collision
                //check with solid ents
                //potentially not perfect, since this doesn't consider their velocities, but
                //solid ents aren't really supposed to move
                for (auto n : g_solid_entities) {
                  if(this->ignoreSolids) {break;}
                  if(n == this) {continue;}
                  if(!n->tangible) {continue;}
                  //if(n->zeight < g_stepHeight) {continue;} //if you ever wanna solve the bug
                                                   //where the game keeps jiggling ents
                                                   //out of spiketraps, you should then 
                                                   //remove this line
                  //update bounds with new pos
                  rect thatmovedbounds = rect(n->bounds.x + n->x, n->bounds.y + n->y, n->bounds.width, n->bounds.height);
                  thatmovedbounds.z = n->bounds.z;
                  thatmovedbounds.zeight = n->bounds.zeight;

                  //uh oh, did we collide with something?
                  if(RectOverlap3d(movedbounds, thatmovedbounds)) {
                    if(movedbounds.z >= thatmovedbounds.z + thatmovedbounds.zeight) {
                    } else {
                      inCollision = 1;
                    }

                    break;
                  }
                }
              }
  
  
              //check for implied slopes
              //might be much better to just stop xcollide from being set to 1 if grounded
  //            if(!inCollision) { //shortcut if already in collision
  //              
  //              for(auto i : g_impliedSlopes) {
  //                rect simslope = rect(0,0,0,0);
  //                
  //                bool overlapY = RectOverlap(movedbounds, i->bounds);
  //    
  //                float heightFactor;
  //                if(i->bounds.z >= this->z) {
  //                  heightFactor = 1;
  //                } else if (this->z - 63 > i->bounds.z) {
  //                  heightFactor = 0;
  //                } else {
  //                  heightFactor = 1 - ((this->z - i->bounds.z) /63);
  //                }
  //    
  //                if(overlapY) {
  //                  rect simslope = rect(i->bounds.x, i->bounds.y + (16 * (1- heightFactor) ), i->bounds.width, i->bounds.height -  (64 * (1- heightFactor) ));
  //                  if(heightFactor < 0.95 && heightFactor > 0 && RectOverlap(simslope, movedbounds)) {
  //                    heightFactor = pow(heightFactor, 2);
  //    
  //                    if(heightFactor < 0.3) {
  //                      inCollision = 1;
  //                    }
  //                  }
  //                }
  //    
  //                if(heightFactor > 0) {
  //                  
  //                  if(overlapY) {
  //                    bool overlapX = RectOverlap(movedbounds, simslope);
  //      
  //                    if(overlapX) {
  //                      inCollision = 1;
  //                    }
  //      
  //                  } else {
  //                    movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)) , bounds.width, bounds.height);
  //                    bool overlapX = RectOverlap(movedbounds, i->bounds);
  //     
  //                    if(overlapX) {
  //                      inCollision = 1;
  //                    }
  //      
  //                  }
  //                }
  //              }
  //            }
  
  
            }
  
            if(!inCollision) {
              //cool, we checked everything and the entity isn't stuck inside a collision
              //now break and apply the jiggleOption to the ent's position, and they're free!
              break;
            }
   
  
            //try the next jiggleOption
            jiggleOptionIndex++;
            if(jiggleOptionIndex > jiggleOptions.size() -1) {
              if(g_showCRMessages) {
                E("Couldn't jiggle an entity, it's stuck in collisionboxes with solid entities/map geo.");
                D(this->name);
              }
              break;
  
            }
          }
  
          if(jiggleOptionIndex != 0 && jiggleOptionIndex < jiggleOptions.size() - 1) { //this ent will be moved some dist to get it out of collisions
            this->x += jiggleOptions[jiggleOptionIndex].first;
            this->y += jiggleOptions[jiggleOptionIndex].second;
  //          this->xvel = 0;
  //          this->yvel = 0;

//            if(g_showCRMessages) {
//              M("Jiggled an ent out of collisions with solid entities/map geo");
//              D(jiggleOptionIndex);
//            }
          }
        }

        rect movedbounds;
        bool ycollide = 0;
        bool xcollide = 0;

        int oxvel = xvel;
        int oyvel = yvel;
        

        //observed problem - sometimes the zombie gets stuck trying to go thru a narrow doorway
        //For instance, the zombie is trying to move left and up to pass thru a doorway
        //But the horizontal component of his velocity is much greater than the vertical component
        //the vertical component of velocity is negated due to friction
        //but the horizontal component is negated due to normal force, and thus, the zombie is eternally stuck
        //idea - if an entity's xcomp of velocity is reduced to zero, increase the y comp to their full agility value


        //turn off boxs if using the map-editor
        if(boxsenabled) {
          //..check door
          if(this == protag) {
            for (int i = 0; i < (int)doors.size(); i++) {
              //update bounds with new posj
              rect movedbounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y  + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
              //did we walk into a door?
              if(RectOverlap(movedbounds, doors[i]->bounds) && (protag->z > doors[i]->z && protag->z < doors[i]->z + doors[i]->zeight)  ) {
                //take the door.
                return doors[i];
              }
            }
          }
          groundedByEntity = 0;
          for (auto n : g_solid_entities) {
            if(this->ignoreSolids) {break;}
            if(n == this) {continue;}
            if(!n->tangible) {continue;}
            //update bounds with new pos
            rect thismovedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
            thismovedbounds.z = z;
            thismovedbounds.zeight = bounds.zeight;

            rect thatmovedbounds = rect(n->bounds.x + n->x, n->bounds.y + n->y, n->bounds.width, n->bounds.height);
            thatmovedbounds.z = n->z;
            thatmovedbounds.zeight = n->bounds.zeight;

            
            //uh oh, did we collide with something?
            if(RectOverlap3d(thismovedbounds, thatmovedbounds)) {
              if((this == protag || this->isAI || this->identity == 5) && (thatmovedbounds.z + thatmovedbounds.zeight) - z < g_stepHeight) {
                z += (thatmovedbounds.z + thatmovedbounds.zeight) - z;
              } else {
                ycollide = true;
                yvel = 0;
              }
            }

            //update bounds with new pos
            thismovedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
            thismovedbounds.z = z;
            thismovedbounds.zeight = bounds.zeight;

            //uh oh, did we collide with something?
            if(RectOverlap3d(thismovedbounds, thatmovedbounds)) {
              if((this == protag || this->isAI || this->identity == 5) && (thatmovedbounds.z + thatmovedbounds.zeight) - z < g_stepHeight) {
                z += (thatmovedbounds.z + thatmovedbounds.zeight) - z;
                
              } else {
                xcollide = true;
                xvel = 0;
              }
            }
          }

          vector<box*> boxesToUse = {};

          int usedCZ = 0;
          rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
          //see if we overlap any collisionzones
          for(auto x : g_collisionZones) {
            if(RectOverlap(movedbounds, x->bounds)) {
              usedCZ++;
              boxesToUse.insert(boxesToUse.end(), x->guests[layer].begin(), x->guests[layer].end());
            }
          }


          if(usedCZ == 0) {
            for (int i = 0; i < (int)g_boxs[layer].size(); i++) {

              //update bounds with new pos
              rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);

              //don't worry about boxes if we're not even close
              rect sleepbox = rect(g_boxs[layer].at(i)->bounds.x - 150, g_boxs[layer].at(i)->bounds.y-150, g_boxs[layer].at(i)->bounds.width+300, g_boxs[layer].at(i)->bounds.height+300);
              if(!RectOverlap(sleepbox, movedbounds)) {continue;}

              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
                ycollide = true;
                yvel = 0;




              }
              //update bounds with new pos
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
                //box detected
                xcollide = true;
                xvel = 0;

              }


            }
            for (int i = 0; i < (int)g_boxs[layer].size(); i++) {
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, g_boxs[layer].at(i)->bounds)) {
                //box detected
                xcollide = true;
                ycollide = true;
                xvel = 0;
                yvel = 0;
                continue;
              }
            }

            //now do that for implied slopes if I've chosen to treat them as regular collisions
            if(g_useSimpleImpliedGeometry) {
              for(auto n : g_impliedSlopes) {
                //update bounds with new pos
                rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
  
                //don't worry about boxes if we're not even close
                rect sleepbox = rect(n->bounds.x - 150, n->bounds.y-150, n->bounds.width+300, n->bounds.height+300);
                if(!RectOverlap(sleepbox, movedbounds)) {continue;}
  
                //uh oh, did we collide with something?
                if(RectOverlap(movedbounds, n->bounds)) {
                  ycollide = true;
                  yvel = 0;
                }
                //update bounds with new pos
                movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
                //uh oh, did we collide with something?
                if(RectOverlap(movedbounds, n->bounds)) {
                  //box detected
                  xcollide = true;
                  xvel = 0;
                }

                movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
                //uh oh, did we collide with something?
                if(RectOverlap(movedbounds, n->bounds)) {
                  //box detected
                  xcollide = true;
                  ycollide = true;
                  xvel = 0;
                  yvel = 0;
                  continue;
                }
  
  
  
  
                }
            }

          } else {
            for (int i = 0; i < (int)boxesToUse.size(); i++) {

              //update bounds with new pos
              rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);

              //don't worry about boxes if we're not even close
              rect sleepbox = rect(boxesToUse.at(i)->bounds.x - 150, boxesToUse.at(i)->bounds.y-150, boxesToUse.at(i)->bounds.width+300, boxesToUse.at(i)->bounds.height+300);
              if(!RectOverlap(sleepbox, movedbounds)) {continue;}

              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
                ycollide = true;
                yvel = 0;




              }
              //update bounds with new pos
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
                //box detected
                xcollide = true;
                xvel = 0;

              }


            }
            for (int i = 0; i < (int)boxesToUse.size(); i++) {
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
              //uh oh, did we collide with something?
              if(RectOverlap(movedbounds, boxesToUse.at(i)->bounds)) {
                //box detected
                xcollide = true;
                ycollide = true;
                xvel = 0;
                yvel = 0;
                continue;
              }
            }
          }

          //if we didnt use a cz, use all boxes



          //how much to push the player to not overlap with triangles.
          float ypush = 0;
          float xpush = 0;
          float jerk = -abs( pow(pow(yvel,2) + pow(xvel, 2), 0.5))/2; //how much to push the player aside to slide along walls
          //float jerk = -1;
          //!!! try counting how many triangles the ent overlaps with per axis and disabling jerking
          //if it is more than 1

          //and try setting jerk to a comp of the ent velocity to make the movement faster and better-scaling


          for(auto n : g_triangles[layer]){
            rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
            if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
              //if we move the player one pixel up will we still overlap?
              if(n->type == 3 || n->type == 2)  {
                xpush = jerk;
              }

              if(n->type == 0 || n->type == 1) {
                xpush = -jerk; ;
              }


              ycollide = true;
              yvel = 0;

            }

            movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
            if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
              //if we move the player one pixel up will we still overlap?
              if(n->type == 1 || n->type == 2) {
                ypush = jerk;
              }

              if(n->type == 0 || n->type == 3){
                ypush = -jerk;
              }

              xcollide = true;
              xvel = 0;

            }

            movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
            if(TriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
              xcollide = true;
              ycollide = true;
              xvel = 0;
              yvel = 0;
              continue;


            }

          }

          //test for triangular implied slopes
          if(g_useSimpleImpliedGeometry == 0) {
            for(auto n : g_impliedSlopeTris) {
              rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
  
              float heightFactor = 0;
  
              if(n->layer * 64 >= this->z) {
                heightFactor = 1;
              } else if (this->z - 63 > n->layer * 64) {
                heightFactor = 0;
              } else {
                heightFactor = 1 - ((this->z - n->layer * 64) /63);
              }
  
              //behaves as a triangle if heightFactor is nearly 1
              if(heightFactor > 0.95) {
    
                if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
                  //if we move the player one pixel up will we still overlap?
                  if(n->type == 3 || n->type == 2)  {
                    xpush = jerk;
                  }
    
                  if(n->type == 0 || n->type == 1) {
                    xpush = -jerk; ;
                  }
    
    
                  //ycollide = true;
                  yvel = 0;
    
                }
    
                movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
                if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
                  //if we move the player one pixel up will we still overlap?
                  if(n->type == 1 || n->type == 2) {
                    ypush = jerk;
                  }
    
                  if(n->type == 0 || n->type == 3){
                    ypush = -jerk;
                  }
    
                  //xcollide = true;
                  xvel = 0;
    
                }
    
                movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
                if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
  //                xcollide = true;
  //                ycollide = true;
                  xvel = 0;
                  yvel = 0;
                  //continue;
    
    
                }
              }
  
              //find modified bounds for player collision based on height
              //I guess, simulate this by moving movedbounds up based on heighFactor
              movedbounds.y -= (1-heightFactor) * 64;
  
              //push the player away 
              if(heightFactor > 0 && ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
  
                if(n->type == 1) {
                  this->y-=2;
                  this->x+=2;
                } else {
                  this->y-=2;
                  this->x-=2;
                }
  
                zvel = max(zvel, -1.0f);
              }
            } 
          } else {
            //treat t implied slopes just like tris
            for(auto n : g_impliedSlopeTris){
              rect movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
              if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
                //if we move the player one pixel up will we still overlap?
                if(n->type == 3 || n->type == 2)  {
                  xpush = jerk;
                }
  
                if(n->type == 0 || n->type == 1) {
                  xpush = -jerk; ;
                }
  
  
                ycollide = true;
                yvel = 0;
  
              }
  
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y, bounds.width, bounds.height);
              if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
                //if we move the player one pixel up will we still overlap?
                if(n->type == 1 || n->type == 2) {
                  ypush = jerk;
                }
  
                if(n->type == 0 || n->type == 3){
                  ypush = -jerk;
                }
  
                xcollide = true;
                xvel = 0;
  
              }
  
              movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
              if(ITriRectOverlap(n, movedbounds.x, movedbounds.y, movedbounds.width, movedbounds.height)) {
                xcollide = true;
                ycollide = true;
                xvel = 0;
                yvel = 0;
                continue;
  
  
              }
  
            }

          }


          //test for implied slopes
          if(g_useSimpleImpliedGeometry == 0) {
            for(auto i : g_impliedSlopes) {
              rect movedbounds;
              rect simslope = rect(0,0,0,0);
              movedbounds = rect(bounds.x + x, bounds.y + y  + (yvel * ((double) elapsed / 256.0)), bounds.width, bounds.height);
              
              bool overlapY = RectOverlap(movedbounds, i->bounds);
  
              float heightFactor;
              if(i->bounds.z >= this->z) {
                heightFactor = 1;
              } else if (this->z - 63 > i->bounds.z) {
                heightFactor = 0;
              } else {
                heightFactor = 1 - ((this->z - i->bounds.z) /63);
              }
  
  
              float yDiff = 0; //this is used to compare the ycoords of the ent and the slope, so if the ent has lower y than the slope, we can just treat it as a wall
              yDiff = (this->y + bounds.y + this->bounds.height) - i->bounds.y;
  
  
              if(overlapY) {
                rect simslope = rect(i->bounds.x, i->bounds.y + (16 * (1- heightFactor) ), i->bounds.width, i->bounds.height -  (64 * (1- heightFactor) ));
                if(heightFactor < 0.95 && heightFactor > 0 && RectOverlap(simslope, movedbounds)) {
                  heightFactor = pow(heightFactor, 2);
                  //yvel = -1 * ( (y + bounds.y + this->bounds.height) - (i->bounds.y + (64 * (1- heightFactor) )) );
  
                  float difference = ( (y + bounds.y + this->bounds.height) - (i->bounds.y + (8 * (1- heightFactor) )) );
                  //y += -1 * difference;
                  //yvel = -1 * difference;
                  y-=2;
                  yvel = 0;
                  xaccel = 0;
                  if(heightFactor < 0.3) {
                    if(this->z > 1) {
                      xcollide = 1;
                    }
                  }
  
  
                  //zaccel -= -1;
                  zvel = max(zvel, -1.0f);
                  //ycollide = 1;
  
                  rect movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y  , bounds.width, bounds.height);
                }
              }
  
              if(heightFactor > 0) {
                
                if(overlapY) {
                  ycollide = 1;
                  movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)) , bounds.width, bounds.height);
                  bool overlapX = RectOverlap(movedbounds, simslope);
    
                  if(overlapX) {
                    if(this->z > 1 || yDiff < 10) {
                      xcollide = 1;
                    }
                  }
    
                } else {
                  movedbounds = rect(bounds.x + x + (xvel * ((double) elapsed / 256.0)), bounds.y + y + (yvel * ((double) elapsed / 256.0)) , bounds.width, bounds.height);
                  bool overlapX = RectOverlap(movedbounds, i->bounds);
   
                  if(overlapX) {
                    if(this->z > 1 || yDiff > 10) {
                      xcollide = 1;
                    }
                  }
    
                }
              }
            } 
          }
          yvel += ypush;
          xvel += xpush;
        }


        if((xcollide || ycollide) && ( pow( pow(oxvel,2) + pow(oyvel, 2), 0.5) > 30 )) {
          if(fragileMovement) {
            timeToLiveMs = -1;
            usingTimeToLive = 1;
          }

          //detect stuckness
          stuckTime++;
        } else {
          stuckTime = 0;
        }

        specialObjectsBump(this, xcollide, ycollide);

        if(!ycollide && !transition) {
          y+= yvel * ((double) elapsed / 256.0);
        }

        //when coordinates are bungled, it isnt happening here
        if(!xcollide && !transition) {
          x+= xvel * ((double) elapsed / 256.0);
        }

        if(slowSeconds > 0) {
          slowSeconds -= elapsed/1000;
          friction = baseFriction * slowPercent;
        } else {
          friction = baseFriction;
        }


        if(grounded) {
          yvel *= pow(friction, ((double) elapsed / 256.0));
          xvel *= pow(friction, ((double) elapsed / 256.0));
          stableLayer = layer;
        } else {
          yvel *= pow(friction*this->currentAirBoost, ((double) elapsed / 256.0));
          xvel *= pow(friction*this->currentAirBoost, ((double) elapsed / 256.0));
          this->currentAirBoost += (g_deltaBhopBoost * ((double) elapsed / 256.0)) * min(pow( pow(x,2) + pow(y, 2), 0.2) / xmaxspeed, 1.0);
          if(this->currentAirBoost > g_maxBhoppingBoost) {
            this->currentAirBoost = g_maxBhoppingBoost;
          }
          // !!! make this not suck
        }

        float heightfloor = 0; //filled with floor z from heightmap
        if(g_heightmaps.size() > 0 /*&& update_z_time < 1*/) {
          bool using_heightmap = 0;
          bool heightmap_is_tiled = 0;
          tile* heighttile = nullptr;
          int heightmap_index = 0;

          rect tilerect;
          rect movedbounds;
          //movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, this->bounds.width, this->bounds.height);
          movedbounds = rect(this->getOriginX(), this->getOriginY(), 0, 0);
          //get what tile entity is on
          //this is poorly set up. It would be better if heightmaps were assigned to tiles, obviously, with a pointer
          bool breakflag = 0;

          for (int i = (int)g_tiles.size() - 1; i >= 0; i--) {
            if(g_tiles[i]->fileaddress == "textures/marker.qoi") {continue; }
            tilerect = rect(g_tiles[i]->x, g_tiles[i]->y, g_tiles[i]->width, g_tiles[i]->height);

            if(RectOverlap(tilerect, movedbounds)) {
              for (int j = 0; j < (int)g_heightmaps.size(); j++) {
                //M("looking for a heightmap");
                //e(g_tiles[i]->fileaddress);
                if(g_heightmaps[j]->name == g_tiles[i]->fileaddress) {
                  //M("found it");
                  heightmap_index = j;
                  using_heightmap = 1;
                  breakflag = 1;
                  heightmap_is_tiled = g_tiles[i]->wraptexture;
                  heighttile = g_tiles[i];
                  break;
                }
              }
              //current texture has no mask, keep looking
            }
          }



          update_z_time = max_update_z_time;
          //update z position
          SDL_Color rgb = {0, 0, 0};
          heightmap* thismap = g_heightmaps[heightmap_index];
          Uint8 maxred = 0;
          if(using_heightmap) {
            //try each corner;
            //thismap->image->w;
            //code for middle
            //Uint32 data = thismap->heighttilegetpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + 0.5 * this->width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - 0.5 * this->bounds.height) % thismap->image->h);
            Uint32 data;
            if(heightmap_is_tiled) {
              data = thismap->getpixel(thismap->image, (int)(getOriginX()) % thismap->image->w, (int)(getOriginY()) % thismap->image->h);
            } else {
              //tile is not tiled, so we have to get clever with the heighttile pointer

              data = thismap->getpixel(thismap->image, (int)( ((this->getOriginX() - heighttile->x) /heighttile->width) * thismap->image->w), (int)( ((this->getOriginY() - heighttile->y) /heighttile->height) * thismap->image->h));
            }
            SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);
            if(RectOverlap(tilerect, movedbounds)) {
              maxred = rgb.r;
            }
            // //code for each corner:
            // Uint32 data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
            // SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);

            // movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0), 1, 1);
            // if(RectOverlap(tilerect, movedbounds)) {
            // 	maxred = max(maxred, rgb.r);
            // }

            // data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0)) % thismap->image->h);
            // SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);

            // movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0), 1, 1);
            // if(RectOverlap(tilerect, movedbounds)) {
            // 	maxred = max(maxred, rgb.r);
            // }

            // data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0)) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
            // SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);

            // movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0), this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
            // if(RectOverlap(tilerect, movedbounds)) {
            // 	maxred = max(maxred, rgb.r);
            // }

            // data = thismap->getpixel(thismap->image, (int)(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width) % thismap->image->w, (int)(this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height) % thismap->image->h);
            // SDL_GetRGB(data, thismap->image->format, &rgb.r, &rgb.g, &rgb.b);

            // movedbounds = rect(this->x + xvel * ((double) elapsed / 256.0) + this->bounds.width, this->y + yvel * ((double) elapsed / 256.0) - this->bounds.height, 1, 1);
            // if(RectOverlap(tilerect, movedbounds)) {
            // 	maxred = max(maxred, rgb.r);
            // }

          }
          //oldz = this->z;
          if(using_heightmap) {
            heightfloor = ((maxred * thismap->magnitude));
          }

        } else {
          //update_z_time--;
          //this->z = ((oldz) + this->z) / 2 ;

        }

        layer = max(z /64, 0.0f);
        layer = min(layer, (int)g_boxs.size() - 1);
        //should we fall?
        //bool should_fall = 1;
        float floor = 0;
        if(layer > 0) {
          //!!!
          rect thisMovedBounds = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
          //rect thisMovedBounds = rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
          for (auto n : g_boxs[layer - 1]) {
            if(RectOverlap(n->bounds, thisMovedBounds)) {
              floor = 64 * (layer);
              break;
            }
          }
          for (auto n : g_triangles[layer - 1]) {
            if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
              floor = 64 * (layer);
              break;
            }

          }


          float shadowFloor = floor;
          floor = max(floor, heightfloor);

          bool breakflag = 0;
          for(int i = layer - 1; i >= 0; i--) {
            for (auto n : g_boxs[i]) {
              if(RectOverlap(n->bounds, thisMovedBounds)) {
                shadowFloor = 64 * (i + 1);
                breakflag = 1;
                break;
              }
            }
            if(breakflag) {break;}
            for (auto n : g_triangles[i]) {
              if(TriRectOverlap(n, thisMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
                shadowFloor = 64 * (i + 1);
                breakflag = 1;
                break;
              }

            }
            if(breakflag) {break;}
          }
          if(breakflag == 0) {
            //just use heightmap
            shadowFloor = floor;
          }
          this->shadow->z = shadowFloor;
        } else {
          this->shadow->z = heightfloor;
          floor = heightfloor;
        }

        //try ramps?
        //!!! can crash if the player gets too high

        if(layer < g_layers) {
          for(auto r : g_ramps[this->layer]) {
            rect a = rect(r->x, r->y, 64, 55);
            rect movedBounds = rect(bounds.x + x, bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
            if(RectOverlap(movedBounds, a)) {
              if(r->type == 0) {
                //contribute to protag z based on how far we are along
                //the y axis
                float push = (55 - abs((((float)movedBounds.y - (float)r->y ))))/55;

                float possiblefloor = r->layer * 64 + 64 * push;
                if( abs(this->z - possiblefloor ) < 15) {
                  floor = possiblefloor;
                  this->shadow->z = floor + 1;
                }

              } else {
                if(r->type == 1) {
                  float push = (64 - abs((( (float)(movedBounds.x + movedBounds.width) - (float)(r->x + 64) ))))/64;

                  float possiblefloor = r->layer * 64 + 64 * push;
                  if( abs(this->z - possiblefloor ) < 15) {
                    floor = possiblefloor;
                    this->shadow->z = floor + 1;
                  }
                } else {
                  if(r->type == 2) {
                    //contribute to protag z based on how far we are along
                    //the y axis
                    float push = (55 - abs((( (float)(movedBounds.y + movedBounds.height) - (float)(r->y + 55) ))))/55;

                    float possiblefloor = r->layer * 64 + 64 * push;
                    if( abs(this->z - possiblefloor ) < 15) {
                      floor = possiblefloor;
                      this->shadow->z = floor + 1;
                    }

                  } else {
                    float push = (64 - abs((( (float)(movedBounds.x) - (float)(r->x) ))))/64;

                    float possiblefloor = r->layer * 64 + 64 * push;
                    if( abs(this->z - possiblefloor ) < 15) {
                      floor = possiblefloor;
                      this->shadow->z = floor + 1;
                    }
                  }
                }
              }
            }
          }
        }

        //look for solid entities (new as of Nov 2023 for the bed)
        if(this == protag || this->isAI) { //seems to be causing slowdown
          groundedByEntity = 0;
          for(auto n : g_solid_entities) {
            if(this->ignoreSolids) {break;}

            if(XYWorldDistanceSquared(this->x, this->y, n->x, n->y) < 102400) //arbitrary distance chosen for optimization, it indicates a origin-to-origin distance of five blocks which is usually enough
            {
              rect thisb = rect(bounds.x + x + xvel * ((double) elapsed / 256.0), bounds.y + y + yvel * ((double) elapsed / 256.0), bounds.width, bounds.height);
              thisb.z = z;
              thisb.zeight = bounds.zeight;
  
              rect that = rect(n->bounds.x + n->x -2, n->bounds.y + n->y-2, n->bounds.width+1, n->bounds.height+1);
              that.z = n->z;
              that.zeight = n->bounds.zeight;
  
  
              if(RectOverlap(thisb,that) && (thisb.z >= that.z + that.zeight)) {
                floor = that.z + that.zeight + 0.1;
                this->shadow->z = floor + 1;
  
              }
            }
          }
        }
 

        if(z > floor + 1) {
          if(useGravity) {
            zaccel -= g_gravity * ((double) elapsed / 256.0);
          }
          grounded = 0;
        } else {
          // !!! maybe revisit this to let "character" entities have fallsounds
          if(grounded == 0 && this == protag && g_boardingCooldownMs < 0) {
            //need to check if we landed in a boardable entity
            
            rect thisMovedBounds = rect(this->bounds.x + this->x, this->bounds.y + this->y, this->bounds.width, this->bounds.height);
            for(auto n : g_boardableEntities) {
              if(n->tangible && n->grounded) {
                rect thatMovedBounds = {n->bounds.x + n->x, n->bounds.y + n->y, n->bounds.width, n->bounds.height};
                if(RectOverlap(thisMovedBounds, thatMovedBounds) && (abs(this->z - n->z) < 32 )) {
                  //M("Protag boarded entity named " + n->name);
                  
                  //decide to de-agro entities
//                  if(n->isHidingSpot) {
//                    for(auto x:g_ai) {
//                      if(x->target == protag) {
//                        //I want a "realistic" or clever way of de-agroing enemies
//                        //I could make it so that there's a timer after fomm is last in listening range of an agrod enemy
//                        if(!g_protagIsInHearingRange) {
//                          //x->poiIndex = 0;
//                          // i want to have a fun way of having the enemy de-agro
//                          // I want them to approach the hiding spot and stare at it for a moment
//                          // before a "?" appears above their head and they then wander off
//
//                          //this makes them instantly give up the moment the protag hides
//                          x->agrod = 0;
//                          x->smellAgroMs = 0;
//                          x->target = nullptr;
//
//                          //x->myTravelstyle = patrol;
//                          //x->traveling = 1;
//                          //x->readyForNextTravelInstruction = 1;
//                          M("begin losthim sequence");
//                          
//                          x->lostHimSequence = 1;
//                          
//                          //we want to pick the node closest to the edge of the hs,
//                          //in the direction of the behemoth
//                          //so get the angle from the hs to the behemoth
//                          
//                          float angleToBehemoth = atan2(x->getOriginX() - n->getOriginX(), x->getOriginY() - n->getOriginY()) - M_PI/2;
//                          angleToBehemoth = wrapAngle(angleToBehemoth);
//
//                          float interestingX = n->getOriginX() + cos(angleToBehemoth) * n->bounds.width/2;
//                          float interestingY = n->getOriginY() + sin(angleToBehemoth) * n->bounds.width/2;
//
//                          Destination = getNodeByPosition(x->lostHimX, x->lostHimY);
//                          x->lostHimX = n->getOriginX();
//                          x->lostHimY = n->getOriginY();
//                        }
//                      }
//                    }
//                  }

                  if(n->transportEntPtr != nullptr) {
                    g_boardedEntity = n->transportEntPtr;
                    g_formerBoardedEntity = n;
                    g_transferingByBoardable = 1;
                    g_maxTransferingByBoardableTime = XYWorldDistance(n->getOriginX(), n->getOriginY(), g_boardedEntity->getOriginX(), g_boardedEntity->getOriginY()) / n->transportRate;
                    g_transferingByBoardableTime = 0;
                  } else {
                    g_boardedEntity = n;
                    g_transferingByBoardable = 0;
                  }


                  g_protagIsWithinBoardable = 1;
                  g_msSinceBoarding = 0;
                  protag->tangible = 0;
                  smokeEffect->happen(n->getOriginX(), n->getOriginY(), protag->z, 0);
                  break;
                }
              }

            }

            //play landing sound
            //playSound(-1, g_land, 0);

            if(!storedJump) {
              //penalize the player for not bhopping
              if(protagConsecutiveBhops > 3) {
                protag->slowPercent = g_jump_afterslow;
                protag->slowSeconds = g_jump_afterslow_seconds;
                protag->currentAirBoost = g_defaultBhoppingBoost;
              }
              protagConsecutiveBhops = 0;

            } else {
              protagConsecutiveBhops++;
            }
          }
          grounded = 1;
          zvel = max(zvel, 0.0f);
          zaccel = max(zaccel, 0.0f);

        }

        if(groundedByEntity) { grounded = 1;}


        zvel += zaccel * ((double) elapsed / 256.0);

        if(this == protag && zvel < 75 && g_jumpGaranteedAccelMs < 0 ) {
          if(!input[8]) {
            if(zvel > 0) {zvel = 0;}
          }
        }

        //for banish animation from scripts
        if(this->banished && zvel <= 0) {
          this->dynamic = 0;
          //SDL_SetTextureAlphaMod(this->texture, 127);
          this->opacity = 127;
          //if we are solid, disable nodes beneath
          if(this->canBeSolid) {

            for(auto x : overlappedNodes) {

              x->enabled = 1;
              // for(auto y : x->friends) {
              // 	y->enabled = 1;
              // }
            }

          }
          return nullptr;
        }

        //zvel *= pow(friction, ((double) elapsed / 256.0));
        z += zvel * ((double) elapsed / 256.0);
        z = max(z, floor + 1);


        z = max(z, heightfloor);
        layer = max(z /64, 0.0f);
        layer = min(layer, (int)g_boxs.size() - 1);

        shadow->x = x + shadow->xoffset;
        shadow->y = y + shadow->yoffset;

        //is out scriptcaller sleeping? Lets update it, and maybe wake it up
        if(myScriptCaller != nullptr) {
          if(myScriptCaller->sleepflag){
            if(myScriptCaller->sleepingMS > 1) { myScriptCaller->sleepingMS -= elapsed;}
            else {myScriptCaller->sleepflag = 0;myScriptCaller->continueDialogue();}
          }
        }

        //and what about our contactscriptcaller?
        if(contactScriptCaller != nullptr) {
          if(contactScriptCaller->sleepflag){
            if(contactScriptCaller->sleepingMS > 1) { contactScriptCaller->sleepingMS -= elapsed;}
            else {contactScriptCaller->sleepflag = 0;contactScriptCaller->continueDialogue();}
          }
        }

        //update combat
        if(isWorlditem) {return nullptr;}

        //hisweapon->comboResetMS+=elapsed;

        if(shooting) {
          //spawn shot.
          shoot();
        }

        //check for everyone, even if they are invincible
        if(1) {
          //check for projectile box
          vector<projectile*> projectilesToDelete;
          for(auto x : g_projectiles) {
            rect thatMovedBounds = rect(x->x + x->bounds.x, x->y + x->bounds.y, x->bounds.width, x->bounds.height);
            thatMovedBounds.z = x->z;
            thatMovedBounds.zeight = thatMovedBounds.width * XtoZ;
            rect thisMovedBounds = rect(this->x + bounds.x, y + bounds.y, bounds.width, bounds.height);
            thisMovedBounds.z = this->z;
            thisMovedBounds.zeight = this->bounds.zeight;
            if(x->owner->faction != this->faction && RectOverlap3d(thatMovedBounds, thisMovedBounds)) {

              //destroy projectile
              projectilesToDelete.push_back(x);

              if(!invincible) {
                //take damage
                this->hp -= x->gun->damage;
                this->flashingMS = g_flashtime;
                if(this->faction != 0) {
                  //playSound(1, g_enemydamage, 0);
                } else {
                  if(this == protag) {
                    //playSound(2, g_playerdamage, 0);
                  } else {
                    //playSound(3, g_npcdamage, 0);
                  }
                }
              }

              //if(this->weaponName == "unarmed") {break;} //some entities should be able to target themselves (fleshpit)

              //under certain conditions, agro the entity hit and set his target to the shooter
              if(target == nullptr && useAgro) {
              target = x->owner;
              targetFaction = x->owner->faction;
                agrod = 1;

              //agro all of the boys on this's team who aren't already agrod, and set their target to a close entity from x's faction
              //WITHIN A RADIUS, because it doesnt make sense to agro everyone on the map.

              for (auto y : g_entities) {
                if(y->tangible && y != this && y->faction == this->faction && (y->agrod == 0 || y->target == nullptr) && XYWorldDistance(y->x, y->y, this->x, this->y) < g_earshot && y->useAgro) {
                  y->targetFaction = x->owner->faction;
                  y->agrod = 1;
                }
              }

              
            }
          }
          for(auto x:projectilesToDelete) {
            delete x;
          }
        }


        //alert nearby friends who arent fighting IF we are agrod
        if(agrod && useAgro) {
          for (auto y : g_entities) {
            if(y->tangible && y != this && y->faction == this->faction && (y->agrod == 0 || y->target == nullptr) && XYWorldDistance(y->x, y->y, this->x, this->y) < g_earshot) {
              if(y->useAgro) {
                y->agrod = 1;
  
                if(this->target != nullptr) {
                  y->targetFaction = this->targetFaction;
                  if(y->target == nullptr) {
                    y->target = this->target;
                  }
                }
              }
            }
          }
        }

        if(isAI && useAgro) {
          //check the auto-agro-range
          potentialTarget = nullptr;
          smellsPotentialTarget = 0;
          seesPotentialTarget = 0;
          if(target == nullptr) {

            //could make this a bit nicer by splitting into finding a potential target and progressing towards detection for that target instead of having them together like this
            for(auto x : g_entities) {
              if(x->faction == this->targetFaction) {
                //detect instantly, as if by hearing breath
                if(XYWorldDistance(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY()) < 4 * 64) {
                  if(LineTrace(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY(), false, 30, 0, 10, false)) 
                  {
                    if(!devMode 
                        && 
                        !g_ninja
                        &&
                        !g_protagIsWithinBoardable
                        )  {
                      M("I felt fomm's breath!");
                      //close and have LOS - detection
                      this->target = x;
                      this->traveling = 0;
                      this->agrod = 1;
                    }
                  }
                
                }

                //can it smell fomm?
                if(XYWorldDistance(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY()) < this->smellAgroRadius) {

                  { //check for smell
                    potentialTarget = x;
                    smellsPotentialTarget = 1;
                    if(potentialTarget == protag) {
                      
                      g_protagIsBeingDetectedBySmell = 1;
                    }
                  }
                }

                //can it see fomm?
                if(XYWorldDistance(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY()) < this->visionRadius) {

                  if(
                      LineTrace(x->getOriginX(), x->getOriginY(), this->getOriginX(), this->getOriginY(), false, 30, 0, 10, false)
                      &&
                      x->tangible


                      ) 
                  {
                    if(x == protag && g_protagIsWithinBoardable) { break;}
                    if(opacity != 255) {break;}

                    //is the monster facing the target? compare angle to target to this steeringangel

                    float angleToPotTarget = atan2(x->getOriginX() - getOriginX(), x->getOriginY() - getOriginY()) - M_PI/2;
                    angleToPotTarget = wrapAngle(angleToPotTarget);
                    if(abs(angleDiff(angleToPotTarget, this->steeringAngle)) < 1.6) {
                      potentialTarget = x;
                      if(potentialTarget == protag) {
                        g_protagIsBeingDetectedBySight = 1;
                      }
                      seesPotentialTarget = 1;
                      break;
                    }

                    


                  }
                }
              }
            }


          } else {
            //if we are agrod on the player, check if we are perceiving him
            perceivingProtag = 0;
            if(target == protag) {
              //check smell
              if(XYWorldDistance(protag->getOriginX(), protag->getOriginY(), this->getOriginX(), this->getOriginY()) < this->smellAgroRadius) {

                { //check for smell
                  perceivingProtag = 1;
                }
              }



              //check sight
              if(XYWorldDistance(protag->getOriginX(), protag->getOriginY(), this->getOriginX(), this->getOriginY()) < this->visionRadius) {
                if(
                    LineTrace(protag->getOriginX(), protag->getOriginY(), this->getOriginX(), this->getOriginY(), false, 30, 0, 10, false)
                    &&
                    protag->tangible
                    ) 
                {
                  if(!g_protagIsWithinBoardable) {
  
                    //is the monster facing the target? compare angle to target to this steeringangel
                    float angleToPotTarget = atan2(protag->getOriginX() - getOriginX(), protag->getOriginY() - getOriginY()) - M_PI/2;
                    angleToPotTarget = wrapAngle(angleToPotTarget);
                    if(abs(angleDiff(angleToPotTarget, this->steeringAngle)) < 1.6) {
                      perceivingProtag = 1;
                    }
                  }
                }
              }

            }
            if(!perceivingProtag) {
              c_deagroMs += elapsed;
            } else {
              c_deagroMs -= elapsed;
            }
            if(c_deagroMs > deagroMs) {
              agrod = 0;
              traveling = 1;
              readyForNextTravelInstruction = 1;
              target = nullptr;
              M("I deagrod of fomm");
            } else if(c_deagroMs < 0) {
              c_deagroMs = 0;

            }
          }

          //this is so it takes time to agro on someone
          if(smellsPotentialTarget && !devMode) {
            smellAgroMs += elapsed;
          } else {
            smellAgroMs -= elapsed;
          }

          if(smellAgroMs >= maxSmellAgroMs && potentialTarget != nullptr && !devMode && !g_ninja) {
            M("I smelled fomm!");
            this->traveling = 0;
            this->target = potentialTarget;
            this->agrod = 1;
          }


          if(smellAgroMs < 0) {
            smellAgroMs = 0;
          }

          if(seesPotentialTarget && !devMode) {
            seeAgroMs += elapsed;
          } else {
            seeAgroMs -= elapsed;
          }

          if(seeAgroMs >= visionTime && potentialTarget != nullptr && !devMode && !g_ninja) {
            M("I saw fomm!");
            this->traveling = 0;
            this->target = potentialTarget;
            this->agrod = 1;
          }

          if(seeAgroMs < 0) {
            seeAgroMs = 0;
          }

//          //is out scriptcaller sleeping? Lets update it, and maybe wake it up
//          if(myScriptCaller != nullptr) {
//            if(myScriptCaller->sleepflag){
//              if(myScriptCaller->sleepingMS > 1) { myScriptCaller->sleepingMS -= elapsed;}
//              else {myScriptCaller->sleepflag = 0;myScriptCaller->continueDialogue();}
//            }
//          }




        } else if(isAI && !useAgro) {
          //HORROR STALKER NERVOUS AGGRESSIVE
          //This is the code for having fun enemies who vary in aggressiveness
          //prototypically, aggressiveness makes them faster, do more damage, and spend less time wandering
          if(target == nullptr) {
            //get the target
            for(auto x : g_entities) {
              if(x->faction == targetFaction) {
                target = x;
                break;
              }
            }
          }

          if(target != nullptr) {
            smellsPotentialTarget = 0;
            seesPotentialTarget = 0;
            //can it smell fomm?
            if(distanceToTarget < this->smellAgroRadius) {
              smellsPotentialTarget = 1;
            }

            //can it see fomm?
            if(distanceToTarget < this->visionRadius) {
              if(
                  LineTrace(target->getOriginX(), target->getOriginY(), this->getOriginX(), this->getOriginY(), false, 30, 0, 10, false)
                  &&
                  target->tangible
                  ) 
              {
                if(target != protag || !g_protagIsWithinBoardable) {
                  //is the monster facing the target? compare angle to target to this steeringangel
                  float angleToPotTarget = atan2(target->getOriginX() - getOriginX(), target->getOriginY() - getOriginY()) - M_PI/2;
                  angleToPotTarget = wrapAngle(angleToPotTarget);
                  if(abs(angleDiff(angleToPotTarget, this->steeringAngle)) < 1.6) {
                    seesPotentialTarget = 1;
                  }
                }
              }
            }

            //can it hear fomm?

            //if we are using the state system, let's update that
            if(useStateSystem) {
              
              if(distanceToTarget < states[activeState].blocks || states[activeState].blocks == 0) {
                states[activeState].interval -= elapsed;
              }

              if(states[activeState].interval < 0) {
                int numChoices = states[activeState].nextStates.size();
                float random = (double)rand() / RAND_MAX;
                float sumOfOdds = 0;
                int destinationState = 0;
                for(int i = 0; i < states[activeState].nextStates.size(); i++) {
                  float chance = states[activeState].nextStateProbabilities[i];
                  sumOfOdds += chance;
                  if(sumOfOdds >= random) { 
                    destinationState = states[activeState].nextStates[i];
                    break;
                  }
                }
                //cout << "State change from " << activeState << " to " << destinationState << endl;
                states[activeState].interval = states[activeState].nextInterval;
                activeState = destinationState;
              }
            }
          }
          
          //aggressiveness is frozen if the player is hiding
          if(!g_protagIsWithinBoardable) {
            if(seesPotentialTarget && opacity == 255) {

              if(target == protag) {
                g_protagIsBeingDetectedBySight = 1;
              }
              aggressiveness += elapsed * aggressivenessGain;
            }
            if(hearsPotentialTarget) {
              aggressiveness += elapsed * aggressivenessNoiseGain;
            }
            if(smellsPotentialTarget) {
              aggressiveness += elapsed * aggressivenessGain;
            }
  
            if(!seesPotentialTarget && !hearsPotentialTarget && !smellsPotentialTarget) {
              aggressiveness -= elapsed * aggressivenessLoss;
            }
          }

          if(aggressiveness < minAggressiveness) {
            aggressiveness = minAggressiveness;
          } else if(aggressiveness > maxAggressiveness) {
            aggressiveness = maxAggressiveness;
          }
        }

        if(isAI) {
          //likely has abilities to use
          //are any abilities ready?
          for(auto &x : myAbilities) {
            if(x.ready == 1) {continue;}
            
            //accumulate-abilities will always decrease CD
            if(x.resetStableAccumulate == 2) {
              x.cooldownMS -= elapsed;
            }
            if(target == nullptr) {if(x.resetStableAccumulate == 0) {x.cooldownMS = x.upperCooldownBound;}}

            float inRange = 0;
            float dist = std::numeric_limits<float>::max();

            if(target != nullptr) {
              dist = distanceToTarget;
            }

            if((dist <= x.upperRangeBound  && dist >= x.lowerRangeBound) || x.upperRangeBound == x.lowerRangeBound && x.lowerRangeBound == 0) {
              inRange = 1;
              if(x.resetStableAccumulate != 2) {
                x.cooldownMS -= elapsed;
              }
            } else {
              //reset-abilities are reset when out of range
              if(x.resetStableAccumulate == 0) {
                x.cooldownMS = (x.lowerCooldownBound + x.upperCooldownBound)/2;
              }
            }

            if(x.cooldownMS < 0) {
              //are we in range to the player?
              if(inRange) {
                x.ready = 1;
              }
            }
          }

        }


        //apply statuseffect
        this->stunned = hisStatusComponent.stunned.updateStatuses(elapsed);
        if(this->stunned) {stop_hori(); stop_verti();}
        this->marked = hisStatusComponent.marked.updateStatuses(elapsed);
        this->disabled = hisStatusComponent.disabled.updateStatuses(elapsed);
        this->enraged = hisStatusComponent.enraged.updateStatuses(elapsed);
        this->buffed = hisStatusComponent.buffed.updateStatuses(elapsed);
        
        
        int damageFromPoison = round(hisStatusComponent.poisoned.updateStatuses(elapsed));
        this->poisoned = damageFromPoison;
        if(this->poisoned) {poisonFlickerFrames = 6;}
        this->hp -= damageFromPoison;

        int healthFromHealen = round(hisStatusComponent.healen.updateStatuses(elapsed));
        this->healen = healthFromHealen;
        this->hp += healthFromHealen;
        if(this->hp > this->maxhp) {this->hp = this->maxhp;}
        this->statusSlownPercent =  hisStatusComponent.slown.updateStatuses(elapsed);
        if(statusSlownPercent > 1) {statusSlownPercent = 1;}

        hisStatusComponent.invincible.updateStatuses(elapsed);

        this->hisStatusComponent.stunned.cleanUpStatuses();
        this->hisStatusComponent.marked.cleanUpStatuses();
        this->hisStatusComponent.disabled.cleanUpStatuses();
        this->hisStatusComponent.enraged.cleanUpStatuses();
        this->hisStatusComponent.buffed.cleanUpStatuses();
        this->hisStatusComponent.poisoned.cleanUpStatuses();
        this->hisStatusComponent.healen.cleanUpStatuses();
        this->hisStatusComponent.slown.cleanUpStatuses();
        this->hisStatusComponent.invincible.cleanUpStatuses();


        //check if he has died
        if(hp <= 0) {
          if(this != protag) {
            tangible = 0;
            return nullptr;
          }
        }





        //push him away from close entities
        //if we're even slightly stuck, don't bother
        if(this->dynamic && this->pushable && elapsed > 0) {

          for(auto x : g_entities) {
            if(this->isAI && x->isAI) {continue;} //behemoths don't collide
            if(this == x) {continue;}

            //entities with a semisolid value of 2 are only solid to the player
            bool solidfits = 0;
  
            //this is not a silly check
            if(this == protag) {
              solidfits = x->semisolid;
            } else {
              solidfits = (x->semisolid == 1);
            }

            //entities with a semisolid value of 3 are semisolid with a square collision
            bool semisolidsquare = x->semisolid == 3;
            if(semisolidsquare) {
              bool m = RectOverlap(this->getMovedBounds(), x->getMovedBounds());
              if(m && x->tangible && solidfits) {
                float r = pow( max(Distance(getOriginX(), getOriginY(), x->getOriginX(), x->getOriginY()), (float)10.0 ), 2);
                float mag =  30000/r;
                if(identity == 29) {
                  mag = 0.3/r;
                }
                float xdif = (this->getOriginX() - x->getOriginX());
                float ydif = (this->getOriginY() - x->getOriginY());
                float len = pow( pow(xdif, 2) + pow(ydif, 2), 0.5);
                float normx = xdif * 0.1;
                float normy = ydif * 0.1;
  
                if(!isnan(mag * normx) && !isnan(mag * normy)) {
                  xvel += normx * mag;
                  yvel += normy * mag;
                }


              }

            } else {

              bool m = CylinderOverlap(this->getMovedBounds(), x->getMovedBounds());
  
  
              if(solidfits && x->tangible && m) {
                //push this one slightly away from x
                float r = pow( max(Distance(getOriginX(), getOriginY(), x->getOriginX(), x->getOriginY()), (float)10.0 ), 2);
                float mag =  30000/r;
                float xdif = (this->getOriginX() - x->getOriginX());
                float ydif = (this->getOriginY() - x->getOriginY());
                float len = pow( pow(xdif, 2) + pow(ydif, 2), 0.5);
                float normx = xdif/len;
                float normy = ydif/len;
  
                if(!isnan(mag * normx) && !isnan(mag * normy)) {
                  xvel += normx * mag;
                  yvel += normy * mag;
                }
  
              } else if(m && x->usesContactScript && x->contactReadyToProc && x->tangible && this->faction != x->faction && !x->contactScriptCaller->executingScript) {
                x->contactScriptCaller->dialogue_index = -1;
                x->target = this;
                x->sayings = x->contactScript;
                x->contactScriptCaller->continueDialogue();
              }
            }
          }
        }

        flashingMS -= elapsed;
        darkenMs -= elapsed;
        if(this->inParty) {
          return nullptr;
        }

//        if(!canFight) {
//          return nullptr;
//        }

        //shooting ai
        if(canFight && agrod) {
          //do we have a target?
          if(target != nullptr) {
            //check if target is still valid
            if(target->hp <= 0 || (!target->tangible && !(target == protag && g_protagIsWithinBoardable))) {
              //can we get a new target from the same faction that we are agrod against?
              bool setNewTarget = 0;
              for(auto x : g_entities) {
                if(x->tangible && x->faction == target->faction && LineTrace(this->getOriginX(), this->getOriginY(), x->getOriginX(), x->getOriginY(), false, 30, this->layer, 10, 0)) {
                  target = x;
                  setNewTarget = 1;
                  break;
                }
              }
              if(!setNewTarget){
                for(auto x : g_entities) {
                  if(x->tangible && x->faction == target->faction) {
                    target = x;
                    setNewTarget = 1;
                    break;
                  }
                }
              }
              if(!setNewTarget) {
                target = nullptr;
                //agrod = 0;
                //I'd like to check if there are entities we can't see but we can path too but its so much work :(

                Destination = nullptr;
              }
            } else {
              //re-evaluate target if someone else has delt high damage to us recently

              //this constant is what factor of the range must be had to the player
              //in these types of games, humans seem to shoot even when they are out
              // of range, so lets go with that
              shooting = 0;
              //float distanceToTarget = XYWorldDistance(target->getOriginX(), target->getOriginY(), getOriginX(), getOriginY());
//              if(distanceToTarget < this->hisweapon->attacks[hisweapon->combo]->range) {
//                shooting = 1;
//              }

              float xvector;
              float yvector;
              //bool recalcAngle = 0; //should we change his angle in the first place?
              //combatrange is higher than shooting range because sometimes that range is broken while a fight is still happening, so he shouldnt turn away

              //now that we have a direction, shoot
              if(shooting) {
                shoot();
              }

            }
          } else {
            //another placeholder - target is protag
            if(faction != 0) {
              extern entity* protag;
              target = protag;
            }
            if(targetFaction != -1) {
              //set target to first visible enemy from target faction
              for(auto x : g_entities) {
                if(x->faction == this->targetFaction ) {
                  //found an entity that we have vision to
                  this->target = x;
                }
              }
            }
          }
        } else if(dynamic) {

          //code for becoming agrod when seeing a hostile entity will go here
          //face the direction we are moving
          //bool recalcAngle = 0; //should we change his angle in the first place?
          //combatrange is higher than shooting range because sometimes that range is broken while a fight is still happening, so he shouldnt turn away

          //here's the code for roaming/patrolling
          //we aren't agrod.
          if(traveling) {
            if(readyForNextTravelInstruction && g_setsOfInterest.at(poiIndex).size() != 0) {
              readyForNextTravelInstruction = 0;

              float r = ((double) rand() / RAND_MAX);
              if(curPatrolPerRoam < minPatrolPerRoam) {
                myTravelstyle = patrol;
                curPatrolPerRoam++;
                //M("Can't roam now");
              } else {
                if(r >= roamRate) {
                  myTravelstyle = patrol;
                } else {
                  myTravelstyle = roam;
                  curPatrolPerRoam = 0;
                }
              }
              if(myTravelstyle == roam && g_setsOfInterest.size() > poiIndex) {
                //generate random number corresponding to an index of our poi vector
                int random = rand() % ((int)g_setsOfInterest.at(poiIndex).size() - 1);
                pointOfInterest* targetDest = g_setsOfInterest.at(poiIndex).at(random);
                if(random == lastTargetDestIndex) {
                  random++;
                  targetDest = g_setsOfInterest.at(poiIndex).at(random);
                }
                if(random == lastTargetDestIndex) {
                  //M("They're equal, this shouldn't happen");
                } 
//                else if(random < lastTargetDestIndex) {
//                  patrolDirection = 1; //patrol decreasingly
//                  M("New direction is Clock.");
//                } else {
//                  patrolDirection = 0; //patrol increasingly
//                  M("New direction is Ounter.");
//                }
                //set new direction to clock if it's faster to go clockwise
                int numPoiNodes = ((int)g_setsOfInterest.at(poiIndex).size());

                //get the distance by counting upwards (ounter)
                int ounterDistance = 0;
                int thisSpot = lastTargetDestIndex;
                while(thisSpot != random) {
                  thisSpot++;
                  ounterDistance++;
                  if(thisSpot > numPoiNodes) {thisSpot = 0;}
                }

                int clockDistance = numPoiNodes - ounterDistance;

                if(ounterDistance < clockDistance) {
                  patrolDirection = 0;
                  //M("New Direction is Ounter");
                } else {
                  patrolDirection = 1;
                  //M("New Direction is Clock");

                }


                lastTargetDestIndex = random;
                currentPoiForPatrolling = random;
                Destination = getNodeByPosition(targetDest->x, targetDest->y);
              } else if(myTravelstyle == patrol) {
                if(patrolDirection == 0) {
                  currentPoiForPatrolling++;
                  //M("Going Ounter...");
                } else {
                  currentPoiForPatrolling--;
                  //M("Going Clock...");
                }
                if(currentPoiForPatrolling > (int)g_setsOfInterest.at(poiIndex).size()-1) {currentPoiForPatrolling = 0;}
                if(currentPoiForPatrolling < 0) {currentPoiForPatrolling = g_setsOfInterest.at(poiIndex).size()-1;}

                pointOfInterest* targetDest = g_setsOfInterest.at(poiIndex).at(currentPoiForPatrolling);
                lastTargetDestIndex = currentPoiForPatrolling;
                Destination = getNodeByPosition(targetDest->x, targetDest->y);
              }
            } else {
              //should we be ready for our next travel-instruction?
              if(Destination == nullptr) {
                Destination = getNodeByPosition(getOriginX(), getOriginY());
              }
              if(Destination != nullptr && XYWorldDistance(this->getOriginX(), this->getOriginY(), Destination->x, Destination->y) < 32) {
                readyForNextTravelInstruction = 1;
              }
            }

            BasicNavigate(Destination);
          }
        }



        float dist = XYWorldDistanceSquared(this->getOriginX(), this->getOriginY(), protag->getOriginX(), protag->getOriginY());
        if(dist < g_entitySleepDistance || this->isAI) {
          specialObjectsUpdate(this, elapsed);
        }

        if(this->missile) {
          // missile movment
          if(target !=  nullptr && target->tangible) {
            int rangeToUse = 0;
            
            if(g_protagIsWithinBoardable) {
              rangeToUse = 64 * 3;
            } else {
              rangeToUse = this->hisweapon->attacks[hisweapon->combo]->range;
            }

            if( XYWorldDistance(this->getOriginX(), this->getOriginY(), target->getOriginX(), target->getOriginY()) > rangeToUse) {

              angleToTarget = atan2(target->getOriginX() - getOriginX(), target->getOriginY() - getOriginY()) - M_PI/2;
              angleToTarget = wrapAngle(angleToTarget);
              targetSteeringAngle = angleToTarget;
              forwardsVelocity = xagil;
  
            } else {
              //stop if in range
              forwardsVelocity = 0;
              forceAngularUpdate = 1;
            }

          } else if (target == nullptr) {
            forwardsVelocity = xagil;
          }

          
        } else if (target != nullptr && agrod) {
          // monster movement 
          
          float distToTarget = 10000;
          angleToTarget = atan2(target->getOriginX() - getOriginX(), target->getOriginY() - getOriginY()) - M_PI/2;
          angleToTarget = wrapAngle(angleToTarget);
          distToTarget = XYWorldDistance(this->getOriginX(), this->getOriginY(), target->getOriginX(), target->getOriginY());
          g_dijkstraEntity = this;

          //chase if custommovement type is 0 or we are within movementTypeSwitchRadius
          //to target
          if(customMovement == 0 || distToTarget < movementTypeSwitchRadius)
          { //blindrun movement
            
            if(( (LineTrace(this->getOriginX(), this->getOriginY(), target->getOriginX(), target->getOriginY(), false, 64 + 32, this->layer, 10, true) )  || (distToTarget < 180) ) ) {
            //just walk towards the target, need to use range to stop walking if we are at target (for friendly npcs)
            targetSteeringAngle = angleToTarget;
            
            int rangeToUse = 0;
            
            if(g_protagIsWithinBoardable) {
              rangeToUse = 64 * 3;
            } else {
              rangeToUse = 128;
            }
  
            if( distToTarget > rangeToUse) {
              forwardsVelocity = xagil;
            } else {
              //stop if in range
              forwardsVelocity = 0;
              forceAngularUpdate = 1;
            }

            //not calling BasicNavigate, so we need to set nearby nodes to indicate that they're being used
            //we have LOS so this trick really isn't a hack
            
            int xval = getOriginX();
            int yval = getOriginY();
//            int xval = getOriginX() + 15 * xvel;
//            int yval = getOriginY() + 15 * yvel;
            navNode* closestNode = getNodeByPos(g_navNodes, xval, yval);
            if(closestNode != nullptr) {
              for(auto u : closestNode->friends) {
                u->costFromUsage = 1000000;
              }
            }

  
            //recalculate current when we lose los
            current = nullptr;
            dest = nullptr;
            Destination = nullptr;
            timeSinceLastDijkstra = -1;
            justLostLosToTarget = 1;
            path.clear();

          } else {
            if(Destination != nullptr) {
              BasicNavigate(Destination);
            }
            justLostLosToTarget = 0;
          }
        } else if( customMovement == 1) 
        { //precede
          
          //BasicNavigate();
        } else if(customMovement == 2)
        { //corner
          if(Destination != nullptr) {
            BasicNavigate(Destination);
          }
        }


        //walking ai
        if(agrod) {
          if(timeSinceLastDijkstra - elapsed < 0) {
            //need to update our Destination member variable, dijkstra will
            //be called this frame
            if(target != nullptr && (target->tangible || target == protag && g_protagIsWithinBoardable)) {
              //requirements for a valid place to navigate to
              //must be
              //1 - in range
              //2 - have LOS to player (we can even check if the attack pierces walls later)
              //3 - not be a wall
              //hopefully thats enuff, because i dont really want to make an algorithm
              //to check if a spot is easy to walk to, but its possible.

              //how will we write the algorithm?
              //check the cardinal points of the player having 2/3 of the range
              //thats eight places to check.
              //start with the closest first, otherwise every dijkstra call the AI
              //would basically walk thru the player between these cardinal points
              //if the closest cardinal place fails the second or third requirements
              //try testing another
              //if none work, (e.g. player is standing in the middle of a small room)
              //repeat with half of 2/3 of the range. it might just be better to have them navigate
              //to the player

              //DEPRECATED - use frame to get prefered cardinal point
              //NOW - use angle to target to get prefered cardinal point
              int index = convertAngleToFrame(angleToTarget);

              vector<int> ret;
              if(this->hisweapon->attacks[hisweapon->combo]->melee)  {
                ret = getCardinalPoint(target->getOriginX(), target->getOriginY(), 0, index);
               
                Destination = getNodeByPosition(ret[0], ret[1]);
              } else {
                ret = getCardinalPoint(target->getOriginX(), target->getOriginY(), this->hisweapon->attacks[hisweapon->combo]->range, index);
      
                if( LineTrace(ret[0], ret[1], target->getOriginX(), target->getOriginY(), false, 30, 0, 10, 0) && abs(target->z- verticalRayCast(ret[0], ret[1])) < 32 ) {
      
                  Destination = getNodeByPosition(ret[0], ret[1]);
                } else {
                  //Can't get our full range, so use the values in LineTraceX and LineTraceY
                  extern int lineTraceX, lineTraceY;
                  Destination = getNodeByPosition(lineTraceX, lineTraceY);
                }
              }
              
            }
          }

          //detect stuckness- if we're stuck, try heading to a random nearby node for a moment

          if(stuckTime > maxStuckTime) {
            M("A PATHFINDER IS STUCK");
            stuckTime = 0;
            current = Get_Closest_Node(g_navNodes);
            if(current != nullptr) {
              int c = rand() % current->friends.size();
              Destination = current->friends[c];
              M("stuck resolved?");
            }
            readyForNextTravelInstruction = 1;
          }
        }



      } 
        }

      

      return nullptr;
    }

//all-purpose pathfinding function
void entity::BasicNavigate(navNode* ultimateTargetNode) {
  if(g_navNodes.size() < 1) {return;}
  bool popOffPath = 0;
  if(current == nullptr) { //modified during rotational overhaul
    popOffPath = 1;
    
    //around the time when I started getting organs to follow the player
    //i noticed that sometimes entities would lose LOS and take the long
    //way around to get to the player
    //i suspect this line of code is the problem, so im changing the second
    //parameter from 1 to 0
    
    //sometime after that I started using the second node in the path, but that still hasn't quite solved it
    //i'll try re-enabling the useVelocity param and trying to fix it

    current = Get_Closest_Node(g_navNodes, 1);
    dest = current;
    path.clear();
  }

  // current->Render(255,0,0);
  // dest->Render(0,255,0);
  // Destination->Render(0,0,255);

  if(dest != nullptr) {
    float angleToTarget = atan2(dest->x - getOriginX(), dest->y - getOriginY()) - M_PI/2;
    angleToTarget = wrapAngle(angleToTarget);
    if(!specialAngleOverride) {
      targetSteeringAngle = angleToTarget;
    }
    forwardsVelocity = xagil;
  }

  
  int prog = 0;
  if(dest != nullptr) {
    if(abs(dest->y - getOriginY() ) < 64) {
      prog ++;
    }
    if(abs(dest->x - getOriginX()) < 55) {
      prog ++;
    }
  }

  if(prog == 2) {
    if( path.size() > 0) {
      //take next node in path
      current = dest;
      dest = path.at(path.size() - 1);
      path.erase(path.begin() + path.size()-1);
    }
  }

  if(timeSinceLastDijkstra < 0) {

    current = dest;
    if(current == nullptr) {
      //shit!
      current = Get_Closest_Node(g_navNodes, 1);
      dest = current;
    }


    //randomized time to space out dijkstra calls -> less framedrops
    timeSinceLastDijkstra = dijkstraSpeed + rand() % 500;
    navNode* targetNode = ultimateTargetNode;
    vector<navNode*> bag;
    for (int i = 0; i < (int)g_navNodes.size(); i++) {
      bag.push_back(g_navNodes[i]);
      g_navNodes[i]->costFromSource = numeric_limits<float>::max();
    }

    current->costFromSource = 0;
    int overflow = 2500; //seems that 500 is too small for medium-sized maps

    while(bag.size() > 0) {
      overflow --;
      if(overflow < 0) { E("Dijkstra Overflow"); break; } //might be worth de-agroing in case of a dijkstra overflow


      navNode* u;
      float min_dist = numeric_limits<float>::max();
      bool setU = false;



      for (int i = 0; i < (int)bag.size(); i++) {

        // !!! the second condition was added early december 2021
        // it it could cause problems
        if(bag[i]->costFromSource < min_dist && bag[i]->enabled){
          u = bag[i];
          min_dist = u->costFromSource;
          setU = true;
        }
      }

      if(!setU){
        //could issue an error msg
        break;
      }
          
        if(customMovement == 0) {
          if(u->costFromUsage >= 50) {
            u->costFromUsage -= 50;
          } else {
            u->costFromUsage = 0;
          }
        }

      //u is closest node in bag
      bag.erase(remove(bag.begin(), bag.end(), u), bag.end());
      for (long long unsigned int i = 0; i < u->friends.size(); i++) {
        if(u->enabled) {

          float alt = u->costFromSource + u->costs[i];
          
          //if our customMovement is set to corner, 
          //penalize nodes which were used by another behemoth
          if(customMovement == 2) {
            alt += u->costFromUsage;
          }

          if(alt < u->friends[i]->costFromSource && (u->friends[i]->z + 64 >= u->z)) {
            if(u->friends[i]->enabled) {
              u->friends[i]->costFromSource = alt; 
              u->friends[i]->prev = u;
            } else {
              u->friends[i]->prev = nullptr;
            }
          }
        } else {
          u->prev = nullptr;
        }
      }

    }
    path.clear();
    int secondoverflow = 350;

    while(targetNode != nullptr) {
      secondoverflow--;
      if(secondoverflow < 0) { /*M("prevented a PF crash.");*/ break;} //preventing this crash results in pathfinding problems

      path.push_back(targetNode);

      if(targetNode == current) {
        break;
      }
      targetNode = targetNode->prev;
    }

    if(popOffPath && path.size() > 1) {
      path.erase(path.begin());
      path.erase(path.begin());
    }

    if(justLostLosToTarget) {
//        This code might be handy later for trying to understand why
//        entities doesn't travel as you might expect.
//        Today's efforts (probably) mark the conclusion of a struggle
//        I've had with getting entities to path properly.
//        The problem was that they would take a bad path the moment they lose
//        LOS to their target.
//        Many improvements have been made, and in this case the problem was
//        with the navmesh, the closest node was one nex to a wall, which was 
//        too close to travel to another node close to the wall (there was no line between them)
//        and because of how I have nodes attach, the nodes didnt connect to some of the closest
//        nodes, but kinda past them and through them, making it look like he was taking a 
//        very long path. I'll ajust how navnodes are linked
    }

    for(auto x : path) {
      if(!x->enabled){
        M("TRYING TO USE DISABLED NODE");
        current = getNodeByPosition(getOriginX(), getOriginY());
        dest = current;

        path.clear(); //seems to fix it


      }
    }

    //path is considered "valid"
    //we should penalize all nodes in the path, and all nodes connected
    //so that other ents don't use them
    if(customMovement == 0) {
      int enough = 0;
      for(auto x : path) {
        enough++;
        if(enough < 1) {continue;}
        if(enough > 5) {break;}

        x->costFromUsage = 1000000;
        for(auto y : x->friends) {
          y->costFromUsage = 1000000;
        }
      }
    }
    if(path.size() != 0) {
      dest = path.at(path.size() - 1); 
    } else {
      dest = getNodeByPosition(getOriginX(), getOriginY());
    }
    if( path.size() > 0) {
      //take next node in path
      current = dest;
      dest = path.at(path.size() - 1);
      path.erase(path.begin() + path.size()-1);
    }
  } else {
    timeSinceLastDijkstra -= elapsed;
  }
  
}

//I want something like BasicNavigate, but increase node cost in some way to 
//penalize going near other entities
//the idea is that two entities blocking the same route is a poor allocation of resources, 
//for the behemoths as a team
//really, it has nothing to do with distance, but rather the topology of the map (routes)

//functions for inv
//add an item to an entities
int entity::getItem(indexItem* a, int count) {
  for(auto& x : inventory) {
    if(x.first->name == a->name) {
      x.second += count;
      return 0;
    }
  }
  pair<indexItem*, int> pushMeBack{ a, count };

  inventory.push_back( pushMeBack );
  return 0;
}

    //returns 0 if the transaction was successful, and 1 otherwise
int entity::loseItem(indexItem* a, int count) {
  for(auto& x : inventory) {
    if(x.first->name == a->name) {
      if(x.second > count) {
        x.second-=count;
        return 0;
      } else {

        for(auto y : inventory) {
        }
        delete x.first;
        x.second = 0;

        inventory.erase(remove(inventory.begin(), inventory.end(), x), inventory.end());
        return 0;
      }
    }
  }
  return 1;
}

//returns 0 if the entity has the nummer of items
//returns 1 if the entity does not have the proper nummer
int entity::checkItem(indexItem* a, int count) {
  for(auto x : inventory) {
    if(x.first == a) {
      if(x.second >= count) {
        return 0;
      }
    }
  }
  return 1;
}


//search entity by name
entity* searchEntities(string fname, entity* caller) {
  if(fname == "protag") {
    return protag;
  }
  if(fname == "null" || fname == "nullptr") {
    return nullptr; //this has some uses, e.g. making missiles have null target
  }
  if(caller != 0 && fname == "target") {
    if(caller->target != nullptr && (caller->target->tangible || caller->target == protag && g_protagIsWithinBoardable));
    return caller->target;
  }
  if(caller != 0 && (fname ==  "this" || fname == "me") ) {
    return caller;
  }


  for(auto n : g_entities) {
    if(n->name == fname && (n->tangible || n == protag && g_protagIsWithinBoardable)) {
      return n;
    }
  }

  return nullptr;
}

entity* searchEntities(string fname) {
  if(fname == "protag") {
    return protag;
  }
  for(auto n : g_entities) {
    if(n->name == fname && (n->tangible || n == protag && g_protagIsWithinBoardable)) {
      return n;
    }
  }
  return nullptr;
}

//return list of tangible entities with the name
vector<entity*> gatherEntities(string fname) {
  vector<entity*> ret = {};
  for(auto n : g_entities) {
    if(n->name == fname && (n->tangible || n == protag && g_protagIsWithinBoardable)) {
      ret.push_back(n);
    }
  }
  return ret;
}

levelNode::levelNode(string p3, string p4, string p5, SDL_Renderer * renderer, int fmouthStyle, int ffloors, int fgoldMs, vector<string> fbehemoths, int ffirstfloor, int frestlen, int fchaselen, string fmusic, string fchasemusic, int fdarkness) {
  name = p3;
  mapfilename = p4;
  waypointname = p5;
  mouthStyle = fmouthStyle;

  dungeonFloors = ffloors;
  dungeonGoldMs = fgoldMs;
  behemoths = fbehemoths;
  firstActiveFloor = ffirstfloor;
  avgRestSequence = frestlen;
  avgChaseSequence = fchaselen;

  music = fmusic;
  chasemusic = fchasemusic;

  darkness = fdarkness;

  //load graphic
  string lowerName = name;

  int myPellets = 0; //pellets collected from this map

  std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), 
      [](unsigned char c) { if(c == ' ') {int e = '-'; return e;} else {return std::tolower(c);}  } ); //I convinced c++ that e is a number for this uber-efficient line of code

  string loadSTR = "resources/levelsequence/icons/" + lowerName + ".qoi";

  sprite = loadTexture(renderer, loadSTR);


  for(int i = 0; i < g_levelNodes.size(); i++) {
    levelNode* other = g_levelNodes[i];
    if(other->eyeStyle == this->eyeStyle) {
      this->eyeTexture = other->eyeTexture;
      break;
    }
  }

  for(int i = 0; i < g_levelNodes.size(); i++) {
    levelNode* other = g_levelNodes[i];
    if(other->mouthStyle == this->mouthStyle) {
      this->mouthTexture = other->mouthTexture;
      break;
    }
  }

  if(eyeTexture == nullptr) {
    string loadSTR = "resources/levelsequence/icons/eyes.qoi";
    this->eyeTexture = loadTexture(renderer, loadSTR);
  }

  if(mouthTexture == nullptr) {
    string loadSTR = "resources/levelsequence/icons/mouth2.qoi";
    if(mouthStyle == 1) {
      loadSTR = "resources/levelsequence/icons/mouth.qoi";
    }

    this->mouthTexture = loadTexture(renderer, loadSTR);
  }

  blinkCooldownMS = rng(minBlinkCooldownMS, maxBlinkCooldownMS);
  
  g_levelNodes.push_back(this);

}

levelNode::~levelNode() {
  if(loadedEyes) { SDL_DestroyTexture(eyeTexture);}
  if(loadedMouth) { SDL_DestroyTexture(mouthTexture);}
  SDL_DestroyTexture(sprite);

  g_levelNodes.erase(remove(g_levelNodes.begin(), g_levelNodes.end(), this), g_levelNodes.end());
}

SDL_Rect levelNode::getEyeRect() {
  SDL_Rect srect = {0, 0, 196, 196};
  if(blinkCooldownMS < 120) { srect.x += 196;};
  if(blinkCooldownMS < 60) { srect.x += 196;};
  return srect;
}


levelSequence::levelSequence(string filename, SDL_Renderer * renderer){
}

//add levels from a file
void levelSequence::addLevels(string filename) {
  filename = "resources/levelsequence/" + filename;
  
  istringstream file(loadTextAsString(filename));

  string temp;
  
  string level_name;
  string map_name;
  string way_name;

  int i = 0;
  for(;;) {

    getline(file,level_name);
    if(level_name[0] == '-') { break;}
    getline(file,map_name);
    getline(file,way_name);
    level_name.pop_back();
    map_name.pop_back();
    way_name.pop_back();

    getline(file, temp);
    int special = stoi(temp);

    getline(file, temp);
    int dungeonFloors = stoi(temp);

    getline(file, temp);
    int dungeonGoldMs = stoi(temp);
    D(dungeonGoldMs);

    getline(file, temp);
    vector<string> behemoths = splitString(temp, ' ');

    behemoths.back().pop_back(); // this is a newline char

    getline(file, temp);
    int firstActiveFloor = stoi(temp);
    
    getline(file, temp);
    int avgRestSequence = stoi(temp);

    getline(file, temp);
    int avgChaseSequence = stoi(temp);

    getline(file,temp);
    string music = temp;
    music.pop_back();

    getline(file,temp);
    string chasemusic = temp;
    chasemusic.pop_back();

    getline(file,temp);
    int darkness = stoi(temp);

    map_name = "resources/maps/" + map_name;

    if(file.eof()) {break;}

    levelNode* newLevelNode = new levelNode(level_name, map_name, way_name, renderer, special, dungeonFloors, dungeonGoldMs, behemoths, firstActiveFloor, avgRestSequence, avgChaseSequence, music, chasemusic, darkness);
    i++;
    levelNodes.push_back(newLevelNode);
    getline(file,temp);

  }
}





usable::usable(string fname) {
  internalName = fname;
  string loadstr;

  string filepath = "resources/static/usables/" + fname + "/";

  //open specs file
  loadstr = filepath + "specs_" + fname + ".txt";

  istringstream file(loadTextAsString(loadstr));
  
  string comment;
  string line;
  string empty;

  //display name
  getline(file, comment);
  getline(file, name);
  getline(file, empty);

  //cooldownMs
  getline(file, comment);
  getline(file, line);
  maxCooldownMs = stoi(line);
  getline(file, empty);

  //specialAction
  getline(file, comment);
  getline(file, line);
  specialAction = stoi(line);
  getline(file, empty);

  //about
  getline(file, comment);
  getline(file, aboutTxt);
  //getline(file, empty);


  //load sprite
  loadstr = filepath + "img_" + fname + ".qoi";

  texture = loadTexture(renderer, loadstr);

}



usable::~usable() {
  SDL_DestroyTexture(texture);
}


int loadSave() {
  g_save.clear();
  g_saveStrings.clear();
  ifstream file;
  string line;

  string address = "user/saves/" + g_saveName + ".save";
  D(address);
  const char* plik = address.c_str();
  file.open(plik);

  string field = "";
  string value = "";
  //load save fields
  while(getline(file, line)) {
    if(line == "&") { break;}
    D(line == "&");
    D(line.size());
    field = line.substr(0, line.find(' '));
    value = line.substr(line.find(" "), line.length()-1);

    try {
      g_save.insert( pair<string, int>(field, stoi(value)) );
    } catch(...) {
      E("Error reading save.");
      return -1;
    }

  }

  //load saved strings
  while(getline(file, line)) {
    if(line == "&") { break;}
    field = line.substr(0, line.find(' '));
    value = line.substr(line.find(" ")+1, line.length()-1);


    try {
      g_saveStrings.insert( pair<string, string>(field, value) );
      //for debugging saved strings
    } catch(...) {
      E("Error writing save.");
      return -1;
    }

  }


  file >> g_mapOfLastSave >> g_waypointOfLastSave;
  getline(file,line);
  getline(file,line);


  party.clear();
  delete protag;

  bool setMainProtag = 0;
  //load party
  while(getline(file, line)) {
    if(line == "&") {break;}

    auto tokens = splitString(line, ' ');
    string name = tokens[0];
    float level = stoi(tokens[1]);
    float currentHP = stof(tokens[2]);

    entity* a = new entity(renderer, name);
    a->level = level;
    a->hp = currentHP;
    party.push_back(a);
    a->tangible = 0;
    a->inParty = 1;
    if(a->essential) {
      mainProtag = a;
      setMainProtag = 1;
    }
    
  }

  party[0]->tangible = 1;
  if(setMainProtag) {
    protag = mainProtag;
  } else {
    //fick
    //E("No essential entity found in save");
    protag = party[0];
    mainProtag = protag;
  }

  g_focus = protag;

  //load spin entity
  if(g_spin_enabled && g_spin_entity == nullptr) {
    
    string spinEntFilename = protag->name + "-spin";
    entity* a = new entity(renderer, spinEntFilename);
    g_spin_entity = a;
    g_spin_entity->visible = 0;
    g_spin_entity->msPerFrame = 50;
    g_spin_entity->loopAnimation = 1;
    g_spin_entity->canFight = 0;
  }


  int indexItemsSize = g_indexItems.size();
  for(int i = 0; i < indexItemsSize; i++) {
    delete g_indexItems[0];
  }

  inventorySelection = 0;

  //load inventory
  while(getline(file, line)) {
    if(line == "&") { break;}

    field = line.substr(0, line.find(' '));
    value = line.substr(line.find(" "), line.length()-1);
    indexItem* a = new indexItem(field, 0);
    protag->getItem(a, stoi(value));
  }


  //load which levels are unlocked, as a list of lowercase names
  //M("LETS UNLOCK LEVELS");
  //
  //unlocked levels should have '-' instead of ' ' in the name, for parsing reasons
  //
  while(getline(file, line)) {
    if(line == "&") { break;}
    for(int i = 0; i < g_levelSequence->levelNodes.size(); i++) {
      string lowerName = g_levelSequence->levelNodes[i]->name;
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),  [](unsigned char c) { if(c == ' ') {int e = '-'; return e;} else {return std::tolower(c);}  } );
      if (lowerName == line) {
        g_levelSequence->levelNodes[i]->locked = 0;
        //M("UNLOCKED A LEVEL");
      }
    }

  }

  //delete all usables
  int size = g_backpack.size();
  for(int i = 0; i < size; i++) {
    delete g_chest[i];
  }
  g_chest.clear();

  adventureUIManager->hideBackpackUI();
  adventureUIManager->resetBackpackUITextures();
    
  g_backpackIndex = 0; //otherwise we might segfault

  //load protag's usables
  while(getline(file, line)) {
    if(line == "&") { break;}
    int good = 1;
    for(auto x: g_backpack) {
      if(x->internalName == line) {
        good = 0;
        break;
      }
    }

    if(good == 1) {
      usable* newUsable = new usable(line);
      g_chest.push_back(newUsable);
      adventureUIManager->showBackpackUI();
    }

  }

  file.close();

  for(auto x : g_entities) {
    x->children.clear(); // might be a leak here
  }

  //re-attach persistent orbitals
  for(auto x : g_entities) {
    if(x->persistentGeneral && x->parentName != "null") {
      entity* hopeful = searchEntities(x->parentName);
      if(hopeful != nullptr) {
        x->isOrbital = 1;
        x->parent = hopeful;
        x->parent->children.push_back(x);
      }

    }
  }

  return 0;
}

int writeSave() {
  ofstream file;

  string address = "user/saves/" + g_saveName + ".save";
  const char* plik = address.c_str();
  file.open(plik);

  //if the loaded name is "Blank", change it to "Fomm"
  if(readSaveStringField("playername") == "Blank") {
    writeSaveFieldString("playername", "Fomm");
  }

  auto it = g_save.begin();

  while (it != g_save.end() ) {
    file << it->first << " " << it->second << endl;
    it++;
  }
  file << "&" << endl; //token to stop writing savefields
                       
                       
  auto it2 = g_saveStrings.begin();

  while (it2 != g_saveStrings.end() ) {
    file << it2->first << " " << it2->second << endl;
    it2++;
  }
  file << "&" << endl; //token to stop writing savestrings
                       
                       
  file << g_mapdir + "/" + g_map << " " << g_waypoint << endl;
  file << "&" << endl;

  //write party
  for(auto x : party) {
    file << x->name << " " << x->level << " " << x->hp << endl;
  }
  file << "&" << endl;
 
  //write protag's inventory
  extern entity* protag;
  for(auto x : protag->inventory) {
    file << x.first->name << " " << x.second << endl;
  }
  file << "&" << endl; //token to stop writing inventory
  
  for(int i = 0; i < g_levelSequence->levelNodes.size(); i++) {
    if(g_levelSequence->levelNodes[i]->locked == 0) {
      string lowerName = g_levelSequence->levelNodes[i]->name;
      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),  [](unsigned char c) { if(c == ' ') {int e = '-'; return e;} else {return std::tolower(c);} } );
      file << lowerName << endl;
    }
  }

  file << "&" << endl; //token to stop writing unlocked levels
  
  //write protag's usables
  for(auto x : g_chest) {
    file << x->internalName << endl;
    D(x->internalName);
  }
  
  file << "&" << endl; //token to stop writing usables


  file.close();
  return 0;
}

int checkSaveField(string field) {
  std::map<string, int>::iterator it = g_save.find(field);
  if(it != g_save.end()) {
    return it->second;
  } else {
    return -1;
  }
}

void writeSaveField(string field, int value) {
  auto it = g_save.find(field);

  if (it == g_save.end()) {
    g_save.insert(std::make_pair(field, value));
  } else {
    g_save[field] = value;
  }
}

string readSaveStringField(string field) {
  std::map<string, string>::iterator it = g_saveStrings.find(field);
  if(it != g_saveStrings.end()) {
    return it->second;
  } else {
    return "unset";
  }
}

void writeSaveFieldString(string field, string value) {
  auto it = g_saveStrings.find(field);

  if (it == g_saveStrings.end()) {
    g_saveStrings.insert(std::make_pair(field, value));
  } else {
    g_saveStrings[field] = value;
  }
}

void entity::shoot() {
  //M("shoot()");
  if(!tangible) {return;}
  if(this->cooldown <= 0) {
    //M("pow pow");
    if(hisweapon->comboResetMS > hisweapon->maxComboResetMS) {
      //waited too long- restart the combo
      hisweapon->combo = 0;
    }
    for(float i = 0; (i < this->hisweapon->attacks[hisweapon->combo]->numshots); i++) {
      if(i > 1000) {
        //M("Handled an infinite loop");
        quit = 1;
        return;

      }
      cooldown = hisweapon->attacks[hisweapon->combo]->maxCooldown;
      projectile* p = new projectile(hisweapon->attacks[hisweapon->combo]);
      p->owner = this;
      p->x = getOriginX() - p->width/2;
      p->y = getOriginY() - p->height/2;
      p->z = z + 20;
      p->zeight = p->width * XtoZ;
      p->animation = this->animation;
      p->flip = this->flip;

      //inherit velo from parent
      p->xvel = xvel/15;
      p->yvel = yvel/15;
      //angle
      if(up) {
        if(left) {
          p->angle = 3 * M_PI / 4;

        } else if (right) {
          p->angle = M_PI / 4;
        } else {
          p->angle = M_PI / 2;

        }
      } else if (down) {
        if(left) {
          p->angle = 5 * M_PI / 4;

        } else if (right) {
          p->angle = 7 * M_PI / 4;

        } else {
          p->angle = 3 * M_PI / 2;

        }
      } else {
        if(left) {
          p->angle = M_PI;

        } else if (right) {
          p->angle = 0;

        } else {
          //default
          p->angle = 3 * M_PI / 4;

        }
      }

      //give it an angle based on attack spread
      if(hisweapon->attacks[hisweapon->combo]->randomspread != 0) {
        float randnum = (((float) rand()/RAND_MAX) * 2) - 1;
        p->angle += (randnum * hisweapon->attacks[hisweapon->combo]->randomspread);
      }
      if(hisweapon->attacks[hisweapon->combo]->spread != 0) {
        p->angle += ( (i - ( hisweapon->attacks[hisweapon->combo]->numshots/2) ) * hisweapon->attacks[hisweapon->combo]->spread);
      }

      //move it out of the shooter and infront
      p->x += cos(p->angle) * p->bounds.width;
      p->y += cos(p->angle + M_PI / 2) * p->bounds.height;




    }
    hisweapon->combo++;

    if(hisweapon->combo > (int)(hisweapon->attacks.size() - 1)) {hisweapon->combo = 0;}
    hisweapon->comboResetMS = 0;
  }
}


//returns true if there was no hit
//visibility is 1 to check for just navblock (very solid) entities
// LineTrace definition
int LineTrace(int x1, int y1, int x2, int y2, bool display, int size, int layer, int resolution, bool visibility, bool fogOfWar) {
  //float resolution = 10;

  if(display) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  }
  for (float i = 1; i < resolution; i++) {
    int xsize = size * p_ratio;
    int xpos = (i/resolution) * x1 + (1 - i/resolution) * x2;
    int ypos = (i/resolution) * y1 + (1 - i/resolution) * y2;
    rect a = rect(xpos - xsize/2, ypos - size/2, xsize, size);
    SDL_Rect b = {(int)(((xpos- xsize/2) - g_camera.x) * g_camera.zoom), (int)(((ypos- size/2) - g_camera.y) * g_camera.zoom), (int)(xsize), (int)(size)};

    if(display) {
      SDL_RenderDrawRect(renderer, &b);
    }

    if(fogOfWar) {
      for(auto x : g_large_entities) {
        if(RectOverlap(a, x->getMovedBounds())) {
          lineTraceX = a.x + a.width/2;
          lineTraceY = a.y + a.height/2;
          return true;
        }
      }
    } else {

      for (long long unsigned int j = 0; j < g_is_collisions.size(); j++) {
        if(RectOverlap(a, g_is_collisions[j]->bounds)) {
          lineTraceX = a.x + a.width/2;
          lineTraceY = a.y + a.height/2;
          return false;
        }
      }

    }



    for (long long unsigned int j = 0; j < g_lt_collisions.size(); j++) {
      if(RectOverlap(a, g_lt_collisions[j]->bounds)) {
        lineTraceX = a.x + a.width/2;
        lineTraceY = a.y + a.height/2;
        return false;
      }
    }


    for(auto x : g_solid_entities) {
      if(visibility) {
        if(!x->navblock) {continue;}
      }
      if(RectOverlap(a, x->getMovedBounds())) {
        lineTraceX = a.x + a.width/2;
        lineTraceY = a.y + a.height/2;
        return false;
      }
    }


  }
  return true;
}

textbox::textbox(SDL_Renderer* renderer, const char* fcontent, float size, float fx, float fy, float fwidth) {
  //M("textbox()" );
  fontsize = size;
  content = fcontent;
  font = loadFont(g_font, size);

  textsurface = TTF_RenderText_Blended_Wrapped(font, content.c_str(), textcolor, fwidth * WIN_WIDTH);
  texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

  int texW = 0;
  int texH = 0;
  x = fx;
  y = fy;
  boxX = fx;
  boxY = fy;
  SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
  //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
  this->width = texW;
  this->height = texH;
  thisrect = { fx, fy, (float)texW, (float)texH };

  g_textboxes.push_back(this);
}

textbox::~textbox() {
  //M("~textbox()" );
  g_textboxes.erase(remove(g_textboxes.begin(), g_textboxes.end(), this), g_textboxes.end());
  TTF_CloseFont(font);
  SDL_DestroyTexture(texttexture);
  SDL_FreeSurface(textsurface);
}

void textbox::render(SDL_Renderer* renderer, int winwidth, int winheight) {
  if(show) {
    if(blinking) {
      if(g_blinkHidden) {
        return;
      }
    }

    if(worldspace) {
      if(align == 1) {
        //right
        SDL_FRect dstrect = {(boxX * winwidth)-width, boxY * winheight, (float)width,  (float)thisrect.h};
        dstrect.x /= g_zoom_mod;
        dstrect.y /= g_zoom_mod;
        dstrect.w /= g_zoom_mod;
        dstrect.x /= g_zoom_mod;
        if(dropshadow) {
          SDL_FRect shadowRect = dstrect;
          float booshAmount = g_textDropShadowDist * fontsize;
          shadowRect.x += booshAmount;
          shadowRect.y += booshAmount;
          SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
          SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
          SDL_SetTextureColorMod(texttexture, 255,255,255);
        }
        SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
      } else {
        if(align == 0) {
          //left
          SDL_FRect dstrect = {boxX * winwidth, boxY * winheight, (float)width,  (float)thisrect.h};
          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * fontsize;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texttexture, 255,255,255);
          }

          SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
        } else {
          //center text
          SDL_FRect dstrect = {(boxX * winwidth)-width/2, boxY * winheight, (float)width,  (float)thisrect.h};
          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * fontsize;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texttexture, 255,255,255);
          }
          SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
        }
      }


    } else {
      if(align == 1) {
        SDL_FRect dstrect = {(boxX * winwidth)-width, boxY * winheight, (float)width,  (float)thisrect.h};
        dstrect.x /= g_zoom_mod;
        dstrect.y /= g_zoom_mod;
        dstrect.w /= g_zoom_mod;
        dstrect.h /= g_zoom_mod;
        if(dropshadow) {
          SDL_FRect shadowRect = dstrect;
          float booshAmount = g_textDropShadowDist * fontsize;
          shadowRect.x += booshAmount;
          shadowRect.y += booshAmount;
          SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
          SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
          SDL_SetTextureColorMod(texttexture, 255,255,255);
        }
        SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
      } else {
        if(align == 0) {
          SDL_FRect dstrect = {boxX * winwidth, boxY * winheight, (float)width,  (float)thisrect.h};
          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * fontsize;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texttexture, 255,255,255);
          }
          SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
        } else {
          //center text
          SDL_FRect dstrect = {(boxX * winwidth)-width/2, boxY * winheight, (float)width,  (float)thisrect.h};
          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * fontsize;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texttexture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texttexture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texttexture, 255,255,255);
          }
          SDL_RenderCopyF(renderer, texttexture, NULL, &dstrect);
        }
      }
    }
  }
}

void textbox::updateText(string content, float size, float fwidth, SDL_Color fcolor, string fontstr) {
  if(size < 0) { //easy way to preserve fontsize
    size = fontsize;
  }
  SDL_DestroyTexture(texttexture);
  SDL_FreeSurface(textsurface);
  textsurface =  TTF_RenderText_Blended_Wrapped(font, content.c_str(), fcolor, fwidth * WIN_WIDTH);
  texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);
  int texW = 0;
  int texH = 0;
  SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
  //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
  width = texW;
  thisrect = { (float)x, (float)y, (float)texW, (float)texH };

}



ui::ui(SDL_Renderer * renderer, const char* ffilename, float fx, float fy, float fwidth, float fheight, int fpriority) {
  //M("ui()" );
  filename = ffilename;

  texture = loadTexture(renderer, ffilename);

  width = fwidth;
  height = fheight;
  x = fx;
  y = fy;
  
  g_ui.push_back(this);

  priority = fpriority;
}

ui::~ui() {
  SDL_DestroyTexture(texture);
  g_ui.erase(remove(g_ui.begin(), g_ui.end(), this), g_ui.end());
}

void ui::render(SDL_Renderer * renderer, camera fcamera, float elapsed) {
  //proportional gliding
  if(targetx != -10) {
    x = (targetx * glideSpeed) + (x * (1-glideSpeed));
  }

  if(targety != -10) {
    y = (targety * glideSpeed) + (y * (1-glideSpeed));
  }

  if(targetwidth != -10) {
    width = (targetwidth * widthGlideSpeed) + (width * (1-widthGlideSpeed));
  }

  if(this->show) {
    SDL_SetTextureAlphaMod(texture, opacity/100);
    if(is9patch) {
      int ibound = width * WIN_WIDTH;
      int jbound = height * WIN_HEIGHT;

      ibound /= g_zoom_mod;
      jbound /= g_zoom_mod;
      float tempx = x / g_zoom_mod;
      float tempy = y / g_zoom_mod;

      if(heightFromWidthFactor) {
        jbound = ibound * height;
      }



      if(WIN_WIDTH != 0) {
        patchscale = WIN_WIDTH;
        patchscale /= 4000;
      }
      int scaledpatchwidth = patchwidth * patchscale * patchfactor;
      int i = 0;
      while (i < ibound) {
        int j = 0;
        while (j < jbound) {
          SDL_FRect dstrect = {i + (tempx * WIN_WIDTH), j + (tempy * WIN_HEIGHT), (float)scaledpatchwidth, (float)scaledpatchwidth}; //change patchwidth in this declaration for sprite scale
          SDL_Rect srcrect;
          srcrect.h = patchwidth;
          srcrect.w = patchwidth;
          if(i==0) {
            srcrect.x = 0;

          } else {
            if(i + scaledpatchwidth >= ibound) {
              srcrect.x = 2 * patchwidth;
            }else {
              srcrect.x = patchwidth;
            }}
          if(j==0) {
            srcrect.y = 0;
          } else {
            if(j + scaledpatchwidth >= jbound) {
              srcrect.y = 2 * patchwidth;
            }else {
              srcrect.y = patchwidth;
            }}

          //shrink the last non-border tile to fit well.
          int newheight = jbound - (j + scaledpatchwidth);
          if(j + (2 * scaledpatchwidth) >= jbound && newheight > 0) {

            dstrect.h = newheight;
            j+=  newheight;
          } else {
            j+= scaledpatchwidth;
          }

          int newwidth = ibound - (i + scaledpatchwidth);
          if(i + (2 * scaledpatchwidth) >= ibound && newwidth > 0) {

            dstrect.w = newwidth;
          } else {
          }

          //done to fix occasional 1px gap. not a good fix
          dstrect.h += 1;
          SDL_RenderCopyF(renderer, texture, &srcrect, &dstrect);



        }
        //increment i based on last shrink
        int newwidth = ibound - (i + scaledpatchwidth);
        if(i + (2 * scaledpatchwidth) >= ibound && newwidth > 0) {
          i+=  newwidth;
        } else {
          i+= scaledpatchwidth;
        }
      }

    } else {
      if(worldspace) {
        SDL_FRect dstrect = {x, y, width, height};
        
          dstrect.x *= g_camera.zoom;
          dstrect.y *= g_camera.zoom;
          dstrect.w *= g_camera.zoom;
          dstrect.h *= g_camera.zoom;

        dstrect = transformRect( dstrect );
        if(dropshadow) {
          SDL_FRect shadowRect = dstrect;
          float booshAmount = g_textDropShadowDist * 52.7;
          shadowRect.x += booshAmount;
          shadowRect.y += booshAmount;
          SDL_SetTextureColorMod(texture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
          SDL_RenderCopyF(renderer, texture, NULL, &shadowRect);
          SDL_SetTextureColorMod(texture, 255,255,255);
        }
        SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
      } else {
        if(heightFromWidthFactor != 0) {
          SDL_FRect dstrect = {x * WIN_WIDTH + (shrinkPixels / scalex) + (shrinkPercent * WIN_WIDTH), y * WIN_HEIGHT + (shrinkPixels / scalex) + (shrinkPercent * WIN_WIDTH), width * WIN_WIDTH - (shrinkPixels / scalex) * 2 - (shrinkPercent * WIN_WIDTH) * 2,  heightFromWidthFactor * (width * WIN_WIDTH - (shrinkPixels / scalex) * 2 - (shrinkPercent * WIN_WIDTH) * 2) };
          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * 52.7;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texture, 255,255,255);
          }



          if(xframes > 1) {
            if(msPerFrame != 0) {
              msTilNextFrame += elapsed;
              if(msTilNextFrame > msPerFrame && xframes > 1) {
                frame++;
                msTilNextFrame = 0; //bad but w/e
                if(frame >= xframes) {
                  frame = 0;
                }
              }
            }
            SDL_Rect srcrect = {0 + frame * framewidth , 0,  framewidth, frameheight};
            SDL_RenderCopyF(renderer, texture, &srcrect, &dstrect);
          } else {
            SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
          }

        } else {
          SDL_FRect dstrect = {x * WIN_WIDTH + (shrinkPixels / scalex) + (shrinkPercent * WIN_WIDTH), y * WIN_HEIGHT + (shrinkPixels / scalex) + (shrinkPercent * WIN_WIDTH), width * WIN_WIDTH - (shrinkPixels / scalex) * 2 - (shrinkPercent * WIN_WIDTH) * 2, height * WIN_HEIGHT - (shrinkPixels / scalex) * 2 - (shrinkPercent * WIN_WIDTH) * 2};
          if(dropshadow) {
            SDL_FRect shadowRect = dstrect;
            float booshAmount = g_textDropShadowDist * 52.7;
            shadowRect.x += booshAmount;
            shadowRect.y += booshAmount;
            SDL_SetTextureColorMod(texture, g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
            SDL_RenderCopyF(renderer, texture, NULL, &shadowRect);
            SDL_SetTextureColorMod(texture, 255,255,255);
          }

          dstrect.x /= g_zoom_mod;
          dstrect.y /= g_zoom_mod;
          dstrect.w /= g_zoom_mod;
          dstrect.h /= g_zoom_mod;

          if(xframes > 1) {
            if(msPerFrame != 0) {
              msTilNextFrame += elapsed;
              if(msTilNextFrame > msPerFrame && xframes > 1) {
                frame++;
                msTilNextFrame = 0; //bad but w/e
                if(frame >= xframes) {
                  frame = 0;
                }
              }
            }
            SDL_Rect srcrect = {0 + frame * framewidth , 0,  framewidth, frameheight};
            SDL_RenderCopyF(renderer, texture, &srcrect, &dstrect);
          } else {
            SDL_RenderCopyF(renderer, texture, NULL, &dstrect);
          }
        }
      }
    }
  }
}


musicNode::musicNode(string fileaddress, int fx, int fy) {
  name = fileaddress;

  string temp = "resources/static/music/" + fileaddress + ".ogg";

  //blip = Mix_LoadMUS(temp.c_str());
  blip = loadMusic(temp);
  x = fx;
  y = fy;
  g_musicNodes.push_back(this);
}

musicNode::~musicNode() {
  Mix_FreeMusic(blip);
  g_musicNodes.erase(remove(g_musicNodes.begin(), g_musicNodes.end(), this), g_musicNodes.end());
}


cueSound::cueSound(string fileaddress, int fx, int fy, int fradius) {
  name = fileaddress;
  string existSTR;
  existSTR = "resources/static/sounds/" + fileaddress + ".wav";
  blip = loadWav(existSTR.c_str());
  x = fx;
  y = fy;
  radius = fradius;
  g_cueSounds.push_back(this);
}

cueSound::~cueSound() {
  Mix_FreeChunk(blip);
  g_cueSounds.erase(remove(g_cueSounds.begin(), g_cueSounds.end(), this), g_cueSounds.end());
}


//play a sound by name at a position
void playSoundByName(string fname, float xpos, float ypos) {
  Mix_Chunk* sound = 0;
  for (auto s : g_cueSounds) {
    if (s->name == fname) {
      sound = s->blip;
      break;
    }
  }
  if(sound == NULL) {
    E("Cuesound " + fname + " not found in level.");
    for (auto s : g_cueSounds) {
      D(s->name);
    }
    return;
  }

  //!!! this could be better if it used the camera's position
  float dist = XYWorldDistance(g_focus->getOriginX(), g_focus->getOriginY(), xpos, ypos);
  const float maxDistance = 1200; //a few screens away
  float cur_volume = (maxDistance - dist)/maxDistance * 128;
  if(cur_volume < 0) {cur_volume = 0;}
  //M(cur_volume);
  Mix_VolumeChunk(sound, cur_volume);
  if(!g_mute && sound != NULL) {
    Mix_PlayChannel(0, sound,0);
  }
}

//play a sound given a string of its name. just make sure there's a cue with the same name
void playSoundByName(string fname) {
  Mix_Chunk* sound = 0;
  for (auto s : g_cueSounds) {
    if (s->name == fname) {
      sound = s->blip;
      break;
    }
  }
  if(sound == NULL) {
    E("Soundcue " + fname + " not found in level.");
  }

  if(!g_mute && sound != NULL) {
    Mix_PlayChannel(0, sound,0);
  }
}

void playSoundAtPosition(int channel, Mix_Chunk *sound, int loops, int xpos, int ypos, float volume)
{
  // M("play sound");
  if (!g_mute && sound != NULL)
  {
    //!!! this could be better if it used the camera's position
    float dist = XYWorldDistance(g_focus->getOriginX(), g_focus->getOriginY(), xpos, ypos);
    const float maxDistance = 1200; //a few screens away
    float cur_volume = (maxDistance - dist)/maxDistance * 128;
    cur_volume *= volume;
    if(cur_volume < 0) {cur_volume = 0;}
    Mix_VolumeChunk(sound, cur_volume);
    if(!g_mute && sound != NULL) {
      Mix_PlayChannel(channel, sound,loops);
    }
  }
}

waypoint::waypoint(string fname, float fx, float fy, int fz, int fangle) {
  name = fname;
  x = fx;
  y = fy;
  z = fz;
  angle = fangle;
  g_waypoints.push_back(this);
}

waypoint::~waypoint() {
  g_waypoints.erase(remove(g_waypoints.begin(), g_waypoints.end(), this), g_waypoints.end());
}


trigger::trigger(string fbinding, int fx, int fy, int fz, int fwidth, int fheight, int fzeight, string ftargetEntity) {
  x = fx;
  y = fy;
  z = fz;
  width = fwidth;
  height = fheight;
  zeight = fzeight;
  binding = fbinding;
  targetEntity = ftargetEntity;
  g_triggers.push_back(this);
  //open and read from the script file
  ifstream stream;
  string loadstr;
  //try to open from local map folder first

  loadstr = "resources/maps/" + g_mapdir + "/" + fbinding + ".txt";
  const char* plik = loadstr.c_str();

  stream.open(plik);

  if (!stream.is_open()) {
    stream.open("scripts/" + fbinding + ".txt");
  }
  string line;

  getline(stream, line);

  while (getline(stream, line)) {
    script.push_back(line);
  }

  parseScriptForLabels(script);
  // for(auto x : script) {
  // }
}

trigger::~trigger() {
  g_triggers.erase(remove(g_triggers.begin(), g_triggers.end(), this), g_triggers.end());
}

hitbox::hitbox() {
  g_hitboxes.push_back(this);
}

hitbox::~hitbox() {
  g_hitboxes.erase(remove(g_hitboxes.begin(), g_hitboxes.end(), this), g_hitboxes.end());
}

rect hitbox::getMovedBounds() {
  if(parent == 0) {
    return rect(bounds.x + x, bounds.y + y, z, bounds.width, bounds.height, bounds.zeight);
  } else {
    return rect(bounds.x + x + parent->getOriginX() - bounds.width/2, bounds.y + y + parent->getOriginY() - bounds.height/2, z + parent->z, bounds.width, bounds.height, bounds.zeight);
  }
}

listener::listener(string fname, int fblock, int fcondition, string fbinding, int fx, int fy) {
  x = fx;
  y = fy;
  binding = fbinding;
  entityName = fname;
  block = fblock;
  condition = fcondition;

  g_listeners.push_back(this);
  //open and read from the script file
  ifstream stream;
  string loadstr;
  //try to open from local map folder first

  loadstr = "resources/maps/" + g_map + "/" + fbinding + ".event";
  const char* plik = loadstr.c_str();

  stream.open(plik);

  if (!stream.is_open()) {
    stream.open("events/" + fbinding + ".event");
  }
  string line;

  while (getline(stream, line)) {
    script.push_back(line);
  }
  parseScriptForLabels(script);
  //M("Check item script");

  //build listenList from current entities
  for(auto x : g_entities) {
    if(x->name == entityName) {
      listenList.push_back(x);
    }
  }
}

listener::~listener() {
  g_listeners.erase(remove(g_listeners.begin(), g_listeners.end(), this), g_listeners.end());
}

int listener::update() {
  if (!active) {return 0;}
  for(auto x : listenList) {
    if(x->data[block] == condition) {
      //do nothing
    } else {
      return 0;
    }
  }
  if(oneWay) {active = 0;}
  return 1;
}


escapeUI::escapeUI() {

  ninePatch = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", xStart, yStart, xEnd-xStart + 0.05, yEnd-yStart + 0.05, 0);
  ninePatch->patchwidth = 213;
  ninePatch->patchscale = 0.4;
  ninePatch->is9patch = true;
  ninePatch->persistent = true;
  ninePatch->show = 1;
  ninePatch->priority = 2;

  handMarker = new ui(renderer, "resources/static/ui/hand_selector.qoi", markerHandX, 0.1, markerWidth, 1, 2);
  handMarker->persistent = 1;
  handMarker->show = 1;
  handMarker->priority = 3;
  handMarker->heightFromWidthFactor = 1;
  handMarker->renderOverText = 1;

  fingerMarker = new ui(renderer, "resources/static/ui/finger_selector_angled.qoi", markerFingerX, 0.1, markerWidth, 1, 2);
  fingerMarker->persistent = 1;
  fingerMarker->show = 1;
  fingerMarker->priority = 3;
  fingerMarker->heightFromWidthFactor = 1;
  fingerMarker->renderOverText = 1;

  vector<string> optionStrings;
  
  optionStrings.push_back("Back");
  optionStrings.push_back("Levelselect");
  optionStrings.push_back("Settings");
  optionStrings.push_back("End");


  numLines = optionStrings.size();
  float spacing = (yEnd - yStart) / (numLines);

//    backButton = new ui(renderer, "resources/static/ui/menu_back.qoi", bbXStart, bbYStart, bbWidth, 1, 2);
//    backButton->heightFromWidthFactor = 1;
//    backButton->shrinkPercent = 0.03;
//    backButton->persistent = 1;
//    backButton->show = 0;
//    backButton->priority = 1;
//    backButton->dropshadow = 1;
//
//    bbNinePatch = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", bbXStart, bbYStart, bbWidth, 1, 0);
//    bbNinePatch->patchwidth = 213;
//    bbNinePatch->patchscale = 0.4;
//    bbNinePatch->is9patch = true;
//    bbNinePatch->persistent = true;
//    bbNinePatch->show = 1;
//    bbNinePatch->priority = 0;
//    bbNinePatch->heightFromWidthFactor = 1;


  int i = 0;
  for(float yPos = yStart + spacing/2; yPos < yEnd; yPos+= spacing ) 
  {
    textbox* newTextbox = new textbox(renderer, optionStrings[i].c_str(), 40 * g_fontsize, 0.5, yPos, 0);
    newTextbox->dropshadow = 1;
    newTextbox->show = 0;
    newTextbox->align = 2;
    optionTextboxes.push_back(newTextbox);
    
    i++;
  }

  maxPositionOfCursor = optionTextboxes.size() - 1;

  hide();
}

escapeUI::~escapeUI() {
  delete ninePatch;
  delete inventoryMarker;
  delete handMarker;
  delete fingerMarker;
  //delete backButton;

}

void escapeUI::show() {
  ninePatch->show = 1;
  uiSelecting();
  //backButton->show = 1;
  //bbNinePatch->show = 1;
  for(auto x : optionTextboxes) {
    x->show = 1;
  }
}

void escapeUI::hide() {
  ninePatch->show = 0;
  handMarker->show = 0;
  fingerMarker->show = 0;
  //backButton->show = 0;
  //bbNinePatch->show = 0;
  for(auto x : optionTextboxes) {
    x->show = 0;
  }
}

void escapeUI::uiModifying() {
  g_escapeUI->fingerMarker->show = 0;
  g_escapeUI->handMarker->show = 1;
}

void escapeUI::uiSelecting() {
  g_escapeUI->handMarker->show = 0;
  g_escapeUI->fingerMarker->show = 1;
}


//clear map
//CLEAR MAP
void clear_map(camera& cameraToReset) {
  g_budget = 0;
  enemiesMap.clear();
  g_ai.clear();
  g_musicalEntities.clear();
  g_boardableEntities.clear();
  g_objective = 0;
  g_familiars.clear();
  g_ex_familiars.clear();
  g_lt_collisions.clear();
  g_is_collisions.clear();
  if(g_dungeonSystemOn == 0) {
    g_behemoths.clear();
    g_behemoth0 = 0;
    g_behemoth1 = 0;
    g_behemoth2 = 0;
    g_behemoth3 = 0;
  }

  
  if(g_waterAllocated) {
    g_waterTexture = 0;
    g_waterAllocated = 0;
    SDL_FreeSurface(g_waterSurface);
  }

  adventureUIManager->crosshair->show = 0;
  if(!g_levelFlashing){
    Mix_FadeOutMusic(1000);

    //SDL_GL_SetSwapInterval(0);
    bool cont = false;
    float ticks = 0;
    float lastticks = 0;
    float transitionElapsed = 5;
    float mframes = 60;
    float transitionMinFrametime = 5;
    transitionMinFrametime = 1/mframes * 1000;


    SDL_Surface* transitionSurface = loadSurface("resources/engine/transition.qoi");

    int imageWidth = transitionSurface->w;
    int imageHeight = transitionSurface->h;

    SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
    SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);


    void* pixelReference;
    int pitch;

    float offset = imageHeight;

    SDL_Texture* frame = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);
    SDL_SetRenderTarget(renderer, frame);

    //to stop the clock and the goal text from rendering
    adventureUIManager->scoreText->show = 0;
    adventureUIManager->systemClock->show = 0;

    //render current frame to texture -- this is gonna get weird
    {

      if(g_backgroundLoaded && g_useBackgrounds) { //if the level has a background and the user would like to see it
        SDL_RenderCopy(renderer, background, NULL, NULL);
      }

      for(auto n : g_entities) {
        n->cooldown -= elapsed;
      }


      //tiles
      for(long long unsigned int i=0; i < g_tiles.size(); i++){
        if(g_tiles[i]->z ==0) {
          g_tiles[i]->render(renderer, g_camera);
        }
      }

      for(long long unsigned int i=0; i < g_tiles.size(); i++){
        if(g_tiles[i]->z ==1) {
          g_tiles[i]->render(renderer, g_camera);
        }
      }


      SDL_Rect FoWrect;

      //sort
      sort_by_y(g_actors);
      for(long long unsigned int i=0; i < g_actors.size(); i++){
        g_actors[i]->render(renderer, g_camera);
      }

      for(long long unsigned int i=0; i < g_tiles.size(); i++){
        if(g_tiles[i]->z == 2) {
          g_tiles[i]->render(renderer, g_camera);
        }
      }




      //Fogofwar
      if(g_fogofwarEnabled && !devMode) {

        // int functionalX = g_focus->getOriginX();
        // int functionalY = g_focus->getOriginY();

        // functionalX -= functionalX % 64;
        // functionalX += 32;
        // functionalY -= functionalY % 55;
        // functionalY += 26;

        // if(functionalX != g_lastFunctionalX || functionalY != g_lastFunctionalY) {
        // 	bool flipper = 0;
        // 	for(int i = 0; i < g_fogcookies.size(); i++) {
        // 		for(int j = 0; j < g_fogcookies[0].size(); j++) {
        // 			flipper = !flipper;
        // 			int xpos = ((i - g_fogMiddleX) * 64) + functionalX;
        // 			int ypos = ((j - g_fogMiddleY) * 55) + functionalY;
        // 			if(LineTrace(functionalX, functionalY, xpos, ypos, 0, 15, 0, 15, 1)) {
        // 				g_fogcookies[i][j] = 1;
        // 				g_fc[i][j] = 1;

        // 				g_sc[i][j] = 1;
        // 			} else {
        // 				g_fogcookies[i][j] = 0;

        // 				g_fc[i][j] = 0;
        // 				g_sc[i][j] = 0;
        // 			}
        // 		}
        // 	}
        // }

        //save cookies that are just dark because they are inside of walls to g_savedcookies
        // for(int i = 0; i < g_fogcookies.size(); i++) {
        // 	for(int j = 0; j < g_fogcookies[0].size(); j++) {
        // 		int xpos = ((i - 10) * 64) + functionalX;
        // 		int ypos = ((j - 9) * 55) + functionalY;
        // 		//is this cookie in a wall? or behind a wall
        // 		if(!LineTrace(xpos, ypos, xpos, ypos, 0, 15, 0, 2, 1)) {
        // 			g_fc[i][j] = 1;

        // 		}
        // 		if(!LineTrace(xpos, ypos + 55, xpos, ypos +55, 0, 15, 0, 2, 1)) {
        // 			g_fc[i][j] = 1;
        // 		}
        // 	}
        // }

        // g_lastFunctionalX = functionalX;
        // g_lastFunctionalY = functionalY;

        //these are the corners and the center
        // g_fogcookies[0][0] = 1;
        // g_fogcookies[20][0] = 1;
        // g_fogcookies[20][17] = 1;
        // g_fogcookies[0][17] = 1;
        // g_fogcookies[10][9] = 1;

        int px = -(int)g_focus->getOriginX() % 64;

        //offset us to the protag's location
        //int yoffset =  ((g_focus->y- (g_focus->z + g_focus->zeight) * XtoZ)) * g_camera.zoom;
        //the zeight is constant at level 2  for now
        int yoffset =  (g_focus->getOriginY() ) * g_camera.zoom;

        //and then subtract half of the screen
        yoffset -= yoffset % 55;
        yoffset -= (g_fogheight * 55 + 12)/2;
        yoffset -= g_camera.y;

        //we do this nonsense to keep the offset on the grid
        //yoffset -= yoffset % 55;

        //px = 64 - px - 64;
        //py = 55 - py - 55;
        // 50 50
        SDL_SetRenderTarget(renderer, NULL);
        addTextures(renderer, g_fc, canvas, light, 500, 500, 250, 250, 0);


        TextureC = IlluminateTexture(renderer, TextureA, canvas, result);

        //render graphics
        FoWrect = {px - 23, yoffset +15, g_fogwidth * 64 + 50, g_fogheight * 55 + 18};
        SDL_SetRenderTarget(renderer, frame);
        SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);

        //do it for z = 64
        FoWrect.y -= 64 * XtoZ;
        SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);


        SDL_SetRenderTarget(renderer, NULL);
        addTextures(renderer, g_sc, canvas, light, 500, 500, 250, 250, 1);


        TextureC = IlluminateTexture(renderer, TextureA, canvas, result);
        SDL_SetRenderTarget(renderer, frame);

        //render graphics
        FoWrect.y -= 67 * XtoZ;
        SDL_RenderCopy(renderer, TextureC, NULL, &FoWrect);

        //black bars
        SDL_Rect topbar = {px, FoWrect.y - 5000, 1500, 5000};
        SDL_RenderCopy(renderer, blackbarTexture, NULL, &topbar);

        SDL_Rect botbar = {px, FoWrect.y +  g_fogheight * 55 + 12, 1500, 5000};
        SDL_RenderCopy(renderer, blackbarTexture, NULL, &botbar);
        SDL_RenderPresent(renderer);

      }



      //ui
      // if(!inPauseMenu && g_showHUD) {
      // 	// !!! segfaults on mapload sometimes
      // 	adventureUIManager->healthText->updateText( to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), WIN_WIDTH * g_minifontsize, 0.9);
      // 	adventureUIManager->healthText->show = 1;

      // } else {
      // 	adventureUIManager->healthText->show = 0;

      // }

      // //move the healthbar properly to the protagonist
      // rect obj; // = {( , (((protag->y - ((protag->height))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom, (protag->width * g_camera.zoom), (protag->height * g_camera.zoom))};
      // obj.x = ((protag->x -g_camera.x) * g_camera.zoom);
      // obj.y = (((protag->y - ((floor(protag->height)* 0.9))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom;
      // obj.width = (protag->width * g_camera.zoom);
      // obj.height = (floor(protag->height) * g_camera.zoom);

      // protagHealthbarA->x = (((float)obj.x + obj.width/2) / (float)WIN_WIDTH) - protagHealthbarA->width/2.0;
      // protagHealthbarA->y = ((float)obj.y) / (float)WIN_HEIGHT;
      // protagHealthbarB->x = protagHealthbarA->x;
      // protagHealthbarB->y = protagHealthbarA->y;

      // protagHealthbarC->x = protagHealthbarA->x;
      // protagHealthbarC->y = protagHealthbarA->y;
      // protagHealthbarC->width = (protag->hp / protag->maxhp) * 0.05;
      // adventureUIManager->healthText->boxX = protagHealthbarA->x + protagHealthbarA->width/2;
      // adventureUIManager->healthText->boxY = protagHealthbarA->y - 0.005;

      for (long long unsigned int i = 0; i < g_ui.size(); i++)
      {
        if(!g_ui[i]->renderOverText) {
          g_ui[i]->render(renderer, g_camera, 0);
        }
      }
      for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
      {
        g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      }
  
      //some ui are rendered over text
      for (long long unsigned int i = 0; i < g_ui.size(); i++)
      {
        if(g_ui[i]->renderOverText) {
          g_ui[i]->render(renderer, g_camera, 0);
        }
      }

      //this is the menu for quitting or going back to the "overworld"
      if (g_inEscapeMenu) 
      {
        //move reticle to the correct position
        g_escapeUI->handMarker->targety
          = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
          + (g_escapeUI->handOffset);
  
        g_escapeUI->handMarker->targetx
          = g_escapeUI->markerHandX;
  
        g_escapeUI->fingerMarker->targety
          = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
          + (g_escapeUI->fingerOffset);
  
        float ww = WIN_WIDTH;
        float fwidth = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->width;
        g_escapeUI->fingerMarker->targetx
          = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxX + 
            fwidth / ww / 2;
        
  
  
        if(g_firstFrameOfSettingsMenu) {
          g_firstFrameOfSettingsMenu = 0;
          g_escapeUI->handMarker->x = g_escapeUI->handMarker->targetx;
          g_escapeUI->handMarker->y = g_escapeUI->handMarker->targety;
          g_escapeUI->fingerMarker->x = g_escapeUI->fingerMarker->targetx;
          g_escapeUI->fingerMarker->y = g_escapeUI->fingerMarker->targety;
  
        }
  
      }

      // draw pause screen
      if (inPauseMenu)
      {
        adventureUIManager->crosshair->x = 5;
  
        // iterate thru inventory and draw items on screen
        float defaultX = WIN_WIDTH * 0.05;
        float defaultY = WIN_HEIGHT * adventureUIManager->inventoryYStart;
        float x = defaultX;
        float y = defaultY;
        float maxX = WIN_WIDTH * 0.9;
        float maxY = WIN_HEIGHT * adventureUIManager->inventoryYEnd;
        float itemWidth = WIN_WIDTH * 0.07;
        float padding = WIN_WIDTH * 0.01;
  
        int i = 0;
  
        if (g_inventoryUiIsLevelSelect == 0) {
          //populate boxes based on inventory
          for (auto it = mainProtag->inventory.rbegin(); it != mainProtag->inventory.rend(); ++it)
          {
    
            if (i < itemsPerRow * inventoryScroll)
            {
              // this item won't be rendered
              i++;
              continue;
            }
    
            SDL_Rect drect = {(int)x, (int)y, (int)itemWidth, (int)itemWidth};
            if (it->second > 0)
            {
              SDL_RenderCopy(renderer, it->first->texture, NULL, &drect);
            }
            // draw number
            if (it->second > 1)
            {
              inventoryText->show = 1;
              inventoryText->updateText(to_string(it->second), -1, 100);
              inventoryText->boxX = (x + (itemWidth * 0.8)) / WIN_WIDTH;
              inventoryText->boxY = (y + (itemWidth - inventoryText->boxHeight / 2) * 0.6) / WIN_HEIGHT;
              inventoryText->worldspace = 1;
              inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
            }
            else
            {
              inventoryText->show = 0;
            }
    
            if (i == inventorySelection)
            {
              // this item should have the marker
              inventoryMarker->show = 1;
              inventoryMarker->x = x / WIN_WIDTH;
              inventoryMarker->y = y / WIN_HEIGHT;
              inventoryMarker->x += 0.02 * 40 * ((float)WIN_WIDTH / (float)WIN_HEIGHT); 
              inventoryMarker->y += 0.03 * 40 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              inventoryMarker->width = itemWidth / WIN_WIDTH;
    
              float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
              inventoryMarker->x -= biggen;
              inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              inventoryMarker->width += biggen * 2;
              inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }
    
            x += itemWidth + padding;
            if (x > maxX)
            {
              x = defaultX;
              y += itemWidth + padding;
              if (y > maxY)
              {
                // we filled up the entire inventory, so lets leave
                break;
              }
            }
            i++;
          }
          g_itemsInInventory = mainProtag->inventory.size();
    
          if (mainProtag->inventory.size() > 0 && mainProtag->inventory.size() - 1 - inventorySelection < mainProtag->inventory.size())
          {
            string description = mainProtag->inventory[mainProtag->inventory.size() - 1 - inventorySelection].first->script[0];
            // first line is a comment so take off the //
            description = description.substr(2);
            adventureUIManager->escText->updateText(description, -1, 0.9);
          }
          else
          {
            adventureUIManager->escText->updateText("No items in inventory", -1, 0.9);
          }
        } else {
          //populate the UI based on the loaded level sequence.
          for(int j = 0; j < g_levelSequence->levelNodes.size(); j++) {
            if( i < itemsPerRow * inventoryScroll) {
              i++;
              continue;
            }
            SDL_Rect drect = {(int)x, (int)y, (int)itemWidth, (int)itemWidth}; 
            int boosh = 5;
            drect.w += boosh * 2;
            drect.h += boosh * 2;
            drect.x -= boosh;
            drect.y -= boosh;
  
            //should we draw the locked graphic?
            if(g_levelSequence->levelNodes[j]->locked) {
              SDL_RenderCopy(renderer, g_locked_level_texture, NULL, &drect);
            } else {
              SDL_RenderCopy(renderer, g_levelSequence->levelNodes[j]->sprite, NULL, &drect);
            }
  
            if (i == inventorySelection)
            {
  
              if(g_levelSequence->levelNodes[i]->locked) {
                adventureUIManager->escText->updateText("Locked", -1, 0.9);
              } else {
                string dispText = g_levelSequence->levelNodes[i]->name;
                std::replace(dispText.begin(), dispText.end(),'_',' ');
                adventureUIManager->escText->updateText(g_levelSequence->levelNodes[i]->name, -1, 0.9);
  
              }
  
              // this item should have the marker
              inventoryMarker->show = 1;
              float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
                                   
              if(g_firstFrameOfPauseMenu) {
                inventoryMarker->x = x / WIN_WIDTH;
                inventoryMarker->y = y / WIN_HEIGHT;
                inventoryMarker->x -= biggen;
                inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->x += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targetx = inventoryMarker->x;
                inventoryMarker->targety = inventoryMarker->y;
                g_firstFrameOfPauseMenu = 0;
              } else {
                inventoryMarker->targetx = x / WIN_WIDTH;
                inventoryMarker->targety = y / WIN_HEIGHT;
                inventoryMarker->targetx -= biggen;
                inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                //now that it's a hand
                inventoryMarker->targetx += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
                inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
              }

              inventoryMarker->width = itemWidth / WIN_WIDTH;
    
              inventoryMarker->width += biggen * 2;
              inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            }
    
            x += itemWidth + padding;
            if (x > maxX)
            {
              x = defaultX;
              y += itemWidth + padding;
              if (y > maxY)
              {
                // we filled up the entire inventory, so lets leave
                break;
              }
            }
            i++;
  
          }
          g_itemsInInventory = g_levelSequence->levelNodes.size();
          
        }
  
        //re-render inventory reticle so it goes on top of the items/level icons
        inventoryMarker->render(renderer, g_camera, 0);
        inventoryMarker->show = 0;
      }
      else
      {
        inventoryMarker->show = 0;
        inventoryText->show = 0;
      }


      SDL_RenderCopy(renderer, g_shade, NULL, NULL);
      //SDL_RenderPresent(renderer);
    }

    //to stop the clock and the goal text from rendering
    adventureUIManager->scoreText->show = 1;
    adventureUIManager->systemClock->show = 1;

    SDL_SetRenderTarget(renderer, NULL);
    while (!cont) {

      //onframe things
      SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

      memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
      Uint32 format = SDL_PIXELFORMAT_ARGB8888;
      SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
      Uint32* pixels = (Uint32*)pixelReference;
      //int numPixels = imageWidth * imageHeight;
      Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);
      //Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);

      offset += g_transitionSpeed + 0.02 * offset;

      for(int x = 0;  x < imageWidth; x++) {
        for(int y = 0; y < imageHeight; y++) {


          int dest = (y * imageWidth) + x;
          //int src =  (y * imageWidth) + x;

          if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
            pixels[dest] = transparent;
          } else {
            // if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < 10 + offset) {
            // 	pixels[dest] = halftone;
            // } else {
            pixels[dest] = 0;
            // }
          }

        }
      }





      ticks = SDL_GetTicks();
      transitionElapsed = ticks - lastticks;
      //lock framerate
      if(transitionElapsed < transitionMinFrametime) {
        SDL_Delay(transitionMinFrametime - transitionElapsed);
        ticks = SDL_GetTicks();
        transitionElapsed = ticks - lastticks;
      }
      lastticks = ticks;

      SDL_RenderClear(renderer);
      //render last frame
      SDL_RenderCopy(renderer, frame, NULL, NULL);
      SDL_UnlockTexture(transitionTexture);
      SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
      SDL_RenderPresent(renderer);

      if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
        cont = 1;
      }
    }
    SDL_FreeSurface(transitionSurface);
    SDL_DestroyTexture(transitionTexture);
    transition = 1;
    SDL_GL_SetSwapInterval(1);
  }

  cameraToReset.resetCamera();
  int size;
  size = (int)g_entities.size();

  g_actors.clear();

  g_pellets.clear();

  navNodeMap.clear();


  //copy protag to a pointer, clear the array, and re-add protag
  vector<entity*> persistentEnts;
  for(int i=0; i< size; i++) {
    if(g_entities[0]->inParty) {
      //remove from array without deleting
      g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());

      g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());
    } else if (g_entities[0]->persistentHidden) {
      //do nothing because nar is handled differently now
      persistentEnts.push_back(g_entities[0]);
      g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());
      g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());
    } else if(g_entities[0]->persistentGeneral) {
      g_entities[0]->current = 0;
      g_entities[0]->dest = 0;
      g_entities[0]->Destination = 0;

      persistentEnts.push_back(g_entities[0]);
      g_entities.erase(remove(g_entities.begin(), g_entities.end(), g_entities[0]), g_entities.end());
      g_actors.erase(remove(g_actors.begin(), g_actors.end(), g_entities[0]), g_actors.end());



    } else {
      delete g_entities[0];
    }
  }

  g_behemoths.clear();
  g_behemoth0 = 0;
  g_behemoth1 = 0;
  g_behemoth2 = 0;
  g_behemoth3 = 0;

  for(int i = 0; i < g_ribbons.size(); i++) {
    delete g_ribbons[0];
  }

  //push back any entities that were in the party
  for (long long unsigned int i = 0; i < party.size(); i++) {
    g_entities.push_back(party[i]);
    g_actors.push_back(party[i]);
  }
  //push back any shadows we want to keep
  for(auto n : g_shadows) {
    g_actors.push_back(n);
  }

  //push back any ents that were persisent (arms)
  for(auto n : persistentEnts) {
    g_entities.push_back(n);
    g_actors.push_back(n);
  }


  //clear lastReferencedEnt
  //possible issue with adventureUI instances of persistent ents
  //with lastReferencedEntity fields set to deleted entity
  adventureUIManager->lastReferencedEntity = 0;

  size = (int)g_tiles.size();
  for(int i = 0; i < size; i++) {
    delete g_tiles[0];
  }

  size = (int)g_emitters.size();
  for(int i = 0; i < size; i++) {
    delete g_emitters[0];
  }

  size = (int)g_navNodes.size();
  for(int i = 0; i < size; i++) {
    delete g_navNodes[0];
  }

  g_pelletGoalScripts.clear();
 

  //was there a reason to do this?
  //none of the entities which persist through map clears
  //should ever have worldsounds
//  vector<worldsound*> savedSounds;
//
//  size = (int)g_worldsounds.size();
//  for(int i = 0; i < size; i++) {
//    if(g_worldsounds[0]->owner == nullptr) {
//      delete g_worldsounds[0];
//    } else {
//      savedSounds.push_back(g_worldsounds[0]);
//      g_worldsounds.erase(remove(g_worldsounds.begin(), g_worldsounds.end(), g_worldsounds[0]), g_worldsounds.end());
//    }
//  }
//
//  //put savedSounds back to g_worldsounds
//  for(auto s : savedSounds) {
//
//
//  }
  
  //just do a typical clear
  size = (int)g_worldsounds.size();
  for(int i = 0; i < size; i++) {
    delete g_worldsounds[0];
  }

  size = (int)g_musicNodes.size();
  for(int i = 0; i < size; i++) {
    delete g_musicNodes[0];
  }

  size = (int)g_cueSounds.size();
  for(int i = 0; i < size; i++) {
    delete g_cueSounds[0];
  }

  size = (int)g_waypoints.size();
  for(int i = 0; i < size; i++) {
    delete g_waypoints[0];
  }

  size = (int)g_doors.size();
  for(int i = 0; i < size; i++) {
    delete g_doors[0];
  }

  size = (int)g_dungeonDoors.size();
  for(int i = 0; i < size; i++) {
    delete g_dungeonDoors[0];
  }

  size = (int)g_triggers.size();
  for(int i = 0; i < size; i++) {
    delete g_triggers[0];
  }

  size = (int)g_heightmaps.size();
  for(int i = 0; i < size; i++) {
    delete g_heightmaps[0];
  }

  size = (int)g_listeners.size();
  for(int i = 0; i < size; i++) {
    delete g_listeners[0];
  }

  vector<effectIndex*> savedEffectIndexes;
  size = (int)g_effectIndexes.size();
  for(int i = 0; i < size; i++) {
    if(g_effectIndexes[0]->persistent) {
      savedEffectIndexes.push_back(g_effectIndexes[0]);
      g_effectIndexes.erase(remove(g_effectIndexes.begin(), g_effectIndexes.end(), g_effectIndexes[0]), g_effectIndexes.end());
    } else {
      delete g_effectIndexes[0];
    }
  }
  
  for(auto x : savedEffectIndexes) {
    g_effectIndexes.push_back(x);
  }


  g_particles.clear();
                       
  //used to delete attacks and then weapons
  //but that's going to cause a segfault since weapons
  //delete their own attacks
//  size = g_attacks.size();
//  bool contflag = 0;
//  for(int i = 0; i < size; i++) {
//    for(auto x : protag->hisweapon->attacks) {
//      if(x == g_attacks[0]) {
//        swap(g_attacks[0], g_attacks[g_attacks.size()-1]);
//        contflag = 1;
//        break;
//
//
//      }
//
//    }
//    if(!contflag) {
//      delete g_attacks[0];
//    }
//  }

  vector<weapon*> persistentweapons;
  size = (int)g_weapons.size();
  for(int i = 0; i < size; i++) {
    bool persist = false;
    //check if party members own the weapons
    for(auto x: party) {
      if(x->hisweapon->name == g_weapons[0]->name) {
        persist  = true;
      }
    }
    if(g_weapons[0]->persistent) {
      persist = true;

    }
    if(persist) {
      persistentweapons.push_back(g_weapons[0]);
      g_weapons.erase(remove(g_weapons.begin(), g_weapons.end(), g_weapons[0]), g_weapons.end());
    } else {
      delete g_weapons[0];
    }
  }

  for(auto x : persistentweapons) {
    g_weapons.push_back(x);
  }


  vector<ui*> persistentui;
  size = (int)g_ui.size();
  for(int i = 0; i < size; i++) {
    if(g_ui[0]->persistent) {
      persistentui.push_back(g_ui[0]);
      g_ui.erase(remove(g_ui.begin(), g_ui.end(), g_ui[0]), g_ui.end());
    } else {
      delete g_ui[0];
    }
  }

  for(auto x : persistentui) {
    g_ui.push_back(x);
  }


  g_solid_entities.clear();

  //unloading takes too long- probably because whenever a mapObject is deleted we search the array
  //lets try clearing the array first, because we should delete everything properly afterwards anyway
  g_mapObjects.clear();

  //new, delete all mc, which will automatycznie delete the others
  //here's where we could save some textures if we're going to a map in the same level, might be worth it
  size = (int)g_mapCollisions.size();
  for (int i = 0; i < size; i++) {
    //M("Lets delete a mapCol");
    int jsize = (int)g_mapCollisions[0]->children.size();
    for (int j = 0; j < jsize; j ++) {
      //M("Lets delete a mapCol child");
      delete g_mapCollisions[0]->children[j];
    }
    delete g_mapCollisions[0];
  }

  for(auto x : g_collisionZones) {
    delete x;
  }
  g_collisionZones.clear();

  //clear layers of boxes and triangles
  for(long long unsigned int i = 0; i < g_boxs.size(); i++) {
    g_boxs[i].clear();
  }
  for(long long unsigned int i = 0; i < g_triangles.size(); i++) {
    g_triangles[i].clear();
  }
  for(long long unsigned int i = 0; i < g_ramps.size(); i++) {
    g_ramps[i].clear();
  }
  if(g_backgroundLoaded && background != 0) {
    //M("deleted background");
    SDL_DestroyTexture(background);
    extern string backgroundstr;
    backgroundstr = "";
    background = 0;
  }

  for(int i = 0; i < g_numberOfInterestSets; i++) {
    while(g_setsOfInterest[i].size() > 0) {
      delete g_setsOfInterest[i][0];
    }
  }


  newClosest = 0;
}

//make an entity from the file worlditem.ent
worldItem::worldItem(string fname, bool fisKeyItem) : entity(renderer, 5, fname) {
  isWorlditem = 1;
  name = "ITEM-" + fname;
  bounceindex = rand() % 8;
  g_worldItems.push_back(this);

}

worldItem::~worldItem() {
  g_worldItems.erase(remove(g_worldItems.begin(), g_worldItems.end(), this), g_worldItems.end());
}


settingsUI::settingsUI() {

  ninePatch = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", xStart, yStart, xEnd-xStart + 0.05, yEnd-yStart + 0.05, 0);
  ninePatch->patchwidth = 213;
  ninePatch->patchscale = 0.4;
  ninePatch->is9patch = true;
  ninePatch->persistent = true;
  ninePatch->show = 1;
  ninePatch->priority = 0;

  handMarker = new ui(renderer, "resources/static/ui/hand_selector.qoi", markerHandX, 0.1, markerWidth, 1, 2);
  handMarker->persistent = 1;
  handMarker->show = 1;
  handMarker->priority = 3;
  handMarker->heightFromWidthFactor = 1;
  handMarker->renderOverText = 1;

  fingerMarker = new ui(renderer, "resources/static/ui/finger_selector_angled.qoi", markerFingerX, 0.1, markerWidth, 1, 2);
  fingerMarker->persistent = 1;
  fingerMarker->show = 1;
  fingerMarker->priority = 3;
  fingerMarker->heightFromWidthFactor = 1;
  fingerMarker->renderOverText = 1;

  vector<string> optionStrings;
  
  optionStrings.push_back("Up");
  optionStrings.push_back("Down");
  optionStrings.push_back("Left");
  optionStrings.push_back("Right");
  optionStrings.push_back("Spring");
  optionStrings.push_back("Interact");
  optionStrings.push_back("Select Item");
  optionStrings.push_back("Use Item");

  optionStrings.push_back("Maximize");
  optionStrings.push_back("Music Volume");
  optionStrings.push_back("Sounds Volume");
  optionStrings.push_back("Graphical Quality");
  optionStrings.push_back("Brightness");

  numLines = optionStrings.size();
  float spacing = (yEnd - yStart) / (numLines);

  backButton = new ui(renderer, "resources/static/ui/menu_back.qoi", bbXStart, bbYStart, bbWidth, 1, 2);
  backButton->heightFromWidthFactor = 1;
  backButton->shrinkPercent = 0.03;
  backButton->persistent = 1;
  backButton->show = 1;
  backButton->priority = 1;
  backButton->dropshadow = 1;

  bbNinePatch = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", bbXStart, bbYStart, bbWidth, 1, 0);
  bbNinePatch->patchwidth = 213;
  bbNinePatch->patchscale = 0.4;
  bbNinePatch->is9patch = true;
  bbNinePatch->persistent = true;
  bbNinePatch->show = 1;
  bbNinePatch->priority = 0;
  bbNinePatch->heightFromWidthFactor = 1;


  int i = 0;
  for(float yPos = yStart + spacing/2; yPos < yEnd; yPos+= spacing ) 
  {
    textbox* newTextbox = new textbox(renderer, optionStrings[i].c_str(), 30 * g_fontsize, xStart + (spacing/2 * (WIN_WIDTH/WIN_HEIGHT)), yPos, xEnd);
    newTextbox->dropshadow = 1;
    newTextbox->show = 0;
    optionTextboxes.push_back(newTextbox);
    
    string content = "";
    if(i == 0) {content = SDL_GetScancodeName(bindings[0]);}
    if(i == 1) {content = SDL_GetScancodeName(bindings[1]);}
    if(i == 2) {content = SDL_GetScancodeName(bindings[2]);}
    if(i == 3) {content = SDL_GetScancodeName(bindings[3]);}

    if(i == 4) {content = SDL_GetScancodeName(bindings[8]);}
    if(i == 5) {content = SDL_GetScancodeName(bindings[11]);}
    if(i == 6) {content = SDL_GetScancodeName(bindings[12]);}
    if(i == 7) {content = SDL_GetScancodeName(bindings[13]);}

    if(i == 8) {content = (g_fullscreen) ? g_affirmStr : g_negStr; }
    if(i == 9) {content = to_string((int)round(g_music_volume * 100)) + "%"; }
    if(i == 10) {content = to_string((int)round(g_sfx_volume * 100)) + "%"; }
    if(i == 11) {content = g_graphicsStrings[g_graphicsquality]; }
    if(i == 12) {content = to_string((int)round(g_brightness)) + "%"; }

    newTextbox = new textbox(renderer, content.c_str(), 30 * g_fontsize, xEnd + 0.02, yPos, 0.3);
    newTextbox->show = 0;
    newTextbox->align = 1;
    newTextbox->dropshadow = 1;
    valueTextboxes.push_back(newTextbox);


    i++;
  }

  maxPositionOfCursor = optionTextboxes.size() - 1;

  hide();
}

settingsUI::~settingsUI() {
  delete ninePatch;
  delete inventoryMarker;
  delete handMarker;
  delete fingerMarker;
  delete backButton;

}

void settingsUI::show() {
  M("settingsUI::show()");
  ninePatch->show = 1;
  uiSelecting();
  backButton->show = 1;
  bbNinePatch->show = 1;
  for(auto x : optionTextboxes) {
    x->show = 1;
  }
  for(auto x : valueTextboxes) {
    x->show = 1;
  }
}

void settingsUI::hide() {
  ninePatch->show = 0;
  handMarker->show = 0;
  fingerMarker->show = 0;
  backButton->show = 0;
  bbNinePatch->show = 0;
  for(auto x : optionTextboxes) {
    x->show = 0;
  }
  for(auto x : valueTextboxes) {
    x->show = 0;
  }
}

void settingsUI::uiModifying() {
  g_settingsUI->fingerMarker->show = 0;
  g_settingsUI->handMarker->show = 1;
}

void settingsUI::uiSelecting() {
  g_settingsUI->handMarker->show = 0;
  g_settingsUI->fingerMarker->show = 1;
}



//I added this to help debug a problem with multiple copies of UI elements
void debugUI() {
  D(g_ui.size());
  for(auto x : g_ui) {
    D(x->filename);
  }
}

void adventureUI::hideBackpackUI() {
  t1->show = 0;
  t2->show = 0;
  t3->show = 0;
  t4->show = 0;
  t5->show = 0;
}

void adventureUI::showBackpackUI() {
  t1->show = 1;
  t2->show = 1;
  t3->show = 1;
  t4->show = 1;
  t5->show = 1;
}

void adventureUI::resetBackpackUITextures() {
  t1->texture = noIconTexture;
  t2->texture = noIconTexture;
  t3->texture = noIconTexture;
  t4->texture = noIconTexture;
  t5->texture = noIconTexture;
}

void adventureUI::showTalkingUI()
{
  // M("showTalkingUI()");
  talkingBox->show = 1;
  //dialogProceedIndicator->show = 1;
  // talkingBoxTexture->show = 1;
  talkingText->show = 1;
  talkingText->updateText("", -1, 34);
  responseText->show = 1;
  responseText->updateText("", -1, 34);
}

void adventureUI::hideTalkingUI()
{
  // M("hideTalkingUI()");
  talkingBox->show = 0;
  dialogProceedIndicator->show = 0;
  //dialogProceedIndicator->y = 0.88;
  // talkingBoxTexture->show = 0;
  talkingText->show = 0;
  currentTextcolor = defaultTextcolor;
  currentFontStr = defaultFontStr;
  talkingText->updateText("", -1, 34, defaultTextcolor);
  responseText->show = 0;
  responseText->updateText("", -1, 34, defaultTextcolor);
}

void adventureUI::showScoreUI() {
  scoreText->show = 1;
}

void adventureUI::hideScoreUI() {
  scoreText->show = 0;
  //scoreText->updateText("", 34, 34);
}

void adventureUI::showInventoryUI()
{
  if(!light) {
    inventoryA->show = 1;
    inventoryB->show = 1;
    escText->show = 1;
    levelTimeText->show = 1;
    levelHitsText->show = 1;
    inputText->show = 1;
  }
}

void adventureUI::hideInventoryUI()
{
  if(!light) {
    inventoryA->show = 0;
    inventoryB->show = 0;
    escText->show = 0;
    levelTimeText->show = 0;
    levelHitsText->show = 0;
    escText->updateText("",-1,1);
    levelTimeText->updateText("",-1,1);
    levelHitsText->updateText("",-1,1);
    inputText->show = 0;
    inputText->updateText("", -1, 0.9);
  }
}

adventureUI::adventureUI(SDL_Renderer *renderer, bool plight) //a bit strange, but due to the declaration plight is 0 by default
{
  this->light = plight;
  if(!light) {
    talkingBox = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
    talkingBox->patchwidth = 213;
    talkingBox->patchscale = 0.4;
    talkingBox->is9patch = true;
    talkingBox->persistent = true;

    dialogProceedIndicator = new ui(renderer, "resources/static/ui/dialog_proceed.qoi", 0.92, 0.88, 0.05, 1, 0);
    dialogProceedIndicator->heightFromWidthFactor = 1;
    dialogProceedIndicator->persistent = true;
    dialogProceedIndicator->priority = 8;
    dialogProceedIndicator->dropshadow = 1;
    

    // talkingBoxTexture = new ui(renderer, "resources/static/ui/ui-background.qoi", 0.1, 0.45, 0.9, 0.25, 0);
    // talkingBoxTexture->persistent = true;
    /// SDL_SetTextureBlendMode(talkingBoxTexture->texture, SDL_BLENDMODE_ADD);

    talkingText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
    talkingText->boxWidth = 0.9;
    talkingText->width = 0.9;
    talkingText->boxHeight = 0.25;
    talkingText->boxX = 0.05;
    talkingText->boxY = 0.7;
    talkingText->worldspace = 1; //right align
    talkingText->dropshadow = 1;

    responseText = new textbox(renderer, "Yes", 1700 * g_fontsize, 0, 0, 0.9);
    responseText->boxWidth = 0.95;
    responseText->width = 0.95;
    responseText->boxHeight = 0.2;
    responseText->boxX = 0.5;
    responseText->boxY = 0.87;
    responseText->worldspace = 1;
    responseText->align = 2; // center-align
    responseText->dropshadow = 1;
                             

    scoreText = new textbox(renderer, "Yes", 1700 * g_fontsize, 0, 0, 0.9);
    scoreText->boxWidth = 0;
    scoreText->width = 0.95;
    scoreText->boxHeight = 0;
    scoreText->boxX = 0.2;
    scoreText->boxY = 1-0.1;
    scoreText->worldspace = 1;
    scoreText->align = 1; // right
    scoreText->dropshadow = 1;
    scoreText->layer0 = 1;
    scoreText->show = 0;

    systemClock = new textbox(renderer, "Yes", 1700 * g_fontsize, 0, 0, 0.9);
    systemClock->boxWidth = 1;
    systemClock->width = 0.95;
    systemClock->boxHeight = 0;
    systemClock->boxX = 1 -0.2;
    systemClock->boxY = 1-0.1;
    systemClock->worldspace = 1;
    systemClock->align = 2; // left
    systemClock->dropshadow = 1;
    systemClock->layer0 = 1;
    //systemClock->show = 0;


    inventoryA = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.01, 0.01, 0.98, 0.75 - 0.01, 1);
    inventoryA->is9patch = true;
    inventoryA->patchwidth = 213;
    inventoryA->patchscale = 0.4;
    inventoryA->persistent = true;
    inventoryA->show = 0;

    inventoryB = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.01, 0.75 + 0.01, 0.98, 0.25 - 0.02, 1);
    inventoryB->is9patch = true;
    inventoryB->patchwidth = 213;
    inventoryB->patchscale = 0.4;
    inventoryB->persistent = true;
    inventoryB->priority = -4;

    crosshair = new ui(renderer, "resources/static/ui/crosshair.qoi", 0, 0, 0.05, 0.05, -15);
    crosshair->persistent = 1;
    crosshair->heightFromWidthFactor = 1;
    crosshair->show = 0;
    crosshair->xframes = 4;
    crosshair->framewidth = 128;
    crosshair->frameheight = 128;
    crosshair->priority = -5; //crosshair goes ontop usable icons
    
    b0_element = new ui(renderer, "resources/static/ui/behemoth_element.qoi", 0, 0, 0.05, 0.05, -15);
    b0_element->persistent = 1;
    b0_element->heightFromWidthFactor = 1;
    b0_element->show = 0;
    b0_element->xframes = 4;
    b0_element->framewidth = 128;
    b0_element->frameheight = 128;
    b0_element->priority = -5; //crosshair goes ontop usable icons

    b1_element = new ui(renderer, "resources/static/ui/behemoth_element.qoi", 0, 0, 0.05, 0.05, -15);
    b1_element->persistent = 1;
    b1_element->heightFromWidthFactor = 1;
    b1_element->show = 0;
    b1_element->xframes = 4;
    b1_element->frame = 1;
    b1_element->framewidth = 128;
    b1_element->frameheight = 128;
    b1_element->priority = -5; //crosshair goes ontop usable icons


    b2_element = new ui(renderer, "resources/static/ui/behemoth_element.qoi", 0, 0, 0.05, 0.05, -15);
    b2_element->persistent = 1;
    b2_element->heightFromWidthFactor = 1;
    b2_element->show = 0;
    b2_element->xframes = 4;
    b2_element->frame = 2;
    b2_element->framewidth = 128;
    b2_element->frameheight = 128;
    b2_element->priority = -5; //crosshair goes ontop usable icons

    b3_element = new ui(renderer, "resources/static/ui/behemoth_element.qoi", 0, 0, 0.05, 0.05, -15);
    b3_element->persistent = 1;
    b3_element->heightFromWidthFactor = 1;
    b3_element->show = 0;
    b3_element->xframes = 4;
    b3_element->frame = 3;
    b3_element->framewidth = 128;
    b3_element->frameheight = 128;
    b3_element->priority = -5; //crosshair goes ontop usable icons
    
    hearingDetectable = new ui(renderer, "resources/static/ui/detection-hearing.qoi", 0.85, 0.05, 0.1, 1, -10);
    hearingDetectable->persistent = 1;
    hearingDetectable->heightFromWidthFactor = 1.3392;
    hearingDetectable->show = 1;
    hearingDetectable->priority = -3;

    seeingDetectable = new ui(renderer, "resources/static/ui/detection-seeing.qoi", 0.85, 0.075, 0.1, 1, -10);
    seeingDetectable->persistent = 1;
    seeingDetectable->heightFromWidthFactor = 1;
    seeingDetectable->xframes = 8;
    seeingDetectable->msPerFrame = 100;
    seeingDetectable->framewidth = 256;
    seeingDetectable->frameheight = 256;
    seeingDetectable->show = 1;
    seeingDetectable->priority = -2;



//    healthText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
//    healthText->boxWidth = 0.95;
//    healthText->width = 0.95;
//    healthText->boxHeight = 0;
//    healthText->boxX = 0.05;
//    healthText->boxY = 0.15; //0.3 to get it under the heart
//    healthText->worldspace = 1;
//    healthText->show = 1;
//    healthText->align = 0;
//    healthText->dropshadow = 1;
//    healthText->layer0 = 1;

//    hungerText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
//    hungerText->boxWidth = 0.95;
//    hungerText->width = 0.95;
//    hungerText->boxHeight = 0;
//    hungerText->boxX = 1 - 0.05;
//    hungerText->boxY = 1 - 0.15;
//    hungerText->worldspace = 1;
//    hungerText->show = 1;
//    hungerText->align = 1;
//    hungerText->dropshadow = 1;
//    hungerText->layer0 = 1;

    escText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);

    escText->boxX = 0.5;
    escText->boxY = 0.83;
    escText->boxWidth = 0.98;
    escText->boxHeight = 0.25 - 0.02;
    escText->worldspace = 0;
    escText->show = 1;
    escText->align = 2;
    escText->dropshadow = 1; 

    levelHitsText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);

    levelHitsText->boxX = 0.1;
    levelHitsText->boxY = 0.83;
    levelHitsText->boxWidth = 0.98;
    levelHitsText->boxHeight = 0.25 - 0.02;
    levelHitsText->worldspace = 0;
    levelHitsText->show = 1;
    levelHitsText->align = 0;
    levelHitsText->dropshadow = 1; 

    levelTimeText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);

    levelTimeText->boxX = 0.9;
    levelTimeText->boxY = 0.83;
    levelTimeText->boxWidth = 0.98;
    levelTimeText->boxHeight = 0.25 - 0.02;
    levelTimeText->worldspace = 0;
    levelTimeText->show = 1;
    levelTimeText->align = 1;
    levelTimeText->dropshadow = 1; 

    inputText = new textbox(renderer, "", 1700 * g_fontsize * 1.4, 0, 0, 0.9);

    inputText->boxX = 0.5;
    inputText->boxY = 0.3;
    inputText->boxWidth = 0.98;
    inputText->boxHeight = 0.25 - 0.02;
    inputText->worldspace = 0;
    inputText->show = 1;
    inputText->align = 2;
    inputText->dropshadow = 1; 



    playersUI = 1;

    hideInventoryUI();
    hideTalkingUI();
    hideScoreUI();
  }

}

void adventureUI::initFullUI() {



//    tastePicture = new ui(renderer, "resources/static/ui/taste.qoi", 0.2 + 0.01, 1-0.1, 0.05, 1, -15);
//    tastePicture->persistent = 1;
//    tastePicture->heightFromWidthFactor = 1;
//    tastePicture->show = 1;
//    tastePicture->framewidth = 410;
//    tastePicture->frameheight = 465;
//    tastePicture->layer0 = 1;
//    tastePicture->glideSpeed = 0.1;
//    tastePicture->widthGlideSpeed = 0.1;
//    tastePicture->priority = -10; //taste is behind everything
//    tastePicture->show = 1;

  adventureUIManager->tungShakeDurationMs = adventureUIManager->maxTungShakeDurationMs;
  adventureUIManager->tungShakeIntervalMs = adventureUIManager->maxTungShakeIntervalMs + rand() % adventureUIManager->tungShakeIntervalRandomMs;

//    hungerPicture = new ui(renderer, "resources/static/ui/hunger.qoi", 0.8, 0.6, 0.25, 1, -15);
//    hungerPicture->persistent = 1;
//    hungerPicture->heightFromWidthFactor = 1;
//    hungerPicture->show = 1;
//    hungerPicture->framewidth = 410;
//    hungerPicture->frameheight = 465;
//    hungerPicture->layer0 = 1;
//    hungerPicture->glideSpeed = 0.1;
//    hungerPicture->widthGlideSpeed = 0.1;
//    hungerPicture->priority = -10; //hunger is behind everything
//    hungerPicture->show = 0;
  
  adventureUIManager->stomachShakeDurationMs = adventureUIManager->maxstomachShakeDurationMs;
  adventureUIManager->stomachShakeIntervalMs = adventureUIManager->maxstomachShakeIntervalMs + rand() % adventureUIManager->stomachShakeIntervalRandomMs;

//  healthPicture = new ui(renderer, "resources/static/ui/health.qoi", -0.04, -0.09, 0.25, 1, -15);
//  healthPicture->persistent = 1;
//  healthPicture->heightFromWidthFactor = 1;
//  healthPicture->show = 1;
//  healthPicture->framewidth = 410;
//  healthPicture->frameheight = 465;
//  healthPicture->layer0 = 1;
//  healthPicture->glideSpeed = 0.1;
//  healthPicture->widthGlideSpeed = 0.1;
//  healthPicture->priority = -10; //health is behind everything

  emotion = new ui(renderer, "resources/static/ui/emoticons.qoi", 0, 0, 0.05, 0.05, -15);
  emotion->persistent = 1;
  emotion->heightFromWidthFactor = 1;
  emotion->show = 0;
  emotion->framewidth = 64;
  emotion->frameheight = 64;
  emotion->priority = 0;

  hotbar = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", g_hotbarX + g_backpackHorizontalOffset, 0.84, 0.1, 0.1, 1);
  hotbar->is9patch = true;
  hotbar->patchwidth = 213;
  hotbar->patchscale = 0.5;
  hotbar->persistent = true;
  hotbar->heightFromWidthFactor = 1;
  hotbar->priority = -8;

  hotbarFocus = new ui(renderer, "resources/static/ui/hotbar_focus.qoi", g_hotbarX + g_backpackHorizontalOffset + 0.005, 0.84 + 0.005, 0.1-0.01, 0.1-0.01, 1);
  hotbarFocus->persistent = true;
  hotbarFocus->heightFromWidthFactor = 1;
  hotbarFocus->priority = -7;
  hotbarFocus->dropshadow = 1;

  noIconTexture = loadTexture(renderer, "resources/engine/sp-no-texture.qoi");

  nextUsableIcon = new ui(renderer, "resources/engine/sp-no-texture.qoi", 0.45 + 0.1, 0.84, 0.1, 1, 1);
  nextUsableIcon->persistent = true;
  nextUsableIcon->heightFromWidthFactor = 1;
  SDL_DestroyTexture(nextUsableIcon->texture);
  nextUsableIcon->texture = noIconTexture;
  nextUsableIcon->shrinkPercent = 0.01; 
  nextUsableIcon->priority = -7;
  nextUsableIcon->show = 0;

  prevUsableIcon = new ui(renderer, "resources/engine/sp-no-texture.qoi", 0.45 - 0.1, 0.84, 0.1, 1, 1);
  prevUsableIcon->persistent = true;
  prevUsableIcon->heightFromWidthFactor = 1;
  SDL_DestroyTexture(prevUsableIcon->texture);
  prevUsableIcon->texture = noIconTexture;
  prevUsableIcon->shrinkPercent = 0.01; 
  prevUsableIcon->priority = -7;

  thisUsableIcon = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  thisUsableIcon->persistent = true;
  thisUsableIcon->heightFromWidthFactor = 1;
  SDL_DestroyTexture(thisUsableIcon->texture);
  thisUsableIcon->texture = noIconTexture;
  thisUsableIcon->shrinkPercent = 0.01; 
  thisUsableIcon->priority = -7;

  float shrinkPercent = 0.015;

  t1 = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  t1->persistent = true;
  t1->heightFromWidthFactor = 1;
  SDL_DestroyTexture(t1->texture);
  t1->texture = noIconTexture;
  t1->shrinkPercent = shrinkPercent; 
  t1->priority = -7;

  t2 = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  t2->persistent = true;
  t2->heightFromWidthFactor = 1;
  SDL_DestroyTexture(t2->texture);
  t2->texture = noIconTexture;
  t2->shrinkPercent = shrinkPercent; 
  t2->priority = -7;

  t3 = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  t3->persistent = true;
  t3->heightFromWidthFactor = 1;
  SDL_DestroyTexture(t3->texture);
  t3->texture = noIconTexture;
  t3->shrinkPercent = shrinkPercent; 
  t3->priority = -6;

  t4 = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  t4->persistent = true;
  t4->heightFromWidthFactor = 1;
  SDL_DestroyTexture(t4->texture);
  t4->texture = noIconTexture;
  t4->shrinkPercent = shrinkPercent; 
  t4->priority = -7;

  t5 = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.45, 0.84, 0.1, 1, 1);
  t5->persistent = true;
  t5->heightFromWidthFactor = 1;
  SDL_DestroyTexture(t5->texture);
  t5->texture = noIconTexture;
  t5->shrinkPercent = shrinkPercent; 
  t5->priority = -7;

  hotbarTransitionIcons.push_back(t1);
  hotbarTransitionIcons.push_back(t2);
  hotbarTransitionIcons.push_back(t3);
  hotbarTransitionIcons.push_back(t4);
  hotbarTransitionIcons.push_back(t5);
  
  prevUsableIcon->show = 0;
  thisUsableIcon->show = 0;
  nextUsableIcon->show = 0;

  cooldownIndicator = new ui(renderer, "resources/engine/cooldownIndicator.qoi", g_hotbarX + g_backpackHorizontalOffset, 0.83, 0.03, 1, 1);
  cooldownIndicator->priority = -6;
  cooldownIndicator->persistent = 1;
  cooldownIndicator->heightFromWidthFactor = 1;
  cooldownIndicator->xframes = 48;
  cooldownIndicator->framewidth = 64;
  cooldownIndicator->frameheight = 64;

  for(auto x : hotbarTransitionIcons) {
    x->opacity = 0;
    x->targetx = x->x;
    x->targety = x->y;
    x->glideSpeed = 0.3;
  }

  hotbarMutedXIcon = new ui(renderer, "resources/static/ui/red_x.qoi", g_hotbarX + g_backpackHorizontalOffset + 0.005, 0.84 + 0.005, 0.1-0.01, 0.1-0.01, 1);
  hotbarMutedXIcon->persistent = true;
  hotbarMutedXIcon->priority = -6;
  hotbarMutedXIcon->heightFromWidthFactor = 1;
  hotbarMutedXIcon->dropshadow = 1;


}

adventureUI::~adventureUI()
{
  if (this->playersUI)
  {
    Mix_FreeChunk(blip);
    Mix_FreeChunk(confirm_noise);
  }
  
  if(!light) {
    delete talkingBox;
    // delete talkingBoxTexture;
    delete talkingText;
    delete scoreText;

    delete inventoryA;
    delete inventoryB;
  }

}

void adventureUI::pushFancyText(entity * ftalker)
{
  inPauseMenu = 0;
  talkingText->show = 0;
  adventureUIManager->hideInventoryUI();
  talkingBox->show = 1;
  talker = ftalker;
  typing = 1;

  g_talker = ftalker;
  string arrangeText = scriptToUse->at(dialogue_index);
  //parse arrangeText for variables within $$, e.g. $$playername$$
  int position = arrangeText.find("$$");  
  if(position != string::npos) {
    int position2 = arrangeText.find("$$", position+1);
    if(position2 != string::npos) {
      //get the text between those two positions
      string variableName = arrangeText.substr(position + 2, position2 - position -2);

      //is there a savestring for that?
      string res = readSaveStringField(variableName);
      arrangeText.erase(arrangeText.begin() + position, arrangeText.begin() + position2 + 2);
      arrangeText.insert(position, res);

    }
  }
  g_fancybox->arrange(arrangeText);
  dialogProceedIndicator->show = 0;
}

void adventureUI::pushText(entity *ftalker)
{
  inPauseMenu = 0;
  adventureUIManager->hideInventoryUI();
  talker = ftalker;
  g_talker = ftalker;
  if (scriptToUse->at(dialogue_index).at(0) == '%' || scriptToUse->at(dialogue_index).at(0) == ')' || scriptToUse->at(dialogue_index).at(0) == '(')
  {
    pushedText = scriptToUse->at(dialogue_index).substr(1);
  }
  else if(scriptToUse->at(dialogue_index).at(0) == '-') {
    pushedText = scriptToUse->at(dialogue_index).substr(1);
    pushedText = pushedText + " " + g_saveToDelete + "?";
  } else {
    pushedText = scriptToUse->at(dialogue_index);
  }

  //parse pushedText for variables within $$, e.g. $$playername$$
  int position = pushedText.find("$$");  
  if(position != string::npos) {
    int position2 = pushedText.find("$$", position+1);
    if(position2 != string::npos) {
      //get the text between those two positions
      string variableName = pushedText.substr(position + 2, position2 - position -2);

      //is there a savestring for that?
      string res = readSaveStringField(variableName);
      pushedText.erase(pushedText.begin() + position, pushedText.begin() + position2 + 2);
      pushedText.insert(position, res);

    }
  }
  

  curText = "";
  typing = true;
  dialogProceedIndicator->show = 0;
  showTalkingUI();
}

void adventureUI::updateText()
{
  talkingText->updateText(curText, -1, 0.85, currentTextcolor, currentFontStr);
  g_fancybox->reveal();

  //used to be code here for unsleeping the player's ui

  if (askingQuestion)
  {
    string former = "   ";
    string latter = "   ";
    if (response_index > 0)
    {
      former = " < ";
    }
    if (response_index < responses.size() - 1)
    {
      latter = " > ";
    }

    string content = responses[response_index];
    if(g_saveOverwriteResponse == 1 || g_saveOverwriteResponse == 2) {
      if(response_index < 3) {
        content = g_saveNames[response_index];
      }
    }

    responseText->updateText(former + content + latter, -1, 0.9, currentTextcolor, currentFontStr);
    responseText->show = 1;
    response = responses[response_index];
    if(g_saveOverwriteResponse == 2) {
      if(response_index <3) {
        g_saveToDelete = g_saveNames[response_index];
      }
    }

  }
  else
  {
    responseText->updateText("", -1, 0.9, currentTextcolor, currentFontStr);
    responseText->show = 0;
  }

  if (pushedText != curText)
  {
    int index = curText.length();
    curText += pushedText.at(index);

    // Play a clank
    if (blip != NULL)
    {
      Mix_HaltChannel(6);
      Mix_VolumeChunk(blip, 20);
      //playSound(6, blip, 0);
    }
  }
  else
  {
    if(!g_fancybox->show) {
      typing = false;
    }
    if(!askingQuestion && this == adventureUIManager && executingScript && (curText != "") && !g_fancybox->show) {
      if(dialogProceedIndicator->show == 0) {
        dialogProceedIndicator->show = 1;
        dialogProceedIndicator->y = 0.9;
        adventureUIManager->c_dpiDesendMs = 0;
        adventureUIManager->c_dpiAsending = 0;
      }
    }
  }
}

void adventureUI::skipText() {
  g_fancybox->revealAll();
  if(pushedText != "") {
    curText = pushedText;
    Mix_HaltChannel(6);
    Mix_VolumeChunk(blip, 20);
  }
  //playSound(6, blip, 0);
}

//for resetting text color
void adventureUI::initDialogue() {
  currentTextcolor = textcolors[0].second;
  currentFontStr = fonts[0].second;
}

//scripting system 
//scripting interpreter
//script system
//script interpreter
//scripts
void adventureUI::continueDialogue()
{
  g_fancybox->show = 0;
  // has our entity died?
  if (g_forceEndDialogue && playersUI)
  {
    g_forceEndDialogue = 0;
    protag_is_talking = 2;
    adventureUIManager->hideTalkingUI();
    return;
  }

  if (sleepingMS > 1)
  {
    sleepingMS -= elapsed;
    if( playersUI) {
      protag_is_talking = !mobilize;
    }
    return;
  }
  else
  {
    if (sleepflag)
    {
      mobilize = 0;
      //this->showTalkingUI();
      sleepflag = 0;
    }
  }

  if (playersUI)
  {
    protag_is_talking = 1;
  }
  executingScript = 1;

  if(useOwnScriptInsteadOfTalkersScript) {
    scriptToUse = &ownScript;
  } else {
    scriptToUse = &talker->sayings;
  }


  if (scriptToUse->size() <= dialogue_index + 2 || scriptToUse->at(dialogue_index + 1) == "#")
  {
    if (playersUI)
    {
      protag_is_talking = 2;
    }
    executingScript = 0;

    mobilize = 0;
    if(this == adventureUIManager) {
      adventureUIManager->hideTalkingUI();
      g_fancybox->show = 0;
      g_fancybox->clear();
    }



    if(!useOwnScriptInsteadOfTalkersScript) {
      dialogue_index = 0;
      talker->animate = 0;
      if (talker->turnToFacePlayer)
      {
        if (talker->defaultAnimation == 0 || talker->defaultAnimation == 4)
        {
          talker->flip = SDL_FLIP_NONE;
        }
        talker->animation = talker->defaultAnimation;
      }
    }
    return;
  }


  // question
  if (scriptToUse->at(dialogue_index + 1).at(0) == '%')
  {
    g_saveOverwriteResponse = 0;
    // make a question
    dialogue_index++;
    pushText(talker);
    askingQuestion = true;
    // put responses in responses vector
    int j = 1;
    string res = scriptToUse->at(dialogue_index + j).substr(1);
    responses.clear();
    while (res.find(':') != std::string::npos)
    {
      responses.push_back(res.substr(0, res.find(':')));
      j++;
      res = scriptToUse->at(dialogue_index + j).substr(1);
    }
    return;
  }
  else
  {
    askingQuestion = false;
  }

  // loadsavenames
  if (scriptToUse->at(dialogue_index + 1).substr(0,14) == "/menuloadnames")
  {
    vector<string> savenames = {"user/saves/a.save",
                                "user/saves/b.save",
                                "user/saves/c.save"};

    ifstream data;
    string line;
    int i = 0;
    string name = "New File";
    g_saveNames.clear();
    while(i < 3) {
      data.open(savenames[i]);
      while(!data.eof()) {
        getline(data, line);
        if(line.find("playername") != string::npos) {
          vector<string> x = splitString(line, ' ');
          if(x[1] == "Blank") { x[1] = "New File";}
          g_saveNames.push_back(x[1]);
          break;
        }
      }
      data.close();
      i++;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // menu question
  if (scriptToUse->at(dialogue_index + 1).at(0) == ')')
  {
    g_saveOverwriteResponse = 1;
    // make a question
    dialogue_index++;
    pushText(talker);
    askingQuestion = true;
    // put responses in responses vector
    int j = 1;
    string res = scriptToUse->at(dialogue_index + j).substr(1);
    responses.clear();
    while (res.find(':') != std::string::npos)
    {
      responses.push_back(res.substr(0, res.find(':')));
      j++;
      res = scriptToUse->at(dialogue_index + j).substr(1);
    }
    
    return;
  }

  // menu erase prompt
  if (scriptToUse->at(dialogue_index + 1).at(0) == '(')
  {
    g_saveOverwriteResponse = 2;
    // make a question
    dialogue_index++;
    pushText(talker);
    askingQuestion = true;
    // put responses in responses vector
    int j = 1;
    string res = scriptToUse->at(dialogue_index + j).substr(1);
    responses.clear();
    while (res.find(':') != std::string::npos)
    {
      responses.push_back(res.substr(0, res.find(':')));
      j++;
      res = scriptToUse->at(dialogue_index + j).substr(1);
    }
    
    return;
  }

  // menu erase confirm prompt
  if (scriptToUse->at(dialogue_index + 1).at(0) == '-')
  {
    g_saveOverwriteResponse = 3;
    // make a question
    dialogue_index++;
    pushText(talker);
    askingQuestion = true;
    // put responses in responses vector
    int j = 1;
    string res = scriptToUse->at(dialogue_index + j).substr(1);
    responses.clear();
    while (res.find(':') != std::string::npos)
    {
      responses.push_back(res.substr(0, res.find(':')));
      j++;
      res = scriptToUse->at(dialogue_index + j).substr(1);
    }
    
    return;
  }

  // item prompt
  //$
  //Would confict with string variables, so I added a second check
  if (scriptToUse->at(dialogue_index + 1).at(0) == '$' && scriptToUse->at(dialogue_index + 1).at(1) != '$')
  {
    int j = 1;
    // parse which block of memory we are interested in
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 1);

    int numberOfItem = 0;
    // indexItem* itemref = 0;
    for (auto x : mainProtag->inventory)
    {
      if (x.first->name == s)
      {
        numberOfItem = x.second;
        // itemref = x.first;
      }
    }

    string res = scriptToUse->at(dialogue_index + 1 + j);
    while (res.find('*') != std::string::npos)
    {

      // parse option
      //  *15 29 -> if data is 15, go to line 29
      string s = scriptToUse->at(dialogue_index + 1 + j);
      s.erase(0, 1);
      int condition = stoi(s.substr(0, s.find(':')));
      s.erase(0, s.find(':') + 1);
      int jump = stoi(s);
      if (numberOfItem >= condition)
      {
        dialogue_index = jump - 3;
        this->continueDialogue();
        return;
      }
      j++;
      res = scriptToUse->at(dialogue_index + 1 + j);
    }
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // give item
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/give")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    vector<string> x = splitString(s, ' ');

    indexItem *a = new indexItem(x[0], 0);

    // if you just type the name of the item, it's assumed that we are giving one
    if ((int)x.size() < 2)
    {
      x.push_back("1");
    }

    mainProtag->getItem(a, stoi(x[1]));

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/take")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    vector<string> x = splitString(s, ' ');

    indexItem *a = new indexItem(x[0], 0);

    // if you just type the name of the item, it's assumed that we are giving one
    if ((int)x.size() < 2)
    {
      x.push_back("1");
    }

    mainProtag->loseItem(a, stoi(x[1]));

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // check if collectible familiars contains these entities
  // /familiarcheck circus/ticket-a circus/ticket-b circus/ticket-c :success
  // `You still need to collect some tickets to enter the circus
  // `Come back when you have three tickets
  // #
  // <success>
  // `Okay, I will let you enter the circus
  // #
  if (scriptToUse->at(dialogue_index + 1).substr(0, 14) == "/familiarcheck")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    vector<string> names;

    int fail = 0;
    for(int i = 1; i < x.size()-1; i++) {
      int good = 0;
      for(auto y : g_familiars) {
        if(y->name == x[i]) { good = 1;}
      }
      if(!good) { fail = 1; break;}
    }

    if(fail) {
      dialogue_index++;
      this->continueDialogue();
      return;
    } else {
      //do the jump
      string jumpst = x[x.size()-1];
      jumpst.erase(jumpst.begin());
      int jump = stoi(jumpst);
      dialogue_index = jump - 3;
      this->continueDialogue();
      return;

    }
  }

  // suck the selected familiars towards a given entity
  // removes them from the player's familiars
  // /familiarsuck common/chest circus/ticket-a circus/ticket-b circus/ticket-c 
  if (scriptToUse->at(dialogue_index + 1).substr(0, 13) == "/familiarsuck")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    vector<string> names;

    for(int i = 2; i < x.size(); i++) {
      for(auto y : g_familiars) {
        if(y->name == x[i]) { 
          g_ex_familiars.push_back(y);
          g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), y), g_familiars.end());
        }
      }
    }

    g_exFamiliarParent = searchEntities(x[1]);
    g_exFamiliarTimer = 10000;



    dialogue_index++;
    this->continueDialogue();
    return;
  }


  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/grossup") {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    string file = "";
    if(x.size() > 1) {
      file = x.at(1);
    }

    file = "resources/static/sprites/" + file +  ".qoi";

    if(g_grossupLoaded) {
      SDL_DestroyTexture(g_grossup);
    }

    g_grossup = loadTexture(renderer, file);
    g_grossupLoaded = 1;
    g_grossupShowMs = g_maxGrossupShowMs;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // check number of living entities by name
  //  /count
  if (scriptToUse->at(dialogue_index + 1).substr(0, 6) == "/count")
  {
    int j = 1;
    // parse which block of memory we are interested in
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 7);
    string name = s;

    int numberOfEntity = 0;

    for (auto x : g_entities)
    {
      if (x->name == name && x->tangible)
      {
        numberOfEntity++;
      }
    }

    string res = scriptToUse->at(dialogue_index + 1 + j);
    while (res.find('*') != std::string::npos)
    {
      // parse option
      //  *15 29 -> if data is 15, go to line 29
      string s = scriptToUse->at(dialogue_index + 1 + j);
      s.erase(0, 1);
      int condition = stoi(s.substr(0, s.find(':')));
      s.erase(0, s.find(':') + 1);
      int jump = stoi(s);
      if (numberOfEntity >= condition)
      {
        dialogue_index = jump - 3;
        this->continueDialogue();
        return;
      }
      j++;
      res = scriptToUse->at(dialogue_index + 1 + j);
    }
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // (this explanation is backwards, but the example is correct.
  // see fleshpit_update.txt)
  // Switch-statement by distance between two entities
  // coditions are listed from least to greatest
  // the first condition taken will be the one
  // which is <= the measured distance
  //  /distance protag zombie
  //  *500:close
  //  *1000:far
  //  #
  //  <close>
  //  You are close to me.
  //  #
  //  <far>
  //  You are far away.
  //  #
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/distance")
  {
    int j = 1;
    string s = scriptToUse->at(dialogue_index + 1);

    auto x = splitString(s, ' ');

    //I(x.size());

    string fStr = x[1];
    string sStr = x[2];
  
    entity* firstEnt = searchEntities(fStr);
    entity* secondEnt = searchEntities(sStr);

    if(firstEnt != nullptr && secondEnt != nullptr) {

      int distance = XYWorldDistance(firstEnt->getOriginX(), firstEnt->getOriginY(), secondEnt->getOriginX(), secondEnt->getOriginY());

      string res = scriptToUse->at(dialogue_index + 1 + j);
      while (res.find('*') != std::string::npos)
      {
        // parse option
        //  *15:29 -> if distance is less than 15, go to line 29
        string s = scriptToUse->at(dialogue_index + 1 + j);
I("s");
        //I(s);
        s.erase(0, 1);
        int condition = stoi(s.substr(0, s.find(':')));
        //I("condition");
        //I(condition);
        s.erase(0, s.find(':') + 1);
        int jump = stoi(s);
        //I("jump");
        //I(jump);
        //I("distance");
        //I(distance);
        if (distance < condition)
        {
          dialogue_index = jump - 3;
          this->continueDialogue();
          return;
        }
        j++;
        res = scriptToUse->at(dialogue_index + 1 + j);
      }

    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //write debug message to console
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/print ")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    if(x.size() >= 2) {
      string printMe = "Print from Script: " + s.substr(7);
      M(printMe);
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // select command
  //change talker (useful for writing selfdata to entities from non-dialogue scripts)
  // do
  // /select common/train
  // 5->[4]
  // to set the forth selfdata of common/train to 4
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/select")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    if(x.size() < 2) {
      E("Not enough args for /select call.");
    } else {

      string entName = x[1];
      entity *hopeful = 0;
      hopeful = searchEntities(entName, talker);
      if (hopeful != nullptr)
      {
        selected = hopeful;
      }
      else
      {
        E("Couldn't find entity for /select call.");
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // write selfdata 5->[4]
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("[[:digit:]]+\\-\\>\\[[[:digit:]]+\\]")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}
    string s = scriptToUse->at(dialogue_index + 1);
    int value = stoi(s.substr(0, s.find('-')));
    s.erase(0, s.find('-') + 1);
    string blockstr = s.substr(s.find('['));
    blockstr.pop_back();
    blockstr.erase(0, 1);
    int block = stoi(blockstr);
    selected->data[block] = value;
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //write random number to selfdata
  // 0-1000->[4]
  if(regex_match (scriptToUse->at(dialogue_index + 1), regex("[[:digit:]]+\\-+[[:digit:]]+\\-\\>\\[[[:digit:]]+\\]"))) {
    string s = scriptToUse->at(dialogue_index + 1);
    int firstvalue = stoi( s.substr(0, s.find('-')) ); s.erase(0, s.find('-') + 1);
    int secondvalue = stoi( s.substr(0, s.find('-')) ); s.erase(0, s.find('-') + 1);
    
    string blockstr = s.substr(s.find('[')); 
    blockstr.pop_back(); blockstr.erase(0, 1);
    int block = stoi (blockstr);

    selected->data[block] = rand() % (secondvalue - firstvalue + 1) + firstvalue;
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //print selfdata to console for debuging
  if (scriptToUse->at(dialogue_index + 1).substr(0, 10) == "/printdata")
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}
    
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    if(x.size() >= 2) {
      int block = stoi(s.substr(11));
      int value = selected->data[block];
      D(block);
      D(value);
      string valueSTR = to_string(value);
      string printMe = "Print selfdata from Script: " + selected->name + " [" + to_string(block) + "]: " + valueSTR;
      M(printMe);
    }
    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // add to selfdata
  // [4]+1
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\[[[:digit:]]+\\]\\+[[:digit:]]+")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}

    string s = scriptToUse->at(dialogue_index + 1);
    
    vector<string> x = splitString(s, '+');
    x[0] = x[0].substr(1, x[0].size()-2);

    int block = stoi(x[0]);
    int value = selected->data[block];
    value += stoi(x[1]);
    selected->data[block] = value;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // subtract from selfdata
  // [4]-1
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\[[[:digit:]]+\\]\\-[[:digit:]]+")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}

    string s = scriptToUse->at(dialogue_index + 1);
    
    vector<string> x = splitString(s, '-');
    x[0] = x[0].substr(1, x[0].size()-2);

    int block = stoi(x[0]);
    int value = selected->data[block];
    value -= stoi(x[1]);
    selected->data[block] = value;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // multiply selfdata
  // [4]*3
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\[[[:digit:]]+\\]\\*[[:digit:]]+")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}

    string s = scriptToUse->at(dialogue_index + 1);
    
    vector<string> x = splitString(s, '*');
    x[0] = x[0].substr(1, x[0].size()-2);

    int block = stoi(x[0]);
    int value = selected->data[block];
    value *= stoi(x[1]);
    selected->data[block] = value;

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // divide selfdata
  // [4]/3
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\[[[:digit:]]+\\]\\/[[:digit:]]+")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}

    string s = scriptToUse->at(dialogue_index + 1);
    
    vector<string> x = splitString(s, '/');
    x[0] = x[0].substr(1, x[0].size()-2);

    int block = stoi(x[0]);
    int value = selected->data[block];
    value /= stoi(x[1]);
    selected->data[block] = value;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // read selfdata [5]
  //
  // [5]
  // *0:waszero
  // *1:wasone
  // #
  // <waszero>
  // It was zero.
  // #
  // <wasone>
  // It was one.
  // #
  //
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\[[[:digit:]]+\\]")))
  {
    if(selected == nullptr) {E("Accessed selfdata without calling /select first");}
    int j = 1;
    // parse which block of memory we are interested in
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 1);
    string blockstr = s.substr(0, s.find(']'));
    int block = stoi(blockstr);
    string res = scriptToUse->at(dialogue_index + 1 + j);
    while (res.find('*') != std::string::npos)
    {

      // parse option
      //  *15 29 -> if data is 15, go to line 29
      string s = scriptToUse->at(dialogue_index + 1 + j);
      s.erase(0, 1);
      int condition = stoi(s.substr(0, s.find(':')));
      s.erase(0, s.find(':') + 1);
      int jump = stoi(s);
      if (selected->data[block] <= condition)
      {
        dialogue_index = jump - 3;
        this->continueDialogue();
        return;
      }
      j++;
      res = scriptToUse->at(dialogue_index + 1 + j);
    }
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // comment
  if (scriptToUse->at(dialogue_index + 1).substr(0, 2) == "//")
  {
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // option/jump
  if (scriptToUse->at(dialogue_index + 1).at(0) == '*')
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 1);
    string condition = s.substr(0, s.find(':'));
    s.erase(0, s.find(':') + 1);
    int jump = stoi(s);
    if (response == condition)
    {
      response = "";
      response_index = 0;
      //playSound(-1, confirm_noise, 0);
      dialogue_index = jump - 3;
      this->continueDialogue();
    }
    else
    {
      dialogue_index++;
      this->continueDialogue();
    }

    return;
  }

  // change textbox text color
  if (scriptToUse->at(dialogue_index + 1).substr(0, 10) == "/textcolor")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    string desiredColor = x[1];
    currentTextcolor = textcolors[0].second;
    bool good = 0;
    for(auto y : textcolors) {
      if(y.first == desiredColor) {
        currentTextcolor = y.second;
        good = 1;
        break;
      }
    }
    if(!good) {
      E("Failed to set textcolor");
      M(desiredColor);
      quit = 1;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //change textbox font
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/font")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    string desiredFontStr = x[1];
    currentFontStr = fonts[0].second;
    for(auto y : fonts) {
      if(y.first == desiredFontStr) {
        currentFontStr = y.second;
        break;
      }
    }

    M("set font");

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change mapdir
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/mapdir")
  {
    //M("setting mapdir");
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 8);
    g_mapdir = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change map
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/map ")
  {
    M("changing map");
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 5);
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    string dest_waypoint = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    // if the script just has "@" and that's all, send the player to the last saved map
    if (name.length() == 0)
    {
      name = g_mapOfLastSave;
      cout << g_waypointOfLastSave << endl;
      dest_waypoint = g_waypointOfLastSave;
    }

    // close dialogue
    adventureUIManager->hideTalkingUI();
    dialogue_index = 0;
    talker->animate = 0;

    clear_map(g_camera);
    g_map = name;
    const string toMap = "resources/maps/" + g_mapdir + "/" + g_map + ".map";
    load_map(renderer, toMap, dest_waypoint);

    // //clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
    if (canSwitchOffDevMode)
    {
      init_map_writing(renderer);
    }
    protag_is_talking = 0;
    protag_can_move = 1;
    // clear talker so that g_forceEndDialogue will not be set to 1
    // g_talker = nullptr;
    return;
  }

  // spawn entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 6) == "/spawn")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    // erase '&'
    M("spawned entity");
    s.erase(0, 7);
    int xpos, ypos;
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    xpos = stoi(s.substr(0, s.find(' ')));
    s.erase(0, s.find(' ') + 1);
    ypos = stoi(s.substr(0, s.find(' ')));
    s.erase(0, s.find(' ') + 1);
    entity *e = new entity(renderer, name.c_str());
    e->x = xpos;
    e->y = ypos;
    e->shadow->x = e->x + e->shadow->xoffset;
    e->shadow->y = e->y + e->shadow->yoffset;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // destroy entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/destroy")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);

    entity* hopeful = searchEntities(s, talker);
    if(hopeful != nullptr) {
      hopeful->usesContactScript = 0;
      if(hopeful->asset_sharer) {
        hopeful->usingTimeToLive = 1;
        hopeful->timeToLiveMs = -1;
      } else {
        hopeful->tangible = 0;
      }
      smokeEffect->happen(hopeful->getOriginX(), hopeful->getOriginY(), hopeful->z, 0);
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // fade out entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/fadeout")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);

    entity* hopeful = searchEntities(s, talker);
    if(hopeful != nullptr) {
      hopeful->opacity_delta = -1;
    }


    dialogue_index++;
    this->continueDialogue();
    return;
  }



  // set entity's ttl in ms
  // /setttl splatter 500
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/setttl")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    string name = x[1];
    string timestr = x[2];
    int time = stoi(timestr);

    entity* hopful = searchEntities(name);
    if(hopful != nullptr) {
      hopful->usingTimeToLive = 1;
      hopful->timeToLiveMs = time;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // move entity
  // "move"
  if (scriptToUse->at(dialogue_index + 1).substr(0, 6) == "/move ")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    vector<string> x = splitString(s, ' ');
    string name = x[0];
    int p0 = stoi(x[1]);
    int p1 = stoi(x[2]);
    entity *hopeful = searchEntities(name);

    // if this entity called the function for itself, set hopeful to it
    if (name == this->talker->name)
    {
      hopeful = this->talker;
    }

    if (hopeful != nullptr)
    {

      hopeful->agrod = 0;
      hopeful->Destination = getNodeByPosition(p0, p1);
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // teleport entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/teleport")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 10);
    vector<string> x = splitString(s, ' ');
    string name = x[0];
    int p0 = stoi(x[1]);
    int p1 = stoi(x[2]);
    entity *hopeful = searchEntities(name);
    if (hopeful != nullptr)
    {

      hopeful->x = p0;
      hopeful->y = p1;
      hopeful->xvel = 0;
      hopeful->yvel = 0;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //set pellet goal
  //meaning, set a script which runs when the player collects X pellets from the level
  // /pelletgoal 60 finishLevel 
  // finishLevel scripts need to be local, setting the level to complete and enabling the train
  if (scriptToUse->at(dialogue_index + 1).substr(0, 11) == "/pelletgoal")
  {
    auto x = splitString(scriptToUse->at(dialogue_index + 1), ' ');
    if(x.size() > 2) {
      M("Adding pelletgoal for " + x[1] + " pellets");
      std::pair<int,string> a(stoi(x[1]),x[2]);
      g_pelletGoalScripts.push_back(a);
      g_pelletsNeededToProgress = stoi(x[1]);
    } else {
      E("Not enough arguments for /pelletgoal");
    }
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //hide pellets
  if (scriptToUse->at(dialogue_index + 1).substr(0, 12) == "/hidepellets")
  {
    g_showPellets = 0;

    for(auto p : g_pellets) {
      //p->tangible = 0;
      p->shrinking = 1;
      p->originalWidth = p->width;
      p->originalHeight = p->height;
      p->width = 0;
      p->height = 0;
      p->shadow->visible = 0;
    }
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //show pellets
  if (scriptToUse->at(dialogue_index + 1).substr(0, 12) == "/showpellets")
  {
    g_showPellets = 1;

    for(auto p : g_pellets) {
      p->tangible = 1;
      p->curwidth = 0;
      p->curheight = 0;
      p->width = p->originalWidth;
      p->height = p->originalHeight;
      p->shadow->visible = 1;
    }
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //use fademode for objective crosshair, so it only shows if the player is standing still
  if (scriptToUse->at(dialogue_index + 1).substr(0, 16) == "/objectivefadeon")
  {
    g_objectiveFadeModeOn = 1;
    g_objectiveFadeWaitMs = g_objectiveFadeMaxWaitMs;
    g_objectiveOpacity = 0;
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  if (scriptToUse->at(dialogue_index + 1).substr(0, 17) == "/objectivefadeoff")
  {
    g_objectiveFadeModeOn = 0;
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }


  //load/spawn particle effect
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/effect")
  {
    
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    //       /effect smoke zombie forwards_offset rightwards_offset (duration?)
    if(x.size() >= 5) 
    {
      string effects_name = x[1];
      string ents_name = x[2];
      string forwards_offset = x[3];
      string rightwards_offset = x[4];

      entity *hopeful = searchEntities(effects_name);
      effectIndex* hopeEffect;
      if (hopeful != nullptr)
      {
        for(int i = 0; i < g_effectIndexes.size(); i++) {
          if(g_effectIndexes[i]->name == effects_name) {
            hopeEffect = g_effectIndexes[i];
            
          }

        }

        if(hopeEffect == nullptr) {
          hopeEffect = new effectIndex( effects_name, renderer );
        }
        
        hopeEffect->happen(hopeful->getOriginX(), hopeful->getOriginY(), hopeful->z, 0);

      }
      

    }

    

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //load/spawn spawner from eft file
  //eft file also contains some data for a spawner (name of parent, offsets, interval)
  //don't need to use it (e.g., in a cutscene someone disapears and you make a puff of smoke)
  //but it can be used to create lasting effects (smoke continuously billowing from a brazier)
  //putting zero for the duration will make it everlasting
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/emitter")
  {
    
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    //       /emitter smoke zombie xoffset yoffset zoffset duration
    if(x.size() >= 6) 
    {
      string effects_name = x[1];
      string ents_name = x[2];
      string sxoffset = x[3];
      string syoffset = x[4];
      string szoffset = x[5];
      int fxoffset = stoi(sxoffset);
      int fyoffset = stoi(syoffset);
      int fzoffset = stoi(szoffset);
      string stimeToLiveMs = x[6];
      int ftimeToLiveMs = stoi(stimeToLiveMs);

      entity *hopeful = searchEntities(ents_name);
      effectIndex* hopeEffect = nullptr;
      if (hopeful != nullptr)
      {
        for(int i = 0; i < g_effectIndexes.size(); i++) {
          if(g_effectIndexes[i]->name == effects_name) {
            hopeEffect = g_effectIndexes[i];
            M("Found effect");
            
          }

        }

        if(hopeEffect == nullptr) {
          hopeEffect = new effectIndex( effects_name, renderer );
        }
        
        //hopeEffect->happen(hopeful->getOriginX(), hopeful->getOriginY(), hopeful->z );
        emitter* e = new emitter();
        e->type = hopeEffect;
        e->parent = hopeful;
        e->xoffset = fxoffset;
        e->yoffset = fyoffset;
        e->zoffset = fzoffset;
        e->timeToLiveMs = ftimeToLiveMs;
        e->maxIntervalMs = hopeEffect->spawnerIntervalMs;

      } else {
        E("/emitter error - ent not found");
      }

    } else {
      E("/emitter error - not enough args");
    }

    

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //select talker
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/tselect")
  {
    selected = talker;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // teleport entity to another entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 12) == "/entteleport")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 13);
    vector<string> x = splitString(s, ' ');
    string teleportMeSTR = x[0];
    string teleportToMeSTR = x[1];
    entity *teleportMe = searchEntities(teleportMeSTR);
    entity *teleportToMe = searchEntities(teleportToMeSTR);
    if (teleportMe != nullptr && teleportToMe != nullptr)
    {
      teleportMe->setOriginX(teleportToMe->getOriginX());
      teleportMe->setOriginY(teleportToMe->getOriginY());
      teleportMe->xvel = 0;
      teleportMe->yvel = 0;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // spawn entity at an entity with ttl (0 for no ttl)
  // /entspawn spawnMe spawnAtMe ttlMs setDirection forwardsOffset
  // /entspawn puddle zombie 5000 1
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/entspawn")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 10);
    vector<string> x = splitString(s, ' ');
    string teleportMeSTR = x[0];
    string teleportToMeSTR = x[1];
    string ttlSTR = "0"; ttlSTR = x[2];
    string setDirectionSTR = "0"; setDirectionSTR = x[3];
    entity *teleportMe = new entity(renderer, teleportMeSTR.c_str());
    teleportMe->dontSave = 1;
    entity *teleportToMe = searchEntities(teleportToMeSTR, talker);
    lastReferencedEntity = teleportMe;
    int ttl = stoi(ttlSTR);
    int setDirection = stoi(setDirectionSTR);
    if (teleportMe != nullptr && teleportToMe != nullptr)
    {
      teleportMe->setOriginX(teleportToMe->getOriginX());
      teleportMe->setOriginY(teleportToMe->getOriginY());
      teleportMe->xvel = 0;
      teleportMe->yvel = 0;
      teleportMe->shadow->x = teleportMe->x + teleportMe->shadow->xoffset;
      teleportMe->shadow->y = teleportMe->y + teleportMe->shadow->yoffset;
      if(ttl > 0) {
        teleportMe->usingTimeToLive = 1;
        teleportMe->timeToLiveMs = ttl;
      }

      if(setDirection == 1) {
        teleportMe->animation = teleportToMe->animation;
        teleportMe->flip = teleportToMe->flip;

      }
      teleportMe->steeringAngle = teleportToMe->steeringAngle;
      teleportMe->targetSteeringAngle = teleportToMe->steeringAngle;
      
      selected = teleportMe;
    }
    

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // copy an entity from the spawnlist at the talking entity with ttl (0 for no ttl)
  // always at talker
  // /tentcopy indexInSpawnlist ttlMs setDirection
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/tentcopy")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 10);
    vector<string> x = splitString(s, ' ');
    string teleportMeIndex = x[0];

    string ttlSTR = "0"; ttlSTR = x[1];
    string setDirectionSTR = "0"; setDirectionSTR = x[2];

    entity *teleportMe = new entity(renderer, talker->spawnlist[stoi(teleportMeIndex)]);
    teleportMe->setOriginX(talker->getOriginX());
    teleportMe->setOriginY(talker->getOriginY());

    teleportMe->dontSave = 1;
    entity *teleportToMe = talker;
    lastReferencedEntity = teleportMe;
    int ttl = stoi(ttlSTR);
    int setDirection = stoi(setDirectionSTR);
    if (teleportMe != nullptr && teleportToMe != nullptr)
    {
      teleportMe->setOriginX(teleportToMe->getOriginX());
      teleportMe->setOriginY(teleportToMe->getOriginY());
      teleportMe->xvel = 0;
      teleportMe->yvel = 0;
      teleportMe->shadow->x = teleportMe->x + teleportMe->shadow->xoffset;
      teleportMe->shadow->y = teleportMe->y + teleportMe->shadow->yoffset;
      if(ttl > 0) {
        teleportMe->usingTimeToLive = 1;
        teleportMe->timeToLiveMs = ttl;
      }

      if(setDirection == 1) {
        teleportMe->animation = teleportToMe->animation;
        teleportMe->flip = teleportToMe->flip;

      }
      teleportMe->steeringAngle = teleportToMe->steeringAngle;
      teleportMe->targetSteeringAngle = teleportToMe->steeringAngle;
      
      selected = teleportMe;
    }
    

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //apply statuseffects
  // /inflict [entity] [status] [duration, MS] [factor]
  // /inflict target poisoned 5000 1
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/inflict")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);
    vector<string> x = splitString(s, ' ');
    string entSTR = x[0];
    string factorSTR = x[3];
    string durationSTR = x[2];
    string statusSTR = x[1];

    float durationFLOAT = 0; durationFLOAT = stof(durationSTR);
    float factorFLOAT = 0; factorFLOAT = stof(factorSTR);

    entity *inflictMe = searchEntities(entSTR, talker);
    if(inflictMe != nullptr)
    {
      if(statusSTR == "slown" || statusSTR == "slow")
      {
        inflictMe->hisStatusComponent.slown.addStatus(durationFLOAT, factorFLOAT);
      }
      else if(statusSTR == "stunned" || statusSTR == "petrify")
      {
        inflictMe->hisStatusComponent.stunned.addStatus(durationFLOAT, factorFLOAT);
      }
      else if (statusSTR == "poisoned" || statusSTR == "")
      {
        inflictMe->hisStatusComponent.marked.addStatus(durationFLOAT, factorFLOAT);
      }
      else if (statusSTR.substr(0,4) == "heal" || statusSTR == "mend")
      {
        inflictMe->hisStatusComponent.healen.addStatus(durationFLOAT, factorFLOAT);
      }
      else if (statusSTR == "enraged" || statusSTR == "angst")
      {
        inflictMe->hisStatusComponent.enraged.addStatus(durationFLOAT, factorFLOAT);
      }
      else if(statusSTR == "marked" || statusSTR == "deathly")
      {
        inflictMe->hisStatusComponent.marked.addStatus(durationFLOAT, factorFLOAT);
      }
      else if(statusSTR == "disabled" || statusSTR == "kinderly")
      {
        inflictMe->hisStatusComponent.disabled.addStatus(durationFLOAT, factorFLOAT);
      }
      else if(statusSTR == "buffed" || statusSTR == "meatcake")
      {
        inflictMe->hisStatusComponent.buffed.addStatus(durationFLOAT, factorFLOAT);
      }
    }


    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // change objective
  // /setobjective heart
  if (scriptToUse->at(dialogue_index + 1).substr(0, 14) == "/setobjective ")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    string name = x[1];

    entity *hopeful = searchEntities(name);
    if (hopeful != nullptr)
    {
      g_objective = hopeful;
      adventureUIManager->crosshair->show = 1;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //set crosshair frame
  if (scriptToUse->at(dialogue_index + 1).substr(0, 18) == "/setobjectivetype ")
  {
    M("Set objective type");
    g_usingPelletsAsObjective = 0;
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    int frame = stoi(x[1]);
    
    if(frame < adventureUIManager->crosshair->xframes) {
      adventureUIManager->crosshair->frame = frame;
    } else {
      E("Tried to set the objective type too highly.");

    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //set pellet objective
  //until the objective is set again, when the player collects a pellet, 
  //the objective will be set to a nearby pellet
  if (scriptToUse->at(dialogue_index + 1).substr(0, 16) == "/pelletobjective")
  {
    g_usingPelletsAsObjective = 1;
    g_objective = nullptr;

    int frame = 1;
    
    if(frame < adventureUIManager->crosshair->xframes) {
      adventureUIManager->crosshair->frame = frame;
    } else {
      E("No frame for /pelletobjective call.");
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // clear objective
  if (scriptToUse->at(dialogue_index + 1).substr(0, 15) == "/clearobjective")
  {

    g_objective = 0;
    adventureUIManager->crosshair->show = 0;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change cameratarget
  //  /lookat ward 0 0
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/lookat ")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 8);
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    string transtr = "0";
    transtr = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    float transitionspeed = stof(transtr);

    entity *hopeful = searchEntities(name);
    if (hopeful != nullptr)
    {

      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
          g_fogcookies[i][j] = 0;
          g_sc[i][j] = 0;
          g_fc[i][j] = 0;
        }
      }

      g_focus = hopeful;

      g_force_cookies_update = 1;

      int px = -(int)g_focus->getOriginX() % 64;
      for (size_t i = 0; i < g_fogslates.size(); i++)
      {
        g_fogslates[i]->x = (int)g_focus->getOriginX() + px - 658;                                         // 655
        g_fogslates[i]->y = (int)g_focus->getOriginY() - ((int)g_focus->getOriginY() % 55) + 55 * i - 453; // 449
      }

      if (transitionspeed != 0)
      {
        // This check means that if the camera is already moving, don't reset
        // it's velocity, because that would be jarring
        if (g_camera.lag == 0)
        {
          g_camera.lag = transitionspeed;
          g_camera.lagaccel = transitionspeed;
        }
        else
        {
          g_camera.lagaccel = transitionspeed;
        }
      }
      else
      {
        g_camera.lag = 0;
        g_camera.lagaccel = g_camera.DEFAULTLAGACCEL;
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change cameratarget
  //  /lookatall ward 0 0
  if (scriptToUse->at(dialogue_index + 1).substr(0, 10) == "/lookatall")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 11);
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    string transtr = "0";
    transtr = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    float transitionspeed = stof(transtr);

    vector<entity *> hopefuls = gatherEntities(name);
    if (hopefuls.size() != 0)
    {
      g_focus = hopefuls[countEntities];
      for (long long unsigned i = 0; i < g_fogcookies.size(); i++)
      {
        for (long long unsigned j = 0; j < g_fogcookies[0].size(); j++)
        {
          g_fogcookies[i][j] = 0;
          g_sc[i][j] = 0;
          g_fc[i][j] = 0;
        }
      }

      g_force_cookies_update = 1;
      int px = -(int)g_focus->getOriginX() % 64;
      for (size_t i = 0; i < g_fogslates.size(); i++)
      {
        g_fogslates[i]->x = (int)g_focus->getOriginX() + px - 658;                                         // 655
        g_fogslates[i]->y = (int)g_focus->getOriginY() - ((int)g_focus->getOriginY() % 55) + 55 * i - 453; // 449
      }

      if (transitionspeed != 0)
      {
        // This check means that if the camera is already moving, don't reset
        // it's velocity, because that would be jarring
        if (g_camera.lag == 0)
        {
          g_camera.lag = transitionspeed;
          g_camera.lagaccel = transitionspeed;
        }
        else
        {
          g_camera.lagaccel = transitionspeed;
        }
      }
      else
      {
        g_camera.lag = 0;
        g_camera.lagaccel = g_camera.DEFAULTLAGACCEL;
      }
    }
    countEntities++;
    if (countEntities == (int)hopefuls.size())
    {
      // continue
      countEntities = 0;
      dialogue_index++;
      this->continueDialogue();
      return;
    }
    else
    {
      // wait for input
      curText = "";
      pushedText = "";
      typing = true;
      // showTalkingUI();
      // updateText();
      hideTalkingUI();
      // dialogue_index++;

      return;
    }
  }

  // spawn item in world
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/item")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    vector<string> x = splitString(s, ' ');
    string name = x[0];
    int p0 = stoi(x[1]);
    int p1 = stoi(x[2]);
    D(x[0]);
    D(x[1]);
    D(x[2]);
    worldItem *a = new worldItem(name, 0);
    a->x = p0;
    a->y = p1;
    M(a->x);
    M(a->y);

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // refresh a trigger by ID
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/refresh")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);

    int tID = stoi(s);
    if (tID < (int)g_triggers.size())
    {
      g_triggers[tID]->active = 1;
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // sleep
  if (scriptToUse->at(dialogue_index + 1).substr(0, 6) == "/sleep")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 7);
    string msstr = s;
    int ms = 0;
    ms = stoi(msstr);
    sleepingMS = ms;
    dialogue_index++;
    sleepflag = 1;
    this->continueDialogue();
    return;
  }

  // mobile sleep, sleep but let the player walk
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/msleep")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 8);
    string msstr = s;
    int ms = 0;
    ms = stoi(msstr);
    sleepingMS = ms;
    sleepflag = 1;
    mobilize = 1;
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // call a script
  // you probably don't want to use this, or overhaul it so it makes a new script caller
  // !!!
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/script")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 8);
    D(s);

    ifstream stream;
    string loadstr;

    loadstr = "resources/maps/" + g_map + "/" + s + ".txt";
    const char *plik = loadstr.c_str();

    stream.open(plik);

    if (!stream.is_open())
    {
      stream.open("scripts/" + s + ".txt");
    }
    string line;

    getline(stream, line);

    vector<string> nscript;
    while (getline(stream, line))
    {
      nscript.push_back(line);
    }

    parseScriptForLabels(nscript);

    adventureUIManager->blip = g_ui_voice;
    adventureUIManager->ownScript = nscript;
    adventureUIManager->talker = protag;
    protag->sayings = nscript;
    adventureUIManager->dialogue_index = -1;
    adventureUIManager->continueDialogue();

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // load savefile
  // /loadsave
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/loadsave")
  {
    loadSave();

    dialogue_index = 0;

    clear_map(g_camera);
    auto x = splitString(g_mapOfLastSave, '/');
    g_mapdir = x[0];
    g_map = x[1];
    
    //load_map(renderer, "resources/maps/" + g_mapOfLastSave + ".map", g_waypointOfLastSave);
    
    string filename = g_levelSequence->levelNodes[0]->mapfilename;
    load_map(renderer, filename,"a");

    // clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
    if (canSwitchOffDevMode)
    {
      init_map_writing(renderer);
    }
    adventureUIManager->hideTalkingUI();

    if(playersUI) {
      protag_is_talking = 2;
    }
    executingScript = 0;
    g_inTitleScreen = 0;
    adventureUIManager->showHUD();

    return;
  }

  // write savefile
  // /save
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/save" || scriptToUse->at(dialogue_index + 1).substr(0, 10) == "/writesave")
  {
    M("in writeSave interpreter");
    writeSave();
    M("writeSave() finished");

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change user
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/user")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    g_saveName = s;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // check savefield
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("\\{([a-zA-Z0-9_]){1,}\\}")))
  {
    M("Tried to check a save field");
    //
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 1);
    s.erase(s.length() - 1, 1);

    int value = checkSaveField(s);

    int j = 1;
    string res = scriptToUse->at(dialogue_index + 1 + j);
    while (res.find('*') != std::string::npos)
    {
      string s = scriptToUse->at(dialogue_index + 1 + j);
      s.erase(0, 1);
      int condition = stoi(s.substr(0, s.find(':')));
      s.erase(0, s.find(':') + 1);
      int jump = stoi(s);
      if (value == condition)
      {
        dialogue_index = jump - 3;
        this->continueDialogue();
        return;
      }
      j++;
      res = scriptToUse->at(dialogue_index + 1 + j);
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // write to savefield
  if (regex_match(scriptToUse->at(dialogue_index + 1), regex("[[:digit:]]+\\-\\>\\{([a-zA-Z0-9_]){1,}\\}")))
  {
    M("tried to write to savedata");
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(s.length() - 1, 1);

    string valuestr = s.substr(0, s.find('-'));
    int value = stoi(valuestr);

    string field = s.substr(s.find('{') + 1, s.length() - 1);
    writeSaveField(field, value);
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // set the prompt for the next question
  // /prompt
  //
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/prompt")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    auto parts = splitString(s, ' ');
    keyboardPrompt = s.substr(8);
    adventureUIManager->dialogProceedIndicator->show = 0;

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // prompt user for text input and save the answer to a save-field
  //
  // /keyboard playername
  // AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrXxTtUuVvWwXxYyZz
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/keyboard")
  {

    string s = scriptToUse->at(dialogue_index + 1);
    auto parts = splitString(s, ' ');
    g_keyboardSaveToField = parts[1];
    g_keyboardInput = "";
    inventorySelection = 0;
    g_alphabet = g_alphabet_lower;
    g_alphabet_textures = &g_alphabetLower_textures;

    //keyboardPrompt = pushedText;

    g_inventoryUiIsLevelSelect = 0;
    g_inventoryUiIsLoadout = 0;
    g_inventoryUiIsKeyboard = 1;
    inPauseMenu = 1;
    g_firstFrameOfPauseMenu = 1;
    adventureUIManager->positionKeyboard();
    adventureUIManager->showInventoryUI();


    // this is the stuff we do when we read '#' (end scripting)
    if (playersUI)
    {
      protag_is_talking = 2;
    }
    executingScript = 0;

    mobilize = 0;
    if(this == adventureUIManager) {
      adventureUIManager->hideTalkingUI();
    }

    return;
  }

  //Launch options menu
  /*
    
    Up              Up        1
    Down            Down      2
    Left            Left      3
    Right           Right     4
    Jump            X         5
    Interact        Z         6
    Inventory       C         7
    Spin/Use Item   A         8
    Fullscreen      Off       9
    Music Volume    0.4      10
    Sound Volume    0.4      11
    Graphics        3        12
    Brightness      100      13

  */
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/settings") 
  {
    M("Check dialogue index before");
    D(adventureUIManager->dialogue_index);
    //write to settingsUi from related variables
    g_settingsUI->show();
    g_inSettingsMenu = 1;
    g_firstFrameOfSettingsMenu = 1;
    g_settingsUI->positionOfCursor = 0;
    g_settingsUI->cursorIsOnBackButton = 0;

    // this is the stuff we do when we read '#' (end scripting)
    executingScript = 0;

    mobilize = 0;
    if(this == adventureUIManager) {
      adventureUIManager->hideTalkingUI();
    }

    return;
  }

  // unconditional jump
  if (scriptToUse->at(dialogue_index + 1).at(0) == ':')
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 1);
    string DIstr = "0";
    DIstr = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    int DI = 0;
    DI = stoi(DIstr);
    dialogue_index = DI - 3;
    this->continueDialogue();
    return;
  }

  // solidify entity
  //  /solidify door 1
  //  /solidify wall 0
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/solidify")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 10);

    entity *solidifyMe = 0;
    auto parts = splitString(s, ' ');

    solidifyMe = searchEntities(parts[0]);
    bool solidifystate = (parts[1] == "1");
    if (solidifyMe != 0)
    {
      if (solidifystate)
      {
        solidifyMe->solidify();
      }
      else
      {
        solidifyMe->unsolidify();
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // semisolidify entity
  //  /semisolidify door 1
  //  /semisolidify wall 0
  if (scriptToUse->at(dialogue_index + 1).substr(0, 13) == "/semisolidify")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 14);

    entity *solidifyMe = 0;
    auto parts = splitString(s, ' ');

    solidifyMe = searchEntities(parts[0]);
    bool solidifystate = (parts[1] == "1");
    if (solidifyMe != 0)
    {
      if (solidifystate)
      {
        solidifyMe->semisolid = 1;
      }
      else
      {
        solidifyMe->semisolid = 0;
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //change an entity's agility, max speed, and slippiness
  // /movement entity agility maxSpeed slippiness
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/movement")
  {
    string s = scriptToUse->at(dialogue_index + 1);


    entity *modifyMe = 0;
    auto x = splitString(s, ' ');
    
    D("check params");
    for(auto i : x) {
      D(i);
    }


    if(x.size() >= 5) {
      modifyMe = searchEntities(x[1]);
      string agilitySTR = x[2];
      string maxSpeedSTR = x[3];
      string slippinessSTR = x[4];
      int agility = stoi(agilitySTR);
      int maxSpeed = stoi(maxSpeedSTR);
      int slippiness = stoi(slippinessSTR);

      if(modifyMe != nullptr) {
        modifyMe->xagil = agility;
        modifyMe->xmaxspeed = maxSpeed;
        modifyMe->friction = slippiness;
      }
    } else {
      E("Not enough parameters for /movement call");
    }
    



    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // change animation data for selected
  // animate direction msPerFrame frameInAnimation LoopAnimation reverse
  // set direction to -1 to not set the direction
  // set msperframe to 0 to not animate
  // set frameInAnimation to -1 to not change
  // set looppAnimation to loop after done
  // set reverse to 1 to play backwards
  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/animate")
  {
    //M("Animate interpreter");
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);
    vector<string> split = splitString(s, ' ');

    entity *ent = selected;
    if (ent != 0)
    {
      int animationset = stoi(split[0]);
      if (animationset != -1)
      {
        ent->animation = stoi(split[0]);
        //I("Set animation to ");
        //I(ent->animation);
      }
      ent->msPerFrame = stoi(split[1]);
      //I("Set msPerFrame to");
      //I(ent->msPerFrame);

      int frameset = stoi(split[3]);
      if(frameset != -1) {
        ent->frameInAnimation = stoi(split[3]);
        //I("Set frameInAnimation to ");
        //I(ent->frameInAnimation);
      }

      ent->loopAnimation = stoi(split[4]);
      //I("Set loopAnimation to ");
      //I(ent->loopAnimation);
      
      ent->reverseAnimation = stoi(split[5]);
      //I("Set reverseAnimation to ");
      //I(ent->reverseAnimation);

      ent->scriptedAnimation = 1;
    } 

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // play sound at an entity
  //  /entsound heavy-door-open doora
  //  /entsound croak protag
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/entsound")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 9);
    vector<string> split = splitString(s, ' ');
    string soundName = split[0];
    string entName = split[1];
    entity *hopeful = 0;
    hopeful = searchEntities(entName);
    if (hopeful != nullptr)
    {
      // play a sound at the entity
      playSoundByName(soundName, hopeful->getOriginX(), hopeful->getOriginY());
    }
    else
    {
      // entity was not found, so lets not play a sound
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // play a sound that's been loaded into the level as a cue
  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/mapsound")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    auto x = splitString(s, ' ');
    if(x.size() > 0) {
      D(x[1]);
      playSoundByName(x[1]);
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  if (scriptToUse->at(dialogue_index + 1).substr(0, 8) == "/nomusic")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    auto x = splitString(s, ' ');
    if(x.size() > 0) {
      g_musicSilenceMs = stoi(x[1]);
    }
    Mix_FadeOutMusic(200);

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // play a sound from the disk
  if (scriptToUse->at(dialogue_index + 1).substr(0, 14) == "/loadplaysound")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> split = splitString(s, ' ');
    string loadstring = "resources/static/sounds/" + split[1] + ".wav";
    D(loadstring);
    
    float volume = stof(split[2]);

    Mix_Chunk *a = nullptr;

    if(a == nullptr) {
      a = loadWav(loadstring.c_str());
    }

    if (!g_mute && a != nullptr)
    {
      Mix_Volume(0, volume * g_sfx_volume * 128);
      Mix_PlayChannel(0, a, 0);
      Mix_Volume(0, g_sfx_volume * 128);
    }

    //if the sound is longer than 15 seconds, just place it in the level and call it from the script
    g_loadPlaySounds.push_back(pair<int,Mix_Chunk*>(15000,a));

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // hide textbox
  if (scriptToUse->at(dialogue_index + 1).substr(0, 7) == "/hideui")
  {
    this->hideTalkingUI();
    this->curText = "";
    this->pushedText = "";
    this->hideInventoryUI();
    inPauseMenu = 0;
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // wait for input
  if (scriptToUse->at(dialogue_index + 1).substr(0, 13) == "/waitforinput")
  {
    protag_is_talking = 1;
    curText = "";
    pushedText = "";
    typing = true;
    hideTalkingUI();
    dialogue_index++;

    return;
  }
  
  // make selected entity tangible
  //  /tangible 0
  //  /tangible 1

  if (scriptToUse->at(dialogue_index + 1).substr(0, 9) == "/tangible")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    auto x = splitString(s, ' ');

    selected->tangible = stoi(x[1]);

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // heal entity
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/heal")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    string tangiblestatestr = "0";
    tangiblestatestr = s; // s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
    int tangiblestate = stof(tangiblestatestr);

    entity *hopeful = searchEntities(name);
    if (hopeful != nullptr)
    {
      hopeful->hp += tangiblestate;
      if (hopeful->hp > hopeful->maxhp)
      {
        hopeful->hp = hopeful->maxhp;
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // /hurt entityname damage
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/hurt")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 6);
    string name = s.substr(0, s.find(' '));
    s.erase(0, s.find(' ') + 1);
    string tangiblestatestr = "0";
    tangiblestatestr = s; // s.substr(0, s.find(' ')); s.erase(0, s.find(' ') + 1);
    int tangiblestate = stof(tangiblestatestr);

    entity *hopeful = searchEntities(name, talker);
    if (hopeful != nullptr)
    {
      hopeful->hp -= tangiblestate;
      hopeful->flashingMS = g_flashtime;
      if(hopeful->faction != 0) {
        //playSound(1, g_enemydamage, 0);
      } else {
        if(hopeful == protag) {
          //playSound(2, g_playerdamage, 0);
        } else {
          //playSound(3, g_npcdamage, 0);
        }
      }
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }


  // overwrite a save
  if (scriptToUse->at(dialogue_index + 1).substr(0, 10) == "/clearsave")
  {
    string s = scriptToUse->at(dialogue_index + 1);
    s.erase(0, 11);
    string name = s;
    I(s);
    if(name != "a" && name != "b" && name != "c") {
      //bad name
      dialogue_index++;
      this->continueDialogue();
      return;
    }
    
    //for safety
//    if(s.find("..") != std::string::npos) {
//      E("Tried to write a save outside of user/saves directory");
//      dialogue_index++;
//      this->continueDialogue();
//      return;
//    }

    I("trying to clear save " + s);
    try {
      filesystem::remove_all("user/saves/" + s + ".save");
      filesystem::copy("user/saves/newsave.save", "user/saves/" + s + ".save");
    } catch (const filesystem::filesystem_error& e) {
      cerr << e.what() << endl;
    }
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // quit the game
  if (scriptToUse->at(dialogue_index + 1).substr(0, 5) == "/quit")
  {
    quit = 1;
    if(playersUI) {
      protag_is_talking = 2;
    }
    return;
  }

  //unlock a level by name
  // /unlock twistland
  if (scriptToUse->at(dialogue_index + 1).substr(0, 12) == "/unlocklevel")
  {
    M("unlocklevel interpreter");
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');

    for(int i = 0; i < g_levelSequence->levelNodes.size(); i++) {
      string lowerName = g_levelSequence->levelNodes[i]->name;

      std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) { if(c == ' ') {int e = '-'; return e;} else {return std::tolower(c);}  } );
      
      if(lowerName == x[1]) {
        g_levelSequence->levelNodes[i]->locked = 0;
      }

      //!!! add some popup message

    }
    
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // show the level select interface
  // /levelselect
  if (scriptToUse->at(dialogue_index + 1).substr(0, 12) == "/levelselect")
  {
    M("level select interp");
    g_inventoryUiIsLevelSelect = 1;
    g_inventoryUiIsKeyboard = 0;
    g_inventoryUiIsLoadout = 0;
    inventorySelection = 0;
    inPauseMenu = 1;
    g_firstFrameOfPauseMenu = 1;
    
    M("level select interp A");
    //clear_map(g_camera);
    adventureUIManager->escText->updateText("", -1, 0.9);
    adventureUIManager->positionInventory();
    adventureUIManager->showInventoryUI();
    adventureUIManager->hideHUD();
    M("level select interp B");

    // this is the stuff we do when we read '#' (end scripting)
    if (playersUI)
    {
      protag_is_talking = 2;
    }
    executingScript = 0;
    M("level select interp C");

    mobilize = 0;
    if(this == adventureUIManager) {
      adventureUIManager->hideTalkingUI();
    }
    M("level select interp D");

    return;
  }


  //if this is a label, don't print that
  if (scriptToUse->at(dialogue_index + 1).substr(0, 1) == "<")
  {
    dialogue_index++;
    this->continueDialogue();
    return;
  }

  //display fancytext
  if (scriptToUse->at(dialogue_index + 1).substr(0, 1) == "`")
  {
    dialogue_index++;
    g_fancybox->show = 1;
    g_fancybox->clear();
    pushFancyText(talker);
    return;
  }

  //give the player a new usable, which will later be written to their save file
  // /unlockusable cicada
  if (scriptToUse->at(dialogue_index + 1).substr(0, 13) == "/unlockusable")
  {
    M("/unlockusable");
    string s = scriptToUse->at(dialogue_index + 1);
    vector<string> x = splitString(s, ' ');
    
    if(x.size() <= 1) { E("Not enough args for /unlockusable call");}
    
    string nameOfUsable = x[1];

    //make sure we haven't already unlocked it
    int good = 1;
    for(auto x: g_backpack) {
      if(x->internalName == nameOfUsable) {
        good = 0;
        break;
      }
    }

    if(good == 1) {
      usable* newUsable = new usable(nameOfUsable);
      //!!! temporary for debugging
      g_backpack.push_back(newUsable);
      //adventureUIManager->showBackpackUI();
    }

    dialogue_index++;
    this->continueDialogue();
    return;
  }

  // default - keep talking
  dialogue_index++;
  pushText(talker);
}

//position UI elements for keyboard
void adventureUI::positionKeyboard() {
  inventoryYStart = 0.5;
  inventoryYEnd = 1;
  escText->boxY = 0.1; //now holds prompt
  inventoryB->y = 0.01;
  inventoryA->y = 0.25;
}

//position UI elements for inventory
void adventureUI::positionInventory() {
  inventoryYStart = 0.05;
  inventoryYEnd = 0.6;
  escText->boxY = 0.83; //now holds prompt
  inventoryB->y = 0.76;
  inventoryA->y = 0.01;

}

//hide heart and other stuff if the player is in the menus
void adventureUI::hideHUD() {
  showHud = 0;
  //healthPicture->show = 0;
  //healthText->show = 0;
  hideScoreUI();
  hotbarFocus->show = 0;
  hotbarMutedXIcon->show = 0;
  hotbar->show = 0;
  cooldownIndicator->show = 0;
  systemClock->show = 0;
}

void adventureUI::showHUD() {
  showHud = 1;
  //healthPicture->show = 1;
  //healthText->show = 1;
  hotbarFocus->show = 1;
  hotbarMutedXIcon->show = 1;
  hotbar->show = 1;
  cooldownIndicator->show = 1;
  systemClock->show = 1;
}

ribbon::ribbon() {
  g_ribbons.push_back(this);
}

ribbon::~ribbon() {
  g_ribbons.erase(remove(g_ribbons.begin(), g_ribbons.end(), this), g_ribbons.end());
}

void ribbon::render(SDL_Renderer* renderer, camera fcamera) {
  if(!visible) {return;}
  if(r_length == 0) {
    SDL_QueryTexture(texture, NULL, NULL, &r_length, &r_thickness);
  }


  float u1; float v1;
  transform3dPoint(x1,y1,z1,u1,v1);
  float u2; float v2;
  transform3dPoint(x2,y2,z2,u2,v2);

  float uc = (u1 + u2)/2;
  float vc = (v1 + v2)/2;

  int dist = Distance(u1, v1, u2, v2);


  SDL_Rect drect = {(int)u2, (int)v2, dist, r_thickness};

  float angle = atan2( (v1 - v2) , (u1 - u2) ) * (180 / M_PI);

  SDL_Point center;
  center.x = 0;
  center.y = r_thickness/2;

  SDL_RenderCopyEx(renderer, texture, NULL, &drect, angle, &center, SDL_FLIP_NONE);
}

