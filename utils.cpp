#include "utils.h"
#include <string>
#include <sstream>

SDL_Texture* loadTexture(SDL_Renderer* renderer, string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    SDL_Texture* texture = IMG_LoadTextureTyped_RW(renderer, myWop, 1, "QOI");
    PHYSFS_close(myfile);
    return texture;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    if(!devMode) {
      abort();
    }
    return nullptr;
  }
}

SDL_Surface* loadSurface(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    SDL_Surface* surface = IMG_LoadTyped_RW(myWop, 1, "QOI");
    PHYSFS_close(myfile);
    return surface;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    abort();
    return nullptr;
  }
}

Mix_Chunk* loadWav(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    Mix_Chunk* myChunk = Mix_LoadWAV_RW(myWop, 1);
    PHYSFS_close(myfile);
    return myChunk;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    abort();
    return nullptr;
  }
}

vector<string> loadText(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }

    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);

    PHYSFS_close(myfile);
    string myString;
    for(int i = 0; i < filesize; i++) {
      myString.push_back(buf[i]);
    }

    vector<string> x = splitString(myString, '\n');

    //this is needed for windows text files
    {
      if(x.back()[0] == '\r') { 
        x.pop_back();
      }
      for(int i = 0; i < x.size(); i++) {
        if(x[i].back() == '\r') {
          x[i].pop_back();
        }
      }
    }

    delete[] buf;
    return x;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    if(!devMode) {
      abort();
    }
    return {};
  }

}

string loadTextAsString(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }

    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize+1];
    buf[filesize] = '\0';
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);

    PHYSFS_close(myfile);
    string myString(buf);
    delete[] buf;
    return myString;

  } else {
    E("FNF: " + fileaddress);
    int i = 0;
    breakpoint();
//    abort();
    return {};
  }
}


TTF_Font* loadFont(string fileaddress, float fontsize)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);

    PHYSFS_close(myfile);
    TTF_Font* ret;
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);
    ret = TTF_OpenFontRW(myWop, 1, fontsize);

    //delete buf; //leak?
    return ret;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    abort();
    return {};
  }
}

Mix_Music* loadMusic(string fileaddress)
{
  if(PHYSFS_exists(fileaddress.c_str())) 
  {
    PHYSFS_getLastErrorCode();
    PHYSFS_file* myfile = PHYSFS_openRead(fileaddress.c_str());
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    
    if(error != 0) {
      D(fileaddress);
      D(error);
      const char* errorStr = PHYSFS_getErrorByCode(error);
      D(errorStr);
      breakpoint();
      abort();
    }
    PHYSFS_sint64 filesize = PHYSFS_fileLength(myfile);
    char* buf;
    buf = new char[filesize];
    int length_read = PHYSFS_readBytes(myfile, buf, filesize);
    SDL_RWops* myWop = SDL_RWFromMem(buf, filesize);

    Mix_Music* ret = Mix_LoadMUS_RW(myWop, 1);

    PHYSFS_close(myfile);
    return ret;

  } else {
    E("FNF: " + fileaddress);
    breakpoint();
    abort();
    return nullptr;
  }
}
