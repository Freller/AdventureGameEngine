#ifndef loss_h
#define loss_h
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include "globals.h"

void getLossInput();

void LossLoop();

class lossUI {
public:
  SDL_Texture* floor;
  SDL_Texture* protag;
  SDL_Texture* shadow;
  ui* splat;
  textbox* yes;
  textbox* no;
  ui* handMarker;

  int protagX = 0;
  int protagY = 0;
  int protagW = 0;
  int protagH = 0;

  int shadowX = 0;
  int shadowY = 0;
  int shadowW = 0;
  int shadowH = 0;

  int timer = 0;
//  int shake1Ms = 1000;
//  int pause1Ms = 2000;
//  int shake2Ms = 3000;
//  int pause2Ms = 4000;
//  int shake3Ms = 5500;
  int shake1Ms = 000;
  int pause1Ms = 000;
  int shake2Ms = 000;
  int pause2Ms = 200;
  int shake3Ms = 1200;

  int pMinX =0;
  int pMaxX =0;
  int offset = 0;
  int delta = 3;
  int magnitude = 3;

  bool option = 0;

  float handOffset = 0.02;

  Uint8 redness = 255;

  void hideAll();

  lossUI();
  ~lossUI();
};


#endif
