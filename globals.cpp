#include <iostream>
#include <sstream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <cmath>	 //pow
#include <math.h>	 //sin()
#include <fstream> //loading
#include <vector>
#include <cctype> //make input lowercase for map console
#include <ctime>	//debug clock
#include <string>
#include <map> //saves
#include <ctime> //clock display
#include "stdio.h"
#include "physfs.h"
#include "unistd.h"
#include "stdio.h"
#include "title.h"
#include "globals.h"

#include "objects.h"

#undef M_PI
#define M_PI 3.14159265358979323846

using namespace std;


bool cmpCoord::operator()(const pair<int, int> a, const pair<int, int> b) const
{
  return a.first + a.second < b.first + b.second;
}

map<pair<int, int>, navNode*, cmpCoord> navNodeMap;

vector<cshadow *> g_shadows;

vector<entity *> g_entities;

vector<entity *> g_boardableEntities;

vector<entity*> g_ai;

vector<entity *> g_pellets; //for making pellets bounce and squash

vector<entity *> g_solid_entities;

vector<entity *> g_large_entities;

vector<tile *> g_tiles;

vector<door *> g_doors; //generally speaking, a load-level trigger

vector<entity *> g_poweredDoors;

vector<entity *> g_poweredLevers;

vector<dungeonDoor*> g_dungeonDoors;

vector<vector<box *>> g_boxs;

vector<impliedSlope *> g_impliedSlopes; //slopes which are implied to be behind walls, preventing entities from "hiding" behind them, as in zelda
                                        
vector<impliedSlopeTri *> g_impliedSlopeTris; //same as g_impliedSlopes, but for triangular slopes

vector<textbox *> g_textboxes;

vector<ui *> g_ui;

vector<actor *> g_actors;

vector<mapObject *> g_mapObjects;

vector<mapCollision *> g_mapCollisions;

vector<vector<tri *>> g_triangles;

vector<vector<ramp *>> g_ramps;

vector<heightmap *> g_heightmaps;

vector<navNode *> g_navNodes;

vector<vector<pointOfInterest *>> g_setsOfInterest;

vector<effectIndex *> g_effectIndexes;

vector<worldsound *> g_worldsounds;

vector<musicNode *> g_musicNodes;

vector<cueSound *> g_cueSounds;

vector<waypoint *> g_waypoints;

vector<trigger *> g_triggers;

vector<hitbox *> g_hitboxes;

vector<listener *> g_listeners;

vector<projectile *> g_projectiles;

vector<attack *> g_attacks;

vector<weapon *> g_weapons;

vector<particle *> g_particles;

vector<emitter *> g_emitters;

vector<collisionZone *> g_collisionZones;

vector<entity *> g_musicalEntities;

vector<levelNode *> g_levelNodes;

vector<ribbon *> g_ribbons;

vector<combatant *> g_enemyCombatants;

vector<combatant*> g_deadCombatants; //for fade-out

vector<combatant *> g_partyCombatants;

vector<miniEnt *> g_miniEnts;

vector<miniBullet *> g_miniBullets;

vector<tallGrass*> g_tallGrasses;

vector<camBlocker*> g_camBlockers;

vector<gradient*> g_gradients;

SDL_Texture* g_gradient_a = 0;
SDL_Texture* g_gradient_b = 0;
SDL_Texture* g_gradient_c = 0;
SDL_Texture* g_gradient_d = 0;
SDL_Texture* g_gradient_e = 0;
SDL_Texture* g_gradient_f = 0;
SDL_Texture* g_gradient_g = 0;
SDL_Texture* g_gradient_h = 0;
SDL_Texture* g_gradient_i = 0;
SDL_Texture* g_gradient_j = 0;

map<string, int> enemiesMap; // stores (file,cost) for enemies to be spawned procedurally in the map
int g_budget = 0;						 // how many points this map can spend on enemies;


bool boxsenabled = 1; // affects both map editor and full game. Dont edit here
bool g_collisionResolverOn = 1; //also referred to as "jiggling" ents out of solid collisions
bool g_showCRMessages = 0; //collisionResolver messages

bool onionmode = 0; // hide custom graphics


//I worked to make implied slopes and triangular slopes to work realistically, in 
//that they would push the player away like walls but if the player jumped they could
//jump on top of the slope, and that would be best for 3d levels
//since I'm not making 3d levels and they're too demanding for users to play anyways
//I'm simplifying the code which considers collisions with IS and IST
//now they'll just be like regular geometry
//if you ever want to reenable this, it should be easier to toggle this value
const bool g_useSimpleImpliedGeometry = 1;


bool genericmode = 0;
bool freecamera = 0;
bool devMode = 1;
bool g_ship = 0; //!!!
string g_language = "english";
bool canSwitchOffDevMode = 0;
bool inputRefreshCanSwitchOffDevMode = 0;
bool showDevMessages = 1;
bool showErrorMessages = 1;
bool showImportantMessages = 1;

void breakpoint()
{
  //I("First breakpoint");
  return;
}

void breakpoint2()
{
  //I("Second breakpoint");
  return;
}

// for visuals
float p_ratio = 1.151;
float g_brightness_setting = 100;		 // from 0 to 100, 0 is what is normal to me
float g_brightness_map_factor = 0; // this is meant to be set in the mapeditor so some maps can be darker
SDL_Texture* g_shade = 0;  //drawn to darken the frame uniformally
int g_dungeonDarkness = 0;
bool g_vsync = true;
float g_background_darkness = 0; // 0 - show bg, 1 - show black
SDL_Texture *background = 0;
bool g_backgroundLoaded = 0;
bool g_useBackgrounds = 1; // a user setting, if the user wishes to see black screens instead of colorful backgrounds
int g_brightness = 100; // brightness of map
// x length times x_z_ratio is proper screen length in z
float XtoZ = 0.496; // 4/2.31, arctan (4/ 3.21) = 60 deg
float XtoY = 0.866;
float YtoX = 1/XtoY;
float g_ratio = 1.618;
bool transition = 0;
SDL_Texture* transitionTexture;
SDL_Surface* transitionSurface;
void* transitionPixelReference;
int transitionPitch;
int transitionDelta;
const int transitionImageWidth = 300;
const int transitionImageHeight = 300;
int g_walldarkness = 55;			// 65, 75. could be controlled by the map unless you get crafty with recycling textures across maps
bool g_unlit = 0;							// set to 1 if the user has the lowest graphical setting, to disable lighting in maps for performance. Don't eh, don't dev like this
int g_graphicsquality = 3;		// 0 is least, 4 is max
float g_extraShadowSize = 20; // how much bigger are shadows in comparison to their hitboxes.
int g_fogofwarEnabled = 1;
int g_fogofwarRays = 100;
bool g_showHealthbar = 0;
int g_entitySleepDistance = 1048576*5;
effectIndex *smokeEffect;
effectIndex *littleSmokeEffect;
effectIndex *blackSmokeEffect;
effectIndex *sparksEffect;

particle* g_lastParticleCreated = 0;

// for fow
SDL_Texture *result;
SDL_Texture *result_c;
SDL_Texture *canvas;
SDL_Texture *canvas_fc;
SDL_Texture *light;

SDL_Texture *lighta;
SDL_Texture *lightb;
SDL_Texture *lightc;
SDL_Texture *lightd;

SDL_Texture *lightaro;
SDL_Texture *lightbro;
SDL_Texture *lightcro;
SDL_Texture *lightdro;

SDL_Texture *lightari;
SDL_Texture *lightbri;
SDL_Texture *lightcri;
SDL_Texture *lightdri;

SDL_Texture *TextureA;
SDL_Texture *TextureB;
SDL_Texture *TextureC;
SDL_Texture *TextureD;
SDL_Texture *blackbarTexture;

//#define g_fogheight 19;
//#define g_fogwidth 21;
//constexpr size_t g_fogheight = 19;
//constexpr size_t g_fogwidth = 21;
int g_lastFunctionalX = 0; // for optimizing the FoW calcs
int g_lastFunctionalY = 0;
int g_fogMiddleX = 10;
int g_fogMiddleY = 9;
float g_viewdist = 310; // 240, 310 is casual, 340 could be from an upgrade.   it was 310 500 is max
//instead of calculating distance every frame, lookup which boxes should be open in a table

bool g_fogIgnoresLOS = 0;
int g_tile_fade_speed = 90; // 40, 50
int xtileshift = 0;
int ytileshift = 0;
bool g_force_cookies_update = 0;

std::vector<std::vector<int>> g_fogcookies(g_fogwidth, std::vector<int>(g_fogheight));
// this second vector is for storing the cookies that are ontopof walls
// that way, we can draw too layers of shadow before actors and one after
std::vector<std::vector<int>> g_savedcookies(g_fogwidth, std::vector<int>(g_fogheight));

// data for two passes of cookies
std::array<std::array<int, g_fogheight>, g_fogwidth> g_fc;
std::array<std::array<int, g_fogheight>, g_fogwidth> g_sc;
std::array<std::array<int, g_fogheight>, g_fogwidth> g_shc;

// fogslates - 19 x 2 = 38 actors.
std::vector<entity *> g_fogslates;
std::vector<entity *> g_fogslatesA;
std::vector<entity *> g_fogslatesB;

//for the stutter/punishment effect
//"don't do that"
//stutter the game to punish the player for misbehaving
float punishValue = 0;
float punishValueDegrade = -0.1; //make a punishment last longer
float basePunishValueDegrade = -0.1;

// for having items bounce
vector<float> g_itemsines;

//float g_elapsed_accumulator = 0;

// I've bounced around thinking these matter and turning them down
// or deciding that they don't matter and pumping them up
// Here's what I know atm: the first value should be left at 11 prettymuch always
// 2 is okay - g_TiltResolution actually doesn't affect loading
// but it will effect CPU usage, particularly when the triangles are onscreen
// 2 or 4 for large maps, seems okay 1 is more detail than I think anyone needs.
int g_platformResolution = 11; // a factor of 55. 11 is fine. USE 11
float g_TiltResolution = 4;		 // 1, 2, 4, 16 //what size step to use for triangular walls, 2 is almost unnoticable. must be a factor of 64 USE 4

// triangular walls are the most expensive things to draw, so here s an idea:
// when this variable is set to 1, hide triangular walls which are behind fogofwar.
//  !!! this feature is incomplete
bool g_trifog_optimize = 0;

// these are nonparticular values for drawing the blackbars
SDL_Rect FoWrect;
int px;

bool g_protagHasBeenDrawnThisFrame = 0;
bool g_protagIsBeingDetectedBySmell = 0; //used for when the protag is too close to an enemy and will soon agro that enemy
bool g_protagIsBeingDetectedBySight = 0;
bool g_protagIsInHearingRange = 0;

float summationXVel = 0;
float summationYVel = 0;
float effectiveSummationXVel = 0;
float effectiveSummationYVel = 0;
float savedPrecedePlayerX = 0;
float savedPrecedePlayerY = 0;
int maxNavCalcMs = 4000;
int navCalcMs = 0; //counts up to maxPrecedeCalcMs, and at that point, resets summationNVel. Old values are passed to oldSummationNVel

//so these ones are quite important, these are for
//precisely how far ahead the monster wishes to plan,
//how far the 

navNode* precedeProtagNode = nullptr; //set in main loop, used by some monsters.
                                      
bool g_loadingATM = 0; // are we loading a new map atm?
SDL_Texture *g_shadowTexture;
SDL_Texture *g_shadowTextureAlternate;
int g_flashtime = 300;		 // ms to flash red after taking damage
float g_whiteFlashtime = 300; 
float g_cameraShove = 150; // factor of the screen to move the camera when shooting.
float g_cameraAimingOffsetX = 0;
float g_cameraAimingOffsetY = 0;
float g_cameraAimingOffsetXTarget = 0;
float g_cameraAimingOffsetYTarget = 0;
float g_cameraAimingOffsetLerpScale = 0.91;

// text
string g_font;
float g_fontsize = 0.031; // 0.021 - 0.04
TTF_Font* g_ttf_fontLarge;
TTF_Font* g_ttf_fontMedium;
TTF_Font* g_ttf_fontSmall;
TTF_Font* g_ttf_fontTiny;
float g_minifontsize = 0.01;
float g_transitionSpeed = 1; // 3, 9

// inventory - we're switching things up. This will be the picnic-box, the inventory for consumables
float use_cooldown = 0; // misleading, its not for attacks at all
vector<attack *> AdventureattackSet;
int inPauseMenu = 0;
bool g_firstFrameOfPauseMenu = 0; //true if this is the first frame of the pause menu, for aligning the cursor
bool g_firstFrameOfSettingsMenu = 0; //true if this is the first frame of the pause menu, for aligning the cursor
float g_UiGlideSpeedX = 0.012;
float g_UiGlideSpeedY; //set from g_UiGlideSpeedX based on aspect ratio
int inventoryScroll = 0;		// how many rows in the inventory we've scrolled thru
int inventorySelection = 1; // which item in the inventory is selected
int itemsPerRow = ceil((0.9 - 0.05) / (0.07 + 0.01));
int g_inventoryColumns = ceil((0.74 - 0.05) / 0.07);
int g_itemsInInventory = 0;
int g_inventoryRows = 4;
SDL_Texture* g_locked_level_texture;
textbox *inventoryText; //showing numbers by items
ui *inventoryMarker; //indicates inventory selection
// for not counting extra presses in UI for shooting and moving axisen
int oldUIUp = 1;
int oldUIDown = 1;
int oldUILeft = 1;
int oldUIRight = 1;
int SoldUIUp = 1;
int SoldUIDown = 1;
int SoldUILeft = 1;
int SoldUIRight = 1;
int g_inputDelayFrames = 30;      //For the nice effect where the user can press a button or hold it
int g_inputDelayRepeatFrames = 6;

amState g_amState = amState::CLOSED;

//options / settings menu
bool g_inSettingsMenu = 0;
settingsUI* g_settingsUI;
SDL_Scancode g_swallowedKey; //for swallowing a key from a polled event
bool g_swallowAKey = 0;          //set this to one to swallow the next key pressed
bool g_awaitSwallowedKey = 0;
bool g_swallowedAKeyThisFrame = 0;
int g_pollForThisBinding = -1;
int g_whichRebindValue;
string g_affirmStr;
string g_negStr;
vector<string> g_graphicsStrings = {};

//escape menu
bool g_inEscapeMenu = 0;
escapeUI* g_escapeUI;

//blinking text
int g_blinkingMS = 0;
const int g_maxBlinkingMS = 1000;
bool g_blinkHidden = 0; //alternates every maxBlinkingMS


// physics
float g_gravity = 220;
float g_stepHeight = 33;
int g_hasBeenHoldingJump = 0; //for letting the player hold jump to go higher
int g_jumpGaranteedAccelMs = 0; 
int g_maxJumpGaranteedAccelMs = 150; //for x ms protag is garanteed to accelerate, was 150 
// These two variables contain the position of the hit of the last lineTrace()
int lineTraceX, lineTraceY;

// gameplay loop
int g_maxPelletsInLevel = 0; //number of pellets loaded into the level
int g_currentPelletsCollected = 0; //how many they have
int g_pelletsNeededToProgress = 1; //player has beaten the level, just needs to leave
vector <std::pair<int, string>> g_pelletGoalScripts; //each entry contains the path to a script which
bool g_showPellets = 1;

//for the boarding to entities
entity* g_boardedEntity = 0;
entity* g_formerBoardedEntity = 0; //when transfering, this points to the entity were are transfering from
bool g_transferingByBoardable = 0; //set to true when protag entered a transfering boardable ent, and set to false when the protag has reached the other entity
float g_transferingByBoardableTime = 0; //ms left before arriving at the destination boardable.
float g_maxTransferingByBoardableTime = 0; //set at the same time g_transferingByBoardableTime is, and used in comparison to set protag's position (even though they won't see anything, they will see the indicators and it will also affect audio)

int g_protagIsWithinBoardable = 0;
int g_boardingCooldownMs = 0;
const int g_maxBoardingCooldownMs = 2000;
//for animating the boardable
int g_msSinceBoarding = 0; //counts up from 0 after boarding

camera::camera(float fx, float fy)
{
  fx = x;
  fy = y;
}

void camera::update_movement(float elapsed, float targetx, float targety) {
    if (!isfinite(targetx) || !isfinite(targety)) {
        return;
    }

    desiredX = targetx;
    desiredY = targety;

    if (lag == 0) {
        x = targetx;
        y = targety;
    } else {
        x += (targetx - oldx) * (elapsed / 256) * lag;
        y += (targety - oldy) * (elapsed / 256) * lag;

        oldx = x;
        oldy = y;
        // if we're there, within a pixel, set the lagResetTimer to nothing
        if (abs(targetx - x) < 1.4 && abs(targety - y) < 1.4) {
            lag = 0;
        } else {
            // if not, consider increasing lag to catch up
            lag += lagaccel;
        }
    }

    if (devMode == 0) {
      g_camera.free = 1;

        // Check for collisions with camBlockers and adjust camera position
        for (const auto& blocker : g_camBlockers) {
            intersectsX = (x < blocker->bounds.x + blocker->bounds.width) && (x + width > blocker->bounds.x);
            intersectsY = (y < blocker->bounds.y + blocker->bounds.height) && (y + height > blocker->bounds.y);
            int cameraIsFree = 1;
            if(intersectsX && intersectsY) {
              cameraIsFree = 0;
            }
            if(!cameraIsFree) {
              g_camera.free = 0;
            }

            if (intersectsX && intersectsY) {
              if (blocker->direction == 0 || blocker->direction == 2) {
                  // Adjust X axis
                  if (blocker->direction == 2) {
                      x = blocker->bounds.x - width;
                  } else {
                      x = blocker->bounds.x + blocker->bounds.width;
                  }
              } else {
                  // Adjust Y axis
                  if (blocker->direction == 3) {
                      y = blocker->bounds.y - height;
                  } else {
                      y = blocker->bounds.y + blocker->bounds.height;
                  }
              }
            }
        }
        if(g_camera.free) {

          repoAccu += elapsed;
          if(repoAccu > 200) {
            repoMag++;
            repoAccu = 0;
          }
          int total = 0;
          if(repoX != -1) {total+= abs(repoX - x);}
          if(repoY != -1) {total+= abs(repoY - y);}
          if(total > 600) {
            repoX = -1;
            repoY = -1;
          }
          //lerp from repoX, repoY to x, y
          if(repoX != -1) {
            if(abs(repoX - x) > repoMag) {
              if(x > repoX) {
                repoX += repoMag;
              } else {
                repoX -= repoMag;
              }
              x = repoX;
            } else {
              repoX = -1;
            }
          }

          if(repoY != -1) {
            if(abs(repoY - y) > repoMag) {
              if(y > repoY) {
                repoY += repoMag;
              } else if(y < repoY) {
                repoY -= repoMag;
              }
              y = repoY;
            } else {
              repoY = -1;
            }
          }
          natX = x;
          natY = y;
          natMag = 5;
          natAccu = 0;
        } else {
          natAccu += elapsed;
          if(natAccu > 200) {
            natMag++;
            natAccu = 0;
          }
          int total = 0;
          if(natX != -1) {total+= abs(natX - x);}
          if(natY != -1) {total+= abs(natY - y);}
          if(total > 600) {
            natX = -1;
            natY = -1;
          }
          if(natX != -1) {
            if(abs(natX - x) > natMag) {
              if(x > natX) {
                natX += natMag;
              } else {
                natX -= natMag;
              }
              x = natX;
            } else {
              natX = -1;
            }
          }
    
          if(natY != -1) {
            if(abs(natY - y) > natMag) {
              if(y > natY) {
                natY += natMag;
              } else if(y < natY) {
                natY -= natMag;
              }
              y = natY;
            } else {
              natY = -1;
            }
          }
          repoX = x;
          repoY = y;
          repoMag = 5;
          repoAccu = 0;
        }


    }
}




void camera::resetCamera()
{
  enforceLimits = 0;
  lowerLimitX = 0;
  lowerLimitY = 0;
  upperLimitX = 0;
  upperLimitY = 0;
}


// zoom is really g_defaultZoom when screenwidth is STANDARD_SCREENWIDTH
int WIN_WIDTH = 640;
int WIN_HEIGHT = 400;
// theres some warping if STANDARD_SCREENWIDTH < WIN_WIDTH but that shouldn't ever happen
// if in the future kids have screens with 10 million pixels across feel free to mod the game
const int STANDARD_SCREENWIDTH = 1080;
//int WIN_WIDTH = 1280; int WIN_HEIGHT = 720;
// int WIN_WIDTH = 640; int WIN_HEIGHT = 360;
int old_WIN_WIDTH = 0; // used for detecting change in window width to recalculate scalex and y
int old_WIN_HEIGHT = 0; // used for detecting change in window width to recalculate scalex and y
int saved_WIN_WIDTH = WIN_WIDTH;
int saved_WIN_HEIGHT = WIN_HEIGHT;
SDL_Window *window;
SDL_DisplayMode DM;
bool g_fullscreen = false;
camera g_camera(0, 0);
entity *protag;
entity *mainProtag; // for letting other entities use this ones inventory; game ends when this one dies
entity * g_hog=0;
entity* g_behemoth0=0;
entity* g_behemoth1=0;
entity* g_behemoth2=0;
entity* g_behemoth3=0;
vector<entity*> g_behemoths;

// zoom is planned to be 1.0 for a resolution of 1920 pixels across
float g_defaultZoom = 0.85;
float g_zoom_mod = 1;		// for devmode
bool g_update_zoom = 0; // update the zoom this frame

float scalex = 1;
float scaley = 1;
float min_scale = 0.000000000001;
float max_scale = 2.4;

entity *g_focus;
entity *narrarator;
vector<entity *> party;
float g_max_framerate = 120;
float g_min_frametime = 1 / g_max_framerate * 1000;
SDL_Event event;
float ticks, lastticks, elapsed = 0, halfsecondtimer;
float camx = 0;
float camy = 0;
SDL_Renderer *renderer;

//for benchmarking the Entity Update function
// (EU)
float g_eu_a = 0;
float g_eu_b = 0;
float g_eu_c = 0;
float g_eu_d = 0;
float g_eu_e = 0;
float g_eu_f = 0;
float g_eu_g = 0;
float g_eu_h = 0;

float g_eu_ab=-1;
float g_eu_bb=-1;
float g_eu_cb=-1;
float g_eu_db=-1;
float g_eu_eb=-1;
float g_eu_fb=-1;
float g_eu_gb=-1;
float g_eu_hb=-1;

int g_eu_timer = 0;
int g_eu_exec = 0;

// g_map specifies the name of the map, g_mapdir specifies the folder with in maps the map is in.
// so its maps/{g_mapdir}/{g_map}.map
string g_map = "sp-title";
string g_mapdir = "sp-title";
string g_waypoint;
string g_mapOfLastSave = "sp-title";
string g_waypointOfLastSave = "a";

// input
const Uint8 *keystate = SDL_GetKeyboardState(NULL);
bool devinput[60] = {false};
bool g_ignoreInput = 1;

//these are meant to make it easy to port bindable inputs
bool input[16] = {false};
bool oldinput[16] = {false};

//these are for UNbindable inputs, which are for using the settings menu
//that way, the player can't be stuck with impossible binds
bool staticInput[5] = {false};
bool oldStaticInput[5] = {false};

SDL_Scancode bindings[16];
bool left_ui_refresh = false; // used to detect when arrows is pressed and then released
bool right_ui_refresh = false;
bool fullscreen_refresh = true;
bool quit = false;
string g_config = "player";
bool g_holdingCTRL = 0;
bool g_holdingTAB = 0;
// this is most noticable with a rifle, but sometimes when you try to shoot
// diagonally, you press one button (e.g. up) a frame or so early then the other (e.g. left)
// as a result, the game instantly shoots up and its unnacceptable.
// this is variable is the amount of frames to wait between getting input and shooting
const int g_diagonalHelpFrames = 4;
int g_cur_diagonalHelpFrames = 0;

// sounds and music
float g_volume = 0;
float g_music_volume = 0.8;
float g_sfx_volume = 1;
bool g_mute = 0;
entity *g_currentMusicPlayingEntity = 0;

vector<std::pair<Mix_Chunk*,string>> g_preloadedSounds;
Mix_Chunk *g_ui_voice;
musicNode *g_closestMusicNode;
musicNode *newClosest;

int g_musicSilenceMs = 0; //this is set by scripts to fade music out for x ms
int g_currentMusicSilenceMs = 0;
int g_musicSilenceFadeMs = 200; // if g_musicSilenceMs == 5000, than we spend 1000ms fading out and then 1000ms fading in

int musicFadeTimer = 0;
bool fadeFlag = 0; // for waiting between fading music in and out
bool entFadeFlag = 0;
int musicUpdateTimer = 0;
vector<Mix_Chunk*> g_staticSounds;

std::map<string, Mix_Chunk> g_static_sounds = {};

// ui
int g_textDropShadowColor = 100;
float g_textDropShadowDist = 2; //this is the pixels of the texture, for better or worse
bool protag_can_move = true;
int protag_is_talking = 0; // 0 - not talking 1 - talking 2 - about to stop talking
adventureUI *adventureUIManager;
float textWait = 30;		 // seconds to wait between typing characters of text
float text_speed_up = 1; // speed up text if player holds button. set to 1 if the player isn't pressing it, or 1.5 if she is
float curTextWait = 0;
bool old_z_value = 1; // the last value of the z key. used for advancing dialogue, i.e. z key was down and is now up or was up and is now down if old_z_value != SDL[SDL_SCANCODE_Z]
float g_healthbarBorderSize = 0;
bool g_showHUD = 1;

//for hungersystem
int g_foodpoints = 100;
int g_maxFoodpoints = 150; //150 foodpoints means the player can't possibly eat more, but they won't realize they're hungry until they get down to 100 points (they're wellfed, which is good)
int g_maxVisibleFoodpoints = 100;
int g_foodpointsDecreaseIntervalMs = 1000;
int g_foodpointsDecreaseAmount = 1;
int g_currentFoodpointsDecreaseIntervalMs = 0;
int g_starvingFoodpoints = 20; //need 20 or more foodpoints to use items

bool g_inventoryUiIsLevelSelect = 0; //set to 1 to repurpose to inventory UI for level select UI

bool g_inventoryUiIsLoadout = 0;

bool g_inventoryUiIsKeyboard = 0; //set to 1 to repurpose to inventory UI for player string input
string g_keyboardInput = "";

string g_alphabet = "abcdefghijklmnopqrstuvwxyz<^;";
string g_alphabet_lower = "abcdefghijklmnopqrstuvwxyz<^;";
string g_alphabet_upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ<^;";

string g_fancyAlphabetChars = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ?!@#$%^&*()+-._:;,\"\'0123456789";

//the pair contains a texture and a width
std::map<int, std::pair<SDL_Texture*, float>> g_fancyAlphabet{};
//here's a map of chars to ints, not containing special characters which are handled later
std::map<char, int> g_fancyCharLookup;

fancybox* g_fancybox;

vector<SDL_Texture*>* g_alphabet_textures;
vector<SDL_Texture*> g_alphabetLower_textures;
vector<float> g_alphabet_widths;
vector<SDL_Texture*> g_alphabetUpper_textures;
vector<float> g_alphabet_upper_widths;

int g_keyboardInputLength = 12;
string g_keyboardSaveToField = ""; //the save-field to write keyboard input to, when ready
SDL_Color g_textcolor = { 155, 115, 115 };
SDL_Color g_goldcolor = { 156, 127, 11 };
SDL_Color g_healthtextcolor = { 220, 203, 25 };
SDL_Color g_healthtextlowcolor = { 130, 24, 48 };

string g_levelSequenceName = "default"; //use the default level sequence for the base game by default
levelSequence* g_levelSequence;
int g_score; //generally used for unlocking levels, has checks/sets in scripting system and its own hud element

// scripts
float dialogue_cooldown = 0; // seconds until he can have dialogue again.
entity *g_talker = 0;				 // drives scripts, must be referenced before deleting an entity to avoid crashes
bool g_forceEndDialogue = 0; // used to end dialogue when the talking entity has been destroyed.

// debuging
SDL_Texture *nodeDebug;
clock_t debugClock;
string g_lifecycle = "Alpha";
ui* g_dijkstraDebugRed;
ui* g_dijkstraDebugBlue;
ui* g_dijkstraDebugYellow;
entity* g_dijkstraEntity;
bool g_ninja = 0;

//temporary debug stuff
int fdebug = -1;

// world
int g_layers = 12;							 // max blocks in world
int g_numberOfInterestSets = 50; // number of individual sets of pointsOfInterest available for entities to use
string g_first_map = "resources/maps/first/1.map";

// map editing, mapeditor, map-editor
bool g_mousemode = 1;
bool keyboard_marker_vertical_modifier_refresh = 0;
bool keyboard_marker_vertical_modifier_refresh_b = 0;

entity *nudge = 0;												// for nudging entities while map-editing
bool adjusting = 0;												// wether to move selected entity or change its hitbox/shadow position
bool g_autoSetThemesFromMapDirectory = 0; // if 1, loading a map will also set the texturedirectory/theme to the mapdir

// userdata - will be set on some file-select-screen
string g_saveName = "a";
vector<string> g_saveNames;
int g_saveOverwriteResponse = 0;
string g_saveToDelete = "";

std::map<string, int> g_save = {};
std::map<string, string> g_saveStrings = {};

// movement
float g_bhoppingBoost = 0; // the factor applied to friction whilst airborn
float g_defaultBhoppingBoost = 1;
float g_maxBhoppingBoost = 1;
float g_deltaBhopBoost = 1;
int protagConsecutiveBhops = 0; //increased for every successive bhop

float g_jump_afterslow = 0;
float g_jump_afterslow_seconds = 0; //make it so that the longer you bhop the longer you are slowed

int g_protagBonusSpeedMS = 0;
int g_protagMsToStunned = 0;
int g_usingMsToStunned = 0;

bool g_spin_enabled = 1;
entity* g_spin_entity = nullptr;
//entity* g_protag_s_ent = nullptr;
float g_spin_cooldown = 400;
float g_spin_max_cooldown = 100; //100, for spinning at will
float g_spinning_duration = 0;
float g_spinning_duration_max = 400; //400, for spinning at will
float g_afterspin_duration = 0;
float g_afterspin_duration_max = 200; //200, for spinning at will duration of afterspin imobility
float g_spinning_xvel = 0; //x and y velocities are locked upon starting a spin
float g_spinning_yvel = 0;
float g_spinning_boost = 1.8; //2.6 is pretty fast
float g_doubleSpinHelpMs = 16; //a spin can cancel another spin in the last x ms (double spin)
float g_spinJumpHelpMs = 0; //if you spin a frame after jumping you will jump and spin (spinjump)
float g_currentSpinJumpHelpMs = g_spinJumpHelpMs;
bool g_protag_jumped_this_frame = 0;

entity* g_spurl_entity = 0;

entity* g_chain_entity = 0;
float g_chain_time = 0;

bool storedJump = 0;
bool storedSpin = 0;

//for map-editor
ofstream ofile;
bool olddevinput[60];
bool makingtile, makingbox, makingdoor;         // states
int lx, ly, rx, ry;                             // for map editing corners;
float grid = 64;                                // default is 64
int width, height;                              // for selection display
int selectIndex = 0;                            // select entity
bool tiling = 1;                                // make current texture tile
bool drawhitboxes = 0;                          // devMode; //visualize hitboxes in map editor with command
bool drawNavMesh = 0; //I'm tired of the navmesh slowing down the framerate so much, this will be toggled separately
bool drawNavMeshEdges = 0; //really, drawing the nodes is fine and helpful for debugging, but the computational cost
                           //of drawing the edges is way too much on bigger maps (and should be contrained to camera dimensions

int debug_r = 255, debug_g = 255, debug_b = 50; // for draw color
int shine = 0;
bool occlusion = 0;                             // visualize occlusion (crappily) when map editing
int wallheight = 128;
int wallstart = 0;
bool showMarker = 1;
int lastAction = 0;                             // for saving the user's last action to be easily repeated
float navMeshDensity = 2;                       // navnodes every two blocks
int limits[4] = {0};
int m_enemyPoints = 0;                          // points the map has to spend on enemies when spawned and every x seconds afterwards
string textureDirectory = "mapeditor";          // for choosing a file to load textures from, i.e. keep textures for a desert style level, a laboratory level, and a forest level separa
float mapeditorNavNodeCullRadius = 96;
float mapeditorNavNodeTraceRadius = 150;        // for choosing the radius of the traces between navNodes when creating them in the editor. Should be the radius of the biggest entity
// in the level, so that he does not get stuck on corners

// for checking old console commands
vector<string> consolehistory;
int consolehistoryindex = 0;

string captex = "resources/static/diffuse/mapeditor/cap.qoi";
string walltex = "resources/static/diffuse/mapeditor/wall.qoi";
string floortex = "resources/static/diffuse/mapeditor/floor.qoi";
string masktex = "&";
// vector of strings to be filled with each texture in the user's texture directory, for easier selection
vector<string> texstrs;
// indexes representing which element of the array these textures make up
int captexIndex = 0;
int walltexIndex = 0;
int floortexIndex = 0;
ui *captexDisplay;
ui *walltexDisplay;
ui *floortexDisplay;

entity *selectPointer;
int layer = 0;
// these three variables are used in creating walls. By default, the v-key will only make rectangular boxs
bool autoMakeWalls = 1;
bool makeboxs = 1;
bool autoMakeWallcaps = 1;

tile *selection;
tile *markerz;
tile *marker;
tile *navNodeIconBlue;
tile *navNodeIconRed;
tile *navNodeIconYellow;
tile *worldsoundIcon;
tile *listenerIcon;
tile *musicIcon;
tile *cueIcon;
tile *waypointIcon;
tile *poiIcon;
tile *doorIcon;
tile *ddoorIcon;
tile *triggerIcon;
SDL_Texture* grassTexture;
SDL_Texture* cameraBlockerTextureA;
SDL_Texture* cameraBlockerTextureB;
SDL_Texture* cameraBlockerTextureC;
SDL_Texture* cameraBlockerTextureD;
textbox *nodeInfoText;
string entstring = ""; // last entity spawned;

string mapname = "";
string backgroundstr = "";

float g_earshot = 4 * 64; // how close do entities need to be to join their friends in battle

vector<int> g_creepyLocks = {2, 6, 12, 15};

//gameplay
float g_invincibleMs = 1000;

vector<entity*> g_familiars;
vector<entity*> g_ex_familiars;
entity* g_exFamiliarParent = 0;
float g_exFamiliarTimer = 0;
float g_familiarCombineX;
float g_familiarCombineY;
vector<entity*> g_combineFamiliars;
entity* g_combinedFamiliar = 0;

//for firetraps
const vector<int> g_ft_frames = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
const vector<bool> g_ft_flipped = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0, 1,  1,  1,  1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
const float g_ft_p = M_PI/24;
const vector<float> g_ft_angles = {0, g_ft_p, g_ft_p*2, g_ft_p*3 ,g_ft_p*4, g_ft_p*5, g_ft_p*6, g_ft_p*7, g_ft_p*8, g_ft_p*9, g_ft_p*10, g_ft_p*11, g_ft_p*12, g_ft_p*13, g_ft_p*14, g_ft_p*15, g_ft_p*16, g_ft_p*17, g_ft_p*18, g_ft_p*19, g_ft_p*20, g_ft_p*21, g_ft_p*22, g_ft_p*23, M_PI};

bool g_hide_protag = 0;

SDL_Texture* g_waterTexture = 0;
SDL_Surface* g_waterSurface = 0;
bool g_waterAllocated = 0;
bool g_waterOnscreen = 0;
Uint32* g_wPixels = 0;
const int g_wNumPixels = 512 * 440;
SDL_Surface* g_wDistort = 0;
float g_wAcc = 0;
SDL_Texture* g_wSpec;

//for optimizing line traces to only use collisions
//close to the player
vector<mapCollision*> g_lt_collisions;
vector<mapCollision*> g_is_collisions;
vector<entity*> g_l_entities;


//DUNGEON SYSTEM ( :DD )
vector<dungeonFloorInfo> g_dungeon;
int g_dungeonIndex;
//need a list of behemoths chasing player and how many rooms they will continue to chase
vector<dungeonBehemothInfo> g_dungeonBehemoths;
//need a list of mapobjects which are persistent over the course
//of the dungeon so that we don't have to reload textures constantly
vector<mapObject*> g_dungeonPersistentMOs;
vector<string> g_dungeonCommonFloors;
vector<string> g_dungeonUncommonFloors;
vector<string> g_dungeonRareFloors;
vector<string> g_dungeonSpecialFloors;
vector<string> g_dungeonEggFloors;
int g_dungeonDarkEffect;
int g_dungeonDarkEffectDelta;
bool g_dungeonDoorActivated = 0;

bool g_dungeonSystemOn;
bool g_levelFlashing;

int g_levelSequenceIndex;

Mix_Music* g_dungeonMusic = nullptr;
Mix_Music* g_dungeonChaseMusic = nullptr;
bool g_dungeonRedo = 0;

float g_dungeonMs = 0;
int g_dungeonHits = 0;

SDL_Texture* g_grossup = 0;
int g_grossupLoaded = 0;
int g_grossupShowMs = 0;
int g_maxGrossupShowMs = 1000;

vector<pair<int, Mix_Chunk*>> g_loadPlaySounds;

//for preventing the player from begining dialog after closing a menu
int g_menuTalkReset = 0;

float g_entlineDistance = 64;

int g_holddelete = 0;

int g_globalAccumulator = 0;
int g_tempAccumulator = 0;
bool g_benchmarking = 0;
bool g_entityBenchmarking = 0;

gamemode g_gamemode = gamemode::TITLE; //exploration, combat, gameover

turn g_turn = turn::PLAYER;

submode g_submode = submode::TEXT;

combatUI* combatUIManager;

int curCombatantIndex = 0; //the party combatant for which the player currently builds the turnserial

int curStatusIndex = 0;

int g_autoFight = 0;

int curLevelIndex = 0;

vector<int> g_combatInventory;

entity* g_combatWorldEnt; //the ent that walked into the player to start combat

int g_maxInventorySize = 14;

titleUI* titleUIManager;

//encounter system (pokemon tall grass)
string g_encountersFile = "";
vector<vector<pair<string, int>>> loadedEncounters = {};
int g_lastGrassX = 0;
int g_lastGrassY = 0;
float g_encounterChance = 0;
vector<string> loadedBackgrounds={};
int g_combatEntryType = 0;
lossSub g_lossSub = lossSub::INWIPE;

lossUI* lossUIManager = 0;

vector<keyItemInfo*> g_keyItems;

bool g_keyItemFlavorDisplay = 0;

bool fileExists(const std::string &name)
{
  if (FILE *file = fopen(name.c_str(), "r"))
  {
    fclose(file);
    return true;
  }
  else
  {
    return false;
  }
  //return PHYSFS_exists(name.c_str());
}


void playSound(int channel, Mix_Chunk *sound, int loops)
{
  // M("play sound");
  if (!g_mute && sound != NULL)
  {
    Mix_Volume(channel, g_sfx_volume * 128);
    Mix_PlayChannel(channel, sound, loops);
  }
}


SDL_Texture *MaskTexture(SDL_Renderer *renderer, SDL_Texture *mask, SDL_Texture *diffuse)
{
  int w, h;
  SDL_QueryTexture(diffuse, NULL, NULL, &w, &h);

  SDL_Texture *result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

  SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);
  SDL_SetRenderTarget(renderer, result);

  SDL_SetTextureBlendMode(mask, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(diffuse, SDL_BLENDMODE_NONE);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  SDL_RenderCopy(renderer, diffuse, NULL, NULL);
  SDL_RenderCopy(renderer, mask, NULL, NULL);

  SDL_SetRenderTarget(renderer, NULL);
  return result;
}

float Distance(int x1, int y1, int x2, int y2)
{
  return pow(pow((x1 - x2), 2) + pow((y1 - y2), 2), 0.5);
}

float XYDistance(int x1, int y1, int x2, int y2)
{
  return pow(pow((x1 - x2), 2) + pow((y1 - y2), 2) * XtoY, 0.5);
}

// old crappy code
//  template<class T>
//  navNode* getNodeByPosition(vector<T*> array, int fx, int fy) {
//  	//this is a placeholder solution for testing AI
//  	//this requires a binary search and sorted nodes to work reasonably for larger maps
//  	float min_dist = 0;
//  		navNode* ret;
//  		bool flag = 1;

// 		//just pitifully slow
// 		for (int i = 0; i < array.size(); i++) {
// 			float dist = Distance(fx, fy, array[i]->x, array[i]->y);
// 			if(dist < min_dist || flag) {
// 				min_dist = dist;
// 				ret = array[i];
// 				flag = 0;
// 			}
// 		}
// 		return ret;
// }

// get cardinal points about a position
//  0 is 12oclock, and 2 is 3oclock and so on
vector<int> getCardinalPoint(int x, int y, float range, int index)
{
  float angle = 0;
  switch (index)
  {
    case 6:
      angle = 0;
      break;
    case 7:
      angle = M_PI / 4;
      break;
    case 0:
      angle = M_PI / 2;
      break;
    case 1:
      angle = M_PI * (3.0 / 4.0);
      break;
    case 2:
      angle = M_PI;
      break;
    case 3:
      angle = M_PI * (5.0 / 4.0);
      break;
    case 4:
      angle = M_PI * (3.0 / 2.0);
      break;
    case 5:
      angle = M_PI * (7.0 / 4.0);
      break;
  }
  vector<int> ret;
  ret.push_back(x + (range * cos(angle)));
  ret.push_back(y + (range * sin(angle) * XtoY));
  return ret;
}

// convert frame of sprite to angle
float convertFrameToAngle(int frame, bool flipped)
{
  if (flipped)
  {
    if (frame == 0)
    {
      return M_PI / 2;
    }
    if (frame == 1)
    {
      return (M_PI * 3) / 4;
    }
    if (frame == 2)
    {
      return M_PI;
    }
    if (frame == 3)
    {
      return (M_PI * 5) / 4;
    }
    if (frame == 4)
    {
      return (M_PI * 6) / 4;
    }
  }
  else
  {
    if (frame == 0)
    {
      return M_PI / 2;
    }
    if (frame == 1)
    {
      return (M_PI * 1) / 4;
    }
    if (frame == 2)
    {
      return 0;
    }
    if (frame == 3)
    {
      return (M_PI * 7) / 4;
    }
    if (frame == 4)
    {
      return (M_PI * 6) / 4;
    }
  }

  return 0;
}


//idk why i have two
//this is for steeringAngle
//the only difference is that "flipped" is treated oppositely
float convertFrameToAngleNew(int frame, bool flipped)
{
  if (!flipped)
  {
    if (frame == 0)
    {
      return M_PI / 2;
    }
    if (frame == 1)
    {
      return (M_PI * 3) / 4;
    }
    if (frame == 2)
    {
      return M_PI;
    }
    if (frame == 3)
    {
      return (M_PI * 5) / 4;
    }
    if (frame == 4)
    {
      return (M_PI * 6) / 4;
    }
  }
  else
  {
    if (frame == 0)
    {
      return M_PI / 2;
    }
    if (frame == 1)
    {
      return (M_PI * 1) / 4;
    }
    if (frame == 2)
    {
      return 0;
    }
    if (frame == 3)
    {
      return (M_PI * 7) / 4;
    }
    if (frame == 4)
    {
      return (M_PI * 6) / 4;
    }
  }

  return 0;
}



//wrap an angle so that it is within the range of 0 and 2pi radians
float wrapAngle(float input) {
  while(input < 0) {
    input+= 2*M_PI;
  }
  while(input > 2*M_PI) {
    input-= 2*M_PI;
  }
  return input;
}

// convert an angle to a sprite's frame, for eight-frame sprites (arms)
int convertAngleToFrame(float angle)
{
  //up is pi/2
  vector<float> angles = {0, (M_PI * 1) / 4, M_PI / 2, (M_PI * 3) / 4, M_PI, (M_PI * 5) / 4, (M_PI * 6) / 4, (M_PI * 7) / 4, (M_PI * 2) }; //gonna guess that that last element is a bit off
  vector<int> frames = {6, 7, 0, 1, 2, 3, 4, 5, 6};

  for (long long unsigned int i = 0; i < angles.size(); i++)
  {
    if ((angles[i] + M_PI / 8) > angle)
    {
      return frames[i];
    }
  }

  //return 5; //this was a crutch
  return 0; //shouldn't happen
}

// measures distance in the world, not by the screen.
float XYWorldDistance(int x1, int y1, int x2, int y2)
{
  y1 *= 1 / XtoY;
  y2 *= 1 / XtoY;
  return pow(pow((x1 - x2), 2) + pow((y1 - y2), 2), 0.5);
}

float XYWorldDistance(entity* a, entity* b)
{
  int x1 = a->getOriginX();
  int y1 = a->getOriginY();

  int x2 = b->getOriginX();
  int y2 = b->getOriginY();

  return XYWorldDistance(x1, y1, x2, y2);
}

//faster, use this if you can
float XYWorldDistanceSquared(int x1, int y1, int x2, int y2)
{
  y1 *= 1 / XtoY;
  y2 *= 1 / XtoY;
  return pow((x1 - x2), 2) + pow((y1 - y2), 2);

}

float XYWorldDistanceSquared(entity* a, entity* b)
{
  int x1 = a->getOriginX();
  int y1 = a->getOriginY();

  int x2 = b->getOriginX();
  int y2 = b->getOriginY();

  return XYWorldDistanceSquared(x1, y1, x2, y2);
}

vector<string> splitString(string s, char delimiter)
{
  int start;
  long long unsigned int end;
  start = 0;
  string token;
  vector<string> ret;
  end = s.find(delimiter, start);
  while (end != string::npos)
  {
    token = s.substr(start, end - start);
    start = end + 1;
    ret.push_back(token);
    end = s.find(delimiter, start);
  }

  ret.push_back(s.substr(start));
  return ret;
}

bool replaceString(std::string &str, const std::string &from, const std::string &to)
{
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, from.length(), to);
  return true;
}

int yesNoPrompt(string msg)
{
  // warn for file overwrite
  const SDL_MessageBoxButtonData buttons[] = {
    {/* .flags, .buttonid, .text */ 0, 0, "Yes"},
    {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "No"},
  };
  const SDL_MessageBoxColorScheme colorScheme = {
    {/* .colors (.r, .g, .b) */
      /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
      {0, 0, 0},
      /* [SDL_MESSAGEBOX_COLOR_TEXT] */
      {200, 200, 200},
      /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
      {180, 180, 180},
      /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
      {0, 0, 0},
      /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
      {255, 255, 255}}};
  const SDL_MessageBoxData messageboxdata = {
    SDL_MESSAGEBOX_INFORMATION, /* .flags */
    NULL,												/* .window */
    "",													/* .title */
    msg.c_str(),								/* .message */
    SDL_arraysize(buttons),			/* .numbuttons */
    buttons,										/* .buttons */
    &colorScheme								/* .colorScheme */
  };
  int buttonid;
  if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0)
  {
    SDL_Log("error displaying message box");
  }
  return buttonid;
}

int rng(int min, int max) {
  if(max == min) {return min;}
  return ( ((int)rand() % (1+max-min))  + min);
}

float frng(float min, float max) {
  if(max == min) {return min;}
  return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX/(max-min)));
}

float clamp(float value, float min, float max) {
  return std::max(min, std::min(max, value));
}

void hurtProtag(int dmg) {
  
}

/* Restart dungeon floor upon taking damage
void hurtProtag(int dmg) {
  if(devMode) {return;}
  if(protag->invincibleMS > 0) {return;}
  if(g_dungeonDoorActivated) {return;} //don't trigger multiple times at once, don't get hurt if he already finished the level
 
  g_dungeonHits++;
  g_dungeonDoorActivated = 1;
  g_dungeonIndex--;

  g_dungeonRedo = 1; //don't deactive behemoths
}
*/

void transform3dPoint(float x, float y, float z, float &u, float &v) {
  u = floor(x) - g_camera.x;
  v = floor(y) - (floor(z) * XtoZ) - g_camera.y;
}

void doSpringForce(entity* target, entity* him)
{
  int xvec, yvec;

  const float stiffness = 0.08;

  xvec = target->getOriginX() - him->getOriginX();
  yvec = target->getOriginY() - him->getOriginY();

  float distancemag = XYWorldDistance(target, him);

  float springX = (xvec / distancemag) * (distancemag - 64) * stiffness;
  float springY = (yvec / distancemag) * (distancemag - 64) * stiffness;


  if(distancemag > 1) {
    him->x += springX;
    him->y += springY;
  }
}

string getCurrentDir() {
  char cwd[2000];
  getcwd(cwd, sizeof(cwd));
  string curdir(cwd);
  return curdir;
}

//string getCurrentDir() {
//  char buf[6000];
//  GetCurrentDirectory(6000, buf);
//  string curdir(buf);
//  return curdir;
//}

//trim remove trailing zeroes zeros
string to_stringF(double value) {
//  std::ostringstream out;
//  out << value;
//  string result = out.str();
  string result = to_string(value);

  result.erase(result.find_last_not_of('0') + 1, std::string::npos);

  if(result.back() == '.') {
    result.pop_back();
  }
  return result;
}
