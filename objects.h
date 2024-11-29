#ifndef objects_h
#define objects_h

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <cctype> //tolower()
#include <limits>
#include <stdlib.h>

#include <filesystem> //checking if a usable dir exists

#include "globals.h"
#include "lightcookies.h"

#include <utility>

#define PI 3.14159265

using namespace std;

void cyclePalette(SDL_Surface* source, SDL_Surface* destination, std::vector<Uint32>& palette);

navNode* getNodeByPos(vector<navNode*> array, int x, int y);
entity* searchEntities(string fname, entity* caller);
entity* searchEntities(string fname);
void playSoundAtPosition(int channel, Mix_Chunk *sound, int loops, int xpos, int ypos, float volume);
void debugUI();

/*
 *         pi/2 up
 *
 *
 *   pi left  *      0 right
 *    
 *
 *         3pi/4 down
 */

//returns the direction to turn from angle a to angle b
bool getTurningDirection(float a, float b);

float angleMod(float a, float n);

//get difference between angles
float angleDiff(float a, float b);

void parseScriptForLabels(vector<string> &sayings);

class heightmap {
  public:
    SDL_Surface* image = 0;
    string name;
    string binding;
    float magnitude = 0.278; //0.278 was a former value. I'd love to expose this value but I cant think of a good way

    heightmap(string fname, string fbinding, float fmagnitude);

    ~heightmap();

    Uint32 getpixel(SDL_Surface *surface, int x, int y);
};

class navNode {
  public:
    int x;
    int y;
    int z = 0;
    vector<navNode*> friends;
    vector<float> costs;

    bool highlighted = 0;
    float costFromSource = 0; //updated with dijkstras algorithm
    navNode* prev = nullptr; //updated with dijkstras algorithm
    
    int costFromUsage = 0; //used to make nodes less appealing if
                             //another entity is using them

    string name = "";
    bool enabled = 1; //closing doors can disable navNodes, so that entities will try to find another way

    navNode(int fx, int fy, int fz);

    void Add_Friend(navNode* newFriend);

    void Update_Costs();

    void Render(int red, int green, int blue);

    ~navNode();
};

navNode* getNodeByPosition(int fx, int fy);

void RecursiveNavNodeDelete(navNode* a);

void Update_NavNode_Costs(vector<navNode*> fnodes);

class coord {
  public:
    int x;
    int y;
};

class rect {
  public:
    int x;
    int y;
    int z=0; //rarely used
    int width;
    int height;
    int zeight = 32;


    rect();

    rect(int a, int b, int c, int d);

    rect(int fx, int fy, int fz, int fw, int fh, int fzh);

    void render(SDL_Renderer * renderer);
};

int LineTrace(int x1, int y1, int x2, int y2, bool display, int size = 30, int layer = 0, int resolution = 10, bool visibility = 0, bool fogOfWar = 0);

class pointOfInterest {
  public:
    int x = 0;
    int y = 0;
    int index = 0;

    pointOfInterest(int fx, int fy, int findex);

    //probably best to not call this when unloading a level
    ~pointOfInterest();
};

class mapCollision {
  public:
    rect bounds;
    //related to saving/displaying the block
    string walltexture;
    string captexture;
    bool capped = false;
    bool hidden = 0; // 1 when this object is hidden behind something and doesn't need to be drawn. used for triangular walls

    //tiles created from the mapCollision, to be appropriately deleted
    vector<mapObject*> children;

    //tri and boxes which are part of the map are pushed back on
    //an array of mapCollisions to be kept track of for deletion/undoing
    mapCollision();

    //copy constructor
    mapCollision(const mapCollision & other);

    // //move constructor
    // mapCollision(mapCollision && other) {
    // 	this->walltexture = other.walltexture;
    // 	this->captexture = other.captexture;
    // 	this->capped = other.capped;
    // 	this->children = other.children;
    // }

    //copy assignment
    mapCollision& operator=(const mapCollision &other);


    virtual ~mapCollision();
};

class tri:public mapCollision {
  public:
    int x1; int y1;
    int x2; int y2;
    int type; //orientation
    int style = 0; // 0 - plain 1 - outround
    float m; //slope
    int b; //offset
    int layer = 0;
    bool shaded = 0;

    int x;
    int y;
    int width;
    int height;

    tri(int fx1, int fy1, int fx2, int fy2, int flayer, string fwallt, string fcapt, bool fcapped, bool fshaded, int fstyle = 0);

    ~tri();

    void render(SDL_Renderer* renderer);
};

class impliedSlopeTri:public mapCollision {
  public:
    int x1; int y1;
    int x2; int y2;
    int type; //only types for impliedSlopeTri are 1 (:.) and 2 (.:)
    int style = 0; //set during bake, used for shading
    float m;
    int b;
    int layer = 0;
    
    int x;
    int y;
    int width;
    int height;

    impliedSlopeTri(int fx1, int fy1, int fx2, int fy2, int flayer, int fstyle);

    ~impliedSlopeTri();

    void render(SDL_Renderer* renderer);
};

//sortingfunction for optimizing fog and triangular walls
//sort based on x and y
//was inline, can I make this inline again?
int trisort(tri* one, tri* two); 

class ramp : public mapCollision {
  public:
    int x, y;
    // int width = 64;
    // int height = 55;
    int layer = 0;
    int type; //0 means the higher end is north, 1 is east, and so on

    ramp(int fx, int fy, int flayer, int ftype, string fwallt, string fcapt);

    ~ramp(); 
};


bool PointInsideRightTriangle(tri* t, int px, int py);

bool IPointInsideRightTriangle(impliedSlopeTri* t, int px, int py); 

bool RectOverlap(rect a, rect b); 

bool RectOverlap3d(rect a, rect b); 

bool ElipseOverlap(rect a, rect b); 

bool CylinderOverlap(rect a, rect b, int skin = 0); 

bool RectOverlap(SDL_Rect a, SDL_Rect b); 

bool RectOverlap(SDL_FRect a, SDL_FRect b); 

//is a inside b?
bool RectWithin(rect a, rect b); 

bool TriRectOverlap(tri* a, int x, int y, int width, int height); 

bool TriRectOverlap(tri* a, rect r); 

//for impliedSlopeTris
bool ITriRectOverlap(impliedSlopeTri* a, int x, int y, int width, int height); 

SDL_Rect transformRect(SDL_Rect input); 

SDL_FRect transformRect(SDL_FRect input); 

rect transformRect(rect input); 

class box:public mapCollision {
  public:
    //rect bounds;
    bool active = true;
    int layer = 0;
    bool shineTop = 0;
    bool shineBot = 0;

    bool shadeTop = 0;

    //why is this one an int? because different values can affect the front corners.
    int shadeBot = 0;
    //0 - no shading
    //1 - standard shading (corners will be there if their side is there)
    //2 - just corners, that way if this block is behind a triangle

    bool shadeLeft = 0;
    bool shadeRight = 0;

    int valid = 1; //to try and fix the infamous "heisenbug" upon calling EB in the console out of gdb


    box(int x1f, int y1f, int x2f, int y2f, int flayer, string &fwallt, string &fcapt, bool fcapped, bool fshineTop, bool fshineBot, const char* shading); 

    ~box(); 
};


//For walls which are not drawn, this object implies that they are sloped so that entities cannot hide behind them, which makes this aspect of the world a bit easier to understand and it feels very natural and "forgetable"
//always two blocks zeight, one block in y, and whatever in x
//support for layers?
class impliedSlope:public mapCollision {
public:
  int layer = 0;
  bool shadeLeft = 0;
  bool shadeRight = 0;
  bool shadedAtAll = 1; //the invisible wall feature is being moved to islopes
  
  impliedSlope(int x1, int y1, int x2, int y2, int flayer, int fsleft, int fsright, int fShadedAtAll); 

  ~impliedSlope(); 
};


// The idea of a collisionZone is to reduce overhead for large maps
// by having entities check if they are overlapping a collisionZone and only test for
// collision with other walls/projectiles/entities also overlapping that collisionZone
// They will be able to be placed manually or procedurally
class collisionZone {
  public:
    rect bounds = {0,0,10,10};
    vector<vector<box*>> guests;

    collisionZone(int x, int y, int width, int height); 

    ~collisionZone(); 

    //ATM we will be doing this on mapload
    void inviteAllGuests(); 

    void debugRender(SDL_Renderer* renderer); 
};



//cast a ray from the sky at a xy position and returns a z position of an intersection with a block
int verticalRayCast(int fx, int fy); 

class door {
  public:
    float x = 0;
    float y = 0;
    float z = 0;
    float width = 50;
    float height = 50;
    float zeight = 64;
    float friction;
    rect bounds;
    string to_map;
    string to_point;


    door(SDL_Renderer * renderer, const char* fmap, string fto_point,  int fx, int fy, int fz, int fwidth, int fheight, int fzeight); 

    ~door(); 

};

//unlike doors, dungeon doors don't initiate a loading sequence, they just progress the dungeon
class dungeonDoor {
  public:
    float x = 0;
    float y = 0;
    float width = 50;
    float height = 50;
    dungeonDoor(int fx, int fy, int fwidth, int fheight);
    ~dungeonDoor();
};


class tile {
  public:
    float x = 0;
    float y = 0;
    int z = 0; //really just layer

    float width = 0;
    float height = 0;
    float xoffset = 0; //for aligning texture across the map, change em to floats later
    float yoffset = 0;
    float dxoffset = 0; //for changing texture coords
    float dyoffset = 0;
    int texwidth = 0;
    int texheight = 0;

    bool wraptexture = 1; //should we tile the image or stretch it?
    bool wall = 0; //darken image if it is used for a wall as opposed to a floor
    bool asset_sharer = 0; //1 if the tile is sharing another tile's texture, and this texture should in this case not be deleted in the destructor
    bool software = 0; //tiles used for the mapeditor/ui/etc are specially marked to avoid saving them or trying to delete them from a map

    SDL_Surface* image = 0;
    SDL_Texture* texture = 0;
    string fileaddress = "df"; //for checking if someone else has already loaded a texture
    string mask_fileaddress = "&"; //unset value



    tile(SDL_Renderer * renderer, const char* filename, const char* mask_filename, int fx, int fy, int fwidth, int fheight, int flayer, bool fwrap, bool fwall, float fdxoffset, float fdyoffset); 

    ~tile(); 

    void reloadTexture(); 

    void reassignTexture(); 

    rect getMovedBounds(); 

    void render(SDL_Renderer * renderer, camera fcamera); 
};


class attack {
  public:
    float maxCooldown = 400; //ms
    float cooldown = 0;
    string name = "";
    bool canBeHeldDown = 1;
    float shotLifetime = 500; //ms
    int width = 20;
    int height = 20;
    float damage = 1;
    float spread = 0;
    float randomspread = 0;
    int range = 512; // max range, entities will try to be 8% of this to hit safely. in worldpixels
    float size = 0.1;
    float speed = 1;
    int numshots = 1;
    SDL_Texture* texture;

    bool melee = 0; //melee attacks are special in that the entity will try to go directly to their target, but might attack
    //before they reach them, depending on their range
    //if this distinction didn't exist, melee enemies would stutter while chasing their targets, because
    //as soon as they get in range they will stop.

    bool snake = 0; //can bullets travel through walls

    float xoffset = 5;

    bool assetsharer = 0;
    string spritename = "";


    //animation
    int framewidth;
    int frameheight;
    int xframes;
    int yframes;
    vector<coord> framespots;

    //given some float time, return floats for x and y position
    //will be rotated/flipped

    //i should update this with variables, which are expressions from an attack file
    float forward(float time); 

    float sideways(float time); 

    //new param to attack()
    //entities that are deleted on map closure
    //can try to share attack graphics
    //but not entities that could possibly join the party
    attack(string filename, bool tryToShareTextures); 

    ~attack(); 
};


class weapon {
  public:
    string name;
    int combo = 0;
    float maxComboResetMS = 1000;
    float comboResetMS = 0;
    vector<attack*> attacks;

    int persistent = 0;

    weapon();

    //add constructor and field on entity object
    //second param should be 0 for entities
    //that could join the party and 1 otherwise
    weapon(string fname, bool tryToShareGraphics); 

    ~weapon(); 
};

//anything that exists in the 3d world
class actor {
  public:
    float x = 0;
    float y = 0;
    float z = 0;
    float width = 0;
    float height = 0;
    float zeight = 0;
    float sortingOffset = 0;
    float bonusSortingOffset = 0; //added oct 2023 to make fogslates look better when there's just a bit of fog but it gets rendered above an ent and the fogslate behind renders behind that ent, but the slates are meant to blend out
    float baseSortingOffset = 0;
    SDL_Texture* texture = nullptr;
    rect bounds = {0, 0, 10, 10};
    string name = "unnamed";

    bool tangible = 1;
    bool visible = 1;

    //add entities and mapObjects to g_actors with dc
    actor(); 

    virtual ~actor(); 

    virtual void render(SDL_Renderer * renderer, camera fcamera); 


    float getOriginX(); 

    float getOriginY(); 

    //for moving the object by its origin
    //this won't move the origin relative to the sprite or anything like that
    void setOriginX(float fx); 

    void setOriginY(float fy); 
};

//this was inline :/
//int compare_ent (actor* a, actor* b); 

void sort_by_y(vector<actor*> &g_entities); 


class effectIndex {
  public:
    string texname = "default";
    string name ="";
    bool OwnsTexture = 1;
    SDL_Texture* texture;
    int spawnNumber = 12;
    float spawnRadius = 1;
    int msPerFrame = 0;
    bool killAfterAnim = 0;

    int plifetime = 5;
    int disappearMethod = 0; // 0 -> shrink, 1-> fade

    int pwidth = 50;
    int pheight = 50;

    int yframes = 1;
    int chooseRandomFrame = 0;

    int framewidth = 1;
    int frameheight = 1;

    int alpha = 255;
    int deltaAlpha = 0;

    int persistent = 0;

    float pvelocityx = 0;
    float pvelocityy = 0;
    float pvelocityz = 0;
    float paccelerationx = 0;
    float paccelerationy = 0;
    float paccelerationz = 0;
    float pdeltasizex = -10;
    float pdeltasizey = 10;

    //for spawners, which are attached to entities
    entity* spawnerParent = nullptr;
    float spawnerXOffset = 0;
    float spawnerYOffset = 0;
    float spawnerZOffset = 0;
    float spawnerIntervalMs = 0;

    

    effectIndex(string filename, SDL_Renderer* renderer);

    //given coordinates, spawn particles in the level
    void happen(int fx, int fy, int fz, float fangle);

    ~effectIndex();

};



class particle : public actor {
  public:
    effectIndex* type = nullptr;
    int lifetime = 0;
    float velocityx = 0;
    float velocityy = 0;
    float velocityz = 0;
    float accelerationx = 0;
    float accelerationy = 0;
    float accelerationz = 0;
    float deltasizex = 0;
    float deltasizey = 0;

    int msPerFrame = 0;
    int msTilNextFrame = 0;
    bool killAfterAnim = 0;

    float angle = 0;

    int frame = 0;
    int yframes = 1;
    int framewidth = 0;
    int frameheight = 0;

    float alpha = 255; //this is out of 25500, converted later
    float curAlpha = 0;
    float deltaAlpha = 0;

    SDL_Texture* texture;

    particle(effectIndex* ftype); 

    ~particle(); 

    void update(int elapsed, camera fcamera); 

    // particle render
    void render(SDL_Renderer* renderer, camera fcamera);
};

//a class associated with an effectIndex.
//It has a parent entity and an offset
//and triggers the effect every n seconds
//
//the data actually comes from the .eft file
class emitter {
public:
  entity* parent = nullptr;
  effectIndex* type = nullptr;
 
  int timeToLiveMs; //if 0, particle is everlasting

  int xoffset = 0;
  int yoffset = 0;
  int zoffset = 0;

  int maxIntervalMs = 1000;
  int currentIntervalMs = 0; //counts down to zero

  emitter(); 

  ~emitter(); 
};


class cshadow:public actor {
  public:
    float size;
    actor* owner = 0;
    SDL_Surface* image = 0;
    SDL_Texture* texture = g_shadowTexture;
    int xoffset = 0;
    int yoffset = 0;
    int alphamod = 255;
    bool enabled = 1;

    cshadow(SDL_Renderer * renderer, float fsize); 

    ~cshadow(); 

    void render(SDL_Renderer* renderer, camera fcamera);
};



class projectile : public actor {
  public:
    int layer;
    float xvel;
    float yvel;

    bool asset_sharer;

    bool animate = 0;
    int frame = 0;
    int animation = 0;
    int xframes = 0;
    int frameInAnimation = 0;
    SDL_RendererFlip flip = SDL_FLIP_NONE;

    int curheight = 0;
    int curwidth = 0;

    entity* owner = nullptr;
    float maxLifetime = 0.4;
    float lifetime = 500;
    float angle = 0;
    attack* gun;
    cshadow * shadow = 0;

    projectile(attack* fattack); 

    ~projectile(); 

    void update(float elapsed); 

    void render(SDL_Renderer * renderer, camera fcamera); 
};



class mapObject:public actor {
  public:

    float xoffset = 0;
    float yoffset = 64;
    bool wall; //to darken entwalls
    //float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
    float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
    //0 - slant left, 1 - slant right, 2 - pillar
    int effect = 0;
    bool asset_sharer = 0;
    int framewidth = 0;
    int frameheight = 0;
    bool diffuse = 1; //is this mapobject used for things such as walls or floors, as opposed to props or lighting
    string mask_fileaddress = "&"; //unset value
    SDL_Texture* alternative = nullptr; //representing the texture tinted as if it were the opposite of the texture in terms of shading
    mapCollision* parent = nullptr;

    mapObject(SDL_Renderer * renderer, string imageadress, const char* mask_filename, float fx, float fy, float fz, float fwidth, float fheight, bool fwall = 0, float extrayoffset = 0); 

    //There was a problem when I ported to windows, where mapobjects which used masks for their textures (e.g. the tops of triangular walls)
    //would lose their textures. This function exists to reload them after the windowsize has been changed.
    void reloadTexture(); 

    //this is used with reloadTexture for any mapObjects which are asset-sharers
    void reassignTexture(); 

    ~mapObject(); 

    rect getMovedBounds(); 

    void render(SDL_Renderer * renderer, camera fcamera); 
};


//work as INSTANCES and not INDEXES
class ability {
  public:
    int lowerCooldownBound = 40000;
    int upperCooldownBound = 60000;

    int cooldownMS = 50000;

    float lowerRangeBound = 0; //in worldpixels
    float upperRangeBound = 5;

    //I need a way to make abilities feel right, in terms of how they're charged
    //for instance, it would be odd if as soon as the player found an enemy, they suddenly used three abilities almost
    //at the same time, the moment they came into range
    //and, it would be odd if there were abilities that were meant to be seen rarely with high cooldowns,
    //but were never used because the player would break range before it could happen
    //so abilities must be set with different ways of cooling down
    // 0 -> reset -> ability is reset when out of range. The ability is only charged when in range
    // 1 -> stable -> ability isn't reset, but it only charges when in range.
    // 2 -> accumulate -> ability charges regardless of range, meaning it will often activate as soon as the enemy finds the player

    int resetStableAccumulate = 1;

    string name = "unset";

    bool ready = 0;

};

// I want a better system for displaying text in the dialog box
// The new system will involve an instance of class FANCYBOX
// this class has functions for pushing text and advancing it
// one instance of FANCYBOX has multiple instances of FANCYWORD
// these are positioned relative to the origin of the parent FANCYBOX
// instances of FANCYWORD have FANCYCHAR children which are positioned
// relative to their parent FANCYWORD 
//
// There are some benefits to doing all of this
// 
// One, the words are arranged while all of the letters are hidden,
// meaning as the letters are revealed one at a time, long words drawn at the 
// end of a line don't travel from the end of one line to the begining of another
// 
// Two, individual words can be colored differently or even animated
// 
// Three, the size/position of letters can be animated to produce
// effects similar to that of paper mario ttyd
//
// Finally, special characters are easier to handle
//
// g_alphabet_lower, g_alphabet_upper, g_specialChars
// g_alphabet_widths, g_alphabet_upper_widths, g_specialChars_widths
// g_alphabetLower_textures, g_alphabetUpper_textures, g_specialChars_textures

class fancyword;
class fancybox;

class fancychar {
public:
  float x = 0;
  float y = 0;

  float xd = 0; //x dynamic
  float yd = 0;

  int index = 0;

  int color = 0;

  float bonusWidth = 0; //this doesn't change

  float bonusSize = 0;
  int opacity = 255;

  float accumulator = 0;
  float accSpeed = 0.001;

  float width = 0; 
  bool show = 0;
  SDL_Texture* texture; //pointer to texture in g_fancyAlphabet

  char movement = '0';
  
  void setIndex(int findex); 

  void render(fancyword* parent);

  void update(float elapsed);
};

class fancyword {
public:
  float x = 0;
  float y = 0;
  float width = 0;
  vector<fancychar> chars;

  void append(int index, float fbw);

  void render(); 

  void update(float elapsed);
};

class fancybox {
public:
  rect bounds; //screenspace bb
  vector<fancyword> words;
  
  int wordProgress = 0;
  int letterProgress = 0;

  bool show = 0;

  fancybox(); 

  void render(); 

  void update(float elapsed);

  //minding newlines and keeping letters of a word on the same line, arrange words/letters together before revealing them
  //this param might contain some encoding for special chars
  void arrange(string fcontent); 

  int reveal(); 

  void revealAll();

  void clear();
  
  
};

class adventureUI {
  public:
    bool showHud = 1;

    int dialogue_index = 0;

    bool playersUI = 1; //is this the UI the player will use
    //for talking, using items, etc?

    bool executingScript = 0;
    
    //Since the script interpreter system grew out of
    //a simple dialogue system, it was impossible
    //to have an entity which both ran abilities
    //and could be interacted with.
    //Now, when an entity uses an ability, she will
    //set this flag to 1 and load the lines of the
    //ability to ownScript
    bool useOwnScriptInsteadOfTalkersScript = 0;
    vector<string> ownScript;


    ui* talkingBox = 0;
    ui* talkingBoxTexture = 0;

    ui* dialogProceedIndicator = 0; //the arrow which appears when there's no more text to write
    const int dpiDesendMs = 400;
    int c_dpiDesendMs = 0;
    int c_dpiAsendTarget = 0.9;
    bool c_dpiAsending = 0;
    const float dpiAsendSpeed = 0.0002;

   

    SDL_Color currentTextcolor = {155, 115, 115};
    SDL_Color defaultTextcolor = {155, 115, 115};
    
    //gruvbox
//    vector<pair<string,SDL_Color>> textcolors = {
//      {"default", {251, 73, 52}}, //Pink, regular textbox
//
//      {"lightred", {251, 73, 52}},
//      {"lightgreen", {184, 187, 38}},
//      {"lightyellow", {250, 189, 47}},
//      {"lightblue", {131, 165, 152}},
//      {"lightpurple", {211, 134, 155}},
//      {"lightteal", {142, 192, 124}},
//      {"lightorange", {254, 128, 25}},
//
//
//      {"red", {204, 36, 29}},
//      {"green", {152, 151, 26}},
//      {"yellow", {215, 153, 33}},
//      {"blue", {69, 133, 136}},
//      {"purple", {177, 98, 134}},
//      {"teal", {104, 157, 106}},
//      {"orange", {214, 93, 14}},
//
//
//      {"darkred", {157, 0, 6}},
//      {"darkgreen", {121, 116, 14}},
//      {"darkyellow", {181,118,20}},
//      {"darkblue", {7,102,120}},
//      {"darkpurple", {143,63,113}},
//      {"darkteal", {66,123,88}},
//      {"darkorange", {175,58,3}},
//                                   };

    // DIALOG TEXT COLORS
    // FANCY COLORS
    // FANCYBOX COLORS
    vector<pair<string,SDL_Color>> textcolors = {
      {"default", {155, 115, 115}}, //0, pink

      {"red", {155, 115, 115}}, //1
      {"orange", {155, 135, 115}}, //2
      {"yellow", {155, 155, 115}}, //3
      {"green", {115, 155, 115}}, //4
      {"aqua", {115, 155, 155}}, //5
      {"purple", {155, 115, 155}}, //6
      {"blue", {115, 115, 155}}, //7

      {"bred", {180, 45, 45}}, //8
      {"borange", {180, 113, 45}}, //9
      {"byellow", {180, 180, 45}}, //a
      {"bgreen", {45, 180, 45}}, //b
      {"baqua", {45, 180, 180}}, //c
      {"bpurple", {180, 45, 180}}, //d
      {"bblue", {45, 45, 180}}, //e

      {"fred", {130, 55, 55}}, //f
      {"forange", {130, 92, 55}}, //g
      {"fyellow", {130, 130, 55}}, //h
      {"fgreen", {55, 130, 115}}, //i
      {"faqua", {55, 130, 130}}, //j
      {"fpurple", {130, 55, 130}}, //k
      {"fblue", {55, 55, 130}}, //l
      {"white", {155, 155, 155}}, //m
      {"grey", {100, 100, 40}}, //n

                                   };

    string currentFontStr = g_font;
    string defaultFontStr = g_font;

    vector<pair<string, string>> fonts = {
      {"default","engine/fonts/Rubik-Bold.ttf"},
      {"creepy","engine/fonts/RubikPuddles-Regular.ttf"},
      {"dripping","engine/fonts/RubikWetPaint-Regular.ttf"},
      {"outline","engine/fonts/RubikBurned-Regular.ttf"},
      {"bubbly","engine/fonts/RubikBubbles-Regular.ttf"},
      {"handwritten","engine/fonts/EduNSWACTFoundation-Bold.ttf"},
      {"innocent","engine/fonts/ConcertOne-Regular.ttf"}
    };
   
    //scripts need a way to remember
    //an entity so that we can spawn someone
    //and then animate them
    //without worrying if we are dealing with
    //the same entity
    entity* lastReferencedEntity = 0;

    textbox* talkingText = 0;
    textbox* responseText = 0;
    textbox* escText = 0;
    textbox* levelTimeText = 0;
    textbox* levelHitsText = 0;
    textbox* inputText = 0;

    string keyboardPrompt = "";

    string pushedText; //holds what will be the total contents of the messagebox.
    string curText; //holds what the user currently sees; e.g. half of the message because it hasnt been typed out yet
    bool typing = false; //true if text is currently being typed out to the window.
    Mix_Chunk* blip =  Mix_LoadWAV( "sounds/voice-bogged.wav" );
    Mix_Chunk* confirm_noise = Mix_LoadWAV( "sounds/peg.wav" );
    //vector<string>* sayings;
    vector<string>* scriptToUse;
    entity* talker = 0;
    entity* selected = nullptr; //this is used for setting selfdata, instead of just using the talker pointer (that was limited)
    bool askingQuestion = false; //set if current cue is a question
    string response = "tired"; //contains the last response the player gave to a question
    vector<string> responses; //contains each possible response to a question
    long long unsigned int response_index = 0; //number of response from array responses
    int sleepingMS = 0; //MS to sleep cutscene/script
    bool sleepflag = 0; //true for one frame after starting a sleep
    bool mobilize = 0; //used to allow the player to move during /sleep calls

    ui* emotion = 0;


    ui* inventoryA = 0; //big box, which has all of the items that the player has
    ui* inventoryB = 0; //small box, which will let the player quit or close the inventory

    ui* crosshair = 0; //for guiding player to objectives
   
    //a ui element for each behemoth, for pointing towards them similar to the crosshair
    ui* b0_element = 0; 
    ui* b1_element = 0;
    ui* b2_element = 0;
    ui* b3_element = 0;

    //this element is shown when the player is within hearing range to a behemoth
    ui* hearingDetectable = 0;
    
    //shown when a behemoth is finding the player by sight
    ui* seeingDetectable = 0;

    textbox* healthText = 0;
    textbox* hungerText = 0;

    ui* healthPicture = 0; //picture of a heart
    ui* hungerPicture = 0; //picture of a heart
    ui* tastePicture = 0; //picture of a tung
    ui* thoughtPicture = 0; //picture of a brain

    //for making the heart shake every now and then
    int heartShakeIntervalMs = 12000;
    int heartShakeIntervalRandomMs = 5000;
    int maxHeartShakeIntervalMs = 12000;
    int heartShakeDurationMs = 0;
    int maxHeartShakeDurationMs = 1100;

    //for making the heart beat
    int heartbeatDurationMs = 1000;
    int maxHeartbeatDurationMs = 1000;
    float heartShrinkPercent = 0.01;

    //stomach shake (rumbling)
    int stomachShakeIntervalMs = 12000;
    int stomachShakeIntervalRandomMs = 5000;
    int maxstomachShakeIntervalMs = 12000;
    int stomachShakeDurationMs = 0;
    int maxstomachShakeDurationMs = 1100;

    //tung shake (swallowing)
    int tungShakeIntervalMs = 42500; //swallow every 45 seconds, or after eating
    int tungShakeIntervalRandomMs = 5000;
    int maxTungShakeIntervalMs = 12000;
    int tungShakeDurationMs = 0;
    int maxTungShakeDurationMs = 1100;


    bool light = 0;

    //textbox* scoreText = 0;

    //textbox* systemClock = 0;

    int countEntities = 0; //used now for /lookatall to count how many entities we've looked at
        
    void showTalkingUI();
    void hideTalkingUI();

    void showInventoryUI();

    void hideInventoryUI();

    adventureUI(SDL_Renderer* renderer, bool plight = 0);
 
    ~adventureUI();

    void initFullUI();

    void pushText(entity* ftalker);

    void pushText(string text);

    void pushFancyText(entity * ftalker);

    void updateText();

    void skipText();
    
    void initDialogue();

    void continueDialogue();

    void positionKeyboard(); //position UI elements for keyboard- so the prompt is at the top, than the keyboard, than the input
                             
    void positionInventory();

    float inventoryYStart = 0.05;
    float inventoryYEnd = 0.6;
    
    void hideHUD(); //hide heart and other stuff if the player is in the menus
    void showHUD();
};


class worldsound {
  public:
    //a playable sound in the world, with a position
    Mix_Chunk* blip;
    float volumeFactor = 1;
    float maxDistance = 1200; //distance at which you can no longer hear the sound
    float minWait = 1;
    float maxWait = 5;

    string name;
    int x = 0;
    int y = 0;
    float cooldown = 0;

    entity* owner = nullptr;

    worldsound(string filename, int fx, int fy); 

    ~worldsound(); 

    void update(float elapsed);
};

//manage an entities statuseffects
class statusComponent {
  public:
    class status {
      public:
        float lifetime = 0;
        float factor = 1;
        float currentProckWaitMS = 0; //timer for when to prock
        bool prockThisFrame = 0;
    };

    class statusSet {
      public:
        bool complex = 0;
        bool active = 0;

        float maxProckWaitMS = 0;
        float resistence = 0;
        bool immunity = 0;
        bool integerbound = 0;

        vector<status> statuses;

        void addStatus(float ptime, float pfactor); 

        void clearStatuses(); 

        float updateStatuses(float elapsedMS); 

        void cleanUpStatuses(); 

        int check();
    };

    statusSet stunned;
    statusSet marked;
    statusSet disabled;
    statusSet enraged;
    statusSet poisoned;
    statusSet slown;
    statusSet healen;
    statusSet buffed;
    statusSet invincible;

    statusComponent(); 

};

//part of a statemachine for a behemoth, e.g. a stalking state, a sleeping state, a hiding state, a roaming state
struct state {
  string name = "";
  int interval = 0;
  int nextInterval = 0;
  int blocks = 0;
  vector<int> nextStates; //after the interval has elapsed, a next state will be chosen with rng
  vector<float> nextStateProbabilities; //based on probabilities
};

class entity:public actor {
  public:
    int ignoreSolids = 0;
    float widthmodifier;
    float heightmodifier;
 
    vector<entity*> spawnlist;
    vector<actor*> actorlist;

    //dialogue

    //int dialogue_index = 0; //we really don't want to use a dialogue_index in 2023

    //"cache" values for originX and originY to save time
    int cachedOriginX = 0;
    int cachedOriginY = 0;

    bool specialAngleOverride = 0;

    bool cachedOriginValsAreGood = 0;

    int isFogSlate = 0;

    //sounds
    Mix_Chunk* footstep;
    Mix_Chunk* footstep2;
    Mix_Chunk* voice;

    //a set of sounds that are attached to this entity
    vector<worldsound*> mobilesounds;

    //for musical entities
    Mix_Music* theme = 0;
    float musicRadius = 50;

    int footstep_reset = 0; //used for playing footsteps accurately with anim

    bool dontSave = 0;

    float distanceToTarget = 0;

    //basic movement
    float xagil = 0;
    float xaccel = 0;
    float yaccel = 0;
    float zaccel = 0;
    float xvel = 0;
    float yvel = 0;
    float zvel = 0;
    bool useGravity = 1;

    // angular reform of movement system
    // as long-planned, entities will not move in explicit directions,
    // but rather specify target angles and move (forwards)
    float steeringAngle = 0; //degrees, 0-360
    float targetSteeringAngle = 0; //degrees
    float turningSpeed = 1; //turns per second

    float forwardsVelocity = 0; //set by inputting movement, this will be converted to xvel and yvel based on to feed back into the old system.

    bool forceAngularUpdate = 0;

    // e.g., targetSteeringAngle is set for the protag when a movement key is pressed,
    // and steeringAngle is slowly interpolated to that angle, used for both
    // the frame chosen and the actual force of movement on the protag.
    
    //I want to make entities be forced in the direction they are facing, so this is used for that
    float forwardsPushVelocity = 0;
    float forwardsPushAngle = 0;

    //this is for making entities point where they are trying to walk
    float walkingxaccel = 0;
    float walkingyaccel = 0;


    int layer = 0; //related to z, used for boxs
    int stableLayer = 0; //layer, but only if it's been held for some ms
    bool grounded = 1; //is standing on ground
    bool groundedByEntity = 0;
    float xmaxspeed = 0;
    float baseMaxSpeed = 0;
    int mobile = 1;
    float bonusSpeed = 0;
    float friction = 0;
    float baseFriction = 0;
    int recalcAngle = 0; //a timer for when to draw sprites with diff angle

    float currentAirBoost = 1;
    float slowSeconds = 0;
    float slowPercent = 0;


    int update_z_time = 0;
    int max_update_z_time = 1; //update zpos every x frames
    float oldz = 0;

    //there are navNode* called dest and current, but those
    //are low-level, and updated fickely.
    //destination will be pursued via pathfinding each frame, if set.
    navNode* Destination = nullptr;
    int usingDisabledNode = 0;
    int lastTargetDestIndex = -1;
    float angleToTarget = 0;




    //animation
    bool animate = false; //does the squash/stretch animation for walking, talking... everything really
    float animtime = 0; //time since having started animating
    float animspeed = 0;
    float animlimit = 0.5; // the extent to the animation. 0.5 means halfway

    bool growFromFloor = 1; //when entities shrink/grow, do they shrink 
                            //to the floor or to their center?
    float curwidth = 0;
    float curheight = 0;
    int sizeRestoreMs = 0; //this is set for boardables to recover their size if protag leaves
    float originalWidth = 0; //for shrink effect
    float originalHeight = 0; 
    bool shrinking = 0; //used to animate entities shrinking away
    bool turnToFacePlayer = true; //face player when talking
    bool useAnimForWalking = 0;
    int animWalkFrames = 0; //how long is his walk animation?
    SDL_RendererFlip flip = SDL_FLIP_NONE; //SDL_FLIP_HORIZONTAL; // SDL_FLIP_NONE
    int animationconfig = 0;

    float floatheight = 0; //how far up to float, worlditems use this to bounce
    int bounceindex = 0;

    int frame = 0; //current frame on SPRITESHEET
    int walkAnimMsPerFrame = 100; //how many msperframe to use for walk anim
    int msPerFrame = 0; //how long to wait between frames of animation, 0 being infinite time (no frame animation)
    int msTilNextFrame = 0; //accumulater, when it reaches msPerFrame we advance frame
    int frameInAnimation = 0; //current frame in ANIMATION
    bool loopAnimation = 1; //start over after we finish
    int animation = 4; //current animation, or the row of the spritesheet
    int lastDirection = 4; //so that entities don't switch between directional frames too often
    int directionUpdateCooldownMs = 0;
    static const int maxDirectionUpdateCooldownMs = 40;
    int defaultAnimation = 4;
    bool scriptedAnimation = 0; //0 means the character is animated based on movement. 1 means the character is animated based on a script.
    int reverseAnimation = 0; //step backwards with frames instead of forwards, and end
                              //on first frame instead of last if not looping

    int framewidth = 120; //width of single frame
    int frameheight = 120; //height of frame
    int xframes = 1; //number of frames ACROSS the spritesheet
    int yframes = 1; //number of frames DOWN the spritesheet
    vector<coord> framespots;
    bool up = 0;bool down = 0; bool left = 0; bool right = 0; //for chusing one of 8 animations for facing
    bool hadInput = 0; //had input this frame;
    int shooting = 0; //1 if character is shooting

    int opacity = 255; //opacity from 0 to 255, used for hiding shaded entities.
                       
    int opacity_delta = 0;

    //object-related design
    bool dynamic = true; //true for things such as wallcaps. movement/box is not calculated if this is false
    bool CalcDynamicForOneFrame = false; //set this to true to do dynamic calcs only for one frame
    vector<string> sayings;
    bool inParty = false;
    bool talks = false;
    bool wallcap = false; //used for wallcaps
    float shadowSize = 0;
    cshadow * shadow = 0;
    bool rectangularshadow = 0;
    bool isAI = 0;
    int lostHimSequence = 0; //this is for making entities react nicely to when the protag hides
    int lostHimX = 0;
    int lostHimY = 0;
    int lostHimMs = 0;
    statusComponent hisStatusComponent;
    int timeToLiveMs = 0;  //or ttl, time updated and when <=0 ent is destroyed
    bool usingTimeToLive = 0;
    
    bool usingVisibleMs = 0;
    int visibleMs = 0;
    

    //stuff for orbitals
    bool isOrbital = false;
    int orbitalIgnoreZ = false;
    entity* parent = nullptr;
    string parentName = "null";
    float angularPosition = 0;
    float angularSpeed = 10;
    float orbitRange = 1;
    int orbitOffset = 0; //the frames of offset for an orbital.
                         
    int identity = 0; //used for marking pellets, spiketraps, cannons, etc.

    //for pellets (identity == 1)
    bool wasPellet = 0; //this is true if something was ever a pellet

    //for special objects
    int specialState = 0; //flexible state field
    int cooldownA = 0;
    int cooldownB = 0;
    int cooldownC = 0;
    int cooldownD = 0;
    int flagA = 0;
    int flagB = 0;
    int flagC = 0;
    int flagD = 0;
    int maxCooldownA = 0;
    int maxCooldownB = 0;
    int maxCooldownC = 0;
    int maxCooldownD = 0;
    int lastState = 0;

   
    //for boarding
    bool isBoardable = 0;
    bool isHidingSpot = 0;
    string transportEnt = "";
    float transportRate = 0;
    entity* transportEntPtr = nullptr;
    bool usesBoardingScript = 0;
    string boardingScriptName = "";
    vector<string> boardingScript;

    //change pixel drawing method 
    bool blurPixelsForScaling = 0;
 
    vector<entity*> children;

    //for textured entities (e.g. wallcap)
    float xoffset = 0;
    float yoffset = 64;
    bool wall = 0; //to darken entwalls
    //float sortingOffset = 0; //to make entites float in the air, or sort as if they are higher or lower than they are first use was for building diagonal walls
    float extraYOffset = 0; //used to encode a permanent y axis offset, for diagonal walls
    //0 - slant left, 1 - slant right, 2 - pillar
    int effect = 0;
    bool asset_sharer = 0;

    //self-data
    int data[255] = {0};

    //combat
    weapon* hisweapon = 0;
    bool canFight = 1;
    bool invincible = 0;
    float invincibleMS = 0; //ms before setting invincible to 0
    
    bool useAgro = 0; //the switch from seeing a target and agroing on them (boring)
                                  //to gaining aggressiveness when seeing a target and losing it
                                  //when LOS is broken.
                                  //if useAgro = 1, the entity can be agrod and fight if it sees a target or sees a fight between a friend or a foe
                                  //otherwise, it's a horror-game behemoth who gets stronger by detecting the player
    
    //these control how a behemoth gains and loses aggressiveness
    //by LOS to their target
    float aggressiveness = 0;
    float aggressivenessGain = 0.0001; //per Ms
    float aggressivenessLoss = 0.0005;
    float aggressivenessNoiseGain = 0.0001;
    float minAggressiveness = 0;
    float maxAggressiveness = 100;
    float aggressivenessSpread = 0;

    bool agrod = 0; //are they fighting a target?
    float hearingRadius = 0;
    bool missile = 0; //should we directly pursue an entity like a missle?
    bool phasedMovement = 0; //do walls stop this ent?
    bool fragileMovement = 0; //do walls destroy this ent?
    bool stunned = 0;
    bool marked = 0;
    bool disabled = 0;
    bool enraged = 0;
    bool buffed = 0;
    bool poisoned = 0;
    bool healen = 0;
    float statusSlownPercent = 0;

    //for making a character flash when taking damage from poison
    int poisonFlickerFrames = 0;

    //these two values are for having enemies travel
    int poiIndex = 0;
    bool traveling = 0;
    //this is for the way that the entity will travel.
    //Roam means pick a random poi, go there, and then pick another random one, etc.
    //patrol means go through the nodes in a loop
    travelstyle myTravelstyle = roam;
    int currentPoiForPatrolling = 0;

    //this variable is set to one for one frame
    //if this entity has reached a poi and will start looking again for one.
    bool readyForNextTravelInstruction = 0;

    entity* target = nullptr; //who are they fighting?
    int targetFaction = -10; //what faction are they fighting at the moment? If agrod and they lose their target, they will pick a random visible target having this faction
    coord dcoord; //place where they want to stand
    int faction = 0; //0 is player, 1 is most enemies
    bool essential = 0; //if this entity dies in your party, does the game end?
    int flashingMS = 0; //ms to flash red after taking damage
    float darkenMs = 0;
    float darkenValue = 255;
    int spinningMS = 0; //have they initiated a dodge backwards recently
    int lastSpinFrame = 0; //used for animating characters whilst dodging

    //ability-system
    //enemies need a way to call their own scripts
    //without worrying about being interupted by the player
    //just don't have them use the dialog-box
    adventureUI* myScriptCaller = nullptr;

    adventureUI* contactScriptCaller = nullptr;

    //for fields
    bool usesContactScript = 0;
    int contactScriptWaitMS = 0;
    int curContactScriptWaitMS = 0;
    bool contactReadyToProc = 0;
    vector<string> contactScript;

    //adventureUI* myFieldScriptCaller = nullptr;

    vector<ability> myAbilities;

    entity* potentialTarget = nullptr;
   
    bool smellsPotentialTarget = 0;
    bool seesPotentialTarget = 0;
    bool hearsPotentialTarget = 0;
    



    float smellAgroRadius = -1;
    float maxSmellAgroMs = 1000; //time needed
    float smellAgroMs = 0;
                           
    float visionRadius = 0;
    float visionTime = 0; //time needed
    
    int deagroMs = 10000; //if the behemoth doesn't perceive the player for x ms, he will de-agro
    int c_deagroMs = 0;
    bool perceivingProtag = 0;

    float roamRate = 0;
    int minPatrolPerRoam = 2;
    int curPatrolPerRoam = 0;
    //roamRate == 0 -> never roam
    //roamRate == 1 -> always roam
    //roamRate == 0.2 -> 20% of the time, visit a random POI instead of the next in the sequence
    int patrolDirection = 0;
    //if going to a random node makes the behemoth turn around,
    //if we then start patrolling, patrol backwards

    //for behemoths who use agressiveness
    bool useStateSystem = 0;
    int activeState = 0;
    vector<state> states;


    int attackDamageMultiplier = 1;


    float seeAgroMs = 0;

    int customMovement = 0;
    // 0 - chase
    // 1 - precede
    // 2 - corner
    // 3 - random

    int movementTypeSwitchRadius = 64 * 5; //if closer than this to target, chase them, regardless of what custom movement setting is
    
    float velocityPredictiveFactor = 15; //this is used for selecting a current navnode

    //aiIndex is used for having AI use their own patrolPoints
    //and not someone-else's
    //use -1 for things which call scripts but don't have targets (fleshpit)
    //also used to support interactions between AI, so they know who is who
    int aiIndex = 0;

    //stats/leveling
    float level = 1; //affects damage
    float damagemultiplier = 0.2; // how much damage do you gain per level
    float maxhp = 2;
    float hp = 2;


    float dhpMS = 0; //how long to apply dhp (dot, healing)
    float dhp = 0;
    string weaponName = "";
    int cost = 1000; //cost to spawn in map and then xp granted to killer
    float cooldown = 0;  //anything about the usage of attacks needs to be moved here since the attack object

    float statusMS = 0; //how long to apply status before it becomes "none"
    float statusMag = 0; //magnitude of status

    //pathfinding
    float updateSpeed = 1500;
    float timeSinceLastUpdate = 0;
    navNode* dest = nullptr;
    navNode* current = nullptr;
    vector<navNode*> path;
    // !!! was 800 try to turn this down some hehe
    float dijkstraSpeed = 100; //how many updates to wait between calling dijkstra's algorithm
    float timeSinceLastDijkstra = -1;
    bool justLostLosToTarget = 0;
    bool pathfinding = 0;
    float maxDistanceFromHome = 1400;
    float range = 3;
    int stuckTime = 0; //time since ai is trying to go somewhere but isn't moving
    int maxStuckTime = 80; //frames waited before resolving stuckness
    float lastx = 0;
    float lasty = 0;

    int semisolid = 0; //push away close entities
    int storedSemisolidValue = 0; //semisolid has to be stored for ents that start as not solid but can later have values of 1 or 2

    //will it be pushed away from semisolid entities.
    int pushable = 0;

    //used for atomically lighting large entities, e.g stalkers
    bool large = 0;

    bool boxy = 0;

    //worlditem
    bool isWorlditem = 0;

    //used for setting the nararator
    bool persistentHidden = 0;

    //used for preserving the arms, but it can be used for other stuff
    bool persistentGeneral = 0;

    //misc scripting stuff
    bool banished = 0;
    bool solid = 0;
    bool canBeSolid = 0; //if the entity starts out as solid, which may be toggled
    bool semisolidwaittoenable = 0; //ignore collision with entities spawned at the player at first if not spawned during map loading
    // e.g. wards
    bool createdAfterMapLoad = 0; //stalker-enemies shouldn't be blocked by semisolid items created by the player
    bool navblock = 0; //disable overlapping navnodes when loaded into the level
    vector<navNode*> overlappedNodes; //a collection of navnodes that the entity overlaps. doors can disable the ones they overlap
    //when an entity is banished by a script, it will jump into the air and become intangible when its z velocity hits 0

    //default constructor is called automatically for children
    entity(); 

    //entity constructor
    entity(SDL_Renderer * renderer, string filename, float sizeForDefaults = 1); 

    //copy constructor
    //first intended for spawning in cannonballs
    entity(SDL_Renderer* renderer, entity* a); 

    //for worlditems, load the ent file but use a texture by name
    entity(SDL_Renderer * renderer, int idk,  string texturename); 


    ~entity(); 


    int getOriginX(); 
  
    int getOriginY(); 

    void setOriginX(float fx); 
  
    void setOriginY(float fy); 

    rect getMovedBounds(); 

    void solidify(); 

    void unsolidify(); 

    void shoot();

    //entity render function
    void render(SDL_Renderer * renderer, camera fcamera); 

    void move_up(); 

    void stop_verti(); 

    void move_down(); 

    void move_left(); 

    void stop_hori(); 

    void move_right(); 

    void shoot_up(); 

    void shoot_down(); 

    void shoot_left(); 

    void shoot_right(); 

    // !!! horrible implementation, if you have problems with pathfinding efficiency try making this not O(n)
    template <class T>
    T* Get_Closest_Node(vector<T*> array, int useVelocity = 0); 

    musicNode* Get_Closest_Node(vector<musicNode*> array, int useVelocity = 0); 

    //returns a pointer to a door that the player used
    //entity update
    virtual door* update(vector<door*> doors, float elapsed); 

    //all-purpose pathfinding function
    void BasicNavigate(navNode* ultimateTargetNode); 

    //I want something like BasicNavigate, but increase node cost in some way to 
    //penalize going near other entities
    //the idea is that two entities blocking the same route is a poor allocation of resources, 
    //for the behemoths as a team
    //really, it has nothing to do with distance, but rather the topology of the map (routes)

};

//search entity by name
entity* searchEntities(string fname, entity* caller); 

entity* searchEntities(string fname); 

//return list of tangible entities with the name
vector<entity*> gatherEntities(string fname); 

class levelNode {
public:
  string name = "unamed";
  string mapfilename = "static/maps/unset";
  string waypointname;
  SDL_Texture* sprite;

  bool locked = 1;
  bool hidden = 0; //levels can be hidden so it is impossible to return to them (?)
  
  //dungeon data
  int dungeonFloors = 0; //if >0, this counts as a dungeon for many things
  int dungeonGoldMs = -1; //get a gold color for the level by getting a time under this
  vector<string> behemoths; //which behemoths can spawn here
  int firstActiveFloor = 7; //garanteed behemoth encounter here
  int avgRestSequence = 5; //what is a typical gap between encounters
                         //with behemoths.
  int avgChaseSequence = 5; //when a behemoth is chasing the player, for how many floors do they persist before letting the player explore in peace


  int eyeStyle = 0;
  bool loadedEyes = 0;
  SDL_Texture* eyeTexture = nullptr;

  int mouthStyle = 0;
  bool loadedMouth = 0;
  SDL_Texture* mouthTexture = nullptr;

  int blinkCooldownMS = 0;

  static const int maxBlinkCooldownMS = 10000;
  static const int minBlinkCooldownMS = 2000;

  string music;
  string chasemusic;

  int darkness = 0;

  levelNode(string p3, string p4, string p5, SDL_Renderer * renderer, int fmouthStyle, int ffloors, int fgoldMs, vector<string> fbehemoths, int ffirstfloor, int frestlen, int fchaselen, string fmusic, string fchasemusic, int fdarkness);

  ~levelNode(); 

  SDL_Rect getEyeRect();
};


// levelSequence class.
// A levelSequence contains levelNodes
class levelSequence {
public:
  vector<levelNode*> levelNodes;
  
  //make a levelsequence given a filename, and look in the levelsequences folder
  levelSequence(string filename, SDL_Renderer * renderer);

  //add levels from a file
  void addLevels(string filename); 

};

int loadSave(); 

int writeSave(); 

int checkSaveField(string field); 

void writeSaveField(string field, int value); 

string readSaveStringField(string field); 

void writeSaveFieldString(string field, string value); 

class textbox {
  public:
    SDL_Surface* textsurface = 0;
    SDL_Texture* texttexture = 0;
    SDL_Color textcolor = { 155, 115, 115 };
    SDL_FRect thisrect = {0, 0, 50, 50};
    string content = "Default text.";
    TTF_Font* font = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool show = true;
    int align = 0;  //0 - left
                    //1 - right
                    //2 - center

    float fontsize = 0;

    int errorflag = 0;

    int dropshadow = 0; //use a tiny shadow for readability

    //used for drawing in worldspace
    float boxWidth = 50;
    float boxHeight = 50;
    float boxX = 0;
    float boxY = 0;
    float boxScale = 40;
    bool worldspace = false; //use worldspace or screenspace;
    bool blinking = 0;
    bool layer0 = 0;
    
    textbox(SDL_Renderer* renderer, const char* fcontent, float size, float fx, float fy, float fwidth); 

    ~textbox();

    void render(SDL_Renderer* renderer, int winwidth, int winheight); 

    void updateText(string content, float size, float fwidth, SDL_Color fcolor = g_textcolor, string fontstr = g_font);

};

class ui {
  public:
    bool assetSharer = 0;

    float x;
    float y;
    
    float targetx = -10; //for gliding to a position
    float targety = -10;
    float glideSpeed = 0.5;

    float targetwidth = -10; //added to animate the heart
    float widthGlideSpeed = 0.5;

    float width = 0.5;
    float height = 0.5;

    float friction = 0;

    float opacity = 25500; //out of 25500

    bool show = true;
    SDL_Surface* image;
    SDL_Texture* texture;

    string filename = "";

    bool mapSpecific = 0;

    //added this for the objective crosshair, I don't think I'll use it much for other stuff
    int frame = 0;
    int xframes = 1;
    int framewidth = 0;
    int frameheight = 0;
    int msPerFrame = 0; //set this to positive int to animate the UI element
    int msTilNextFrame = 0;

    //for 9patch
    bool is9patch = 0;
    int patchwidth = 256; //213
    float patchscale = 0.4;
    float patchfactor = 1;

    bool persistent = 0;
    int priority = 0; //for ordering, where the textbox has priority 0 and 1 would put it above
    bool layer0 = 0; //lowest possible layer of ui, render before layer0 text
    bool renderOverText = 0; //highest possible layer of ui, render after all text
                             //what a shitty system lol

    int worldspace = 0;

    int shrinkPixels = 0; //used for shrinking a ui element by an amount of pixels, usually in combination with some other element intended as a border
    float shrinkPercent = 0; //used for shrinking a ui element by an amount of pixels, usually in combination with some other element intended as a border

    float heightFromWidthFactor = 0; //set this to 0.5 or 1 and the height of the element will be held to that ratio of the width, even if the screen's ratio changes.

    bool dropshadow = 0;

    ui(SDL_Renderer * renderer, const char* ffilename, float fx, float fy, float fwidth, float fheight, int fpriority); 

    virtual ~ui(); 

    void render(SDL_Renderer * renderer, camera fcamera, float elapsed); 
};


class musicNode {
  public:
    Mix_Music* blip;
    string name = "empty";
    int x = 0;
    int y = 0;
    float radius = 1200;
    bool enabled = 1; 

    musicNode(string fileaddress, int fx, int fy); 

    ~musicNode(); 
};

class cueSound {
  public:
    Mix_Chunk* blip;
    string name = "empty";
    int x = 0;
    int y = 0;
    float radius = 1200; //radius is in worldpixels (64 = 1 block)
    bool played = 0;

    cueSound(string fileaddress, int fx, int fy, int fradius); 

    ~cueSound(); 
};

//play a sound by name at a position
void playSoundByName(string fname, float xpos, float ypos); 

//play a sound given a string of its name. just make sure there's a cue with the same name
void playSoundByName(string fname);

void playSoundAtPosition(int channel, Mix_Chunk *sound, int loops, int xpos, int ypos, float volume);

class waypoint {
  public:
    float x = 0;
    float y = 0;
    int z = 0;
    int angle = 0; //this is fed into convertFrameToAngle() to the ent that travels to the waypoint
    
    string name;
    waypoint(string fname, float fx, float fy, int fz, int fangle);

    ~waypoint(); 
};

class trigger {
  public:
    int x, y, z, width, height, zeight;
    string binding;
    vector<string> script;
    bool active = 1;

    string targetEntity = "protag"; //what entity will activate the trigger

    trigger(string fbinding, int fx, int fy, int fz, int fwidth, int fheight, int fzeight, string ftargetEntity);

    ~trigger(); 
};

class hitbox {
  public:
    float x = 0;
    float y = 0;
    float z = 0;
    rect bounds;
    int sleepingMS = 0; //active after sleepingMS elapse
    bool active = 0;
    int activeMS = 0; //ttl, decreases when hitbox is active
    int targetFaction = 0;
    int damage = 1;
    entity* parent = 0;

    hitbox();

    ~hitbox();

    rect getMovedBounds();
};

class listener {
  public:
    int x, y; //just for debugging
    string binding;
    int block = 0;
    int condition = 1;
    string entityName;
    vector<entity*> listenList;
    bool oneWay = 0; //does this become inactive after "hearing" the entity data once?

    vector<string> script;
    bool active = 1;
    listener(string fname, int fblock, int fcondition, string fbinding, int fx, int fy); 

    ~listener(); 

    int update(); 
};

class escapeUI {
  public:
    ui* ninePatch;
    ui* handMarker;
    ui* fingerMarker;

    ui* backButton;
    ui* bbNinePatch;

    vector<textbox*> optionTextboxes;

    int optionIndex = 0;

    float yStart = 0.35;
    float yEnd = 0.65;
    float xStart = 0.35;
    float xEnd = 0.6;

    float bbXStart = 0.8;
    float bbYStart = 0.05;
    float bbWidth = 0.10;

    int numLines;

    int positionOfCursor = 0;
    int cursorIsOnBackButton = 0;
    int minPositionOfCursor = 0;
    int maxPositionOfCursor = 0;
    bool modifyingValue = 0; //set to one when the user selects an option and begins tinkering it

    float fingerOffset = 0.025;
    float handOffset = 0.008;

    float markerWidth = 0.055;
    float markerFingerX = 0.70;
    float markerHandX = 0.70;

    float markerBBOffset = 0.04; //offset position of cursor when on the back button
    float markerBBOffsetY = 0.04; 
                                  
    const float maxVolume = 1;
    const float minVolume = 0;
    const float deltaVolume = 0.05;

    const float maxGraphics = 3;
    const float minGraphics = 0;
    const float deltaGraphics = 1;

    const float maxBrightness = 140;
    const float minBrightness = 60;
    const float deltaBrightness = 5;


    escapeUI(); 

    ~escapeUI(); 

    void show(); 

    void hide(); 

    void uiModifying(); 

    void uiSelecting(); 

};

//clear map
//CLEAR MAP
void clear_map(camera& cameraToReset);

//an item in the world, bouncing around to be picked up
class worldItem : public entity {
  public:

    //to make items bounce, store an int from 0 to 2
    //sine will be done every frame three times
    //and every item's z will be modified based on the
    // height of the sine corresponding to their index


    //make an entity from the file worlditem.ent
    worldItem(string fname, bool fisKeyItem);

    ~worldItem();
};


class settingsUI {
  public:
    ui* ninePatch;
    ui* handMarker;
    ui* fingerMarker;

    ui* backButton;
    ui* bbNinePatch;

    vector<textbox*> optionTextboxes;

    vector<textbox*> valueTextboxes;

    int optionIndex = 0;

    float yStart = 0.05;
    float yEnd = 0.9;
    float xStart = 0.3;
    float xEnd = 0.7;

    float bbXStart = 0.8;
    float bbYStart = 0.05;
    float bbWidth = 0.10;

    int numLines;

    int positionOfCursor = 0;
    int cursorIsOnBackButton = 0;
    int minPositionOfCursor = 0;
    int maxPositionOfCursor = 0;
    bool modifyingValue = 0; //set to one when the user selects an option and begins tinkering it

    float fingerOffset = 0.016;
    float handOffset = 0.008;

    float markerWidth = 0.055;
    float markerFingerX = 0.70;
    float markerHandX = 0.70;

    float markerBBOffset = 0.04; //offset position of cursor when on the back button
    float markerBBOffsetY = 0.04; 
                                  
    const float maxVolume = 1;
    const float minVolume = 0;
    const float deltaVolume = 0.05;

    const float maxGraphics = 3;
    const float minGraphics = 0;
    const float deltaGraphics = 1;

    const float maxBrightness = 140;
    const float minBrightness = 60;
    const float deltaBrightness = 5;


    settingsUI(); 

    ~settingsUI(); 

    void show(); 

    void hide(); 

    void uiModifying(); 

    void uiSelecting(); 
};


//I added this to help debug a problem with multiple copies of UI elements
void debugUI();

//this is for the braintrap
//with a texture and two points in 3d space, draw the texture stretching between the two points
// FOR EASE OF USE LOAD AN ENTITY AND COPY IT'S TEXTURE
class ribbon:public actor {
  public:
    float x1 = 0; float y1 = 0; float z1 = 0;
    float x2 = 0; float y2 = 0; float z2 = 0;

    int r_length = 0; //ribbon width
    int r_thickness = 0; //ribbon height

    ribbon();
    ~ribbon();
    void render(SDL_Renderer * renderer, camera fcamera);
};

struct dungeonBehemothInfo {
  int floorsRemaining = 0;
  entity* ptr;
  bool active = 0;
  int waitFloors = 0;
};

struct dungeonFloorInfo {
  string map;
  char identity; //1, 2, 3, r, s
};

class tallGrass {
public:
  rect bounds;
  tallGrass();
  ~tallGrass();
};


#endif
