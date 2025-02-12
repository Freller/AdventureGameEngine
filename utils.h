#ifndef utils_h
#define utils_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include "physfs.h"
#include "globals.h"
#include <locale>
#include <codecvt>

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

wstring bytes_to_wstring(const std::vector<unsigned char>& bytes);

wstring getLanguageDataSpecial(string handle);

string getLanguageData(string handle);

string stringMultiInject(const string &templateStr, const vector<string> &values);

#endif
