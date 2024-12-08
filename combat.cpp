#include "combat.h"
#include "objects.h"
#include "main.h"
#include "utils.h"
#include <unordered_map>

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
                    if (key == "texture") texture = std::stoi(value);
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
            int index = (int)red;
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

combatant::combatant(string ffilename, int fxp) {
  string loadstr;
  loadstr = "resources/static/combatfiles/" + ffilename + ".cmb";
  istringstream file(loadTextAsString(loadstr));

  string temp;
  file >> temp;
  file >> temp;


  name = temp;
  filename = ffilename;

  file >> temp;
  file >> temp;


  string spritefilevar;
  spritefilevar = "resources/static/combatsprites/" + temp + ".qoi";
  const char* spritefile = spritefilevar.c_str();
  texture = loadTexture(renderer, spritefile);

  file >> temp;
  file >> temp;
  myType = stringToType(temp);

  file >> temp;
  file >> baseAttack;

  file >> temp;
  file >> attackGain;

  file >> temp;
  file >> baseDefense;

  file >> temp;
  file >> defenseGain;

  file >> temp;
  file >> baseHealth;

  file >> temp;
  file >> healthGain;

  file >> temp;
  file >> baseCritical;

  file >> temp;
  file >> criticalGain;

  file >> temp;
  file >> baseSkill;

  file >> temp;
  file >> skillGain;

  file >> temp;
  file >> baseSoul;

  file >> temp;
  file >> soulGain;

  file >> temp;
  file >> baseMind;

  file >> temp;
  file >> mindGain;

  file >> temp;
  file >> deathText;
  for (char &ch : deathText) {
    if(ch == '_') {
      ch = ' ';
    }
  }

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

  maxHealth = baseHealth + (healthGain * level);
  maxSp = baseMind + (mindGain * level);
  

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

type stringToType(const std::string& str) {
  static std::unordered_map<std::string, type> typeMap = {
    {"none", NONE},
    {"animal", ANIMAL},
    {"plant", PLANT},
    {"bug", BUG},
    {"robot", ROBOT},
    {"alien", ALIEN},
    {"undead", UNDEAD},
    {"ghost", GHOST},
    {"demon", DEMON}
  };

  auto it = typeMap.find(str);
  if (it != typeMap.end()) {
    return it->second;
  } else {
    return NONE; // Default value if string not found
  }
}

std::unordered_map<int, itemInfo> itemsTable;

std::unordered_map<int, spiritInfo> spiritTable;

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
    itemsTable[0] = itemInfo("Bandage", 1);
    itemsTable[1] = itemInfo("Bomb", 2);
    itemsTable[2] = itemInfo("Glasses", 2);
  }
  
  {
    spiritTable[0] = spiritInfo("Poison", 0, 1); //good damage over time
    spiritTable[1] = spiritInfo("Slime", 0, 1); //does more damage with continued use
    spiritTable[2] = spiritInfo("Bite", 0, 1);
    spiritTable[3] = spiritInfo("Mindblast", 0, 3);
    spiritTable[4] = spiritInfo("Protect", 1, 5);
    spiritTable[5] = spiritInfo("Buffen", 2, 1); //increase attack power
    spiritTable[6] = spiritInfo("Chant", 2, 4); //garantee a crit
    spiritTable[7] = spiritInfo("Curse", 0, 5); //sacrifice health to deal damage
    spiritTable[8] = spiritInfo("Leech", 0, 5); //gain some health by dealing damage
    spiritTable[9] = spiritInfo("Exploit", 0, 2); //bonus damage to status'ed foes
    spiritTable[10] = spiritInfo("Finale", 0, 10); //gain attack for three turns, then die
    spiritTable[11] = spiritInfo("Explode", 2, 10); //self-detonate and deal massive area damage
    spiritTable[12] = spiritInfo("Earthquake", 2, 5); //area damage
    spiritTable[13] = spiritInfo("Frost", 0, 5); //damage and reduce attack
    spiritTable[14] = spiritInfo("Magma", 0, 5); //Inflict burn
    spiritTable[15] = spiritInfo("Slice", 0, 1); //High damage to animals
    spiritTable[16] = spiritInfo("Plague", 0, 1); //High damage to plant
    spiritTable[17] = spiritInfo("Swat", 0, 1); //High damage to bug
    spiritTable[18] = spiritInfo("Comet", 0, 1); //high damage to flying
    spiritTable[19] = spiritInfo("Crush", 0, 1); //high damage to swimming
    spiritTable[20] = spiritInfo("Shock", 0, 1); //high damage to robot
    spiritTable[21] = spiritInfo("Fling", 0, 1); //high damage to alien
    spiritTable[22] = spiritInfo("Antimagic", 0, 1); //High damage to undead
    spiritTable[23] = spiritInfo("Flash", 0, 1); //High damage to ghost
    spiritTable[24] = spiritInfo("Distract", 0, 4); // Chance for enemy to not attack

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
  
  return level;
}

void useItem(int item, int target, combatant* user) {
  switch(item) {
    case 0:
    {
      //Bandage
      int mag = 50.0f * frng(0.70, 1.30) * (float)((float)user->baseSkill + ((float)user->skillGain * user->level));
      g_partyCombatants[target]->health += mag;
      string message = user->name + " healed " + g_partyCombatants[target]->name + " for " + to_string(mag) + ".";
      combatUIManager->finalText = message;
      combatUIManager->currentText = "";
      combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
      if(g_partyCombatants[target]->health >= g_partyCombatants[target]->maxHealth) {
        g_partyCombatants[target]->health = g_partyCombatants[target]->maxHealth;
      }
      combatUIManager->dialogProceedIndicator->y = 0.25;
      break;
    }
    case 1:
    {
      //Bomb
      int mag = 50.0f * frng(0.70, 1.30) * (float)((float)user->baseSkill + ((float)user->skillGain * user->level));
      for(int i = 0; i < g_enemyCombatants.size(); i++) {
        g_enemyCombatants[i]->health -= mag;
        string message = g_enemyCombatants[i]->name + " took " + to_string(mag) + " from the bomb.";
        combatUIManager->queuedStrings.push_back(make_pair(message,0));
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[i];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
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

void useSpiritMove(int spiritNumber, int target, combatant* user) {
  user->sp -= spiritTable[spiritNumber].cost;
  switch(spiritNumber) {
    case 19:
      {
        float baseDmg = 10;
        if(g_enemyCombatants[target]->myType == type::DEMON) {
          baseDmg *= 2;
        }

        int mag = baseDmg * frng(0.70, 1.30) * (float)((float)user->baseSoul + ((float)user->soulGain * user->level));
        if(mag < 0) {mag = 0;}
        g_enemyCombatants[target]->health -= mag;
        string message = user->name + " hurt " + g_enemyCombatants[target]->name + " for " + to_string(mag) + ".";
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

  }
}

void combatUI::calculateXP() {
  xpToGrant = 0;
  for(auto x : g_deadCombatants) {
    int dmg = x->baseAttack + x->attackGain*x->level;
    int def = x->baseDefense + x->defenseGain*x->level;
    int het = x->baseHealth + x->healthGain*x->level;
    int sum = dmg + def + het;
    xpToGrant += sum;
  }
}

combatUI::combatUI(SDL_Renderer* renderer) {
  initTables();

  partyHealthBox = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0, 0.65, 1, 0.35, 0);
  partyHealthBox->patchwidth = 213;
  partyHealthBox->patchscale = 0.4;
  partyHealthBox->is9patch = true;
  partyHealthBox->persistent = true;
  partyHealthBox->show = 0;

  partyText = new textbox(renderer, "Hey", 1600 * g_fontsize, 0, 0, 0.9);
  partyText->boxWidth = 0;
  partyText->width = 0.95;
  partyText->boxHeight = 0;
  partyText->boxX = 0.2;
  partyText->boxY = 1-0.1;
  partyText->align = 0;
  partyText->dropshadow = 1;
  partyText->show = 1;

  partyMiniText = new textbox(renderer, "Hey", 800 * g_fontsize, 0, 0, 0.9);
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

  dialogProceedIndicator = new ui(renderer, "resources/static/ui/dialog_proceed.qoi", 0.92, 0.88, 0.05, 1, 0);
  dialogProceedIndicator->heightFromWidthFactor = 1;
  dialogProceedIndicator->persistent = true;
  dialogProceedIndicator->priority = 8;
  dialogProceedIndicator->dropshadow = 1;
  dialogProceedIndicator->y =  0.25;

  mainText = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
  mainText->boxWidth = 0.9;
  mainText->width = 0.9;
  mainText->boxHeight = 0.25;
  mainText->boxX = 0.05;
  mainText->boxY = 0.05;
  mainText->worldspace = 1; //right align
  mainText->dropshadow = 1;

  optionsPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.05, 0.05, 0.6, 0.25, 0);
  optionsPanel->patchwidth = 213;
  optionsPanel->patchscale = 0.4;
  optionsPanel->is9patch = true;
  optionsPanel->persistent = true;

  optionsText = new textbox(renderer, "", 1600 * g_fontsize, 0, 0, 0.9);
  optionsText->boxWidth = 0.9;
  optionsText->width = 0.9;
  optionsText->boxHeight = 0.25;
  optionsText->boxX = 0.1;
  optionsText->boxY = 0.05;
  optionsText->dropshadow = 1;

  optionsMiniText = new textbox(renderer, "", 800 * g_fontsize, 0, 0, 0.9);
  optionsMiniText->boxWidth = 0;
  optionsMiniText->width = 0.95;
  optionsMiniText->boxHeight = 0;
  optionsMiniText->boxX = 0.127;
  optionsMiniText->boxY = 0.07;
  optionsMiniText->dropshadow = 1;
  optionsMiniText->show = 1;

  menuPicker = new ui(renderer, "resources/static/ui/menu_picker.qoi", 0.92, 0.88, 0.04, 1, 0);
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

  targetText = new textbox(renderer, "", 1600 * g_fontsize, 0, 0, 0.9);
  targetText->boxWidth = 0.3;
  targetText->boxHeight = 0.12;
  targetText->boxX = 0.55;
  targetText->boxY = 0.22;
  targetText->align = 2;
  targetText->dropshadow = 1;

  inventoryPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.05, 0.5, 0.65, 0);
  inventoryPanel->patchwidth = 213;
  inventoryPanel->patchscale = 0.4;
  inventoryPanel->is9patch = true;
  inventoryPanel->persistent = true;

  inventoryText = new textbox(renderer, "", 1600 * g_fontsize, 0, 0, 0.9);
  inventoryText->boxWidth = 0.3;
  inventoryText->boxHeight = 0.12;
  inventoryText->boxX = 0.45;
  inventoryText->boxY = 0.22;
  inventoryText->dropshadow = 1;

  spiritPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.4, 0.05, 0.3, 0.42, 0);
  spiritPanel->patchwidth = 213;
  spiritPanel->patchscale = 0.4;
  spiritPanel->is9patch = true;
  spiritPanel->persistent = true;

  spiritText = new textbox(renderer, "", 1600 * g_fontsize, 0, 0, 0.9);
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

  forgetText = new textbox(renderer, "", 1600 * g_fontsize, 0, 0, 0.9);
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

  yes = new textbox(renderer, "Yes", 1700 * g_fontsize, 0, 0, 0.9);
  yes->boxX = 0.50 - 0.07;
  yes->boxY = 0.2;
  yes->boxWidth = 0.01;
  yes->boxHeight = 0.35;
  yes->dropshadow = 1;
  yes->align = 2;

  no = new textbox(renderer, "No", 1700 * g_fontsize, 0, 0, 0.9);
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
  const float width = 0.18;
  const float height = 0.08;
  const float initialX = 0.125;
  const float initialY = 0.1;
  int index = 0;
  for(int i = 0; i < columns; i++) {
    for(int j = 0; j < rows; j++) {
      combatUIManager->optionsText->boxX = initialX + (i * width);
      combatUIManager->optionsText->boxY = initialY + (j * height);
      combatUIManager->optionsText->updateText(combatUIManager->options[index], -1, 0.85, g_textcolor, g_font);
      if(index == combatUIManager->currentOption) {
        combatUIManager->menuPicker->x = initialX + (i * width) - 0.035;
        combatUIManager->menuPicker->y = initialY + (j * height) + 0.005;
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

void drawCombatants() {
  int count = g_enemyCombatants.size();
  int gap = 0.05 * WIN_WIDTH; // Space between combatants
  
  for (int i = 0; i < count; ++i) {
      combatant* combatant = g_enemyCombatants[i];
  
      if(combatant->renderQuad.x == -1) {
        // Convert percentage-based width and height to actual pixel values
        int actual_width = static_cast<int>(WIN_WIDTH * combatant->width);
        int actual_height = static_cast<int>(WIN_WIDTH * combatant->height);
    
        // Calculate X position to arrange horizontally
        int total_width = (count * actual_width) + ((count - 1) * gap);
        int x = (WIN_WIDTH - total_width) / 2 + i * (actual_width + gap);
        int y = ((WIN_HEIGHT - actual_height) / 2); // Centering vertically
        SDL_Rect renderQuad = { x, y, actual_width, actual_height };
        combatant->renderQuad = renderQuad;
      }

      if(g_submode == submode::TARGETING && i == combatUIManager->currentTarget) {
        SDL_SetTextureColorMod(combatant->texture, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod);
      } else {
        SDL_SetTextureColorMod(combatant->texture, 255, 255, 255);
      }

      SDL_RenderCopy(renderer, combatant->texture, nullptr, &combatant->renderQuad);
  }

  for(auto x : g_deadCombatants) {
    SDL_SetTextureAlphaMod(x->texture, x->opacity);
    if(x->disappearing && x->opacity-1 > 0) {
      x->opacity-=5;
    }
    SDL_RenderCopy(renderer, x->texture, nullptr, &x->renderQuad);
  }

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
          g_submode == submode::RUNWARNING) {
        if(i == curCombatantIndex){
          bonusY = -0.05;
        }

      }

      if(g_submode == submode::ALLYTARGETING) {
        if(i == combatUIManager->currentTarget) {
          combatUIManager->partyText->textcolor = { 108, 80, 80};
        } else {
          combatUIManager->partyText->textcolor = { 155, 115, 115};
        }
      } else {
        combatUIManager->partyText->textcolor = { 155, 115, 115};
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
      combatUIManager->partyMiniText->boxX = x + 0.02 + 0.15;
      combatUIManager->partyMiniText->boxY = 0.7 + 0.02 + bonusY + 0.028;
      combatUIManager->partyMiniText->boxWidth = actual_width;
      combatUIManager->partyMiniText->boxHeight = actual_height;

      combatUIManager->partyMiniText->boxY += 0.07;
      combatUIManager->partyMiniText->updateText('/' + to_string(combatant->maxHealth), -1, 34, combatUIManager->partyMiniText->textcolor);
      combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      combatUIManager->partyMiniText->boxY += 0.07;
      combatUIManager->partyMiniText->updateText('/' + to_string(combatant->maxSp), -1, 34, combatUIManager->partyMiniText->textcolor);
      combatUIManager->partyMiniText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

  
  }
  combatUIManager->partyHealthBox->show = 0;


}

void CombatLoop() {

  getCombatInput();

  SDL_RenderClear(renderer);
   
  updateWindowResolution();

  drawBackground();

  drawCombatants();
  switch (g_submode) {
    case submode::INWIPE:
    {
      g_forceEndDialogue = 0;
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

        drawBackground();
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
          drawBackground();
        
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

        drawBackground();
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
          drawBackground();
        
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

      combatUIManager->targetText->updateText("To " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

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
      combatant* c = g_partyCombatants[combatUIManager->executePIndex];
      while(g_partyCombatants[combatUIManager->executePIndex]->health <= 0 && combatUIManager->executePIndex+1 < g_partyCombatants.size()) {
        combatUIManager->executePIndex++;
      }
      if(combatUIManager->executePIndex == g_partyCombatants.size() - 1 && g_partyCombatants[combatUIManager->executePIndex]->health <= 0) {
        g_submode = submode::EXECUTE_E;
        combatUIManager->executeEIndex = 0;
        break;
      }
      if(combatUIManager->executePIndex >= g_partyCombatants.size()) {
        g_submode = submode::EXECUTE_E;
        combatUIManager->executeEIndex = 0;
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
        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
        damage *= frng(0.70,1.30);
        if(damage < 0) {damage = 0;}
        e->health -= damage;
        string message = c->name + " deals " + to_string(damage) + " to " + e->name + "!";
  
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
        while(c->serial.target >= g_enemyCombatants.size()) {
          c->serial.target-= 1;
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
        combatUIManager->finalText = g_partyCombatants[combatUIManager->executePIndex]->name + " shrank down!";
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
          if(combatUIManager->executePIndex + 1 ==  g_partyCombatants.size()) {
            g_submode = submode::EXECUTE_E;
            combatUIManager->executeEIndex = 0;
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
      M("EXECUTE_E");
      D(combatUIManager->executeEIndex);
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
        combatant* e = validCombatants[dodgingIndex];
        int adjustedDIndex = 0;
        for(auto x : g_partyCombatants) {
          if(x == e) {
            break;
          }
          adjustedDIndex++;
        }
        combatUIManager->partyDodgingCombatant = e;
        D(combatUIManager->partyDodgingCombatant->name);
        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
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
        string message = c->name + " attacks " + e->name + " for " + to_string(damage) + " damage!";
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
          combatUIManager->curPatterns = e->attackPatterns[rng(0, e->attackPatterns.size()-1)];
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
    case submode::FINAL:
    {
      //calculate XP based on total stats of defeated enemies
      //award xp grant xp award exp grant exp give xp give exp

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
      x->xp += combatUIManager->xpToGrant;
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
      combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " has reached level " + to_string(combatUIManager->thisLevel) + ".";
      combatUIManager->queuedStrings.push_back(make_pair("New Attack is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseAttack + g_partyCombatants[curCombatantIndex]->attackGain* combatUIManager->thisLevel) + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Defense is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseDefense + g_partyCombatants[curCombatantIndex]->defenseGain* combatUIManager->thisLevel) + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Health is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseHealth + g_partyCombatants[curCombatantIndex]->healthGain * combatUIManager->thisLevel)  + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Critical is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseCritical + g_partyCombatants[curCombatantIndex]->criticalGain * combatUIManager->thisLevel) + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Skill is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseSkill + g_partyCombatants[curCombatantIndex]->skillGain * combatUIManager->thisLevel) + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Soul is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseSoul + g_partyCombatants[curCombatantIndex]->soulGain * combatUIManager->thisLevel) + ".",0));
      combatUIManager->queuedStrings.push_back(make_pair("New Mind is " + to_stringF(g_partyCombatants[curCombatantIndex]->baseMind + g_partyCombatants[curCombatantIndex]->mindGain * combatUIManager->thisLevel) + ".",0));
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
            break;
          case 1:
            //teamate
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = g_combatInventory[combatUIManager->currentInventoryOption];
            g_submode = submode::ALLYTARGETING;
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

      const int rows = 7;
      const int columns = 2;
      const float width = 0.22;
      const float height = 0.08;
      const float initialX = 0.45;
      const float initialY = 0.1;
      int index = 0;
      if(g_combatInventory.size() > 0) {
        for(int i = 0; i < columns; i++) {
          for(int j = 0; j < rows; j++) {
            combatUIManager->inventoryText->boxX = initialX + (i * width);
            combatUIManager->inventoryText->boxY = initialY + (j * height);
            int itemIndex = g_combatInventory[index];
            string itemName = "";
            itemName = itemsTable[itemIndex].name;
            combatUIManager->inventoryText->updateText(itemName, -1, 0.85, g_textcolor, g_font);
            if(index == combatUIManager->currentInventoryOption) {
              combatUIManager->menuPicker->x = initialX + (i * width) - 0.035;
              combatUIManager->menuPicker->y = initialY + (j * height) + 0.005;
            }
            if(index < g_combatInventory.size()) {
              combatUIManager->inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
            }
       
       
            index++;
          }
        }
        combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
      }
     

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
        if(currentSp < cost) {
          combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " doesn't have enough SP!";
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
              int spiritNumber = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
              g_submode = submode::CONTINUE;
              break;
            }
          }
        }

      }

      const int rows = 4;
      const int columns = 1;
      const float width = 0.22;
      const float height = 0.08;
      const float initialX = 0.45;
      const float initialY = 0.1;
      int index = 0;
      if(g_partyCombatants[curCombatantIndex]->spiritMoves.size() > 0) {
        for(int i = 0; i < columns; i++) {
          for(int j = 0; j < rows; j++) {
            combatUIManager->spiritText->boxX = initialX + (i * width);
            combatUIManager->spiritText->boxY = initialY + (j * height);
            string spiritName = "";
            spiritName = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index]].name;
  
            combatUIManager->spiritText->updateText(spiritName, -1, 0.85, g_textcolor, g_font);
            if(index == combatUIManager->currentInventoryOption) {
              combatUIManager->menuPicker->x = initialX + (i * width) - 0.035;
              combatUIManager->menuPicker->y = initialY + (j * height) + 0.005;
            }
            //if(index < g_partyCombatants[curCombatantIndex]->spiritMoves.size()) {
            combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
            //}
       
       
            index++;
          }
        }
        combatUIManager->menuPicker->render(renderer, g_camera, elapsed);
      }
     

      break;
    }
    case submode::SPWARNING: 
    {
      curCombatantIndex = 0;
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
      if(combatUIManager->partyDodgingCombatant->health <= 0) {
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
          M("A");

          int deadPartyMembers = 0;
          for(auto x : g_partyCombatants) {
            if(x->health <= 0) {
              deadPartyMembers++;
            }
          }

          if(combatUIManager->partyDodgingCombatant->health <= 0) {
            M("B");

            combatUIManager->finalText = combatUIManager->partyDodgingCombatant->name + " passed out!";
            combatUIManager->currentText = "";
            combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          }


          if(deadPartyMembers == g_partyCombatants.size()) {
            M("C");
            string message = "All party members are knocked-out!";
            combatUIManager->queuedStrings.clear();
            combatUIManager->queuedStrings.push_back(make_pair(message,0));
            g_submode = submode::ALLDEADTEXT;
            break;

          }

          if(combatUIManager->partyDodgingCombatant->health <= 0) {
            M("D");
            g_submode = submode::MEMBERDEADTEXT;
            break;
          }

          combatUIManager->mainPanel->show = 0;
          combatUIManager->mainText->show = 0;
          combatUIManager->dialogProceedIndicator->show = 0;
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
          for(int i = 0; i < 4; i ++) {
            combatUIManager->dodgingThisTurn[combatUIManager->executePIndex] = 0;
          }
          while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
            curCombatantIndex ++;
          }

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
    case submode::RUNWARNING: 
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
              g_submode = submode::MAIN;
              combatUIManager->currentOption = 0;
              for(int i = 0; i < 4; i ++) {
                combatUIManager->dodgingThisTurn[combatUIManager->executePIndex] = 0;
              }
              while(g_partyCombatants[curCombatantIndex]->health <= 0 && curCombatantIndex+1 < g_partyCombatants.size()) {
                curCombatantIndex ++;
              }
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
