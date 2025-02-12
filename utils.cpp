#include "utils.h"
#include "globals.h"
#include <string>
#include <sstream>
#include <vector>

map<string, string> languagePack;

map<string, pair<int, int>> languagePackIndices;

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
    SDL_Texture* texture = IMG_LoadTextureTyped_RW(renderer, myWop, 1, ".qoi");
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
    SDL_Surface* surface = IMG_LoadTyped_RW(myWop, 1, ".qoi");
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

void generateIndicesFile() {
  if(!devMode || g_ship) {
    E("Tried to generate languagepack indices file for release!");
  }
  string input_file = "resources/languagepack/" + g_language + "/major.txt";
  string output_file = "resources/languagepack/" + g_language + "/indices.dat";
  std::istringstream infile(loadTextAsString(input_file));
  std::ofstream outfile(output_file);
  std::string line;

  while (std::getline(infile, line)) {
    if (line.find(':') != std::string::npos) {
      std::istringstream iss(line);
      std::string handle;
      if (std::getline(iss, handle, ':')) {
        handle = handle.substr(1, handle.size() - 2);  // Remove quotes
        int pos = (unsigned int)infile.tellg() - (unsigned int)line.length() - 1;
        int length = line.length();
        outfile << handle << ":" << pos << "," << length << std::endl;
      }
    }
  }
}

void initLanguageIndices() {
  string input_file = "resources/languagepack/" + g_language + "/indices.dat";

  istringstream infile(loadTextAsString(input_file));
  string line = "";
  while (std::getline(infile, line)) {
    auto x = splitString(line, ':');
    auto y = splitString(x[1], ',');
    if(y[1].back() == '\r') {
      y[1].pop_back();
    }
  
    string key = x[0];
    int pos = stoi(y[0]);
    int length = stoi(y[1]);
    languagePackIndices[key] = std::make_pair(pos, length);
  }
}

std::wstring bytes_to_wstring(const std::vector<unsigned char>& bytes) {
    std::string str(bytes.begin(), bytes.end());
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

//first stage: fetch dialog from the major.json file using it's handle
//this will get to be more wonderful later
wstring getLanguageDataSpecial(string handle) {
  string fileaddress = "resources/languagepack/" + g_language + "/major.txt";

  int position = 0;
  int length = 0;
  try {
    position = languagePackIndices[handle].first;
    length = languagePackIndices[handle].second;
  } catch (...) {
    E("Languge-pack key error for " + handle + ".");
    abort();
  }

  if(PHYSFS_exists(fileaddress.c_str())) {
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

    PHYSFS_seek(myfile, position);
    vector<unsigned char> buf(length);
    PHYSFS_readBytes(myfile, buf.data(), length);
    PHYSFS_close(myfile);

    wstring myWStr = bytes_to_wstring(buf);
    if(myWStr.back() == '\r') {
      myWStr.pop_back();
    }
    myWStr.pop_back();

    int pos = myWStr.find(':');

    if(pos == string::npos) {
      E("Missing ':' in language-pack-file, check near " + handle + ".");
    }

    //string ret = myString.substr(pos+2, myString.size()-(pos+2));
    wstring ret = myWStr.substr(pos+2, myWStr.size()-(pos+2));
    D(ret.size());
    return ret;


  } else {
    E("No language-pack-file found for " + g_language + ".");
    abort();
  }

  return L"";
}


//first stage: fetch dialog from the major.json file using it's handle
//this will get to be more wonderful later
string getLanguageData(string handle) {
  string fileaddress = "resources/languagepack/" + g_language + "/major.txt";

  int position = 0;
  int length = 0;
  try {
    position = languagePackIndices[handle].first;
    length = languagePackIndices[handle].second;
  } catch (...) {
    E("Languge-pack key error for " + handle + ".");
    abort();
  }

  if(PHYSFS_exists(fileaddress.c_str())) {
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

    PHYSFS_seek(myfile, position);
    char* buf;
    buf = new char[length];
    PHYSFS_readBytes(myfile, buf, length);
    PHYSFS_close(myfile);
    string myString = "";
    for(int i = 0; i < length; i++) {
      myString.push_back(buf[i]);
    }
    if(myString.back() == '\r') {
      myString.pop_back();
    }

    myString.pop_back();
    int pos = myString.find(':');

    if(pos == string::npos) {
      E("Missing ':' in language-pack-file, check near " + handle + ".");
    }

    string ret = myString.substr(pos+2, myString.size()-(pos+2));
    return ret;


  } else {
    E("No language-pack-file found for " + g_language + ".");
    abort();
  }

  return "";
}

std::string stringMultiInject(const std::string &templateStr, const std::vector<std::string> &values) {
  std::string result = templateStr;

  size_t pos = 0;
  int valueIndex = 0;
  while ((pos = result.find('<', pos)) != std::string::npos) {
    size_t endPos = result.find('>', pos);
    if (endPos != std::string::npos) {
      string valstr = result.substr(pos+1, endPos-pos-1);
      valueIndex = stoi(valstr);
      if (valueIndex >= values.size()) {
        break;
      }
      result.replace(pos, endPos - pos + 1, values[valueIndex]);
      pos = endPos + values[valueIndex].length();
    } else {
      break;
    }
  }

  return result;
}
