#include "combat.h"
#include "objects.h"
#include "main.h"
#include "utils.h"
#include <unordered_map>
#include <vector>

void loadPalette(SDL_Renderer* renderer, const char* filePath, std::vector<Uint32>& palette) {
  // Load the image into a surface
  SDL_Surface* surface = IMG_Load(filePath);
  if (!surface) {
    std::cout << "Unable to load image! SDL_image Error: " << IMG_GetError() << std::endl;
  }

  SDL_PixelFormat* format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888); 
  for (int x = 0; x < 16; ++x) { 
    Uint32 pixel = ((Uint32*)surface->pixels)[x]; 
    Uint8 r, g, b, a; 
    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a); 
    Uint32 mappedColor = SDL_MapRGBA(format, a, r, g, b); 
    palette.push_back(mappedColor); 
  }

  SDL_FreeSurface(surface);
}

string getPossessivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[0];
      break;
    case 1:
      return pronounTable[1];
      break;
    case 2:
      return pronounTable[2];
      break;
    case 3:
      return pronounTable[3];
      break;
  }
  return "";
}

string getReflexivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[4];
      break;
    case 1:
      return pronounTable[5];
      break;
    case 2:
      return pronounTable[6];
      break;
    case 3:
      return pronounTable[7];
      break;
  }
  return pronoun;
}

string getSubjectivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      return pronounTable[8];
      break;
    case 1:
      return pronounTable[9];
      break;
    case 2:
      return pronounTable[10];
      break;
    case 3:
      return pronounTable[11];
      break;
  }
  return pronoun;
}

string getObjectivePronoun(combatant* c) {
  string pronoun = "";
  switch(c->gender) {
    case 0:
      pronoun = "him";
      break;
    case 1:
      pronoun = "her";
      break;
    case 2:
      pronoun = "it";
      break;
    case 3:
      pronoun = "them";
      break;
  }
  return pronoun;
}

bground::bground() {};

bground::bground(SDL_Renderer* renderer, const char* configFilePath) {
  std::ifstream configFile(configFilePath);
  if (!configFile) {
    std::cerr << "Unable to open config file!" << std::endl;
    return;
  }

  std::string line;
  while (std::getline(configFile, line)) {
    std::istringstream iss(line);
    std::string key;
    if (std::getline(iss, key, ':')) {
      std::string value;
      if (std::getline(iss, value)) {
        if(key == "scene") scene = value;
        else if (key == "texture") texture = std::stoi(value);
        else if (key == "interleaved") interleaved = std::stoi(value);
        else if (key == "horizontalIntensity") horizontalIntensity = std::stof(value);
        else if (key == "horizontalPeriod") horizontalPeriod = std::stof(value);
        else if (key == "verticalIntensity") verticalIntensity = std::stof(value);
        else if (key == "verticalPeriod") verticalPeriod = std::stof(value);
        else if (key == "scrollXMagnitude") scrollXMagnitude = std::stof(value);
        else if (key == "scrollYMagnitude") scrollYMagnitude = std::stof(value);
        else if (key == "paletteFile") {
          std::string paletteFilePath = "resources/static/backgrounds/pallets/" + value + ".qoi";
          loadPalette(renderer, paletteFilePath.c_str(), palette);
        }
        else if (key == "texture2") texture2 = std::stoi(value);
        else if (key == "interleaved2") interleaved2 = std::stoi(value);
        else if (key == "horizontalIntensity2") horizontalIntensity2 = std::stof(value);
        else if (key == "horizontalPeriod2") horizontalPeriod2 = std::stof(value);
        else if (key == "verticalIntensity2") verticalIntensity2 = std::stof(value);
        else if (key == "vertialPeriod2") vertialPeriod2 = std::stof(value);
        else if (key == "scrollXMagnitude2") scrollXMagnitude2 = std::stof(value);
        else if (key == "scrollYMagnitude2") scrollYMagnitude2 = std::stof(value);
        else if (key == "paletteFile2") {
          std::string paletteFilePath2 = "resources/static/backgrounds/pallets/" + value + ".qoi";
          loadPalette(renderer, paletteFilePath2.c_str(), palette2);
        }
      }
    }
  }
}


// Warp effect function implementation
void applyWarpEffect(SDL_Texture* texture, SDL_Renderer* renderer, float time, bool interleaved, float horizontalWaveIntensity, float horizontalWavePeriod, float verticalWaveIntensity, float verticalWavePeriod, float scrollXMagnitude, float scrollYMagnitude) {
  int width, height;
  SDL_QueryTexture(texture, NULL, NULL, &width, &height);

  SDL_Texture* warpedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
  SDL_SetRenderTarget(renderer, warpedTexture);
  SDL_SetTextureBlendMode(warpedTexture, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  int scrollX = static_cast<int>((time * scrollXMagnitude)) % width;
  int scrollY = static_cast<int>((time * scrollYMagnitude)) % height;

  for (int y = 0; y < height; ++y) {
    float scaleY = verticalWaveIntensity * sinf(((y) * 0.01f) + time * verticalWavePeriod);
    int srcY = (static_cast<int>((y + scrollY) - scaleY)) % height;
    if (srcY < 0) srcY += height;

    float offsetX = horizontalWaveIntensity * sinf((srcY + time * horizontalWavePeriod) * (2 * M_PI / width));
    if (interleaved && y % 2 == 0) {
      offsetX = -offsetX;
    }

    int srcX = (static_cast<int>(offsetX + scrollX)) % width;
    if (srcX < 0) srcX += width;

    SDL_Rect srcRect1 = {srcX, srcY, width - srcX, 1};
    SDL_Rect destRect1 = {0, y, width - srcX, 1};
    SDL_RenderCopy(renderer, texture, &srcRect1, &destRect1);

    if (srcX > 0) {
      SDL_Rect srcRect2 = {0, srcY, srcX, 1};
      SDL_Rect destRect2 = {width - srcX, y, srcX, 1};
      SDL_RenderCopy(renderer, texture, &srcRect2, &destRect2);
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderCopy(renderer, warpedTexture, NULL, NULL);
  SDL_DestroyTexture(warpedTexture);
}

// Palette-cycling function
void cyclePalette(SDL_Surface* source, SDL_Surface* destination, std::vector<Uint32>& palette) {
  // Rotate the palette by one color
  Uint32 firstColor = palette[0];
  for (size_t i = 0; i < palette.size() - 1; ++i) {
    palette[i] = palette[i + 1];
  }
  palette.back() = firstColor;

  SDL_LockSurface(source);
  SDL_LockSurface(destination);
  Uint32* srcPixels = (Uint32*)source->pixels;
  Uint32* dstPixels = (Uint32*)destination->pixels;
  int width = source->w;
  int height = source->h;
  int paletteSize = palette.size();

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      uint8_t red = (srcPixels[y * width + x] >> 16) & 0xFF;
      int index = (int)red/25;
      //std::cout << index << std::endl;
      int currentColorIndex = index % paletteSize;
      dstPixels[y * width + x] = palette[currentColorIndex];
    }
  }

  // Blur dstPixels
  std::vector<Uint32> tempPixels(width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int r = 0, g = 0, b = 0, a = 0, count = 0;
      for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
          int ix = x + dx;
          int iy = y + dy;
          if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
            Uint32 pixel = dstPixels[iy * width + ix];
            r += (pixel & 0x00FF0000) >> 16;
            g += (pixel & 0x0000FF00) >> 8;
            b += (pixel & 0x000000FF);
            a += (pixel & 0xFF000000) >> 24;
            count++;
          }
        }
      }
      Uint32 avgR = r / count;
      Uint32 avgG = g / count;
      Uint32 avgB = b / count;
      Uint32 avgA = a / count;
      tempPixels[y * width + x] = (avgA << 24) | (avgR << 16) | (avgG << 8) | avgB;
    }
  }

  // Copy the blurred pixels back to dstPixels
  std::copy(tempPixels.begin(), tempPixels.end(), dstPixels);


  SDL_UnlockSurface(source);
  SDL_UnlockSurface(destination);
}

void drawBackground() {
  combatUIManager->time += elapsed;

  // Cycle the palette every 0.5 seconds
  if (combatUIManager->time - combatUIManager->cycleTime >= 500) {
    cyclePalette(combatUIManager->sb1, combatUIManager->db1, combatUIManager->loadedBackground.palette);
    combatUIManager->cycleTime = combatUIManager->time;
  }

  if(combatUIManager->tb1 != 0) {
    SDL_DestroyTexture(combatUIManager->tb1);
  }
  combatUIManager->tb1 = SDL_CreateTextureFromSurface(renderer, combatUIManager->db1);



  applyWarpEffect(combatUIManager->tb1, renderer, combatUIManager->time/10000.0f, combatUIManager->loadedBackground.interleaved, combatUIManager->loadedBackground.horizontalIntensity, combatUIManager->loadedBackground.horizontalPeriod, combatUIManager->loadedBackground.verticalIntensity, combatUIManager->loadedBackground.verticalPeriod, combatUIManager->loadedBackground.scrollXMagnitude, combatUIManager->loadedBackground.scrollYMagnitude);
}

void drawSimpleBackground() {
  SDL_RenderCopy(renderer, combatUIManager->scene, NULL, NULL);
}

combatant::combatant(string ffilename, int fxp) {
  string loadstr;
  loadstr = "resources/static/combatfiles/" + ffilename + ".cmb";
  istringstream file(loadTextAsString(loadstr));

  string temp;
  file >> temp;
  file >> temp;


  name = temp;
  if(name.back() == '\r') {
    name.pop_back();
  }
  
  name = getLanguageData(name.substr(1, name.size()-2));


  filename = ffilename;

  std::transform(name.begin(), name.end(), name.begin(), 
      [](unsigned char c) { if(c == '_') {int e = ' '; return e;} else {return int(c);}  } );

  file >> temp;
  file >> temp;
  gender = stoi(temp);

  file >> temp;
  file >> temp;


  string spritefilevar;
  spritefilevar = "resources/static/combatsprites/" + temp + ".qoi";
  const char* spritefile = spritefilevar.c_str();
  texture = loadTexture(renderer, spritefile);

  file >> temp;
  file >> temp;
  offset = stof(temp);

  file >> temp;
  file >> temp;
  myType = (type)stoi(temp);

  file >> temp;
  file >> l0Attack;

  file >> temp;
  file >> attackGain;
  attackGain -= l0Attack;
  attackGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Defense;

  file >> temp;
  file >> defenseGain;
  defenseGain -= l0Defense;
  defenseGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Strength;

  file >> temp;
  file >> strengthGain;
  strengthGain -= l0Strength;
  strengthGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Critical;

  file >> temp;
  file >> criticalGain;
  criticalGain -= l0Critical;
  criticalGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Skill;

  file >> temp;
  file >> skillGain;
  skillGain -= l0Skill;
  skillGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Soul;

  file >> temp;
  file >> soulGain;
  soulGain -= l0Soul;
  soulGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Mind;

  file >> temp;
  file >> mindGain;
  mindGain -= l0Mind;
  mindGain /= 100; //attack at level 100

  file >> temp;
  file >> l0Recovery;

  file >> temp;
  file >> recoveryGain;
  recoveryGain -= l0Recovery;
  recoveryGain /= 100; //attack at level 100

  file >> temp;
  file >> deathText;

  if(deathText.back() == '\r') {
    deathText.pop_back();
  }

  deathText = getLanguageData(deathText.substr(1, deathText.size()-2));

//  for (char &ch : deathText) {
//    if(ch == '_') {
//      ch = ' ';
//    }
//  }

  file >> temp;
  file >> temp; // Read the '{'
  while (true) {
    std::getline(file, temp);
    temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end()); // Remove carriage return if present
    if(temp.empty()) {continue;}
    if (temp == "}") break;
    vector<string> x = splitString(temp, ' ');
    attackPatterns.push_back({});
    for(auto y : x) {
      attackPatterns[attackPatterns.size()-1].push_back(stoi(y));
    }
  }

  file >> temp;
  file >> temp; //read the '{'
  while (true) {
    std::getline(file, temp);
    temp.erase(std::remove(temp.begin(), temp.end(), '\r'), temp.end()); // Remove carriage return if present
    if(temp.empty()) {continue;}
    if (temp == "}") break;
    vector<string> x = splitString(temp, ':');
    pair<int, int> a; a.first = stoi(x[0]);
    a.second = stoi(x[1]);
    spiritTree.push_back(a);
  }


  xp = fxp;
  level = xpToLevel(xp);

  int fw, fh;
  SDL_QueryTexture(texture, NULL, NULL, &fw, &fh);

  width = fw;
  height = fh; 

  width /= 1920.0;
  height /= 1920.0; 
  serial.target = -1;
  serial.action = turnAction::ATTACK;
  serial.actionIndex = -1;
}

combatant::~combatant() {
  SDL_DestroyTexture(texture);
}


itemInfo::itemInfo(string a, int b) {
  name = a;
  targeting = b;
}

itemInfo::itemInfo() {
  name = "";
  targeting = 0;
}

spiritInfo::spiritInfo(string a, int b, int c) {
  name = a;
  targeting = b;
  cost = c;
}

spiritInfo::spiritInfo() {
  name = "";
  targeting = 0;
  cost = 0;
}

std::unordered_map<int, itemInfo> itemsTable;

std::unordered_map<int, spiritInfo> spiritTable;

vector<string> pronounTable;

void spawnBullets(int pattern, int& accumulator) {
  switch(pattern) {
    case 0:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->red = 0;
          }
        }
        break;
      }
    case 1:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->angle = atan2(combatUIManager->dodgerY - a->y, combatUIManager->dodgerX - a->x);
            a->texture = combatUIManager->bulletTexture;
            a->homing = 1;
            a->blue = 0;
          }
        }
      }
    case 2:
      {
        int cooldown = 2000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->x = SCREEN_WIDTH + SPAWN_MARGIN;
            a->y = rng(0, SCREEN_HEIGHT);
            a->angle = M_PI;
            a->texture = combatUIManager->bulletTexture;
            a->green = 0;
          }
        }
        break;
      }
    case 3:
      {
        int cooldown = 2000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 6; // Number of bullets in the sine wave
          float amplitude = 0.5f; // Amplitude of the sine wave
          float frequency = 0.1f; // Frequency of the sine wave

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = -SPAWN_MARGIN; // Start from the left side, slightly off-screen
            a->y = -SPAWN_MARGIN; // Start from the bottom left corner

            // Calculate the angle using the sine function
            float offsetAngle = amplitude * sin(frequency * accumulator + i);
            a->angle = M_PI/4 + offsetAngle; // Sweep up and down

            a->texture = combatUIManager->bulletTexture;
            a->green = 128; // Set a different color for sine-pattern bullets
          }
        }
        break;
      }
    case 4:
      {
        // Stream bullets with sweeping angle pattern
        int cooldown = 600; // Short cooldown for continuous stream
        static float sweepTime = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float amplitude = 0.7f; // Amplitude of the sweep (radians)
          float frequency = 0.6f; // Frequency of the sweep (adjust as needed)
          float baseAngle = -M_PI / 2 + M_PI/4; // Base angle (straight up)

          miniBullet* a = new miniBullet();
          a->x = -SPAWN_MARGIN; // Start from the left side, slightly off-screen
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom left corner

          // Calculate the sweeping angle using the sine function
          float sweepAngle = baseAngle + amplitude * sin(frequency * sweepTime);
          a->angle = sweepAngle;

          a->texture = combatUIManager->bulletTexture;
          a->red = 128; // Set a different color for sine-pattern bullets
          a->blue = 128;
          sweepTime += 1; // Increment sweep time
        }
        break;
      }
    case 5:
      {
        // Shotgun blast pattern
        int cooldown = 1500; // Cooldown between each blast
        int numBullets = 3; // Number of bullets in the shotgun blast
        float spreadAngle = M_PI / 4; // Total spread angle (in radians)

        if (accumulator >= cooldown) {
          accumulator = 0;

          // Center point of the blast (e.g., from the bottom left corner)
          float startX = SCREEN_WIDTH + SPAWN_MARGIN;
          float startY = -SPAWN_MARGIN;

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = startX;
            a->y = startY;
            a->velocity = frng(0.2, 0.7);
            a->w = frng(80, 120);
            a->h = a->w;

            // Calculate the angle for each bullet
            float angle = M_PI* (3.0/4.0) - spreadAngle / 2 + (spreadAngle / (numBullets - 1)) * i;
            angle += frng(-M_PI/6.0, M_PI/6.0);
            a->angle = angle;

            // Set the texture and color for the bullets
            a->texture = combatUIManager->bulletTexture;

            a->red = 255;
            a->blue = 128;
            a->green = 128;
          }
        }
        break;
      }
    case 6:
      {
        // Pattern using exploding bullets
        int cooldown = 2000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->y = rng(0, SCREEN_HEIGHT); // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.3; // Set bullet velocity
          a->exploding = true; // Enable explosion feature
          a->numFragments = 6;
          a->explosionTimer = rng(1000, 3000); // Set explosion timer
          a->texture = combatUIManager->bulletTexture;
          a->red = 255;   // Set color for initial bullet
          a->green = 0;
          a->blue = 0;
        }
        break;
      }
    case 7:
      {
        // Pattern using exploding bullets
        int cooldown = 3000; // Cooldown between each shot

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH + SPAWN_MARGIN; // Spawn off-screen
          a->y = rng(0, SCREEN_HEIGHT); // Random y position
          a->angle = -M_PI; // Shoot left
          a->velocity = 0.3; // Set bullet velocity
          a->exploding = 2; // Enable explosion feature
          a->numFragments = 3;
          a->fragSize = 0.75;
          a->explosionTimer = rng(1000, 3000); // Set explosion timer
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;   // Set color for initial bullet
          a->green = 128;
          a->blue = 128;
          a->randomExplodeAngle = 1;
        }
        break;
      }
    case 8:
      {
        int cooldown = 800;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI/2;
          a->velocity = 0;
          a->acceleration = 0.003;
          a->texture = combatUIManager->bulletTexture;
          a->red = 0;
          a->green = 128;
          a->blue = 128;
        }
        break;
      }
    case 9:
      {
        int cooldown = 1200;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI/2;
          a->velocity = 2;
          a->acceleration = -0.002;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 128;
          a->blue = 0;
        }
        break;
      }
    case 10:
      {
        int cooldown = 1200;

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->velocity = 1.6;
          a->acceleration = -0.0015;
          a->texture = combatUIManager->bulletTexture;
          a->red = 0;
          a->green = 128;
          a->blue = 128;
        }
        break;
      }
    case 11:
      {
        // Offscreen radial burst pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 18; // Number of bullets in the radial burst
          float spawnX, spawnY;
          // Ensure bullets spawn completely offscreen
          if (rng(0, 1)) {
            spawnX = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
            spawnY = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          } else {
            spawnX = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
            spawnY = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
          }

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
        }
        break;
      }

    case 12:
      {
        // Offscreen radial burst pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 12; // Number of bullets in the radial burst
          float spawnX, spawnY;
          // Ensure bullets spawn completely offscreen
          if (rng(0, 1)) {
            spawnX = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
            spawnY = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          } else {
            spawnX = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
            spawnY = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
          }

          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            a->x = spawnX;
            a->y = spawnY;
            a->velocity = 0.2;
            a->angle = i * (2 * M_PI / numBullets); // Spread bullets evenly in a circle
            a->angle += M_PI/numBullets;
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 255;
            a->blue = 0;
          }
        }
        break;
      }

    case 13:
      {
        // Wave pattern
        int cooldown = 1800;
        static float waveTime = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 5; // Number of bullets in the wave
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            a->x = SCREEN_WIDTH + SPAWN_MARGIN;
            a->y = (SCREEN_HEIGHT / numBullets) * i;
            a->angle = -M_PI; // Move left
                              // Calculate wave offset
            a->velocity = 0.3 + 0.4 * sin(waveTime + i * M_PI / numBullets);
            a->texture = combatUIManager->bulletTexture;
            a->red = 128;
            a->green = 0;
            a->blue = 255;
          }
          waveTime += 0.5;
        }
        break;
      }

    case 14:
      {
        // Zigzag pattern
        int cooldown = 1500;
        static bool direction = true; // Direction of the zigzag
        if (accumulator >= cooldown) {
          accumulator = 0;
          miniBullet* a = new miniBullet();
          // Ensure bullets spawn completely offscreen
          a->x = direction ? SCREEN_WIDTH + SPAWN_MARGIN : -SPAWN_MARGIN;
          a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
          a->angle = direction ? -M_PI : 0; // Move left or right
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 255;
          a->green = 255;
          a->blue = 128;
          direction = !direction; // Toggle direction
        }
        break;
      }


    case 15:
      {
        // Random scatter pattern
        int cooldown = 2500;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 20; // Number of bullets to scatter
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            if (rng(0, 1)) {
              a->x = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
              a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
            } else {
              a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
              a->y = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
            }
            a->angle = rng(0, 2 * M_PI); // Random direction
            a->velocity = frng(0.1, 0.5);
            a->texture = combatUIManager->bulletTexture;
            a->red = rng(0, 255);
            a->green = rng(0, 255);
            a->blue = rng(0, 255);
          }
        }
        break;
      }

    case 16:
      {
        // Converging pattern
        int cooldown = 3000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 5; // Number of bullets converging to a point
          float targetX = SCREEN_WIDTH / 2;
          float targetY = SCREEN_HEIGHT / 2;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            if (rng(0, 1)) {
              a->x = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_WIDTH + SPAWN_MARGIN;
              a->y = rng(-SPAWN_MARGIN, SCREEN_HEIGHT + SPAWN_MARGIN);
            } else {
              a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
              a->y = (rng(0, 1)) ? -SPAWN_MARGIN : SCREEN_HEIGHT + SPAWN_MARGIN;
            }
            a->angle = atan2(targetY - a->y, targetX - a->x); // Aim towards the center
            a->velocity = frng(0.2, 0.7);
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 0;
            a->blue = 255;
          }
        }
        break;
      }

    case 17:
      {
        // L-shaped pattern
        int cooldown = 1800;
        if (accumulator >= cooldown) {
          accumulator = 0;
          miniBullet* a = new miniBullet();
          // Ensure bullets spawn completely offscreen
          a->x = rng(-SPAWN_MARGIN, SCREEN_WIDTH + SPAWN_MARGIN);
          a->y = -SPAWN_MARGIN;
          a->angle = M_PI / 2; // Move downward
          a->velocity = 0.4;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 0;
        }
        break;
      }

    case 18:
      {
        // Star pattern
        int cooldown = 8000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          int numBullets = 6; // Number of bullets in the star pattern
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            // Ensure bullets spawn completely offscreen
            a->x = (centerX + cos(i * 2 * M_PI / numBullets) * (SCREEN_WIDTH / 2 + SPAWN_MARGIN));
            a->y = (centerY + sin(i * 2 * M_PI / numBullets) * (SCREEN_HEIGHT / 2 + SPAWN_MARGIN));
            //a->angle = atan2(centerY - a->y, centerX - a->x); // Aim towards the center
            a->velocity = 0.1;
            a->texture = combatUIManager->bulletTexture;
            a->red = 255;
            a->green = 128;
            a->blue = 128;
            a->exploding = 1;
            a->explosionTimer = 5300;
            a->spinSpeed = 0.001;
            a->spinAngle = i * 2 * M_PI / numBullets;
            a->radius = 512;
          }
        }
        break;
      }
    case 19:
      {
        // Sin wave pattern for the emitter's x position
        int cooldown = 800; // Time between each bullet
        static float time = 0; // Time variable for the sine wave
        float amplitude = SCREEN_WIDTH / 2; // Amplitude of the sine wave
        float frequency = 0.4f; // Frequency of the sine wave

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = SCREEN_WIDTH / 2 + amplitude * sin(frequency * time); // Sin wave x position
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom
          a->angle = -M_PI / 2; // Move upward
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 128;

          // Increment time for the next bullet to create the sin wave effect
          time += 1;
        }
        break;
      }
    case 20:
      {
        // Spinning spiral pattern with bullets spawning offscreen
        int cooldown = 300;
        static float phase = 0;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          int numBullets = 5;
          for (int i = 0; i < numBullets; i++) {
            miniBullet* a = new miniBullet();
            float angle = phase + i * (2 * M_PI / numBullets);
            float radius = SCREEN_WIDTH / 2 + SPAWN_MARGIN; // Spawn offscreen
            a->x = centerX + cos(angle) * radius;
            a->y = centerY + sin(angle) * radius;
            a->angle = angle + M_PI / 2;
            a->texture = combatUIManager->bulletTexture;
            a->red = 128;
            a->green = 128;
            a->blue = 255;
            a->spinSpeed = 0.001;
            a->spinAngle = angle;
            a->radius = radius;
            a->centerX = centerX;
            a->centerY = centerY;
          }
          phase += 0.1;
        }
        break;
      }
    case 21:
      {
        // Random wave pattern with bullets spawning offscreen
        int cooldown = 200; // Time between each bullet
        static float phase = 0; // Time variable for the sine wave
        float amplitude = SCREEN_WIDTH / 4; // Amplitude of the sine wave
        float frequency = 0.05f; // Frequency of the sine wave

        if (accumulator >= cooldown) {
          accumulator = 0;

          miniBullet* a = new miniBullet();
          a->x = rng(0, SCREEN_WIDTH); // Random x position along the bottom edge
          a->y = SCREEN_HEIGHT + SPAWN_MARGIN; // Start from the bottom offscreen
          a->angle = -M_PI / 2; // Move upward
          a->velocity = 0.5;
          a->texture = combatUIManager->bulletTexture;
          a->red = 128;
          a->green = 255;
          a->blue = 128;

          // Increment phase for the next bullet to create the wave effect
          phase += 1;
        }
        break;
      }
    case 22:
      {
        // Concentric circles pattern
        int cooldown = 3000;
        if (accumulator >= cooldown) {
          accumulator = 0;
          float centerX = SCREEN_WIDTH / 2;
          float centerY = SCREEN_HEIGHT / 2;
          int numCircles = 3;
          int bulletsPerCircle = 4;
          float initialRadius = 560;
          for (int j = 0; j < numCircles; j++) {
            for (int i = 0; i < bulletsPerCircle; i++) {
              miniBullet* a = new miniBullet();
              float angle = i * 2 * M_PI / bulletsPerCircle;
              float radius = initialRadius + j * 50;
              a->x = centerX + cos(angle) * radius;
              a->y = centerY + sin(angle) * radius;
              a->angle = angle + M_PI / 2;
              a->texture = combatUIManager->bulletTexture;
              a->red = 50;
              a->green = 190;
              a->blue = 180;
              a->spinSpeed = 0.0001 + j * 0.0002;
              a->spinAngle = angle;
              a->radius = radius;
              a->centerX = centerX;
              a->centerY = centerY;
              a->velocity = 0.08;
            }
          }
        }
        break;
      }
    case 23:
      {
        int cooldown = 100000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 3; i++) {
            miniBullet* a = new miniBullet();
            a->angle = atan2(512 - a->y, 512 - a->x);
            a->texture = combatUIManager->bulletTexture;
            a->blue = 0;
            a->green = 80;
            a->canBounce = 1;
            a->gravityAccelY = 0.02;

          }
        }
        break;
      }
    case 24:
      {
        int cooldown = 100000;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->blue = 80;
            a->angle = atan2(512 - a->y, 512 - a->x);
            a->green = 0;
            a->canBounce = 1;
            a->velocity = 0.25;
            a->w = 250;
            a->h = 250;

          }
        }
        break;
      }
    case 25:
      {
        int cooldown = 3500;
        if(accumulator >= cooldown) {
          accumulator = 0;
          for(int i = 0; i < 1; i++) {
            miniBullet* a = new miniBullet();
            a->texture = combatUIManager->bulletTexture;
            a->red = 80;
            a->blue = 0;
            a->canBounce = 1;
            a->velocity = 0.25;
            a->w = 150;
            a->h = 150;
            a->exploding = 2;
            a->numFragments = 4;
            a->fragSize = 0.5;
            a->completelyRandomExplodeAngle =1;
            a->explosionTimer = rng(700, 2200);
          }
        }
        break;
      }
  }
}

void initTables() {
  {
    pronounTable.push_back(getLanguageData("PossessivePronoun0"));
    pronounTable.push_back(getLanguageData("PossessivePronoun1"));
    pronounTable.push_back(getLanguageData("PossessivePronoun2"));
    pronounTable.push_back(getLanguageData("PossessivePronoun3"));

    pronounTable.push_back(getLanguageData("ReflexivePronoun0"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun1"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun2"));
    pronounTable.push_back(getLanguageData("ReflexivePronoun3"));

    pronounTable.push_back(getLanguageData("SubjectivePronoun0"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun1"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun2"));
    pronounTable.push_back(getLanguageData("SubjectivePronoun3"));
  }

  {
    itemsTable[0] = itemInfo(getLanguageData("I0"), 1);
    itemsTable[1] = itemInfo(getLanguageData("I1"), 2);
    itemsTable[2] = itemInfo(getLanguageData("I2"), 2);
  }

  {
    // 0 -> enemy targeted
    // 1 -> ally targeted
    // 2 -> untargeted
    //name, targeting, cost
    spiritTable[0] = spiritInfo(getLanguageData("S0"), 0, 1); //Debug
    spiritTable[1] = spiritInfo(getLanguageData("S1"), 2, 5); //Harden
    spiritTable[2] = spiritInfo(getLanguageData("S2"), 0, 2); //Tackle
    spiritTable[3] = spiritInfo(getLanguageData("S3"), 1, 5); //Coffee 
    spiritTable[4] = spiritInfo(getLanguageData("S4"), 2, 2); //Chant
    spiritTable[5] = spiritInfo(getLanguageData("S5"), 0, 5); //Inspect
    spiritTable[6] = spiritInfo(getLanguageData("S6"), 0, 2); //Taunt
    spiritTable[7] = spiritInfo(getLanguageData("S7"), 0, 2); //Slime
    spiritTable[8] = spiritInfo(getLanguageData("S8"), 2, 2); //Synchronize


  }
}

void initCombat() {

}

int xpToLevel(int xp) {
  int baseXP = 100;
  int level = 0;
  int totalXP = baseXP;

  while(xp >= totalXP) {
    level++;
    totalXP+= static_cast<int>(baseXP * pow(1.5, level - 1));
  }
  if(level > 100) {
    level = 100;
  }

  return level;
}

int levelToXp(int level) {
  if(level > 100) {
    level = 100;
  }
  int baseXP = 100;
  int totalXP = baseXP;

  for (int i = 1; i < level; i++) {
    totalXP += static_cast<int>(baseXP * std::pow(1.5, i - 1));
  }

  return totalXP;
}

void useItem(int item, int target, combatant* user) {
  switch(item) {
    case 0:
      {
        //Bandage
        int mag = 50.0f * frng(0.70, 1.30) * (user->baseSkill);
        g_partyCombatants[target]->health += mag;
        string message = user->name + " healed " + g_partyCombatants[target]->name + " for " + to_string(mag) + ".";
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        if(g_partyCombatants[target]->health >= g_partyCombatants[target]->curStrength) {
          g_partyCombatants[target]->health = g_partyCombatants[target]->curStrength;
        }
        combatUIManager->dialogProceedIndicator->y = 0.25;
        break;
      }
    case 1:
      {
        //Bomb
        int mag = 50.0f * frng(0.70, 1.30) * user->baseSkill;
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          g_enemyCombatants[i]->health -= mag;
          user->dmgDealtOverFight += min(g_enemyCombatants[i]->health, (int)mag);
          string message = g_enemyCombatants[i]->name + " took " + to_string(mag) + " from the bomb.";
          combatUIManager->queuedStrings.push_back(make_pair(message,0));
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          combatant* e = g_enemyCombatants[i];
          if(e->health < 0) {
            string deathmessage = e->name + " " + e->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(deathmessage,1));
            g_enemyCombatants.erase(g_enemyCombatants.begin() + i);
            g_deadCombatants.push_back(e);
            //delete e;
            i--;
          }
        }
        combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
        combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
        user->serial.target = 1;
        break;
      }
    case 2:
      {
        //Glasses
        //raise damage of next spirit attack by 300%

        break;
      }
  }
}

//note that statuses are procked after the enemy turn for the enemies and after the
//player turn for the protags.
//
// As such, if a protag's move is meant to decrease an enemy's defense, the move itself
// should have a line like enemy->curDefense = enemy->baseDefense * 0.8;
// and the status prock code should have a similar line, sine curDefense is reset before 
// applying statuses
//
void useSpiritMove(int spiritNumber, int target, combatant* user) {
  user->sp -= spiritTable[spiritNumber].cost;
  switch(spiritNumber) {
    case 0: //test move
      {
        float baseDmg = 10;
        if(g_enemyCombatants[target]->myType == type::DEMON) {
          baseDmg *= 2;
        }

        int mag = (baseDmg + (user->curSoul * 1) );
        mag*= frng(0.8, 1.2);

        if(mag < 0) {mag = 0;}
        g_enemyCombatants[target]->health -= mag;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);

        string message = getLanguageData("DebugMoveText");
        message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[target];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,1));
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
          g_deadCombatants.push_back(e);
          //delete e;
        }
        break;
      }
    case 1: //Harden
      {
        //see if we already have the status
        bool alreadyHave = 0;
        for(auto &x : user->statuses) {
          if(x.type == status::TOUGHENED) {
            alreadyHave = 1;
            if(x.turns < 6) {
              x.turns = 6;
            }
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry e;
          e.type = status::TOUGHENED;
          e.turns = 6;
          e.magnitude = 1.1 + (0.02 * user->curSoul);
          user->statuses.push_back(e);
        }

        string message = getLanguageData("HardenMoveText");
        message = stringMultiInject(message, {user->name, getPossessivePronoun(user)});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 2: //Tackle
      {
        float baseDmg = 10;

        float mag = (baseDmg + (user->curSoul * 1.2) );
        mag*= frng(0.8, 1.2);

        int dmg = mag - g_enemyCombatants[target]->curDefense;
        if(dmg < 0) {dmg = 0;}

        g_enemyCombatants[target]->health -= dmg;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)mag);
 
        int selfdmg = (mag - user->curDefense) * 0.2;
        if(selfdmg < 0) {selfdmg = 0;}
        if(selfdmg > user->health) {selfdmg = user->health;}
        user->health -= selfdmg;

        string message = getLanguageData("TackleMoveText");
        message = stringMultiInject(message, {user->name, g_enemyCombatants[target]->name, to_string(dmg), to_string(selfdmg)});

        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[target];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(make_pair(deathmessage,1));
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
          g_deadCombatants.push_back(e);
          //delete e;
        }
        break;
      }
    case 3: //Coffee
      {
        float baseHeal = 2.5;
        float mag = baseHeal + (user->curSoul * 0.5);

        mag*= frng(0.8, 1.2);
        int heal = mag;
        if(heal < 0) {heal = 0;}
        combatant* healedOne = g_partyCombatants[target];
        if(heal + healedOne->health > healedOne->curStrength) {
          heal = healedOne->curStrength - healedOne->health;
        }
        bool healedOneAlive = 0;
        string message = "";
        if(healedOne->health > 0) {
          healedOneAlive = 1;
        }
        if(healedOneAlive) {
          healedOne->health += heal;
          if(healedOne->health > healedOne->curStrength) {
            healedOne->health = healedOne->curStrength;
          }
          message = stringMultiInject(getLanguageData("CoffeeMoveSuccessText"), {healedOne->name, to_string(heal)});
        } else {
          message = stringMultiInject(getLanguageData("CoffeeMoveFailText"), {healedOne->name});
        }


        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        break;
      }
    case 4: //Chant
      {
        //see if we already have the status
        bool alreadyHave = 0;
        for(auto &x : user->statuses) {
          if(x.type == status::CHANTED) {
            alreadyHave = 1;
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry e;
          e.type = status::CHANTED;
          e.turns = 1;
          e.magnitude = 100;
          user->statuses.push_back(e);
        }

        string message = stringMultiInject(getLanguageData("ChantMoveText"), {user->name});
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 5: //Inspect
      {
        combatant* e = g_enemyCombatants[target];
        string message = getLanguageData("InspectMoveText0");
        D(message);
        message = stringMultiInject(message, {to_string(e->level), e->name, to_string(e->baseStrength), to_stringF(e->baseAttack), to_stringF(e->baseDefense)});
        string message2 = getLanguageData("InspectMoveText1");
        message2 = stringMultiInject(message2, {getSubjectivePronoun(e), to_string(e->health), to_stringF(e->curStrength), to_stringF(e->curDefense)});
        string message3 = getLanguageData("InspectMoveText2");
        message3 = stringMultiInject(message3, {e->name, getLanguageData("CombatType"+ to_string((int)e->myType)) });



        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        combatUIManager->queuedStrings.push_back(make_pair(message2,0));
        combatUIManager->queuedStrings.push_back(make_pair(message3,0));
        break;
      }
    case 6: //Taunt
      {

        //see if they already have the status
        bool alreadyHave = 0;
        combatant* e = g_enemyCombatants[target];
        for(auto &x : e->statuses) {
          if(x.type == status::TAUNTED) {
            alreadyHave = 1;
            if(x.turns < 6) {
              x.turns = 6;
            }
            break;
          }
        }

        if(!alreadyHave) {
          statusEntry se;
          se.type = status::TAUNTED;
          se.turns = 6;
          se.magnitude = 40 + (user->curSoul * 1);
          se.datastr = user->filename;
          e->statuses.push_back(se);
        }

        //string message = user->name + " taunts " + e->name + ".";
        string message = getLanguageData("TauntMoveText");
        message = stringMultiInject(message, {user->name, e->name});


        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 7: //Slime
      {
        bool alreadyHave = 0;
        combatant* e = g_enemyCombatants[target];
        int dmg = 0;
        for(auto &x : e->statuses) {
          if(x.type == status::SLIMED) {
            alreadyHave = 1;
            x.magnitude += x.magnitude * (user->curSoul / 40);
            dmg = x.magnitude * frng(0.8, 1.2);
            break;
          }
        }


        if(!alreadyHave) {
          statusEntry se;
          se.type = status::SLIMED;
          se.turns = 1;
          se.magnitude = 5;
          se.datastr = user->filename;
          e->statuses.push_back(se);
          dmg = se.magnitude * frng(0.8, 1.2);
        }

        g_enemyCombatants[target]->health -= dmg;
        user->dmgDealtOverFight += min(g_enemyCombatants[target]->health, (int)dmg);

        string message = user->name + " slimes " + e->name + " for " + to_string(dmg) + " damage.";
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        break;
      }
    case 8:
      {
        vector<string> msgs = {};
        for(unsigned int i = 0; i < g_partyCombatants.size(); i++) {
          //see if they already have the status
          bool alreadyHave = 0;
          combatant*e = g_partyCombatants[i];
          for(auto &x : e->statuses) {
            if(x.type == status::SYNCHRONIZED) {
              alreadyHave = 1;
              if(x.turns < 4) {
                x.turns = 4;
              }
              break;
            }
          }
  
          if(!alreadyHave) {
            statusEntry se;
            se.type = status::SYNCHRONIZED;
            se.turns = 4;
            //can't boost Synchronize with Synchronize
            //int soulToUse = min(user->baseSoul, user->curSoul);
            se.magnitude = 0 + (user->curSoul * 0.01);
            se.datastr = user->filename;
            e->statuses.push_back(se);
          }

          string message = e->name + " is in sync with " + getPossessivePronoun(e) + " friends.";
          msgs.push_back(message);
        }

        combatUIManager->finalText = msgs.at(0);
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;

        for(int i = 1; i < msgs.size(); i++) {
          combatUIManager->queuedStrings.push_back(make_pair(msgs[i], 0));
        }

        break;
      }
  }
}

//given a combatant, and a statusEntry with duration, type, and magnitude, apply it to them
bool applyStatus(combatant* c, statusEntry* e) {
  switch(e->type) {
    case status::NONE:
      {
        break;
      }
    case status::TOUGHENED:
      {
        if(e->turns <= 0) {
          //the status wore off
          combatUIManager->finalText = c->name + "'s Harden has worn off.";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;

        } else {
          //the status is applied
          c->curDefense = c->baseDefense * e->magnitude;
          e->turns--;
          //curStatusIndex++;
        }
        break;
      }
    case status::CHANTED:
      {
        if(e->turns <= 0) {
          combatUIManager->finalText = c->name + "'s chant has worn off.";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;
        }
        c->curCritical = 100;
        //curStatusIndex++;
        break;
      }
    case status::TAUNTED:
      {
        if(e->turns <= 0) {
          combatUIManager->finalText = c->name + "'s is no longer Taunted.";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_E;
          return 1;
        } else {
          e->turns--;
        }
        break;
      }

    case status::SLIMED:
      {
        //complete
        break;
      }
    case status::SYNCHRONIZED:
      {
        if(e->turns <= 0) {
          combatUIManager->finalText = c->name + " is out-of-sync.";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_STATUS_P;
          return 1;
        }
        c->curSoul *= e->magnitude;
        e->turns--;

        break;
      }
  }
  curStatusIndex++;
  return 0;
}

void combatUI::calculateXP() {
  xpToGrant = 0;
  for(auto x : g_deadCombatants) {
    int dmg = x->baseAttack;
    int def = x->baseDefense;
    int het = x->baseStrength;
    int sum = dmg + def + het;
    xpToGrant += sum;
  }
}

combatUI::combatUI(SDL_Renderer* renderer) {
  initTables();

  directionalPreposition = getLanguageData("DirectionalPreposition");

  options[0] = getLanguageData("CombatOption1");
  options[1] = getLanguageData("CombatOption2");
  options[2] = getLanguageData("CombatOption3");
  options[3] = getLanguageData("CombatOption4");
  options[4] = getLanguageData("CombatOption5");
  options[5] = getLanguageData("CombatOption6");

  partyHealthBox = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
  partyHealthBox->patchwidth = 213;
  partyHealthBox->patchscale = 0.4;
  partyHealthBox->is9patch = true;
  partyHealthBox->persistent = true;
  partyHealthBox->show = 0;

  partyText = new textbox(renderer, "", 1, 0, 0, 0.9);
  partyText->boxWidth = 0;
  partyText->width = 0.95;
  partyText->boxHeight = 0;
  partyText->boxX = 0.2;
  partyText->boxY = 1-0.1;
  partyText->align = 0;
  partyText->dropshadow = 1;
  partyText->show = 1;

  partyMiniText = new textbox(renderer, "", 0, 0, 0, 0.9);
  partyMiniText->boxWidth = 0;
  partyMiniText->width = 0.95;
  partyMiniText->boxHeight = 0;
  partyMiniText->boxX = 0.2;
  partyMiniText->boxY = 1-0.1;
  partyMiniText->align = 1;
  partyMiniText->dropshadow = 1;
  partyMiniText->show = 1;

  mainPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
  mainPanel->patchwidth = 213;
  mainPanel->patchscale = 0.4;
  mainPanel->is9patch = true;
  mainPanel->persistent = true;
  mainPanel->y = 0;

  dialogProceedIndicator = new ui(renderer, "resources/engine/dialog_proceed.qoi", 0.92, 0.88, 0.05, 1, 0);
  dialogProceedIndicator->heightFromWidthFactor = 1;
  dialogProceedIndicator->persistent = true;
  dialogProceedIndicator->priority = 8;
  dialogProceedIndicator->dropshadow = 1;
  dialogProceedIndicator->y =  0.25;

  mainText = new textbox(renderer, "", 2, 0, 0, 0.9);
  mainText->boxWidth = 0.9;
  mainText->width = 0.9;
  mainText->boxHeight = 0.25;
  mainText->boxX = 0.05;
  mainText->boxY = 0.05;
  mainText->dropshadow = 1;

  optionsPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.08, 0.05, 0.48, 0.225, 0);
  optionsPanel->patchwidth = 213;
  optionsPanel->patchscale = 0.4;
  optionsPanel->is9patch = true;
  optionsPanel->persistent = true;

  optionsText = new textbox(renderer, "", 1, 0, 0, 0.9);
  optionsText->boxWidth = 0.9;
  optionsText->width = 0.9;
  optionsText->boxHeight = 0.25;
  optionsText->boxX = 0.1;
  optionsText->boxY = 0.05;
  optionsText->dropshadow = 1;

  optionsMiniText = new textbox(renderer, "", 0, 0, 0, 0.9);
  optionsMiniText->boxWidth = 0;
  optionsMiniText->width = 0.95;
  optionsMiniText->boxHeight = 0;
  optionsMiniText->boxX = 0.127;
  optionsMiniText->boxY = 0.07;
  optionsMiniText->dropshadow = 1;
  optionsMiniText->show = 1;

  menuPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.03, 1, 0);
  menuPicker->heightFromWidthFactor = 1;
  menuPicker->persistent = true;
  menuPicker->priority = 8;
  menuPicker->dropshadow = 1;
  menuPicker->y =  0.25;

  targetPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.18, 0.3, 0.14, 0);
  targetPanel->patchwidth = 213;
  targetPanel->patchscale = 0.4;
  targetPanel->is9patch = true;
  targetPanel->persistent = true;

  targetText = new textbox(renderer, "", 1, 0, 0, 0.9);
  targetText->boxWidth = 0.3;
  targetText->boxHeight = 0.12;
  targetText->boxX = 0.55;
  targetText->boxY = 0.22;
  targetText->align = 2;
  targetText->dropshadow = 1;

  inventoryPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.05, 0.46, 0.6, 0);
  inventoryPanel->patchwidth = 213;
  inventoryPanel->patchscale = 0.4;
  inventoryPanel->is9patch = true;
  inventoryPanel->persistent = true;

  inventoryText = new textbox(renderer, "", 1, 0, 0, 0.9);
  inventoryText->boxWidth = 0.3;
  inventoryText->boxHeight = 0.12;
  inventoryText->boxX = 0.45;
  inventoryText->boxY = 0.22;
  inventoryText->dropshadow = 1;

  spiritPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.41, 0.05, 0.34, 0.38, 0);
  spiritPanel->patchwidth = 213;
  spiritPanel->patchscale = 0.4;
  spiritPanel->is9patch = true;
  spiritPanel->persistent = true;

  spiritText = new textbox(renderer, "", 1, 0, 0, 0.9);
  spiritText->boxWidth = 0.3;
  spiritText->boxHeight = 0.12;
  spiritText->boxX = 0.45;
  spiritText->boxY = 0.22;
  spiritText->dropshadow = 1;

  forgetPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.35, 0.25, 1- 0.35*2, 0.42, 0);
  forgetPanel->patchwidth = 213;
  forgetPanel->patchscale = 0.4;
  forgetPanel->is9patch = true;
  forgetPanel->persistent = true;

  forgetText = new textbox(renderer, "", 1, 0, 0, 0.9);
  forgetText->boxWidth = 0.3;
  forgetText->boxHeight = 0.12;
  forgetText->boxX = 0.45;
  forgetText->boxY = 0.22;
  forgetText->dropshadow = 1;

  forgetPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.04, 1, 0);
  forgetPicker->heightFromWidthFactor = 1;
  forgetPicker->persistent = true;
  forgetPicker->priority = 8;
  forgetPicker->dropshadow = 1;
  forgetPicker->y =  0.25;

  yes = new textbox(renderer, "Yes", 1, 0, 0, 0.9);
  yes->boxX = 0.50 - 0.07;
  yes->boxY = 0.2;
  yes->boxWidth = 0.01;
  yes->boxHeight = 0.35;
  yes->dropshadow = 1;
  yes->align = 2;

  no = new textbox(renderer, "No", 1, 0, 0, 0.9);
  no->boxX = 0.50 + 0.07;
  no->boxY = 0.2;
  no->boxWidth = 0;
  no->boxHeight = 0.35;
  no->dropshadow = 1;
  no->align = 2;

  confirmPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.04, 1, 0);
  confirmPicker->heightFromWidthFactor = 1;
  confirmPicker->persistent = true;
  confirmPicker->priority = 8;
  confirmPicker->dropshadow = 1;
  confirmPicker->y =  0.25;

  aspect = 16.0f / 10.0f;
  dodgePanelFullWidth = 0.25;
  dodgePanelFullHeight = 0.25 * aspect;
  dodgePanelFullX = 0.5 - dodgePanelFullWidth/2;
  dodgePanelFullY = 0.5 - dodgePanelFullHeight/2;

  dodgePanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.5- dodgePanelFullX, dodgePanelFullY, dodgePanelFullWidth, dodgePanelFullHeight, 0);
  dodgePanel->patchwidth = 213;
  dodgePanel->patchscale = 0.4;
  dodgePanel->is9patch = true;
  dodgePanel->persistent = true;

  const char* file = "resources/static/sprites/minigame/fomm.qoi";
  dodgerTexture = loadTexture(renderer, file);

  file = "resources/static/sprites/minigame/bullet.qoi";
  bulletTexture = loadTexture(renderer, file);

  rendertarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, 1024, 1024);

  db1 = SDL_CreateRGBSurface(0, 512, 512, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);



}

combatUI::~combatUI() {
  delete mainPanel;
  delete dialogProceedIndicator;
  delete mainText;
  delete optionsPanel;
  delete optionsText;
  delete menuPicker;
}

void drawOptionsPanel() {
  combatUIManager->optionsPanel->render(renderer, g_camera, elapsed);

  combatUIManager->optionsMiniText->updateText(g_partyCombatants[curCombatantIndex]->name, -1, 0.85, g_textcolor, g_font);
  combatUIManager->optionsMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  const int rows = 2;
  const int columns = 3;
  const float width = 0.16;
  const float height = 0.07;
  const float initialX = 0.125;
  const float initialY = 0.1;
  int index = 0;
  for(int i = 0; i < columns; i++) {
    for(int j = 0; j < rows; j++) {
      combatUIManager->optionsText->boxX = initialX + (i * width);
      combatUIManager->optionsText->boxY = initialY + (j * height);
      combatUIManager->optionsText->updateText(combatUIManager->options[index], -1, 0.85, g_textcolor, g_font);
      if(index == combatUIManager->currentOption) {
        combatUIManager->menuPicker->x = initialX + (i * width) - 0.027;
        combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
      }

      combatUIManager->optionsText->render(renderer, WIN_WIDTH, WIN_HEIGHT);


      index++;
    }
  }

  combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
}

void combatUI::hideAll() {
  partyHealthBox->show = 0;
  partyText->show = 0;
  partyMiniText->show = 0;
  mainPanel->show = 0;
  dialogProceedIndicator->show = 0;
  mainText->show = 0;
  optionsPanel->show = 0;
  optionsText->show = 0;
  optionsMiniText->show = 0;
  menuPicker->show = 0;
  targetPanel->show = 0;
  targetText->show = 0;
  inventoryPanel->show = 0;
  inventoryText->show = 0;
  spiritPanel->show = 0;
  spiritText->show = 0;
  dodgePanel->show = 0;
  forgetPanel->show = 0;
  forgetText->show = 0;
  forgetPicker->show = 0;
  yes->show = 0;
  no->show = 0;
  confirmPicker->show = 0;
}

void getCombatInput() {
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  SDL_PollEvent(&event);

  if (keystate[SDL_SCANCODE_F] && fullscreen_refresh)
  {
    toggleFullscreen();
  }

  if (keystate[SDL_SCANCODE_G] && !inputRefreshCanSwitchOffDevMode && canSwitchOffDevMode)
  {
    toggleDevmode();
  }

  //up
  if (keystate[bindings[0]])
  {
    input[0] = 1;
  }
  else
  {
    input[0] = 0;
  }

  //down
  if (keystate[bindings[1]])
  {
    input[1] = 1;
  }
  else
  {
    input[1] = 0;
  }

  //left
  if (keystate[bindings[2]])
  {
    input[2] = 1;
  }
  else
  {
    input[2] = 0;
  }

  //right
  if (keystate[bindings[3]])
  {
    input[3] = 1;
  }
  else
  {
    input[3] = 0;
  }

  //item button
  if (keystate[bindings[10]])
  {
    input[10] = 1;
  }
  else
  {
    input[10] = 0;
  }

  //interact button
  if (keystate[bindings[11]])
  {
    input[11] = 1;
  }
  else
  {
    input[11] = 0;
  }

  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }

  //jump button
  if (keystate[bindings[8]])
  {
    input[8] = 1;
  }
  else
  {
    input[8] = 0;
  }

}

void renderSpiritPanel()
{
  const int rows = 4;
  const int columns = 1;
  const float width = 0.16;
  const float height = 0.07;
  const float initialX = 0.45;
  const float initialY = 0.1;
  int index = 0;

  combatUIManager->menuPicker->x = 10;
  if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
    for(int i = 0; i < columns; i++) {
      for(int j = 0; j < rows; j++) {
        combatUIManager->spiritText->boxX = initialX + (i * width);
        combatUIManager->spiritText->boxY = initialY + (j * height);
        string spiritName = "";
        spiritName = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].name;

        combatUIManager->spiritText->align = 0;
        combatUIManager->spiritText->updateText(spiritName, -1, 0.85, g_textcolor, g_font);
        if(index == combatUIManager->currentInventoryOption) {
          combatUIManager->menuPicker->x = initialX + (i * width) - 0.027;
          combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
        }
        combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

        //render cost
        if(index < g_partyCombatants[curCombatantIndex]->spiritMoves.size() && spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].cost > 0)
        {
          combatUIManager->spiritText->align = 1;
          combatUIManager->spiritText->boxX += 0.27;
          combatUIManager->spiritText->boxWidth = 0;
          combatUIManager->spiritText->updateText(to_string(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].cost), -1, 0.85, g_textcolor, g_font);
          combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        }



        index++;
      }
    }
    combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
  }
}

void renderInventoryPanel() 
{
  const int rows = 7;
  const int columns = 2;
  const float width = 0.24;
  const float height = 0.07;
  const float initialX = 0.45;
  const float initialY = 0.1;
  int index = 0;
  if(g_combatInventory.size() > 0) {
    for(int i = 0; i < columns; i++) {
      for(int j = 0; j < rows; j++) {
        combatUIManager->inventoryText->boxX = initialX + (i * width);
        combatUIManager->inventoryText->boxY = initialY + (j * height);

        int itemIndex = 0;
        if(index < g_combatInventory.size() && index >= 0) {
          itemIndex = g_combatInventory[index];
        }

        string itemName = "";
        if(itemIndex >= 0 && itemIndex < itemsTable.size()) {
          itemName = itemsTable[itemIndex].name;
        } else {
          E("Bad itemIndex " + itemIndex);
          abort();
        }
        combatUIManager->inventoryText->updateText(itemName, -1, 0.85, g_textcolor, g_font);
        if(index == combatUIManager->currentInventoryOption) {
          combatUIManager->menuPicker->x = initialX + (i * width) - 0.028;
          combatUIManager->menuPicker->y = initialY + (j * height) + 0.007;
        }
        if(index < (int)g_combatInventory.size()) {
          combatUIManager->inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        }

        index++;
      }
    }
    combatUIManager->inventoryText->boxX = 10;
    combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
  }
}


void drawCombatants() {
  int count = g_enemyCombatants.size();

  // Total padding on both sides
  int padding = 0.05 * WIN_WIDTH;

  // Calculate total available width for spacing
  int availableWidth = WIN_WIDTH - (2 * padding);

  // Calculate gap between combatants' centers
  int gap = availableWidth / (count + 1);

  // Starting x position for the first combatant's center
  int xCenter = padding + gap;

  for (int i = 0; i < count; ++i) {
    combatant* combatant = g_enemyCombatants[i];

    // Set renderQuad if it hasn't been initialized
    if (combatant->renderQuad.x == -1) {

      // Calculate actual width and height
      int actual_width = static_cast<int>(WIN_WIDTH * combatant->width);
      int actual_height = static_cast<int>(WIN_WIDTH * combatant->height);

      // Calculate x and y position for this combatant
      int x = xCenter - (actual_width / 2);
      int y = WIN_HEIGHT *(0.43 + combatant->offset); // Center vertically

      //to make them seem equidistant from the camera, move ones closer to the
      //edges of the screen down a bit (to go on the guidecircle)
      int distFromCenter = 0.00000003* pow(abs(xCenter - WIN_WIDTH/2), 3);
      y += distFromCenter;
      actual_width += distFromCenter *2;
      actual_height += distFromCenter *2;


      SDL_Rect renderQuad = { x, y, actual_width, actual_height };
      combatant->renderQuad = renderQuad;
    }

    // Apply targeting color mod if needed
    if (g_submode == submode::TARGETING && i == combatUIManager->currentTarget) {
      SDL_SetTextureColorMod(combatant->texture, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod);
    } else {
      SDL_SetTextureColorMod(combatant->texture, 255, 255, 255);
    }

    // Render the combatant
    SDL_RenderCopy(renderer, combatant->texture, nullptr, &combatant->renderQuad);

    // Update xCenter for the next combatant
    xCenter += gap;
  }

  for(auto x : g_deadCombatants) {
    SDL_SetTextureAlphaMod(x->texture, x->opacity);
    if(x->disappearing && x->opacity-1 > 0) {
      x->opacity-=5;
    }
    SDL_RenderCopy(renderer, x->texture, nullptr, &x->renderQuad);
  }

  SDL_SetTextureAlphaMod(g_shade, g_dungeonDarkEffect);
  SDL_RenderCopy(renderer, g_shade, NULL, NULL);

  count = g_partyCombatants.size();
  combatUIManager->partyHealthBox->show = 1;
  gap = 0.1;

  for (int i = 0; i < count; ++i) {
    combatant* combatant = g_partyCombatants[i];
    float bonusY = 0;
    if(g_submode == submode::MAIN ||
        g_submode == submode::TARGETING ||
        g_submode == submode::ITEMCHOOSE ||
        g_submode == submode::ALLYTARGETING ||
        g_submode == submode::SPIRITCHOOSE ||
        g_submode == submode::SPWARNING ||
        g_submode == submode::RUNWARNING ||
        g_amState == amState::SPIRIT ||
        g_amState == amState::STARGETING ||
        g_amState == amState::SPIRITSELECT
        ) {
      if(i == curCombatantIndex){
        bonusY = -0.05;
      }

    }

    combatUIManager->partyText->textcolor = { 155, 115, 115};
    if(combatant->health <= 0) {
      combatUIManager->partyText->textcolor = g_healthtextlowcolor;
    }

    if(g_submode == submode::ALLYTARGETING
        || g_amState == amState::STARGETING
        || g_amState == amState::ITARGETING) {
      if(i == combatUIManager->currentTarget) {
        combatUIManager->partyText->textcolor = { 108, 80, 80};
      } else {
        combatUIManager->partyText->textcolor = { 155, 115, 115};
      }
    }



    // Convert percentage-based width and height to actual pixel values
    float actual_width = 0.3 *  WIN_HEIGHT / WIN_WIDTH;
    float actual_height = 0.3;

    // Calculate X position to arrange horizontally
    float total_width = (count * actual_width) + ((count - 1) * gap);
    float x = (1 - total_width) / 2 + i * (actual_width + gap);
    float y = (1 - actual_height) / 2; // Centering vertically

    combatUIManager->partyHealthBox->x = x;
    combatUIManager->partyHealthBox->y = 0.7 + bonusY;
    combatUIManager->partyHealthBox->width = actual_width;
    combatUIManager->partyHealthBox->height = actual_height;

    combatUIManager->partyHealthBox->render(renderer, g_camera, elapsed);



    combatUIManager->partyText->boxX = x + 0.02;
    combatUIManager->partyText->boxY = 0.7 + 0.02 + bonusY;
    combatUIManager->partyText->boxWidth = actual_width;
    combatUIManager->partyText->boxHeight = actual_height;
    combatUIManager->partyText->updateText(combatant->name, -1, 34, combatUIManager->partyText->textcolor);
    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    combatUIManager->partyText->boxY += 0.07;
    combatUIManager->partyText->updateText(to_string(combatant->health), -1, 34, combatUIManager->partyText->textcolor);
    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->partyText->boxY += 0.07;
    combatUIManager->partyText->updateText(to_string(combatant->sp), -1, 34, combatUIManager->partyText->textcolor);
    combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    combatUIManager->partyMiniText->show = 1;
    combatUIManager->partyMiniText->boxX = x + 0.15 + 0.02;
    combatUIManager->partyMiniText->boxY = 0.7 + 0.02 + bonusY + 0.073;
    combatUIManager->partyMiniText->boxWidth = actual_width;
    combatUIManager->partyMiniText->boxHeight = actual_height;

    combatUIManager->partyMiniText->updateText("HP", -1, 1, combatUIManager->partyText->textcolor);
    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->partyMiniText->boxY += 0.025;
    combatUIManager->partyMiniText->updateText('/' + to_stringF(combatant->curStrength), -1, 1, combatUIManager->partyText->textcolor);
    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->partyMiniText->boxY += 0.045;
    combatUIManager->partyMiniText->updateText("SP", -1, 1, combatUIManager->partyText->textcolor);
    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->partyMiniText->boxY += 0.025;
    combatUIManager->partyMiniText->updateText('/' + to_stringF(combatant->curMind), -1, 1, combatUIManager->partyText->textcolor);
    combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);


  }
  combatUIManager->partyHealthBox->show = 0;
  combatUIManager->partyMiniText->x = 10;


}

void CombatLoop() {
  getCombatInput();

  SDL_RenderClear(renderer);

  updateWindowResolution();

  if(combatUIManager->loadedBackground.scene[0] == '>') {
    drawBackground();
  } else {
    drawSimpleBackground();
  }

  drawCombatants();
  switch (g_submode) {
    case submode::BEFORE:
      {
        for(int i = 0; i < 4; i ++) {
          combatUIManager->dodgingThisTurn[i] = 0;
        }

        for(auto &x : g_partyCombatants) {
          x->dmgDealtOverFight = 0;
          x->dmgTakenOverFight = 0;
        }

        g_autoFight = 0;

        //clear statuses
        for(auto x : g_partyCombatants) {
          x->statuses.clear();
          x->curStrength = x->baseStrength;
          x->curMind = x->baseMind;
          x->curAttack = x->baseAttack;
          x->curDefense = x->baseDefense;
          x->curSoul = x->baseSoul;
          x->curSkill = x->baseSkill;
          x->curCritical = x->baseCritical;
          x->curRecovery = x->baseSoul;
        }

        g_forceEndDialogue = 0;
        g_submode = submode::INWIPE;
      }
    case submode::INWIPE:
      {
        // onframe things
        SDL_LockTexture(transitionTexture, NULL, &transitionPixelReference, &transitionPitch);

        memcpy(transitionPixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
        Uint32 format = SDL_PIXELFORMAT_ARGB8888;
        SDL_PixelFormat *mappingFormat = SDL_AllocFormat(format);
        Uint32 *pixels = (Uint32 *)transitionPixelReference;
        // int numPixels = transitionImageWidth * transitionImageHeight;
        Uint32 transparent = SDL_MapRGBA(mappingFormat, 0, 0, 0, 255);
        // Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
        transitionDelta += g_transitionSpeed + 0.02 * transitionDelta;
        for (int x = 0; x < transitionImageWidth; x++)
        {
          for (int y = 0; y < transitionImageHeight; y++)
          {
            int dest = (y * transitionImageWidth) + x;

            if (pow(pow(transitionImageWidth / 2 - x, 2) + pow(transitionImageHeight + y, 2), 0.5) < transitionDelta)
            {
              pixels[dest] = 0;
            }
            else
            {
              pixels[dest] = transparent;
            }
          }
        }

        ticks = SDL_GetTicks();
        elapsed = ticks - lastticks;

        SDL_UnlockTexture(transitionTexture);
        SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

        if (transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth / 2, 2) + pow(transitionImageHeight, 2), 0.5))
        {
          g_submode = submode::TEXT;
        }
        break;
      }
    case submode::OUTWIPE:
      {
        M("OUTWIPE");

        for(auto x : g_partyCombatants) {
          writeSaveField(x->filename + "-dealt", x->dmgDealtOverFight);
          writeSaveField(x->filename + "-taken", x->dmgTakenOverFight);
        }

        {
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

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
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
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        if(g_combatWorldEnt != nullptr) {
          for(auto x : g_combatWorldEnt->children) {
            //x->tangible = 0;
            x->opacity_delta = -3;
            x->semisolid = 0;
          }
          //g_combatWorldEnt->tangible = 0;
          g_combatWorldEnt->opacity_delta = -3;
          g_combatWorldEnt->semisolid = 0;
        }

        g_gamemode = gamemode::EXPLORATION;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();
        if(g_combatEntryType == 0) {
          //continue script
          adventureUIManager->dialogue_index++;
          adventureUIManager->continueDialogue();
        } else {
          //wasn't a scripted fight
        }

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        writeSave();

        break;
      }
    case submode::OUTWIPEL:
      {
        {
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

          if(combatUIManager->loadedBackground.scene[0] == '>') {
            drawBackground();
          } else {
            drawSimpleBackground();
          }
          drawCombatants();

          SDL_SetRenderTarget(renderer, NULL);
          SDL_RenderClear(renderer);

          while (!cont) {

            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);

            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);

            offset += g_transitionSpeed + 0.02 * offset;

            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {


                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;

                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  pixels[dest] = 0;
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
            //SDL_RenderCopy(renderer, frame, NULL, NULL);
            if(combatUIManager->loadedBackground.scene[0] == '>') {
              drawBackground();
            } else {
              drawSimpleBackground();
            }

            drawCombatants();

            SDL_UnlockTexture(transitionTexture);
            SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);
            SDL_RenderPresent(renderer);

            if(offset > imageHeight + pow(pow(imageWidth/2,2) + pow(imageHeight,2),0.5)) {
              cont = 1;
            }
          }
          SDL_FreeSurface(transitionSurface);
          SDL_DestroyTexture(transitionTexture);
          SDL_DestroyTexture(frame);
          SDL_GL_SetSwapInterval(1);
        }

        adventureUIManager->executingScript = 0;

        adventureUIManager->mobilize = 0;
        adventureUIManager->hideTalkingUI();
        protag_is_talking = 2;

        g_gamemode = gamemode::LOSS;
        //lossUIManager->redness = 255;
        transition = 1;
        transitionDelta = transitionImageHeight;
        combatUIManager->hideAll();

        //if the player ran away, we should delete these
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();

        break;
      }
    case submode::TEXT:
      {
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
              while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
                curCombatantIndex ++;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MAIN:
      {
        if(input[8]) {
          g_autoFight = 0;
        }

        if(g_autoFight) {

          while(curCombatantIndex < g_partyCombatants.size()) {
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
            g_partyCombatants[curCombatantIndex]->serial.target = 0;
            curCombatantIndex++;

          }
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;

          break;
        }


        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 4) {
            combatUIManager->currentOption ++;
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3 ||
              combatUIManager->currentOption == 4 ||
              combatUIManager->currentOption == 5) {
            combatUIManager->currentOption -= 2;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentOption == 0 ||
              combatUIManager->currentOption == 1 ||
              combatUIManager->currentOption == 2 ||
              combatUIManager->currentOption == 3) {
            combatUIManager->currentOption += 2;
          }
        }

        if(input[11] && !oldinput[11]) {
          switch(combatUIManager->currentOption) {
            case 0: 
              {
                //attack
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                //now, choose a target
                g_submode = submode::TARGETING;
                combatUIManager->currentTarget = 0;

                break;
              }
            case 1:
              {
                //Spirit move
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;

                //now, choose a move
                g_submode = submode::SPIRITCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 2:
              {
                //Bag
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;

                combatUIManager->currentInventoryOption = 0;

                //now, choose a target
                g_submode = submode::ITEMCHOOSE;
                combatUIManager->currentInventoryOption = 0;

                break;
              }
            case 3:
              {
                //Defend
                g_partyCombatants[curCombatantIndex]->serial.action = turnAction::DEFEND;
                g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

                g_submode = submode::CONTINUE;
                break;
              }
            case 4:
              {
                //Run
                if(adventureUIManager->executingScript) {
                  //can't run from this fight
                  combatUIManager->finalText = "Can't run from this fight!";
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  combatUIManager->dialogProceedIndicator->y = 0.25;
                  g_submode = submode::RUNWARNING;

                } else {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::FLEE;
                  g_submode = submode::CONTINUE;
                  break;



                }

                break;
              }
            case 5:
              {
                //Autofight
                g_autoFight = 1;

                while(curCombatantIndex < g_partyCombatants.size()) {
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ATTACK;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;
                  g_partyCombatants[curCombatantIndex]->serial.target = 0;
                  curCombatantIndex++;

                }
                g_submode = submode::EXECUTE_P;
                combatUIManager->executePIndex = 0;
                curCombatantIndex = 0;

                break;
              }

          }

        }

        if(input[8] && !oldinput[8]) {
          if(curCombatantIndex > 0) {
            int newCombatantIndex = curCombatantIndex - 1;
            while(newCombatantIndex >= 0 && g_partyCombatants[newCombatantIndex]->health <= 0) {
              newCombatantIndex--;
            }
            if(newCombatantIndex >= 0) {
              curCombatantIndex = newCombatantIndex;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }


        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;
        combatUIManager->optionsMiniText->show = 1;

        drawOptionsPanel();
        break;
      }
    case submode::TARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        //combatUIManager->targetingColorMod = (sin(combatUIManager->tcm_accumulator) + 1) * 128;
        combatUIManager->targetingColorMod = 128;


        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_enemyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_enemyCombatants.size()) { combatUIManager->currentTarget = g_enemyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        //combatUIManager->targetText->updateText("To " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);
        combatUIManager->targetText->updateText(combatUIManager->directionalPreposition + " " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::CONTINUE:
      {

        drawOptionsPanel();
        if(curCombatantIndex == g_partyCombatants.size() - 1) {
          g_submode = submode::EXECUTE_P;
          combatUIManager->executePIndex = 0;
          curCombatantIndex = 0;
        } else {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
          curCombatantIndex++;
          while(g_partyCombatants[curCombatantIndex]->health <= 0) {
            curCombatantIndex++;
            if(curCombatantIndex >= g_partyCombatants.size()) {
              g_submode = submode::EXECUTE_P;
              combatUIManager->executePIndex = 0;
              curCombatantIndex = 0;
            }
          }
        }


        break;
      }
    case submode::EXECUTE_P:
      {

        while(combatUIManager->executePIndex+1 <= g_partyCombatants.size() && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          combatUIManager->executePIndex++;
        }
        combatant* c = g_partyCombatants[combatUIManager->executePIndex];
        if(combatUIManager->executePIndex == g_partyCombatants.size() - 1 && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(combatUIManager->executePIndex >= g_partyCombatants.size()) {
          //reset all stats
          for(auto x : g_partyCombatants) {
            x->curStrength = x->baseStrength;
            x->curMind = x->baseMind;
            x->curAttack = x->baseAttack;
            x->curDefense = x->baseDefense;
            x->curSoul = x->baseSoul;
            x->curSkill = x->baseSkill;
            x->curCritical = x->baseCritical;
            x->curRecovery = x->baseSoul;
          }
          g_submode = submode::STATUS_P;
          break;
        }
        if(c->serial.action == turnAction::ATTACK) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_enemyCombatants.size()) {
            c->serial.target-= 1;
          }
          combatant* e = g_enemyCombatants[c->serial.target];

          //do we crit?
          bool crit = 0;
          if(frng(0, 100) < c->curCritical) {
            crit = 1;
          }
          
          //chant wears off
          for(auto &x : c->statuses) {
            if(x.type == status::CHANTED) {
              x.turns = 0;
            }
          }


          int damage = c->curAttack - e->curDefense;
          damage *= frng(0.80,1.20);
          if(crit) { damage *= 3;}
          if(damage < 0) {damage = 0;}
          int dmgToReport = damage;
          if(e->health < dmgToReport) {
            dmgToReport = e->health;
          }
          c->dmgDealtOverFight += dmgToReport;

          e->health -= damage;
          string message;
          if(crit) {
            //message = c->name + " crits " + e->name + " for " + to_string(damage) + ".";
            message = getLanguageData("CombatProtagCrit");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          } else {
            //message = c->name + " deals " + to_string(damage) + " to " + e->name + ".";
            message = getLanguageData("CombatProtagAttack");
            message = stringMultiInject(message, {c->name, e->name, to_string(damage)});
          }


          if(e->health < 0) {
            string deathmessage = e->name + " " +  e->deathText;
            combatUIManager->queuedStrings.push_back(make_pair(deathmessage, 1));
            g_enemyCombatants.erase(g_enemyCombatants.begin() + c->serial.target);
            g_deadCombatants.push_back(e);
            //delete e;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::ITEM) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          while(c->serial.target >= g_enemyCombatants.size()) {
            c->serial.target-= 1;
            if(c->serial.target <0) {break;}
          }
          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int a = com->serial.actionIndex; //which item
          int b = com->serial.target; //which ally/enemy
          com->itemToUse = -1;

          useItem(a, b, com);

          g_submode = submode::TEXT_P;

        } else if(c->serial.action == turnAction::SPIRITMOVE) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }

          if(spiritTable[c->serial.actionIndex].targeting == 0) {
            while(c->serial.target >= (int)g_enemyCombatants.size()) {
              c->serial.target-= 1;
              M("Stuck in loop which confused me earlier");
            }
          }

          combatant* com = g_partyCombatants[combatUIManager->executePIndex];
          int whichSpiritAbility = com->serial.actionIndex; //which spirit ability
          int target = com->serial.target;
          useSpiritMove(whichSpiritAbility, target, com);
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::DEFEND) {
          if(g_enemyCombatants.size() == 0) {
            g_submode = submode::FINAL;
            break;
          }
          string text = getLanguageData("CombatProtagShrinks");
          text = stringMultiInject(text, {g_partyCombatants[combatUIManager->executePIndex]->name});
          combatUIManager->finalText = text;

          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dodgingThisTurn[combatUIManager->executePIndex] = 1;
          g_submode = submode::TEXT_P;
        } else if(c->serial.action == turnAction::FLEE) {
          int levelDifference = 0;
          int highestTeamateLevel = 0;
          for(auto x : g_partyCombatants) {
            if(x->level > highestTeamateLevel) {
              highestTeamateLevel = x->level;
            }
          }
          int highestEnemyLevel = 0;
          for(auto x : g_enemyCombatants) {
            if(x->level > highestEnemyLevel) {
              highestEnemyLevel = x->level;
            }
          }
          levelDifference = highestTeamateLevel - highestEnemyLevel;
          int random = rng(0,10);
          if(random + levelDifference >= 7) {
            //successful fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " tries to run away...";
            combatUIManager->queuedStrings.push_back(make_pair("And did!", 0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNSUCCESSTEXT;

          } else {
            //failed fleeing
            combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " tries to run away...";
            combatUIManager->queuedStrings.push_back(make_pair("But couldn't!", 0));
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::RUNFAILTEXT;

          }
        }

        break;
      }
    case submode::TEXT_P: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {
          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }
          curTextWait = 0;
        }

        if( combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
            int someoneDied = combatUIManager->queuedStrings.at(0).second;
            if(someoneDied != 0) {
              int index = 0;
              while(index < g_deadCombatants.size()) {
                if(g_deadCombatants.at(index)->disappearing == 0) {
                  g_deadCombatants.at(index)->disappearing = 1;
                  break;
                }
                index++;
              }
            }
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {

            //make sure to handle death from selfdamage properly
            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {

              combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              string message = "All party members are knocked-out!";
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT_P;
              break;
            }
            

            if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
              //reset all stats
              for(auto x : g_partyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
              }
              g_submode = submode::STATUS_P;
            } else {
              combatUIManager->executePIndex++;
              g_submode = submode::EXECUTE_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }




        break;
      }
    case submode::EXECUTE_E:
      {
        //it's possible that the protags died during or right after their turn
        //it can crash in this switch case if you aren't careful handling
        //status damage or self damage

        if(combatUIManager->executeEIndex >= g_enemyCombatants.size()) {
          //fight over
          g_submode = submode::FINAL;
          break;
        }
        combatant* c = g_enemyCombatants[combatUIManager->executeEIndex];

        //temp
        c->serial.action = turnAction::ATTACK;
        //this should connect to specialcombatants.cpp
        //by an identity field

        if(c->serial.action == turnAction::ATTACK) {
          //        combatant* e = g_partyCombatants[rng(0, g_partyCombatants.size() - 1)];
          //        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
          //        damage *= frng(0.70,1.30);
          //        e->health -= damage;
          //        string message = c->name + " deals " + to_string(damage) + " to " + e->name + "!";
          //        g_submode = submode::TEXT_E;
          //        combatUIManager->finalText = message;
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->dialogProceedIndicator->y = 0.25;
          //        combatUIManager->mainPanel->show = 0;
          //        combatUIManager->dialogProceedIndicator->show = 0;
          //        combatUIManager->mainText->show = 0;

          vector<combatant*> validCombatants = {};
          for(int i = 0; i < g_partyCombatants.size(); i++) {
            if(g_partyCombatants[i]->health > 0) {
              validCombatants.push_back(g_partyCombatants[i]);
            }
          }

          if(validCombatants.size() <=0) { abort();}

          int dodgingIndex = rng(0, validCombatants.size() - 1);

          //check for taunt
          bool breakflag = 0;
          for(auto &x : c->statuses) {
            if(x.type == status::TAUNTED) {
              for(int i = 0; i < validCombatants.size(); i++) {
                if(validCombatants[i]->filename == x.datastr) {
                  dodgingIndex = i;
                  breakflag = 1;
                  break;
                }
              }
            }
            if(breakflag) break;
          }

          combatant* e = validCombatants[dodgingIndex];
          int adjustedDIndex = 0;
          for(auto x : g_partyCombatants) {
            if(x == e) {
              break;
            }
            adjustedDIndex++;
          }
          combatUIManager->partyDodgingCombatant = e;
          int damage = c->curAttack - e->curDefense;
          damage *= frng(0.70,1.30);
          if(damage < 0) {damage = 0;}
          combatUIManager->damageFromEachHit = damage;
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelSmallX;
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelSmallY;
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelSmallWidth;
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelSmallHeight;
          combatUIManager->dodgePanel->show = 1;
          combatUIManager->incrementDodgeTimer = 0;
          combatUIManager->dodgeTimer = 0;
          combatUIManager->damageTakenFromDodgingPhase = 0;
          combatUIManager->invincibleMs = 0;

          //g_submode = submode::DODGING;
          //string message = c->name + " attacks " + e->name + " for " + to_string(damage) + " damage.";
          string message = getLanguageData("CombatEnemyAttack");
          message = stringMultiInject(message, {c->name, e->name, to_string(damage)});

          if(combatUIManager->dodgingThisTurn[adjustedDIndex] == 1) {
            combatUIManager->shrink = 1;
          } else {
            combatUIManager->shrink = 0;
          }

          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_E;

        }

        break;
      }
    case submode::TEXT_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          } 
          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText && input[11] && !oldinput[11]) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {
            g_submode = submode::DODGING;
            combatUIManager->accuA = 1000000;
            combatUIManager->accuB = 1000000;
            combatUIManager->accuC = 1000000;
            combatUIManager->dodgerX = 512;
            combatUIManager->dodgerY = 512;
            combatant* e = g_enemyCombatants[combatUIManager->executeEIndex];
            if(e->attackPatterns.size() <0) {
              E("Add attack patterns for " + e->name);
              abort();
            }
            combatUIManager->curPatterns = e->attackPatterns[rng(0, e->attackPatterns.size()-1)];

//            M("Spawning bullets for");
//
//            for(auto x : combatUIManager->curPatterns) {
//              cout << x << " ";
//            }
//            cout << endl;

            for(int i = 0; i < g_miniEnts.size(); i++) {
              delete g_miniEnts[i];
              i--;
            }
            g_miniBullets.clear();
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::STATUS_E:
      {
        int breakout = 0;
        while(curStatusIndex >= (int)g_enemyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == (int)g_enemyCombatants.size()) {
            curCombatantIndex = 0;
            for(int i = 0; i < 4; i ++) {
              combatUIManager->dodgingThisTurn[i] = 0;
            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < (int)g_partyCombatants.size()) {
              curCombatantIndex ++; //used for choosing which protag picks action in submode::MAIN
            }
            combatUIManager->currentOption = 0;
            g_submode = submode::MAIN;
            breakout = 1;
            break;

          }
        }
        if(breakout) {break;}

        combatant* c = g_enemyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }

        break;
      }
    case submode::TEXT_STATUS_E:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              //reset all stats
              for(auto x : g_enemyCombatants) {
                x->curStrength = x->baseStrength;
                x->curMind = x->baseMind;
                x->curAttack = x->baseAttack;
                x->curDefense = x->baseDefense;
                x->curSoul = x->baseSoul;
                x->curSkill = x->baseSkill;
                x->curCritical = x->baseCritical;
                x->curRecovery = x->baseSoul;
              }

              g_submode = submode::STATUS_E;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FINAL:
      {
        //calculate XP based on total stats of defeated enemies
        //award xp grant xp award exp grant exp give xp give exp

        //return held items to inventory (combatant.itemToUse)
        for(auto x : g_partyCombatants) {
          if(x->itemToUse != -1) {
            if(g_combatInventory.size() < g_maxInventorySize) {
              g_combatInventory.push_back(x->itemToUse);;
              x->itemToUse = -1;
            }

          }
        }

        //apply recovery
        for(auto &x : g_partyCombatants) {
          if(x->health > 0) {
            x->health += x->curRecovery * x->baseStrength;
            x->sp += x->curRecovery * x->baseMind;
            if(x->health > x->baseStrength) {
              x->health = x->baseStrength;
            }
            if(x->sp > x->baseMind) {
              x->sp = x->baseMind;
            }
          }
        }

        combatUIManager->calculateXP();
        D(combatUIManager->xpToGrant);
        //combatUIManager->xpToGrant = 1000;
        curCombatantIndex = 0;

        g_submode = submode::CHARAXP;

        //      combatUIManager->currentText = "";
        //      combatUIManager->finalText = "Fomm has won the battle!";
        //      g_submode = submode::FINALTEXT;
        break;
      }
    case submode::CHARAXP:
      {
        if(curCombatantIndex >= g_partyCombatants.size()) {
          //        combatUIManager->currentText = "";
          //        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          //        combatUIManager->finalText = "Fomm has won the battle!";
          //        g_submode = submode::FINALTEXT;
          //        break;
          g_submode = submode::OUTWIPE;
          break;
        }
        combatant* x = g_partyCombatants[curCombatantIndex];
        x->level = xpToLevel(x->xp);
        combatUIManager->oldLevel = x->level;
        x->xp += combatUIManager->xpToGrant * frng(0.95, 1.05);
        combatUIManager->newLevel= xpToLevel(x->xp);
        combatUIManager->thisLevel = combatUIManager->oldLevel+1;
        //g_submode = submode::LEVELUP;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " gains " + to_string(combatUIManager->xpToGrant) + " xp.";
        g_submode = submode::XPTEXT;


        break;
      }
    case submode::XPTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::LEVELUP;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELUP:
      {
        if(combatUIManager->thisLevel > combatUIManager->newLevel) {
          curCombatantIndex++;
          g_submode = submode::CHARAXP;
          break;
        }

        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        string line = "";
        line = stringMultiInject(getLanguageData("CombatLevelUp0"), {g_partyCombatants[curCombatantIndex]->name, to_string(combatUIManager->thisLevel)});
        combatUIManager->finalText = line;


        g_partyCombatants[curCombatantIndex]->baseStrength += g_partyCombatants[curCombatantIndex]->strengthGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp1"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseStrength)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseMind += g_partyCombatants[curCombatantIndex]->mindGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp2"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseMind)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseAttack += g_partyCombatants[curCombatantIndex]->attackGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp3"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseAttack)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseDefense += g_partyCombatants[curCombatantIndex]->defenseGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp4"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseDefense)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseSoul += g_partyCombatants[curCombatantIndex]->soulGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp5"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSoul)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseCritical += g_partyCombatants[curCombatantIndex]->criticalGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp6"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseCritical)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));

        g_partyCombatants[curCombatantIndex]->baseSkill += g_partyCombatants[curCombatantIndex]->skillGain * frng(0.8, 1.2);
        line = stringMultiInject(getLanguageData("CombatLevelUp7"), {to_stringF((int)g_partyCombatants[curCombatantIndex]->baseSkill)});
        combatUIManager->queuedStrings.push_back(make_pair(line,0));


        g_partyCombatants[curCombatantIndex]->level = combatUIManager->thisLevel;

        g_submode = submode::LEVELTEXT;



        break;
      }
    case submode::FINALTEXT:
      {
        for(int i = 0; i < g_enemyCombatants.size(); i++) {
          delete g_enemyCombatants[i];
        }
        g_enemyCombatants.clear();
        for(int i = 0; i < g_deadCombatants.size(); i++) {
          delete g_deadCombatants[i];
        }
        g_deadCombatants.clear();
        curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::OUTWIPE;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEVELTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //check and see if they can learn a spiritmove
              bool canLearnMove = 0;
              for(auto x : g_partyCombatants[curCombatantIndex]->spiritTree) {
                int a = x.first;
                int b = x.second;
                if(a == combatUIManager->thisLevel) {
                  canLearnMove = 1;
                  combatUIManager->moveToLearn = x.second;
                  break;
                }
              }

              if(canLearnMove) {
                if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() < 4) {
                  //just learn it 
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " learned " + spiritTable[combatUIManager->moveToLearn].name + ".";
                  g_partyCombatants[curCombatantIndex]->spiritMoves.push_back(combatUIManager->moveToLearn);
                  g_submode = submode::LEARNEDTEXT;
                  break;
                } else {
                  combatUIManager->currentText = "";
                  combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
                  combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " can learn " + spiritTable[combatUIManager->moveToLearn].name + ", but would need to forget another move.";
                  combatUIManager->queuedStrings.push_back(make_pair("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.",0));
                  g_submode = submode::LEARNTEXT;
                  break;
                }
              } else {
                //no move to learn, go to the next levelup
                combatUIManager->thisLevel++;
                g_submode = submode::LEVELUP;
                break;
              }
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNEDTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              //no move to learn, go to the next levelup
              combatUIManager->thisLevel++;
              g_submode = submode::LEVELUP;

              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::LEARNTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGET;
              combatUIManager->forgetPanel->show = 1;
              combatUIManager->forgetText->show = 1;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGET:
      {
        if(input[0] && !oldinput[0]) {
          if(combatUIManager->forgetOption > 0) {
            combatUIManager->forgetOption -= 1;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->forgetOption += 1;
          }
        }
        if(input[11] && !oldinput[11]) {
          if(combatUIManager->forgetOption < 4) {
            combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will forget " + spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption]].name + " and learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            break;
          } else {
            combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " will not learn " + spiritTable[combatUIManager->moveToLearn].name + ".";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGETTEXT;
            break;

          }
          break;
        }

        break;
      }
    case submode::FORGETTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[11]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if((input[11] && !oldinput[11]) || 1) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::FORGETCONFIRM;
              combatUIManager->confirmOption = 0;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::FORGETCONFIRM:
      {
        combatUIManager->dialogProceedIndicator->show = 0;

        if(input[2] && !oldinput[2]) {
          combatUIManager->confirmOption = 0;
        }
        if(input[3] && !oldinput[3]) {
          combatUIManager->confirmOption = 1;
        }

        if(input[11] && !oldinput[11]) {
          if(combatUIManager->confirmOption == 0) {
            g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->forgetOption] = combatUIManager->moveToLearn;
            combatUIManager->thisLevel++;
            g_submode = submode::LEVELUP;
          } else {
            combatUIManager->mainText->updateText("Choose a move for " + g_partyCombatants[curCombatantIndex]->name + " to do without.", -1, 0.85, g_textcolor, g_font);
            g_submode = submode::FORGET;
          }
        }

        break;
      }
    case submode::ITEMCHOOSE: 
      {

        if(g_partyCombatants[curCombatantIndex]->itemToUse != -1) {
          if(g_combatInventory.size() < g_maxInventorySize) {
            g_combatInventory.push_back(g_partyCombatants[curCombatantIndex]->itemToUse);
          }
          g_partyCombatants[curCombatantIndex]->itemToUse = -1;
        }
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->inventoryPanel->show = 1;
        combatUIManager->inventoryText->show = 1;

        drawOptionsPanel();

        combatUIManager->inventoryPanel->render(renderer, g_camera, elapsed);

        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption != 0 &&
              combatUIManager->currentInventoryOption != 7) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption != 6 &&
              combatUIManager->currentInventoryOption != 13) {
            if(combatUIManager->currentInventoryOption + 1 < g_combatInventory.size()) {
              combatUIManager->currentInventoryOption ++;
            }
          }
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentInventoryOption >= 7) {
            combatUIManager->currentInventoryOption -= 7;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentInventoryOption <= 6) {
            if(combatUIManager->currentInventoryOption + 7 < g_combatInventory.size()) {
              combatUIManager->currentInventoryOption += 7;
            }
          }
        }


        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->inventoryPanel->show = 0;
          combatUIManager->inventoryText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_combatInventory.size() > 0) {
          g_partyCombatants[curCombatantIndex]->itemToUse = g_combatInventory[combatUIManager->currentInventoryOption];
          switch(itemsTable[g_combatInventory[combatUIManager->currentInventoryOption]].targeting) {
            case 0:
              //enemy
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_combatInventory[combatUIManager->currentInventoryOption];

              g_submode = submode::TARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 1:
              //teamate
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_combatInventory[combatUIManager->currentInventoryOption];
              g_submode = submode::ALLYTARGETING;
              combatUIManager->currentTarget = 0;
              break;
            case 2:
              //none
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_combatInventory[combatUIManager->currentInventoryOption];
              g_submode = submode::CONTINUE;
              break;
          }
          g_combatInventory.erase(g_combatInventory.begin() + combatUIManager->currentInventoryOption);

        }

        renderInventoryPanel();

        break;
      }
    case submode::ALLYTARGETING: 
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 1;
        combatUIManager->targetText->show = 1;

        combatUIManager->tcm_accumulator += 0.1;
        if(combatUIManager->tcm_accumulator > M_PI * 2) {
          combatUIManager->tcm_accumulator -= M_PI * 2;
        }

        if(input[2] && !oldinput[2]) {
          if(combatUIManager->currentTarget > 0) {
            combatUIManager->currentTarget --;
          }
        }

        if(input[3] && !oldinput[3]) {
          if(combatUIManager->currentTarget < g_partyCombatants.size() - 1) {
            combatUIManager->currentTarget ++;
          }
        }

        if(combatUIManager->currentTarget < 0) { combatUIManager->currentTarget = 0; }
        if(combatUIManager->currentTarget >= g_partyCombatants.size()) { combatUIManager->currentTarget = g_partyCombatants.size() - 1; }


        if(input[11] && !oldinput[11]) {
          g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
          g_submode = submode::CONTINUE;
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        }


        drawOptionsPanel();

        combatUIManager->targetText->updateText("To " + g_partyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

        combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
        combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



        break;
      }
    case submode::SPIRITCHOOSE:
      {
        combatUIManager->mainPanel->show = 0;
        combatUIManager->dialogProceedIndicator->show = 0;
        combatUIManager->mainText->show = 0;

        combatUIManager->optionsPanel->show = 1;
        combatUIManager->menuPicker->show = 1;
        combatUIManager->optionsText->show = 1;

        combatUIManager->targetPanel->show = 0;
        combatUIManager->targetText->show = 0;

        combatUIManager->spiritPanel->show = 1;
        combatUIManager->spiritText->show = 1;

        drawOptionsPanel();

        combatUIManager->spiritPanel->render(renderer, g_camera, elapsed);

        if(input[0] && !oldinput[0]) {
          if(combatUIManager->currentInventoryOption > 0) {
            combatUIManager->currentInventoryOption --;
          }
        }

        if(input[1] && !oldinput[1]) {
          if(combatUIManager->currentInventoryOption + 1 < g_partyCombatants[curCombatantIndex]->spiritMoves.size()) {
            combatUIManager->currentInventoryOption ++;
          }
        }

        if(input[8] && !oldinput[8]) {
          g_submode = submode::MAIN;
          //combatUIManager->currentOption = 0;
          combatUIManager->spiritPanel->show = 0;
          combatUIManager->spiritText->show = 0;
        }

        if(input[11] && !oldinput[11] && g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
          //does he have enough sp?
          int cost = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].cost;
          int currentSp = g_partyCombatants[curCombatantIndex]->sp;
          string name = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].name;

          if(currentSp < cost) {
            combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " doesn't have enough SP for " + name + ".";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            combatUIManager->dialogProceedIndicator->y = 0.25;
            g_submode = submode::SPWARNING;

          } else {
            switch(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption]].targeting) {
              case 0:
                {
                  //enemy
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::TARGETING;
                  break;
                }
              case 1:
                {
                  //teamate
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  int spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  int spiritNumber = spiritMove;
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;

                  g_submode = submode::ALLYTARGETING;
                  break;
                }
              case 2:
                {
                  //none
                  g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
                  g_partyCombatants[curCombatantIndex]->serial.target = -1;
                  int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
                  g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
                  g_submode = submode::CONTINUE;
                  break;
                }
            }
          }

        }

        renderSpiritPanel();

        break;
      }
    case submode::SPWARNING: 
      {
        //curCombatantIndex = 0;
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::SPIRITCHOOSE;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::DODGING:
      {
        combatUIManager->dialogProceedIndicator->show = 0;
        float rate = 2;
        if(combatUIManager->dodgePanel->x > combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x -= 0.01 * rate;
        }
        if(combatUIManager->dodgePanel->x <= combatUIManager->dodgePanelFullX) {
          combatUIManager->dodgePanel->x = combatUIManager->dodgePanelFullX;
          combatUIManager->incrementDodgeTimer = 1;
        }

        if(combatUIManager->dodgePanel->y > combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y -= 0.01 * rate * (16.0f / 10.0f);;
        }
        if(combatUIManager->dodgePanel->y < combatUIManager->dodgePanelFullY) {
          combatUIManager->dodgePanel->y = combatUIManager->dodgePanelFullY;
        }

        if(combatUIManager->dodgePanel->width < combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width += 0.02 * rate;
        }
        if(combatUIManager->dodgePanel->width > combatUIManager->dodgePanelFullWidth) {
          combatUIManager->dodgePanel->width = combatUIManager->dodgePanelFullWidth;
        }

        if(combatUIManager->dodgePanel->height < combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height += 0.02 * rate * (16.0f / 10.0f);
        }
        if(combatUIManager->dodgePanel->height > combatUIManager->dodgePanelFullHeight) {
          combatUIManager->dodgePanel->height = combatUIManager->dodgePanelFullHeight;
        }

        if(combatUIManager->incrementDodgeTimer) {
          combatUIManager->dodgeTimer += elapsed;
          bool movingUp = input[0];
          bool movingDown = input[1];
          bool movingLeft = input[2];
          bool movingRight = input[3];

          // Determine if diagonal movement is happening
          bool diagonalMovement = (movingUp || movingDown) && (movingLeft || movingRight);

          // Normalize speed for diagonal movement
          float speed = diagonalMovement ? combatUIManager->dodgerSpeed / sqrt(2) : combatUIManager->dodgerSpeed;

          if (movingUp) {
            combatUIManager->dodgerY -= speed;
          }
          if (movingDown) {
            combatUIManager->dodgerY += speed;
          }
          if (movingLeft) {
            combatUIManager->dodgerX -= speed;
          }
          if (movingRight) {
            combatUIManager->dodgerX += speed;
          }

          float margin = combatUIManager->dodgerWidth/2;
          if(combatUIManager->dodgerX < 0 + margin) {
            combatUIManager->dodgerX = 0 + margin;
          }
          if(combatUIManager->dodgerY < 0 + margin) {
            combatUIManager->dodgerY = margin;
          }
          if(combatUIManager->dodgerX > 1024 - margin) {
            combatUIManager->dodgerX = 1024 - margin;
          }
          if(combatUIManager->dodgerY > 1024 - margin) {
            combatUIManager->dodgerY = 1024 - margin;
          }
        }
        if(combatUIManager->partyDodgingCombatant->health <= 0 || (devMode && input[8])) {
          //end early
          combatUIManager->dodgeTimer = combatUIManager->maxDodgeTimer + 1;
        }

        if(combatUIManager->dodgeTimer > combatUIManager->maxDodgeTimer) {
          // delete all miniEnts
          int size = g_miniEnts.size();
          for(int i = 0; i < size; i++) {
            delete g_miniEnts[0];
          }

          if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }


            if(deadPartyMembers == g_partyCombatants.size()) {
              string message = "All party members are knocked-out!";
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }

            combatUIManager->mainPanel->show = 0;
            combatUIManager->mainText->show = 0;
            combatUIManager->dialogProceedIndicator->show = 0;
            curCombatantIndex = 0;
            curStatusIndex = 0;

            //reset all stats
            for(auto x : g_enemyCombatants) {
              x->curStrength = x->baseStrength;
              x->curMind = x->baseMind;
              x->curAttack = x->baseAttack;
              x->curDefense = x->baseDefense;
              x->curSoul = x->baseSoul;
              x->curSkill = x->baseSkill;
              x->curCritical = x->baseCritical;
              x->curRecovery = x->baseSoul;
            }

            g_submode = submode::STATUS_E;

          } else {
            //if the character died, report on it with MEMBERDEADTEXT. If all characters are dead, report on it with ALLDEADTEXT

            int deadPartyMembers = 0;
            for(auto x : g_partyCombatants) {
              if(x->health <= 0) {
                deadPartyMembers++;
              }
            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {

              combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
              combatUIManager->currentText = "";
              combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
            }

            if(deadPartyMembers == g_partyCombatants.size()) {
              string message = "All party members are knocked-out!";
              combatUIManager->queuedStrings.clear();
              combatUIManager->queuedStrings.push_back(make_pair(message,0));
              g_submode = submode::ALLDEADTEXT;
              break;

            }

            if(combatUIManager->partyDodgingCombatant->health <= 0) {
              g_submode = submode::MEMBERDEADTEXT;
              break;
            }


            combatUIManager->executeEIndex++;
            g_submode = submode::EXECUTE_E;
          }

        }
        break;
      }
    case submode::STATUS_P:
      {
        int breakout = 0;
        while(curStatusIndex >= g_partyCombatants[curCombatantIndex]->statuses.size()) {
          curStatusIndex = 0;
          curCombatantIndex++;
          if(curCombatantIndex == g_partyCombatants.size()) {
            curCombatantIndex = 0;
            for(int i = 0; i < 4; i ++) {
              combatUIManager->dodgingThisTurn[i] = 0;
            }
            while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
              curCombatantIndex ++;
            }
            g_submode = submode::EXECUTE_E;
            combatUIManager->executeEIndex = 0;
            breakout = 1;
            break;
          }
        }
        if(breakout) {break;}

        combatant* c = g_partyCombatants[curCombatantIndex];
        if(applyStatus(c, &c->statuses[curStatusIndex])) {
          c->statuses.erase(c->statuses.begin() + curStatusIndex);
          curStatusIndex--;
        }
        break;
      }
    case submode::TEXT_STATUS_P:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              curStatusIndex++;

              g_submode = submode::STATUS_P;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNWARNING: 
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }


        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNSUCCESSTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              combatUIManager->mainPanel->show = 0;
              combatUIManager->mainText->show = 0;
              combatUIManager->dialogProceedIndicator->show = 0;
              combatUIManager->optionsPanel->show = 1;
              g_submode = submode::OUTWIPE;
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::RUNFAILTEXT:
      {
        combatUIManager->mainPanel->show = 1;
        combatUIManager->mainText->show = 1;
        combatUIManager->optionsPanel->show = 0;

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                g_submode = submode::EXECUTE_E;
                combatUIManager->executeEIndex = 0;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
                combatUIManager->mainPanel->show = 0;
                combatUIManager->mainText->show = 0;
                combatUIManager->dialogProceedIndicator->show = 0;
                curCombatantIndex = 0;
                curStatusIndex = 0;
                //reset all stats
                for(auto x : g_enemyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                }
                g_submode = submode::STATUS_E;
                //g_submode = submode::MAIN;
                break;
              } else {
                combatUIManager->executeEIndex++;
                g_submode = submode::EXECUTE_E;
                break;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::MEMBERDEADTEXT_P:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
                //reset all stats
                for(auto x : g_partyCombatants) {
                  x->curStrength = x->baseStrength;
                  x->curMind = x->baseMind;
                  x->curAttack = x->baseAttack;
                  x->curDefense = x->baseDefense;
                  x->curSoul = x->baseSoul;
                  x->curSkill = x->baseSkill;
                  x->curCritical = x->baseCritical;
                  x->curRecovery = x->baseSoul;
                }
                g_submode = submode::STATUS_P;
              } else {
                combatUIManager->executePIndex++;
                g_submode = submode::EXECUTE_P;
              }
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
    case submode::ALLDEADTEXT:
      {

        if(input[8]) {
          text_speed_up = 50;
        } else {
          text_speed_up = 1;
        }

        curTextWait += elapsed * text_speed_up;

        if(combatUIManager->finalText == combatUIManager->currentText) {
          combatUIManager->dialogProceedIndicator->show = 1;
        } else {
          combatUIManager->dialogProceedIndicator->show = 0;
        }

        if (curTextWait >= textWait)
        {

          if(combatUIManager->finalText != combatUIManager->currentText) {
            if(input[8]) {
              combatUIManager->currentText = combatUIManager->finalText;
            } else {
              combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
              playSound(6, g_ui_voice, 0);
            }
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);

          }

          curTextWait = 0;
        }

        if(combatUIManager->finalText == combatUIManager->currentText) {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {


              //g_gamemode = gamemode::LOSS;
              g_lossSub = lossSub::INWIPE;
              transitionDelta = transitionImageHeight;
              g_submode = submode::OUTWIPEL;//dont run the code to draw the minients after the switch statement
              break;
            }
          }
        }

        //animate dialogproceedarrow
        {
          combatUIManager->c_dpiDesendMs += elapsed;
          if(combatUIManager->c_dpiDesendMs > combatUIManager->dpiDesendMs) {
            combatUIManager->c_dpiDesendMs = 0;
            combatUIManager->c_dpiAsending = !combatUIManager->c_dpiAsending;

          }

          if(combatUIManager->c_dpiAsending) {
            combatUIManager->dialogProceedIndicator->y += combatUIManager->dpiAsendSpeed;
          } else {
            combatUIManager->dialogProceedIndicator->y -= combatUIManager->dpiAsendSpeed;

          }
        }

        break;
      }
  }


  combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
  combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  combatUIManager->dialogProceedIndicator->render(renderer, g_camera, elapsed);


  if(g_submode == submode::DODGING) {
    combatUIManager->dodgePanel->render(renderer, g_camera, elapsed);
    SDL_SetRenderTarget(renderer, combatUIManager->rendertarget);
    SDL_SetRenderDrawColor(renderer, 6, 7, 6, 255);
    SDL_RenderClear(renderer);

    combatUIManager->accuA += elapsed;
    combatUIManager->accuB += elapsed;
    combatUIManager->accuC += elapsed;

    if(combatUIManager->curPatterns.size() > 0) {
      spawnBullets(combatUIManager->curPatterns[0], combatUIManager->accuA);
    }

    if(combatUIManager->curPatterns.size() > 1) {
      spawnBullets(combatUIManager->curPatterns[1], combatUIManager->accuB);
    }

    if(combatUIManager->curPatterns.size() > 2) {
      spawnBullets(combatUIManager->curPatterns[2], combatUIManager->accuC);
    }

    {
      for(auto x : g_miniEnts) {
        x->update(elapsed);
      }
      for(int i = 0; i < g_miniBullets.size(); i++) {
        g_miniBullets[i]->bulletUpdate(elapsed);
      }
      for(int i = 0; i < g_miniEnts.size(); i++) {
        if(g_miniEnts[i]->x < -SPAWN_MARGIN || g_miniEnts[i]->x > SCREEN_WIDTH + SPAWN_MARGIN ||
            g_miniEnts[i]->y < -SPAWN_MARGIN || g_miniEnts[i]->y > SCREEN_HEIGHT + SPAWN_MARGIN) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
        if(g_miniEnts[i]->exploded) {
          delete g_miniEnts[i];
          i--;
          continue;
        }
      }
      for(auto x : g_miniEnts) {
        x->render();
      }
      if(combatUIManager->shrink) {
        combatUIManager->dodgerWidth = 50;
        combatUIManager->dodgerHeight = 50;
      } else {
        combatUIManager->dodgerWidth = 100;
        combatUIManager->dodgerHeight = 100;
      }
      if(combatUIManager->invincibleMs <= 0) {
        for(auto x : g_miniBullets) {
          if(Distance(combatUIManager->dodgerX, combatUIManager->dodgerY, x->x, x->y) < (combatUIManager->dodgerWidth + x->w)/2) {
            combatUIManager->partyDodgingCombatant->health -= combatUIManager->damageFromEachHit;
            int dmgToReport = combatUIManager->damageFromEachHit;
            if(dmgToReport > combatUIManager->partyDodgingCombatant->health) {
              dmgToReport = combatUIManager->partyDodgingCombatant->health;
            }
            combatUIManager->partyDodgingCombatant->dmgTakenOverFight += dmgToReport;
            if(combatUIManager->partyDodgingCombatant->health < 0) {combatUIManager->partyDodgingCombatant->health = 0;}

            combatUIManager->damageTakenFromDodgingPhase += combatUIManager->damageFromEachHit;
            combatUIManager->invincibleMs = combatUIManager->maxInvincibleMs;
            break;
          }
        }
      }
      SDL_Rect drect;
      drect.x = combatUIManager->dodgerX - combatUIManager->dodgerWidth/2;
      drect.y = combatUIManager->dodgerY - combatUIManager->dodgerHeight/2;
      drect.w = combatUIManager->dodgerWidth;
      drect.h = combatUIManager->dodgerHeight;

      if(combatUIManager->invincibleMs > 0) {
        if(combatUIManager->blinkMs > 50) {
          combatUIManager->drawDodger = !combatUIManager->drawDodger;
          combatUIManager->blinkMs = 0;
        }
      } else {
        combatUIManager->drawDodger = 1;
      }

      if(combatUIManager->drawDodger) {
        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255*0.7, 255*0.7, 255*0.7);
        SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
        SDL_SetTextureColorMod(combatUIManager->dodgerTexture, 255, 255, 255);
        drect.x += 10;
        drect.y += 10;
        drect.w -= 20;
        drect.h -= 20;
        SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
      }


      combatUIManager->invincibleMs -= elapsed;
      combatUIManager->blinkMs += elapsed;
    }







    SDL_SetRenderTarget(renderer, NULL);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Rect dstrect;
    float padding = 0.04;
    dstrect.x = (combatUIManager->dodgePanel->x + padding/2) * WIN_WIDTH;
    dstrect.y = (combatUIManager->dodgePanel->y + (padding*combatUIManager->aspect)/2) * WIN_HEIGHT;
    dstrect.w = (combatUIManager->dodgePanel->width - padding) * WIN_WIDTH;
    dstrect.h = (combatUIManager->dodgePanel->height - (padding*combatUIManager->aspect))* WIN_HEIGHT;
    SDL_RenderCopy(renderer, combatUIManager->rendertarget, NULL, &dstrect);
  }

  if(g_submode== submode::FORGET) {
    combatUIManager->dialogProceedIndicator->show = 0;
    combatUIManager->forgetPanel->render(renderer, g_camera, elapsed);
    float y = 0.275;
    float yDelta = 0.075;
    float x = 0.4;

    float px = 0.37;
    float pxoffset = -0.009; //slightly negative
    float pyoffset = 0.006;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[0]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->forgetPicker->show = 1;

    if(combatUIManager->forgetOption == 0) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[1]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    if(combatUIManager->forgetOption == 1) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[2]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 2) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[3]].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 3) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }

    y+= yDelta;

    combatUIManager->forgetText->updateText(spiritTable[combatUIManager->moveToLearn].name, -1, 0.85, g_textcolor, g_font);
    combatUIManager->forgetText->boxX = x;
    combatUIManager->forgetText->boxY = y;
    combatUIManager->forgetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    if(combatUIManager->forgetOption == 4) {
      combatUIManager->forgetPicker->x = px + pxoffset;
      combatUIManager->forgetPicker->y = combatUIManager->forgetText->boxY + pyoffset;
      combatUIManager->forgetPicker->render(renderer, g_camera, elapsed);
    }



  }

  if(g_submode == submode::FORGETCONFIRM) {
    combatUIManager->yes->show = 1;
    combatUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->no->show = 1;
    combatUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    combatUIManager->confirmPicker->show = 1;
    combatUIManager->confirmPicker->render(renderer, g_camera, elapsed);

    if(combatUIManager->confirmOption == 0) {
      combatUIManager->confirmPicker->x = combatUIManager->yes->boxX - 0.08;
      combatUIManager->confirmPicker->y = combatUIManager->yes->boxY + 0.01;
    } else {
      combatUIManager->confirmPicker->x = combatUIManager->no->boxX - 0.07;
      combatUIManager->confirmPicker->y = combatUIManager->no->boxY + 0.01;

    }

  }



  SDL_RenderPresent(renderer);
}

miniEnt::miniEnt() {
  g_miniEnts.push_back(this);
}

miniEnt::~miniEnt() {
  g_miniEnts.erase(remove(g_miniEnts.begin(), g_miniEnts.end(), this), g_miniEnts.end());
}

void miniEnt::update(float elapsed) {
  // Calculate new position based on velocity
  float deltaX = velocity * cos(angle) * elapsed;
  float deltaY = velocity * sin(angle) * elapsed;
  x += deltaX;
  y += deltaY;

  gravityVX += gravityAccelX * elapsed;
  x += gravityVX;
  gravityVY += gravityAccelY * elapsed;
  y += gravityVY;

  // If spinSpeed is non-zero, apply the spinning motion
  if (spinSpeed != 0) {
    spinAngle += spinSpeed * elapsed;

    // Move the bullet along the perpendicular vector
    x = centerX + radius * cos(spinAngle);
    y = centerY + radius * sin(spinAngle);

    radius -= velocity * elapsed;
  }

  if(!isInPlayArea && x -w/2>= 0 && x + w/2 <= SCREEN_WIDTH && y -h/2 >= 0 && y +h/2 <= SCREEN_WIDTH) {
    isInPlayArea = 1;
  }
  if (isInPlayArea && canBounce) {
    if (x < w / 2 || x > SCREEN_WIDTH - w / 2) {
      angle = M_PI - angle; // Reflect horizontally
      if (x < w / 2) { 
        x = w / 2;
        gravityVX = -gravityVX;

      }
      if (x > SCREEN_WIDTH - w / 2) {
        x = SCREEN_WIDTH - w / 2;
        gravityVX = -gravityVX;
      }
    }
    if (y < w / 2 || y > SCREEN_HEIGHT - w / 2) {
      angle = -angle; // Reflect vertically
      if (y < w / 2)  {
        y = w / 2;
        gravityVY = -gravityVY;
      }
      if (y > SCREEN_HEIGHT - w / 2) {
        y = SCREEN_HEIGHT - w / 2;
        gravityVY = -gravityVY;
      }
    }
  }
}

void miniEnt::render() {
  SDL_Rect drect = {
    (int)x - w/2,
    (int)y - h/2,
    (int)w,
    (int)h
  };
  SDL_SetTextureColorMod(texture, red*0.7, blue*0.7, green*0.7);
  SDL_RenderCopy(renderer, texture, NULL, &drect);
  drect.x += 10;
  drect.y += 10;
  drect.w -= 20;
  drect.h -= 20;
  SDL_SetTextureColorMod(texture, red, blue, green);
  SDL_RenderCopy(renderer, texture, NULL, &drect);
}

miniBullet::miniBullet(float f_angle, float f_velocity) {
  angle = f_angle;
  velocity = f_velocity;
  x = 512.0f + SPAWN_MARGIN * cos(angle);
  y = 512.0f + SPAWN_MARGIN * sin(angle);
  g_miniBullets.push_back(this);
}

miniBullet::miniBullet() {
  float spawnX, spawnY;
  float targetX, targetY;
  targetX = rng(0, SCREEN_WIDTH);
  targetY = rng(0, SCREEN_HEIGHT);
  int side = rand() % 4;
  switch(side) {
    case 0:
      spawnX = -SPAWN_MARGIN;
      spawnY = rng(0, SCREEN_HEIGHT);
      break;
    case 1:
      spawnX = SCREEN_WIDTH + SPAWN_MARGIN;
      spawnY = rng(0, SCREEN_HEIGHT);
      break;
    case 2:
      spawnX = rng(0, SCREEN_WIDTH);
      spawnY = -SPAWN_MARGIN;
      break;
    case 3:
      spawnX = rng(0, SCREEN_WIDTH);
      spawnY = SCREEN_HEIGHT + SPAWN_MARGIN;
      break;
  }
  x = spawnX;
  y = spawnY;
  w = 100;
  h = 100;
  angle = atan2(targetY - spawnY, targetX - spawnX);
  velocity = 0.3;
  g_miniBullets.push_back(this);
}

miniBullet::~miniBullet() {
  g_miniBullets.erase(remove(g_miniBullets.begin(), g_miniBullets.end(), this), g_miniBullets.end());
}

void miniBullet::explode(int numFragments, int exploding, float fragSize) {
  // Create smaller bullets upon explosion

  float baseAngle = 0;
  if(randomExplodeAngle) {
    baseAngle = frng(0, M_PI * 2);
  }
  for (int i = 0; i < numFragments; i++) {
    miniBullet* fragment = new miniBullet();
    fragment->x = x;
    fragment->y = y;
    fragment->angle = 2 * M_PI / numFragments * i;
    fragment->angle += baseAngle;
    if(completelyRandomExplodeAngle) {
      fragment->angle = frng(0, 2*M_PI);
      fragment->completelyRandomExplodeAngle = 1;
    }
    fragment->canBounce = canBounce;
    fragment->velocity = 0.3; // Set velocity of fragments
    fragment->texture = texture;
    fragment->red = this->red;
    fragment->green = this->green;
    fragment->blue = this->blue;
    fragment->texture = this->texture;
    fragment->exploding = exploding;
    fragment->explosionTimer = 1000;
    fragment->w = this->w * fragSize;
    fragment->h = fragment->w;
    fragment->numFragments = numFragments;
    fragment->fragSize = fragSize;
    fragment->randomExplodeAngle = randomExplodeAngle;
    if(exploding == 0) {
      fragment->canBounce = 0;
    }
  }
}

void miniBullet::bulletUpdate(float elapsed) {
  if(homing) {
    float dx = combatUIManager->dodgerX - x;
    float dy = combatUIManager->dodgerY - y;
    float targetAngle = atan2(dy, dx);
    float angleDifference = targetAngle - angle;
    if(angleDifference > M_PI) angleDifference -= 2 * M_PI;
    if(angleDifference < -M_PI) angleDifference += 2 * M_PI;
    angle += angleDifference * 0.001f * elapsed;
  }
  if (exploding) {
    explosionTimer -= elapsed;
    if (explosionTimer <= 0 && exploded == 0) {
      explode(numFragments, exploding - 1, fragSize);
      exploded = 1;
    }
  }
}
