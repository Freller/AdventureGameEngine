#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <stdio.h>
#include <string>
#include <limits>
#include <stdlib.h>

#include "globals.h"
#include "objects.h"
#include "specialobjects.h"

//shared cooldown data
float cannonCooldown = 0;
int cannonEvent = 0;
int cannonToggleEvent = 0;
float cannonCooldownM = 800;
int cannonToggleTwo = 0;

float shortSpikesCooldown = 0;
int shortSpikesState = 0;
int lastShortSpikesState = -1;

void specialObjectsInit(entity* a) {
  switch(a->identity) {
    case 1:
    {
      //pellet
      g_pellets.push_back(a);
      a->bounceindex = rand() % 8;
      a->wasPellet = 1;
      a->CalcDynamicForOneFrame = 1;

      break;
    }
    case 2: 
    {
      //spiketrap
      for(auto entry:a->spawnlist) {
        entry->visible = 0;
      }
      a->maxCooldownA = 1300;
      a->maxCooldownB = 1300;
      break;
    }
    case 3:
    {
      //cannon
      for(auto entry:a->spawnlist) {
        entry->visible = 0;
      }
      break;
    }
    case 6:
    {
      //smarttrap
      a->poiIndex = 10;
      break;
    }
    case 8:
    {
      //psychotrap
      a->parent->visible = 0;
      for(auto entry : a->spawnlist) {
        entry->visible = 1;
        entry->msPerFrame = 2;
        entry->loopAnimation = 1;
        entry->scriptedAnimation = 1;
      }
      break;
    }
    case 9:
    {
      //inventory chest

      break;
    }
    case 10:
    {
      //corkboard

      break;
    }
    case 11:
    {
      //lever
      
      //search for door
      for(auto x: g_entities) {
        if(x->identity == 12) {
          char b = x->name.at(x->name.size() - 1);
          char c = a->name.at(a->name.size() - 1);
          if(c == b) {
            a->spawnlist.push_back(x);
          }
        }

      }
      break;
    }
    case 12:
    {
      //door
      
      //search for lever
      for(auto x: g_entities) {
        if(x->identity == 11) {
          char c = x->name.at(x->name.size() - 1);
          char b = a->name.at(a->name.size() - 1);
          if(c == b) {
            x->spawnlist.push_back(a);
          }
        }

      }

      break;
    }
    case 13:
    {
      //braintrap
      a->spawnlist[0]->visible = 0;
      ribbon* zap = new ribbon();
      zap->texture = a->spawnlist[0]->texture;
      zap->sortingOffset = 500;
      a->actorlist.push_back(zap);

      break;
    }
    case 14:
    {
      //basic firetrap

      a->spawnlist[0]->msPerFrame = 70;
      a->spawnlist[0]->loopAnimation = 1;
      a->spawnlist[0]->scriptedAnimation = 1;

      a->spawnlist[1]->msPerFrame = 70;
      a->spawnlist[1]->loopAnimation = 1;
      a->spawnlist[1]->scriptedAnimation = 1;

      a->spawnlist[2]->msPerFrame = 70;
      a->spawnlist[2]->loopAnimation = 1;
      a->spawnlist[2]->scriptedAnimation = 1;
      break;
    }
    case 15:
    {
      //fast firetrap

      a->spawnlist[0]->msPerFrame = 70;
      a->spawnlist[0]->loopAnimation = 1;
      a->spawnlist[0]->scriptedAnimation = 1;

      a->spawnlist[1]->msPerFrame = 70;
      a->spawnlist[1]->loopAnimation = 1;
      a->spawnlist[1]->scriptedAnimation = 1;

      a->spawnlist[2]->msPerFrame = 70;
      a->spawnlist[2]->loopAnimation = 1;
      a->spawnlist[2]->scriptedAnimation = 1;

      a->spawnlist[3]->msPerFrame = 70;
      a->spawnlist[3]->loopAnimation = 1;
      a->spawnlist[3]->scriptedAnimation = 1;
      break;
    }
    case 16:
    {
      //long firetrap

      a->spawnlist[0]->msPerFrame = 70;
      a->spawnlist[0]->loopAnimation = 1;
      a->spawnlist[0]->scriptedAnimation = 1;

      a->spawnlist[1]->msPerFrame = 70;
      a->spawnlist[1]->loopAnimation = 1;
      a->spawnlist[1]->scriptedAnimation = 1;

      a->spawnlist[2]->msPerFrame = 70;
      a->spawnlist[2]->loopAnimation = 1;
      a->spawnlist[2]->scriptedAnimation = 1;

      a->spawnlist[3]->msPerFrame = 70;
      a->spawnlist[3]->loopAnimation = 1;
      a->spawnlist[3]->scriptedAnimation = 1;

      a->spawnlist[4]->msPerFrame = 70;
      a->spawnlist[4]->loopAnimation = 1;
      a->spawnlist[4]->scriptedAnimation = 1;

      a->spawnlist[5]->msPerFrame = 70;
      a->spawnlist[5]->loopAnimation = 1;
      a->spawnlist[5]->scriptedAnimation = 1;

      a->spawnlist[6]->msPerFrame = 70;
      a->spawnlist[6]->loopAnimation = 1;
      a->spawnlist[6]->scriptedAnimation = 1;
      break;
    }
    case 17:
    {
      //double firetrap

      a->spawnlist[0]->msPerFrame = 70;
      a->spawnlist[0]->loopAnimation = 1;
      a->spawnlist[0]->scriptedAnimation = 1;

      a->spawnlist[1]->msPerFrame = 70;
      a->spawnlist[1]->loopAnimation = 1;
      a->spawnlist[1]->scriptedAnimation = 1;

      a->spawnlist[2]->msPerFrame = 70;
      a->spawnlist[2]->loopAnimation = 1;
      a->spawnlist[2]->scriptedAnimation = 1;

      a->spawnlist[3]->msPerFrame = 70;
      a->spawnlist[3]->loopAnimation = 1;
      a->spawnlist[3]->scriptedAnimation = 1;

      a->spawnlist[4]->msPerFrame = 70;
      a->spawnlist[4]->loopAnimation = 1;
      a->spawnlist[4]->scriptedAnimation = 1;

      a->spawnlist[5]->msPerFrame = 70;
      a->spawnlist[5]->loopAnimation = 1;
      a->spawnlist[5]->scriptedAnimation = 1;
      break;
    }
    case 18:
    {
      //smart firetrap
      
      a->spawnlist[0]->visible = 0;

      //start by facing protag
      float angleToProtag = atan2(protag->getOriginX() - a->getOriginX(), protag->getOriginY() - a->getOriginY()) - M_PI/2;
      angleToProtag = wrapAngle(angleToProtag);
      a->steeringAngle = angleToProtag - M_PI/2 - g_ft_p/2;

      break;
    }
    case 19:
    {
      //fast smart firetrap

      break;
    }
    case 20:
    {
      //collectible familiar

      break;
    }
    case 21:
    {
      //shortspiketrap
      for(auto entry:a->spawnlist) {
        entry->visible = 0;
      }

      a->flagB = 0;

      break;
    }
    case 22:
    {
      //dungeon door
    }
    case 23:
    {
      //firering
      for(auto x : a->spawnlist) {
        x->msPerFrame = 70;
        x->loopAnimation = 1;
        x->scriptedAnimation = 1;
        x->visible = 1;
        x->animation = 0;
      }
      break;
    }
    case 24:
    {
      //fireball
      a->msPerFrame = 70;
      a->loopAnimation = 1;
      a->scriptedAnimation = 1;
      a->visible = 1;
      a->animation = 0;

      break;
    }
    case 25:
    {
      //dungeon lock

      break;
    }
    case 28:
    {
      //crushing crate
      a->flagA = 0; //search for crate
      

      break;
    }
    case 29:
    {
      //crushable
      a->data[0] = 0;
      a->flagA = 0; //search for crate
      a->cooldownA = a->bounds.width;
      a->cooldownB = a->bounds.height;
      a->spawnlist[0]->setOriginX(0);
      a->spawnlist[0]->setOriginY(0);

      break;
    }
    case 30:
    {
      a->msPerFrame = 50;
      a->scriptedAnimation = 0;
      a->animation = 0;
      a->loopAnimation = 0;
      break;
    }
    case 31:
    {
      //father washing dishes
      a->flagA = 5000;
      break;
    }
    case 32:
    {
      //flamering-alt
      for(auto x : a->spawnlist) {
        x->msPerFrame = 70;
        x->loopAnimation = 1;
        x->scriptedAnimation = 1;
        x->visible = 1;
        x->animation = 0;
      }
      break;

    }
    case 33:
    {
      //natural's door, the shine ent

      break;
    }

    case 100:
    {
      //zombie
      a->spawnlist[0]->visible = 0;
      a->maxCooldownA = 300; //this is the time it takes to get from the start to the end of the bite animation
      a->aggressiveness = exponentialCurve(60, 10);
      a->aggressiveness += exponentialCurve(15, 3);
      a->poiIndex = 0;

      break;
    }
    case 101:
    {
      //disaster
      a->agrod = 1;
      a->target = protag;
      a->traveling = 0;
      a->spawnlist[0]->visible = 0;
      a->aggressiveness = exponentialCurve(60, 10);
      break;
    }
    case 102:
    {
      //creep
      a->aggressiveness = exponentialCurve(60, 10);
      a->spawnlist[0]->visible = 0;
      a->activeState = 0;
      break;

    }
    case 103:
    {
      //fnomunon
      a->traveling = 1;
      break;
    }

  }
}

void specialObjectsBump(entity* a, bool xcollide, bool ycollide) {
  switch(a->identity) {
    case 5:
    {
      //bladetrap
      if(xcollide || ycollide) {
        float offset = 50;
        if(!a->flagA) {
          offset = -50;
        }
        float xoff = offset * cos(a->steeringAngle);
        float yoff = -offset * sin(a->steeringAngle);
        sparksEffect->happen(a->getOriginX() + xoff, a->getOriginY() + yoff, a->z + 25, 0);
        g_lastParticleCreated->sortingOffset = 50;
        if(a->cooldownA > a->maxCooldownA) {
          a->cooldownA = 0;
          a->flagA = !a->flagA;
        }
  
      }
      break;
    }
    case 19:
    {
      //bouncetrap
      
      if((xcollide || ycollide) && a->cooldownA < 0) {
        if(a->flagA) {
          a->targetSteeringAngle -= M_PI/2;
        } else {
          a->targetSteeringAngle += M_PI/2;
        }
        a->flagA = !a->flagA;

        a->targetSteeringAngle = wrapAngle(a->targetSteeringAngle);
        a->steeringAngle = a->targetSteeringAngle;
        a->cooldownA = 1000;
      }
      a->forwardsVelocity = 49;
       
      break;
    }
  }
  
}

void specialObjectsUpdate(entity* a, float elapsed) {
  switch(a->identity) {
    case 2: 
    {
      //spiketrap
      if(a->flagA == 0) {
        if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
          a->flagA = 1;
          a->cooldownA = 0;
        }
      }
      if(a->flagA == 1) {
        //wait a second, and then extend
        a->cooldownA += elapsed;
        if(a->cooldownA > a->maxCooldownA) {
          a->cooldownB = 0;
          //show spikes
          for(auto entry : a->spawnlist) {
            entry->frameInAnimation = 0;
            entry->loopAnimation = 0;
            entry->msPerFrame = 50;
            entry->scriptedAnimation = 1;
            entry->reverseAnimation = 0;
            entry->visible = 1;
          }
          a->flagA = 2;
          a->flagB = 0;
          a->cooldownA = 0;
        }
    
      }
      if(a->flagA == 2) {
        if(!a->flagB) {
          rect changeMe = a->getMovedBounds();
          changeMe.zeight = 32;
          if(RectOverlap3d(changeMe, protag->getMovedBounds())) {
            hurtProtag(1);
          }
        }
    
        a->cooldownA += elapsed;
        if(a->cooldownA > a->maxCooldownB) {
          a->cooldownB = 0;
          //hide spikes
          for(auto entry : a->spawnlist) {
            entry->frameInAnimation = 1;
            entry->loopAnimation = 0;
            entry->msPerFrame = 50;
            entry->scriptedAnimation = 1;
            entry->reverseAnimation = 1;
          }
          a->flagA = 3;
          a->cooldownA = 0;
        }
    
      }
      if(a->flagA == 3) {
        a->cooldownA += elapsed;
        if(a->cooldownA > 125) {
          for(auto entry : a->spawnlist) {
            entry->visible = 0;
          }
          a->flagA = 0;
          a->cooldownA = 0;
        }
    
      }
      break;
    } 
    case 3: 
    {
      //cannon
      if(cannonEvent) {
        entity *copy = new entity(renderer, a->spawnlist[0]);
        copy->z = a->z + 10;
        copy->dontSave = 1;
        copy->usingTimeToLive = 1;
        copy->timeToLiveMs = 8000;
        copy->steeringAngle = a->steeringAngle;
        copy->targetSteeringAngle = a->steeringAngle;
        copy->missile = 1;
        copy->visible = 1;
        copy->fragileMovement = 1;
        copy->msPerFrame = 70;
        copy->loopAnimation = 1;
        copy->useGravity = 0;
        copy->identity = 4;
         
        float offset = 50;
        float yoff = -offset * sin(a->steeringAngle);
        float xoff = offset * cos(a->steeringAngle);
    
        copy->setOriginX(a->getOriginX() + xoff);
        copy->setOriginY(a->getOriginY() + yoff);
        
    
        if(cannonToggleEvent == 1 || cannonToggleEvent == 3) {
          blackSmokeEffect->happen(a->getOriginX() + xoff, a->getOriginY() + yoff, a->z + 40, a->steeringAngle);
        }
      }
      break;
    } 
    case 4: 
    {
      //cannonball
      if(RectOverlap3d(a->getMovedBounds(), protag->getMovedBounds())) {
        a->timeToLiveMs = -1;
        hurtProtag(1);
      }
    } 
    case 5: 
    {
      //bladetrap
      a->cooldownA += elapsed;
      a->cooldownB += elapsed;
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownB)
      {
        a->cooldownA = 0;
        hurtProtag(1);
    
      }

      if(a->flagA) {
        a->forwardsVelocity = a->xagil;
      } else {
        a->forwardsVelocity = -a->xagil;
      }
      a->steeringAngle = a->targetSteeringAngle;
      break;
    } 
    case 6: 
    {
      //smarttrap
    
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && a->cooldownA > a->maxCooldownA)
      {
        a->cooldownA = 0;
        hurtProtag(1);
      }

      navNode* hdest = (navNode*)g_setsOfInterest.at(a->poiIndex)[a->flagA];

      a->readyForNextTravelInstruction = (hdest != nullptr && XYWorldDistance(a->getOriginX(), a->getOriginY(), hdest->x, hdest->y) < 32);

      if(a->readyForNextTravelInstruction) {
        a->flagA ++;
        if(a->flagA >= g_setsOfInterest.at(a->poiIndex).size()) {
          a->flagA = 0;
        }
      }


      float angleToTarget = atan2(hdest->x - a->getOriginX(), hdest->y - a->getOriginY()) - M_PI/2;
      
      a->targetSteeringAngle = angleToTarget;

      a->xaccel = cos(a->steeringAngle) * a->xmaxspeed;
      a->yaccel = -sin(a->steeringAngle) * a->xmaxspeed;

      a->xvel += a->xaccel * ((double) elapsed / 256.0);
      a->yvel += a->yaccel * ((double) elapsed / 256.0);

      a->x += a->xvel * ((double) elapsed / 256.0);
      a->y += a->yvel * ((double) elapsed / 256.0);
    
      break;
    } 
    case 7: 
    {
      //facetrap
      float dist = XYWorldDistanceSquared(a->getOriginX(), a->getOriginY(), protag->getOriginX(), protag->getOriginY());
      const float Dist = 16384;
      const float minDist = -2000;
      if(dist > Dist) {
        a->opacity = 0;
    
      } else {
        a->opacity = 255.0 * (1 -((dist + minDist) / (Dist +minDist)));
        if(a->opacity > 255) {a->opacity = 255;}
        //SDL_SetTextureAlphaMod(texture, a-opacity);
      }
      break;
    } 
    case 8: 
    {
      a->cooldownB -= elapsed;
      //psychotrap
      if(CylinderOverlap(protag->getMovedBounds(), a->getMovedBounds())) {
        //show graphic
        for(auto entry : a->spawnlist) {
          entry->visible = 1;
        }
    
        if(CylinderOverlap(a->parent->getMovedBounds(), protag->getMovedBounds()) && a->cooldownB <= 0) {
          hurtProtag(1);
          a->cooldownB = 1300;
        }
    
      } else {
        //hide graphic
        for(auto entry : a->spawnlist) {
          entry->visible = 0;
        }
      }
      break;
    } 
    case 13:
    {
      //braintrap
      float dist = XYWorldDistanceSquared(a->getOriginX(), a->getOriginY(), protag->getOriginX(), protag->getOriginY());
      const float Dist = 36864;

      a->cooldownA -= elapsed;
      a->cooldownB -= elapsed;
      if(dist < Dist) {
        if(a->cooldownA < 0) {
          a->cooldownA = 3000;
          a->cooldownB = 100;
          hurtProtag(1);
          ribbon* zap = ((ribbon*)a->actorlist[0]);
      
          zap->x1 = protag->getOriginX();
          zap->x = zap->x1;
          zap->y1 = protag->getOriginY();
          zap->y = zap->y1;
          zap->z1 = protag->z + 100;
      
          zap->x2 = a->getOriginX();
          zap->y2 = a->getOriginY();
          zap->z2 = a->z + 160;
        }
      }

      if(a->cooldownB < 0) {
        a->actorlist[0]->visible = 0;
      } else {
        a->actorlist[0]->visible = 1;
      }
      



      break;
    }
    case 14:
    {
      //basic firetrap
      a->steeringAngle += 0.01;
      a->animation = 0;

      float angleToUse = fmod(a->steeringAngle, M_PI);

      a->frameInAnimation = -1;
      for(int i = 0; i < g_ft_angles.size(); i++) {
        if(angleToUse < g_ft_angles[i]) {
          a->frameInAnimation = g_ft_frames[i];
          if(g_ft_flipped[i]) {
            a->flip = SDL_FLIP_HORIZONTAL;
          } else {
            a->flip = SDL_FLIP_NONE;
          }
          break;
        }
      }
      if(a->frameInAnimation == -1) {
        a->frameInAnimation = 0;
        a->flip = SDL_FLIP_NONE;
      }

      //update fireballs
      float offset = 1;
      float yoff = -offset * sin(a->steeringAngle + g_ft_p/2 + M_PI/2);
      float xoff = offset * cos(a->steeringAngle + g_ft_p/2 + M_PI/2);

      const int dist = 60;

      a->spawnlist[0]->setOriginX(a->getOriginX() + (xoff*dist));
      a->spawnlist[0]->setOriginY(a->getOriginY() + (yoff*dist));

      a->spawnlist[1]->setOriginX(a->getOriginX() + (xoff*dist*2));
      a->spawnlist[1]->setOriginY(a->getOriginY() + (yoff*dist*2));

      a->spawnlist[2]->setOriginX(a->getOriginX() + (xoff*dist*3));
      a->spawnlist[2]->setOriginY(a->getOriginY() + (yoff*dist*3));


      break;
    }
    case 15:
    {
      //fast firetrap
      a->steeringAngle += 0.02;
      a->animation = 0;

      float angleToUse = fmod(a->steeringAngle, M_PI);

      a->frameInAnimation = -1;
      for(int i = 0; i < g_ft_angles.size(); i++) {
        if(angleToUse < g_ft_angles[i]) {
          a->frameInAnimation = g_ft_frames[i];
          if(g_ft_flipped[i]) {
            a->flip = SDL_FLIP_HORIZONTAL;
          } else {
            a->flip = SDL_FLIP_NONE;
          }
          break;
        }
      }
      if(a->frameInAnimation == -1) {
        a->frameInAnimation = 0;
        a->flip = SDL_FLIP_NONE;
      }

      //update fireballs
      float offset = 1;
      float yoff = -offset * sin(a->steeringAngle + g_ft_p/2 + M_PI/2);
      float xoff = offset * cos(a->steeringAngle + g_ft_p/2 + M_PI/2);

      const int dist = 60;

      a->spawnlist[0]->setOriginX(a->getOriginX() + (xoff*dist));
      a->spawnlist[0]->setOriginY(a->getOriginY() + (yoff*dist));

      a->spawnlist[1]->setOriginX(a->getOriginX() + (xoff*dist*2));
      a->spawnlist[1]->setOriginY(a->getOriginY() + (yoff*dist*2));

      a->spawnlist[2]->setOriginX(a->getOriginX() + (xoff*dist*3));
      a->spawnlist[2]->setOriginY(a->getOriginY() + (yoff*dist*3));

      a->spawnlist[3]->setOriginX(a->getOriginX() + (xoff*dist*4));
      a->spawnlist[3]->setOriginY(a->getOriginY() + (yoff*dist*4));


      break;
    }
    case 16:
    {
      //long firetrap
      a->steeringAngle -= 0.01;;
      a->animation = 0;

      float angleToUse = fmod(a->steeringAngle, M_PI);

      a->frameInAnimation = -1;
      for(int i = 0; i < g_ft_angles.size(); i++) {
        if(angleToUse < g_ft_angles[i]) {
          a->frameInAnimation = g_ft_frames[i];
          if(g_ft_flipped[i]) {
            a->flip = SDL_FLIP_HORIZONTAL;
          } else {
            a->flip = SDL_FLIP_NONE;
          }
          break;
        }
      }
      if(a->frameInAnimation == -1) {
        a->frameInAnimation = 0;
        a->flip = SDL_FLIP_NONE;
      }

      //update fireballs
      float offset = 1;
      float yoff = -offset * sin(a->steeringAngle + g_ft_p/2 + M_PI/2);
      float xoff = offset * cos(a->steeringAngle + g_ft_p/2 + M_PI/2);

      const int dist = 60;

      a->spawnlist[0]->setOriginX(a->getOriginX() + (xoff*dist));
      a->spawnlist[0]->setOriginY(a->getOriginY() + (yoff*dist));

      a->spawnlist[1]->setOriginX(a->getOriginX() + (xoff*dist*2));
      a->spawnlist[1]->setOriginY(a->getOriginY() + (yoff*dist*2));

      a->spawnlist[2]->setOriginX(a->getOriginX() + (xoff*dist*3));
      a->spawnlist[2]->setOriginY(a->getOriginY() + (yoff*dist*3));

      a->spawnlist[3]->setOriginX(a->getOriginX() + (xoff*dist*4));
      a->spawnlist[3]->setOriginY(a->getOriginY() + (yoff*dist*4));
      
      a->spawnlist[4]->setOriginX(a->getOriginX() + (xoff*dist*5));
      a->spawnlist[4]->setOriginY(a->getOriginY() + (yoff*dist*5));

      a->spawnlist[5]->setOriginX(a->getOriginX() + (xoff*dist*6));
      a->spawnlist[5]->setOriginY(a->getOriginY() + (yoff*dist*6));
      
      a->spawnlist[6]->setOriginX(a->getOriginX() + (xoff*dist*7));
      a->spawnlist[6]->setOriginY(a->getOriginY() + (yoff*dist*7));

      

      break;
    }

    case 17:
    {
      //double firetrap
      a->steeringAngle -= 0.02;
      a->animation = 0;

      float angleToUse = fmod(a->steeringAngle, M_PI);

      a->frameInAnimation = -1;
      for(int i = 0; i < g_ft_angles.size(); i++) {
        if(angleToUse < g_ft_angles[i]) {
          a->frameInAnimation = g_ft_frames[i];
          if(g_ft_flipped[i]) {
            a->flip = SDL_FLIP_HORIZONTAL;
          } else {
            a->flip = SDL_FLIP_NONE;
          }
          break;
        }
      }
      if(a->frameInAnimation == -1) {
        a->frameInAnimation = 0;
        a->flip = SDL_FLIP_NONE;
      }

      //update fireballs
      float offset = 1;
      float yoff = -offset * sin(a->steeringAngle + g_ft_p/2 + M_PI/2);
      float xoff = offset * cos(a->steeringAngle + g_ft_p/2 + M_PI/2);

      const int dist = 60;
      const int coff = 10;

      a->spawnlist[0]->setOriginX(a->getOriginX() + (xoff*(dist*1 + coff)));
      a->spawnlist[0]->setOriginY(a->getOriginY() + (yoff*(dist*1 + coff)));

      a->spawnlist[1]->setOriginX(a->getOriginX() + (xoff*(dist*2 + coff)));
      a->spawnlist[1]->setOriginY(a->getOriginY() + (yoff*(dist*2 + coff)));
      
      a->spawnlist[2]->setOriginX(a->getOriginX() + (xoff*(dist*3 + coff)));
      a->spawnlist[2]->setOriginY(a->getOriginY() + (yoff*(dist*3 + coff)));

      a->spawnlist[3]->setOriginX(a->getOriginX() + (xoff*(dist*-1 - coff)));
      a->spawnlist[3]->setOriginY(a->getOriginY() + (yoff*(dist*-1 - coff)));

      a->spawnlist[4]->setOriginX(a->getOriginX() + (xoff*(dist*-2 - coff)));
      a->spawnlist[4]->setOriginY(a->getOriginY() + (yoff*(dist*-2 - coff)));
      
      a->spawnlist[5]->setOriginX(a->getOriginX() + (xoff*(dist*-3 - coff)));
      a->spawnlist[5]->setOriginY(a->getOriginY() + (yoff*(dist*-3 - coff)));

      break;
    }
    case 18:
    {
      //smart firetrap
      float angleToProtag = atan2(protag->getOriginX() - a->getOriginX(), protag->getOriginY() - a->getOriginY()) - M_PI/2;
      angleToProtag = wrapAngle(angleToProtag);
      a->targetSteeringAngle = angleToProtag - M_PI/2 - g_ft_p/2;

      float angleToUse = fmod(a->steeringAngle, M_PI);
      a->animation = 0;

      a->frameInAnimation = -1;
      for(int i = 0; i < g_ft_angles.size(); i++) {
        if(angleToUse < g_ft_angles[i]) {
          a->frameInAnimation = g_ft_frames[i];
          if(g_ft_flipped[i]) {
            a->flip = SDL_FLIP_HORIZONTAL;
          } else {
            a->flip = SDL_FLIP_NONE;
          }
          break;
        }
      }
      if(a->frameInAnimation == -1) {
        a->frameInAnimation = 0;
        a->flip = SDL_FLIP_NONE;
      }

      
      a->cooldownA += elapsed;
      if(a->cooldownA > 3000) {
        a->cooldownA = 0;
        entity *copy = new entity(renderer, a->spawnlist[0]);
        copy->z = a->z + 10;
        copy->dontSave = 1;
        copy->xmaxspeed = 150;
        copy->baseMaxSpeed = 150;
        copy->xagil = 150;
        copy->usingTimeToLive = 1;
        copy->timeToLiveMs = 8000;
        copy->steeringAngle = a->steeringAngle + M_PI/2 + g_ft_p/2;
        copy->targetSteeringAngle = a->steeringAngle + M_PI/2 + g_ft_p/2;
        copy->missile = 1;
        copy->visible = 1;
        copy->fragileMovement = 1;
        copy->msPerFrame = 70;
        copy->loopAnimation = 1;
        copy->useGravity = 0;
        copy->identity = 4;
        a->cooldownA = 0;

        float offset = 50;
        float yoff = -offset * sin(a->steeringAngle + M_PI/2 + g_ft_p/2);
        float xoff = offset * cos(a->steeringAngle + M_PI/2 + g_ft_p/2);
    
        copy->setOriginX(a->getOriginX() + xoff);
        copy->setOriginY(a->getOriginY() + yoff);

      }

      break;
    }
    case 19:
    {
      //bouncetrap
      //a->forwardsVelocity = a->xmaxspeed;
      a->cooldownA -= elapsed;
      D(a->cooldownA);
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()))
      {
        //a->cooldownA = 0;
        hurtProtag(1);
    
      }
      break;
    }
    case 20:
    {
      //collectible familiar
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()))
      {
        if(!a->flagA) {
          a->dynamic = 0;
          a->flagA = 1;
          g_familiars.push_back(a);
          g_chain_time = 1000;
        }

        //check familiars to see if we should combine any
        entity* first = 0;
        entity* second = 0;
        entity* third = 0;
        for(auto x : g_familiars) {
          int count = 1;
          for(auto y : g_familiars) {
            if(y == x) {break;}
            if(y->name.substr(0, y->name.find('-')) == x->name.substr(0, x->name.find('-'))) {
              count ++;
              first = x;
              if(count == 2) { second = y; }
              if(count == 3) { third = y; break; }
            }


          }
          if(count == 3) {
            //combine these familiars
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), x), g_familiars.end());
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), second), g_familiars.end());
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), third), g_familiars.end());
            g_combineFamiliars.push_back(x);
            g_combineFamiliars.push_back(second);
            g_combineFamiliars.push_back(third);

            g_familiarCombineX = (x->getOriginX() + second->getOriginX() + third->getOriginX()) /3;
            g_familiarCombineY = (x->getOriginY() + second->getOriginY() + third->getOriginY()) / 3;

            g_combinedFamiliar = new entity(renderer, x->name.substr(0, x->name.find('-')) + "-full");
            g_combinedFamiliar->x = 0;
            g_chain_time = 0;
            x->darkenMs = 300;
            x->darkenValue = 255;
            second->darkenMs = 300;
            x->darkenValue = 255;
            third->darkenMs = 300;
            x->darkenValue = 255;
          }


        }

      }
      a->cooldownA -= elapsed;
      break;
    }
    case 21:
    {
      //shortspiketrap
      if(shortSpikesState == 1) {
        //wait a second, and then extend
        a->cooldownA += elapsed;
        //show spikes
        for(auto entry : a->spawnlist) {
          entry->frameInAnimation = 0;
          entry->loopAnimation = 0;
          entry->msPerFrame = 50;
          entry->scriptedAnimation = 1;
          entry->reverseAnimation = 0;
          entry->visible = 1;
        }
      }

      if(lastShortSpikesState == 1) {
        rect changeMe = a->getMovedBounds();
        changeMe.zeight = 32;
        if(RectOverlap3d(changeMe, protag->getMovedBounds())) {
          hurtProtag(1);
          a->flagB = 1;
        }
      }

      if(shortSpikesState == 4) {
        //hide spikes
        for(auto entry : a->spawnlist) {
          entry->frameInAnimation = 1;
          entry->loopAnimation = 0;
          entry->msPerFrame = 50;
          entry->scriptedAnimation = 1;
          entry->reverseAnimation = 1;
        }
        a->flagB = 0;
      }

      if(shortSpikesState == 3) {
        for(auto entry : a->spawnlist) {
          entry->visible = 0;
        }
      }
      break;
    } 
    case 22:
    {
      //dungeon door
      break;
    }
    case 23:
    {
      //firering flamering
      a->steeringAngle += 0.01;

      const int dist = 160;

      float angleToUse = a->steeringAngle;

      for(auto x : a->spawnlist) {
        angleToUse += M_PI / 2;
        angleToUse = wrapAngle(angleToUse);
        float offset = dist;
        float yoff = -offset * sin(angleToUse);
        float xoff = offset * cos(angleToUse);
  
        x->setOriginX(a->getOriginX() + (xoff));
        x->setOriginY(a->getOriginY() + (yoff));
      }
      break;
    }
    case 24:
    {
      //fireball
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds())) {
        hurtProtag(1);
      }

      break;
    }
    case 25:
    {
      //dungeon lock

      break;
    }
    case 26:
    {
      //prop blocker
      
      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds())) {
        //push this one slightly away from x
        float r = pow( max(Distance(protag->getOriginX(), protag->getOriginY(), a->getOriginX(), a->getOriginY()), (float)10.0 ), 2);
        float mag =  10000/r;
        float xdif = (protag->getOriginX() - a->getOriginX());
        float ydif = (protag->getOriginY() - a->getOriginY());
        float len = pow( pow(xdif, 2) + pow(ydif, 2), 0.5);
        float normx = xdif/len;
        float normy = ydif/len;
  
        if(!isnan(mag * normx) && !isnan(mag * normy)) {
          protag->x += normx * mag;
          protag->y += normy * mag;

        }
      }

      break;
    }
    case 27:
    {
      //slime puddle

      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds())) {
        protag->hisStatusComponent.slown.addStatus(100, 0.5);
      }

      break;
    }
    case 28:
    {
      //pushable heavy crate
      if(a->flagA == 0) {
        for(auto &x : g_entities) {
          if(x->identity == 29) {
            a->spawnlist.push_back(x);
            a->flagA = 1;
          }
        }
      }
      
      //left box
      rect bounds = a->getMovedBounds();
      int vx = 0;
      int vy = 0;

      rect left = bounds;
      left.width = 5;
      left.x -= 5;

      rect right = bounds;
      right.x += right.width;
      right.width = 5;

      rect up = bounds;
      up.y += up.height;
      up.height = 5;

      rect down = bounds;
      down.y -= 5;
      down.height = 5;


      if(RectOverlap3d(protag->getMovedBounds(), left)) {
        vx = 1;
      }

      if(RectOverlap3d(protag->getMovedBounds(), right)) {
        vx = -1;
      }

      if(RectOverlap3d(protag->getMovedBounds(), up)) {
        vy = -1;
      }

      if(RectOverlap3d(protag->getMovedBounds(), down)) {
        vy = 1;
      }
      float mag = 20;

      if(a->flagA) {
        //is a crushable ent in our way?
        rect forw = bounds;
        forw.x + vx * 2;
        forw.y + vy * 2;
        if(RectOverlap3d(a->spawnlist[0]->getMovedBounds(), forw)) {
          float rat = a->spawnlist[0]->orbitRange;

          if(rat != 0 && a->spawnlist[0]->tangible) {
            mag *= 1.8 - (rat*1.5);
          }
        }
      }

      vx *= mag;
      vy *= mag;

      a->xvel += vx;
      a->yvel += vy;
      break;
    }

    case 29:
    {
      //crushable entity
      
      //search for crate if we haven't found it yet
      if(a->flagA == 0) {
        for(auto &x : g_entities) {
          if(x->identity == 28) {
            a->spawnlist.push_back(x);
            a->flagA = 1;
            a->flagB = 0;
          }
        }
      }

      if(a->flagA == 1){ //check y crush
        auto r1 = a->getMovedBounds();
        auto r2 = a->spawnlist[1]->getMovedBounds();


        pair<float,float> L1 = pair<float,float>(r1.x, r1.y);
        pair<float,float> R1 = pair<float,float>(r1.x + r1.width, r1.y + r1.height);

        pair<float,float> L2 = pair<float,float>(r2.x, r2.y);
        pair<float,float> R2 = pair<float,float>(r2.x + r2.width, r2.y + r2.height);

        if(a->flagB == 0) { a->flagB = r1.width * r1.height;}

        float xdist = min(R1.first, R2.first) - max(L1.first, L2.first);
        float ydist = min(R1.second, R2.second) - max(L1.second, L2.second);

        float area = xdist * ydist;
        if(xdist < 0 || ydist <0) {
          area = 0;
        }

        float rat = area / a->flagB;

        float xrat = xdist / r1.width;
        float yrat = ydist / r1.height;


        //how should he respond to the crate?
        if(rat > 0.1) {
          a->data[0] = 1;
          //punishment
          punishValue = 6;
          punishValueDegrade = -0.1;
          if(xrat < yrat) {
            a->bounds.width -= 1;
          } else {
            a->bounds.height -= 1;
          }
        } else {
          //decompress
          if(a->bounds.width <= a->cooldownA) {
            a->bounds.width += 1;
          }

          if(a->bounds.height <= a->cooldownB) {
            a->bounds.height += 1;
          }
        }
        float xshmush = a->bounds.width;
        xshmush /= a->cooldownA;

        float yshmush = a->bounds.height;
        yshmush /= a->cooldownB;

        if(xshmush < 0.7 || yshmush < 0.7) {
          //punishment
          punishValue = 24;
          punishValueDegrade = -0.01;

          a->tangible = 0;
          a->spawnlist[0]->frameInAnimation = 0;
          a->spawnlist[0]->msPerFrame = 50;
          a->spawnlist[0]->setOriginX(a->getOriginX());
          a->spawnlist[0]->setOriginY(a->getOriginY());
          a->spawnlist[0]->curwidth = 0;
          a->spawnlist[0]->curheight = 0;
        }

        a->orbitRange = min(xshmush, yshmush);

        xshmush *= a->width;
        yshmush *= a->height;

        a->curwidth = xshmush;
        a->curheight = yshmush;

      }

      break;
    }
    case 30:
    {
      //corpse explosion | common/blood.ent
      break;
    }
    case 31:
    {
      //father washing dishes
      a->animate = 0;
      a->cooldownA -= elapsed;
      if(a->cooldownA < 0) {
        a->cooldownA = rng(3000, 1100);
        a->msPerFrame = 150;
        a->scriptedAnimation = 1;
        a->animation = 0;
      }

      if(a->msPerFrame > 0 && a->frameInAnimation == 0) {
        if(rng(0,1) == 0) {
          a->loopAnimation = 1;
        } else {
          a->loopAnimation = 0;
        }
      }

      break;
    }
    case 32:
    {
      // flamering-alt
      a->steeringAngle += 0.02;

      const int dist = 160;

      float angleToUse = a->steeringAngle;

      for(auto x : a->spawnlist) {
        angleToUse += M_PI / 2;
        angleToUse = wrapAngle(angleToUse);
        float offset = dist;
        float yoff = -offset * sin(angleToUse);
        float xoff = offset * cos(angleToUse);
  
        x->setOriginX(a->getOriginX() + (xoff));
        x->setOriginY(a->getOriginY() + (yoff));
      }
      break;
    }
    case 33:
    {
      a->animate = 0;
      a->msPerFrame = 32;
      a->scriptedAnimation = 1;
      a->animation = 0;
      break;

    }

   
    case 100: 
    {

      if(a->stunned) {break;}
      //zombie
      a->cooldownA -= elapsed;
      if(a->flagA && a->cooldownA < 0 ) {
        a->cooldownA = a->maxCooldownA;
        a->flagA = 0;
        a->msPerFrame = 0;
        a->cooldownB = 300;
        a->flagB = 1;
      }
    
      if(a->flagB) {
        a->cooldownB -= elapsed;
        if(a->cooldownB < 0) {
          a->frameInAnimation = 0;
          a->flagB = 0;
        }
      }
    
      if(a->flagC) {
        a->cooldownC -= elapsed;
        if(a->cooldownC < 0) {
          a->flagC = 0;
          a->spawnlist[0]->visible = 1;
          a->spawnlist[0]->frameInAnimation = 0;
          a->spawnlist[0]->scriptedAnimation = 1;
          a->spawnlist[0]->msPerFrame = 70;
          a->spawnlist[0]->loopAnimation = 0;
          a->flagD = 1;
          a->cooldownD = 300;
        }
    
      }
    
      if(a->flagD) {
        a->cooldownD -= elapsed;
        if(a->cooldownD < 0) {
          a->spawnlist[0]->visible = 0;
          a->flagD = 0;
    
    
          { //create hitbox
            hitbox* h = new hitbox();
      
            float offset = 120;
            float yoff = -offset * sin(a->steeringAngle);
            float xoff = offset * cos(a->steeringAngle);
      
            h->x = a->getOriginX() + xoff - 96;
            h->y = a->getOriginY() + yoff - 96;
            h->z = a->z;
        
            h->bounds.x = 0;
            h->bounds.y = 0;
            h->bounds.z = 20;
            h->bounds.width = 250;
            h->bounds.height = 250;
            h->bounds.zeight = 250;
        
            h->activeMS = 10;
            h->sleepingMS = 0;
          }
        }
      }
    
      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
          case 0:
            {
              { //only attack if it would hit
                
                float offset = 150;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
    
                rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
    
                if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
                  a->myAbilities[i].ready = 1;
                  break;
                }
                if(g_protagIsWithinBoardable) {
                  a->myAbilities[i].ready = 1;
                  break;
                }
              }
    
    
              a->cooldownA = a->maxCooldownA;
              a->flagA = 1;
    
              a->cooldownC = 200;
              a->flagC = 1;
              
    
              a->frameInAnimation = 0;
              a->scriptedAnimation = 1;
              a->msPerFrame = 65;
              a->loopAnimation = 0;
              
    
              break;
            }
          }
        }
      }
      break;
    } 
    case 101:
    {
      //disaster
      a->cooldownA -= elapsed;
      if(a->flagA) {
        a->frameInAnimation = 8;
        float angleToTarget = atan2(a->target->getOriginX() - a->getOriginX(), a->target->getOriginY() - a->getOriginY()) - M_PI/2;
        angleToTarget = wrapAngle(angleToTarget);
        a->steeringAngle = angleToTarget;
        a->targetSteeringAngle = angleToTarget;
      }
      if(a->flagA && a->cooldownA <= 0) {
        a->specialAngleOverride = 0;
        a->flagA = 0;
        a->msPerFrame = 0;
        a->scriptedAnimation = 0;
        a->spawnlist[0]->visible = 0;
        a->xmaxspeed = 2;
      }
    
      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->opacity < 255) { a->myAbilities[i].ready = 0;}
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
          case 0:
            {
              {
                float offset = 50;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
    
//                rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
//    
//                if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
//                  a->myAbilities[i].ready  = 1;
//                  a->myAbilities[i].cooldownMS  = 10;
//                  break;
//                }

                if(g_protagIsWithinBoardable) {
                  //kick the player out of the boardable and destroy it
                  //puzzle potential?
                  entity* destroyMe = g_boardedEntity;

                  smokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);
                  g_protagIsWithinBoardable = 0;
                  protag->steeringAngle = wrapAngle(-M_PI/2);
                  protag->animation = 4;
                  protag->animation = 4;
                  protag->flip = SDL_FLIP_NONE;
                  protag->xvel = 0;
                  //protag->yvel = 200;
                  protag->tangible = 1;
                  g_boardingCooldownMs = g_maxBoardingCooldownMs;

                  destroyMe->tangible = 0;
                  destroyMe->x = -1000;
                  destroyMe->y = -1000;
                  //break;
                }
              }
      
      
              a->cooldownA = 1000;
              a->flagA = 1;
              
              //interesting!
//              a->forwardsPushAngle = a->steeringAngle;
//              a->forwardsPushVelocity = 400;
    
              a->frameInAnimation = 8;
              a->scriptedAnimation = 1;
              a->xmaxspeed = 0;
              a->bonusSpeed = 0;

              a->spawnlist[0]->visible = 1;
              a->spawnlist[0]->frameInAnimation = 0;
              a->spawnlist[0]->scriptedAnimation = 1;
              a->spawnlist[0]->msPerFrame = 30;
              a->spawnlist[0]->loopAnimation = 1;

              { //create hitbox
                hitbox* h = new hitbox();
          
                h->bounds.x = 0;
                h->bounds.y = 0;
                h->bounds.z = 20;
                h->bounds.width = 192;
                h->bounds.height = 192;
                h->bounds.zeight = 128;
            
                h->activeMS = 2000;
                h->sleepingMS = 0;
                h->parent = a->spawnlist[0];
              }
              
      
              break;
            }
          case 1:
            {
              //update aggressiveness
              if(!a->flagA) {
                //a->bonusSpeed = a->aggressiveness/10;
              }
    
              break;
            }
          }
          
        }
      }
      break;


      break;
    }
    case 102:
    {
      //creep
      float dist = XYWorldDistanceSquared(a, protag);
      const float range = 262144; //8 blocks
      
      if(dist < range) {
        //protag->hisStatusComponent.slown.addStatus(1,0.4);
        protag->hisStatusComponent.disabled.addStatus(1,0.4);
      }


      //a->bonusSpeed = a->aggressiveness;

      a->cooldownA -= elapsed;

      for(int i = 0; i < a->myAbilities.size(); i++) {
        if(a->myAbilities[i].ready) {
          a->myAbilities[i].ready = 0;
          a->myAbilities[i].cooldownMS = rng(a->myAbilities[i].lowerCooldownBound, a->myAbilities[i].upperCooldownBound);
          switch (i) {
            case 0:
              {
                float offset = 150;
                float yoff = -offset * sin(a->steeringAngle);
                float xoff = offset * cos(a->steeringAngle);
                
                { //only attack if it would hit
                  M("A");
      
                  rect prediction(a->getOriginX() + xoff -96, a->getOriginY() + yoff - 96, a->z + 20, 192, 192, 128);
      
                  if(!CylinderOverlap(prediction, protag->getMovedBounds())) {
                    a->myAbilities[i].ready = 1;
                    break;
                  }
                  if(g_protagIsWithinBoardable) {
                    a->myAbilities[i].ready = 1;
                    break;
                  }
                }

//                { // only attack if she is active
//                  if(a->activeState != 1) {
//                    a->myAbilities[i].ready = 1;
//                    break;
//                  }
//                }

                a->specialState = 1;
                a->spawnlist[0]->frameInAnimation = 0;
                a->spawnlist[0]->scriptedAnimation = 1;
                a->spawnlist[0]->msPerFrame = 60;
                a->spawnlist[0]->loopAnimation = 0;
                M("Hitting");

                { //create hitbox
                  hitbox* h = new hitbox();
            
                  h->x = a->getOriginX() + xoff - 96;
                  h->y = a->getOriginY() + yoff - 96;
                  h->z = a->z;
              
                  h->bounds.x = 0;
                  h->bounds.y = 0;
                  h->bounds.z = 20;
                  h->bounds.width = 192;
                  h->bounds.height = 192;
                  h->bounds.zeight = 128;
              
                  h->activeMS = 10;
                  h->sleepingMS = 120;
                }

                break;
              }
          }
        }
      }


      switch(a->specialState) {
        case 0:
        {
          //walking
          a->cooldownA = 500;
          

          if(a->lastState == a->activeState) {break;}
          switch(a->activeState) {
            case 0:
            {
              M("switched to passive");
              //passive - roam
              a->readyForNextTravelInstruction = 1;
              a->agrod = 0;
              a->target = nullptr;
              a->traveling = 1;
              
              break;
            }
            case 1:
            {
              M("switched to active");
              a->agrod = 1;
              a->target = protag;
              a->traveling = 0;

            }

          }
          a->lastState = a->activeState;
          break;
        }
        case 1: 
        {
          //prepare for attack
          a->frameInAnimation = 7;
          a->scriptedAnimation = 1;
          a->specialAngleOverride = 1;
          a->spawnlist[0]->visible = 1;
          float angleToTarget = atan2(a->target->getOriginX() - a->getOriginX(), a->target->getOriginY() - a->getOriginY()) - M_PI/2;
          angleToTarget = wrapAngle(angleToTarget);
          //a->steeringAngle = angleToTarget;
          a->targetSteeringAngle = angleToTarget;
          a->hisStatusComponent.slown.addStatus(100,1);

          if(a->cooldownA < 0) {
            a->specialState = 2;
          }
          break;
        }
        case 2:
        {
          a->spawnlist[0]->visible = 0;
          a->scriptedAnimation = 0;
          a->specialState = 0;
          a->specialAngleOverride = 0;
        }
      }

    }
  }
}

void specialObjectsInteract(entity* a) {
  switch(a->identity) {

    case 9:
    {
      //inventory chest
      g_inventoryUiIsLevelSelect = 0;
      g_inventoryUiIsLoadout = 1;
      g_inventoryUiIsKeyboard = 0;
      inventorySelection = 0;
      inPauseMenu = 1;
      g_firstFrameOfPauseMenu = 1;
      old_z_value = 1;
      adventureUIManager->escText->updateText("", -1, 0.9);
      adventureUIManager->positionInventory();
      adventureUIManager->showInventoryUI();
      //adventureUIManager->hideHUD();
      break;
    }
    case 10:
    {
      //corkboard
      clear_map(g_camera);
      g_inventoryUiIsLevelSelect = 1;
      g_inventoryUiIsKeyboard = 0;
      g_inventoryUiIsLoadout = 0;
      inventorySelection = 0;
      inPauseMenu = 1;
      g_firstFrameOfPauseMenu = 1;
      old_z_value = 1;
      adventureUIManager->escText->updateText("", -1, 0.9);
      adventureUIManager->positionInventory();
      adventureUIManager->showInventoryUI();
      adventureUIManager->hideHUD();
      break;
    }
    case 11:
    {
      //lever
      if(a->flagA == 0) {
        //open door
        M("open");
        a->spawnlist[0]->banished = 1;
        a->spawnlist[0]->zaccel = 220;
        a->spawnlist[0]->shadow->enabled = 0;
        a->flagA = 1;
        a->frameInAnimation = 1;
        a->spawnlist[0]->navblock = 0;
      } else {
        //close door
        //need to add check for entities
        int good = 1;
        for(auto x : g_entities) {
          if(!x->dynamic || !x->tangible) {continue;}
          if(RectOverlap(a->spawnlist[0]->getMovedBounds(), x->getMovedBounds())) {
            good = 0;
          }
        }
        if(!good) {break;}
        
        M("close");
        a->spawnlist[0]->banished = 0;
        a->spawnlist[0]->dynamic = 1;
        a->spawnlist[0]->opacity = 255;
        a->spawnlist[0]->shadow->enabled = 1;
        a->spawnlist[0]->navblock = 1;
        for(auto x : a->spawnlist[0]->overlappedNodes) {
          x->enabled = 0;
        }

        a->flagA = 0;
        a->frameInAnimation = 0;
      }
      break;
    }
    case 20:
    {
      //collectible familiar
        if(!a->flagA) {
          a->dynamic = 0;
          a->flagA = 1;
          g_familiars.push_back(a);
          g_chain_time = 1000;
        }

        //check familiars to see if we should combine any
        entity* first = 0;
        entity* second = 0;
        entity* third = 0;
        for(auto x : g_familiars) {
          int count = 1;
          for(auto y : g_familiars) {
            if(y == x) {break;}
            if(y->name.substr(0, y->name.find('-')) == x->name.substr(0, x->name.find('-'))) {
              count ++;
              first = x;
              if(count == 2) { second = y; }
              if(count == 3) { third = y; break; }
            }


          }
          if(count == 3) {
            //combine these familiars
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), x), g_familiars.end());
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), second), g_familiars.end());
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), third), g_familiars.end());
            g_combineFamiliars.push_back(x);
            g_combineFamiliars.push_back(second);
            g_combineFamiliars.push_back(third);

            g_familiarCombineX = (x->getOriginX() + second->getOriginX() + third->getOriginX()) /3;
            g_familiarCombineY = (x->getOriginY() + second->getOriginY() + third->getOriginY()) / 3;

            g_combinedFamiliar = new entity(renderer, x->name.substr(0, x->name.find('-')) + "-full");
            g_combinedFamiliar->x = 0;
            g_chain_time = 0;
            x->darkenMs = 300;
            x->darkenValue = 255;
            second->darkenMs = 300;
            x->darkenValue = 255;
            third->darkenMs = 300;
            x->darkenValue = 255;
          }


        }
        break;
    }
    case 22:
    {
      //dungeon door
      if(g_dungeonSystemOn) {
        M("Dungeon system is ON");
        g_dungeonDoorActivated = 1;
      } else {
        M("Dungeon system is OFF");
        //clear_map(g_camera);
        g_dungeonDarkness = 0;
        g_inventoryUiIsLevelSelect = 1;
        g_inventoryUiIsKeyboard = 0;
        g_inventoryUiIsLoadout = 0;
        inventorySelection = 0;
        inPauseMenu = 1;
        g_firstFrameOfPauseMenu = 1;
        old_z_value = 1;
        adventureUIManager->escText->updateText("", -1, 0.9);
        adventureUIManager->positionInventory();
        adventureUIManager->showInventoryUI();
        adventureUIManager->hideHUD();
      }
      break;
      
    }
    case 25:
    {
      //dungeon lock

      {
        entity* key = 0;

        for(auto &x : g_familiars)
        {
          if(x->name == "common/key") {
            key = x;
            break;
          }
        }

        if(key != 0) {
          { //suck
            g_ex_familiars.push_back(key);
            g_familiars.erase(remove(g_familiars.begin(), g_familiars.end(), key), g_familiars.end());
            g_exFamiliarParent = a;
            g_exFamiliarTimer = 10000;
            key->opacity_delta = -7;
          }

          {
            //banish
            a->banished = 1;
            a->zaccel = 220;
            a->shadow->alphamod = -1;
            a->navblock = 0;


          }
        } else {
          //play a noise to confirm input

        }
      }
      
      break;
    }
    case 33: 
    {
      //shine, the dungeon door for natural
      if(g_dungeonSystemOn) {
        g_dungeonDoorActivated = 1;
      } else {
        clear_map(g_camera);
        g_inventoryUiIsLevelSelect = 1;
        g_inventoryUiIsKeyboard = 0;
        g_inventoryUiIsLoadout = 0;
        inventorySelection = 0;
        inPauseMenu = 1;
        g_firstFrameOfPauseMenu = 1;
        old_z_value = 1;
        adventureUIManager->escText->updateText("", -1, 0.9);
        adventureUIManager->positionInventory();
        adventureUIManager->showInventoryUI();
        adventureUIManager->hideHUD();
      }
      break;

    }
  }
}

void specialObjectsOncePerFrame(float elapsed) 
{
  cannonCooldown -= elapsed;
  cannonEvent = 0;
  if(cannonCooldown < 0) {
    cannonEvent = 1;
    cannonToggleEvent++;
    if(cannonToggleEvent < 4){
      if(cannonToggleTwo) {
        cannonCooldownM = 500;
        cannonToggleTwo = !cannonToggleTwo;
      } else {
        cannonCooldownM = 166;

      }
    } else {
      cannonToggleTwo = !cannonToggleTwo;
      cannonCooldownM = 1000;
      cannonToggleEvent = 0;
    }
    cannonCooldown += cannonCooldownM;
  }

  shortSpikesCooldown -= elapsed;
  if(shortSpikesCooldown < 0) {
    shortSpikesCooldown += 2600;
  }
  
  shortSpikesState = 4;

  if(shortSpikesCooldown < 2450) {
    shortSpikesState = 3;
  }

  if(shortSpikesCooldown < 1300) {
    shortSpikesState = 2;
  }

  if(shortSpikesCooldown < 1050) {
    shortSpikesState = 1;
  }

  if(shortSpikesState == lastShortSpikesState) {
    shortSpikesState = -1;
  } else {
    lastShortSpikesState = shortSpikesState;
  }

}

void usableItemCode(usable* a) {
  switch(a->specialAction) {
    case 3:
    {
      //mechanism
      protag->forwardsPushVelocity = 3900;
      protag->forwardsPushAngle = protag->steeringAngle;
      littleSmokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);
      g_lastParticleCreated->accelerationz = 0;
      g_lastParticleCreated->velocityz = 0;

      break;
    }
    case 4:
    {
      //rewinder
      if(g_dungeonSystemOn) {
        g_dungeonDoorActivated = 1;
        g_dungeonIndex--;
        g_dungeonRedo = 1;
      }
    }
    case 5:
    {
      //mechanism 2
      //give protag invincible status
      protag->invincibleMS = 3000;
    }
    case 6:
    {
      //boots
      //give protag bonus speed
      protag->bonusSpeed = 50;
      g_protagBonusSpeedMS = 3000;
    }
    case 7:
    {
      //mechanism 3
      //give protag enraged status, than slowed status
      //reset if they leave the room

    }
  }
}


float exponentialCurve(int max, int exponent) {
  float x = rng(0, 100);
  float b = pow(x, exponent);
  return (b * max)/(pow(100,exponent));
}
