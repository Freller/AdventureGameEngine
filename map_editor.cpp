#ifndef mapeditor_h
#define mapeditor_h

#include <iostream>
#include <vector>
#include <fstream> //loading
#include <string>
#include <cstring>   //loading
#include <sstream>   //loading
#include <algorithm> //sort
#include <fstream>
#include <filesystem> //making directories
#include <regex>      //for readings scripts

#include "map_editor.h"
#include "objects.h"
#include "globals.h"
#include "utils.h"

void dungeonFlash();

using namespace std;

// for sorting ui on mapload
int compare_ui(ui *a, ui *b)
{
  return a->priority < b->priority;
}

void sort_ui(vector<ui *> &g_ui)
{
  stable_sort(g_ui.begin(), g_ui.end(), compare_ui);
}

void populateMapWithEntities()
{
  bool stillAdding = 1;
  int currentBudget = g_budget;    // we'll subtract from this current budget until we can't fit any more entities in the map
  vector<string> possibleEntities; // a vector of entities that we could add to the map.
  vector<int> associatedCosts;     // a vector of costs for each entity
  for (std::map<string, int>::iterator it = enemiesMap.begin(); it != enemiesMap.end(); ++it)
  {
    possibleEntities.push_back(it->first);
    associatedCosts.push_back(it->second);
  }
  if (possibleEntities.size() > 0 && g_navNodes.size() > 0)
  {
    while (stillAdding)
    {
      // pick an entity from possible entities
      int randomElement = rand() % possibleEntities.size();
      if (associatedCosts[randomElement] <= currentBudget)
      {
        // add this enemy
        entity *a = new entity(renderer, possibleEntities[randomElement]);
        int randomNavnode = rand() % (int)g_navNodes.size();
        a->x = g_navNodes[randomNavnode]->x - a->bounds.x - a->bounds.width / 2;
        a->y = g_navNodes[randomNavnode]->y - a->bounds.y - a->bounds.height / 2;
        a->z = g_navNodes[randomNavnode]->z + 1000;
        currentBudget -= associatedCosts[randomElement];
        //!!! - revist this
        if (associatedCosts[randomElement] == 0)
        {
          // we are probably going to crash
          associatedCosts[randomElement] = 30;
        }
      }
      else
      {
        // we can't afford this guy, take him out of the lists
        possibleEntities.erase(possibleEntities.begin() + randomElement);
        associatedCosts.erase(associatedCosts.begin() + randomElement);
      }

      if (possibleEntities.size() <= 0 || currentBudget <= 0)
      {
        stillAdding = 0;
      }
    }
  }
}

void load_map(SDL_Renderer *renderer, string filename, string destWaypointName)
{
  breakpoint();
  M("Loading map: " + filename);
  transition = 1;
  debugClock = clock();
  mapname = filename;
  g_loadingATM = 1;
  g_hog = 0;
  protag_is_talking = 0;

  // parse name from fileaddress
  auto x = splitString(mapname, '/');

  if(x.size() > 3) {
    g_map = x.at(3);
    g_map.pop_back();
    g_map.pop_back();
    g_map.pop_back();
    g_map.pop_back();
    g_waypoint = destWaypointName;
  }

  // hide HUD if this is a special map, show it otherwise
  g_showHUD = !(g_map.substr(0, 3) == "sp-");

  vector<string> strings = {};
  if(PHYSFS_exists(filename.c_str())) {
    strings = loadText(filename);
  }

  string line;
  string word, s0, s1, s2, s3, s4;
  float p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10;
  (void)p10;

  background = 0;

  g_budget = 0;
  g_budget = strtol(word.c_str(), NULL, 10);
  int index = 0;

  while (index < strings.size())
  {
    line = strings[index];
    index++;
    istringstream iss(line);
    word = line.substr(0, line.find(" "));

    if (word == "enemy")
    {
      iss >> s0 >> s1 >> p0;
      enemiesMap[s1] = p0;
    }

    if (word == "limits")
    {
      iss >> s0 >> p1 >> p2 >> p3 >> p4;
      g_camera.lowerLimitX = p1;
      limits[0] = p1;
      g_camera.lowerLimitY = p2;
      limits[1] = p2;
      g_camera.upperLimitX = p3;
      limits[2] = p3;
      g_camera.upperLimitY = p4;
      limits[3] = p4;
      g_camera.enforceLimits = 1;
    }
    if (word == "bg" && g_useBackgrounds)
    {
      iss >> s0 >> backgroundstr;
      background = loadTexture(renderer, "resources/static/backgrounds/" + backgroundstr + "qoi");
      g_backgroundLoaded = 1;
      SDL_SetTextureColorMod(background, 255 * (1 - g_background_darkness), 255 * (1 - g_background_darkness), 255 * (1 - g_background_darkness));
    }
    if (word == "dark")
    {
      iss >> s0 >> p1;
      g_fogofwarEnabled = p1;
      if (g_fogofwarEnabled)
      {
        for (auto x : g_fogslates)
        {
          x->tangible = 1;
        }
        for (auto x : g_fogslatesA)
        {
          x->tangible = 1;
        }
        for (auto x : g_fogslatesB)
        {
          x->tangible = 1;
        }
      }
      else
      {
        for (auto x : g_fogslates)
        {
          x->tangible = 0;
        }
        for (auto x : g_fogslatesA)
        {
          x->tangible = 0;
        }
        for (auto x : g_fogslatesB)
        {
          x->tangible = 0;
        }
      }
    }
    if (word == "box")
    {
      // M("loading collisisons" << endl;
      iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5 >> s1 >> s2 >> p6 >> p7 >> p8 >> s3;
      box *c = new box(p1, p2, p3, p4, p5, s1, s2, p6, p7, p8, s3.c_str());
      (void)c;
    }
    if (word == "islope")
    {
      iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> p7 >> p8;
      impliedSlope *i = new impliedSlope(p1, p2, p3, p4, p5, p6, p7, p8);
      (void)i;
    }
    if (word == "islopet")
    {
      iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6;
      impliedSlopeTri *i = new impliedSlopeTri(p1, p2, p3, p4, p5, p6);
      (void)i;
    }
    if (word == "entity")
    {
      // M("loading entity" << endl;
      iss >> s0 >> s1 >> p0 >> p1 >> p2 >> p3 >> p4;
      const char *plik = s1.c_str();
      entity *e = new entity(renderer, plik);
      e->x = p0;
      e->y = p1;
      e->z = p2;
      e->shadow->x = e->x + e->shadow->xoffset;
      e->shadow->y = e->y + e->shadow->yoffset;
      e->defaultAnimation = p3;
      e->animation = p3;
      if (p4 == 1)
      {
        e->flip = SDL_FLIP_HORIZONTAL;
      }
      e->steeringAngle = convertFrameToAngleNew(e->animation, e->flip == SDL_FLIP_HORIZONTAL);
      e->targetSteeringAngle = e->steeringAngle;

      if(e->animation > e->yframes) {
        e->animation = e->yframes - 1;
      }
      
      if(e->parent != nullptr) {

          //position orbital properly
//          {
//          e->z = e->parent->z -10 - (e->parent->height - e->parent->curheight);
//
//
//          float angle = convertFrameToAngle(e->parent->frame, e->parent->flip == SDL_FLIP_HORIZONTAL);
//
//
//
//          //orbitoffset is the number of frames, counter-clockwise from facing straight down
//          float fangle = angle;
//          fangle += (float)e->orbitOffset * (M_PI/4);
//          fangle = fmod(fangle , (2* M_PI));
//
//          e->setOriginX(e->parent->getOriginX() + cos(fangle) * e->orbitRange);
//          e->setOriginY(e->parent->getOriginY() + sin(fangle) * e->orbitRange);
//
//          e->sortingOffset = e->baseSortingOffset + sin(fangle) * 21 + 10 + (e->parent->height - e->parent->curheight);
//
//          if(e->yframes == 8) {
//            e->animation = convertAngleToFrame(fangle);
//            e->flip = SDL_FLIP_NONE;
//          } else {
//            e->flip = e->parent->flip;
//            e->animation = e->parent->animation;
//            if(e->yframes == 1) {
//              e->animation = 0;
//            }
//          }
//
//          //update shadow
//          float heightfloor = 0;
//          e->layer = max(e->z /64, 0.0f);
//          e->layer = min(layer, (int)g_boxs.size() - 1);
//
//          //should we fall?
//          //bool should_fall = 1;
//          float floor = 0;
//          if(e->layer > 0) {
//            //!!!
//            rect eMovedBounds = rect(e->bounds.x + e->x + e->xvel * ((double) elapsed / 256.0), e->bounds.y + e->y + e->yvel * ((double) elapsed / 256.0), e->bounds.width, e->bounds.height);
//            //rect eMovedBounds = rect(bounds.x + x, bounds.y + y, bounds.width, bounds.height);
//            for (auto n : g_boxs[layer - 1]) {
//              if(RectOverlap(n->bounds, eMovedBounds)) {
//                floor = 64 * (layer);
//                break;
//              }
//            }
//            for (auto n : g_triangles[layer - 1]) {
//              if(TriRectOverlap(n, eMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
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
//                if(RectOverlap(n->bounds, eMovedBounds)) {
//                  shadowFloor = 64 * (i + 1);
//                  breakflag = 1;
//                  break;
//                }
//              }
//              if(breakflag) {break;}
//              for (auto n : g_triangles[i]) {
//                if(TriRectOverlap(n, eMovedBounds.x, thisMovedBounds.y, thisMovedBounds.width, thisMovedBounds.height)) {
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
//            e->shadow->z = shadowFloor;
//          } else {
//            e->shadow->z = heightfloor;
//            floor = heightfloor;
//          }
//          shadow->x = x + shadow->xoffset;
//          shadow->y = y + shadow->yoffset;
//          }

      }

      // if an entity has been set to navblock, disable overlapping nodes now that the position has been set
      if (e->navblock)
      {
        auto r = e->getMovedBounds();
        for (auto x : g_navNodes)
        {
          // !!! this also isn't 3d-safe
          rect nodespot = {x->x - 32, x->y - 22, 64, 45};
          if (RectOverlap(r, nodespot))
          {
            e->overlappedNodes.push_back(x);
            x->enabled = 0;
          }
        }
      }
      if(e->name == "common/doora") {
        D(e->y);
        D(e->bounds.y);
      }

    }
    if (word == "item")
    {
      // M("loading entity" << endl;
      iss >> s0 >> s1 >> p0 >> p1 >> p2;
      const char *plik = s1.c_str();
      worldItem *e = new worldItem(plik, 0);
      e->x = p0;
      e->y = p1;
      e->z = p2;
      e->shadow->x = e->x + e->shadow->xoffset;
      e->shadow->y = e->y + e->shadow->yoffset;
      (void)e;
    }
    if (word == "tile")
    {
      // M("loading tile" << endl;
      iss >> s0 >> s1 >> s2 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> p7 >> p8 >> p9;
      const char *plik1 = s1.c_str();
      const char *plik2 = s2.c_str();
      tile *t = new tile(renderer, plik1, plik2, p1, p2, p3, p4, p5, p6, p7, p8, p9);
      (void)t;
    }
    if (word == "triangle")
    {
      // M("loading triangle" << endl;
      iss >> s0 >> p1 >> p2 >> p3 >> p4 >> p5 >> s1 >> s2 >> p6 >> p7 >> p8;
      tri *t = new tri(p1, p2, p3, p4, p5, s1, s2, p6, p7, p8);
      (void)t;
    }
    if (word == "ramp")
    {
      // ofile << "ramp " << n->x << " " << n->y << " " << i << " " << n->type << " " << n->walltexture << " " << n->captexture << endl;
      iss >> s0 >> p0 >> p1 >> p2 >> p3 >> s1 >> s2;
      ramp *r = new ramp(p0, p1, p2, p3, s1, s2);
      (void)r;
    }
    if (word == "mapObject")
    {
      iss >> s0 >> s1 >> s2 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> p7;
      mapObject *e = new mapObject(renderer, s1, s2.c_str(), p1, p2, p3, p4, p5, p6, p7);
      (void)e;
    }
    if (word == "door")
    {
      iss >> s0 >> s1 >> s2 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6;
      const char *map = s1.c_str();
      door *d = new door(renderer, map, s2, p1, p2, p3, p4, p5, p6);
      (void)d;
    }
    if(word == "ddoor")
    {
      M("Loaded a ddoor");
      iss >> s0 >> p1 >> p2 >> p3 >> p4;
      D(p1);
      D(p2);
      D(p3);
      D(p4);
      dungeonDoor *d = new dungeonDoor(p1, p2, p3, p4);
      (void)d;
    }

    if (word == "trigger")
    {
      iss >> s0 >> s1 >> p1 >> p2 >> p3 >> p4 >> p5 >> p6 >> s2;
      const char *binding = s1.c_str();
      trigger *t = new trigger(binding, p1, p2, p3, p4, p5, p6, s2);
      (void)t;
    }
    if (word == "worldsound")
    {
      iss >> s0 >> s1 >> p1 >> p2;
      const char *sprite = s1.c_str();
      worldsound *w = new worldsound(sprite, p1, p2);
      (void)w;
    }
    if (word == "music")
    {
      iss >> s0 >> s1 >> p1 >> p2;
      const char *sprite = s1.c_str();
      musicNode *m = new musicNode(sprite, p1, p2);
      (void)m;
    }
    if (word == "cuesound")
    {

      iss >> s0 >> s1 >> p1 >> p2 >> p3;
      const char *sprite = s1.c_str();
      cueSound *c = new cueSound(sprite, p1, p2, p3);
      (void)c;
    }
    if (word == "waypoint")
    {
      iss >> s0 >> s1 >> p1 >> p2 >> p3 >> p4;
      waypoint *w = new waypoint(s1, p1, p2, p3, p4);
      (void)w;
    }

    if (word == "poi")
    {
      iss >> s0 >> p1 >> p2 >> p3;
      pointOfInterest *p = new pointOfInterest(p1, p2, p3);
      (void)p;
    }

    if (word == "ui")
    {
      iss >> s0 >> s1 >> p0 >> p1 >> p2 >> p3 >> p4;
      const char *plik = s1.c_str();
      ui *u = new ui(renderer, plik, p0, p1, p2, p3, p4);
      u->mapSpecific = 1;
    }

    if (word == "heightmap")
    {
      iss >> s0 >> s1 >> s2 >> p0;
      heightmap *h = new heightmap(s2.c_str(), s1.c_str(), p0);
      (void)h;
    }
    if (word == "navNode")
    {
      iss >> s0 >> p1 >> p2 >> p3;
      navNode *n = new navNode(p1, p2, p3);
      (void)n;
    }
    if (word == "navNodeEdge")
    {
      iss >> s0 >> p1 >> p2;
      g_navNodes[p1]->Add_Friend(g_navNodes[p2]);
      while (iss >> p2)
      {
        g_navNodes[p1]->Add_Friend(g_navNodes[p2]);
      }
    }
    if (word == "listener")
    {
      iss >> s0 >> s1 >> p1 >> p2 >> s2 >> p3 >> p4;
      listener *l = new listener(s1, p1, p2, s2, p3, p4);
      (void)l;
    }
    if (word == "collisionZone")
    {
      iss >> s0 >> p1 >> p2 >> p3 >> p4;
      collisionZone *c = new collisionZone(p1, p2, p3, p4);
      (void)c;
    }
  }

  {
    mapObject *child;

    // build map from boxs
    for (long long unsigned int i = 0; i < g_boxs.size(); i++)
    {
      for (box *box : g_boxs[i])
      {
        // don't calculate lighting by invisible walls
        if (box->walltexture == "resources/engine/seethru.qoi")
        {
          break;
        }
        // handle rect
        child = new mapObject(renderer, box->walltexture, "&", box->bounds.x, box->bounds.y + box->bounds.height, i * 64, box->bounds.width, 55, 1);
        child->parent = box;
        box->children.push_back(child);
        // if there's no box above, make a cap
        if (box->capped)
        {
          // related to resolution of wallcap

          int step = g_platformResolution;
          for (int i = 0; i < box->bounds.height; i += step)
          {
            child = new mapObject(renderer, box->captexture, "&", box->bounds.x, box->bounds.y + i + step, box->layer * 64 + 64, box->bounds.width, step, 0);
            child->parent = box;
            box->children.push_back(child);
          }
          // child = new mapObject(renderer, box->captexture, "&", box->bounds.x, box->bounds.y + box->bounds.height, box->layer * 64 + 64, box->bounds.width, box->bounds.height, 0);
          // box->children.push_back(child);

          if (box->shineBot)
          {
            // shine
            child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", box->bounds.x, box->bounds.y + box->bounds.height + 54 / 2, box->layer * 64 + 64, box->bounds.width, 54);
            child->sortingOffset = -26;
            child->parent = box;

            box->children.push_back(child);
          }
          if (box->shineTop)
          {
            // back
            child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", box->bounds.x, box->bounds.y + 54 / 2, box->layer * 64 + 64, box->bounds.width, 54 / 2);
            child->parent = box;
            box->children.push_back(child);
          }
        }
        // floor shadows

        // front shading
        if (box->shadeBot == 1)
        {
          child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", box->bounds.x, box->bounds.y + box->bounds.height + 19 + 2, 64 * box->layer + 2, box->bounds.width, 55);
          child->parent = box;
          box->children.push_back(child);
        }
        // left
        int step = g_platformResolution;
        if (box->shadeLeft)
        {
          for (int i = 0; i < box->bounds.height; i += step)
          {
            child = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", box->bounds.x - 27, box->bounds.y + i + g_platformResolution, 64 * box->layer, 55 / 2, step);
            child->parent = box;
            box->children.push_back(child);
          }
        }
        if (box->shadeRight)
        {
          for (int i = 0; i < box->bounds.height; i += step)
          { // 5, 8
            child = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", box->bounds.x + box->bounds.width, box->bounds.y + i + g_platformResolution, 64 * box->layer, 55 / 2, step);
            child->parent = box;
            box->children.push_back(child);
          }
        }

        // corner a
        if (box->shadeLeft && box->shadeTop)
        {
          child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", box->bounds.x - (38 - 19), box->bounds.y, 64 * box->layer, 32, 19, 0, 0);
          child->parent = box;
          box->children.push_back(child);
        }
        // corner b
        if (box->shadeRight && box->shadeTop)
        {
          child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", box->bounds.x + box->bounds.width, box->bounds.y, 64 * box->layer, 32, 19, 0, 0);
          child->parent = box;
          box->children.push_back(child);
        }
        // corner c
        if (box->shadeLeft && (box->shadeBot == 1))
        {
          child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", box->bounds.x - (38 - 19), box->bounds.y + box->bounds.height + (38 - 19), 64 * box->layer, 19, 19, 0, 0);
          child->parent = box;
          box->children.push_back(child);
        }
        // corner d
        if (box->shadeRight && (box->shadeBot == 1))
        {
          child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", box->bounds.x + box->bounds.width, box->bounds.y + box->bounds.height + (38 - 19), 64 * box->layer, 19, 19, 0, 0);
          child->parent = box;
          box->children.push_back(child);
        }
      }
    }
   
    //add shading for implied slopes
    for (auto i : g_impliedSlopes) {
      if(!i->shadedAtAll) {continue;}
      //all implied slopes have top shading
      child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->bounds.x, i->bounds.y + 19 + 2, 64 * i->layer + 2, i->bounds.width, 55);
      child->parent = i;
      i->children.push_back(child);

      if(i->shadeLeft) {
        for (int j = 0; j < i->bounds.height; j += g_platformResolution)
        {
          child = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", i->bounds.x - 27, i->bounds.y + j + g_platformResolution, 64 * i->layer, 55 / 2, g_platformResolution);
          child->parent = i;
          i->children.push_back(child);
        }

        child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", i->bounds.x - (38 - 19), i->bounds.y, 64 * i->layer, 19, 19, 0, 0);
        child->parent = i;
        i->children.push_back(child);

      }

      if(i->shadeRight) {
        for (int j = 0; j < i->bounds.height; j += g_platformResolution)
        { // 5, 8
          child = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", i->bounds.x + i->bounds.width, i->bounds.y + j + g_platformResolution, 64 * i->layer, 55 / 2, g_platformResolution);
          child->parent = i;
          i->children.push_back(child);
        }

        child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", i->bounds.x + i->bounds.width, i->bounds.y, 64 * i->layer, 32, 19, 0, 0);
        child->parent = i;
        i->children.push_back(child);

      }


    }

    for (auto i : g_impliedSlopeTris) {
      //0 - straight
      //1 - outcurve
      //2 - incurve
      switch(i->style) {
        case 0: {
          if(i->type == 1) {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 + j, i->y1 + 30 - 64+ (j * XtoY) - 1, i->layer * 64, step, 50, 0, -(j * XtoY));
              child->parent = i;
              i->children.push_back(child);
            }

          } else {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 + j - 64, i->y1 + 30 - (j * XtoY) - 1, i->layer * 64, step, 50, 0, (j * XtoY));
              child->parent = i;
              i->children.push_back(child);
            }

          }

          break;
        }
        case 1: { //outcurve
          if(i->type == 1) {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 + j, i->y1 + 35 - (((pow(pow(64, 2) - pow(j, 2), 0.5))) * XtoY) - 0.5 - 32, i->layer * 64, step, 34, 0, (((pow(pow(64, 2) - pow(j, 2), 0.5))) * XtoY));
              child->parent = i;
              i->children.push_back(child);
            }

          } else {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 - j, i->y1 + 35 - (((pow(pow(64, 2) - pow(j, 2), 0.5))) * XtoY) - 0.5 - 32, i->layer * 64, step, 34, 0, (((pow(pow(64, 2) - pow(j, 2), 0.5))) * XtoY));
              child->parent = i;
              i->children.push_back(child);
            }

          }
          break;
        
        }

        case 2: { //incurve
          if(i->type == 1) {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 - j + 64, i->y1 + 55 + 30 - ((64 - pow(pow(64, 2) - pow(j, 2), 0.5)) * XtoY) - 1 - 64, i->layer * 64, step, 50, 0, ((64 - pow(pow(64, 2) - pow(j, 2), 0.5)) * XtoY) + 0);
              child->parent = i;
              i->children.push_back(child);
            }

          } else {

            int step = g_TiltResolution;
            for (int j = 0; j < 64; j += step)
            {
              child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", i->x2 + j - 64, i->y1 + 55 + 30 - ((64 - pow(pow(64, 2) - pow(j, 2), 0.5)) * XtoY) - 1 - 64, i->layer * 64, step, 50, 0, ((64 - pow(pow(64, 2) - pow(j, 2), 0.5)) * XtoY) + 0);
              child->parent = i;
              i->children.push_back(child);
            }

          }
          break;
        }


      }



    }


    for (vector<tri *> layer : g_triangles)
    {
      for (auto triangle : layer)
      {
        // handle plain tri
        if (triangle->style == 0)
        {
          if (triangle->type == 0)
          {
            int step = g_platformResolution;
            if (triangle->capped)
            {
              for (int i = 0; i < 55; i += step)
              {
                child = new mapObject(renderer, triangle->captexture, "resources/engine/a.qoi", triangle->x2, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
              // diagonal shine
              step = g_TiltResolution;
              for (int i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 55 + 35 - (i * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, (i * XtoY) + 0);
                child->sortingOffset = -25;
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }

            step = g_TiltResolution;
            int vstep = 64;
            for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
            {
              for (float i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, triangle->walltexture, "&", triangle->x2 + i, triangle->y1 + 55 - (i * XtoY) - 1, j, step, 32, 1, (i * XtoY));
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }

            step = g_TiltResolution;
            if (triangle->shaded)
            {
              for (int i = 0; i < 64; i += step)
              {

                child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i, triangle->y1 + 55 + 30 - (i * XtoY) - 1, triangle->layer * 64, step, 50, 0, (i * XtoY) + 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
          }
          else
          {
            if (triangle->type == 3)
            {
              int step = g_platformResolution;
              if (triangle->capped)
              {
                for (int i = 0; i < 55; i += step)
                {
                  child = new mapObject(renderer, triangle->captexture, "resources/engine/b.qoi", triangle->x1 + 1, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
                step = g_TiltResolution;
                for (int i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 35 - (((64 - step) - i) * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, ((64 - i) * XtoY));
                  child->sortingOffset = -25;
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              int vstep = 64;
              for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
              {
                for (float i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, triangle->walltexture, "&", triangle->x1 + i, triangle->y1 + 55 - (((64 - step) - i) * XtoY) - 1, j, step, 32, 1, ((64 - i) * XtoY));
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              if (triangle->shaded)
              {
                for (int i = 0; i < 64; i += step)
                {

                  child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 30 - (((64 - step) - i) * XtoY) - 1, 64 * triangle->layer, step, 50, 0, ((64 - i) * XtoY));
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }
            }
            else
            {
              if (triangle->type == 2)
              {
                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/c.qoi", triangle->x1 + 1, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 35 - (i * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, (i * XtoY) + 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
                // child = new mapObject(renderer, triangle->captexture, "resources/engine/c.qoi", triangle->x1 + 1, triangle->y2 + 55 + 1, triangle->layer * 64 + 64, 64 + 1, 54 + 1, 0, 0, 0);
                // triangle->children.push_back(child);
              }
              else
              {

                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/d.qoi", triangle->x2, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 35 - (((64 - step) - i) * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, ((64 - i) * XtoY));
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
              }
            }
          }
        }
        else if (triangle->style == 1)
        {
          if (triangle->type == 0)
          {
            int step = g_platformResolution;
            if (triangle->capped)
            {
              for (int i = 0; i < 55; i += step)
              {
                child = new mapObject(renderer, triangle->captexture, "resources/engine/aro.qoi", triangle->x2, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
              // diagonal shine
              step = g_TiltResolution;
              for (int i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 55 + 35 - ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) + 0);
                child->sortingOffset = -15;
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            step = g_TiltResolution;
            int vstep = 64;
            for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
            {
              for (float i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, triangle->walltexture, "&", triangle->x2 + i, triangle->y1 + 55 - ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY), j, step, 33, 1, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY));
                child->sortingOffset = 15;
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            step = g_TiltResolution;
            if (triangle->shaded)
            {
              for (int i = 0; i < 64; i += step)
              {

                child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i, triangle->y1 + 55 + 30 - ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64, step, 50, 0, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) + 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }

              //This is to fill in a "gap" of shading between this corner and the block behind
              child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", triangle->x2 + 64, triangle->y1 + 19, 64 * triangle->layer, 19, 19, 0, 0);
              child->parent = triangle;
              triangle->children.push_back(child);
            }
          }
          else
          {
            if (triangle->type == 3)
            {
              int step = g_platformResolution;
              if (triangle->capped)
              {
                for (int i = 0; i < 55; i += step)
                {
                  child = new mapObject(renderer, triangle->captexture, "resources/engine/bro.qoi", triangle->x1 + 1, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
                step = g_TiltResolution;
                for (int i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 35 - (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY));
                  child->sortingOffset = -15;
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              int vstep = 64;
              for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
              {
                for (float i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, triangle->walltexture, "&", triangle->x1 + i, triangle->y1 + 55 - (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY), j, step, 33, 1, ((64 - i) * XtoY));
                  child->parent = triangle;
                  child->sortingOffset = 15;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              if (triangle->shaded)
              {
                for (int i = 0; i < 64; i += step)
                {

                  child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 30 - (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY), 64 * triangle->layer, step, 50, 0, (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY));
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
                //This is to fill in a "gap" of shading between this corner and the block behind
                child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", triangle->x1 - 19, triangle->y1 + 19, 64 * triangle->layer, 19, 19, 0, 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            else
            {
              if (triangle->type == 2)
              {
                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/cro.qoi", triangle->x1 + 1, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 35 - (((pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, (((pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY) + 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
                // child = new mapObject(renderer, triangle->captexture, "resources/engine/c.qoi", triangle->x1 + 1, triangle->y2 + 55 + 1, triangle->layer * 64 + 64, 64 + 1, 54 + 1, 0, 0, 0);
                // triangle->children.push_back(child);
              }
              else
              {
                //style is 1 and type is 1
                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/dro.qoi", triangle->x2, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 35 - (((pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY) - 0.5, triangle->layer * 64 + 64, step, 34, 0, (((pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY));
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
              }
            }
          }
        }
        else if (triangle->style == 2)
        {
          if (triangle->type == 0)
          {
            int step = g_platformResolution;
            if (triangle->capped)
            {
              for (int i = 0; i < 55; i += step)
              {
                child = new mapObject(renderer, triangle->captexture, "resources/engine/ari.qoi", triangle->x2, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
              // diagonal shine
              step = g_TiltResolution;
              for (int i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 35 + ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, -((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) + 0);
                child->sortingOffset = -25;
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            step = g_TiltResolution;
            int vstep = 64;
            for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
            {
              for (float i = 0; i < 64; i += step)
              {
                child = new mapObject(renderer, triangle->walltexture, "&", triangle->x2 + i - 1.5, triangle->y1 + ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY), j, step, 32, 1, -((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY));
                child->sortingOffset = 15;
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            step = g_TiltResolution;
            if (triangle->shaded)
            {
              for (int i = 0; i < 64; i += step)
              {

                child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i, triangle->y1 + 30 + ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64, step, 50, 0, -((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) + 0);
                child->parent = triangle;
                triangle->children.push_back(child);

              }

              //This is to fill in a "gap" of shading between this corner and the block behind
              child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", triangle->x2, triangle->y1 + 55, 64 * triangle->layer, 19, 19, 0, 0);
              child->parent = triangle;
              triangle->children.push_back(child);
            }
          }
          else
          {
            if (triangle->type == 3)
            {
              int step = g_platformResolution;
              if (triangle->capped)
              {
                for (int i = 0; i < 55; i += step)
                {
                  child = new mapObject(renderer, triangle->captexture, "resources/engine/bri.qoi", triangle->x1 + 1, triangle->y1 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
                step = g_TiltResolution;
                for (int i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 35 - (((64 - step) - (64 - pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY) - 1, triangle->layer * 64 + 64, step, 55, 0, -(64 - pow(pow(64, 2) - pow(i, 2), 0.5)));
                  child->sortingOffset = -25;
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              int vstep = 64;
              for (int j = triangle->layer * 64; j < triangle->layer * 64 + 64; j += vstep)
              {
                for (float i = 0; i < 64; i += step)
                {
                  child = new mapObject(renderer, triangle->walltexture, "&", triangle->x1 + i, triangle->y1 + 55 - (((64 - step) - (64 - pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY) - 1, j, step, 32, 1, ((64 - i) * XtoY));
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }
              }

              step = g_TiltResolution;
              if (triangle->shaded)
              {
                for (int i = 0; i < 64; i += step)
                {

                  child = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 55 + 30 - (((64 - step) - (64 - pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY) - 1, 64 * triangle->layer, step, 50, 0, (((64 - step) - (64 - pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY));
                  child->parent = triangle;
                  triangle->children.push_back(child);
                }

                //This is to fill in a "gap" of shading between this corner and the block behind
                child = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", triangle->x2 - 19, triangle->y1 + 55, 64 * triangle->layer, 19, 19, 0, 0);
                child->parent = triangle;
                triangle->children.push_back(child);
              }
            }
            else
            {
              if (triangle->type == 2)
              {
                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/cri.qoi", triangle->x1 + 1, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i - 64, triangle->y1 + 35 - ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY));
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
                // child = new mapObject(renderer, triangle->captexture, "resources/engine/c.qoi", triangle->x1 + 1, triangle->y2 + 55 + 1, triangle->layer * 64 + 64, 64 + 1, 54 + 1, 0, 0, 0);
                // triangle->children.push_back(child);
              }
              else
              {

                if (triangle->capped)
                {
                  int step = g_platformResolution;
                  for (int i = 0; i < 55; i += step)
                  {
                    child = new mapObject(renderer, triangle->captexture, "resources/engine/dri.qoi", triangle->x2, triangle->y2 + i + step, triangle->layer * 64 + 64, 64 - 1, step, 0);
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                  step = g_TiltResolution;
                  for (int i = 0; i < 64; i += step)
                  {
                    child = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi", "&", triangle->x2 + i, triangle->y1 + 35 - ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) - 1, triangle->layer * 64 + 64, step, 34, 0, ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY));
                    child->parent = triangle;
                    triangle->children.push_back(child);
                  }
                }
              }
            }
          }
        }
      }
    }
    for (vector<ramp *> layer : g_ramps)
    {
      int tiltstep = g_TiltResolution;
      for (auto r : layer)
      {
        if (r->type == 0)
        {
          for (int i = 0; i < 64; i += tiltstep)
          {
            // make a strip of captex
            child = new mapObject(renderer, r->captexture, "&", r->x, r->y - ((float)i * 55.0 / 64.0) + 55, r->layer * 64 + i, 64, tiltstep + 2, 0, 0);
            child->parent = r;
            r->children.push_back(child);
          }
        }
        else
        {
          if (r->type == 1)
          {
            for (int i = 0; i < 64; i += tiltstep)
            {
              // make a strip of captex
              child = new mapObject(renderer, r->captexture, "&", r->x + i, r->y + 55, r->layer * 64 + i, tiltstep, 55, 0, 0);
              child->parent = r;
              r->children.push_back(child);
            }
            // wall
            child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
            child->parent = r;
            r->children.push_back(child);
          }
          else
          {
            if (r->type == 2)
            {
              for (int i = 0; i < 55; i += tiltstep)
              {
                // make a strip of captex
                child = new mapObject(renderer, r->captexture, "&", r->x, r->y - i + 55, r->layer * 64 + (64 - (i * (64 / 55))), 64, tiltstep, 0, 0);
                child->parent = r;
                r->children.push_back(child);
              }
              // wall
              child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
              child->parent = r;
              r->children.push_back(child);
            }
            else
            {
              for (int i = 0; i < 64; i += tiltstep)
              {
                // make a strip of captex
                child = new mapObject(renderer, r->captexture, "&", r->x + i, r->y + 55, r->layer * 64 + (64 - i), tiltstep, 55, 0, 0);
                child->parent = r;
                r->children.push_back(child);
              }
              child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
              child->parent = r;
              r->children.push_back(child);
            }
          }
        }
      }
    }
  }

  // now that map geometry is done, add guests for collisionZones
  for (auto x : g_collisionZones)
  {
    x->inviteAllGuests();
  }

  // put enemies in map
  if (!devMode)
  {
    populateMapWithEntities();
  }

  Update_NavNode_Costs(g_navNodes);

  // sort ui by priority
  sort_ui(g_ui);

  double LoadingTook = (std::clock() - debugClock) / (double)CLOCKS_PER_SEC;
  //I("Loading took " + to_string(LoadingTook) + "s");

  // map is loaded in, let's search for the proper waypoint

  if (g_waypoints.size() > 0)
  {
    protag->x = 101024;
    protag->y = 99962;
  }

  for (long long unsigned int i = 0; i < g_waypoints.size(); i++)
  {
    if (g_waypoints[i]->name == destWaypointName)
    {
      protag->x = g_waypoints[i]->x - protag->width / 2;
      protag->y = g_waypoints[i]->y + protag->bounds.height;
      protag->z = g_waypoints[i]->z;
      protag->steeringAngle = convertFrameToAngleNew(g_waypoints[i]->angle, 0);
      protag->animation = g_waypoints[i]->angle;
      D(g_waypoints[i]->angle);
      protag->flip = SDL_FLIP_NONE;
      if(protag->animation == 5) {
        protag->animation = 3;
        protag->flip = SDL_FLIP_HORIZONTAL;
      } else if(protag->animation == 6) {
        protag->animation = 2;
        protag->flip = SDL_FLIP_HORIZONTAL;
        M("Gotta flip the protag");
      } else if(protag->animation == 7) {
        protag->animation = 1;
        protag->flip = SDL_FLIP_HORIZONTAL;
      }
  
      if(protag->animation > 5 || protag->animation < 0) {
        protag->animation = 0;
      }

    }
  }

  g_camera.x = (g_focus->getOriginX() - (g_camera.width / (2 * g_camera.zoom)));
  g_camera.y = ((g_focus->getOriginY() - XtoZ * g_focus->z) - (g_camera.height / (2 * g_camera.zoom)));
  g_camera.oldx = g_camera.x;
  g_camera.oldy = g_camera.y;

  // call map's init-script
  // seemingly crashes the game sometimes
  // don't run the init-script if we're in devmode
  if (!devMode && fileExists("resources/maps/" + g_mapdir + "/scripts/INIT-" + g_map + ".txt"))
  {
    string loadstr = "resources/maps/" + g_mapdir + "/scripts/INIT-" + g_map + ".txt";
    vector<string> script = loadText(loadstr);

    parseScriptForLabels(script);

    narrarator->sayings = script;
    adventureUIManager->ownScript = script;
    adventureUIManager->talker = narrarator;
    adventureUIManager->blip = g_ui_voice;
    adventureUIManager->dialogue_index = -1;
    g_forceEndDialogue = 0;

    if (script.size() > 0)
    {
      adventureUIManager->continueDialogue();
    }
  }
  else
  {
    //initscript not found
  }

  g_maxPelletsInLevel = g_pellets.size();
  g_currentPelletsCollected = 0;

  //link together transport ents
  for(auto x : g_boardableEntities) {
    if(x->transportEnt != "0" || x->transportEnt != "") {
      x->transportEntPtr = searchEntities(x->transportEnt);
    }
  }

  g_loadingATM = 0;

  //reset cookies
  for(int i = 0; i < g_fogwidth; i++) {
    for(int j = 0; j < g_fogheight; j++) {
      g_fc[i][j] = 0;
      g_sc[i][j] = 0;
      g_fogcookies[i][j] = 0;
    }
  }
  transition = 0;
}

void changeTheme(string str)
{
  textureDirectory = str + "/";

  // update bg
  //  backgroundstr = str;

  // if(g_backgroundLoaded) {
  //     SDL_DestroyTexture(background);
  // }
  // SDL_Surface* bs = IMG_Load(("resources/static/backgrounds/" + backgroundstr + ".qoi").c_str());
  // background = SDL_CreateTextureFromSurface(renderer, bs);
  // g_backgroundLoaded = 1;
  // SDL_SetTextureColorMod(background, 255 * (1 - g_background_darkness), 255 * (1 - g_background_darkness), 255 * (1 - g_background_darkness));
  // SDL_FreeSurface(bs);

  // proceed

  // re-populate the array of textures that we rotate thru for creating floors, walls, and caps
  texstrs.clear();
  // check local folder first
  string path;
  if (str == "custom" || str == "map")
  {
    path = "resources/maps/" + g_mapdir + "/diffuse/";
  }
  else
  {
    path = "resources/static/diffuse/" + textureDirectory;
  }

  if (!filesystem::exists(path))
  {
    M("Theme " + path + "not found");
    return;
  }


  for (const auto &entry : filesystem::directory_iterator(path))
  {

    texstrs.push_back(entry.path().u8string());
  }
  floortexIndex = 0;
  captexIndex = -1;
  walltexIndex = 0;

  // search for "floor" and "wall" textures
  for (long long unsigned int i = 0; i < texstrs.size(); i++)
  {
    int pos = texstrs[i].find_last_of('/');
    string filename = texstrs[i].substr(pos + 1);
    if (filename == "floor.qoi")
    {
      floortexIndex = i;
    }
    if (filename == "wall.qoi")
    {
      walltexIndex = i;
    }
    if (filename == "cap.qoi")
    {
      captexIndex = i;
    }
  }

  if (captexIndex == -1)
  {
    captexIndex = walltexIndex;
  }

  floortex = texstrs[floortexIndex];
  delete floortexDisplay;
  floortexDisplay = new ui(renderer, floortex.c_str(), 0, 0, 0.1, 0.1, -100);
  walltex = texstrs[walltexIndex];
  delete walltexDisplay;
  walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0, 0.1, 0.1, -100);
  captex = texstrs[captexIndex];
  delete captexDisplay;
  captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0, 0.1, 0.1, -100);
}

//add support to save effectIndexes to maps
//that way we don't have to load textures in 3 ms
bool mapeditor_save_map(string word)
{
  
  //for safety
  if(word.find("..") != std::string::npos) {
    E("Tried to write a map outside of it's directory");
    return 0;
  }

  // add warning for file overright
  if ((word != g_map) && fileExists("resources/maps/" + g_mapdir + "/" + word + ".map"))
  {
    if (yesNoPrompt("Map \"" + word + "\" already exists, would you like to overwrite?") == 1)
    {
      M("Canceled overwrite");
      return 1;
    }
  }

  std::filesystem::create_directories("resources/maps/" + g_mapdir);
  string local_v_mapdir = word;
  word = "resources/maps/" + g_mapdir + "/" + word + ".map";
  M("Writing to this file");
  D(word);

  // make folders for custom assets
//  vector<string> dirs = {"ai", "attacks", "entities", "items", "music", "scripts", "sounds", "worldsounds", "sprites", "diffuse", "weapons", "masks", "heightmaps", "backgrounds", "ui", "effects"};
//  for (auto x : dirs)
//  {
//    std::filesystem::create_directories("resources/maps/" + g_mapdir + "/" + x);
//  }

  // make INIT.txt
//  if (!fileExists("resources/maps/" + g_mapdir + "/scripts/" + "INIT-" + local_v_mapdir + ".txt"))
//  {
//    ofstream file("resources/maps/" + g_mapdir + "/scripts/" + "INIT-" + local_v_mapdir + ".txt");
//
//    file.close();
//  }

  ofile.open(word);

  // write enemies and budget
  ofile << "budget " << g_budget << endl;
  for (std::map<string, int>::iterator it = enemiesMap.begin(); it != enemiesMap.end(); ++it)
  {
    ofile << "enemy " << it->first << " " << it->second << endl;
  }

  // write fow

  ofile << "dark " << g_fogofwarEnabled << endl;

  bool limitflag = 0;
  for (auto x : limits)
  {
    if (x != 0)
    {
      limitflag = 1;
    }
  }

  if (limitflag)
  {
    ofile << "limits";
    for (auto x : limits)
    {
      ofile << " " << x;
    }
    ofile << endl;
  }
  if (g_backgroundLoaded && backgroundstr != "")
  {
    ofile << "bg " << backgroundstr << endl;
  }

  for (auto x : g_ui)
  {
    if (x->mapSpecific && !x->persistent)
    {
      ofile << "ui " << x->filename << " " << x->x << " " << x->y << " " << x->width << " " << x->height << " " << x->priority << endl;
    }
  }
  for (int i = 0; i < g_layers; i++)
  {
    for (auto n : g_triangles[i])
    {
      ofile << "triangle " << n->x1 << " " << n->y1 << " " << n->x2 << " " << n->y2 << " " << i << " " << n->walltexture << " " << n->captexture << " " << n->capped << " " << n->shaded << " " << n->style << endl;
    }
  }
  for (int i = 0; i < g_layers; i++)
  {
    for (auto n : g_ramps[i])
    {
      ofile << "ramp " << n->x << " " << n->y << " " << i << " " << n->type << " " << n->walltexture << " " << n->captexture << endl;
    }
  }
  for (long long unsigned int i = 0; i < g_tiles.size(); i++)
  {
    // dont save map graphics
    if (g_tiles[i]->fileaddress.find("resources/engine") != string::npos && g_tiles[i]->fileaddress != "resources/engine/black-diffuse.qoi")
    {
      continue;
    }
    // sheared tiles are made on map loading, so dont save em
    if (g_tiles[i]->mask_fileaddress.find("resources/engine") != string::npos)
    {
      continue;
    }
    // lighting, but not occlusion is also generated on map load
    if (g_tiles[i]->fileaddress.find("lighting") != string::npos && !(g_tiles[i]->fileaddress.find("OCCLUSION") != string::npos))
    {
      continue;
    }

    ofile << "tile " << g_tiles[i]->fileaddress << " " << g_tiles[i]->mask_fileaddress << " " << to_string(g_tiles[i]->x) << " " << to_string(g_tiles[i]->y) << " " << to_string(g_tiles[i]->width) << " " << to_string(g_tiles[i]->height) << " " << g_tiles[i]->z << " " << g_tiles[i]->wraptexture << " " << g_tiles[i]->wall << " " << g_tiles[i]->dxoffset << " " << g_tiles[i]->dyoffset << endl;
  }
  for (long long unsigned int i = 0; i < g_heightmaps.size(); i++)
  {

    ofile << "heightmap " << g_heightmaps[i]->binding << " " << g_heightmaps[i]->name << " " << g_heightmaps[i]->magnitude << endl;
  }
  for (int j = 0; j < g_layers; j++)
  {
    for (long long unsigned int i = 0; i < g_boxs[j].size(); i++)
    {
      string shadestring = "";
      if (g_boxs[j][i]->shadeTop)
      {
        shadestring += "1";
      }
      else
      {
        shadestring += "0";
      }
      if (g_boxs[j][i]->shadeBot == 1)
      {
        shadestring += "1";
      }
      else
      {
        if (g_boxs[j][i]->shadeBot == 2)
        {
          shadestring += "2";
        }
        else
        {
          shadestring += "0";
        }
      }
      if (g_boxs[j][i]->shadeLeft)
      {
        shadestring += "1";
      }
      else
      {
        shadestring += "0";
      }
      if (g_boxs[j][i]->shadeRight)
      {
        shadestring += "1";
      }
      else
      {
        shadestring += "0";
      }
      ofile << "box " << to_string(g_boxs[j][i]->bounds.x) << " " << to_string(g_boxs[j][i]->bounds.y) << " " << to_string(g_boxs[j][i]->bounds.width) << " " << to_string(g_boxs[j][i]->bounds.height) << " " << j << " " << g_boxs[j][i]->walltexture << " " << g_boxs[j][i]->captexture << " " << g_boxs[j][i]->capped << " " << g_boxs[j][i]->shineTop << " " << g_boxs[j][i]->shineBot << " " << shadestring << endl;
    }
  }

  for (auto i : g_impliedSlopes) {
    ofile << "islope " << to_string(i->bounds.x) << " " << to_string(i->bounds.y) << " " << to_string(i->bounds.width) << " " << to_string(i->bounds.height) << " " << to_string(i->layer) << " " << to_string(i->shadeLeft) << " " << to_string(i->shadeRight) << " " << to_string(i->shadedAtAll) << endl;
  }

  for (auto i : g_impliedSlopeTris) {
    ofile << "islopet " << i->x1 << " " << i->y1 << " " << i->x2 << " " << i->y2 << " " << i->layer << " " << i->style << endl;
  }

  
  for (long long unsigned int i = 0; i < g_doors.size(); i++)
  {
    ofile << "door " << g_doors[i]->to_map << " " << g_doors[i]->to_point << " " << g_doors[i]->x << " " << g_doors[i]->y << " " << g_doors[i]->z << " " << g_doors[i]->width << " " << g_doors[i]->height << " " << g_doors[i]->zeight << endl;
  }
  for (long long unsigned int i = 0; i < g_dungeonDoors.size(); i++)
  {
    ofile << "ddoor " << g_dungeonDoors[i]->x << " " << g_dungeonDoors[i]->y << " " << g_dungeonDoors[i]->width << " " << g_dungeonDoors[i]->height << endl;
    M("Wrote a ddoor");
  }
  for (long long unsigned int i = 0; i < g_triggers.size(); i++)
  {
    ofile << "trigger " << g_triggers[i]->binding << " " << g_triggers[i]->x << " " << g_triggers[i]->y << " " << g_triggers[i]->z << " " << g_triggers[i]->width << " " << g_triggers[i]->height << " " << g_triggers[i]->zeight << " " << g_triggers[i]->targetEntity << endl;
  }
  for (long long unsigned int i = 0; i < g_worldsounds.size(); i++)
  {
    ofile << "worldsound " << g_worldsounds[i]->name << " " << g_worldsounds[i]->x << " " << g_worldsounds[i]->y << endl;
  }
  for (long long unsigned int i = 0; i < g_musicNodes.size(); i++)
  {
    ofile << "music " << g_musicNodes[i]->name << " " << g_musicNodes[i]->x << " " << g_musicNodes[i]->y << endl;
  }
  for (long long unsigned int i = 0; i < g_cueSounds.size(); i++)
  {
    ofile << "cuesound " << g_cueSounds[i]->name << " " << g_cueSounds[i]->x << " " << g_cueSounds[i]->y << " " << g_cueSounds[i]->radius << endl;
  }
  for (long long unsigned int i = 0; i < g_waypoints.size(); i++)
  {
    ofile << "waypoint " << g_waypoints[i]->name << " " << g_waypoints[i]->x << " " << g_waypoints[i]->y << " " << g_waypoints[i]->z << " " << g_waypoints[i]->angle << endl;
  }

  for (auto x : g_setsOfInterest)
  {
    for (auto y : x)
    {
      ofile << "poi " << y->x << " " << y->y << " " << y->index << endl;
    }
  }

  for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
  {
    ofile << "navNode " << g_navNodes[i]->x << " " << g_navNodes[i]->y << " " << g_navNodes[i]->z << endl;
  }
  for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
  {
    ofile << "navNodeEdge " << i << " ";
    for (long long unsigned int j = 0; j < g_navNodes[i]->friends.size(); j++)
    {
      auto itr = find(g_navNodes.begin(), g_navNodes.end(), g_navNodes[i]->friends[j]);
      ofile << std::distance(g_navNodes.begin(), itr) << " ";
    }
    ofile << endl;
  }

  for (long long unsigned int i = 0; i < g_entities.size(); i++)
  {
    if (!g_entities[i]->inParty && !g_entities[i]->persistentHidden && !g_entities[i]->persistentGeneral && !g_entities[i]->isOrbital && !g_entities[i]->dontSave)
    {
      if (g_entities[i]->isWorlditem)
      {
        ofile << "item " << g_entities[i]->name.substr(5) << " " << to_string(g_entities[i]->x) << " " << to_string(g_entities[i]->y) << " " << to_string(g_entities[i]->z) << endl;
      }
      else
      {
        //some ents have one frame but i still wanna set angle
        if(g_entities[i]->yframes == 1) {
          g_entities[i]->animation = convertAngleToFrame(g_entities[i]->steeringAngle);
        }
        if(g_entities[i]->name == "common/pellet") {
          g_entities[i]->animation = rand() % g_entities[i]->yframes;
        }

        ofile << "entity " << g_entities[i]->name << " " << to_string(g_entities[i]->x) << " " << to_string(g_entities[i]->y) << " " << to_string(g_entities[i]->z) << " " << g_entities[i]->animation << " " << (g_entities[i]->flip == SDL_FLIP_HORIZONTAL) << endl;
        D(g_entities[i]->animation);
      }
    }
  }

  for (long long unsigned int i = 0; i < g_listeners.size(); i++)
  {
    ofile << "listener " << g_listeners[i]->entityName << " " << g_listeners[i]->block << " " << g_listeners[i]->condition << " " << g_listeners[i]->binding << " " << g_listeners[i]->x << " " << g_listeners[i]->y << endl;
  }
  for (auto x : g_collisionZones)
  {
    ofile << "collisionZone " << x->bounds.x << " " << x->bounds.y << " " << x->bounds.width << " " << x->bounds.height << endl;
  }

  D("resources/maps/" + g_mapdir + "/scripts/INIT-" + g_map + ".txt");
  // run script on load

  if (narrarator->myScriptCaller == nullptr)
  {
    narrarator->myScriptCaller = new adventureUI(renderer, 1);
    narrarator->myScriptCaller->talker = narrarator;

  }

  M("Done with recent code");

  ofile.close();
  return 0;
}

// called on init if map_editing is true
void init_map_writing(SDL_Renderer *renderer)
{
  selection = new tile(renderer, "resources/engine/marker.qoi", "&", 0, 0, 0, 0, 1, 1, 1, 0, 0);
  marker = new tile(renderer, "resources/engine/marker.qoi", "&", 0, 0, 0, 0, 2, 0, 0, 0, 0);
  markerz = new tile(renderer, "resources/engine/marker.qoi", "&", 0, 0, 0, 0, 2, 0, 0, 0, 0);
  worldsoundIcon = new tile(renderer, "resources/engine/speaker.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  listenerIcon = new tile(renderer, "resources/engine/listener.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  navNodeIconBlue = new tile(renderer, "resources/engine/walkerBlue.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  navNodeIconRed = new tile(renderer, "resources/engine/walkerRed.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  navNodeIconYellow = new tile(renderer, "resources/engine/walkerYellow.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  musicIcon = new tile(renderer, "resources/engine/music.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  cueIcon = new tile(renderer, "resources/engine/cue.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  waypointIcon = new tile(renderer, "resources/engine/waypoint.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  poiIcon = new tile(renderer, "resources/engine/poi.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  doorIcon = new tile(renderer, "resources/engine/door.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  ddoorIcon = new tile(renderer, "resources/engine/ddoor.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);
  triggerIcon = new tile(renderer, "resources/engine/trigger.qoi", "&", 0, 0, 0, 0, 1, 0, 0, 0, 0);

  selection->software = 1;
  marker->software = 1;
  markerz->software = 1;
  worldsoundIcon->software = 1;
  listenerIcon->software = 1;
  navNodeIconBlue->software = 1;
  navNodeIconRed->software = 1;
  navNodeIconYellow->software = 1;
  musicIcon->software = 1;
  cueIcon->software = 1;
  waypointIcon->software = 1;
  poiIcon->software = 1;
  doorIcon->software = 1;
  ddoorIcon->software = 1;
  triggerIcon->software = 1;

  //i thought i was leaking data but its okay since all tiles are deleted in clear_map();

  floortexDisplay = new ui(renderer, floortex.c_str(), 0.0, 0, 0.1, 0.1, -100);
  walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0, 0.1, 0.1, -100);
  captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0, 0.1, 0.1, -100);

  texstrs.clear();
  string path = "resources/static/diffuse/" + textureDirectory;
  if (fileExists(path + "/floor.qoi"))
  {
    for (const auto &entry : filesystem::directory_iterator(path))
    {
      texstrs.push_back(entry.path().u8string());
    }
  }

  if (devMode)
  {
//    floortexDisplay->show = 1;
//    captexDisplay->show = 1;
//    walltexDisplay->show = 1;
  }
  else
  {
    floortexDisplay->show = 0;
    captexDisplay->show = 0;
    walltexDisplay->show = 0;
    // float scalex = ((float)WIN_WIDTH / 1920) * g_defaultZoom;
    // float scaley = scalex;
    SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
  }
}

// called every frame if map_editing is true
// each bool represents a button, if c is true, c was pressed that frame
void write_map(entity *mapent)
{
  float temp, px, py;

  if (g_mousemode)
  {
    int mxint, myint;
    SDL_GetMouseState(&mxint, &myint);
    float percentx = (float)mxint / (float)WIN_WIDTH;
    float percenty = (float)myint / (float)WIN_HEIGHT;
    temp = percentx / (scalex) * (g_camera.width * ((scalex)*0.2)) + (g_camera.x - 32);
    px = round(temp / grid) * grid;
    temp = percenty / (scalex) * (g_camera.height * ((scalex)*0.2)) + (g_camera.y - 26);
    py = round(temp / (float)round(grid * XtoY)) * (float)round(grid * XtoY);
  }
  else
  {
    temp = mapent->x + 32;
    px = round(temp / grid) * grid;
    temp = mapent->y - mapent->height - 38;
    py = round(temp / (float)round(grid * XtoY)) * (float)round(grid * XtoY);

    // needed for proper grid in negative x or y
    // it doesnt work anyway lol
    if (mapent->x < 0)
      px -= grid;

    if (mapent->y < mapent->height)
      py -= round(grid * XtoY);
  }

  // set marker
  marker->x = px;
  marker->y = py;
  marker->width = grid;
  marker->height = round(grid * XtoY);
  // some debug drawing
  SDL_FRect drect;
  if (drawhitboxes)
  {
    for (long long unsigned int i = 0; i < g_entities.size(); i++)
    {
      if (g_entities[i]->wallcap == 1)
      {
        continue;
      }
      if(g_entities[i]->isFogSlate) { continue; }
      drect.x = (g_entities[i]->bounds.x + g_entities[i]->x - g_camera.x) * g_camera.zoom;
      drect.y = (g_entities[i]->bounds.y + g_entities[i]->y - g_camera.y - g_entities[i]->z * XtoZ) * g_camera.zoom;
      drect.w = g_entities[i]->bounds.width * g_camera.zoom;
      drect.h = g_entities[i]->bounds.height * g_camera.zoom;

      SDL_SetRenderDrawColor(renderer, 80, 150, 0, 255);
      SDL_RenderDrawRectF(renderer, &drect);
    }

    for(auto h : g_hitboxes) {
      drect.x = h->x;
      drect.y = h->y;
      drect.w = h->bounds.width;
      drect.h = h->bounds.height;
      if(h->parent != 0) {
        drect.x += h->parent->getOriginX() - drect.w/2;
        drect.y += h->parent->getOriginY() - drect.h/2;
      }
      drect = transformRect(drect);
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      SDL_RenderDrawRectF(renderer, &drect);
    }

    int layer = 0;
    for (auto n : g_boxs)
    {
      for (long long unsigned int i = 0; i < n.size(); i++)
      {
        drect.x = (n[i]->bounds.x - g_camera.x) * g_camera.zoom;
        drect.y = (n[i]->bounds.y - (38 * layer) - g_camera.y) * g_camera.zoom;
        drect.w = n[i]->bounds.width * g_camera.zoom;
        drect.h = n[i]->bounds.height * g_camera.zoom;

        SDL_SetRenderDrawColor(renderer, 150 - layer * 38, 50, layer * 38, 255);
        SDL_RenderDrawRectF(renderer, &drect);
      }
      layer++;
    }

    for (auto n : g_impliedSlopes)
    {
      drect.x = (n->bounds.x - g_camera.x) * g_camera.zoom;
      drect.y = (n->bounds.y - g_camera.y) * g_camera.zoom;
      drect.w = n->bounds.width * g_camera.zoom;
      drect.h = n->bounds.height * g_camera.zoom;

      SDL_SetRenderDrawColor(renderer, 10, 200, 150, 255);
      SDL_RenderDrawRectF(renderer, &drect);
    }

    for (auto n : g_projectiles)
    {
      drect.x = (n->x + n->bounds.x - g_camera.x) * g_camera.zoom;
      drect.y = (n->y + n->bounds.y - (0.5 * n->z) - g_camera.y) * g_camera.zoom;
      drect.w = n->bounds.width * g_camera.zoom;
      drect.h = n->bounds.height * g_camera.zoom;

      SDL_SetRenderDrawColor(renderer, 255, 70, 30, 255);
      SDL_RenderDrawRectF(renderer, &drect);
    }
    SDL_SetRenderDrawColor(renderer, 150, 50, 0, 255);
    for (int i = 0; i < g_layers; i++)
    {
      for (auto n : g_triangles[i])
      {
        n->render(renderer);
      }
    }

    for (auto t : g_impliedSlopeTris)
    {
      t->render(renderer);
    }

    if(1) {
      // draw navNodes
      for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
      {
        rect cam(0, 0, g_camera.width, g_camera.height);
        rect obj = {(g_navNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_navNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
        if(!RectOverlap(obj,cam)) {continue;} //finally
//         SDL_Rect obj = {(g_navNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_navNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
//         SDL_RenderCopy(renderer, navNodeIconBlue->texture, NULL, &obj);
        //int redness = (g_navNodes[i]->costFromUsage / 10000) * 255;
        //SDL_SetRenderDrawColor(renderer, redness, 100, 150, 255);
        
        if(drawNavMesh) {
          //if(g_navNodes[i]->costFromUsage > 1000) {
          if(g_navNodes[i]->highlighted) {
             SDL_Rect obj = {(g_navNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_navNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
             SDL_RenderCopy(renderer, navNodeIconBlue->texture, NULL, &obj);
          } else {
             SDL_Rect obj = {(g_navNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_navNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
             SDL_RenderCopy(renderer, navNodeIconRed->texture, NULL, &obj);
  
          }
        }
        
        if(drawNavMeshEdges) {
          for (long long unsigned int j = 0; j < g_navNodes[i]->friends.size(); j++)
          {
            float x1 = (g_navNodes[i]->x - g_camera.x) * g_camera.zoom;
            float y1 = (g_navNodes[i]->y - g_camera.y - XtoZ * (g_navNodes[i]->z + 32)) * g_camera.zoom;
            float x2 = (g_navNodes[i]->friends[j]->x - g_camera.x) * g_camera.zoom;
            float y2 = (g_navNodes[i]->friends[j]->y - g_camera.y - XtoZ * (g_navNodes[i]->friends[j]->z + 32)) * g_camera.zoom;
            int colo = 0;
            if (g_navNodes[i]->z + g_navNodes[i]->friends[j]->z > 0)
            {
              colo = 255;
            }
            // dont draw disabled nodes. pretty janky
            if (g_navNodes[i]->enabled && g_navNodes[i]->friends[j]->enabled)
            {
              //SDL_SetRenderDrawColor(renderer, colo, 100, 150, 255);
              SDL_RenderDrawLineF(renderer, x1, y1, x2, y2);
            }
          }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      }
    }
  }
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  // draw rectangle to visualize the selection
  if (makingbox || makingtile || makingdoor)
  {
    // SDL_Rect dstrect = {rx, ry, abs(px-rx) + grid, abs(py-ry) + grid};

    if (lx < px)
    {
      selection->x = lx;
    }
    else
    {
      selection->x = px;
    }
    if (ly < py)
    {
      selection->y = ly;
    }
    else
    {
      selection->y = py;
    }

    selection->width = abs(lx - px) + grid;
    selection->height = abs(ly - py) + (round(grid * XtoY));
    if (tiling && !makingbox && !makingdoor)
    {
      selection->xoffset = (int)selection->x % int(selection->texwidth);
      selection->yoffset = (int)selection->y % int(selection->texheight);
      selection->wraptexture = 1;
    }
    else
    {
      // so that the red boundaries of making a box appear properly
      selection->xoffset = 0;
      selection->yoffset = 0;
    }
  }
  else
  {
    // hide it;
    selection->width = 0;
    selection->height = 0;
  }

  if (devinput[0] && !olddevinput[0] && !makingbox)
  {
    lx = px;
    ly = py;
    makingbox = 1;
    selection->texture = loadTexture(renderer, "resources/engine/trigger.qoi");
    selection->wraptexture = 0;
  }
  else
  {
    if (devinput[0] && !olddevinput[0] && makingbox)
    {
      makingbox = 0;
      trigger *t = new trigger("unset", selection->x, selection->y, wallstart, selection->width, selection->height, wallheight, "protag");
      t->targetEntity = "protag"; // protag by default
      // set to unactive so that if we walk into it, we dont crash
      // t->active = false;
    }
  }
  if (devinput[1] && !olddevinput[1])
  {
    // //entstring = "lifter";
    // M("spawned " + entstring);

    // //actually spawn the entity in the world
    // //entstring = "entities/" + entstring + ".ent";
    const char *plik = entstring.c_str();
    entity *e = new entity(renderer, plik);
    e->x = px + marker->width / 2 - (e->getOriginX());
    e->y = py + marker->height / 2 - (e->getOriginY());
    e->stop_hori();
    e->stop_verti();
    e->z = wallstart;
    e->shadow->x = e->x + e->shadow->xoffset;
    e->shadow->y = e->y + e->shadow->yoffset;
  }
  if (devinput[2] && !olddevinput[2] && makingtile == 0)
  {

    lx = px;
    ly = py;
    makingtile = 1;

    selection->texture = loadTexture(renderer, floortex);
    SDL_QueryTexture(selection->texture, NULL, NULL, &selection->texwidth, &selection->texheight);
  }
  else
  {
    if (devinput[2] && !olddevinput[2] && makingtile == 1)
    {
      if (makingtile)
      {
        // ofile << "tile " << walltex << " " << to_string(selection->x) << " " << to_string(selection->y) << " " << to_string(selection->width) << " " << to_string(selection->height) << " " << layer << " " << tiling << " 0 " <<  endl;

        makingtile = 0;
        const char *plik = floortex.c_str();
        tile *t = new tile(renderer, plik, masktex.c_str(), selection->x, selection->y, selection->width, selection->height, layer, tiling, 0, 0, 0);

        t->z = layer;
        t->wraptexture = tiling;

        // check for heightmap
        string heightmap_fileaddress = "resources/static/heightmaps/" + floortex.substr(17);
        D(heightmap_fileaddress);
        if (fileExists(heightmap_fileaddress))
        {
          // check if we already have a heightmap with the same bindings
          bool flag = 1;
          for (long long unsigned int i = 0; i < g_heightmaps.size(); i++)
          {
            if (g_heightmaps[i]->binding == heightmap_fileaddress)
            {
              flag = 0;
              break;
            }
          }
          if (flag)
          {
            heightmap *e = new heightmap(floortex, heightmap_fileaddress, 0.278); // this value corresponds to just over one block, or a comfortable slope up to one block.
            (void)e;
          }
        }
        M(g_heightmaps.size());
      }
    }
  }
  if (devinput[3] && !olddevinput[3] && makingbox == 0)
  {
    // mouse1
    // select entity first
    nudge = 0;
    for (auto n : g_entities)
    {
      if (n == protag)
      {
        continue;
      }
      if (RectOverlap(n->getMovedBounds(), marker->getMovedBounds()) && n->tangible)
      {
        protag = n;
        nudge = n;
        g_focus = n;
      }
    }
    if (nudge == 0)
    {
      lx = px;
      ly = py;
      makingbox = 1;
      selection->texture = loadTexture(renderer, "resources/engine/wall.qoi");
    }
  }
  else
  {
    if (devinput[3] && !olddevinput[3] && makingbox == 1)
    {
      if (makingbox)
      {

        // makes a tile and also a hitbox

        // if(makeboxs) {
        //     ofile << "box " << to_string(selection->x) << " " << to_string(selection->y) << " " << to_string(selection->width) << " " << to_string(selection->height) << endl;
        // }
        // make "cap" or the top of the wall

        makingbox = 0;
        box *c = 0;

        // spawn related objects
        // string loadstr = "resources/static/wall.qoi";
        if (makeboxs)
        {
          for (int i = wallstart / 64; i < wallheight / 64; i++)
          {
            bool fcap = (!(i + 1 < wallheight / 64)); //&& autoMakeWallcaps;
            // bool fcap = 1;
            c = new box(selection->x, selection->y, selection->width, selection->height, i, walltex, captex, fcap, 0, 0, "0000");
          }
        }
        // const char* plik = walltex.c_str();
        if (autoMakeWallcaps)
        {
          int step = g_platformResolution;
          for (int i = 0; i < selection->height; i += step)
          {
            mapObject *e = new mapObject(renderer, captex, "&", selection->x, selection->y + i + step, wallheight, selection->width, step, 0);
            c->children.push_back(e);
          }
        }

        if (autoMakeWalls)
        {
          int step = 64;
          for (int i = wallstart; i < wallheight; i += step)
          {
            mapObject *e = new mapObject(renderer, walltex, "&", selection->x, selection->y + selection->height, i, selection->width, 55, 1);
            c->children.push_back(e);
          }
          ////entity* e = new entity(renderer, selection->x, selection->y + selection->height, selection->width, wallheight, walltex, 1);
          // entity* e = new entity(renderer, walltex, selection->x, selection->y + selection->height, 0, selection->width, (wallheight) * XtoZ + 2, 1);
        }

        if (shine == 1)
        {
          // front
          // mapObject* f = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi",  "&", selection->x, selection->y + selection->height + marker->height/2,  wallheight + 1, selection->width, marker->height);

          // back
          // f = new mapObject(renderer, "resources/engine/SMOOTHSHADING.qoi",  "&", selection->x, selection->y + marker->height/2, wallheight + 1, selection->width, marker->height/2);
        }

        if (shine == 2)
        {
          mapObject *f = new mapObject(renderer, "resources/engine/SHARPSHADING.qoi", "&", selection->x, selection->y + selection->height - wallheight + marker->height / 2, 0, selection->width, marker->height);
          (void)f;
        }

        if (shine == 3)
        {
          mapObject *f = new mapObject(renderer, "resources/engine/SHARPBRIGHTSHADING.qoi", "&", selection->x, selection->y + selection->height - wallheight + marker->height / 2, 0, selection->width, marker->height);
          (void)f;
        }

        if (occlusion)
        {
          // front shading
          mapObject *e = new mapObject(renderer, "resources/engine/OCCLUSION.qoi", "&", selection->x, selection->y + selection->height + 19, 0, selection->width, marker->height);
          c->children.push_back(e);
          // left
          int step = g_platformResolution;
          for (int i = 0; i < selection->height; i += step)
          {
            mapObject *e = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", selection->x - 19, selection->y + i + g_platformResolution, 0, 55 / 2, step);
            c->children.push_back(e);
          }
          for (int i = 0; i < selection->height; i += step)
          {
            mapObject *e = new mapObject(renderer, "resources/engine/h-OCCLUSION.qoi", "&", selection->x + selection->width, selection->y + i + g_platformResolution, 0, 55 / 2, step);
            c->children.push_back(e);
          }

          // corner a
          e = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", selection->x - (38 - 19), selection->y, 0, 32, 19, 0, 0);
          c->children.push_back(e);
          // corner b
          e = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", selection->x + selection->width, selection->y, 0, 32, 19, 0, 0);
          c->children.push_back(e);
          // corner c
          e = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", selection->x - (38 - 19), selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0);
          c->children.push_back(e);
          // corner d
          e = new mapObject(renderer, "resources/engine/x-OCCLUSION.qoi", "&", selection->x + selection->width, selection->y + selection->height + (38 - 19), 0, 19, 19, 0, 0);
          c->children.push_back(e);
        }
      }
    }
  }

  // make seethru wall

  if (devinput[6] && !olddevinput[6] && makingbox == 0)
  {
    lx = px;
    ly = py;
    makingbox = 1;
    selection->texture = loadTexture(renderer, "resources/engine/invisiblewall.qoi");
  }
  else
  {
    if (devinput[6] && !olddevinput[6] && makingbox == 1)
    {
      if (makingbox)
      {

        makingbox = 0;
        impliedSlope *i = new impliedSlope(selection->x, selection->y, selection->width, selection->height, wallstart, 0, 0, 0);

      }
    }
  }

  if (devinput[34] && !olddevinput[34] && makingbox == 0)
  {
    lx = px;
    ly = py;
    makingbox = 1;
    selection->image = IMG_Load("resources/engine/invisiblewall.qoi");
    selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
    SDL_FreeSurface(selection->image);
  }
  else
  {
    if (devinput[34] && !olddevinput[34] && makingbox == 1)
    {
      if (makingbox)
      {

        makingbox = 0;
        impliedSlope *i = new impliedSlope(selection->x, selection->y, selection->width, selection->height, wallstart, 0, 0, 1);

      }
    }
  }
  
  if(devinput[35] && !olddevinput[35]) { //.:
    int minimize = 0; //pixels to shrink this triangle
    impliedSlopeTri* n = new impliedSlopeTri(marker->x, marker->y + marker->height + minimize, marker->x + marker->width, marker->y + minimize, wallstart, 0);
    
  }

  if(devinput[36] && !olddevinput[36]) { //:.
    int minimize = 0;
    impliedSlopeTri* n = new impliedSlopeTri(marker->x + marker->width, marker->y + marker->height + minimize, marker->x, marker->y + minimize, wallstart, 0);
  }


  if (devinput[20] && !olddevinput[20] && makingbox == 0)
  {
    lx = px;
    ly = py;
    makingbox = 1;
    selection->image = IMG_Load("resources/engine/navmesh.qoi");
    selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
    SDL_FreeSurface(selection->image);
  }
  else
  {
    if (devinput[20] && !olddevinput[20] && makingbox == 1)
    {
      if (makingbox)
      {
        for (int i = selection->x; i < selection->width + selection->x; i += 64 * navMeshDensity)
        {
          for (int j = selection->y; j < selection->height + selection->y; j += (64 * XtoY) * navMeshDensity)
          {
            // lets do a raycast
            int zres = verticalRayCast(i + marker->width / 2, j + marker->height / 2);

            // if this node would be higher than desired, don't make it
            if (zres > markerz->z)
            {
              continue;
            }

            // D(zres);
            navNode *n = new navNode(i + marker->width / 2, j + marker->height / 2, zres);
            (void)n;
          }
        }
        // delete nodes too close to walls
        // int checkinglayer = 0;
        float cullingdiameter = mapeditorNavNodeCullRadius;
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          rect noderect = {(int)(g_navNodes[i]->x - (cullingdiameter / 2)), (int)(g_navNodes[i]->y - (cullingdiameter / 2)), (int)(cullingdiameter), (int)(cullingdiameter * XtoY)};
          bool breakflag = 0;
          for (long long unsigned int j = 0; j < g_boxs[layer].size(); j++)
          {
            //    M(g_boxs[layer][j]->bounds.x);
            //    M(noderect.x);
            //    M( (layer ) * 64);
            //    M(g_navNodes[i]->x);
            //    M(g_navNodes[i]->z);
            if (RectOverlap(g_boxs[layer][j]->bounds, noderect))
            {
              delete g_navNodes[i];
              i--;
              breakflag = 1;
              break;
            }
          }
          if(breakflag) {continue;}
          //do the same for implied geometry
          for (long long unsigned int j = 0; j < g_impliedSlopes.size(); j++) {
            if(RectOverlap(noderect, g_impliedSlopes[j]->bounds)) {
              delete g_navNodes[i];
              i--;
              break;
            }
          }
          
          // break;//temp
        }
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          for (long long unsigned int j = 0; j < g_navNodes.size(); j++)
          {
            if (i == j)
            {
              continue;
            }

            float gwt = max(g_navNodes[i]->z, g_navNodes[j]->z);
            gwt /= 64;

            if (XYDistance(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y) < 181+1 /*&& (LineTrace(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y, 0, mapeditorNavNodeTraceRadius, gwt))*/ && (abs(g_navNodes[i]->z - g_navNodes[j]->z) < 40))
            {

              // dont add a friend we already have
              bool flag = 1;
              for (auto x : g_navNodes[i]->friends)
              {
                if (x == g_navNodes[j])
                {
                  flag = 0;
                }
              }
              if (flag)
              {
                g_navNodes[i]->Add_Friend(g_navNodes[j]);
              }
            }
          }
        }

        // delete nodes with no friends
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          if (g_navNodes[i]->friends.size() == 0)
          {
            delete g_navNodes[i];
            // break; //temp
            i--;
          }
        }

        // delete friends which dont exist anymore
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          for (long long unsigned int j = 0; j < g_navNodes[i]->friends.size(); j++)
          {
            auto itr = find(g_navNodes.begin(), g_navNodes.end(), g_navNodes[i]->friends[j]);

            if (itr == g_navNodes.end())
            {
              // friend has been deleted, remove as well from friend array
              g_navNodes[i]->friends.erase(remove(g_navNodes[i]->friends.begin(), g_navNodes[i]->friends.end(), g_navNodes[i]->friends[j]), g_navNodes[i]->friends.end());
            }
          }
        }

        Update_NavNode_Costs(g_navNodes);
        makingbox = 0;
      }
    }
  }
  if (devinput[21] && !olddevinput[21] && makingbox == 0)
  {
    // make a single navnode
    new navNode(marker->x + 0.5 * marker->width, marker->y + 0.5 * marker->height, wallstart);
  }

  if (devinput[4] && !olddevinput[4])
  { //search terms: delete button, delete functionality, manual delete
    if (nudge != 0)
    {
      nudge = 0;
    }
    else
    {
      if (makingbox || makingtile || makingdoor)
      {
        makingbox = 0;
        makingtile = 0;
        makingdoor = 0;
      }
      else
      {

        //delete functionality
        //delete objects
        //delete feature
        //map editor delete
        //map delete
        //dev delete
        //right click delete
        //delete lists
        bool deleteflag = 1;
        for (auto n : g_entities)
        {
          if (n == protag)
          {
            continue;
          }
          if (n->persistentHidden)
          {
            continue;
          }
          if (n->persistentGeneral)
          {
            continue;
          }
          if (RectOverlap(n->getMovedBounds(), marker->getMovedBounds()) && !n->inParty && n->tangible && n != protag)
          {
            delete n;
            if(n == g_objective) { g_objective = nullptr;}
            if(n == g_behemoth0) { g_behemoth0 = nullptr;}
            if(n == g_behemoth1) { g_behemoth1 = nullptr;}
            if(n == g_behemoth2) { g_behemoth2 = nullptr;}
            if(n == g_behemoth3) { g_behemoth3 = nullptr;}
            deleteflag = 0;
            break;
          }
        }

        vector<tri *> deleteTris;
        vector<box *> deleteRects;
        vector<ramp *> deleteRamps;
        vector<trigger *> deleteTriggers;
        vector<door *> deleteDoors;
        vector<dungeonDoor*> deleteDDoors;
        vector<pointOfInterest *> deletePois;
        vector<impliedSlopeTri*> deleteIST;
        vector<impliedSlope*> deleteIS;
        rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};

        for(auto x : g_setsOfInterest) {
          for(auto n : x) {
            rect poirect = {n->x, n->y, 10, 10};
            if( RectOverlap(markerrect, poirect) ) {
              deletePois.push_back(n);
              deleteflag = 0;
            }
          }
        }

        //delete IS
        for (auto n : g_impliedSlopes)
        {
          if (RectOverlap(markerrect, n->bounds))
          {
            deleteIS.push_back(n);
            deleteflag = 0;
          }
        }
        
        for (auto n : g_impliedSlopeTris)
        {
          if (ITriRectOverlap(n, markerrect.x, markerrect.y, markerrect.width, markerrect.height))
          {
            deleteIST.push_back(n);
            deleteflag = 0;
          }
        }
        

        // delete block at position
        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
        {
          for (auto n : g_boxs[i])
          {
            if (RectOverlap(markerrect, n->bounds))
            {
              deleteRects.push_back(n);
              deleteflag = 0;
            }
          }
        }
        for (long long unsigned int i = 0; i < g_triangles.size(); i++)
        {
          for (auto n : g_triangles[i])
          {
            if (TriRectOverlap(n, marker->x + 6, marker->y + 6, marker->width - 12, marker->height - 12))
            {
              deleteTris.push_back(n);
              deleteflag = 0;
            }
          }
        }
        for (long long unsigned int i = 0; i < g_ramps.size(); i++)
        {
          for (auto n : g_ramps[i])
          {
            rect a = rect(n->x, n->y, 64, 55);
            if (RectOverlap(markerrect, a))
            {
              deleteRamps.push_back(n);
              deleteflag = 0;
            }
          }
        }

        // only delete tiles if we haven't already deleted something else

        for (auto n : deleteTris)
        {
          for (auto child : n->children)
          {
            delete child;
          }
          delete n;
          deleteflag = 0;
        }
        for (auto n : deleteRects)
        {
          for (auto child : n->children)
          {
            delete child;
          }
          delete n;
          deleteflag = 0;
        }
        for (auto n : deleteRamps)
        {
          for (auto child : n->children)
          {
            delete child;
          }
          delete n;
          deleteflag = 0;
        }

        // delete navmesh
        if (drawhitboxes)
        {
          for (auto x : g_navNodes)
          {
            rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
            rect b = {x->x - 10, x->y - 10, 20, 20};

            if (RectOverlap(markerrect, b))
            {
              RecursiveNavNodeDelete(x);
              deleteflag = 0;
              break;
            }
          }
        }
        for (auto x : g_triggers)
        {
          rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
          rect b = {x->x, x->y, x->width, x->height};
          if (RectOverlap(markerrect, b))
          {
            deleteTriggers.push_back(x);
          }
        }

        for (auto x : g_doors)
        {
          rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
          rect b = {(int)x->x, (int)x->y, (int)x->width, (int)x->height};
          if (RectOverlap(markerrect, b))
          {
            deleteDoors.push_back(x);
          }
        }

        for (auto x : g_dungeonDoors)
        {
          rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
          rect b = {(int)x->x, (int)x->y, (int)x->width, (int)x->height};
          if (RectOverlap(markerrect, b))
          {
            deleteDDoors.push_back(x);
          }
        }

        // delete stuff in deleteTriggers and deleteDoors
        for (auto x : deleteTriggers)
        {
          delete x;
        }

        for (auto x : deleteDoors)
        {
          delete x;
        }

        for (auto x : deleteDDoors)
        {
          delete x;
        }

        for (auto x : deleteIST) 
        {
          delete x;
        }

        for (auto x : deletePois)
        {
          delete x;
        }

        for (auto x : deleteIS) 
        {
          delete x;
        }

        if (wallheight == 64 && deleteflag)
        {
          // musicnodes
          for (auto x : g_musicNodes)
          {
            rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
            rect b = {(int)x->x, (int)x->y, 60, 40};
            if (RectOverlap(markerrect, b))
            {
              delete x;
              // !!! Technically this is unsafe, the data should be handled better
            }
          }
          for (auto x : g_cueSounds)
          {
            rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
            rect b = {(int)x->x, (int)x->y, 60, 40};
            if (RectOverlap(markerrect, b))
            {
              delete x;
              // !!! Technically this is unsafe, the data should be handled better
            }
          }
          for (auto x : g_worldsounds)
          {
            rect markerrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
            rect b = {(int)x->x, (int)x->y, 60, 40};
            if (RectOverlap(markerrect, b))
            {
              delete x;
              // !!! Technically this is unsafe, the data should be handled better
            }
          }

          // rect markerrect = {marker->x, marker->y, marker->width, marker->height};
          for (long long unsigned int i = 0; i < g_tiles.size(); i++)
          {
            // M(g_tiles[i]->width);

            if (g_tiles[i]->software == 1)
            {
              continue;
            }
            if (RectOverlap(g_tiles[i]->getMovedBounds(), markerrect))
            {
              delete g_tiles[i];
              break;
            }
          }
        }
      }
    }
  }
 
  if (devinput[5] && !olddevinput[5] && makingdoor == 0)
  {
    lx = px;
    ly = py;
    makingdoor = 1;
    selection->image = IMG_Load("resources/engine/door.qoi");
    selection->wraptexture = 0;
    selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
    SDL_FreeSurface(selection->image);
  }
  else
  {
    if (devinput[5] && !olddevinput[5] && makingdoor == 1)
    {

      makingdoor = 0;

      door *d = new door(renderer, "&", "A", selection->x, selection->y, wallstart, selection->width, selection->height, wallheight);
      (void)d;
    }
  }

  if (devinput[7] && !olddevinput[7])
  {
    // toggle hitboxes
    drawhitboxes = !drawhitboxes;
  }

  if (devinput[8] && !olddevinput[8])
  {
    // grow grid
    //  grid *= 2;
    //  M("Grid raised to " + (int)grid);
  }

  if (devinput[9] && !olddevinput[9] && makingdoor == 0)
  {
    lx = px;
    ly = py;
    makingdoor = 1;
    selection->image = IMG_Load("resources/engine/ddoor.qoi");
    selection->wraptexture = 0;
    selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
    SDL_FreeSurface(selection->image);
  }
  else
  {
    if (devinput[9] && !olddevinput[9] && makingdoor == 1)
    {
      makingdoor = 0;

      dungeonDoor *d = new dungeonDoor(selection->x, selection->y, selection->width, selection->height);
      (void)d;
    }
  }

  if (devinput[10] && !olddevinput[10])
  {
    if (g_holdingCTRL)
    {
      // pop this triangle out
      // check for triangles at mouse
      vector<tri *> deleteMe;
      // rect markerrect = {marker->x, marker->y, marker->width, marker->height };
      for (int i = wallstart / 64; i < wallheight / 64; i++)
      {
        for (auto n : g_triangles[i])
        {
          if (TriRectOverlap(n, marker->x + 6, marker->y + 6, marker->width - 12, marker->height - 12))
          {
            deleteMe.push_back(n);
          }
        }
      }
      if (deleteMe.size() != 0)
      {
        int type = deleteMe[0]->type;
        int style = deleteMe[0]->style;
        for (auto n : deleteMe)
        {
          for (auto child : n->children)
          {
            delete child;
          }
          delete n;
        }
        // make a new triangle with a different type and current wallheight
        if (style == 0)
        {
          switch (type)
          {
            case 0:
              devinput[26] = 1;
              break;
            case 1:
              devinput[29] = 1;
              break;
            case 2:
              devinput[28] = 1;
              break;
            case 3:
              devinput[27] = 1;
              break;
          }
        }
        if (style == 1)
        {
          switch (type)
          {
            case 0:
              devinput[30] = 1;
              break;
            case 1:
              devinput[33] = 1;
              break;
            case 2:
              devinput[32] = 1;
              break;
            case 3:
              devinput[31] = 1;
              break;
          }
        }
        if (style == 2)
        {
          switch (type)
          {
            case 0:
              devinput[12] = 1;
              break;
            case 1:
              devinput[15] = 1;
              break;
            case 2:
              devinput[14] = 1;
              break;
            case 3:
              devinput[13] = 1;
              break;
          }
        }
      }
    }
    else
    {
      // check for triangles at mouse
      vector<tri *> deleteMe;
      // rect markerrect = {marker->x, marker->y, marker->width, marker->height };
      for (int i = wallstart / 64; i < wallheight / 64; i++)
      {
        for (auto n : g_triangles[i])
        {
          if (TriRectOverlap(n, marker->x + 6, marker->y + 6, marker->width - 12, marker->height - 12))
          {
            deleteMe.push_back(n);
          }
        }
      }
      if (deleteMe.size() != 0)
      {
        int type = deleteMe[0]->type;
        int style = deleteMe[0]->style;
        for (auto n : deleteMe)
        {
          for (auto child : n->children)
          {
            delete child;
          }
          delete n;
        }
        // make a new triangle with a different type and current wallheight

        if (style == 0)
        {
          switch (type)
          {
            case 0:
              devinput[15] = 1;
              break;
            case 1:
              devinput[14] = 1;
              break;
            case 2:
              devinput[13] = 1;
              break;
            case 3:
              devinput[12] = 1;
              break;
          }
        }
        if (style == 1)
        {
          switch (type)
          {
            case 0:
              devinput[29] = 1;
              break;
            case 1:
              devinput[28] = 1;
              break;
            case 2:
              devinput[27] = 1;
              break;
            case 3:
              devinput[26] = 1;
              break;
          }
        }
        if (style == 2)
        {
          switch (type)
          {
            case 0:
              devinput[33] = 1;
              break;
            case 1:
              devinput[32] = 1;
              break;
            case 2:
              devinput[31] = 1;
              break;
            case 3:
              devinput[30] = 1;
              break;
          }
        }
      }
      else
      {
        // we looked and didnt find anything, lets make one
        devinput[12] = 1;
      }
    }
  }

  //if holding ctrl, move protag around (for moving entities that normally don't move)
  if(keystate[bindings[0]] && g_holdingCTRL) {
    protag->up = 0;
    protag->down = 0;
    protag->left = 0;
    protag->right = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
    protag->y -= protag->xmaxspeed/20;
  }

  if(keystate[bindings[1]] && g_holdingCTRL) {
    protag->up = 0;
    protag->down = 0;
    protag->left = 0;
    protag->right = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
    protag->y += protag->xmaxspeed/20;
  }

  if(keystate[bindings[2]] && g_holdingCTRL) {
    protag->up = 0;
    protag->down = 0;
    protag->left = 0;
    protag->right = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
    protag->x -= protag->xmaxspeed/20;
  }

  if(keystate[bindings[3]] && g_holdingCTRL) {
    protag->up = 0;
    protag->down = 0;
    protag->left = 0;
    protag->right = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
    protag->x += protag->xmaxspeed/20;
  }


  if(keystate[bindings[0]] && g_holdingTAB) {
    protag->up = 1;
    protag->down = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
  }

  if(keystate[bindings[1]] && g_holdingTAB) {
    protag->up = 0;
    protag->down = 1;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
  }

  if(keystate[bindings[2]] && g_holdingTAB) {
    protag->left = 1;
    protag->right = 0;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
  }

  if(keystate[bindings[3]] && g_holdingTAB) {
    protag->left = 0;
    protag->right = 1;
    protag->xvel = 0;
    protag->yvel = 0;
    protag->forwardsVelocity = 0;
  }

  if(g_holdingTAB) {
    if(protag->up) {
      if(protag->left) {
        protag->steeringAngle = 3*M_PI/4;
  
      } else if(protag->right) {
        protag->steeringAngle = M_PI/4;
  
      } else {
        protag->steeringAngle = M_PI/2;
  
      }
  
    } else if(protag->down) {
      if(protag->left) {
        protag->steeringAngle = 5 * M_PI/4;
  
      } else if(protag->right) {
        protag->steeringAngle = 7 * M_PI/4;
  
      } else {
        protag->steeringAngle = 3 * M_PI/2;
  
      }
   
    } else if(protag->left) {
      protag->steeringAngle = M_PI;
  
    } else if(protag->right) {
      protag->steeringAngle = 0;
    }


    protag->animation = convertAngleToFrame(protag->steeringAngle);
    protag->flip = SDL_FLIP_NONE;
    if(protag->yframes < 8) {
      if(protag->animation == 5) {
        protag->animation = 3;
        protag->flip = SDL_FLIP_HORIZONTAL;
      } else if(protag->animation == 6) {
        protag->animation = 2;
        protag->flip = SDL_FLIP_HORIZONTAL;
      } else if(protag->animation == 7) {
        protag->animation = 1;
        protag->flip = SDL_FLIP_HORIZONTAL;
      }
  
      if(protag->animation > 5 || protag->animation < 0) {
        protag->animation = 0;
      }

    }
  }

  if (devinput[11] && !olddevinput[11])
  {

    // set material
    // textbox(SDL_Renderer* renderer, const char* content, float size, int x, int y, int width) {
    textbox *consoleDisplay = new textbox(renderer, "console", g_fontsize * 30, 0, 0, 4000);
    string input;
    SDL_Event console_event;
    bool polling = 1;
    // bool keypressed = 0;
    // const Uint8* keystate = SDL_GetKeyboardState(NULL);

    // turn off VSYNC because otherwise we jitter between this frame and the last while typing
    // SDL_GL_SetSwapInterval(0);
    while (polling)
    {
      SDL_Delay(10);
      while (SDL_PollEvent(&console_event))
      {

        // update textbox
        string renderinput = ">" + input;
        SDL_Rect rect = {consoleDisplay->x, consoleDisplay->y, WIN_WIDTH, consoleDisplay->height};
        consoleDisplay->updateText(renderinput, -1, WIN_WIDTH);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &rect);
        consoleDisplay->render(renderer, 1, 1);
        SDL_RenderPresent(renderer);
        switch (console_event.type)
        {
          case SDL_QUIT:
            polling = 0;
            quit = 1;
            break;

          case SDL_KEYDOWN:
            string name = SDL_GetKeyName(console_event.key.keysym.sym);

            if (name == "Return")
            {
              polling = false;
              break;
            }
            if (name == "Space")
            {
              input += " ";
              break;
            }
            if (name == "Up")
            {
              consolehistoryindex--;
              if (consolehistoryindex > (int)consolehistory.size() - 1)
              {
                consolehistoryindex = consolehistory.size() - 1;
              }
              if (consolehistoryindex < 0)
              {
                consolehistoryindex = 0;
              }
              if (consolehistory.size())
              {
                input = consolehistory.at(consolehistoryindex);
              }
              break;
            }
            if (name == "Down")
            {
              consolehistoryindex++;
              if (consolehistoryindex > (int)consolehistory.size() - 1)
              {
                consolehistoryindex = consolehistory.size() - 1;
              }
              if (consolehistoryindex < 0)
              {
                consolehistoryindex = 0;
              }
              if (consolehistory.size())
              {
                input = consolehistory.at(consolehistoryindex);
              }
              break;
            }
            if (name == "Backspace")
            {
              if (input.length() > 0)
              {
                input = input.substr(0, input.size() - 1);
                break;
              }
            }

            // if the input wasnt a return or a space or a letter, dont take it at all
            if (name.length() != 1)
            {
              break;
            }
            input += name;

            break;
        }
      }
    }

    consolehistory.push_back(input);
    if (consolehistory.size() > 10)
    {
      consolehistory.erase(consolehistory.begin() + 9);
    }
    consolehistoryindex = 0;
    SDL_GL_SetSwapInterval(1);
    delete consoleDisplay;

    locale loc;
    transform(input.begin(), input.end(), input.begin(), [](unsigned char c)
        { return std::tolower(c); });
    istringstream line(input);
    string word = "";

    while (line >> word)
    {
      if (word == "sb")
      {
        drawhitboxes = !drawhitboxes;
        if(drawhitboxes) {
          floortexDisplay->show = 1;
          captexDisplay->show = 1;
          walltexDisplay->show = 1;

        } else {
          floortexDisplay->show = 0;
          captexDisplay->show = 0;
          walltexDisplay->show = 0;

        }
        break;
      }
      if (word == "nm")
      {
        drawNavMesh = !drawNavMesh;
        break;
      }
      if(word == "nme")
      { 
        drawNavMeshEdges = !drawNavMeshEdges;
        break;
      }
      if (word == "show")
      {
        line >> word;
        if (word == "hitboxes")
        {
          drawhitboxes = !drawhitboxes;
          break;
        }
        if (word == "navmesh")
        {
          drawhitboxes = !drawhitboxes;
          break;
        }
        if (word == "marker") {

          showMarker = !showMarker;
          if (showMarker)
          {
            SDL_SetTextureAlphaMod(marker->texture, 255);
          }
          else
          {
            SDL_SetTextureAlphaMod(marker->texture, 0);
          }
          break;
        }
      }
      if (word == "info")
      {
        M(mapname);

        M("Entity Count: ");
        M(g_entities.size());
        M("MapObject Count:");
        M(g_mapObjects.size());
        M("Floor Count: ");
        M(g_tiles.size() - 9);
        // M("Number of Shadows: ");
        // M(g_shadows.size());
        M("Waypoint Count: ");
        M(g_waypoints.size());
        M("Door Count: ");
        M(g_doors.size());
        D(mapent->x);
        D(mapent->y);
        D(g_camera.x);
        D(g_camera.y);
        D(g_focus->x);
        D(g_focus->y);
        // int hosting = 0;
        // int sharing = 0;
        // for (auto x : g_entities) {
        //     if(x->wallcap) {
        //         if(x->asset_sharer) {
        //             sharing++;
        //         } else {
        //             hosting++;
        //         }
        //     }
        // }
        // D(hosting);
        // for (auto x : g_entities) {
        //     if(x->wallcap) {
        //         if(x->asset_sharer) {

        //         } else {
        //             M(x->name);
        //         }
        //     }
        // }
        // D(sharing);
        // for (auto x : g_entities) {
        //     if(x->wallcap) {
        //         if(x->asset_sharer) {
        //             M(x->name);
        //         } else {

        //         }
        //     }
        // }
        // hosting = 0;
        // sharing = 0;
        // for (auto x : g_tiles) {

        //     if(x->asset_sharer) {
        //         sharing++;
        //     } else {
        //         hosting++;
        //     }

        // }
        // D(hosting);
        // for (auto x : g_tiles) {
        //     if(x->asset_sharer) {

        //     } else {
        //         M(x->fileaddress);
        //     }
        // }
        // D(sharing);
        // for (auto x : g_tiles) {
        //     if(x->asset_sharer) {
        //         M(x->fileaddress);
        //     } else {

        //     }
        // }

        // how many textures are loaded
      }
      if (word == "undo")
      {
        // delete last collision
        delete g_mapCollisions[g_mapCollisions.size() - 1];
        // segfaults on save smt
      }

      if (word == "easybake" || word == "eb")
      {
        I("starting EB-operation");
        // split the box collisions into grid-sized blocks
        // copy array
        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
        {
          vector<box *> tempboxes;
          for (auto b : g_boxs[i])
          {
            tempboxes.push_back(b);
          }
          for (auto b : tempboxes)
          {
            int x = b->bounds.x;
            int y = b->bounds.y;
            while (x < b->bounds.width + b->bounds.x)
            {
              while (y < b->bounds.height + b->bounds.y)
              {
                new box(x, y, 64, 55, b->layer, b->walltexture, b->captexture, b->capped, 0, 0, "0000");
                y += 55;
              }
              x += 64;
              y = b->bounds.y;
            }

            for (auto child : b->children)
            {
              delete child;
            }

            delete b;
          }
        }

        ///////////////////
        I("Halfway through EB-operation");
        // set capped
        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
        {
          for (auto b : g_boxs[i])
          {
            rect inneighbor = {b->bounds.x + 2, b->bounds.y + 2, b->bounds.width - 4, b->bounds.height - 4};
            // set capped for each box
            if ((int)i == g_layers)
            {
              continue;
            }
            for (auto n : g_boxs[i + 1])
            {
              // don't calculate lighting by invisible walls
              if (n->walltexture == "resources/engine/seethru.qoi")
              {
                continue;
              }
              if (RectOverlap(inneighbor, n->bounds))
              {
                b->capped = false;
              }
            }
          }
        }
        for (long long unsigned int i = 0; i < g_triangles.size(); i++)
        {
          for (auto b : g_triangles[i])
          {
            // set capped for each box
            if ((int)i == g_layers)
            {
              continue;
            }
            rect a = {((b->x1 + b->x2) / 2) - 4, ((b->y1 + b->y2) / 2) - 4, 8, 8};
            for (auto n : g_triangles[i + 1])
            {
              // don't calculate lighting by invisible walls
              if (n->walltexture == "resources/engine/seethru.qoi")
              {
                continue;
              }
              if (TriRectOverlap(n, a))
              {
                b->capped = false;
              }
            }
          }
        }
        // get lighting data to mapcollisions
        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
        {

          for (auto b : g_boxs[i])
          {
            // shade
            b->shadeTop = false;
            b->shadeBot = false;
            b->shadeLeft = false;
            b->shadeRight = false;
            rect uneighbor = {b->bounds.x + 2, b->bounds.y - 55 + 2, b->bounds.width - 4, 55 - 4};
            rect dneighbor = {b->bounds.x + 2, b->bounds.y + 2 + b->bounds.height, b->bounds.width - 4, 55 - 4};
            b->shineTop = true;
            b->shineBot = true;

            // don't calculate lighting for invisible walls
            if (b->walltexture == "resources/engine/seethru.qoi")
            {
              b->shineTop = 0;
              b->shineBot = 0;
              continue;
            }

            // check for overlap with all other boxes
            for (auto n : g_boxs[i])
            {
              // don't calculate lighting by invisible walls
              if (n->walltexture == "resources/engine/seethru.qoi")
              {
                continue;
              }
              if (n == b)
              {
                continue;
              }

              if (RectOverlap(n->bounds, uneighbor))
              {
                b->shineTop = false;
              }
              if (RectOverlap(n->bounds, dneighbor))
              {
                b->shineBot = false;
              }
            }
            for (auto n : g_triangles[i])
            {
              if (TriRectOverlap(n, uneighbor.x, uneighbor.y, uneighbor.width, uneighbor.height))
              {
                b->shineTop = false;
              }
              if (TriRectOverlap(n, dneighbor.x, dneighbor.y, dneighbor.width, dneighbor.height))
              {
                b->shineBot = false;
                b->shadeBot = 0; // new change
              }
            }
            // check if there is a ramp above
            if ((int)i < g_layers)
            {
              for (auto n : g_ramps[i + 1])
              {
                rect ramprect = rect(n->x, n->y, 64, 55);
                if (RectOverlap(b->bounds, ramprect))
                {
                  // there is a ramp above this block, so it needs no shine
                  b->shineTop = 0;
                  b->shineBot = 0;
                  b->capped = 0;
                }
              }
            }

            if (!b->capped)
            {
              b->shineTop = false;
              b->shineBot = false;
            }

            uneighbor = {b->bounds.x + 2, b->bounds.y - 55 + 2, b->bounds.width - 4, 55 - 4};
            dneighbor = {b->bounds.x + 2, b->bounds.y + 2 + b->bounds.height, b->bounds.width - 4, 55 - 4};
            rect lneighbor = {b->bounds.x + 2 - 64, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
            rect rneighbor = {b->bounds.x + 2 + b->bounds.width, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
            // check for overlap with tiles if it is on layer 0 and with boxes and triangles a layer below for each dir
            if (i == 0)
            {
              for (auto t : g_tiles)
              {
                if (RectOverlap(t->getMovedBounds(), uneighbor))
                {
                  b->shadeTop = true;
                }
                if (RectOverlap(t->getMovedBounds(), dneighbor) && b->shadeBot != 2)
                {
                  b->shadeBot = true;
                }
                if (RectOverlap(t->getMovedBounds(), lneighbor))
                {
                  b->shadeLeft = true;
                }
                if (RectOverlap(t->getMovedBounds(), rneighbor))
                {
                  b->shadeRight = true;
                }
              }
              // but, if there is a layer 0 block overlapping our neighbor rect, we want to disable that shading
              // because that would be a random shadow under a block with visible corners
              for (auto n : g_boxs[0])
              {
                if (n == b)
                {
                  continue;
                }
                // don't calculate lighting by invisible walls
                if (n->walltexture == "resources/engine/seethru.qoi")
                {
                  continue;
                }
                if (RectOverlap(n->bounds, uneighbor))
                {
                  b->shadeTop = false;
                }
                if (RectOverlap(n->bounds, dneighbor) && 1)
                {
                  b->shadeBot = false;
                }
                if (RectOverlap(n->bounds, lneighbor))
                {
                  b->shadeLeft = false;
                }
                if (RectOverlap(n->bounds, rneighbor))
                {
                  b->shadeRight = false;
                }
              }
              // same for tris
              for (auto t : g_triangles[0])
              {

                if (TriRectOverlap(t, uneighbor))
                {
                  // b->shadeTop = false;
                }
                if (TriRectOverlap(t, dneighbor) && 1)
                {
                  b->shadeBot = false;
                }
                if (TriRectOverlap(t, lneighbor))
                {
                  b->shadeLeft = false;
                }
                if (TriRectOverlap(t, rneighbor))
                {
                  b->shadeRight = false;
                }
              }

              //Don't use top shading if there's an implied slope above
              //Otherwise, there will be a corner of shadow which might be displayed
              for(auto i : g_impliedSlopes) {
                if(i->shadedAtAll) {
                  if(RectOverlap(i->bounds, uneighbor)) {
                    b->shadeTop = false;
  
                  }
                }

              }
            }
            if (i > 0)
            {
              for (auto n : g_boxs[i - 1])
              {
                // don't calculate lighting by invisible walls
                if (n->walltexture == "resources/engine/seethru.qoi")
                {
                  continue;
                }
                if (!n->capped)
                {
                  continue;
                }

                if (RectOverlap(n->bounds, uneighbor))
                {
                  b->shadeTop = true;
                }
                if (RectOverlap(n->bounds, dneighbor))
                {
                  b->shadeBot = true;
                }
                if (RectOverlap(n->bounds, lneighbor))
                {
                  b->shadeLeft = true;
                }
                if (RectOverlap(n->bounds, rneighbor))
                {
                  b->shadeRight = true;
                }
              }
              for (auto t : g_triangles[i - 1])
              {
                if (!t->capped)
                {
                  continue;
                }
                if (TriRectOverlap(t, uneighbor))
                {
                  b->shadeTop = true;
                }
                if (TriRectOverlap(t, dneighbor))
                {
                  b->shadeBot = true;
                }
                if (TriRectOverlap(t, lneighbor))
                {
                  b->shadeLeft = true;
                }
                if (TriRectOverlap(t, rneighbor))
                {
                  b->shadeRight = true;
                }
              }
            }
            if (b->walltexture == "resources/engine/seethru.qoi")
            {
              b->shineTop = 0;
              b->shineBot = 0;
              // continue;
            }
          }
        }

        // bake triangles
        for (int i = 0; i < g_layers; i++)
        {
          for (auto tri : g_triangles[i])
          {
            tri->shaded = 0;
            if (tri->layer == 0)
            {
              tri->shaded = 1;
              for (auto tile : g_tiles)
              {
                if (TriRectOverlap(tri, tile->getMovedBounds()))
                {
                  tri->shaded = 1;
                }
              }
            }
            else
            {
              for (auto b : g_boxs[i - 1])
              {
                // don't calculate lighting by invisible walls
                if (b->walltexture == "resources/engine/seethru.qoi")
                {
                  continue;
                }
                if (TriRectOverlap(tri, b->bounds.x + 2, b->bounds.y + 2, b->bounds.width - 4, b->bounds.height - 4))
                {
                  tri->shaded = 1;
                }
              }
            }
          }
        }

        ///////////////////
        SDL_Delay(500);
        // combine box collisions where possible, likely after a split operation
        bool refresh = true; // wether to restart the operation
        while (refresh)
        {
          refresh = false;
          for (long long unsigned int i = 0; i < g_boxs.size(); i++)
          {
            for (auto him : g_boxs[i])
            {
              if(!him->valid) {
                continue;
              }
              // is there a collision in the space one block to the right of this block, and at the top?
              //   * * x
              //   * *
              for (auto other : g_boxs[i])
              {
                if(!other->valid) {
                  continue;
                }
                if (RectOverlap(rect(him->bounds.x + him->bounds.width + 2, him->bounds.y + 2, 64 - 4, 55 - 4), other->bounds))
                {
                  // does it have the same shineTop as him?
                  if (him->shineTop == other->shineTop)
                  {
                    // does it have the same height and y position as him?
                    if (him->bounds.y == other->bounds.y && him->bounds.height == other->bounds.height)
                    {
                      // does it have the same shineBot as him?
                      if (him->shineBot == other->shineBot)
                      {
                        // shading?
                        if (him->shadeBot == other->shadeBot && him->shadeTop == other->shadeTop)
                        {
                          // textures?
                          if (him->captexture == other->captexture && him->walltexture == other->walltexture)
                          {
                            // both capped or not capped?
                            if (him->capped == other->capped)
                            {
                              // join the two blocks
                              string shadestring = "";
                              if (him->shadeTop)
                              {
                                shadestring += "1";
                              }
                              else
                              {
                                shadestring += "0";
                              }
                              if (him->shadeBot)
                              {
                                shadestring += "1";
                              }
                              else
                              {
                                shadestring += "0";
                              }
                              if (him->shadeLeft)
                              {
                                shadestring += "1";
                              }
                              else
                              {
                                shadestring += "0";
                              }
                              if (other->shadeRight)
                              {
                                shadestring += "1";
                              }
                              else
                              {
                                shadestring += "0";
                              }
                              box *b = new box(him->bounds.x, him->bounds.y, him->bounds.width + other->bounds.width, him->bounds.height, him->layer, him->walltexture, him->captexture, him->capped, him->shineTop, him->shineBot, shadestring.c_str());
                              (void)b;
//                              for (auto child : him->children)
//                              {
//                                delete child;
//                              }

                              // breakpoint();

                              // its this line
                              delete him;
                              //him->valid = 0;

//                              for (auto child : other->children)
//                              {
//
//                                delete child;
//                              }
                              delete other;
                              //other->valid = 0;
                              refresh = true;
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }

//        //clear all invalid boxes
//        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
//        {
//          for(auto x : g_boxs[i]) {
//            if(!x->valid) {
//
//              for (auto child : x->children)
//              {
//                delete child;
//              }
//              delete x;
//
//            }
//          }
//        }

        // vertical pass
        refresh = true;
        while (refresh)
        {
          refresh = false;
          for (long long unsigned int i = 0; i < g_boxs.size(); i++)
          {
            for (auto him : g_boxs[i])
            {
              if(!him->valid) {
                continue;
              }
              // is there a collision in the space directly below this block
              //   * *
              //   * *
              //   x
              for (auto other : g_boxs[i])
              {
                if(!other->valid) {
                  continue;
                }
                if (RectOverlap(rect(him->bounds.x + 2, him->bounds.y + him->bounds.height + 2, 64 - 4, 55 - 4), other->bounds))
                {
                  // does it have the same width and x position as him?
                  if (him->bounds.x == other->bounds.x && him->bounds.width == other->bounds.width)
                  {
                    if (him->shadeLeft == other->shadeLeft && him->shadeRight == other->shadeRight)
                    {
                      // textures?
                      if (him->captexture == other->captexture && him->walltexture == other->walltexture)
                      {
                        // both capped or not capped?
                        if (him->capped == other->capped)
                        {
                          // join the two blocks
                          string shadestring = "";
                          if (him->shadeTop)
                          {
                            shadestring += "1";
                          }
                          else
                          {
                            shadestring += "0";
                          }
                          if (other->shadeBot)
                          {
                            shadestring += "1";
                          }
                          else
                          {
                            shadestring += "0";
                          }
                          if (him->shadeLeft)
                          {
                            shadestring += "1";
                          }
                          else
                          {
                            shadestring += "0";
                          }
                          if (him->shadeRight)
                          {
                            shadestring += "1";
                          }
                          else
                          {
                            shadestring += "0";
                          }

                          new box(him->bounds.x, him->bounds.y, him->bounds.width, him->bounds.height + other->bounds.height, him->layer, him->walltexture, him->captexture, him->capped, him->shineTop, other->shineBot, shadestring.c_str());
//                          for (auto child : him->children)
//                          {
//                            delete child;
//                          }
                          delete him;
                          //him->valid = 0;

//                          for (auto child : other->children)
//                          {
//                            delete child;
//                          }
                          delete other;
                          //other->valid = 0;

                          refresh = true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }

        //calculate shading for impliedslopes
        for(auto b : g_impliedSlopes) {
          rect lneighbor = {b->bounds.x + 2 - 64, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
          rect rneighbor = {b->bounds.x + 2 + b->bounds.width, b->bounds.y + 2, 64 - 4, b->bounds.height - 4};
          b->shadeLeft = 0;
          b->shadeRight = 0;
          for (auto t : g_tiles) {
            if (RectOverlap(t->getMovedBounds(), lneighbor)) {
              b->shadeLeft = true;
            }
            if (RectOverlap(t->getMovedBounds(), rneighbor)) {
              b->shadeRight = true;
            }
          }

          //remove shading if there is a collision there
          for (auto n : g_boxs[0])
          {
            // don't calculate lighting by invisible walls
            if (n->walltexture == "resources/engine/seethru.qoi")
            {
              continue;
            }
            if (RectOverlap(n->bounds, lneighbor))
            {
              b->shadeLeft = false;
            }
            if (RectOverlap(n->bounds, rneighbor))
            {
              b->shadeRight = false;
            }
          }

          for(auto t : g_triangles[0]) {
            if(TriRectOverlap(t, lneighbor)) {
              b->shadeLeft = false;
            }
            if(TriRectOverlap(t, rneighbor)) {
              b->shadeRight = false;
            }
          }
        }

        for(auto t : g_impliedSlopeTris) {
          //check the triangle below and copy its style
          int lowerY = min(t->y1, t->y2);
          int lowerX = min(t->x1, t->x2);
          rect dneighbor = {lowerX + 16, lowerY + 16 + 64, 32, 32};
          
          for(auto tri : g_triangles[0]) {
            if(TriRectOverlap(tri, dneighbor)) {
              M("Baked IST style");
              t->style = tri->style;
              break;
            }
          }

        }
        
//        //clear all invalid boxes
//        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
//        {
//          for(auto x : g_boxs[i]) {
//            if(!x->valid) {
//              for (auto child : x->children)
//              {
//                delete child;
//              }
//              delete x;
//            }
//          }
//        }

        ///////////////////////
        if (g_map != "")
        {
          mapeditor_save_map(g_map);
        }
        // ofile.close();
        word = "resources/maps/" + g_mapdir + "/" + g_map + ".map";
        clear_map(g_camera);
        int savex = mapent->x;
        int savey = mapent->y;
        load_map(renderer, word.c_str(), "a");
        init_map_writing(renderer);
        mapent->x = savex;
        mapent->y = savey;
        I("Finished full EB-operation");
        break;
      }

      if (word == "split")
      {
        // split the box collisions into grid-sized blocks
        // copy array
        for (long long unsigned int i = 0; i < g_boxs.size(); i++)
        {
          vector<box *> tempboxes;
          for (auto b : g_boxs[i])
          {
            tempboxes.push_back(b);
          }
          for (auto b : tempboxes)
          {
            int x = b->bounds.x;
            int y = b->bounds.y;
            while (x < b->bounds.width + b->bounds.x)
            {
              while (y < b->bounds.height + b->bounds.y)
              {
                new box(x, y, 64, 55, b->layer, b->walltexture, b->captexture, b->capped, 0, 0, "0000");
                y += 55;
              }
              x += 64;
              y = b->bounds.y;
            }

            for (auto child : b->children)
            {
              delete child;
            }

            delete b;
          }
        }
      }

      if (word == "ninja") 
      {
        if(line >> word) {
          g_ninja = stoi(word);
          D(g_ninja);
        } else {
          g_ninja = 1;
          M("Turned on ninjamode");
        }
      }

      if (word == "reload" || word == "r")
      {

        if (g_map != "")
        {
          mapeditor_save_map(g_map);
        }
        ofile.close();
        word = "resources/maps/" + g_mapdir + "/" + g_map + ".map";
        D(word);
        D(g_mapdir);
        D(g_map);
        clear_map(g_camera);
        int savex = mapent->x;
        int savey = mapent->y;
        load_map(renderer, word.c_str(), "a");
        init_map_writing(renderer);
        mapent->x = savex;
        mapent->y = savey;
        break;
      }
      if (word == "save")
      {
        if (line >> word)
        {
          mapeditor_save_map(word);
          break;
        }
      }
      if (word == "delete")
      {
        word = "";
        line >> word;
        // e.g. delete " " to delete your mapdir, delete /a.map to delete a map from the mapdir
        if (word == "")
        {
          if (yesNoPrompt("Woa, you wanna delete an entire map-directory? Are you sure?") == 0)
          {
            std::filesystem::remove_all("resources/maps/" + g_mapdir + word);
          }
        }
        else
        {
          std::filesystem::remove_all("resources/maps/" + g_mapdir + "/" + word + ".map");
        }
        break;
      }

      // delete collisionZones overlapping the marker
      if (word == "deletecz")
      {
        for (auto x : g_collisionZones)
        {
          if (RectOverlap(x->bounds, marker->getMovedBounds()))
          {
            M("Deleted collisionzone");
            g_collisionZones.erase(remove(g_collisionZones.begin(), g_collisionZones.end(), x), g_collisionZones.end());
            delete x;
          }
        }
        break;
      }

      if (word == "load")
      {
        if (line >> word)
        {
          fdebug = -1;
          // must close file before renaming it
          ofile.close();
          string theme = word;
          word = "resources/maps/" + g_mapdir + "/" + word + ".map";
          
          clear_map(g_camera);
          load_map(renderer, word.c_str(), "a");

          init_map_writing(renderer);
          if (g_autoSetThemesFromMapDirectory)
          {
            changeTheme(g_mapdir);
          }

          break;
        }
      }
      if(word == "test" || word == "play" || word == "dtest")
      {
        //set the map as next and do a dungeonflash
        if (line >> word)
        {
          g_dungeon.at(g_dungeonIndex+1).map = word + ".map";
          g_dungeonDoorActivated = 1;
        }

      }
      if(word == "gload") //load a map in game-mode
                         //this probably breaks lots of stuff
      {
        if (line >> word)
        {
          devMode = 0;
          // must close file before renaming it
          ofile.close();
          string theme = word;
          word = "resources/maps/" + g_mapdir + "/" + word + ".map";
          
          clear_map(g_camera);
          load_map(renderer, word.c_str(), "a");

          init_map_writing(renderer);
          if (g_autoSetThemesFromMapDirectory)
          {
            changeTheme(g_mapdir);
          }

          break;
        }
      }
      if (word == "clear")
      {
        line >> word;
        if (word == "map")
        {
          clear_map(g_camera);
          // load_map(renderer, "empty.map", "a");
          init_map_writing(renderer);
          break;
        }
        if (word == "navnodes")
        {
          int size = (int)g_navNodes.size();
          for (int i = 0; i < size; i++)
          {
            delete g_navNodes[0];
          }
          break;
        }
        if (word == "waypoints")
        {
          for(auto x : g_waypoints) {
            delete x;
          }
        }
      }
      if (word == "stake")
      {
        // stake as in, stake something at the cursor
        line >> word;
        if (word == "leftbound" || word == "lb")
        {
          limits[0] = marker->x;
          limits[1] = marker->y;
          break;
        }
        if (word == "rightbound" || word == "rb")
        {
          limits[2] = marker->x + marker->width;
          limits[3] = marker->y + marker->height;
          break;
        }
      }
      if (word == "shove")
      {
        protag->x = 100000;
        protag->y = 100000;
      }
      if(word == "hide") 
      {
        g_hide_protag = 1;
        M("hid");
      }
      if(word == "show") 
      {
        g_hide_protag = 0;

      }
      if (word == "populate")
      {
        populateMapWithEntities();
        break;
      }
      if (word == "ghost" || word == "geist" || word == "noclip")
      {
        boxsenabled = !boxsenabled;
        break;
      }
      if(word == "togglecollsionresolver" || word == "tcr") {
        g_collisionResolverOn = !g_collisionResolverOn;
        break;
      }
      if(word == "status")
      {
        line >> word;
        if(word == "slown")
        {
          protag->hisStatusComponent.slown.addStatus(5000, 0.2);
        }
        if(word == "stunned")
        {
          protag->hisStatusComponent.stunned.addStatus(5000, 1);
        }
        if(word == "poisoned")
        {
          protag->hisStatusComponent.poisoned.addStatus(5000, 1);
        }
        if(word == "healen")
        {
          protag->hisStatusComponent.healen.addStatus(5000, 1);
        }
        if(word == "enraged")
        {
          protag->hisStatusComponent.enraged.addStatus(5000, 1);
        }
        if(word == "marked")
        {
          protag->hisStatusComponent.marked.addStatus(5000, 1);
        }
        if(word == "disabled")
        {
          protag->hisStatusComponent.disabled.addStatus(5000, 1);
        }
        if(word == "buffed")
        {
          protag->hisStatusComponent.disabled.addStatus(5000, 1);
        }
        break;
      }
      M("Are we getting here?");
      if (word == "set" || word == "s")
      {
        line >> word;
        D(word);
        if(word == "collisionresolver" || word == "cr") {
          line >> word;
          int val = stoi(word);
          M(val);
          if(val != 0) {
            g_collisionResolverOn = 1;
          } else {
            g_collisionResolverOn = 0;
          }
          
          if(val == 2) {
            g_showCRMessages = 1;
          } else {
            g_showCRMessages = 0;
          }
          break;
        }
        if (word == "protag" || word == "me")
        {
          line >> word;
          entity *hopeful = searchEntities(word);
          if (hopeful != nullptr)
          {
            protag = hopeful;
          }
          break;
        }
        if(word == "brightness" || word == "bright" || word == "b")
        {
          int brightness;
          if(line >> brightness)
          {
            float fbrightness = brightness;
            SDL_SetTextureAlphaMod(g_shade, 255 - ( ( fbrightness/100.0 ) * 255));
            g_brightness = brightness;
          }
          break;
        }

        if (word == "objective" || word == "obj")
        {
          line >> word;
          if (word != "")
          {
            entity *hopeful = searchEntities(word);
            if (hopeful != 0)
            {
              g_objective = hopeful;
              adventureUIManager->crosshair->show = 1;
            }
          }
          break;
        }

        if (word == "focus")
        {
          line >> word;
          entity *hopeful = searchEntities(word);
          if (hopeful != nullptr)
          {
            g_focus = hopeful;
          }
          break;
        }
        if (word == "mapdir" || word == "md")
        {
          string a;
          if (line >> a)
          {
            g_mapdir = a;
          }

          break;
        }
        if (word == "fogofwar" || word == "fow" || word == "dark" || word == "darkness")
        {
          bool b;
          if (line >> b)
          {
            g_fogofwarEnabled = b;
          }
          if (g_fogofwarEnabled)
          {
            for (auto x : g_fogslates)
            {
              x->tangible = 1;
            }
            for (auto x : g_fogslatesA)
            {
              x->tangible = 1;
            }
            for (auto x : g_fogslatesB)
            {
              x->tangible = 1;
            }
          }
          else
          {
            for (auto x : g_fogslates)
            {
              x->tangible = 0;
            }
            for (auto x : g_fogslatesA)
            {
              x->tangible = 0;
            }
            for (auto x : g_fogslatesB)
            {
              x->tangible = 0;
            }
          }
        }
        if (word == "budget")
        {
          int a;
          if (line >> a)
          {
            g_budget = a;
          }
          break;
        }

        if (word == "navnodecorneringwidth" || word == "nncw" || word == "navcornering" || word == "nncr" || word == "cornering")
        {
          // this is the width of a boxtrace used for testing if navnodes should be joined.
          // if AI are getting caught on walls, increase this to about the width of their hitboxes
          float inp;
          if (line >> inp && inp > 0)
          {
            mapeditorNavNodeTraceRadius = inp;
          }
          break;
        }
        if (word == "heightmapmagnitude" || word == "hmmg" || word == "heightmap")
        {
          float inp;
          if (line >> inp && inp > 0 && g_heightmaps.size() > 0)
          {
            // the factor is to convert user input to world blocks
            g_heightmaps.at(g_heightmaps.size() - 1)->magnitude = inp * (float)(64.00 / 255.00);
          }
          break;
        }
        if (word == "background" || word == "bg")
        {
          string str;

          if (line >> str)
          {
            backgroundstr = str;
          }
          if (g_backgroundLoaded)
          {
            SDL_DestroyTexture(background);
          }
          SDL_Surface *bs = IMG_Load(("resources/static/backgrounds/" + backgroundstr + ".qoi").c_str());
          background = SDL_CreateTextureFromSurface(renderer, bs);
          g_backgroundLoaded = 1;
          SDL_FreeSurface(bs);
          break;
        }
        if (word == "texturedirectory" || word == "td" || word == "theme")
        {
          string str;
          if (line >> str)
          {

            changeTheme(str);
          }

          break;
        }
        if (word == "tdx")
        {
          float number;
          line >> number;
          if (g_tiles.size() > 0)
          {
            g_tiles[g_tiles.size() - 1]->dxoffset = number;
          }
          break;
        }

        if (word == "tdy")
        {
          float number;
          line >> number;
          if (g_tiles.size() > 0)
          {
            g_tiles[g_tiles.size() - 1]->dyoffset = number;
          }
          break;
        }

        if (word == "shine" || word == "sh")
        {
          line >> shine;
          break;
        }

        if (word == "limits")
        {
          int a, b, c, d;
          line >> a >> b >> c >> d;
          limits[0] = a;
          limits[1] = b;
          limits[2] = c;
          limits[3] = d;
          break;
        }

        if (word == "navdensity" || word == "navnodedensity" || word == "navmeshdensity")
        {
          M("set nmd");
          line >> navMeshDensity;
          if (navMeshDensity < 0.25)
          {
            navMeshDensity = 0.25;
          }
          break;
        }

        if (word == "wallheight" || word == "wh")
        {
          line >> wallheight;
          wallheight *= 64;
          break;
        }
        if (word == "wallstart" || word == "ws")
        {
          line >> wallstart;
          wallstart *= 64;
          break;
        }
        if (word == "wall" || word == "w")
        {
          line >> walltex;
          walltex = "resources/static/diffuse/" + textureDirectory + walltex + ".qoi";

          break;
        }
        if (word == "floor" || word == "f")
        {
          line >> floortex;

          if (floortex == "black")
          {
            floortex = "resources/engine/black-diffuse.qoi";
          }
          else
          {
            floortex = "resources/static/diffuse/" + textureDirectory + floortex + ".qoi";
          }
          break;
        }
        if (word == "cap" || word == "c")
        {
          line >> captex;
          captex = "resources/static/diffuse/" + textureDirectory + captex + ".qoi";
          break;
        }
        if (word == "mask" || word == "m")
        {
          line >> masktex;
          masktex = "resources/static/masks/" + masktex + ".qoi";
          break;
        }
        if (word == "layer")
        {
          int number;
          if (line >> number)
          {
            layer = number;
          };
          break;
        }
        if (word == "my")
        {
          line >> word;
          if (word == "speed")
          {
            float speed;
            if (line >> speed)
            {
              mapent->xmaxspeed = speed;
              //mapent->ymaxspeed = speed;
            }
            break;
          }
          if (word == "faction")
          {
            int p0;
            if (line >> p0)
            {
              mapent->faction = p0;
            }
            break;
          }
          if (word == "hp")
          {
            float p0;
            if (line >> p0)
            {
              mapent->hp = p0;
            }
            break;
          }
          if (word == "tangible" || word == "tangibility")
          {
            bool b0;
            if (line >> b0)
            {
              mapent->tangible = b0;
            }
            break;
          }
          if (word == "dynamic" || word == "dynamicness")
          {
            bool b0;
            if (line >> b0)
            {
              mapent->dynamic = b0;
            }
            break;
          }
          if (word == "z")
          {
            float z;
            if (line >> z)
            {
              mapent->z = z;
              break;
            }
          }
        }

        if (word == "drawcolor")
        {

          if (line >> debug_r >> debug_g >> debug_b)
          {

            break;
          }
        }
        if (word == "tiling")
        {
          bool num;
          if (line >> num)
          {
            tiling = num;
          }
          break;
        }
        if (word == "collisions" || "collision")
        {
          bool num;
          if (line >> num)
          {
            boxsenabled = num;
          }
          break;
        }
        if (word == "grid")
        {
          line >> grid;
          break;
        }
        if (word == "mute")
        {
          bool num;
          if (line >> num)
          {
            g_mute = num;
            Mix_VolumeMusic(0);
          }
          if (num == 1)
          {
            Mix_HaltMusic();
          }

          break;
        }
      }
      if (word == "mute")
      {
        g_mute = !g_mute;
        if (g_mute)
        {
          Mix_HaltMusic();
        }
      }

      if(word =="goaway")
      {
        for(auto x : g_entities) {
          if(x->dynamic && x->targetFaction == protag->faction) {
            x->agrod = 0;
            x->tangible = 0;
          }
        }

      }

      if(word == "break" || word == "breakpoint") 
      {
        breakpoint();
      }

      //console anim testing
      // anim direction msPerFrame frameInAnimation LoopAnimation reverse
      if(word == "anim" || word == "animate") 
      {
        string entName = "";
        int direction, MsPerFrame, FrameInAnimation, LoopAnimation, reverse;
        line >> entName >> direction >> MsPerFrame >> FrameInAnimation >> LoopAnimation >> reverse;
        entity* hopeful = searchEntities(entName);
        if(hopeful != nullptr) {
          hopeful->animation = direction;
          hopeful->msPerFrame = MsPerFrame;
          hopeful->frameInAnimation = FrameInAnimation;
          hopeful->loopAnimation = LoopAnimation;
          hopeful->reverseAnimation = reverse;
          hopeful->scriptedAnimation = 1;
        }


        break;
      }

      if(word == "msperframe")
      {
        M("Set msperframe");
        string msp;
        line >> msp;
        protag->msPerFrame = 50;

      }

      if (word == "debug") //print some debug info about this entity
      {
        line >> word;
        entity *chosenEntity = searchEntities(word);
        if(chosenEntity == nullptr) { chosenEntity = protag;}
        if (chosenEntity != nullptr)
        {
          M("--DEBUG INFO FOR CHOSEN ENTITY:");
          M("");

          M("Physics Data:");
          D(chosenEntity->canBeSolid);
          D(chosenEntity->solid);
          D(chosenEntity->semisolid);
          D(chosenEntity->pushable);
          D(chosenEntity->navblock);
          D(chosenEntity->xmaxspeed);
          D(chosenEntity->friction);
          D(chosenEntity->steeringAngle);
          D(chosenEntity->targetSteeringAngle);
          M("");

          M("Visual Data:");
          D(chosenEntity->large);
          D(chosenEntity->banished);
          D(chosenEntity->createdAfterMapLoad);
          D(chosenEntity->opacity);
          D(chosenEntity->visible);
          D(chosenEntity->texture);
          D(chosenEntity->darkenValue);
          D(chosenEntity->darkenMs);
          M("");

          M("Animation Data:");
          D(chosenEntity->framewidth);
          D(chosenEntity->frameheight);
          D(chosenEntity->xframes); //frames per anim
          D(chosenEntity->yframes); //directions
          D(chosenEntity->growFromFloor);
          D(chosenEntity->turnToFacePlayer);
          D(chosenEntity->msPerFrame);
          D(chosenEntity->frameInAnimation);
          D(chosenEntity->animation); //this is really direction
          D(chosenEntity->loopAnimation);
          D(chosenEntity->scriptedAnimation);
          M("");

          M("Particle Data:");
          D(g_emitters.size());
          D(g_particles.size());
          M("");

          M("OOP Data:");
          D(chosenEntity->dynamic);
          D(chosenEntity->persistentHidden);
          D(chosenEntity->persistentGeneral);
          D(chosenEntity->isOrbital);
          D(chosenEntity->isWorlditem);
          D(chosenEntity->isAI);
          D(chosenEntity->aiIndex);
          D(chosenEntity->isBoardable);
          D(chosenEntity->transportEntPtr);
          D(chosenEntity->tangible);
          M("");

          M("Combat Data:");
          D(chosenEntity->faction);
          D(chosenEntity->visionRadius);
          D(chosenEntity->visionTime);
          D(chosenEntity->hearingRadius);
          D(chosenEntity->invincible);
          D(chosenEntity->hp);
          D(chosenEntity->maxhp);
          D(chosenEntity->weaponName);
          M("");

          M("Pathfinding:");
          D(chosenEntity->pathfinding);
          D(chosenEntity->current);
          D(chosenEntity->dest);
          D(chosenEntity->maxStuckTime);
          D(chosenEntity->stuckTime);
          D(chosenEntity->navblock);
          M("");

          M("AI");
          for(auto x :g_ai) {
            D(x->name);
            D(x->aiIndex);
            D(x->targetFaction);
            D(x->customMovement);
            D(x->hearingRadius);
            D(x->movementTypeSwitchRadius);
            D(x->agrod);
            M("");
          }

          M("Behemoths:");
          for(auto x :g_behemoths) {
            D(x->name);
            D(x->aiIndex);
            D(x->targetFaction);
            D(x->customMovement);
            D(x->hearingRadius);
            D(x->movementTypeSwitchRadius);
            D(x->agrod);
            M("");
          }


          M("Scripting:");
          D(chosenEntity->usesContactScript);
          D(chosenEntity->contactScriptWaitMS);
          D(chosenEntity->curContactScriptWaitMS);
          D(chosenEntity->contactReadyToProc);
          M("ContactScript:");
          M("{");
          for(auto x : chosenEntity->contactScript){
            D(x);
          }
          M("}");
          D(chosenEntity->talks);
          M("Dialogue:");
          M("{");
          for(auto x : chosenEntity->sayings){
            D(x);
          }
          M("}");
          M("");



        } else {
          //debug g_entities instead of a particular entity
          M("Entities List:");
          M("{");
          for(int i = 0; i < g_entities.size(); i++){
            if("resources/engine" != g_entities[i]->name.substr(0,6)) {
              string iAsString = to_string(i);
              string printMe = "g_entities[" + iAsString + "]->name: " + g_entities[i]->name;
              M(printMe);
            }
          }
          M("}");
        }

      }

      if(word == "entities" || word == "ents" || word == "entlist" || word == "debug all" || word == "debug ents" || word == "debug entities") {
        //debug g_entities instead of a particular entity
        M("Entities List:");
        M("{");
        for(int i = 0; i < g_entities.size(); i++){
          if("resources/engine" != g_entities[i]->name.substr(0,6)) {
            string iAsString = to_string(i);
            vector<string> exceptions = {"engine/sp-fogslate" , "engine/sp-deity" , "common/spurl" , "common/chain" , "common/fomm-spin"};
            bool printName = 1;
            for(auto x : exceptions) {
              if(g_entities[i]->name == x) {printName = 0;}
            }

            if(printName) {
              string printMe = "g_entities[" + iAsString + "]->name: " + g_entities[i]->name;
              M(printMe);
            }
          }
        }
        M("}");
      }
      
      if (word == "adj" || word == "adjust")
      {
        // adjust
        line >> word;
        if (word == "bx")
        {
          // g_entities[g_entities.size() - 1]->bounds.x = (g_entities[g_entities.size() - 1]->width/2 - g_entities[g_entities.size() - 1]->shadow->width/2);
          int number;
          line >> number;
          protag->bounds.x = number;
          protag->bounds.x += protag->width / 2 - protag->bounds.width / 2;
          break;
        }
        if (word == "by")
        {
          int number;
          line >> number;
          protag->bounds.y = number;
          protag->bounds.y += protag->height - protag->bounds.height / 2;
          break;
        }
        if (word == "bw")
        {
          line >> protag->bounds.width;
          break;
        }
        if (word == "bh")
        {
          line >> protag->bounds.height;
          break;
        }
        if (word == "shadowx" || word == "sx")
        {
          line >> protag->shadow->x;
          protag->shadow->xoffset += protag->width / 2 - protag->shadow->width / 2;
          break;
        }
        if (word == "shadowy" || word == "sy")
        {
          line >> protag->shadow->yoffset;
          protag->shadow->yoffset += protag->height - protag->shadow->height / 2;
          break;
        }
      }

      if (word == "where")
      {
        M(px);
        M(py);
      }

      if(word == "waypointangle")
      {
        line >> word;
        g_waypoints[g_waypoints.size() - 1]->angle = convertAngleToFrame(protag->steeringAngle);
      }

      if (word == "possess")
      {
        line >> word;
        if (word != "")
        {
          entity *hopeful = searchEntities(word);
          if (hopeful != 0)
          {
            protag = hopeful;
            g_focus = hopeful;
          }
        }
        break;
      }

      if (word == "reset" || word == "rs")
      {
        line >> word;

        if (word == "enemies")
        {
          enemiesMap.clear();
          break;
        }

        if (word == "texturedirectory" || word == "td" || word == "theme")
        {
          textureDirectory = "mapeditor/";
          // re-populate the array of textures that we rotate thru for creating floors, walls, and caps
          texstrs.clear();
          string path = "resources/static/diffuse/" + textureDirectory;
          if (!filesystem::exists(path))
          {
            M("Theme " + path + "not found");
            break;
          }
          for (const auto &entry : filesystem::directory_iterator(path))
          {
            texstrs.push_back(entry.path().u8string());
          }
          floortexIndex = 0;
          captexIndex = 0;
          walltexIndex = 0;

          floortex = texstrs[floortexIndex];
          delete floortexDisplay;
          floortexDisplay = new ui(renderer, floortex.c_str(), 0, 0, 0.1, 0.1, -100);
          walltex = texstrs[walltexIndex];
          delete walltexDisplay;
          walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0, 0.1, 0.1, -100);
          captex = texstrs[captexIndex];
          delete captexDisplay;
          captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0, 0.1, 0.1, -100);

          captex = "resources/static/diffuse/mapeditor/a.qoi";
          walltex = "resources/static/diffuse/mapeditor/c.qoi";
          floortex = "resources/static/diffuse/mapeditor/b.qoi";
          break;
        }
        if (word == "grid")
        {
          grid = 60;
          break;
        }
        if (word == "mask")
        {
          masktex = "&";
          break;
        }
        if (word == "me")
        {
          mapent->x = 0;
          mapent->y = 0;
          break;
        }
        if (word == "my")
        {
          line >> word;
          if (word == "speed")
          {
            mapent->xmaxspeed = 110;
            //mapent->ymaxspeed = 110;
            break;
          }
          if (word == "agility")
          {
            mapent->xagil = 100;
            mapent->xagil = 100;
            break;
          }
          if (word == "spot" || word == "position")
          {
            mapent->x = 0;
            mapent->y = 0;
            break;
          }
          if (word == "friction")
          {
            mapent->friction = 0.3;
            break;
          }
          if (word == "movement")
          {
            mapent->xmaxspeed = 110;
            //mapent->ymaxspeed = 110;
            mapent->xagil = 100;
            //mapent->xagil = 100;
            mapent->friction = 0.3;
            mapent->dynamic = 1;
          }
        }
        if (word == "bounds")
        {
          g_camera.resetCamera();
          limits[0] = 0;
          limits[1] = 0;
          limits[2] = 0;
          limits[3] = 0;
        }
      }
      if (word == "teleport" || word == "tp")
      {
        int x, y;
        line >> x >> y;
        mapent->x = x;
        mapent->y = y;
        break;
      }

      if(word == "whichmap" || word == "map" || word == "which") {
        M("Map: " + g_mapdir + "/" + g_map);
      }

      if(word == "rp" || word == "randomizepellets") {
        for(auto x : g_pellets) {
          x->animation = rand() % x->yframes;
        }
      }

      if (word == "travel")
      {
        string wayname;
        line >> wayname;

        for (long long unsigned int i = 0; i < g_waypoints.size(); i++)
        {
          if (g_waypoints[i]->name == wayname)
          {
            protag->x = g_waypoints[i]->x - protag->width / 2;
            protag->y = g_waypoints[i]->y + protag->bounds.height;
            protag->z = g_waypoints[i]->z;
            break;
          }
        }

        break;
      }
      if (word == "nudge" || word == "n")
      {
        int x, y;
        line >> x >> y;
        mapent->x += x;
        mapent->y += y;
        break;
      }

      if (word == "textureswap" || word == "texswap")
      {
        string tex1, tex2;
        tex1 = "";
        tex2 = "";
        line >> tex1 >> tex2;
        if (tex1 != "" && tex2 != "")
        {
          // swap textures for walls
          for (auto y : g_boxs)
          {
            for (auto x : y)
            {
              if (x->walltexture == tex1)
              {
                x->walltexture = tex2;
              }
              if (x->captexture == tex1)
              {
                x->captexture = tex2;
              }
            }
          }
          for (auto l : g_triangles)
          {
            for (auto t : l)
            {
              if (t->walltexture == tex1)
              {
                t->walltexture = tex2;
              }
              if (t->captexture == tex1)
              {
                t->captexture = tex2;
              }
            }
          }
          for (auto x : g_tiles)
          {
            if (x->fileaddress == tex1)
            {
              x->fileaddress = tex2;
            }
          }
        }
      }

      if (word == "themeswap" || word == "tswap")
      {
        string tex1, tex2;
        tex1 = "";
        tex2 = "";
        line >> tex1 >> tex2;
        if (tex1 != "" && tex2 != "")
        {
          // swap themes
          for (auto y : g_boxs)
          {
            for (auto x : y)
            {
              replaceString(x->walltexture, tex1, tex2);
              replaceString(x->captexture, tex1, tex2);
            }
          }
          for (auto l : g_triangles)
          {
            for (auto x : l)
            {
              replaceString(x->walltexture, tex1, tex2);
              replaceString(x->captexture, tex1, tex2);
            }
          }
          for (auto x : g_tiles)
          {
            replaceString(x->fileaddress, tex1, tex2);
          }
        }
      }

      if (word == "solidify")
      {
        line >> entstring;

        entity *solidifyMe = 0;
        solidifyMe = searchEntities(entstring);
        if (solidifyMe != 0)
        {
          solidifyMe->solidify();
        }
      }
      if (word == "unsolidify")
      {
        line >> entstring;

        entity *solidifyMe = 0;
        solidifyMe = searchEntities(entstring);
        if (solidifyMe != 0)
        {
          solidifyMe->unsolidify();
        }
      }
      if (word == "entity" || word == "ent" || word == "e")
      {

        line >> entstring;
        // actually spawn the entity in the world
        // string loadstr = "entities/" + entstring + ".ent";
        const char *plik = entstring.c_str();
        entity *e = new entity(renderer, plik);
        (void)e;
        e->x = px + marker->width / 2 - (e->getOriginX());
        e->y = py + marker->height / 2 - (e->getOriginY());
        e->stop_hori();
        e->stop_verti();
        e->z = wallstart;
        e->shadow->x = e->x + e->shadow->xoffset;
        e->shadow->y = e->y + e->shadow->yoffset;
        if(smokeEffect != nullptr) {
          //god this is so annoying, keep it off
          //smokeEffect->happen(e->getOriginX(), e->getOriginY(), e->z );
        }
        break;
      }
      if (word == "item")
      {
        line >> entstring;
        const char *plik = entstring.c_str();
        worldItem *e = new worldItem(plik, 0);
        (void)e;
        e->x = px + marker->width / 2 - (e->getOriginX());
        e->y = py + marker->height / 2 - (e->getOriginY());
        e->stop_hori();
        e->stop_verti();
        e->z = wallstart;
        e->shadow->x = e->x + e->shadow->xoffset;
        e->shadow->y = e->y + e->shadow->yoffset;
        break;
      }
      if (word == "sound" || word == "snd" || word == "worldsound" || word == "ws")
      {
        line >> entstring;
        worldsound *w = new worldsound(entstring, px + marker->width / 2, py + marker->height / 2);
        (void)w;
      }
      if (word == "music" || word == "m")
      {
        line >> entstring;
        musicNode *m = new musicNode(entstring, px + marker->width / 2, py + marker->height / 2);
        (void)m;
      }
      if (word == "cue" || word == "cuesound")
      {
        // for cue, input soundname and radius
        line >> entstring;
        float radius = 0;
        line >> radius;
        radius *= 64;
        cueSound *m = new cueSound(entstring, px + marker->width / 2, py + marker->height / 2, radius);
        (void)m;
      }
      if (word == "pointofinterest" || word == "poi")
      {
        int index = 0;
        if (line >> index)
        {
          pointOfInterest *p = new pointOfInterest(px + marker->width / 2, py + marker->height / 2, index);
          (void)p;
        }

        break;
      }
      if (word == "way" || word == "w")
      {
        line >> entstring;
        waypoint *m = new waypoint(entstring, px + marker->width / 2, py + marker->height / 2, wallstart, convertAngleToFrame(protag->steeringAngle));
        (void)m;
      }
      if (word == "wayatme" || word == "wam")
      {
        line >> entstring;
        waypoint *m = new waypoint(entstring, mapent->getOriginX(), mapent->getOriginY(), mapent->z, convertAngleToFrame(protag->steeringAngle));
        (void)m;
      }
      if (word == "linkdoor" || word == "ld" || word == "door" || word == "d")
      { // consider renaming this "link" or something other than "door" because it doesnt make doors
        string mapdest, waydest;
        line >> mapdest >> waydest;
        if (g_doors.size() > 0)
        {
          g_doors[g_doors.size() - 1]->to_map = mapdest;
          g_doors[g_doors.size() - 1]->to_point = waydest;
        }
      }
      if (word == "trigger" || word == "t")
      {
        string fbinding;
        string fentity;
        line >> fbinding;
        line >> fentity;
        if (g_triggers.size() > 0)
        {
          g_triggers[g_triggers.size() - 1]->binding = fbinding;
          if (fentity.length() > 0)
          {
            D(fentity);
            g_triggers[g_triggers.size() - 1]->targetEntity = fentity;
          }
        }

        ifstream stream;
        string loadstr;

        loadstr = "resources/maps/" + g_map + "/" + fbinding + ".txt";
        const char *plik = loadstr.c_str();

        stream.open(plik);

        if (!stream.is_open())
        {
          stream.open("scripts/" + fbinding + ".txt");
        }
        string line;

        getline(stream, line);

        while (getline(stream, line))
        {
          g_triggers[g_triggers.size() - 1]->script.push_back(line);
        }
      }
      if (word == "listener" || word == "l")
      {
        M("LISTENER EVENT ENTNAME BLOCK VALUE");
        string fbinding, entstr, blockstr, valuestr;
        line >> fbinding >> entstr >> blockstr >> valuestr;
        if (valuestr != "")
        {
          listener *g = new listener(entstr, stoi(blockstr), stoi(valuestr), fbinding, px + 0.5 * marker->width, py + 0.5 * marker->height);
          (void)g;
        }
      }
      if (word == "navnode")
      {
        navNode *n = new navNode(px + marker->width / 2, py + marker->height / 2, 0);
        (void)n;
      }

      // remove a single navnode
      if (word == "peck")
      {
        rect mrect = {(int)marker->x, (int)marker->y, (int)marker->width, (int)marker->height};
        for (auto x : g_navNodes)
        {
          rect nzrect = {x->x - 10, x->y - 10, 20, 20};
          if (RectOverlap(mrect, nzrect))
          {
            delete x;
            break;
          }
        }
      }

      if (word == "navlink")
      {
        // delete nodes too close to walls
        // int checkinglayer = 0;
        float cullingdiameter = mapeditorNavNodeTraceRadius;
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          rect noderect = {(int)(g_navNodes[i]->x - cullingdiameter / 2), (int)(g_navNodes[i]->y - cullingdiameter / 2), (int)(cullingdiameter), (int)(cullingdiameter * XtoY)};
          noderect.z = g_navNodes[i]->z + 30;
          noderect.zeight = 1;

          for (long long unsigned int j = 0; j < g_boxs[layer].size(); j++)
          {
            M(g_boxs[layer][j]->bounds.x);
            M(noderect.x);
            M((layer)*64);
            M(g_navNodes[i]->x);
            M(g_navNodes[i]->z);
            if (RectOverlap3d(g_boxs[layer][j]->bounds, noderect))
            {
              delete g_navNodes[i];
              i--;
              break;
            }
          }

          // break;//temp
        }
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          for (long long unsigned int j = 0; j < g_navNodes.size(); j++)
          {
            if (i == j)
            {
              continue;
            }

            float gwt = max(g_navNodes[i]->z, g_navNodes[j]->z);
            gwt /= 64;

            if (Distance(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y) < 300 && (LineTrace(g_navNodes[i]->x, g_navNodes[i]->y, g_navNodes[j]->x, g_navNodes[j]->y, 0, mapeditorNavNodeTraceRadius, gwt, 10, 0, 0)))
            {

              // dont add a friend we already have
              bool flag = 1;
              for (auto x : g_navNodes[i]->friends)
              {
                if (x == g_navNodes[j])
                {
                  flag = 0;
                }
              }
              if (flag)
              {
                g_navNodes[i]->Add_Friend(g_navNodes[j]);
              }
            }
          }
        }

        // delete nodes with no friends
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          if ((int)g_navNodes[i]->friends.size() == 0)
          {
            delete g_navNodes[i];
            i--;
          }
        }

        // delete friends which dont exist anymore
        for (long long unsigned int i = 0; i < g_navNodes.size(); i++)
        {
          for (long long unsigned int j = 0; j < g_navNodes[i]->friends.size(); j++)
          {
            auto itr = find(g_navNodes.begin(), g_navNodes.end(), g_navNodes[i]->friends[j]);

            if (itr == g_navNodes.end())
            {
              // friend has been deleted, remove as well from friend array
              g_navNodes[i]->friends.erase(remove(g_navNodes[i]->friends.begin(), g_navNodes[i]->friends.end(), g_navNodes[i]->friends[j]), g_navNodes[i]->friends.end());
            }
          }
        }

        Update_NavNode_Costs(g_navNodes);
        break;
      }
    }
  }

  if (devinput[12] && !olddevinput[12])
  {
    // make triangle
    tri *n = 0;
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y, marker->x, marker->y + marker->height, i, walltex, captex, fcap, 0);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/a.qoi", marker->x, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      // a tile on the floor to help with the edge of the diagonal wall pieces
      // this tile won't be saved, because it uses an engine mask
      // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/a.qoi", marker->x, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (i * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, (i * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }
  if (devinput[13] && !olddevinput[13])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y, marker->x + marker->width, marker->y + marker->height, i, walltex, captex, fcap, 0);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/b.qoi", marker->x + 1, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    // a tile on the floor to help with the edge of the diagonal wall pieces
    // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/b.qoi", marker->x + 1, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (((64 - step) - i) * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, ((64 - i) * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }
  if (devinput[14] && !olddevinput[14])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y + marker->height, marker->x + marker->width, marker->y, i, walltex, captex, fcap, 0);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/c.qoi", n->x1, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }
  if (devinput[15] && !olddevinput[15])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y + marker->height, marker->x, marker->y, i, walltex, captex, fcap, 0);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/d.qoi", n->x2, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }

  // make rounded triangles
  if (devinput[26] && !olddevinput[26])
  {
    // make triangle
    tri *n = 0;
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y, marker->x, marker->y + marker->height, i, walltex, captex, fcap, 0, 1);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/aro.qoi", marker->x, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      // a tile on the floor to help with the edge of the diagonal wall pieces
      // this tile won't be saved, because it uses an engine mask
      // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/a.qoi", marker->x, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }
  if (devinput[27] && !olddevinput[27])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y, marker->x + marker->width, marker->y + marker->height, i, walltex, captex, fcap, 0, 1);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/bro.qoi", marker->x + 1, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    // a tile on the floor to help with the edge of the diagonal wall pieces
    // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/b.qoi", marker->x + 1, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5))) * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, ((64 - i) * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }

  if (devinput[28] && !olddevinput[28])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y + marker->height, marker->x + marker->width, marker->y, i, walltex, captex, fcap, 0, 1);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/cro.qoi", n->x1, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }
  if (devinput[29] && !olddevinput[29])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y + marker->height, marker->x, marker->y, i, walltex, captex, fcap, 0, 1);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/dro.qoi", n->x2, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }

  // make rounded triangles
  if (devinput[30] && !olddevinput[30])
  {
    // make triangle
    tri *n = 0;
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y, marker->x, marker->y + marker->height, i, walltex, captex, fcap, 0, 2);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/ari.qoi", marker->x, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      // a tile on the floor to help with the edge of the diagonal wall pieces
      // this tile won't be saved, because it uses an engine mask
      // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/a.qoi", marker->x, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + ((64 - pow(pow(64, 2) - pow(64 - i, 2), 0.5)) * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, ((64 - pow(pow(64, 2) - pow(i, 2), 0.5)) * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }
  if (devinput[31] && !olddevinput[31])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y, marker->x + marker->width, marker->y + marker->height, i, walltex, captex, fcap, 0, 2);
    }
    if (autoMakeWallcaps)
    {
      int step = g_platformResolution;
      for (int i = 0; i < 55; i += step)
      {
        mapObject *e = new mapObject(renderer, captex, "resources/engine/bri.qoi", marker->x + 1, marker->y + i + step, wallheight, 64 - 1, step, 0);
        n->children.push_back(e);
      }
    }

    // a tile on the floor to help with the edge of the diagonal wall pieces
    // tile* t = new tile(renderer, walltex.c_str(), "resources/engine/b.qoi", marker->x + 1, marker->y - 1, 64 - 1, 54 + 1, layer, 1, 1, 0, 0);

    int step = 2;
    int vstep = 64;
    if (autoMakeWalls)
    {
      for (int j = wallstart; j < wallheight; j += vstep)
      {
        for (int i = 0; i < 64; i += step)
        {
          mapObject *e = new mapObject(renderer, walltex, "&", marker->x + i, marker->y + marker->height - (((64 - step) - (64 - pow(pow(64, 2) - pow(i, 2), 0.5))) * XtoY) - 1, j, step, ceil(64 * XtoZ) + 1, 1, ((64 - i) * XtoY));
          n->children.push_back(e);
        }
      }
    }
  }

  if (devinput[32] && !olddevinput[32])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x, marker->y + marker->height, marker->x + marker->width, marker->y, i, walltex, captex, fcap, 0, 2);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/cri.qoi", n->x1, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }

  if (devinput[33] && !olddevinput[33])
  {
    tri *n = 0;
    // make triangle
    for (int i = wallstart / 64; i < wallheight / 64; i++)
    {
      bool fcap = (!(i + 1 < wallheight / 64));
      n = new tri(marker->x + marker->width, marker->y + marker->height, marker->x, marker->y, i, walltex, captex, fcap, 0, 2);
    }
    int step = g_platformResolution;
    for (int i = 0; i < 55; i += step)
    {
      mapObject *child = new mapObject(renderer, n->captexture, "resources/engine/dri.qoi", n->x2, n->y2 + i + step, n->layer * 64 + 64, 64 - 1, step, 0);
      n->children.push_back(child);
    }
  }

  if (devinput[37] && !olddevinput[37]) {
    sparksEffect->happen(g_focus->getOriginX() + 128, g_focus->getOriginY(), g_focus->z, 0);
  }

  // change wall, cap, and floor textures
  if (devinput[16] && !olddevinput[16])
  {
    if (!g_holdingCTRL)
    {
      floortexIndex++;
    }
    else
    {
      floortexIndex--;
    }
    if (floortexIndex == (int)texstrs.size())
    {
      floortexIndex = 0;
    }
    if (floortexIndex == -1)
    {
      floortexIndex = texstrs.size() - 1;
    }

    floortex = texstrs[floortexIndex];
    delete floortexDisplay;
    floortexDisplay = new ui(renderer, floortex.c_str(), 0, 0, 0.1, 0.1, -100);
  }

  if (devinput[17] && !olddevinput[17])
  {
    if (!g_holdingCTRL)
    {
      walltexIndex++;
    }
    else
    {
      walltexIndex--;
    }
    if (walltexIndex == (int)texstrs.size())
    {
      walltexIndex = 0;
    }
    if (walltexIndex == -1)
    {
      walltexIndex = texstrs.size() - 1;
    }
    walltex = texstrs[walltexIndex];
    delete walltexDisplay;
    walltexDisplay = new ui(renderer, walltex.c_str(), 0.1, 0, 0.1, 0.1, 0);
  }

  if (devinput[18] && !olddevinput[18])
  {
    if (!g_holdingCTRL)
    {
      captexIndex++;
    }
    else
    {
      captexIndex--;
    }
    if (captexIndex == (int)texstrs.size())
    {
      captexIndex = 0;
    }
    if (captexIndex == -1)
    {
      captexIndex = texstrs.size() - 1;
    }
    captex = texstrs[captexIndex];
    delete captexDisplay;
    captexDisplay = new ui(renderer, captex.c_str(), 0.2, 0, 0.1, 0.1, 0);
  }

  // make collision-zone
  if (devinput[19] && !olddevinput[19] && makingbox == 0)
  {

    lx = px;
    ly = py;
    makingbox = 1;
    selection->image = IMG_Load("resources/engine/collisionzone.qoi");
    selection->texture = SDL_CreateTextureFromSurface(renderer, selection->image);
    SDL_FreeSurface(selection->image);
  }
  else
  {
    if (devinput[19] && !olddevinput[19] && makingbox == 1)
    {
      if (makingbox)
      {
        makingbox = 0;
        collisionZone *a = new collisionZone(selection->x, selection->y, selection->width, selection->height);
        a->inviteAllGuests();
      }
    }
  }

  if (devinput[22] && !olddevinput[22])
  {
    ramp *r = new ramp(marker->x, marker->y, wallstart / 64, 0, walltex, captex);
    mapObject *child;
    int tiltstep = g_TiltResolution;
    for (int i = 0; i < 64; i += tiltstep)
    {
      // make a strip of captex
      child = new mapObject(renderer, r->captexture, "&", r->x, r->y - ((float)i * 55.0 / 64.0) + 55, r->layer * 64 + i, 64, tiltstep + 2, 0, 0);
      r->children.push_back(child);
    }
  }

  if (devinput[23] && !olddevinput[23])
  {
    ramp *r = new ramp(marker->x, marker->y, wallstart / 64, 1, walltex, captex);
    mapObject *child;
    int tiltstep = g_TiltResolution;
    for (int i = 0; i < 64; i += tiltstep)
    {
      // make a strip of captex
      child = new mapObject(renderer, r->captexture, "&", r->x + i, r->y + 55, r->layer * 64 + i, tiltstep, 55, 0, 0);
      r->children.push_back(child);
    }
    // wall
    child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
    r->children.push_back(child);
  }

  if (devinput[24] && !olddevinput[24])
  {
    ramp *r = new ramp(marker->x, marker->y, wallstart / 64, 2, walltex, captex);
    mapObject *child;
    int tiltstep = g_TiltResolution;
    for (int i = 0; i < 55; i += tiltstep)
    {
      // make a strip of captex
      child = new mapObject(renderer, r->captexture, "&", r->x, r->y - i + 55, r->layer * 64 + (64 - (i * (64 / 55))), 64, tiltstep, 0, 0);
      r->children.push_back(child);
    }
    // wall
    child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
    r->children.push_back(child);
  }

  if (devinput[25] && !olddevinput[25])
  {
    ramp *r = new ramp(marker->x, marker->y, wallstart / 64, 3, walltex, captex);
    mapObject *child;
    int tiltstep = g_TiltResolution;
    for (int i = 0; i < 64; i += tiltstep)
    {
      // make a strip of captex
      child = new mapObject(renderer, r->captexture, "&", r->x + i, r->y + 55, r->layer * 64 + (64 - i), tiltstep, 55, 0, 0);
      r->children.push_back(child);
    }
    child = new mapObject(renderer, r->walltexture, "&", r->x, r->y + 55, r->layer * 64, 64, 32, 1, 0);
    r->children.push_back(child);
  }

  // update position of markerz
  markerz->x = marker->x;
  float boffset = wallheight;
  boffset /= 2;
  markerz->y = marker->y - boffset;
  markerz->width = marker->width;
  markerz->height = marker->height;
  markerz->wraptexture = 0;

  marker->y -= wallstart / 2;
  // done to prevent double keypresses
  for (int i = 0; i < 50; i++)
  {
    olddevinput[i] = devinput[i];
  }
  }

void close_map_writing()
{
  ofile.close();
}


#endif
