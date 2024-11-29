#ifndef combat_h
#define combat_h

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <string>
#include <unordered_map>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 1024;
const float SPAWN_MARGIN = 50.0f;

using namespace std;

void loadPalette(SDL_Renderer* renderer, const char* filePath, std::vector<Uint32>& palette);

enum type {
  NONE,
  ANIMAL,
  PLANT,
  BUG,
  ROBOT,
  ALIEN,
  UNDEAD,
  GHOST,
  DEMON
};

enum class turnAction {
  ATTACK,
  SPIRITMOVE,
  ITEM,
  DEFEND
};

struct bground {
  int texture = 1;
  bool interleaved = 0;

  float horizontalIntensity = 150;
  float horizontalPeriod = 1000;
  float verticalIntensity = 25;
  float verticalPeriod = 50;
  float scrollXMagnitude = 100;
  float scrollYMagnitude = 100;
  std::vector<Uint32> palette = {};

  int texture2 = 1;
  bool interleaved2 = 0;
  float horizontalIntensity2 = 20;
  float horizontalPeriod2 = 20;
  float verticalIntensity2 = 55;
  float vertialPeriod2 = 10;
  float scrollXMagnitude2 = 120;
  float scrollYMagnitude2 = 130;
  std::vector<Uint32> palette2 = {};
  bground();
  bground(SDL_Renderer* renderer, const char* configFilePath);
  
};

struct turnSerialization {
  int target;
  turnAction action;
  int actionIndex;
};

class combatant {
public:

  std::string name;
  std::string filename;

  float baseAttack;
  float attackGain;

  float baseDefense;
  float defenseGain;

  float baseHealth;
  float healthGain;

  float baseCritical; //chance to dodge/crit
  float criticalGain;

  float baseSkill; //effectiveness of items
  float skillGain;

  float baseSoul; //power of spirit moves
  float soulGain;

  float baseMind; //max spirit points
  float mindGain;

  type myType;

  int xp;
  int level;

  std::string deathText;

  SDL_Texture * texture;
  float width;
  float height;

  int health;
  int maxHealth;

  int sp;
  int maxSp;

  //for drawing enemies
  int opacity = 0;
  SDL_Rect renderQuad = {-1,-1,-1,-1};

  turnSerialization serial;

  vector<int> inventory;
  vector<vector<int>> attackPatterns;

  int itemToUse = -1;

  vector<int> spiritMoves;

  combatant(string filename, int level);

  ~combatant();

};

struct itemInfo {
  std::string name = "";
  int targeting = 0; //0 - target enemy
                     //1 - target teammate
                     //2 - no target
  
  bool usedByWhom = -1; //Which party member is using this item
                        //used to stop displaying an item
                        //if another member is using it
  itemInfo(std::string a, int b);
  itemInfo();
};

struct spiritInfo {
  std::string name = "";
  int targeting = 0;
  int cost = 0;
  spiritInfo(std::string a, int b, int c);
  spiritInfo();
};

//extern std::vector<std::pair<int, std::string>> itemNamesTable;
extern std::unordered_map<int, itemInfo> itemsTable;

extern std::unordered_map<int, spiritInfo> spiritTable;

void spawnBullets(int pattern, int& accumulator);

void initTables();

void initCombat();

int xpToLevel(int xp);

void useItem(int item, int target, combatant* user);

class ui;
class textbox;

enum turn {
  PLAYER,
  ENEMY
};


type stringToType(const std::string& str);

//for handing menuing in turn based combat code
enum class submode {
  INWIPE,
  OUTWIPE,
  TEXT, //entry text box
  MAIN, //player chooses between Fight, Items, Spirit, Defend, Run
  SPIRITCHOOSE, //player chooses which spirit move to use
  ITEMCHOOSE, //player chooses which item to use
  TARGETING, //player chooses which enemy to target
  ALLYTARGETING, 
  CONTINUE, //go to next party member, or maybe to execute
  EXECUTE_P, //take serialization and play out the player's turns
  TEXT_P, //Feedback about player's turns
  EXECUTE_E, //play the enemies's turns
  TEXT_E, //Feedback about the enemies's turns
  FINAL,
  FINALTEXT, // Feedback about the battle
  SPWARNING,
  DODGING,
  RUNWARNING
};


class combatUI {
public:
  ui* partyHealthBox = 0;
  textbox* partyText = 0;
  textbox* partyMiniText = 0;

  ui* mainPanel = 0;
  ui* dialogProceedIndicator = 0;
  textbox* mainText = 0;
  const int dpiDesendMs = 400;
  int c_dpiDesendMs = 0;
  int c_dpiAsendTarget = 0.9;
  bool c_dpiAsending = 0;
  const float dpiAsendSpeed = 0.0002;
  std::vector<std::string> options = {"Attack", "Spirit", "Bag", "Shrink", "Run", "Auto"};
  std::string finalText = "";
  std::string currentText = "";
  std::vector<std::string> queuedStrings;

  ui* optionsPanel = 0;
  textbox* optionsText = 0;
  textbox* optionsMiniText = 0;
  ui* menuPicker = 0;
  int currentOption = 0;

  ui* targetPanel = 0;
  textbox* targetText = 0;
  int currentTarget = 0;
  int targetingColorMod = 0;
  float tcm_accumulator = 0;

  int executePIndex = 0;
  int executeEIndex = 0;

  ui* inventoryPanel = 0;
  textbox* inventoryText = 0;
  int currentInventoryOption = 0;

  ui* spiritPanel = 0;
  textbox* spiritText = 0;
  int currentSpiritOption = 0;

  float dodgePanelFullX;
  float dodgePanelFullY;
  float dodgePanelFullWidth;
  float dodgePanelFullHeight;
  float dodgePanelSmallX = 0.5;
  float dodgePanelSmallY = 0.5;
  float dodgePanelSmallWidth = 0;
  float dodgePanelSmallHeight = 0;
  ui* dodgePanel = 0;
  int incrementDodgeTimer = 0;
  int damageTakenFromDodgingPhase = 0;
  int damageFromEachHit = 0;
  int dodgeTimer = 0;
  const int maxDodgeTimer = 15000;
  int invincibleMs = 0;
  const int maxInvincibleMs = 1000;
  int blinkMs = 0;
  bool drawDodger = 1;
  float dodgerSpeed = 10;
  float aspect = 1;
  float dodgerX = 512;
  float dodgerY = 512;
  float dodgerWidth = 100;
  float dodgerHeight = 100;
  SDL_Texture* dodgerTexture = 0;
  SDL_Texture* rendertarget = 0;
  SDL_Texture* bulletTexture = 0;
  int accuA = 0;
  int accuB = 0;
  int accuC = 0;
  vector<int> curPatterns;

  vector<bool> dodgingThisTurn = {0, 0, 0, 0};
  bool shrink = 0;

  bground loadedBackground;
  SDL_Surface* sb1 = 0;
  SDL_Surface* db1 = 0;
  SDL_Texture* tb1 = 0;

  float time = 0.0f;
  float cycleTime = 0;
  combatant* partyDodgingCombatant = 0;

  combatUI(SDL_Renderer* renderer);

  ~combatUI();


  void hideAll();
};

void drawCombatants();

void getCombatInput();

void CombatLoop();

class miniEnt {
public:
  float x = 0;
  float y = 0;
  float w = 0;
  float h = 0;
  float velocity = 0;
  float acceleration = 0;
  float angle = 0;
  int red = 255, blue = 255, green = 255;
  bool homing = 0;
  int exploding = 0;
  bool exploded = 0;
  int explosionTimer = 0;
  int numFragments = 0;
  bool randomExplodeAngle = 0;
  bool completelyRandomExplodeAngle = 0;
  float fragSize = 1;

  float centerX = 512;
  float centerY = 512;
  float spinSpeed = 0;
  float spinAngle = 0;
  float radius = 0;

  bool isInPlayArea = 0;
  bool canBounce = 0;

  float gravityVX = 0;
  float gravityAccelX = 0;
  float gravityVY = 0;
  float gravityAccelY = 0;

  SDL_Texture* texture;
  void update(float elapsed);
  void render();
  miniEnt();
  virtual ~miniEnt();
};

class miniBullet:public miniEnt {
public:
  miniBullet(float f_angle, float f_velocity);
  miniBullet();
  ~miniBullet();
  void bulletUpdate(float elapsed);
  void explode(int numFragments, int exploding, float fragSize);
};

void drawBackground();



#endif
