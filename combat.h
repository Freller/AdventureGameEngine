#ifndef combat_h
#define combat_h

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <string>

class ui;
class textbox;

enum turn {
  PLAYER,
  ENEMY
};

//for handing menuing in turn based combat code
enum class submode {
  TEXT, //text box has focus, writing message to player
  MAIN, //player chooses between Fight, Items, Spirit, Defend, Run
  SPIRITCHOOSE, //player chooses which spirit move to use
  ITEMCHOOSE, //player chooses which item to use
  TARGETING, //player chooses which enemy to target
  CONTINUE, //go to next party member, or maybe to execute
  EXECUTE //take serialization and play out the player's and enemy's turns
};

enum class turnAction {
  ATTACK,
  SPIRITMOVE,
  ITEM,
  DEFEND
};

struct turnSerialization {
  int target;
  turnAction action;
  int actionIndex;
};

class combatUI {
public:
  ui* partyHealthBox = 0;
  textbox* partyText = 0;

  ui* mainPanel = 0;
  ui* dialogProceedIndicator = 0;
  textbox* mainText = 0;
  const int dpiDesendMs = 400;
  int c_dpiDesendMs = 0;
  int c_dpiAsendTarget = 0.9;
  bool c_dpiAsending = 0;
  const float dpiAsendSpeed = 0.0002;
  std::vector<std::string> options = {"Attack", "Spirit", "Bag", "Defend", "Run", "Auto"};
  std::string finalText = "";
  std::string currentText = "";
  std::vector<std::string> queuedStrings;

  ui* optionsPanel = 0;
  textbox* optionsText = 0;
  ui* menuPicker = 0;
  int currentOption = 0;

  ui* targetPanel = 0;
  textbox* targetText = 0;
  int currentTarget = 0;
  int targetingColorMod = 0;
  float tcm_accumulator = 0;


  combatUI(SDL_Renderer* renderer);

  ~combatUI();

  void hideAll();
};

void drawCombatants();

void getCombatInput();

void CombatLoop();

#endif
