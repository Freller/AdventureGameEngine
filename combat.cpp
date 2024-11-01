#include "combat.h"
#include "objects.h"
#include "main.h"
#include "utils.h"
#include <unordered_map>

combatant::combatant(string filename, int fxp) {
  string loadstr;
  loadstr = "resources/static/combatfiles/" + filename + ".cmb";
  istringstream file(loadTextAsString(loadstr));

  string temp;
  file >> temp;
  file >> temp;

  name = temp;

  file >> temp;
  file >> temp;


  string spritefilevar;
  spritefilevar = "resources/static/combatsprites/" + temp + ".qoi";
  const char* spritefile = spritefilevar.c_str();
  texture = loadTexture(renderer, spritefile);

  file >> temp;
  file >> temp;
  type = stringToType(temp);

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
    {"flying", FLYING},
    {"swimming", SWIMMING},
    {"robot", ROBOT},
    {"alien", ALIEN},
    {"undead", UNDEAD},
    {"ghost", GHOST}
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

void initTables() {
  {
    itemsTable[0] = itemInfo("Bandage", 1);
    itemsTable[1] = itemInfo("Bomb", 0);
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
      g_enemyCombatants[target]->health -= mag;
      string message = user->name + " hurt " + g_enemyCombatants[target]->name + " for " + to_string(mag) + ".";
      combatUIManager->finalText = message;
      combatUIManager->currentText = "";
      combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
      combatUIManager->dialogProceedIndicator->y = 0.25;
      combatant* e = g_enemyCombatants[target];
      if(e->health < 0) {
        string deathmessage = e->name + " " +  e->deathText;
        combatUIManager->queuedStrings.push_back(deathmessage);
        g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
      }
      break;
      
      break;
    }
    case 2:
    {
      //Glasses
      //raise damage of next spririt attack by 300%
      
      break;
    }
  }
}

void useSpiritMove(int spiritNumber, int rank, int target, combatant* user) {
  user->sp -= spiritTable[spiritNumber].cost;
  switch(spiritNumber) {
    case 19:
      {
        float baseDmg = 10 * rank;
        if(g_enemyCombatants[target]->type == type::SWIMMING) {
          baseDmg *= 2;
        }

        int mag = baseDmg * frng(0.70, 1.30) * (float)((float)user->baseSoul + ((float)user->soulGain * user->level));
        g_enemyCombatants[target]->health -= mag;
        string message = user->name + " hurt " + g_enemyCombatants[target]->name + " for " + to_string(mag) + ".";
        combatUIManager->finalText = message;
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
        combatUIManager->dialogProceedIndicator->y = 0.25;
        combatant* e = g_enemyCombatants[target];
        if(e->health < 0) {
          string deathmessage = e->name + " " +  e->deathText;
          combatUIManager->queuedStrings.push_back(deathmessage);
          g_enemyCombatants.erase(g_enemyCombatants.begin() + target);
        }
        break;
      }

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
  SDL_SetTextureBlendMode(rendertarget, SDL_BLENDMODE_BLEND);
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
}

void getCombatInput() {
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  SDL_PollEvent(&event);

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
  int gap = -0.01 * WIN_WIDTH; // Space between combatants
  
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
          g_submode == submode::SPWARNING) {
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

  drawCombatants();
  switch (g_submode) {
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
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
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

            break;
          }
          case 2:
          {
            //Bag
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::ITEM;

            combatUIManager->currentInventoryOption = 0;

            //now, choose a target
            g_submode = submode::ITEMCHOOSE;

            break;
          }
          case 3:
          {
            //Defend
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::DEFEND;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

            g_submode = submode::EXECUTE_P;
            break;
          }
          case 4:
          {
            //Run

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
        g_submode = submode::MAIN;
        combatUIManager->currentOption = 0;
        if(curCombatantIndex > 0) {
          curCombatantIndex--;
          combatUIManager->currentOption = 0;
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
      }


      break;
    }
    case submode::EXECUTE_P:
    {
      combatant* c = g_partyCombatants[combatUIManager->executePIndex];
      if(c->serial.action == turnAction::ATTACK) {
        int a = c->serial.target;
        int b = g_enemyCombatants.size();
        while(a >= b) {
          c->serial.target-= 1;
          a = c->serial.target;
        }
        if(g_enemyCombatants.size() == 0) {
          g_submode = submode::FINAL;
        } else {
          combatant* e = g_enemyCombatants[c->serial.target];
          int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
          damage *= frng(0.70,1.30);
          e->health -= damage;
          string message = c->name + " deals " + to_string(damage) + " to " + e->name + "!";
  
          if(e->health < 0) {
            string deathmessage = e->name + " " +  e->deathText;
            combatUIManager->queuedStrings.push_back(deathmessage);
            g_enemyCombatants.erase(g_enemyCombatants.begin() + c->serial.target);
          }
  
          combatUIManager->finalText = message;
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::TEXT_P;
        }
      } else if(c->serial.action == turnAction::ITEM) {
        combatant* com = g_partyCombatants[combatUIManager->executePIndex];
        int a = com->serial.actionIndex; //which item
        int b = com->serial.target; //which ally/enemy
        D(com->serial.actionIndex);
        //int c = itemsTable[com->itemToUse].targeting; //target allies or enemies or neither
        com->itemToUse = -1;

        useItem(a, b, com);

        g_submode = submode::TEXT_P;

      } else if(c->serial.action == turnAction::SPIRITMOVE) {
        combatant* com = g_partyCombatants[combatUIManager->executePIndex];
        int whichSpiritAbility = com->serial.actionIndex; //which spirit ability
        int spiritRank = com->serial.rank;
        int target = com->serial.target;
        useSpiritMove(whichSpiritAbility, spiritRank, target, com);
        g_submode = submode::TEXT_P;
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
          combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
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
        combatant* e = g_partyCombatants[rng(0, g_partyCombatants.size() - 1)];
        int damage = c->baseAttack + (c->attackGain * c->level) - (e->baseDefense + (e->defenseGain * e->level));
        damage *= frng(0.70,1.30);
        combatUIManager->damageFromEachHit = damage;
        combatUIManager->dodgePanel->x = combatUIManager->dodgePanelSmallX;
        combatUIManager->dodgePanel->y = combatUIManager->dodgePanelSmallY;
        combatUIManager->dodgePanel->width = combatUIManager->dodgePanelSmallWidth;
        combatUIManager->dodgePanel->height = combatUIManager->dodgePanelSmallHeight;
        combatUIManager->dodgePanel->show = 1;
        combatUIManager->incrementDodgeTimer = 0;
        combatUIManager->dodgeTimer = 0;
        combatUIManager->damageTakenFromDodgingPhase = 0;
        
        //g_submode = submode::DODGING;
        string message = c->name + " attacks " + e->name + " for " + to_string(damage) + " damage!";
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
          combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
          combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
        } else {
          g_submode = submode::DODGING;
          combatUIManager->bulletTimerAccumulator = 0;
          combatUIManager->dodgerX = 512;
          combatUIManager->dodgerY = 512;
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
      //calculate XP, etc.
      
      combatUIManager->currentText = "";
      combatUIManager->finalText = "Fomm has won the battle!";
      g_submode = submode::FINALTEXT;
      break;
    }
    case submode::FINALTEXT:
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
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {
            g_gamemode = gamemode::EXPLORATION;
            combatUIManager->hideAll();
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
        combatUIManager->spiritPanel->show = 0;
        combatUIManager->spiritText->show = 0;
      }

      if(input[11] && !oldinput[11] && g_combatInventory.size() > 0) {
        //does he have enough sp?
        int cost = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption].first].cost;
        int currentSp = g_partyCombatants[curCombatantIndex]->sp;
        if(currentSp < cost) {
          M("Not enough SP");
          combatUIManager->finalText = g_partyCombatants[curCombatantIndex]->name + " doesn't have enough SP!";
          combatUIManager->currentText = "";
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
          combatUIManager->dialogProceedIndicator->y = 0.25;
          g_submode = submode::SPWARNING;

        } else {
          switch(spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption].first].targeting) {
            case 0:
            {
              //enemy
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
              pair<int, int> spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
              int spiritNumber = spiritMove.first;
              int spiritRank = spiritMove.second;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
              g_partyCombatants[curCombatantIndex]->serial.rank = spiritRank;
  
              g_submode = submode::TARGETING;
              break;
            }
            case 1:
            {
              //teamate
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
              pair<int, int> spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
              int spiritNumber = spiritMove.first;
              int spiritRank = spiritMove.second;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
              g_partyCombatants[curCombatantIndex]->serial.rank = spiritRank;
  
              g_submode = submode::ALLYTARGETING;
              break;
            }
            case 2:
            {
              //none
              g_partyCombatants[curCombatantIndex]->serial.action = turnAction::SPIRITMOVE;
              pair<int, int> spiritMove = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
              int spiritNumber = spiritMove.first;
              int spiritRank = spiritMove.second;
              g_partyCombatants[curCombatantIndex]->serial.actionIndex = spiritNumber;
              g_partyCombatants[curCombatantIndex]->serial.rank = spiritRank;
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
      for(int i = 0; i < columns; i++) {
        for(int j = 0; j < rows; j++) {
          combatUIManager->spiritText->boxX = initialX + (i * width);
          combatUIManager->spiritText->boxY = initialY + (j * height);
          int itemIndex = g_combatInventory[index];
          string spiritName = "";
          spiritName = spiritTable[g_partyCombatants[curCombatantIndex]->spiritMoves[index].first].name;

          int spiritRank = g_partyCombatants[curCombatantIndex]->spiritMoves[index].second;
          string spiritRankStr = "";
          if(spiritRank == 1) {
            spiritRankStr = "I";
          } else if(spiritRank == 2) {
            spiritRankStr = "II";
          } else if(spiritRank == 3) {
            spiritRankStr = "III";
          } else if(spiritRank == 4) {
            spiritRankStr = "IV";
          } else if(spiritRank == 5) {
            spiritRankStr = "V";
          }
          combatUIManager->spiritText->updateText(spiritName + " " + spiritRankStr, -1, 0.85, g_textcolor, g_font);
          if(index == combatUIManager->currentInventoryOption) {
            combatUIManager->menuPicker->x = initialX + (i * width) - 0.035;
            combatUIManager->menuPicker->y = initialY + (j * height) + 0.005;
          }
          if(index < g_combatInventory.size()) {
            combatUIManager->spiritText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          }
     
     
          index++;
        }
      }
     
      combatUIManager->menuPicker->render(renderer, g_camera, elapsed);

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
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
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
        if(input[0]) {
          combatUIManager->dodgerY -= combatUIManager->dodgerSpeed;
        }
        if(input[1]) {
          combatUIManager->dodgerY += combatUIManager->dodgerSpeed;
        }
        if(input[2]) {
          combatUIManager->dodgerX -= combatUIManager->dodgerSpeed;
        }
        if(input[3]) {
          combatUIManager->dodgerX += combatUIManager->dodgerSpeed;
        }
        float margin = 50;
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

      if(combatUIManager->dodgeTimer > combatUIManager->maxDodgeTimer) {
        if(combatUIManager->executeEIndex + 1 ==  g_enemyCombatants.size()) {
          combatUIManager->mainPanel->show = 0;
          combatUIManager->mainText->show = 0;
          combatUIManager->dialogProceedIndicator->show = 0;
          g_submode = submode::MAIN;
          combatUIManager->currentOption = 0;
        } else {
          combatUIManager->executeEIndex++;
          g_submode = submode::EXECUTE_E;
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

    combatUIManager->bulletTimerAccumulator += elapsed;
    if(combatUIManager->bulletTimerAccumulator >= 1000) {
      combatUIManager->bulletTimerAccumulator = 0;
      for(int i = 0; i < 11; i++) {
        miniBullet* a = new miniBullet();
        a->texture = combatUIManager->bulletTexture;
      }
    }
    
    {
      for(auto x : g_miniEnts) {
        x->update(elapsed);
      }
      for(int i = 0; i < g_miniEnts.size(); i++) {
        if(g_miniEnts[i]->x < -SPAWN_MARGIN || g_miniEnts[i]->x > SCREEN_WIDTH + SPAWN_MARGIN ||
        g_miniEnts[i]->y < -SPAWN_MARGIN || g_miniEnts[i]->y > SCREEN_HEIGHT + SPAWN_MARGIN) {
          delete g_miniEnts[i];
          i--;
        }
      }
      for(auto x : g_miniEnts) {
        x->render();
        SDL_Rect brect;
        brect.x = x->x - x->w/2;
        brect.y = x->y - x->h/2;
        brect.w = x->w;
        brect.h = x->h;

        SDL_RenderCopy(renderer, combatUIManager->bulletTexture, NULL, &brect);

      }
      SDL_Rect drect;
      drect.x = combatUIManager->dodgerX - combatUIManager->dodgerWidth/2;
      drect.y = combatUIManager->dodgerY - combatUIManager->dodgerHeight/2;
      drect.w = combatUIManager->dodgerWidth;
      drect.h = combatUIManager->dodgerHeight;
      SDL_RenderCopy(renderer, combatUIManager->dodgerTexture, NULL, &drect);
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



  updateWindowResolution();

  SDL_RenderPresent(renderer);
}

miniEnt::miniEnt() {
  g_miniEnts.push_back(this);
}

miniEnt::~miniEnt() {
  g_miniEnts.erase(remove(g_miniEnts.begin(), g_miniEnts.end(), this), g_miniEnts.end());
}

void miniEnt::update(float elapsed) {
  x += velocity * cos(angle) * elapsed;
  y += velocity * sin(angle) * elapsed;
}

void miniEnt::render() {
  M("miniEnt::render()");
  SDL_Rect renderQuad = {
    (int)x - w/2,
    (int)y - h/2,
    (int)w,
    (int)h
  };
  SDL_RenderCopy(renderer, texture, NULL, &renderQuad);
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
  velocity = 0.5;
}

miniBullet::~miniBullet() {
  g_miniBullets.erase(remove(g_miniBullets.begin(), g_miniBullets.end(), this), g_miniBullets.end());
}
