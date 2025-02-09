#ifndef utils_h
#define utils_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include "physfs.h"
#include "globals.h"

using namespace std;

extern map<string, string> languagePack;

extern map<string, pair<int, int>> languagePackIndices;

SDL_Texture* loadTexture(SDL_Renderer* renderer, string fileaddress);

SDL_Surface* loadSurface(string fileaddress);

Mix_Chunk* loadWav(string fileaddress);

vector<string> loadText(string fileaddress);

string loadTextAsString(string fileaddress);

TTF_Font* loadFont(string fileaddress, float fontsize);

Mix_Music* loadMusic(string fileaddress);

void initLanguageIndices();

void generateIndicesFile();

string getLanguageData(string handle);

#endif
