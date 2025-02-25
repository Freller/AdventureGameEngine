#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <locale>

#include "physfs.h"

#include "globals.h"
#include "main.h"
#include "objects.h"
#include "map_editor.h"
#include "lightcookies.h"
#include "specialobjects.h"
#include "utils.h"
#include "combat.h"
#include "title.h"
#include "loss.h"

using namespace std;

void flushInput();

void getExplorationInput(float &elapsed);

void toggleDevmode();

void toggleFullscreen();

void protagMakesNoise();

void dungeonFlash();

SDL_FPoint calculateIntersection(float x1, float y1, float z1, float x2, float y2, float z2) {
    // Compute direction vector
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;

    // Scale direction vector to move to edge of the screen
    float maxDim = std::max(WIN_WIDTH*2, WIN_HEIGHT*2);
    float scaleFactor = maxDim / std::sqrt(dx * dx + dy * dy + dz * dz);

    SDL_FPoint intersection;
    intersection.x = x1 + dx * scaleFactor;
    intersection.y = y1 + dy * scaleFactor;

    return intersection;
}


void drawUI() {

  adventureUIManager->dialogpointer->render(renderer, g_camera);
  if((g_amState == amState::SPIRIT || g_amState == amState::SPIRITSELECT || g_amState == amState::STARGETING || g_amState == amState::ITARGETING || g_amState == amState::ITEM) && !protag_is_talking ) {
    drawCombatants();
  }



  //bottom-most layer of ui
  for (long long unsigned int i = 0; i < g_ui.size(); i++)
  {
    if(g_ui[i]->layer0) {
      g_ui[i]->render(renderer, g_camera, elapsed);
    }
  }

  for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
  {
    if(g_textboxes[i]->layer0) {
      g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    }
  }

  for (long long unsigned int i = 0; i < g_ui.size(); i++)
  {
    if(!g_ui[i]->renderOverText && !g_ui[i]->layer0) {
      g_ui[i]->render(renderer, g_camera, elapsed);
    }
  }

  for (long long unsigned int i = 0; i < g_textboxes.size(); i++)
  {
    if(!g_textboxes[i]->layer0) {
      g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    }
  }

  //some ui are rendered over text
  for (long long unsigned int i = 0; i < g_ui.size(); i++)
  {
    if(g_ui[i]->renderOverText) {
      g_ui[i]->render(renderer, g_camera, elapsed);
    }
  }

  if(g_amState == amState::SPIRITSELECT || g_amState == amState::STARGETING) {
    renderSpiritPanel();
  }

  if(g_amState == amState::ITEM || g_amState == amState::ITARGETING) {
    renderInventoryPanel();
  }

}

void updateWindowResolution() {
  // update camera
  SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);

  float w = WIN_WIDTH; float h = WIN_HEIGHT;
  // !!! it might be better to not run this every frame
  if (old_WIN_WIDTH != WIN_WIDTH || old_WIN_HEIGHT != WIN_HEIGHT || g_update_zoom)
  {
    g_update_zoom = 0;
    // user scaled window
    scalex = ((float)WIN_WIDTH / STANDARD_SCREENWIDTH) * g_defaultZoom * g_zoom_mod;
    scaley = ((float)WIN_HEIGHT / (STANDARD_SCREENWIDTH * 0.625)) * g_defaultZoom * g_zoom_mod;
    if (scalex < min_scale)
    {
      scalex = min_scale;
    }
    if (scalex > max_scale)
    {
      scalex = max_scale;
    }

    if(w / h > 1.6) {
      SDL_RenderSetScale(renderer, scaley * g_zoom_mod, scaley * g_zoom_mod);
    } else {
      SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
    }
    g_UiGlideSpeedY = 0.012 * WIN_WIDTH/WIN_HEIGHT;
  }

  old_WIN_WIDTH = WIN_WIDTH;
  old_WIN_HEIGHT = WIN_HEIGHT;

  if(w / h > 1.6) {
    WIN_HEIGHT /= scaley;
    WIN_WIDTH = WIN_HEIGHT * 1.6;

  } else {
    WIN_WIDTH /= scalex;
    WIN_HEIGHT = WIN_WIDTH * 0.625;
  }

  SDL_DestroyTexture(g_occluderTarget);
  g_occluderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH*g_occluderResolutionRatio, WIN_HEIGHT*g_occluderResolutionRatio);
  SDL_SetTextureBlendMode(g_occluderTarget, SDL_BLENDMODE_MOD);
}

void ExplorationLoop() {
  // cooldowns
  if(g_dungeonSystemOn) {g_dungeonMs += elapsed;}
  halfsecondtimer += elapsed;
  musicFadeTimer += elapsed;
  musicUpdateTimer += elapsed;
  g_blinkingMS += elapsed;
  g_jumpGaranteedAccelMs -= elapsed;
  g_boardingCooldownMs -= elapsed;
  g_protagBonusSpeedMS -= elapsed;
  if(g_protagBonusSpeedMS < 0) {protag->bonusSpeed = 0;}
  if(g_usingMsToStunned == 1) {
    g_protagMsToStunned -= elapsed;
    if(g_protagMsToStunned <= 0) {
      protag->hisStatusComponent.stunned.addStatus(3000, 1);
      g_usingMsToStunned = 0;
    }
  }
  if(g_protagIsWithinBoardable) { 
    g_msSinceBoarding += elapsed;

    if(g_msSinceBoarding < 500) {
      g_boardedEntity->curwidth = g_boardedEntity->width + 10 * abs(sin( ((float)g_msSinceBoarding)/50.0));
      g_boardedEntity->curheight = g_boardedEntity->height + 10 * abs(sin( ((float)g_msSinceBoarding)/50.0));
    } else {
      g_boardedEntity->sizeRestoreMs = 1000;
    }
  }

  if(g_behemoth0 != nullptr) {
    g_behemoth0->mobile = !protag_is_talking;
  }
  if(g_behemoth1 != nullptr) {
    g_behemoth1->mobile = !protag_is_talking;
  }
  if(g_behemoth2 != nullptr) {
    g_behemoth2->mobile = !protag_is_talking;
  }
  if(g_behemoth3 != nullptr) {
    g_behemoth3->mobile = !protag_is_talking;
  }

  // g_dash_cooldown -= elapsed;

  if(g_blinkingMS >= g_maxBlinkingMS) {
    g_blinkHidden = !g_blinkHidden;
    g_blinkingMS = g_blinkHidden? g_maxBlinkingMS * 0.9 : 0;
  }

  if (inPauseMenu || g_inSettingsMenu)
  {
    // if we're paused, freeze gametime
    elapsed = 0;
  }

  specialObjectsOncePerFrame(elapsed);

  // INPUT
  getExplorationInput(elapsed);


  //lerp protag to boarded ent smoothly
  if(g_protagIsWithinBoardable) {
    if(g_transferingByBoardable) {
      g_transferingByBoardableTime += elapsed;
      if(g_transferingByBoardableTime >= g_maxTransferingByBoardableTime) {
        g_transferingByBoardable = 0;
      }
      float px = g_formerBoardedEntity->getOriginX();
      float py = g_formerBoardedEntity->getOriginY();
      float tx = g_boardedEntity->getOriginX();
      float ty = g_boardedEntity->getOriginY();
      float completion = g_transferingByBoardableTime / g_maxTransferingByBoardableTime;
      protag->setOriginX( px * (1-completion) + tx *(completion) );
      protag->setOriginY( py * (1-completion) + ty *(completion) );

    } else {
      //this is just a visual effect
      float px = protag->getOriginX();
      float py = protag->getOriginY();
      float tx = g_boardedEntity->getOriginX();
      float ty = g_boardedEntity->getOriginY();
      protag->setOriginX( (px + tx)/2 );
      protag->setOriginY( (py + ty)/2);
      //protag->z = (protag->z + g_boardedEntity->z) /2;
    }

  }

  {
    // Adventure Menu
    if(input[12] && !oldinput[12]) {
      if(!inPauseMenu && !g_inSettingsMenu && !protag_is_talking) {
        D(int(g_amState));
        if(g_amState == amState::CLOSED) {
          g_amState = amState::MAJOR;
          adventureUIManager->keyPrompting = 0;
          adventureUIManager->showAm();
          adventureUIManager->amIndex = 0;
        }
        else if(g_amState == amState::MAJOR) {
          g_amState = amState::CLOSED;
          adventureUIManager->hideAm();
        }
      }
    }

    adventureUIManager->hideKi();
    adventureUIManager->hideSt();
    switch(g_amState) {
      case amState::MAJOR:
        {
          if(input[8] && !oldinput[8] && !protag_is_talking) {
            g_amState = amState::CLOSED;
            adventureUIManager->hideAm();
            oldinput[8] = 1;
            break;
          }
          if(input[0] && !oldinput[0] && !protag_is_talking) {
            if(adventureUIManager->amIndex == 1
                || adventureUIManager->amIndex == 3
                || adventureUIManager->amIndex == 5) {
              adventureUIManager->amIndex -= 1;
            }
          }
          if(input[1] && !oldinput[1] && !protag_is_talking) {
            if(adventureUIManager->amIndex == 0
                || adventureUIManager->amIndex == 2
                || adventureUIManager->amIndex == 4) {
              adventureUIManager->amIndex += 1;
            }
          }
          if(input[2] && !oldinput[2] && !protag_is_talking) {
            if(adventureUIManager->amIndex - 2 >= 0) {
              adventureUIManager->amIndex -= 2;
            }
          }
          if(input[3] && !oldinput[3] && !protag_is_talking) {
            if(adventureUIManager->amIndex + 2 <= 5) {
              adventureUIManager->amIndex += 2;
            }
          }
          adventureUIManager->amPicker->x = adventureUIManager->amTexPos[adventureUIManager->amIndex].first - 0.028;
          adventureUIManager->amPicker->y = adventureUIManager->amTexPos[adventureUIManager->amIndex].second + 0.007;
          if(input[11] && !oldinput[11] && !protag_is_talking) {
            switch(adventureUIManager->amIndex) {
              case 0:
                {
                  g_amState = amState::KEYITEM;
                  adventureUIManager->kiIndex = 0;
                  adventureUIManager->kiOffset = 0;
                  break;
                }
              case 1:
                {
                  g_amState = amState::ITEM;
                  combatUIManager->inventoryPanel->show = 1;
                  combatUIManager->inventoryText->show = 1;
                  combatUIManager->currentInventoryOption = 0;
                  combatUIManager->menuPicker->show = 1;
                  combatUIManager->menuPicker->x = 10;
                  combatUIManager->partyText->show = 1;
                  combatUIManager->partyMiniText->show = 1;
                  break;
                }
              case 2:
                {
                  g_amState = amState::STATUS;
                  adventureUIManager->stIndex = 0;
                  break;
                }
              case 3:
                {
                  g_amState = amState::SPIRIT;
                  combatUIManager->partyText->show = 1;
                  combatUIManager->partyMiniText->show = 1;
                  oldinput[11] = 1;
                  curCombatantIndex = 0;
                  break;
                }
              case 4:
                {
                  //help
                  vector<string> helpScript = {};
                  adventureUIManager->talker = narrarator;

                  //keep trying to get language data until it fails
                  int i = 0;
                  for(;;) {
                    string arg = "Help" + to_string(i) + "-" + g_mapdir + "/" + g_map;
                    string resp = getLanguageData(arg);
                    if(resp == "") {break;}
                    helpScript.push_back(resp);
                    i++;
                    if(i > 40) {
                      E("Stuck trying to pull dialog for help");
                      abort();
                    }
                    breakpoint();
                  }
                  if(helpScript.size() == 0) {
                    helpScript.push_back(getLanguageData("NoHelp"));
                  }
                  helpScript.push_back("#");

                  adventureUIManager->ownScript = helpScript;
                  adventureUIManager->dialogue_index = -1;
                  adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
                  adventureUIManager->sleepingMS = 0;
                  protag_is_talking = 1;
                  g_forceEndDialogue = 0;
                  g_keyItemFlavorDisplay = 1;
                  adventureUIManager->continueDialogue();

                  break;
                }
              case 5:
                {
                  //I think that the text from wifechat will not vary by map,
                  //but based on story progression, so scripts will have some way
                  //of updating the wifechatindex, which will be used to pull text
                  if(party.size() > 1 && party[1]->name == "common/neheten") {
                    //wife
                    vector<string> helpScript = {};
                    adventureUIManager->talker = party[1];

                    //keep trying to get language data until it fails
                    int i = 0;
                    int wifeValue = checkSaveField("wifeValue");
                    for(;;) {
                      string arg = "Wife" + to_string(wifeValue) + "-" + to_string(i);
                      string resp = getLanguageData(arg);
                      if(resp == "") {break;}
                      helpScript.push_back(resp);
                      i++;
                      if(i > 40) {
                        E("Stuck trying to pull dialog for help");
                        abort();
                      }
                      breakpoint();
                    }
                    if(helpScript.size() == 0) {
                      helpScript.push_back("");
                      E("Couldn't find wifescript");
                      abort();
                    }
                    helpScript.push_back("#");

                    adventureUIManager->ownScript = helpScript;
                    adventureUIManager->dialogue_index = -1;
                    adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
                    adventureUIManager->sleepingMS = 0;
                    protag_is_talking = 1;
                    g_forceEndDialogue = 0;
                    g_keyItemFlavorDisplay = 1;
                    adventureUIManager->continueDialogue();

                  }
                  break;

                }

            }
          }
          break;
        }
      case amState::KEYITEM:
        {

          adventureUIManager->showKi();


          if(input[8] && !oldinput[8] && !g_keyItemFlavorDisplay) {
            if(adventureUIManager->keyPrompting) {
              if(adventureUIManager->typing == 0) {
                adventureUIManager->response_index = -1; //give nothing
                adventureUIManager->keyPrompting = 2;
                g_amState = amState::CLOSED;
                adventureUIManager->hideKi();
                adventureUIManager->continueDialogue();
                break;
              }
            } else {
              g_amState = amState::MAJOR;
            }
          }
          if(input[11] && !oldinput[11] && !g_keyItemFlavorDisplay) {
            if(adventureUIManager->keyPrompting) {
              if(adventureUIManager->typing == 0) {
                if(adventureUIManager->kiIndex >= 0 && adventureUIManager->kiIndex < (int)g_keyItems.size()) {
                  adventureUIManager->response_index = g_keyItems[adventureUIManager->kiIndex]->index;
                  adventureUIManager->keyPrompting = 2;
                  g_amState = amState::CLOSED;
                  adventureUIManager->hideKi();
                  adventureUIManager->continueDialogue();
                  break;

                } else {
                  adventureUIManager->response_index = -1; //give nothing
                  adventureUIManager->keyPrompting = 2;
                  g_amState = amState::CLOSED;
                  adventureUIManager->hideKi();
                  breakpoint();
                  adventureUIManager->continueDialogue();
                  break;

                }
              }
            } else {
              if(adventureUIManager->kiIndex < g_keyItems.size()) {
                //show flavortext
                adventureUIManager->talker = narrarator;
                vector<string> flavorScript = {};
                flavorScript.push_back(getLanguageData("KeyItem" + to_string(g_keyItems[adventureUIManager->kiIndex]->index) + "Flavor"));
                flavorScript.push_back("#");
                adventureUIManager->ownScript = flavorScript;
                adventureUIManager->dialogue_index = -1;
                adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
                adventureUIManager->sleepingMS = 0;
                g_forceEndDialogue = 0;
                g_keyItemFlavorDisplay = 1;
                adventureUIManager->continueDialogue();
                oldinput[11] = 1;
              }
            }
          }

          if(input[0] && !g_keyItemFlavorDisplay) {
            if(SoldUIUp <= 0) {
              if(adventureUIManager->kiIndex -1 >= 0) {
                adventureUIManager->kiIndex--;
              }
              SoldUIUp = (oldUIUp) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
            } else {
              oldUIUp = 1;
            }
          } else {
            oldUIUp = 0;
            SoldUIUp = 0;
          }

          if(input[1] && !g_keyItemFlavorDisplay) {
            if(SoldUIDown <= 0) {
              if(adventureUIManager->kiIndex + 1< g_keyItems.size()) {
                adventureUIManager->kiIndex++;
              }
              SoldUIDown = (oldUIDown) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
            } else {
              oldUIDown = 1;
            }
          } else {
            oldUIDown = 0;
            SoldUIDown = 0;
          }

          if(adventureUIManager->kiIndex > adventureUIManager->kiOffset + 5) {
            adventureUIManager->kiOffset++;
          }
          if(adventureUIManager->kiIndex < adventureUIManager->kiOffset) {
            adventureUIManager->kiOffset--;
          }

          if(adventureUIManager->kiOffset > 0) {
            adventureUIManager->kiPrecede->show = 1;
          } else {
            adventureUIManager->kiPrecede->show = 0;
          }

          int a = g_keyItems.size();
          int b = 6 + adventureUIManager->kiOffset;
          if(a > b) {
            adventureUIManager->kiAdvance->show = 1;
          } else {
            adventureUIManager->kiAdvance->show = 0;
          }

          for(int i = 0; i < 6; i++) {
            if(i < adventureUIManager->kiTextboxes.size()) {
              int j = i + adventureUIManager->kiOffset;
              if(j < g_keyItems.size()) {
                if(j == adventureUIManager->kiIndex) {
                  adventureUIManager->kiPicker->y = adventureUIManager->kiTextboxes[i]->boxY + 0.0075;
                }
                adventureUIManager->kiTextboxes[i]->updateText(g_keyItems[j]->name);
                adventureUIManager->kiIcons[i]->texture = g_keyItems[j]->texture;
                if(adventureUIManager->kiIcons[i]->texture == 0) {
                  adventureUIManager->kiIcons[i]->show = 0;
                }
                adventureUIManager->kiIcons[i]->show = 1;
              } else {
                adventureUIManager->kiTextboxes[i]->updateText("");
                adventureUIManager->kiIcons[i]->show = 0;

              }
            }
          }

          if(g_keyItems.size() == 0) {
            adventureUIManager->kiPicker->show = 0;
          }

          break;
        }
      case amState::ITEM:
        {
          if(input[8] && !oldinput[8]&& !protag_is_talking) {
            g_amState = amState::MAJOR;
            combatUIManager->inventoryPanel->show = 0;
            combatUIManager->inventoryText->show = 0;
            combatUIManager->menuPicker->show = 0;
            combatUIManager->partyText->show = 0;
            combatUIManager->partyMiniText->show = 0;
          }
          if(input[0] && !oldinput[0]&& !protag_is_talking) {
            if(combatUIManager->currentInventoryOption != 0 &&
                combatUIManager->currentInventoryOption != 7) {
              combatUIManager->currentInventoryOption --;
            }
          }

          if(input[1] && !oldinput[1]&& !protag_is_talking) {
            if(combatUIManager->currentInventoryOption != 6 &&
                combatUIManager->currentInventoryOption != 13) {
              if(combatUIManager->currentInventoryOption + 1 < g_combatInventory.size()) {
                combatUIManager->currentInventoryOption ++;
              }
            }
          }

          if(input[2] && !oldinput[2]&& !protag_is_talking) {
            if(combatUIManager->currentInventoryOption >= 7) {
              combatUIManager->currentInventoryOption -= 7;
            }
          }

          if(input[3] && !oldinput[3]&& !protag_is_talking) {
            if(combatUIManager->currentInventoryOption <= 6) {
              combatUIManager->currentInventoryOption += 7;
            }
          }
          combatUIManager->currentInventoryOption = clamp(combatUIManager->currentInventoryOption, 0, g_combatInventory.size()-1);
          if(input[11] &&!oldinput[11]&& !protag_is_talking) {
            if(combatUIManager->currentInventoryOption >= 0 && combatUIManager->currentInventoryOption < g_combatInventory.size()) {
              if(itemsTable[g_combatInventory[combatUIManager->currentInventoryOption]].targeting == 1) {
                g_amState = amState::ITARGETING;
                combatUIManager->currentTarget = 0;
                combatUIManager->partyText->show = 1;
                combatUIManager->partyMiniText->show = 1;

                break;
              } else {
                vector<string> spiritScript = {};
                adventureUIManager->talker = narrarator;
                spiritScript.push_back(getLanguageData("ItemError"));
                spiritScript.push_back("#");

                adventureUIManager->ownScript = spiritScript;
                adventureUIManager->dialogue_index = -1;
                adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
                adventureUIManager->sleepingMS = 0;
                protag_is_talking = 1;
                g_keyItemFlavorDisplay = 1; //really just means make sure we dont use the input from the dialog ending to start another one
                g_forceEndDialogue = 0;
                adventureUIManager->continueDialogue();

              }
            }
          }
          break;
        }
      case amState::ITARGETING:
        {
          if(input[8] && !oldinput[8]) {
            g_amState = amState::ITEM;
          }
          if(input[2] && !oldinput[2] && !protag_is_talking) {
            combatUIManager->currentTarget--;
          }
          if(input[3] && !oldinput[3] && !protag_is_talking) {
            combatUIManager->currentTarget++;
          }
          combatUIManager->currentTarget = clamp(combatUIManager->currentTarget, 0, g_partyCombatants.size()-1);
          if(input[11] && !oldinput[11]) {
            combatant *c = g_partyCombatants[0];
            for(auto x : g_partyCombatants) {
              if(x->baseSkill > c->baseSkill) {
                c = x;
              }
            }
            useItem(g_combatInventory[combatUIManager->currentInventoryOption], combatUIManager->currentTarget, c);
            g_combatInventory.erase(g_combatInventory.begin() + combatUIManager->currentInventoryOption);
            combatUIManager->currentInventoryOption = clamp(combatUIManager->currentInventoryOption, 0, g_combatInventory.size()-1);
            g_amState = amState::ITEM;
          }
          break;
        }
      case amState::STATUS:
        {
          adventureUIManager->showSt();
          if(input[8] && !oldinput[8]) {
            g_amState = amState::MAJOR;
          }

          if(input[2] && !oldinput[2]) {
            if(adventureUIManager->stIndex - 1 >= 0) {
              adventureUIManager->stIndex -= 1;
            }
          }
          if(input[3] && !oldinput[3]) {
            if(adventureUIManager->stIndex + 1 < g_partyCombatants.size()) {
              adventureUIManager->stIndex += 1;
            }
          }

          combatant* c = g_partyCombatants[adventureUIManager->stIndex];
          int lastXP = levelToXp(c->level);
          int nextXP = levelToXp(c->level+1);
          int percentToNxt = 100*(c->xp - lastXP)/(nextXP - lastXP);

          int usI = 0;
          int usv = 0;
          string cname = "";
          if(c->filename == "common/fomm") {
            usI = 0;
            usv = (int)c->level * 1.3;
            cname = getLanguageData("ProtagManCentered");
          } else if(c->filename == "common/neheten") {
            usI = 1;
            usv = abs(sin((c->level *M_PI *36)/100)) *7 + 4;
            cname = getLanguageData("ProtagWifeCentered");
          } else if(c->filename == "common/blish") {
            usI = 2;
            usv = (int)c->level * 0.8;
            cname = getLanguageData("ProtagFriendCentered");
          } else if(c->filename == "common/dafua") {
            usI = 3;
            usv = (int)c->level * 1.1;
            cname = getLanguageData("ProtagDaughterCentered");
          }

          string precede = " ";
          string procede = " ";

          if(adventureUIManager->stIndex > 0) {
            precede = "    <";
          }

          if(adventureUIManager->stIndex != g_partyCombatants.size()-1) {
            procede = "    >";
          }
          string uselessStat = getLanguageData("UselessStat" + to_string(usI));
          adventureUIManager->stTextbox3->updateText(precede);
          adventureUIManager->stTextbox4->updateText(procede);

          adventureUIManager->stTextbox->updateText(
              "    " + cname +"\n\n"
              +
              "HP: " + to_string(c->health) + "/" + to_string(c->baseStrength) + "\n"
              +
              "SP: " + to_string(c->sp) + "/" + to_string(c->baseStrength) + "\n"
              +
              "Attack: " + to_stringF((int)c->baseAttack)+ "\n"
              +
              "Defense: " + to_stringF((int)c->baseDefense)+ "\n"
              +
              "Soul: " + to_stringF((int)c->baseSoul)+ "\n"
              +
              "Critical: " + to_stringF((int)c->baseCritical)+ "\n"
              +
              "Skill: " + to_stringF((int)c->baseSkill)+ "\n"
              +
              "Recovery: " + to_stringF((int)c->baseRecovery)+ "\n"
              +
              uselessStat + ": " + to_string(usv)+ "\n"
              );
          string s1 = "-";
          string s2 = "-";
          string s3 = "-";
          string s4 = "-";

          if(c->spiritMoves[0] >= 0) {
            s1 = spiritTable[c->spiritMoves[0]].name;
          }
          if(c->spiritMoves[1] >= 0) {
            s2 = spiritTable[c->spiritMoves[1]].name;
          }
          if(c->spiritMoves[2] >= 0) {
            s3 = spiritTable[c->spiritMoves[2]].name;
          }
          if(c->spiritMoves[3] >= 0) {
            s4 = spiritTable[c->spiritMoves[3]].name;
          }

          adventureUIManager->stTextbox2->updateText("Level " + to_string(c->level) + " (" + to_string(percentToNxt) + "% to next)\n\n"
              +
              "Spirit-moves:\n"
              +
              s1 + "\n"
              +
              s2 + "\n"
              +
              s3 + "\n"
              +
              s4 + "\n\n"
              +
              "Last fight:\n"
              + 
              "Damage Dealt: " + to_string((int)c->dmgDealtOverFight) + "\n"
              +
              "Damage Taken: " + to_string((int)c->dmgTakenOverFight) + "\n"


              );

          break;
        }
      case amState::SPIRIT:
        {
          if(input[8] && !oldinput[8]) {
            g_amState = amState::MAJOR;
            combatUIManager->partyText->show = 0;
            combatUIManager->partyMiniText->show = 0;
          }

          if(input[2] && !oldinput[2]) {
            curCombatantIndex--;
          }
          if(input[3] && !oldinput[3]) {
            curCombatantIndex++;
          }
          if(curCombatantIndex < 0) curCombatantIndex = 0;
          if(curCombatantIndex >= g_partyCombatants.size()) curCombatantIndex = g_partyCombatants.size()-1;

          if(input[11] && !oldinput[11]) {
            g_amState = amState::SPIRITSELECT;
            combatUIManager->spiritPanel->show = 1;
            combatUIManager->spiritText->show = 1;
            combatUIManager->menuPicker->show = 1;
            combatUIManager->menuPicker->x = 10;
            combatUIManager->currentInventoryOption = 0;


          }

          break;
        }
      case amState::SPIRITSELECT:
        {
          if(protag_is_talking) {
            combatUIManager->partyText->show = 0;
            combatUIManager->partyMiniText->show = 0;
          } else {
            combatUIManager->partyText->show = 1;
            combatUIManager->partyMiniText->show = 1;
          }
          if(input[8] && !oldinput[8] && !protag_is_talking) {
            g_amState = amState::SPIRIT;
            combatUIManager->spiritPanel->show = 0;
            combatUIManager->spiritText->show = 0;
            combatUIManager->menuPicker->show = 0;
          }
          if(input[0] && !oldinput[0] && !protag_is_talking) {
            combatUIManager->currentInventoryOption--;
          }
          if(input[1] && !oldinput[1] && !protag_is_talking) {
            combatUIManager->currentInventoryOption++;
          }
          if(combatUIManager->currentInventoryOption < 0) {
            combatUIManager->currentInventoryOption = 0;
          }
          if(combatUIManager->currentInventoryOption >= g_partyCombatants[curCombatantIndex]->spiritMoves.size()) {
            combatUIManager->currentInventoryOption = g_partyCombatants[curCombatantIndex]->spiritMoves.size()-1;
          }
          if(input[11] && !oldinput[11] && !protag_is_talking) {
            int spiritIndex = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
            int targeting = spiritTable[spiritIndex].targeting;
            if(targeting == 1) {
              combatUIManager->currentTarget = 0;
              g_amState = amState::STARGETING;
            } else {
              vector<string> spiritScript = {};
              adventureUIManager->talker = narrarator;
              spiritScript.push_back(getLanguageData("SpiritError"));
              spiritScript.push_back("#");

              adventureUIManager->ownScript = spiritScript;
              adventureUIManager->dialogue_index = -1;
              adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
              adventureUIManager->sleepingMS = 0;
              protag_is_talking = 1;
              g_keyItemFlavorDisplay = 1; //really just means make sure we dont use the input from the dialog ending to start another one
              g_forceEndDialogue = 0;
              adventureUIManager->continueDialogue();
            }

          }

          break;
        }
      case amState::STARGETING:
        {
          if(protag_is_talking) {
            combatUIManager->partyText->show = 0;
            combatUIManager->partyMiniText->show = 0;
          } else {
            combatUIManager->partyText->show = 1;
            combatUIManager->partyMiniText->show = 1;
          }
          if(input[8] && !oldinput[8] && !protag_is_talking ) {
            g_amState = amState::SPIRITSELECT;
            combatUIManager->menuPicker->x = 10;
          }
          if(input[2] && !oldinput[2] && !protag_is_talking) {
            combatUIManager->currentTarget--;
          }
          if(input[3] && !oldinput[3] && !protag_is_talking) {
            combatUIManager->currentTarget++;
          }
          if(input[11] && !oldinput[11] && !protag_is_talking) {
            //move must be ally-targeted
            int spiritIndex = g_partyCombatants[curCombatantIndex]->spiritMoves[combatUIManager->currentInventoryOption];
            //int targeting = spiritTable[spiritIndex].targeting;
            int cost = spiritTable[spiritIndex].cost;

            if(g_partyCombatants[curCombatantIndex]->sp >= cost) {
              useSpiritMove(spiritIndex, combatUIManager->currentTarget, g_partyCombatants[curCombatantIndex]);
              //            vector<string> spiritScript = {};
              //            adventureUIManager->talker = narrarator;
              //            spiritScript.push_back(combatUIManager->finalText);
              //            spiritScript.push_back("#");
              //
              //            adventureUIManager->ownScript = spiritScript;
              //            adventureUIManager->dialogue_index = -1;
              //            adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
              //            adventureUIManager->sleepingMS = 0;
              //            protag_is_talking = 1;
              //            g_keyItemFlavorDisplay = 1; //really just means make sure we dont use the input from the dialog ending to start another one
              //            g_forceEndDialogue = 0;
              //            adventureUIManager->continueDialogue();

            }
          }
          if(combatUIManager->currentTarget < 0) combatUIManager->currentTarget = 0;
          if(combatUIManager->currentTarget >= g_partyCombatants.size()) combatUIManager->currentTarget = g_partyCombatants.size()-1;
          break;
        }
    }
  }

  //position dialog pointer
  {
    if(adventureUIManager->talker != nullptr) {

      adventureUIManager->dialogpointer->x1 = adventureUIManager->talker->getOriginX();
      adventureUIManager->dialogpointer->y1 = adventureUIManager->talker->getOriginY();
      transform3dPoint(adventureUIManager->dialogpointer->x1, adventureUIManager->dialogpointer->y1, adventureUIManager->talker->z + adventureUIManager->talker->bounds.zeight, adventureUIManager->dialogpointer->x1, adventureUIManager->dialogpointer->y1);
      adventureUIManager->dialogpointer->x1 /= WIN_WIDTH;
      adventureUIManager->dialogpointer->y1 /= WIN_HEIGHT;

      if(adventureUIManager->dialogpointer->x1 > 1 
          || adventureUIManager->dialogpointer->x1 < 0
          || adventureUIManager->dialogpointer->y1 < 0
          || adventureUIManager->dialogpointer->y1 > 0.6)
      {
        adventureUIManager->dialogpointer->visible = 0;
        adventureUIManager->dialogpointergap->show = 0;
      }

      {
        float x2 = adventureUIManager->dialogpointer->x1 * WIN_WIDTH;
        float y2 = adventureUIManager->dialogpointer->y1 * WIN_HEIGHT;
        float x1 = adventureUIManager->dialogpointer->x2 * WIN_WIDTH;
        float y1 = adventureUIManager->dialogpointer->y2 * WIN_HEIGHT;

        // Calculate slope and y-intercept of the original line
        float m = (y2 - y1) / (x2 - x1);
        float b = y1 - (m * x1);

        // Distance from the line
        float distance = 52.0f;
        float distanceL = 58;

        // Angle for the perpendicular line
        float angle = atan(-1 / m);

        // Offset values for the perpendicular distance
        float offsetX = distance * cos(angle);
        float offsetY = distance * sin(angle);
        float offsetXL = distanceL * cos(angle);
        float offsetYL = distanceL * sin(angle);


        // Final point coordinates
        float endX = x2;
        float endY = y2;

        // Calculate points on either side of the line
        float xLeft = x1 - offsetXL;
        float yLeft = y1 - offsetYL;
        float xRight = x1 + offsetX;
        float yRight = y1 + offsetY;

        // Calculate slopes for lines converging to the end point
        float mlLeft = (endY - yLeft) / (endX - xLeft);
        float mlRight = (endY - yRight) / (endX - xRight);

        // Calculate new y-intercepts for the lines
        float blLeft = yLeft - (mlLeft * xLeft);
        float blRight = yRight - (mlRight * xRight);

        // Use any given y coordinate
        float y = adventureUIManager->dialogpointergap->y * WIN_HEIGHT;

        // Calculate x coordinates along the two other lines for the given y
        float xLeftConverge = (y - blLeft) / mlLeft;
        float xRightConverge = (y - blRight) / mlRight;

        adventureUIManager->dialogpointergap->x = xLeftConverge;
        adventureUIManager->dialogpointergap->width = xRightConverge -xLeftConverge;
        adventureUIManager->dialogpointergap->x /= WIN_WIDTH;
        adventureUIManager->dialogpointergap->width /= WIN_WIDTH;
        //        // Draw squares on either side
        //        SDL_Rect rLeft = {static_cast<int>(xLeftConverge), static_cast<int>(y), 5, 5};
        //        SDL_RenderCopy(renderer, grassTexture, NULL, &rLeft);
        //      
        //        SDL_Rect rRight = {static_cast<int>(xRightConverge), static_cast<int>(y), 5, 5};
        //        SDL_RenderCopy(renderer, grassTexture, NULL, &rRight);
      }	

    }

  }

  // spring
  if ((input[8] && !oldinput[8] && protag->grounded && protag_can_move && g_amState == amState::CLOSED) || (input[8] && storedJump && protag->grounded && protag_can_move && g_amState == amState::CLOSED))
  {
    protagMakesNoise();
    g_hasBeenHoldingJump = 1;
    g_afterspin_duration = 0;
    g_spinning_duration = 0;
    protag->zaccel = 180;
    g_protag_jumped_this_frame = 1;
    g_jumpGaranteedAccelMs = g_maxJumpGaranteedAccelMs;
    storedJump = 0;

    //if we're boarded within an entity, unboard
    if(g_protagIsWithinBoardable && !g_transferingByBoardable) {

      //if the protag was currently duping a behemoth and hopped out, re-agro that behemoth
      for(auto x : g_ai) {
        if(x->useAgro && x->lostHimSequence != 0) {
          x->agrod = 1;
          x->target = protag;
        }
      }

      smokeEffect->happen(protag->getOriginX(), protag->getOriginY(), protag->z, 0);
      g_protagIsWithinBoardable = 0;
      g_boardedEntity->semisolidwaittoenable = 1;
      g_boardedEntity->semisolid = 0;
      g_boardedEntity->sizeRestoreMs = 1000;
      g_boardedEntity->storedSemisolidValue = 1;
      protag->steeringAngle = wrapAngle(-M_PI/2);
      protag->animation = 4;
      protag->animation = 4;
      protag->flip = SDL_FLIP_NONE;
      protag->xvel = 0;
      protag->yvel = 0;
      if(staticInput[0]) {
        protag->yvel = -200;
      } else if(staticInput[1]) {
        protag->yvel = 200;
      }
      if(staticInput[2]) {
        protag->xvel = -200;
      } else if(staticInput[4]) {
        protag->xvel = 200;
      }




      g_boardedEntity = 0;
      protag->tangible = 1;    
      //protag->y += 45; //push us out of the boarded entity in a consistent way
      g_boardingCooldownMs = g_maxBoardingCooldownMs;
    }                          
  }                            
  else                         
  {                            
    g_protag_jumped_this_frame = 0;
    if (input[8] && !oldinput[8] && !protag->grounded)
    {
      storedJump = 1;
    }
  }

  // if we die don't worry about not being able to switch because we can't shoot yet
  if (protag->hp <= 0)
  {
    //playSound(4, g_s_playerdeath, 0);
    protag->cooldown = 0;
  }


  // cycle right if the current character dies
  if ((input[9] && !oldinput[9]) || protag->hp <= 0)
  {
    // keep switching if we switch to a dead partymember
    int i = 0;

    if (party.size() > 1 && protag->cooldown <= 0)
    {
      do
      {
        M("Cycle party right");
        std::rotate(party.begin(), party.begin() + 1, party.end());
        protag->tangible = 0;
        protag->flashingMS = 0;
        party[0]->tangible = 1;
        party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width / 2;
        party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height / 2;
        party[0]->z = protag->z;
        party[0]->xvel = protag->xvel;
        party[0]->yvel = protag->yvel;
        party[0]->zvel = protag->zvel;

        party[0]->animation = protag->animation;
        party[0]->flip = protag->flip;
        protag->zvel = 0;
        protag->xvel = 0;
        protag->yvel = 0;
        protag->zaccel = 0;
        protag->xaccel = 0;
        protag->yaccel = 0;
        protag = party[0];
        protag->shadow->x = protag->x + protag->shadow->xoffset;
        protag->shadow->y = protag->y + protag->shadow->yoffset;
        g_focus = protag;
        protag->curheight = 0;
        protag->curwidth = 0;
        g_cameraShove = protag->hisweapon->attacks[0]->range / 2;
        // prevent infinite loop
        i++;
        if (i > 600)
        {
          M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)");
          break;
          quit = 1;
        }
      } while (protag->hp <= 0);
    }
  }

  SDL_RenderClear(renderer);

  if (g_backgroundLoaded && g_useBackgrounds)
  { // if the level has a background and the user would like to see it
    SDL_RenderCopy(renderer, background, NULL, NULL);
  }

  for (auto n : g_entities)
  {
    n->cooldown -= elapsed;
  }

  // listeners
  for (long long unsigned int i = 0; i < g_listeners.size(); i++)
  {
    if (g_listeners[i]->update())
    {
      // !!! could need check that we aren't in dialogue or running a script
      adventureUIManager->ownScript = g_listeners[i]->script;
      adventureUIManager->talker = narrarator;
      adventureUIManager->dialogue_index = -1;
      narrarator->sayings = g_listeners[i]->script;
      adventureUIManager->initDialogue();
      adventureUIManager->continueDialogue();
      g_listeners[i]->active = 0;
    }
  }


  updateWindowResolution();

  //animate dialogproceedarrow
  {
    adventureUIManager->c_dpiDesendMs += elapsed;
    if(adventureUIManager->c_dpiDesendMs > adventureUIManager->dpiDesendMs) {
      adventureUIManager->c_dpiDesendMs = 0;
      adventureUIManager->c_dpiAsending = !adventureUIManager->c_dpiAsending;

    }

    if(adventureUIManager->c_dpiAsending) {
      adventureUIManager->dialogProceedIndicator->y += adventureUIManager->dpiAsendSpeed;
    } else {
      adventureUIManager->dialogProceedIndicator->y -= adventureUIManager->dpiAsendSpeed;

    }
  }

  if (devMode)
  {

    g_camera.width = WIN_WIDTH / (scalex * g_zoom_mod * 0.2); // the 0.2 is arbitrary. it just makes sure we don't end the camera before the screen
    g_camera.height = WIN_HEIGHT / (scalex * g_zoom_mod * 0.2);
  }
  else
  {
    g_camera.width = WIN_WIDTH;
    g_camera.height = WIN_HEIGHT;
  }


  // update ui
  curTextWait += elapsed * text_speed_up;
  if (curTextWait >= textWait && protag_is_talking)
  {
    adventureUIManager->updateText();
    curTextWait = 0;
  }

  if(0){ //behemoth ui
    if(g_behemoth0 != nullptr && g_behemoth0->tangible) {
      adventureUIManager->b0_element->show = 1;

      float ox = g_behemoth0->getOriginX();
      float oy = g_behemoth0->getOriginY();

      float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
      // update crosshair to current objective
      //

      float crossx = 0;
      float crossy = 0;

      // hide crosshair if we are close
      if(distToObj < pow(64*5.5,2))
      {
        crossx = 5;
        crossy = 5;
      } else {
        //crosshair should point to the object
        float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
        angleToObj += M_PI/2;
        float magnitude = 0.43;

        crossx = 0.5;
        crossy = 0.5;
        float w = WIN_WIDTH;
        float h = WIN_HEIGHT;

        //Since the camera is angled, a world block appears wider than it is tall
        //And so I want the reticles to travel around an elipse rather than a sphere
        //it's not perfectly simple to accomodate for this here, though
        //Let's do math to find the difference between the radius of a circle
        //and of an elipse

        float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
        float b = 1;
        float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
        magnitude *=ellipseRadius;

        crossx += (-cos(angleToObj) * magnitude) * h/w;
        crossy += sin(angleToObj) * magnitude;
      }



      adventureUIManager->b0_element->x = crossx - adventureUIManager->crosshair->width / 2;
      adventureUIManager->b0_element->y = crossy - adventureUIManager->crosshair->height;

    } else {
      adventureUIManager->b0_element->show = 0;
    }
    if(g_behemoth1 != nullptr && g_behemoth1->tangible) {
      adventureUIManager->b1_element->show = 1;

      float ox = g_behemoth1->getOriginX();
      float oy = g_behemoth1->getOriginY();

      float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
      // update crosshair to current objective
      //

      float crossx = 0;
      float crossy = 0;

      // hide crosshair if we are close
      if(distToObj < pow(64*5.5,2))
      {
        crossx = 5;
        crossy = 5;
      } else {
        //crosshair should point to the object
        float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
        angleToObj += M_PI/2;
        float magnitude = 0.43;
        crossx = 0.5;
        crossy = 0.5;
        float w = WIN_WIDTH;
        float h = WIN_HEIGHT;

        //Since the camera is angled, a world block appears wider than it is tall
        //And so I want the reticles to travel around an elipse rather than a sphere
        //it's not perfectly simple to accomodate for this here, though
        //Let's do math to find the difference between the radius of a circle
        //and of an elipse

        float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
        float b = 1;
        float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
        magnitude *=ellipseRadius;

        crossx += (-cos(angleToObj) * magnitude) * h/w;
        crossy += sin(angleToObj) * magnitude;
      }



      adventureUIManager->b1_element->x = crossx - adventureUIManager->crosshair->width / 2;
      adventureUIManager->b1_element->y = crossy - adventureUIManager->crosshair->height;

    } else {
      adventureUIManager->b1_element->show = 0;
    }


    if(g_behemoth2 != nullptr && g_behemoth2->tangible) {
      adventureUIManager->b2_element->show = 1;

      float ox = g_behemoth2->getOriginX();
      float oy = g_behemoth2->getOriginY();

      float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
      // update crosshair to current objective
      //

      float crossx = 0;
      float crossy = 0;

      // hide crosshair if we are close
      if(distToObj < pow(64*5.5,2))
      {
        crossx = 5;
        crossy = 5;
      } else {
        //crosshair should point to the object
        float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
        angleToObj += M_PI/2;
        float magnitude = 0.43;
        crossx = 0.5;
        crossy = 0.5;
        float w = WIN_WIDTH;
        float h = WIN_HEIGHT;

        //Since the camera is angled, a world block appears wider than it is tall
        //And so I want the reticles to travel around an elipse rather than a sphere
        //it's not perfectly simple to accomodate for this here, though
        //Let's do math to find the difference between the radius of a circle
        //and of an elipse

        float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
        float b = 1;
        float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
        magnitude *=ellipseRadius;

        crossx += (-cos(angleToObj) * magnitude) * h/w;
        crossy += sin(angleToObj) * magnitude;
      }



      adventureUIManager->b2_element->x = crossx - adventureUIManager->crosshair->width / 2;
      adventureUIManager->b2_element->y = crossy - adventureUIManager->crosshair->height;

    } else {
      adventureUIManager->b2_element->show = 0;
    }


    if(g_behemoth3 != nullptr && g_behemoth3->tangible) {
      adventureUIManager->b3_element->show = 1;


      float ox = g_behemoth3->getOriginX();
      float oy = g_behemoth3->getOriginY();

      float distToObj = XYWorldDistanceSquared(ox, oy, protag->getOriginX(), protag->getOriginY());
      // update crosshair to current objective
      //

      float crossx = 0;
      float crossy = 0;

      // hide crosshair if we are close
      if(distToObj < pow(64*5.5,2))
      {
        crossx = 5;
        crossy = 5;
      } else {
        //crosshair should point to the object
        float angleToObj = atan2(ox - protag->getOriginX(), oy - protag->getOriginY());
        angleToObj += M_PI/2;
        float magnitude = 0.43;
        crossx = 0.5;
        crossy = 0.5;
        float w = WIN_WIDTH;
        float h = WIN_HEIGHT;

        //Since the camera is angled, a world block appears wider than it is tall
        //And so I want the reticles to travel around an elipse rather than a sphere
        //it's not perfectly simple to accomodate for this here, though
        //Let's do math to find the difference between the radius of a circle
        //and of an elipse

        float a = YtoX; //this ellipse has the same dimensional ratio as an image of a block in the world
        float b = 1;
        float ellipseRadius = (a * b) / ( pow( (pow(a,2) * pow(sin(angleToObj),2) + pow(b,2) * pow(cos(angleToObj),2)  ) , 0.5) );
        magnitude *=ellipseRadius;

        crossx += (-cos(angleToObj) * magnitude) * h/w;
        crossy += sin(angleToObj) * magnitude;
      }



      adventureUIManager->b3_element->x = crossx - adventureUIManager->crosshair->width / 2;
      adventureUIManager->b3_element->y = crossy - adventureUIManager->crosshair->height;

    } else {
      adventureUIManager->b3_element->show = 0;
    }

  }

  //should we show the visionDetectable?
  adventureUIManager->seeingDetectable->show = 0;
  if(g_protagIsBeingDetectedBySight) {
    adventureUIManager->seeingDetectable->show = 1;
  }


  // SDL_Rect FoWrect;

  // update particles
  for (auto x : g_particles)
  {
    x->update(elapsed, g_camera);
  }

  // delete old particles
  for (int i = 0; i < g_particles.size(); i++)
  {
    if (g_particles[i]->lifetime < 0)
    {
      if(g_particles[i]->type->disappearMethod == 0) {
        //shrink
        g_particles[i]->deltasizex = -1;
        g_particles[i]->deltasizey = -1;
      } else {
        //fade
        g_particles[i]->deltaAlpha = -60;
      }
    }
  }

  for (int i = 0; i < g_particles.size(); i++)
  {
    if(g_particles[i]->width <= 0 || g_particles[i]->curAlpha <= 0) {
      delete g_particles[i];
      i--;
    }
  }

  //emitters emit particles
  for(auto e : g_emitters) {

    //check interval ms
    if(e->currentIntervalMs <= 0) {
      e->type->happen(e->parent->getOriginX() + e->xoffset, e->parent->getOriginY() + e->yoffset, e->parent->z + e->zoffset, 0);
      e->currentIntervalMs = e->maxIntervalMs;
    }
    e->currentIntervalMs -= elapsed;


    if(e->timeToLiveMs != 0) {
      if(e->timeToLiveMs - elapsed <= 0) {
        delete e;
      } else {
        e->timeToLiveMs -= elapsed;
      }
    }



  }





  {
    //to fix a problem with too many trees which were large entities
    g_l_entities.clear();
    SDL_FRect cam;
    cam.x = 0;
    cam.y = 0;
    cam.w = g_camera.width;
    cam.h = g_camera.height;
    for(auto x : g_large_entities) {
      SDL_FRect obj;
      obj.x = ((x->x) - g_camera.x + (x->width-(x->curwidth))/2)* 1;

      obj.y = ((x->y) - (((x->curheight) * (XtoY) + (x->height * (1-XtoY)))) 

          - ( 
            (x->height * (XtoY))
            -
            (x->curheight * XtoY)
            )*0.5

          - ((x->z) + x->floatheight) * XtoZ) - g_camera.y;

      obj.w = (x->curwidth);
      obj.h =(x->curheight);

      if(RectOverlap(obj, cam)) {
        g_l_entities.push_back(x);
      }
    }


    g_lt_collisions.clear();
    //for fog of war, keep a list of map Collisions to use 
    //which are close to the player and on layer 0
    g_is_collisions.clear();
    for(auto x : g_impliedSlopes) {
      SDL_FRect obj;
      obj.x = (x->bounds.x -g_camera.x)* g_camera.zoom;
      obj.y = (x->bounds.y -g_camera.y - height) * g_camera.zoom;
      obj.w = x->bounds.width * g_camera.zoom;
      obj.h = x->bounds.height * g_camera.zoom;

      if(RectOverlap(obj, cam))
      {
        g_is_collisions.push_back(x);
      }

    }

    //could foreseeably cause issues if ents try pathfinding around
    //layer 1 blocks and don't "see" layer 0 blocks
    if(g_focus->stableLayer+1 < g_boxs.size()) {
      for(auto x : g_boxs[g_focus->stableLayer+1]) {
        SDL_FRect obj;
        obj.x = (x->bounds.x -g_camera.x)* g_camera.zoom;
        obj.y = (x->bounds.y -g_camera.y - height) * g_camera.zoom;
        obj.w = x->bounds.width * g_camera.zoom;
        obj.h = x->bounds.height * g_camera.zoom;

        if(RectOverlap(obj, cam))
        {
          g_lt_collisions.push_back(x);
        }

      }
    }

  }
  B("close collision check");

  // ui
  if (!inPauseMenu && g_showHUD)
  {
    // !!! segfaults on mapload sometimes

    //      SDL_Color useThisColor = g_healthtextcolor;
    //      if(protag->hp < 5) {
    //        useThisColor = g_healthtextlowcolor;
    //      }
    //      adventureUIManager->healthText->updateText(to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), -1, 0.9,  useThisColor);
    //      adventureUIManager->healthText->show = 1;

    //adventureUIManager->hungerText->updateText(to_string((int)((float)(min(g_foodpoints, g_maxVisibleFoodpoints) * 100) / (float)g_maxVisibleFoodpoints)) + '%', -1, 0.9);
    //adventureUIManager->hungerText->show = 0;

    //animate the guts sometimes
    //heart shake
    //      adventureUIManager->heartShakeIntervalMs -= elapsed;
    //      if(adventureUIManager->heartShakeIntervalMs < 0) {
    //        //make the heart shake back and forth briefly
    //        adventureUIManager->heartShakeDurationMs = adventureUIManager->maxHeartShakeDurationMs;
    //
    //        adventureUIManager->heartShakeIntervalMs = adventureUIManager->maxHeartShakeIntervalMs + rand() % adventureUIManager->heartShakeIntervalRandomMs;
    //      }
    //
    //      if(adventureUIManager->heartShakeDurationMs > 0) {
    //        adventureUIManager->heartShakeDurationMs -= elapsed;
    //
    //        if(adventureUIManager->heartShakeDurationMs % 400 > 200) {
    //          //move left
    //          adventureUIManager->healthPicture->targetx = -0.04 - 0.005;
    //
    //        } else {
    //          //move right
    //          adventureUIManager->healthPicture->targetx = -0.04 + 0.005;
    //
    //        }
    //
    //      } else {
    //        //move the heart back to its normal position
    //        adventureUIManager->healthPicture->targetx = -0.04;
    //
    //      }

    //stomach shaking
    //      adventureUIManager->stomachShakeIntervalMs -= elapsed;
    //      if(adventureUIManager->stomachShakeIntervalMs < 0) {
    //        //make the stomach shake back and forth briefly
    //        adventureUIManager->stomachShakeDurationMs = adventureUIManager->maxstomachShakeDurationMs;
    //
    //        float hungerratio = ((float)(min(g_foodpoints, g_maxVisibleFoodpoints) / (float)g_maxVisibleFoodpoints));
    //        D(hungerratio);
    //
    //        //stomach grumbles less when the protag is less hungry
    //        adventureUIManager->stomachShakeIntervalMs = adventureUIManager->maxstomachShakeIntervalMs * hungerratio + rand() % adventureUIManager->stomachShakeIntervalRandomMs;
    //      }

    //      if(adventureUIManager->stomachShakeDurationMs > 0) {
    //        adventureUIManager->stomachShakeDurationMs -= elapsed;
    //
    //        if(adventureUIManager->stomachShakeDurationMs % 400 > 200) {
    //          //move left
    //          adventureUIManager->hungerPicture->targetx = 0.8 - 0.005;
    //
    //        } else {
    //          //move right
    //          adventureUIManager->hungerPicture->targetx = 0.8 + 0.005;
    //
    //        }
    //
    //      } else {
    //        //move the stomach back to its normal position
    //        adventureUIManager->hungerPicture->targetx = 0.8;
    //
    //      }

    //      //tongue swallowing
    //      adventureUIManager->tungShakeIntervalMs -= elapsed;
    //      if(adventureUIManager->tungShakeIntervalMs < 0) {
    //        //make the tung shake back and forth briefly
    //        adventureUIManager->tungShakeDurationMs = adventureUIManager->maxTungShakeDurationMs;
    //
    //        adventureUIManager->tungShakeIntervalMs = adventureUIManager->maxTungShakeIntervalMs + rand() % adventureUIManager->tungShakeIntervalRandomMs;
    //      }
    //
    //      if(adventureUIManager->tungShakeDurationMs > 0) {
    //        adventureUIManager->tungShakeDurationMs -= elapsed;
    //
    //        //move tung up
    //        adventureUIManager->tastePicture->targety = 0.75 - 0.01;
    //        adventureUIManager->tastePicture->targetx = 0 - 0.01;
    //
    //      } else {
    //        //move the tung back to its normal position
    //        adventureUIManager->tastePicture->targetx = 0;
    //        adventureUIManager->tastePicture->targety = 0.75;
    //
    //      }

    //heart beating
    //      adventureUIManager->heartbeatDurationMs -= elapsed;
    //      if(adventureUIManager->heartbeatDurationMs > 200) {
    //          //expand
    //          adventureUIManager->healthPicture->targetwidth = 0.25;
    //
    //      } else {
    //          //contract
    //          adventureUIManager->healthPicture->targetwidth = 0.25 - adventureUIManager->heartShrinkPercent;
    //
    //      }
    //      if(adventureUIManager->heartbeatDurationMs < 0) {
    //        adventureUIManager->heartbeatDurationMs = (adventureUIManager->maxHeartbeatDurationMs - 300) * ((float)protag->hp / (float)protag->maxhp) + 300;
    //        float hpratio = ((float)protag->hp / (float)protag->maxhp);
    //        if(hpratio < 0.6) {
    //        adventureUIManager->healthPicture->widthGlideSpeed = 0.1 + min(0.2, 0.3 *((float)protag->hp / (float)protag->maxhp));
    //        } else {
    //          adventureUIManager->healthPicture->widthGlideSpeed = 0.1;
    //
    //        }
    //      }


  }
  else
  {
    //adventureUIManager->healthText->show = 0;
    //adventureUIManager->hungerText->show = 0;
  }
  B("UI");

  B("Inventory ui");

  // sines for item bouncing
  g_elapsed_accumulator += elapsed;
  g_itemsines[0] = ( sin(g_elapsed_accumulator / 300) * 10 + 30);
  g_itemsines[1] = ( sin((g_elapsed_accumulator + (235 * 1) ) / 300) * 10 + 30);
  g_itemsines[2] = ( sin((g_elapsed_accumulator + (235 * 2) ) / 300) * 10 + 30);
  g_itemsines[3] = ( sin((g_elapsed_accumulator + (235 * 3) ) / 300) * 10 + 30);
  g_itemsines[4] = ( sin((g_elapsed_accumulator + (235 * 4) ) / 300) * 10 + 30);
  g_itemsines[5] = ( sin((g_elapsed_accumulator + (235 * 5) ) / 300) * 10 + 30);
  g_itemsines[6] = ( sin((g_elapsed_accumulator + (235 * 6) ) / 300) * 10 + 30);
  g_itemsines[7] = ( sin((g_elapsed_accumulator + (235 * 7) ) / 300) * 10 + 30);
  B("Itemsines");


  if (g_elapsed_accumulator > 1800 * M_PI)
  {
    g_elapsed_accumulator -= 1800* M_PI;
  }


  if(g_waterAllocated && g_waterOnscreen) {
    g_wAcc+= 0.1;
    if(g_wAcc > 512) {
      g_wAcc -= 512;
    }
    g_waterTexture = animateWater(renderer, g_waterTexture, g_waterSurface, g_wAcc);
  }
  g_waterOnscreen = 0;
  B("Animate water");


  if(g_dungeonDoorActivated == 0) {
    g_dungeonDarkEffectDelta = -16;
  } else { 
    g_dungeonDarkEffectDelta = 16;
    elapsed = 0;

  }

  //tallgrass update
  /*{
    int grassX = protag->getOriginX();
    int grassY = protag->getOriginY();
    grassX -= grassX % 64;
    grassY -= grassY % 55;
    if(grassX != g_lastGrassX || grassY != g_lastGrassY) {
    g_protagIsBeingDetectedBySight = 0;
    for(auto x : g_tallGrasses) {
    if(RectOverlap(protag->getMovedBounds(), x->bounds)) {
    g_protagIsBeingDetectedBySight = 1;
    if(frng(0,1) <= g_encounterChance) {
    int randomIndex = rng(0, loadedEncounters.size()-1);
    for(auto x : loadedEncounters[randomIndex]) {
    combatant* a = new combatant(x.first, x.second);
    a->level = x.second;
    a->maxHealth = a->baseHealth + (a->healthGain * a->level);
    a->health = a->maxHealth;
    g_enemyCombatants.push_back(a);
    }

    M("Lets load some bgrounds");
    string bgstr;
    if(loadedBackgrounds.size() > randomIndex) {
    bgstr = loadedBackgrounds[randomIndex];
    } else {
    if(loadedBackgrounds.size() == 0) {E("No loaded combat backgrounds for map " + g_mapdir + "/" + g_map); abort();}
    bgstr = loadedBackgrounds[0];
    }

    string loadme = "resources/static/backgrounds/json/" + bgstr + ".json";
    M("Fuck");
    combatUIManager->loadedBackground = bground(renderer, loadme.c_str());

    if(combatUIManager->sb1 != 0) {
    SDL_FreeSurface(combatUIManager->sb1);
    }

    loadme = "resources/static/backgrounds/textures/" + to_string(combatUIManager->loadedBackground.texture) + ".qoi";
    combatUIManager->sb1 = IMG_Load(loadme.c_str());

    if(combatUIManager->scene !=0) {
    SDL_DestroyTexture(combatUIManager->scene);
    }
    loadme = "resources/static/backgrounds/scenes/" + combatUIManager->loadedBackground.scene + ".qoi";
    combatUIManager->scene = loadTexture(renderer, loadme);


    cyclePalette(combatUIManager->sb1, combatUIManager->db1, combatUIManager->loadedBackground.palette);

    {
    g_gamemode = gamemode::COMBAT;
    g_submode = submode::INWIPE;
    writeSave();
    transitionDelta = transitionImageHeight;
    g_combatEntryType = 1;

    combatUIManager->partyHealthBox->show = 1;
    combatUIManager->partyText->show = 1;
    combatUIManager->finalText = "It's an enemy encounter!";
    combatUIManager->currentText = "";
    combatUIManager->dialogProceedIndicator->y = 0.25;

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

  // tiles
  for (long long unsigned int i = 0; i < g_tiles.size(); i++)
  {
    if (g_tiles[i]->z == 0)
    {
      g_tiles[i]->render(renderer, g_camera);
    }
  }

  for (long long unsigned int i = 0; i < g_tiles.size(); i++)
  {
    if (g_tiles[i]->z == 1)
    {
      g_tiles[i]->render(renderer, g_camera);
    }
  }

  // sort
  sort_by_y(g_actors);
  for (long long unsigned int i = 0; i < g_actors.size(); i++)
  {
    g_actors[i]->render(renderer, g_camera);
  }

  for (long long unsigned int i = 0; i < g_tiles.size(); i++)
  {
    if (g_tiles[i]->z == 2)
    {
      g_tiles[i]->render(renderer, g_camera);
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
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
    SDL_RenderCopy(renderer, frame, NULL, NULL);
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
  transition = 1;
  titleUIManager->hideAll();
  SDL_GL_SetSwapInterval(1);
}

}


}
}
}
}
g_lastGrassX = grassX;
g_lastGrassY = grassY;
}*/
B("Tall grass update");

{ //set frames for protags

  for(auto &x : party) {
    if(x->hisCombatant->health <= 0) {
      x->frameInAnimation = 2;
      x->animlimit = 0;
    } else {
      x->animlimit = 0.01;
      if(x->flashingMS > 0) {
        x->frameInAnimation = 1;
      } else {
        x->frameInAnimation = 0;
      }
    }

  }
  if(protag->hisCombatant->health <= 0) {
    g_spin_entity->animation = 1;
  } else {
    g_spin_entity->animation = 0;
  }

}


// ENTITY MOVEMENT (ENTITY UPDATE)
// dont update movement while transitioning
if (1)
{
  for (long long unsigned int i = 0; i < g_entities.size(); i++)
  {
    if (g_entities[i]->isWorlditem || (g_entities[i]->identity == 35&& g_entities[i]->usingTimeToLive == 0))
    {
      // make it bounce
      int index = g_entities[i]->bounceindex;
      g_entities[i]->floatheight = g_itemsines[index];
    }
    door* taken = nullptr;
    if( (protag_is_talking == 0 && g_amState == amState::CLOSED && inPauseMenu == 0)) {
      taken = g_entities[i]->update(g_doors, elapsed);
      if(g_breakFromPrimarySwitch) {
        g_breakFromPrimarySwitch = 0;
        return;
      }
    }


    // added the !transition because if a player went into a map with a door located in the same place
    // as they are in the old map (before going to the waypoint) they would instantly take that door
    if (taken != nullptr && !transition)
    {
      // player took this door
      {
        g_wayOffsetX = 0;
        g_wayOffsetY = 0;
        M("Set old door vals");
        g_oldDoorWidth = taken->width;
        g_oldDoorHeight = taken->height;

        if(taken->width > taken->height) {
          g_wayOffsetX = protag->getOriginX() - (taken->x + (taken->width)/2);
          g_useOffset = 1;
        } else {
          g_wayOffsetY = protag->getOriginY() - (taken->y + (taken->height)/2);
          g_useOffset = 2;
        }
      }

      // clear level

      // we will now clear the map, so we will save the door's destination map as a string
      const string savemap = "resources/maps/" + taken->to_map + ".map";
      const string dest_waypoint = taken->to_point;

      // render this frame

      clear_map(g_camera);
      load_map(renderer, savemap, dest_waypoint);

      // clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
      if (canSwitchOffDevMode)
      {
        init_map_writing(renderer);
      }

      writeSave();

      break;
    }

    if(g_entities[i]->usingTimeToLive) {

      if(g_entities[i]->timeToLiveMs < 0) {
        if(g_entities[i]->dontSave) {
          if(!g_entities[i]->asset_sharer) {
            g_entities[i]->tangible = 0;
            g_entities[i]->usingTimeToLive = 0;
          } else {
            delete g_entities[i];
          }

        } else {

          g_entities[i]->height = 0;
          g_entities[i]->width = 0;

          g_entities[i]->shrinking = 1;

          if(!g_entities[i]->wasPellet) { 
            g_entities[i]->dynamic = 0;
            g_entities[i]->xvel = 0;
            g_entities[i]->yvel = 0;
            g_entities[i]->missile = 0;
          }


          if(g_entities[i]->curheight < 1) {
            //remove this entity from it's parent's 
            //list of children
            if(g_entities[i]->isOrbital) {
              g_entities[i]->parent->children.erase(remove(g_entities[i]->parent->children.begin(), g_entities[i]->parent->children.end(), g_entities[i]), g_entities[i]->parent->children.end());
              g_entities[i]->isOrbital = 0;
            }



            if(!g_entities[i]->asset_sharer) {
              g_entities[i]->tangible = 0;
              g_entities[i]->usingTimeToLive = 0;
            } else {
              delete g_entities[i];
            }
          }
        }
      } 

    }

  }
}
B("Entity update");
if(g_breakFromPrimarySwitch) {
  g_breakFromPrimarySwitch = 0;
  return;
}


//dungeon door flash
for(auto x : g_dungeonDoors) {
  rect b = {(int)x->x, (int)x->y, (int)x->width, (int)x->height};
  if(RectOverlap(b, protag->getMovedBounds())) {
    g_dungeonDarkEffectDelta = 16;
  }
}

g_dungeonDarkEffect += g_dungeonDarkEffectDelta;
if(g_dungeonDarkEffect > 255) { g_dungeonDarkEffect = 255;}
if(g_dungeonDarkEffect < g_dungeonDarkness) { g_dungeonDarkEffect = g_dungeonDarkness;}
if(g_dungeonDarkEffect == 255) {
  dungeonFlash();
}


//dungeon flash effect
SDL_SetTextureAlphaMod(g_shade, g_dungeonDarkEffect);

//familiars
if(protag != nullptr) {
  for(int i = 0; i < g_familiars.size(); i++) {
    g_familiars[i]->shadow->x = g_familiars[i]->x + g_familiars[i]->shadow->xoffset;
    g_familiars[i]->shadow->y = g_familiars[i]->y + g_familiars[i]->shadow->yoffset;

    entity* him = g_familiars[i];
    entity* target;

    int targetX, targetY;

    if(i == 0) {
      target = protag;
    } else {
      target = g_familiars[i-1];
    }

    float speedmod = 10;
    float useDist = 64;

    if(i == g_familiars.size() - 1) {
      if(g_chain_time > 0) {
        useDist = 35;
        target = protag;
        speedmod = 5;
      }
    }

    float dx = target->getOriginX() - him->getOriginX();
    float dy = target->getOriginY() - him->getOriginY();

    float dist = pow( dx * dx + dy * dy, 0.5);

    float factor = 1 - (useDist / dist);
    float speed = speedmod * factor;


    if(dist > useDist) {
      him->x += (dx / dist) * speed;
      him->y += (dy / dist) * speed;
    }
  }

  //for familiars which were just linked, display the flashing chain
  if(g_familiars.size() > 0) {
    if(g_chain_time > 0) {
      entity* x = g_familiars.back();
      g_chain_entity->setOriginX(x->getOriginX());
      g_chain_entity->setOriginY(x->getOriginY());
      g_chain_entity->visible = 1;
      g_chain_time -= elapsed;
    } else {
      g_chain_entity->visible = 0;
    }
  } else {
    g_chain_entity->visible = 0;
  }

  if(g_ex_familiars.size() > 0 && g_exFamiliarTimer > 0) {
    g_exFamiliarTimer -= elapsed;
    for(auto x : g_ex_familiars) {
      if(x->shadow != nullptr) {
        x->shadow->x = x->x + x->shadow->xoffset;
        x->shadow->y = x->y + x->shadow->yoffset;
      }
      const float speed = 0.9;
      const float r = 1 - speed;
      float tx = g_exFamiliarParent->getOriginX();
      float ty = g_exFamiliarParent->getOriginY();

      x->setOriginX(x->getOriginX()*speed + tx*r);
      x->setOriginY(x->getOriginY()*speed + ty*r);

      if(x->getOriginX() < tx-1) {
        x->x ++;
      }
      if(x->getOriginX() > tx+1) {
        x->x --;
      }

      if(x->getOriginY() < ty-1) {
        x->y ++;
      }
      if(x->getOriginY() > ty+1) {
        x->y --;
      }
    }
  } else {
    for(auto x : g_ex_familiars) {
      x->parent = g_exFamiliarParent;
    }
    g_ex_familiars.clear();

  }

  //for combining familiars
  for(auto x : g_combineFamiliars) {
    x->shadow->x = x->x + x->shadow->xoffset;
    x->shadow->y = x->y + x->shadow->yoffset;

    float dx = g_familiarCombineX - x->getOriginX();
    float dy = g_familiarCombineY - x->getOriginY();

    float dist = pow( dx * dx + dy * dy, 0.5);

    SDL_SetTextureColorMod(x->texture, 120, 120, 120);

    x->flashingMS = 1000;


    x->x += (dx / 8);
    x->y += (dy / 8);

    if(dist < 3) {
      //idk get rid of these
      for(auto y : g_combineFamiliars) {
        y->tangible = 0;
        g_combineFamiliars.erase(remove(g_combineFamiliars.begin(), g_combineFamiliars.end(), y), g_combineFamiliars.end());
      }
    }


  }

  if(g_combineFamiliars.size() == 0 && g_combinedFamiliar != 0) {
    g_combinedFamiliar->setOriginX(g_familiarCombineX);
    g_combinedFamiliar->setOriginY(g_familiarCombineY);
    g_familiars.push_back(g_combinedFamiliar);
    //g_familiars.insert(g_familiars.begin(), 1, g_combinedFamiliar);
    g_combinedFamiliar->darkenValue = 0;
    g_combinedFamiliar->flagA = 1;
    g_combinedFamiliar = 0;

  }
}
B("familiars");

g_spurl_entity->setOriginX(protag->getOriginX());
g_spurl_entity->setOriginY(protag->getOriginY());
g_spurl_entity->z = protag->z;

//did the protag collect a pellet?
float protag_x = protag->getOriginX();
float protag_y = protag->getOriginY();
if(g_showPellets) {
  for(int i = 0; i < g_pellets.size(); i++) {
    entity* x = g_pellets[i];

    //well, this is shitty. Somehow I find myself in an unfortunate situation which I dont understand
    //Somehow, I can't get pellets to appear ontopof the lowest fogsheet yet behind fomm, if he's
    //infront.
    //I'm going to cheat and change their sorting offset based on distance to the protag, so they'll still be jank (e.g., other enemy is close to a cupcake), but whatever, it's better than nothin
    //and I really want pellets to stick out from fog
    int ydiff = protag_y - x->y;
    if(-ydiff > -85) {
      //x->sortingOffset = 30;
    } else {
      //x->sortingOffset = 160; //pellets that are close to the fog are artificially boosted in the draw order
    }

    if(ydiff < 85 && ydiff > 0) {
      //x->sortingOffset = 30;
    }
  }
}

//    string systemTimePrint = "";
//    if(g_dungeonSystemOn) {
//      //timer display
//      int ms = g_dungeonMs;
//      int sec = (ms / 1000) % 60;
//      int min = ms / 60000;
//      string secstr = to_string(sec);
//      if(secstr.size() < 2) {secstr = "0" + secstr;}
//      string minstr = to_string(min);
//      if(minstr.size() < 2) {minstr = "0" + minstr;}
//      
//      systemTimePrint = minstr + ":" + secstr;
//    } else {
//      //system clock display
//      time_t ttime = time(0);
//      tm *local_time = localtime(&ttime);
//      
//      int useHour = local_time->tm_hour;
//      if(useHour == 0) {useHour = 12;}
//      string useMinString = to_string(local_time->tm_min);
//      if(useMinString.size() == 1) { 
//        useMinString = "0" + useMinString;
//      }
//      string useHourString = to_string(useHour%12);
//      if(useHourString == "0") {useHourString = "12";}
//      systemTimePrint+= useHourString + ":" + useMinString;
//      
//      if(local_time->tm_hour >=12){
//        systemTimePrint += " PM";
//      } else {
//        systemTimePrint += " AM";
//      }
//    }

//adventureUIManager->systemClock->updateText(systemTimePrint, -1, 1);

// update projectiles
for (auto n : g_projectiles)
{
  n->update(elapsed);
}

// delete projectiles with expired lifetimes
for (long long unsigned int i = 0; i < g_projectiles.size(); i++)
{
  if (g_projectiles[i]->lifetime <= 0)
  {
    delete g_projectiles[i];
    i--;
  }
}

// triggers
for (long long unsigned i = 0; i < g_triggers.size(); i++)
{
  if (!g_triggers[i]->active)
  {
    continue;
  }
  rect trigger = {g_triggers[i]->x, g_triggers[i]->y, g_triggers[i]->width, g_triggers[i]->height};
  entity *checkHim = searchEntities(g_triggers[i]->targetEntity);
  if (checkHim == nullptr)
  {
    continue;
  }
  rect movedbounds = rect(checkHim->bounds.x + checkHim->x, checkHim->bounds.y + checkHim->y, checkHim->bounds.width, checkHim->bounds.height);
  if (RectOverlap(movedbounds, trigger) && (checkHim->z > g_triggers[i]->z && checkHim->z < g_triggers[i]->z + g_triggers[i]->zeight))
  {
    adventureUIManager->blip = g_ui_voice;
    adventureUIManager->ownScript = g_triggers[i]->script;
    adventureUIManager->talker = narrarator;
    adventureUIManager->dialogue_index = -1;
    narrarator->sayings = g_triggers[i]->script;
    adventureUIManager->continueDialogue();
    if (transition)
    {
      break;
    }

    g_triggers[i]->active = 0;
  }
}
B("Triggers update");

//hitboxes
for(auto a : g_hitboxes) {
  if(a->active) {
    a->activeMS -= elapsed;
    if(a->targetFaction == 0) {

      if(CylinderOverlap(a->getMovedBounds(), protag->getMovedBounds()) && !g_protagIsWithinBoardable) {
        hurtProtag(a->damage);
        a->activeMS = -1;
      }
    }
  } else {
    a->sleepingMS -= elapsed;
    if(a->sleepingMS <= 0) {
      a->active = 1;
    }
  }
}

for(int i = 0; i < g_hitboxes.size(); i++) {
  if(g_hitboxes[i]->activeMS <= 0) {
    delete g_hitboxes[i];
    i--;
  }
}

{ //clean up loadplaysounds
  for(auto &x : g_loadPlaySounds) {
    if(x.first < 0) {
      Mix_FreeChunk(x.second);
      g_loadPlaySounds.erase(remove(g_loadPlaySounds.begin(), g_loadPlaySounds.end(), x), g_loadPlaySounds.end());
      break;

    }
    x.first -= elapsed;
  }

}


// worldsounds
for (long long unsigned int i = 0; i < g_worldsounds.size(); i++)
{
  g_worldsounds[i]->update(elapsed);
}

{ //grossup effect
  if(g_grossupShowMs > 0) {
    SDL_Rect dest;
    dest.h = WIN_HEIGHT;
    dest.w = WIN_HEIGHT;
    dest.x = WIN_WIDTH/2 - WIN_HEIGHT/2;
    dest.y = 0;
    SDL_RenderCopy(renderer, g_grossup, NULL, &dest);
    g_grossupShowMs -= elapsed;
  }
}
B("Sounds & grossup");

// transition
{
  if (transition)
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
    //        D(elapsed);
    //        M("What did I break?");

    SDL_UnlockTexture(transitionTexture);
    SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

    if (transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth / 2, 2) + pow(transitionImageHeight, 2), 0.5))
    {
      transition = 0;
    }
  }
  else
  {
    transitionDelta = transitionImageHeight;
  }
}

if(!g_dungeonSystemOn) {
  if(g_musicSilenceMs > 0) {
    g_musicSilenceMs -= elapsed;
    musicUpdateTimer = 500;
    g_currentMusicPlayingEntity = nullptr;
    g_closestMusicNode = nullptr;
  } else { 
    // update music
    if (musicUpdateTimer > 500)
    {
      musicUpdateTimer = 0;

      // check musicalentities
      entity *listenToMe = nullptr;
      for (auto x : g_musicalEntities)
      {
        if (XYWorldDistance(x->getOriginX(), x->getOriginY(), g_focus->getOriginX(), g_focus->getOriginY()) < x->musicRadius && x->agrod && x->target == protag)
        {
          // we should be playing his music
          // incorporate priority later
          listenToMe = x;
        }
      }
      if (listenToMe != nullptr)
      {
        g_closestMusicNode = nullptr;
        if (g_currentMusicPlayingEntity != listenToMe)
        {
          Mix_FadeOutMusic(200);
          // Mix_PlayMusic(listenToMe->theme, 0);
          Mix_VolumeMusic(g_music_volume * 128);
          entFadeFlag = 1;
          fadeFlag = 0;
          musicFadeTimer = 0;
          g_currentMusicPlayingEntity = listenToMe;
        }
      }
      else
      {
        bool hadEntPlayingMusic = 0;
        if (g_currentMusicPlayingEntity != nullptr)
        {
          // stop ent music
          // Mix_FadeOutMusic(1000);
          hadEntPlayingMusic = 1;
          g_currentMusicPlayingEntity = nullptr;
        }
        if (g_musicNodes.size() > 0 && !g_mute)
        {
          newClosest = protag->Get_Closest_Node(g_musicNodes);
          if (g_closestMusicNode == nullptr)
          {
            if (!hadEntPlayingMusic)
            {
              Mix_PlayMusic(newClosest->blip, -1);
              Mix_VolumeMusic(g_music_volume * 128);
              g_closestMusicNode = newClosest;
            }
            else
            {
              g_closestMusicNode = newClosest;
              // change music
              Mix_FadeOutMusic(1000);
              musicFadeTimer = 0;
              fadeFlag = 1;
              entFadeFlag = 0;
            }
          }
          else
          {

            // Segfaults, todo is initialize these musicNodes to have something
            if (newClosest->name != g_closestMusicNode->name)
            {
              // D(newClosest->name);
              // if(newClosest->name == "silence") {
              // Mix_FadeOutMusic(1000);
              //}
              g_closestMusicNode = newClosest;
              // change music
              Mix_FadeOutMusic(1000);
              musicFadeTimer = 0;
              fadeFlag = 1;
              entFadeFlag = 0;
            }
          }
        }
      }
      // check for any cues
      for (auto x : g_cueSounds)
      {
        if (x->played == 0 && Distance(x->x, x->y, protag->x + protag->width / 2, protag->y) < x->radius)
        {
          x->played = 1;
          playSound(-1, x->blip, 0);
        }
      }
    }
    if (fadeFlag && musicFadeTimer > 1000 && newClosest != 0)
    {
      fadeFlag = 0;
      Mix_HaltMusic();
      Mix_FadeInMusic(newClosest->blip, -1, 1000);
    }
    if (entFadeFlag && musicFadeTimer > 200)
    {
      entFadeFlag = 0;
      Mix_HaltMusic();
      Mix_FadeInMusic(g_currentMusicPlayingEntity->theme, -1, 200);
    }
  }
}
B("Music update");

// wakeup manager if it is sleeping
if (adventureUIManager->sleepflag)
{
  adventureUIManager->continueDialogue();
}


//PUNISHMENT!
punishValue += punishValueDegrade;
punishValueDegrade = (punishValueDegrade*0.9 + basePunishValueDegrade*0.1);

if(punishValue > 1) {
  SDL_GL_SetSwapInterval(0);
  if(rng(0,1) == 1) {
    SDL_Delay(pow(rng(1,punishValue),2));
  }
} else {
  SDL_GL_SetSwapInterval(1);
}

g_spin_entity->setOriginX(protag->getOriginX() + protag->xvel * ((double)elapsed/256.0));
g_spin_entity->setOriginY(protag->getOriginY() + protag->yvel * ((double)elapsed/256.0));
g_spin_entity->z = protag->z;


if (freecamera)
{
  g_camera.update_movement(elapsed, camx, camy);
}
else
{
  float zoomoffsetx = ((float)WIN_WIDTH / 2) / g_zoom_mod;
  float zoomoffsety = ((float)WIN_HEIGHT / 2) / g_zoom_mod;
  // g_camera.zoom = 0.9;

  if(g_hog == 0) {
    g_camera.update_movement(elapsed, g_focus->getOriginX() - zoomoffsetx, ((g_focus->getOriginY() - XtoZ * g_focus->z) - zoomoffsety));
  } else {
    int avgX = g_focus->getOriginX() + g_hog->getOriginX(); avgX *= 0.5;
    int avgY = g_focus->getOriginY() + g_hog->getOriginY(); avgY *= 0.5;
    g_camera.update_movement(elapsed, avgX - zoomoffsetx, ((avgY - XtoZ * g_focus->z) - zoomoffsety));
  }
}


// tiles
for (long long unsigned int i = 0; i < g_tiles.size(); i++)
{
  if (g_tiles[i]->z == 0)
  {
    g_tiles[i]->render(renderer, g_camera);
  }
}

for (long long unsigned int i = 0; i < g_tiles.size(); i++)
{
  if (g_tiles[i]->z == 1)
  {
    g_tiles[i]->render(renderer, g_camera);
  }
}

//meshes
for(auto &x : g_meshFloors) {
  if(x->visible) {
    SDL_Vertex v[x->numVertices];
    for(int i = 0; i < x->numVertices; i++) {
      v[i] = x->vertex[i];
      v[i].position.x += x->origin.x - g_camera.x;
      v[i].position.y += x->origin.y - g_camera.y;
    }
  
    SDL_RenderGeometry(renderer, x->texture, v, x->numVertices, NULL, 0);

    //render shade
    for(int i = 0; i < x->numVertices; i++) {
      v[i].tex_coord.x = x->vertexExtraData[i].first;
      v[i].tex_coord.y = x->vertexExtraData[i].second;
    }
    
    SDL_RenderGeometry(renderer, g_meshShadeTexture, v, x->numVertices, NULL, 0);
  
  }
}


{
  //the render target will break the effect and it's additional overhead so w/e
//  SDL_SetRenderTarget(renderer, g_occluderTarget);
//  SDL_SetRenderDrawColor(renderer, 255,255,255,255);
//  SDL_RenderClear(renderer);
  for (auto &o : g_meshOccluders) {
      SDL_Vertex v[o->numVertices];
      o->oGeo.clear();
      for (int i = 0; i < o->numVertices; i++) {
          v[i] = o->vertex[i];
          v[i].position.x += o->origin.x;
          v[i].position.y += o->origin.y;
  
          // (v[i].position.x, v[i].position.y, v[i].tex_coord.y)
          // is the position in the world
  
          // an occluder should have the "top" vertices of the belt have its first x texture coord less than 0.5
          if (v[i].color.r > 128) {
              // move this point to the edge of the screen
              SDL_FPoint intersection = calculateIntersection(protag->getOriginX(), protag->getOriginY(), protag->z, v[i].position.x, v[i].position.y, v[i].tex_coord.y);
              //D(v[i].position.y);
              v[i].position.x = intersection.x;
              v[i].position.y = intersection.y;
              v[i].position.y -= v[i].tex_coord.y * XtoZ;
          }  
          //now use coordinates for rendering
          v[i].position.x += -g_camera.x;
          v[i].position.y += -g_camera.y;
          o->oGeo.push_back(v[i]);
          //o->oGeo[o->oGeo.size()-1].color.r = 120;

          v[i].color.r = 0;
          v[i].color.g = 0;
          v[i].color.b = 0;
  
//          v[i].position.x *= g_occluderResolutionRatio;
//          v[i].position.y *= g_occluderResolutionRatio;
      }
  
  
      SDL_RenderGeometry(renderer, NULL, v, o->numVertices, NULL, 0);
  }
//  SDL_SetRenderTarget(renderer, nullptr);
//  SDL_SetRenderDrawColor(renderer, 0,0,0,255);
//  SDL_RenderCopy(renderer, g_occluderTarget, NULL, NULL);
  
}

//visual walls
for(auto &x : g_meshVWalls) {
  if(x->visible) {
    SDL_Vertex v[x->numVertices];
    for(int i = 0; i < x->numVertices; i++) {
      v[i] = x->vertex[i];
      v[i].position.x += x->origin.x - g_camera.x;
      v[i].position.y += x->origin.y - g_camera.y;
      v[i].color.r = 255;
    }
  
    SDL_RenderGeometry(renderer, x->texture, v, x->numVertices, NULL, 0);
  }
}

//render occlude on visual walls
{
  for(auto o : g_meshOccluders) {
    for(int i = 0; i < o->oGeo.size(); i+=3) {

      for(auto w : g_meshVWalls) {
        for(int j = 0; j < w->numVertices; j+= 3) {

          //make the bottom loop of the wall have 0 red
          //only check if we can find two teal verts
          SDL_Vertex d = w->vertex[j];
          SDL_Vertex e = w->vertex[j+1];
          SDL_Vertex f = w->vertex[j+2];
          d.position.x += w->origin.x - g_camera.x;
          d.position.y += w->origin.y - g_camera.y;

          e.position.x += w->origin.x - g_camera.x;
          e.position.y += w->origin.y - g_camera.y;

          f.position.x += w->origin.x - g_camera.x;
          f.position.y += w->origin.y - g_camera.y;

          SDL_Vertex* useA = nullptr;
          SDL_Vertex* useB = nullptr;

          if(d.color.r < 128) {
            if( e.color.r < 128) {
              useA = &d;
              useB = &e;
            } else if(f.color.r < 128) {
              useA = &d;
              useB = &f;
            }
          } else if (e.color.r < 128) {
            if( f.color.r < 128) {
              useA = &e;
              useB = &f;
            }
          }
          if(useA == nullptr) {continue;}

          SDL_Vertex protagPoint;
          protagPoint.x = protag->getOriginX() - g_camera.x;
          protagPoint.y = protag->getOriginY() - g_camera.y;

          vector<SDL_Vertex> primaryPoints;
          vector<SDL_Vertex> secondaryPoints;
          
  
          SDL_Vertex a = o->oGeo[i];
          SDL_Vertex b = o->oGeo[i+1];
          SDL_Vertex c = o->oGeo[i+2];

          //check R-B lines
          if(a.color.r > 128) {
            if(b.color.r < 128) {
              M("Check ab seg A");
              //check a-b seg
              SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
              SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
              SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
              SDL_RenderDrawLine(renderer, static_cast<int>(a.position.x), static_cast<int>(a.position.y), static_cast<int>(b.position.x), static_cast<int>(b.position.y));


              auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, a.position.x, a.position.y, b.position.x, b.position.y);
              SDL_Rect dstrect = {ix, iy, 20, 20};
              if(intersects) {
                primaryPoints.push_back({ix,iy});
                secondaryPoints.push_back({ix,0});
              } else {
                if(a.color.r > 128) {
                  SDL_Vertex vert; 
                  vert.x = a.position.x;
                  vert.y = a.position.y;
                  primaryPoints.push_back(vert);
                } else {
                  SDL_Vertex vert; 
                  vert.x = b.position.x;
                  vert.y = b.position.y;
                  primaryPoints.push_back(vert);
                }
              }
              SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);
            }
            if(c.color.r < 128) {
              M("Check ac seg A");
              //check a-c seg
              SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
              SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
              SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
              SDL_RenderDrawLine(renderer, static_cast<int>(a.position.x), static_cast<int>(a.position.y), static_cast<int>(c.position.x), static_cast<int>(c.position.y));


              auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, a.position.x, a.position.y, c.position.x, c.position.y);
              SDL_Rect dstrect = {ix, iy, 20, 20};
              if(a.color.r > 128) {
                SDL_Vertex vert; 
                vert.x = a.position.x;
                vert.y = a.position.y;
                primaryPoints.push_back(vert);
              } else {
                SDL_Vertex vert; 
                vert.x = c.position.x;
                vert.y = c.position.y;
                primaryPoints.push_back(vert);
              }
              SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);
            }
          } else {
            if(b.color.r > 128) {
              M("Check ab seg B");
              //check a-b seg
              SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
              SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
              SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
              SDL_RenderDrawLine(renderer, static_cast<int>(a.position.x), static_cast<int>(a.position.y), static_cast<int>(b.position.x), static_cast<int>(b.position.y));


              auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, a.position.x, a.position.y, b.position.x, b.position.y);
              SDL_Rect dstrect = {ix, iy, 20, 20};
              if(intersects) {
                primaryPoints.push_back({ix,iy});
                secondaryPoints.push_back({ix,0});
              } else {
                if(a.color.r > 128) {
                  primaryPoints.push_back({a.position.x, a.position.y});
                } else {
                  primaryPoints.push_back({b.position.x, b.position.y});
                }
              }
              SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);

            }
            if(c.color.r > 128) {
              M("Check ac seg B");
              //check a-c seg
              SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
              SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
              SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
              SDL_RenderDrawLine(renderer, static_cast<int>(a.position.x), static_cast<int>(a.position.y), static_cast<int>(c.position.x), static_cast<int>(c.position.y));


              auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, a.position.x, a.position.y, c.position.x, c.position.y);
              SDL_Rect dstrect = {ix, iy, 20, 20};
              if(intersects) {
                primaryPoints.push_back({ix,iy});
                secondaryPoints.push_back({ix,0});
              } else {
                if(a.color.r > 128) {
                  primaryPoints.push_back({a.position.x, a.position.y});
                } else {
                  primaryPoints.push_back({b.position.x, b.position.y});
                }
              }
              SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);
            }
          }
          if(b.color.r > 128 && c.color.r < 128) {
            M("Check bc seg");
            //check b-c seg
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
            SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
            SDL_RenderDrawLine(renderer, static_cast<int>(b.position.x), static_cast<int>(b.position.y), static_cast<int>(c.position.x), static_cast<int>(c.position.y));


            auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, b.position.x, b.position.y, c.position.x, c.position.y);
            SDL_Rect dstrect = {ix, iy, 20, 20};
              if(intersects) {
                primaryPoints.push_back({ix,iy});
                secondaryPoints.push_back({ix,0});
              } else {
                if(a.color.r > 128) {
                  primaryPoints.push_back({a.position.x, a.position.y});
                } else {
                  primaryPoints.push_back({b.position.x, b.position.y});
                }
              }
            SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);
    
          } else if (b.color.r < 218 && c.color.r > 128) {
            M("Check bc seg");
            //check b-c seg
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for intersecting segment
            SDL_RenderDrawLine(renderer, static_cast<int>(useA->position.x), static_cast<int>(useA->position.y), static_cast<int>(useB->position.x), static_cast<int>(useB->position.y));
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the intersected segment
            SDL_RenderDrawLine(renderer, static_cast<int>(b.position.x), static_cast<int>(b.position.y), static_cast<int>(c.position.x), static_cast<int>(c.position.y));


            auto [intersects, ix, iy] = getIntersection(useA->position.x, useA->position.y, useB->position.x, useB->position.y, b.position.x, b.position.y, c.position.x, c.position.y);
            SDL_Rect dstrect = {ix, iy, 20, 20};
              if(intersects) {
                primaryPoints.push_back({ix,iy});
                secondaryPoints.push_back({ix,0});
              } else {
                if(a.color.r > 128) {
                  primaryPoints.push_back({a.position.x, a.position.y});
                } else {
                  primaryPoints.push_back({b.position.x, b.position.y});
                }
              }
            SDL_RenderCopy(renderer, blackbarTexture, NULL, &dstrect);
            
          }

          if(primaryPoints.size() == 2) { //should always be true
            protagPoint;
            primaryPoints[0];
            primaryPoints[1];

            if(secondaryPoints

          } else {
            E("Error drawing occluder over wall");
          }
        }
      }

    }
  }
}


if(drawhitboxes) {
  for(auto &x : g_meshCollisions) {
    if(x->visible) {
      SDL_Vertex v[x->numVertices];
      for(int i = 0; i < x->numVertices; i++) {
        v[i] = x->vertex[i];
        v[i].position.x += x->origin.x - g_camera.x;
        v[i].position.y += x->origin.y - g_camera.y;
      }
    
      SDL_RenderGeometry(renderer, x->texture, v, x->numVertices, NULL, 0);
  
      }
  }
}

// sort
sort_by_y(g_actors);
for (long long unsigned int i = 0; i < g_actors.size(); i++)
{
  g_actors[i]->render(renderer, g_camera);
}

for (long long unsigned int i = 0; i < g_tiles.size(); i++)
{
  if (g_tiles[i]->z == 2)
  {
    g_tiles[i]->render(renderer, g_camera);
  }
}
B("After tiles");


//render black bars
if(!devMode) {
//occluders

  SDL_Rect blackrect;

  blackrect = {
    g_camera.desiredX - g_camera.width,
    g_camera.desiredY - g_camera.height,
    g_camera.width,
    g_camera.height*3
  };


  blackrect = transformRect(blackrect);

  SDL_RenderCopy(renderer, blackbarTexture, NULL, &blackrect);

  blackrect = {
    g_camera.desiredX + g_camera.width,
    g_camera.desiredY - g_camera.height,
    g_camera.width,
    g_camera.height*3
  };


  blackrect = transformRect(blackrect);

  SDL_RenderCopy(renderer, blackbarTexture, NULL, &blackrect);

  blackrect = {
    g_camera.desiredX,
    g_camera.desiredY - g_camera.height,
    g_camera.width,
    g_camera.height
  };

  blackrect = transformRect(blackrect);

  SDL_RenderCopy(renderer, blackbarTexture, NULL, &blackrect);

  blackrect = {
    g_camera.desiredX,
    g_camera.desiredY + g_camera.height,
    g_camera.width,
    g_camera.height
  };

  blackrect = transformRect(blackrect);

  SDL_RenderCopy(renderer, blackbarTexture, NULL, &blackrect);

  blackrect = {
    g_camera.desiredX,
    g_camera.desiredY,
    g_camera.width,
    g_camera.height
  };

  blackrect = transformRect(blackrect);
  SDL_RenderCopy(renderer, spotlightTexture, NULL, &blackrect);

}


//shade
SDL_RenderCopy(renderer, g_shade, NULL, NULL);

drawUI();

//render fancybox
g_fancybox->render();
g_fancybox->update(elapsed);


// settings menu
if (g_inSettingsMenu) 
{
  //move reticle to the correct position
  if(!g_settingsUI->cursorIsOnBackButton) {
    g_settingsUI->handMarker->targety
      = g_settingsUI->optionTextboxes[g_settingsUI->positionOfCursor]->boxY
      + (g_settingsUI->handOffset);

    g_settingsUI->handMarker->targetx
      = g_settingsUI->markerHandX;

    g_settingsUI->fingerMarker->targety
      = g_settingsUI->optionTextboxes[g_settingsUI->positionOfCursor]->boxY
      + (g_settingsUI->fingerOffset);

    g_settingsUI->fingerMarker->targetx
      = g_settingsUI->markerFingerX;


  } else {
    float ww = WIN_WIDTH;
    float wh = WIN_HEIGHT;

    g_settingsUI->fingerMarker->targetx = g_settingsUI->bbNinePatch->x + (g_settingsUI->markerBBOffset);
    g_settingsUI->fingerMarker->targety = g_settingsUI->bbNinePatch->y + (g_settingsUI->markerBBOffsetY * (ww/wh));

    g_settingsUI->handMarker->targetx = g_settingsUI->bbNinePatch->x + (g_settingsUI->markerBBOffset);
    g_settingsUI->handMarker->targety = g_settingsUI->bbNinePatch->y + (g_settingsUI->markerBBOffsetY * (ww/wh));
  }

  if(g_firstFrameOfSettingsMenu) {
    g_firstFrameOfSettingsMenu = 0;
    g_settingsUI->handMarker->x = g_settingsUI->handMarker->targetx;
    g_settingsUI->handMarker->y = g_settingsUI->handMarker->targety;
    g_settingsUI->fingerMarker->x = g_settingsUI->fingerMarker->targetx;
    g_settingsUI->fingerMarker->y = g_settingsUI->fingerMarker->targety;

  }

}

//this is the menu for quitting or going back to the "overworld"
if (g_inEscapeMenu) 
{
  elapsed = 0;
  //move reticle to the correct position
  g_escapeUI->handMarker->targety
    = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
    + (g_escapeUI->handOffset);

  g_escapeUI->handMarker->targetx
    = g_escapeUI->markerHandX;

  g_escapeUI->fingerMarker->targety
    = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxY
    + (g_escapeUI->fingerOffset);

  float ww = WIN_WIDTH;
  float fwidth = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->width;
  g_escapeUI->fingerMarker->targetx
    = g_escapeUI->optionTextboxes[g_escapeUI->positionOfCursor]->boxX + 
    fwidth / ww / 2;



  if(g_firstFrameOfSettingsMenu) {
    g_firstFrameOfSettingsMenu = 0;
    g_escapeUI->handMarker->x = g_escapeUI->handMarker->targetx;
    g_escapeUI->handMarker->y = g_escapeUI->handMarker->targety;
    g_escapeUI->fingerMarker->x = g_escapeUI->fingerMarker->targetx;
    g_escapeUI->fingerMarker->y = g_escapeUI->fingerMarker->targety;

  }

}

// draw pause screen
if (inPauseMenu)
{
  adventureUIManager->crosshair->x = 5;

  // iterate thru inventory and draw items on screen
  float defaultX = WIN_WIDTH * 0.05;
  float defaultY = WIN_HEIGHT * adventureUIManager->inventoryYStart;
  float x = defaultX;
  float y = defaultY;
  float maxX = WIN_WIDTH * 0.9;
  float maxY = WIN_HEIGHT * adventureUIManager->inventoryYEnd;
  float itemWidth = WIN_WIDTH * 0.07;
  float padding = WIN_WIDTH * 0.01;

  int i = 0;

  if (g_inventoryUiIsLevelSelect == 0) {
    if(g_inventoryUiIsKeyboard == 1) {
      //draw a letter in each box and append to a string


      for(int j = 0; j < g_alphabet.size(); j++) {
        if( i < itemsPerRow * inventoryScroll) {
          i++;
          continue;
        }

        SDL_Rect drect;
        if(g_alphabet == g_alphabet_lower) {
          drect = {(int)x + (0.02 * WIN_WIDTH) - (g_alphabet_widths[i] * itemWidth/230) , (int)y, (int)itemWidth * (g_alphabet_widths[i] / 60), (int)itemWidth}; 
        } else {
          drect = {(int)x + (0.02 * WIN_WIDTH) - (g_alphabet_widths[i] * itemWidth/230), (int)y, (int)itemWidth * (g_alphabet_upper_widths[i] / 60), (int)itemWidth}; 
        }

        // draw the ith letter of "alphabet" in drect
        if(1) {
          SDL_Rect shadowRect = drect;
          float booshAmount = g_textDropShadowDist  * (60 * g_fontsize);
          shadowRect.x += booshAmount;
          shadowRect.y += booshAmount;
          SDL_SetTextureColorMod(g_alphabet_textures->at(i), g_textDropShadowColor,g_textDropShadowColor,g_textDropShadowColor);
          SDL_RenderCopy(renderer, g_alphabet_textures->at(i), NULL, &shadowRect);
          SDL_SetTextureColorMod(g_alphabet_textures->at(i), 255,255,255);
        }
        SDL_RenderCopy(renderer, g_alphabet_textures->at(i), NULL, &drect);


        if (i == inventorySelection || g_firstFrameOfPauseMenu)
        {
          // this item should have the marker
          inventoryMarker->show = 1;
          float biggen = 0; // !!! resolutions : might have problems with diff resolutions

          if(g_firstFrameOfPauseMenu) {
            inventoryMarker->x = x / WIN_WIDTH;
            inventoryMarker->y = y / WIN_HEIGHT;
            inventoryMarker->x -= biggen;
            inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            //now that it's a hand
            inventoryMarker->x += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            inventoryMarker->targetx = inventoryMarker->x;
            inventoryMarker->targety = inventoryMarker->y;
            g_firstFrameOfPauseMenu = 0;
          } else {
            inventoryMarker->targetx = x / WIN_WIDTH;
            inventoryMarker->targety = y / WIN_HEIGHT;
            inventoryMarker->targetx -= biggen;
            inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            //now that it's a hand
            inventoryMarker->targetx += 0.015 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
            inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          }

          inventoryMarker->width = itemWidth / WIN_WIDTH;

          inventoryMarker->width += biggen * 2;
          //inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
        }

        x += itemWidth + padding;
        if (x > maxX)
        {
          x = defaultX;
          y += itemWidth + padding;
          if (y > maxY)
          {
            // we filled up the entire inventory, so lets leave
            break;
          }
        }
        i++;

      }

      //draw current input in the bottom box
      adventureUIManager->inputText->updateText(g_keyboardInput.c_str(), -1, 0.9);
      adventureUIManager->escText->updateText(adventureUIManager->keyboardPrompt, -1, 0.9);


      g_itemsInInventory = g_alphabet.size();


    }
  } else {
    //populate the UI based on the loaded level sequence.
    for(int j = 0; j < g_levelSequence->levelNodes.size(); j++) {
      if( i < itemsPerRow * inventoryScroll) {
        i++;
        continue;
      }
      SDL_Rect drect = {(int)x, (int)y, (int)itemWidth, (int)itemWidth}; 
      int boosh = 5;
      drect.w += boosh * 2;
      drect.h += boosh * 2;
      drect.x -= boosh;
      drect.y -= boosh;

      levelNode* tn = g_levelSequence->levelNodes[j];

      //should we draw the locked graphic?
      if(tn->locked) {

        SDL_RenderCopy(renderer, g_locked_level_texture, NULL, &drect);

        //render the face
        SDL_RenderCopy(renderer, tn->mouthTexture, NULL, &drect);

        SDL_Rect srect = tn->getEyeRect();
        SDL_RenderCopy(renderer, tn->eyeTexture, &srect, &drect);
        g_levelSequence->levelNodes[j]->blinkCooldownMS -= 16;
        if(tn->blinkCooldownMS < 0) { tn->blinkCooldownMS = rng(tn->minBlinkCooldownMS, tn->maxBlinkCooldownMS); }
      } else {
        SDL_RenderCopy(renderer, tn->sprite, NULL, &drect);
      }


      if (i == inventorySelection)
      {

        if(g_levelSequence->levelNodes[i]->locked) {
          adventureUIManager->escText->updateText("Locked", -1, 0.9);
        } else {
          string dispText = g_levelSequence->levelNodes[i]->name;
          std::replace(dispText.begin(), dispText.end(),'_',' ');
          adventureUIManager->escText->updateText(g_levelSequence->levelNodes[i]->name, -1, 0.9);
        }

        // this item should have the marker
        inventoryMarker->show = 1;
        float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions

        if(g_firstFrameOfPauseMenu) {
          inventoryMarker->x = x / WIN_WIDTH;
          inventoryMarker->y = y / WIN_HEIGHT;
          inventoryMarker->x -= biggen;
          inventoryMarker->y -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          //now that it's a hand
          inventoryMarker->x += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          inventoryMarker->y += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          inventoryMarker->targetx = inventoryMarker->x;
          inventoryMarker->targety = inventoryMarker->y;
          g_firstFrameOfPauseMenu = 0;
        } else {
          inventoryMarker->targetx = x / WIN_WIDTH;
          inventoryMarker->targety = y / WIN_HEIGHT;
          inventoryMarker->targetx -= biggen;
          inventoryMarker->targety -= biggen * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          //now that it's a hand
          inventoryMarker->targetx += 0.02 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
          inventoryMarker->targety += 0.03 * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
        }

        inventoryMarker->width = itemWidth / WIN_WIDTH;

        inventoryMarker->width += biggen * 2;
        inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH / (float)WIN_HEIGHT);
      }

      x += itemWidth + padding;
      if (x > maxX)
      {
        x = defaultX;
        y += itemWidth + padding;
        if (y > maxY)
        {
          // we filled up the entire inventory, so lets leave
          break;
        }
      }
      i++;

    }
    g_itemsInInventory = g_levelSequence->levelNodes.size();

  }

  //re-render inventory reticle so it goes on top of the items/level icons
  inventoryMarker->render(renderer, g_camera, 0);
  inventoryMarker->show = 0;

}
else
{
  inventoryMarker->show = 0;
  inventoryText->show = 0;
}

// map editing
if (devMode)
{
  nodeInfoText->textcolor = {0, 0, 0};
  nodeInfoText->show = 1;

  // draw nodes
  for (long long unsigned int i = 0; i < g_worldsounds.size(); i++)
  {
    SDL_Rect obj = {(int)((g_worldsounds[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_worldsounds[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
    SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);

    SDL_Rect textrect = {(int)(obj.x), (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

    //SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_worldsounds[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
    //        SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);
    //
    //        SDL_RenderCopy(renderer, texttexture, NULL, &textrect);
    //
    //        SDL_FreeSurface(textsurface);
    //        SDL_DestroyTexture(texttexture);

    nodeInfoText->x = obj.x;
    nodeInfoText->y = obj.y - 20;
    nodeInfoText->updateText(g_worldsounds[i]->name, -1, 15);
    nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  }


  //draw precede node(s)
  if(precedeProtagNode != nullptr) {
    SDL_Rect obj = { precedeProtagNode->x, precedeProtagNode->y, 40, 40};

    obj = transformRect(obj);
    SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);
  }

  for (long long unsigned int i = 0; i < g_musicNodes.size(); i++)
  {
    SDL_Rect obj = {(int)((g_musicNodes[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_musicNodes[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
    SDL_RenderCopy(renderer, musicIcon->texture, NULL, &obj);

    SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

    SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_musicNodes[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
    SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

    SDL_FreeSurface(textsurface);
    SDL_DestroyTexture(texttexture);
  }

  for (long long unsigned int i = 0; i < g_cueSounds.size(); i++)
  {
    SDL_Rect obj = {(int)((g_cueSounds[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_cueSounds[i]->y - g_camera.y - 20) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
    SDL_RenderCopy(renderer, cueIcon->texture, NULL, &obj);
    SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

    SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_cueSounds[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
    SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

    SDL_FreeSurface(textsurface);
    SDL_DestroyTexture(texttexture);
  }

  for (long long unsigned int i = 0; i < g_waypoints.size(); i++)
  {
    if(!drawhitboxes) {break;}
    SDL_Rect obj = {(int)((g_waypoints[i]->x - g_camera.x - 20) * g_camera.zoom), (int)(((g_waypoints[i]->y - 20 - g_camera.y - g_waypoints[i]->z * XtoZ) * g_camera.zoom)), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
    SDL_RenderCopy(renderer, waypointIcon->texture, NULL, &obj);
    SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

    nodeInfoText->boxX = (float)obj.x / (float)WIN_WIDTH * g_zoom_mod;
    nodeInfoText->boxY = (float)obj.y / (float) WIN_HEIGHT* g_zoom_mod;
    nodeInfoText->boxX -= 0.02;
    nodeInfoText->boxY -= 0.03;
    nodeInfoText->updateText(g_waypoints[i]->name, -1, 15);
    nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

    //SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, g_waypoints[i]->name.c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
    //SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);
    //nodeInfoText->updateText(g_waypoints[i]->name

    //SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

    //SDL_FreeSurface(textsurface);
    //SDL_DestroyTexture(texttexture);
  }

  for (auto x : g_setsOfInterest)
  {
    for (auto y : x)
    {
      SDL_Rect obj = {(int)((y->x - g_camera.x - 20) * g_camera.zoom), (int)((y->y - g_camera.y - 20) * g_camera.zoom), (int)((40 * g_camera.zoom)), (int)((40 * g_camera.zoom))};
      SDL_RenderCopy(renderer, poiIcon->texture, NULL, &obj);

      SDL_Rect textrect = {(int)obj.x, (int)(obj.y + 20), (int)(obj.w - 15), (int)(obj.h - 15)};

      SDL_Surface *textsurface = TTF_RenderText_Blended_Wrapped(nodeInfoText->font, to_string(y->index).c_str(), {15, 15, 15}, 1 * WIN_WIDTH);
      SDL_Texture *texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

      SDL_RenderCopy(renderer, texttexture, NULL, &textrect);

      SDL_FreeSurface(textsurface);
      SDL_DestroyTexture(texttexture);
    }
  }

  // doors
  for (long long unsigned int i = 0; i < g_doors.size(); i++)
  {
    SDL_Rect obj = {(int)((g_doors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_doors[i]->y - g_camera.y ) * g_camera.zoom)), (int)((g_doors[i]->width * g_camera.zoom)), (int)((g_doors[i]->height * g_camera.zoom))};
    SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj);
    // the wall
    SDL_Rect obj2 = {(int)((g_doors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_doors[i]->y - g_camera.y - (g_doors[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_doors[i]->width * g_camera.zoom)), (int)(((g_doors[i]->zeight - g_doors[i]->z) * XtoZ * g_camera.zoom) + (g_doors[i]->height * g_camera.zoom))};
    //SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj2);
    nodeInfoText->boxX = (float)obj.x / (float)WIN_WIDTH * g_zoom_mod;
    nodeInfoText->boxY = (float)obj.y / (float) WIN_HEIGHT* g_zoom_mod;
    nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, -1, 15);
    nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  }

  for (long long unsigned int i = 0; i < g_dungeonDoors.size(); i++)
  {
    SDL_Rect obj = {(int)((g_dungeonDoors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_dungeonDoors[i]->y - g_camera.y - (128) * XtoZ) * g_camera.zoom)), (int)((g_dungeonDoors[i]->width * g_camera.zoom)), (int)((g_dungeonDoors[i]->height * g_camera.zoom))};
    SDL_RenderCopy(renderer, ddoorIcon->texture, NULL, &obj);
    // the wall
    SDL_Rect obj2 = {(int)((g_dungeonDoors[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_dungeonDoors[i]->y - g_camera.y - (128) * XtoZ) * g_camera.zoom)), (int)((g_dungeonDoors[i]->width * g_camera.zoom)), (int)(((128) * XtoZ * g_camera.zoom) + (g_dungeonDoors[i]->height * g_camera.zoom))};
    SDL_RenderCopy(renderer, ddoorIcon->texture, NULL, &obj2);
  }


  for (long long unsigned int i = 0; i < g_triggers.size(); i++)
  {
    SDL_Rect obj = {(int)((g_triggers[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_triggers[i]->y - g_camera.y - (g_triggers[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_triggers[i]->width * g_camera.zoom)), (int)((g_triggers[i]->height * g_camera.zoom))};
    SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj);
    // the wall
    SDL_Rect obj2 = {(int)((g_triggers[i]->x - g_camera.x) * g_camera.zoom), (int)(((g_triggers[i]->y - g_camera.y - (g_triggers[i]->zeight) * XtoZ) * g_camera.zoom)), (int)((g_triggers[i]->width * g_camera.zoom)), (int)(((g_triggers[i]->zeight - g_triggers[i]->z) * XtoZ * g_camera.zoom) + (g_triggers[i]->height * g_camera.zoom))};
    SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj2);

    nodeInfoText->x = obj.x + 25;
    nodeInfoText->y = obj.y + 25;
    nodeInfoText->updateText(g_triggers[i]->binding, -1, 15);
    nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  }

  // listeners
  for (long long unsigned int i = 0; i < g_listeners.size(); i++)
  {
    SDL_Rect obj = {(int)((g_listeners[i]->x - g_camera.x - 20) * g_camera.zoom), (int)((g_listeners[i]->y - g_camera.y - 20) * g_camera.zoom), (int)(40 * g_camera.zoom), (int)(40 * g_camera.zoom)};
    SDL_RenderCopy(renderer, listenerIcon->texture, NULL, &obj);
    nodeInfoText->x = obj.x;
    nodeInfoText->y = obj.y - 20;
    nodeInfoText->updateText(g_listeners[i]->listenList.size() + " of " + g_listeners[i]->entityName, -1, 15);
    nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  }

  write_map(protag);
  for (int i = 0; i < 50; i++)
  {
    devinput[i] = 0;
  }
  nodeInfoText->show = 0;
}
B("After mapedit");

SDL_RenderPresent(renderer);
B("End of frame");
}


int WinMain()
{
  locale::global(locale(""));
  cout.imbue(locale());


  canSwitchOffDevMode = devMode;
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  TTF_Init();
  PHYSFS_init(NULL);

  window = SDL_CreateWindow("Game",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  //SDL_RenderSetLogicalSize(renderer, WIN_WIDTH, WIN_WIDTH * (9.0f/16.0f));
  SDL_RenderSetLogicalSize(renderer, 16, 10);
  SDL_SetWindowMinimumSize(window, 100, 100);

  SDL_SetWindowPosition(window, 1280, 800);

  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
  SDL_RenderSetIntegerScale(renderer, SDL_FALSE);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3");
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

  SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);

  PHYSFS_ErrorCode errnum = PHYSFS_getLastErrorCode();

  if(devMode || 1) {
    string currentDirectory = getCurrentDir();
    PHYSFS_mount(currentDirectory.c_str(), "/", 1);
  }

  int ret = PHYSFS_mount("resources.a", "/", 0); //to deploy, just zip up the "resources" folder in  the "shipping" directory and change the filetype from .zip to .a

  for(char **i = PHYSFS_getSearchPath(); *i != NULL; i++) {
    printf("[%s] is in the search path.\n", *i);
  }

  if(PHYSFS_exists("resources/static/entities/common/fomm.ent")) {
    M("Archive is present"); //this has worked before! Make sure the exe is in the same directory as the archive file
  } else {
    M("Archive is NOT present");
  }

  if(devMode) {
    generateIndicesFile();
  }

  //language pack
  initLanguageIndices();

  g_graphicsStrings.push_back(getLanguageData("Graphics0"));
  g_graphicsStrings.push_back(getLanguageData("Graphics1"));
  g_graphicsStrings.push_back(getLanguageData("Graphics2"));
  g_graphicsStrings.push_back(getLanguageData("Graphics3"));

  g_affirmStr = getLanguageData("Affirmative");
  g_negStr = getLanguageData("Negative");

  // for brightness
  // reuse texture for transition, cuz why not
  SDL_Texture* brightness_a = loadTexture(renderer, "resources/engine/transition.qoi");

  SDL_Texture* brightness_b_s = loadTexture(renderer, "resources/engine/black-diffuse.qoi");

  // entities will be made here so have them set as created during loadingtime and not arbitrarily during play
  g_loadingATM = 1;


  // set global shadow-texture

  g_shadowTexture = loadTexture(renderer, "resources/engine/shadow.qoi");
  g_shadowTextureAlternate = loadTexture(renderer, "resources/engine/shadow-square.qoi");

  // narrarator holds scripts caused by things like triggers
  narrarator = new entity(renderer, "engine/sp-deity");
  narrarator->tangible = 0;
  narrarator->persistentHidden = 1;
  narrarator->turnToFacePlayer = 0; //idk why this doesn't load as expected

  // for transition
  transitionSurface = loadSurface("resources/engine/transition.qoi");

  transitionTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 300, 300);
  SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);

  //void *transitionPixelReference;
  //int transitionPitch;

  transitionDelta = transitionImageHeight;

  // font
  g_font = "resources/engine/fonts/Rubik-Bold.ttf";
  g_ttf_fontLarge = loadFont(g_font, 60);
  g_ttf_fontMedium = loadFont(g_font, 55);
  g_ttf_fontSmall = loadFont(g_font, 40);
  g_ttf_fontTiny = loadFont(g_font, 20);

  // setup UI
  adventureUIManager = new adventureUI(renderer);
  adventureUIManager->blip = g_ui_voice;
  // The adventureUI class has three major uses:
  // The adventureUIManager points to an instance with the full UI for the player
  // The narrarator->myScriptCaller points to an instance which might have some dialogue, so it needs some ui
  // Many objects have a pointer to an instance which is used to just run scripts, and thus needs no dialgue box
  // To init the UI which is wholey unique to the instance pointed to by the adventureUIManager, we must
  adventureUIManager->initFullUI();

  combatUIManager = new combatUI(renderer);
  combatUIManager->hideAll();

  titleUIManager = new titleUI(renderer);

  lossUIManager = new lossUI();


  if (canSwitchOffDevMode)
  {
    //init_map_writing(renderer);
    // done once, because textboxes aren't cleared during clear_map()
    nodeInfoText = new textbox(renderer, "", 1000* g_fontsize, 50, 50, WIN_WIDTH);
    nodeInfoText->dropshadow = 1;
    nodeInfoText->align = 0;
    g_config = "dev";
    nodeDebug = loadTexture(renderer, "resources/engine/walkerYellow.qoi");
  }

  // set bindings from file
  ifstream bindfile;
  bindfile.open("user/configs/" + g_config + ".cfg");
  string line;
  for (int i = 0; i < 14; i++)
  {
    getline(bindfile, line);
    if(line.back() == '\r') {
      line.pop_back();
    }
    bindings[i] = SDL_GetScancodeFromName(line.c_str());
  }

  // set vsync and g_fullscreen from config
  string valuestr;
  float value;

  // get g_fullscreen
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stoi(valuestr);
  g_fullscreen = value;

  // get bg darkness
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_background_darkness = value;

  // get music volume
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_music_volume = value;

  // get sfx volume
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_sfx_volume = value;

  // get standard textsize
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_fontsize = value;

  // get mini textsize
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_minifontsize = value;

  // transitionspeed
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_transitionSpeed = value;

  // mapdetail
  //  0 -   - ultra low - no lighting, crappy settings for g_tilt_resolution
  //  1 -   -
  //  2 -
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stof(valuestr);
  g_graphicsquality = value;

  //adjustment of brightness
  getline(bindfile, line);
  valuestr = line.substr(line.find(' '), line.length());
  value = stoi(valuestr);
  g_brightness = value;
  g_shade = loadTexture(renderer, "resources/engine/black-diffuse.qoi");
  SDL_SetWindowBrightness(window, g_brightness/100.0 );
  SDL_SetTextureAlphaMod(g_shade, 0);

  g_meshShadeTexture = loadTexture(renderer, "resources/engine/mesh-shade.qoi");

  switch (g_graphicsquality)
  {
    case 0:
      g_TiltResolution = 16;
      g_platformResolution = 55;
      g_unlit = 1;
      break;

    case 1:
      g_TiltResolution = 4;
      g_platformResolution = 11;
      break;
    case 2:
      g_TiltResolution = 2;
      g_platformResolution = 11;
      break;
    case 3:
      g_TiltResolution = 1;
      g_platformResolution = 11;
      break;
  }

  bindfile.close();

  // apply vsync
  SDL_GL_SetSwapInterval(1);

  g_fullscreen = 0; //!!!
                    // apply fullscreen
  if (g_fullscreen)
  {
    SDL_GetCurrentDisplayMode(0, &DM);
    SDL_SetWindowSize(window, DM.w, DM.h);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  }
  else
  {
    SDL_SetWindowFullscreen(window, 0);
  }

  // initialize box matrix z
  for (int i = 0; i < g_layers; i++)
  {
    vector<box *> v = {};
    g_boxs.push_back(v);
  }

  for (int i = 0; i < g_layers; i++)
  {
    vector<tri *> v = {};
    g_triangles.push_back(v);
  }

  for (int i = 0; i < g_layers; i++)
  {
    vector<ramp *> v = {};
    g_ramps.push_back(v);
  }

  for (int i = 0; i < g_numberOfInterestSets; i++)
  {
    vector<pointOfInterest *> v = {};
    g_setsOfInterest.push_back(v);
  }


  //for water effect
  g_wPixels = new Uint32[g_wNumPixels];
  g_wDistort = loadSurface("resources/engine/waterRipple.qoi");
  g_wSpec = loadTexture(renderer, "resources/engine/specular.qoi");
  SDL_SetTextureBlendMode(g_wSpec, SDL_BLENDMODE_ADD);

  // init static resources

  { //init static sounds
    //g_staticSounds.push_back(loadWav("resources/static/sounds/....wav"));
    g_staticSounds.push_back(loadWav("resources/static/sounds/pellet.wav"));
    g_staticSounds.push_back(loadWav("resources/static/sounds/protag-step-1.wav"));
    g_staticSounds.push_back(loadWav("resources/static/sounds/protag-step-2.wav"));
    g_staticSounds.push_back(loadWav("resources/static/sounds/land.wav"));
  }

  g_ui_voice = loadWav("resources/static/sounds/voice-normal.wav");


  g_spurl_entity = new entity(renderer, "common/spurl");
  g_spurl_entity->msPerFrame = 75;
  g_spurl_entity->visible = 0;

  g_chain_entity = new entity(renderer, "common/chain");
  g_chain_entity->msPerFrame = 75;

  if(devMode) {
    g_dijkstraDebugRed = new ui(renderer, "resources/engine/walkerRed.qoi", 0,0,32,32, 3);
    g_dijkstraDebugRed->persistent = 1;
    g_dijkstraDebugRed->worldspace = 1;
    g_dijkstraDebugBlue = new ui(renderer, "resources/engine/walkerBlue.qoi", 0,0,32,32, 3);
    g_dijkstraDebugBlue->persistent = 1;
    g_dijkstraDebugBlue->worldspace = 1;
    g_dijkstraDebugYellow = new ui(renderer, "resources/engine/walkerYellow.qoi", 0,0,32,32, 3);
    g_dijkstraDebugYellow->persistent = 1;
    g_dijkstraDebugYellow->worldspace = 1;
  }

  //init user keyboard
  //render each character of the alphabet to a texture
  //TTF_Font* alphabetfont = 0;
  //alphabetfont = TTF_OpenFont(g_font.c_str(), 60 * g_fontsize);
  //TTF_Font* alphabetfont = loadFont(g_font, 60*g_fontsize, );
  TTF_Font* alphabetfont = g_ttf_fontLarge;
  SDL_Surface* textsurface = 0;
  SDL_Texture* texttexture = 0;
  g_alphabet_textures = &g_alphabetLower_textures;
  for (int i = 0; i < g_alphabet.size(); i++) {
    string letter = "";
    letter += g_alphabet_lower[i];
    bool special = 0;
    if(letter == ";") {
      //load custom enter graphic
      textsurface = loadSurface("resources/static/ui/menu_confirm.qoi");
      special = 1;
    } else if (letter == "<") {
      //load custom backspace graphic
      textsurface = loadSurface("resources/static/ui/menu_back.qoi");
      special = 1;
    } else if (letter == "^") {
      //load custom capslock graphic
      textsurface = loadSurface("resources/static/ui/menu_upper_empty.qoi");
      special = 1;
    } else {
      textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), g_textcolor, 70);
    }
    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);
    if(!special) {
      texW *= 1.1; //gotta boosh out those letters
      g_alphabet_widths.push_back(texW);
    } else {
      g_alphabet_widths.push_back(50);
    }

    //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
    g_alphabetLower_textures.push_back(texttexture);
    SDL_FreeSurface(textsurface);
  }

  for (int i = 0; i < g_alphabet.size(); i++) {
    string letter = "";
    letter += g_alphabet_upper[i];
    bool special = 0;
    if(letter == ";") {
      //load custom enter graphic
      textsurface = loadSurface("resources/static/ui/menu_confirm.qoi");
      special = 1;
    } else if (letter == "<") {
      //load custom backspace graphic
      textsurface = loadSurface("resources/static/ui/menu_back.qoi");
      special = 1;
    } else if (letter == "^") {
      //load custom capslock graphic
      textsurface = loadSurface("resources/static/ui/menu_upper.qoi");
      special = 1;
    } else {
      textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), g_textcolor, 70);
    }
    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);

    if(!special) {
      texW *= 1.1; //gotta boosh out those letters
      g_alphabet_upper_widths.push_back(texW);
    } else {
      g_alphabet_upper_widths.push_back(50);
    }

    //SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_ADD);
    g_alphabetUpper_textures.push_back(texttexture);
    SDL_FreeSurface(textsurface);
  }

  //fancy alphabet
  int fancyIndex = 0;
  SDL_Color white = {255, 255, 255};
  for(char character : g_fancyAlphabetChars) { //not a char
    string letter = "";
    letter += character;

    // add support for special chars here
    textsurface = TTF_RenderText_Blended_Wrapped(alphabetfont, letter.c_str(), white, 70);

    texttexture = SDL_CreateTextureFromSurface(renderer, textsurface);

    int texW = 0;int texH = 0;
    SDL_QueryTexture(texttexture, NULL, NULL, &texW, &texH);

    float texWidth = texW;
    texWidth *= 0.2;

    std::pair<SDL_Texture*, float> imSecond(texttexture, texW);
    g_fancyAlphabet.insert( {fancyIndex, imSecond} );

    g_fancyCharLookup.insert({character, fancyIndex});

    fancyIndex++;

    SDL_FreeSurface(textsurface);
  }

  g_fancybox = new fancybox();
  g_fancybox->bounds.x = 0.05;
  g_fancybox->bounds.x = 0.7;
  g_fancybox->show = 1;


  //init options menu
  g_settingsUI = new settingsUI();

  g_escapeUI = new escapeUI();

  { //load static textures
    string loadSTR = "resources/levelsequence/icons/locked.qoi";
    g_locked_level_texture = loadTexture(renderer, loadSTR);
  }

  //load levelSequence
  g_levelSequence = new levelSequence(g_levelSequenceName, renderer);

  //for dlc/custom content, add extra levels from any file that might be there
  char ** entries = PHYSFS_enumerateFiles("resources/levelsequence");
  char **i;
  for(i = entries; *i != NULL; i++) {
    string fn(*i);
    if(fn.substr(fn.size() - 4, 4) == ".txt"){
      g_levelSequence->addLevels(*i);
    }
  }
  PHYSFS_freeList(entries);

  srand(time(NULL));

  if (devMode)
  {
    // g_transitionSpeed = 10000;

    //    entity* a = new entity(renderer, "common/fomm");
    //    protag = a;
    //    a->essential = 1;
    //    a->inParty = 1;
    //    party.push_back(a);
    //    g_focus = protag;
    //loadSave();

    //string filename = g_levelSequence->levelNodes[0]->mapfilename;

    //    protag->x = 100000;
    //    protag->y = 100000;

    //    filename = "resources/maps/crypt/g.map"; //temporary
    //    g_mapdir = "crypt"; //temporary

    //load_map(renderer, filename,"a");
    //    vector<string> x = splitString(filename, '/');
    //    g_mapdir = x[1];
    //
    //    g_mapdir = "crypt"; //temporary

  }
  else
  {
    SDL_ShowCursor(0);
    //loadSave();
    //    entity* a = new entity(renderer, "common/fomm");
    //    protag = a;
    //    a->essential = 1;
    //    ->inParty = 1;
    //    party.push_back(a);
    //    g_focus = protag;
    //load_map(renderer, "resources/maps/base/start.map","a"); //lol
    //g_levelFlashing = 1;
    //clear_map(g_camera); 
    //g_levelFlashing = 0;
    //load_map(renderer, "resources/maps/base/start.map","a");
  }

  inventoryMarker = new ui(renderer, "resources/static/ui/finger_selector_angled.qoi", 0, 0, 0.1, 0.1, 2);
  inventoryMarker->show = 0;
  inventoryMarker->persistent = 1;
  inventoryMarker->renderOverText = 1;
  inventoryMarker->heightFromWidthFactor = 1;
  inventoryMarker->height = 1;

  g_UiGlideSpeedY = 0.012 * WIN_WIDTH/WIN_HEIGHT;

  inventoryText = new textbox(renderer, "1", 40,  g_fontsize, 0, WIN_WIDTH * 0.2);
  inventoryText->dropshadow = 1;
  inventoryText->show = 0;
  inventoryText->align = 1;

    g_itemsines.push_back( sin(g_elapsed_accumulator / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator - 1400) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 925) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 500) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 600) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 630) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 970) / 300) * 10 + 30);
    g_itemsines.push_back( sin((g_elapsed_accumulator + 1020) / 300) * 10 + 30);


  g_occluderTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH*g_occluderResolutionRatio, WIN_HEIGHT *g_occluderResolutionRatio);

  SDL_SetTextureBlendMode(g_occluderTarget, SDL_BLENDMODE_MOD);

  //light gradients to put near where maps transition
  g_gradient_a = loadTexture(renderer, "resources/engine/fade-a.qoi");
  g_gradient_b = loadTexture(renderer, "resources/engine/fade-b.qoi");
  g_gradient_c = loadTexture(renderer, "resources/engine/fade-c.qoi");
  g_gradient_d = loadTexture(renderer, "resources/engine/fade-d.qoi");
  g_gradient_e = loadTexture(renderer, "resources/engine/fade-e.qoi");
  g_gradient_f = loadTexture(renderer, "resources/engine/fade-f.qoi");
  g_gradient_g = loadTexture(renderer, "resources/engine/fade-g.qoi");
  g_gradient_h = loadTexture(renderer, "resources/engine/fade-h.qoi");
  g_gradient_i = loadTexture(renderer, "resources/engine/fade-i.qoi");
  g_gradient_j = loadTexture(renderer, "resources/engine/fade-j.qoi");

  blackbarTexture = loadTexture(renderer, "resources/engine/black-diffuse.qoi");
  spotlightTexture = loadTexture(renderer, "resources/engine/spotlight.qoi");


  result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
  result_c = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);

  SDL_SetTextureBlendMode(result, SDL_BLENDMODE_MOD);
  SDL_SetTextureBlendMode(result_c, SDL_BLENDMODE_MOD);

  //this is used when spawning in entities
  smokeEffect = new effectIndex("puff", renderer);
  smokeEffect->persistent = 1;

  littleSmokeEffect = new effectIndex("steam", renderer);
  littleSmokeEffect->persistent = 1;

  blackSmokeEffect = new effectIndex("blackpowder", renderer);
  blackSmokeEffect->persistent = 1;

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderPresent(renderer);
  SDL_GL_SetSwapInterval(1);

  // textures for adding operation
  canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
  //canvas_fc = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500); seems to be unused

  light = loadTexture(renderer, "resources/engine/light.qoi");

  lighta = loadTexture(renderer, "resources/engine/lighta.qoi");

  lightb = loadTexture(renderer, "resources/engine/lightb.qoi");

  lightc = loadTexture(renderer, "resources/engine/lightc.qoi");

  lightd = loadTexture(renderer, "resources/engine/lightd.qoi");

  lightaro = loadTexture(renderer, "resources/engine/lightaro.qoi");

  lightbro = loadTexture(renderer, "resources/engine/lightbro.qoi");

  lightcro = loadTexture(renderer, "resources/engine/lightcro.qoi");

  lightdro = loadTexture(renderer, "resources/engine/lightdro.qoi");

  lightari = loadTexture(renderer, "resources/engine/lightari.qoi");

  lightbri = loadTexture(renderer, "resources/engine/lightbri.qoi");

  lightcri = loadTexture(renderer, "resources/engine/lightcri.qoi");

  lightdri = loadTexture(renderer, "resources/engine/lightdri.qoi");

  g_loadingATM = 0;
  transitionDelta = transitionImageHeight;
  transition = 1;


  mesh* m = loadMeshFromPly("test/tunnel", {99279, 99455,0}, 100, meshtype::FLOOR);
  m->texture = loadTexture(renderer, "resources/static/meshes/test/stage.qoi");

  mesh* vw = loadMeshFromPly("test/wall", {99279, 99455,0}, 100, meshtype::V_WALL);
  vw->texture = m->texture;

  mesh* w = loadMeshFromPly("test/stage-collision", {99279, 99455,0}, 100, meshtype::COLLISION);

  mesh* o = loadMeshFromPly("test/stage-occluder", {99279, 99455,0}, 100, meshtype::OCCLUDER);


  while (!quit)
  {
    // some event handling
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_WINDOWEVENT:
          switch (event.window.event)
          {
            case SDL_WINDOWEVENT_RESIZED:
              // we need to reload some (all?) textures
              for (auto x : g_mapObjects)
              {
                if (x->mask_fileaddress != "&")
                {
                  x->reloadTexture();
                }
              }

              // reassign textures for asset-sharers
              for (auto x : g_mapObjects)
              {
                if (x->mask_fileaddress != "&")
                {
                  x->reassignTexture();
                }
              }

              // the same must be done for masked tiles
              for (auto t : g_tiles)
              {
                if (t->mask_fileaddress != "&")
                {
                  t->reloadTexture();
                }
              }

              // reassign textures for any asset-sharers
              for (auto x : g_tiles)
              {
                x->reassignTexture();
              }
              break;
            case SDL_WINDOWEVENT_MOVED:
              g_update_zoom = 1;
              break;
          }
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym)
          {
            case SDLK_TAB:
              //g_holdingCTRL = 1;
              // protag->getItem(a, 1);
              break;
            case SDLK_LALT:
              //g_holdingTAB = 1;
              break;
          }
          if(g_swallowAKey) {
            g_swallowedKey = event.key.keysym.scancode;
            g_swallowAKey = 0;
            g_swallowedAKeyThisFrame = 1;
          } else {
            g_swallowedAKeyThisFrame = 0;
          }

          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym)
          {
            case SDLK_TAB:
              g_holdingCTRL = 0;
              break;
            case SDLK_LALT:
              g_holdingTAB = 0;
              break;
          }
          break;
        case SDL_MOUSEWHEEL:
          if (g_holdingCTRL)
          {
            if (event.wheel.y > 0)
            {
              wallstart -= 64;
            }
            else if (event.wheel.y < 0)
            {
              wallstart += 64;
            }
            if (wallstart < 0)
            {
              wallstart = 0;
            }
            else
            {
              if (wallstart > 64 * g_layers)
              {
                wallstart = 64 * g_layers;
              }
              if (wallstart > wallheight - 64)
              {
                wallstart = wallheight - 64;
              }
            }
          }
          else
          {
            if (event.wheel.y > 0)
            {
              wallheight -= 64;
            }
            else if (event.wheel.y < 0)
            {
              wallheight += 64;
            }
            if (wallheight < wallstart + 64)
            {
              wallheight = wallstart + 64;
            }
            else
            {
              if (wallheight > 64 * g_layers)
              {
                wallheight = 64 * g_layers;
              }
            }
            break;
          }

        case SDL_MOUSEBUTTONUP:
          if(event.button.button == SDL_BUTTON_RIGHT) {
            g_holddelete = 0;
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
          if (event.button.button == SDL_BUTTON_LEFT)
          {
            devinput[3] = 1;
          }
          if (event.button.button == SDL_BUTTON_MIDDLE)
          {
            devinput[10] = 1;
          }
          if (event.button.button == SDL_BUTTON_RIGHT)
          {
            g_holddelete = 1;
            devinput[4] = 1;
          }
          break;

        case SDL_QUIT:
          quit = 1;
          break;
      }
    }

    ticks = SDL_GetTicks();
    //g_globalAccumulator += ticks;
    elapsed = ticks - lastticks;
    lastticks = ticks;
    B("On Tick");

    if(g_entityBenchmarking) {
      g_eu_exec++;

      if(g_eu_exec > 600) {
        g_eu_a /= 600;
        g_eu_b /= 600;
        g_eu_c /= 600;
        g_eu_d /= 600;
        g_eu_e /= 600;
        g_eu_f /= 600;
        g_eu_g /= 600;
        g_eu_h /= 600;
        //calculate multipliers
        if(g_eu_ab == -1) {
          g_eu_ab = g_eu_a;
          g_eu_bb = g_eu_b;
          g_eu_cb = g_eu_c;
          g_eu_db = g_eu_d;
          g_eu_eb = g_eu_e;
          g_eu_fb = g_eu_f;
          g_eu_gb = g_eu_g;
          g_eu_hb = g_eu_h;
        }
        float am = g_eu_a / g_eu_ab;
        float bm = g_eu_b / g_eu_bb;
        float cm = g_eu_c / g_eu_cb;
        float dm = g_eu_d / g_eu_db;
        float em = g_eu_e / g_eu_eb;
        float fm = g_eu_f / g_eu_fb;
        float gm = g_eu_g / g_eu_gb;
        float hm = g_eu_h / g_eu_hb;

        M("----------------");
        M("A region: " + to_string(g_eu_a) + " (" + to_string(am) + "x)");
        M("B region: " + to_string(g_eu_b) + " (" + to_string(bm) + "x)");
        M("C region: " + to_string(g_eu_c) + " (" + to_string(cm) + "x)");
        M("D region: " + to_string(g_eu_d) + " (" + to_string(dm) + "x)");
        M("E region: " + to_string(g_eu_e) + " (" + to_string(em) + "x)");
        M("F region: " + to_string(g_eu_f) + " (" + to_string(fm) + "x)");
        M("G region: " + to_string(g_eu_g) + " (" + to_string(gm) + "x)");
        M("H region: " + to_string(g_eu_h) + " (" + to_string(hm) + "x)");
        g_eu_exec -= 600;
        g_eu_a = 0;
        g_eu_b = 0;
        g_eu_c = 0;
        g_eu_d = 0;
        g_eu_e = 0;
        g_eu_f = 0;
        g_eu_g = 0;
        g_eu_h = 0;
      }
    }
    //D(elapsed);


    // lock time
    elapsed = 16.6666666667;

    switch(g_gamemode) {
      case gamemode::TITLE:
        {
          TitleLoop();
          break;
        }
      case gamemode::EXPLORATION: 
        {
          ExplorationLoop();
          break;
        }
      case gamemode::COMBAT:
        { 
          CombatLoop();
          break;
        }
      case gamemode::LOSS:
        {
          LossLoop();
          break;
        }

    }

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

    switch(g_gamemode) {
      case gamemode::TITLE:
        {
          TitleLoop();
          break;
        }
      case gamemode::EXPLORATION: 
        {
          ExplorationLoop();
          break;
        }
      case gamemode::COMBAT:
        { 
          CombatLoop();
          break;
        }
      case gamemode::LOSS:
        {
          LossLoop();
          break;
        }

    }

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
      SDL_RenderCopy(renderer, frame, NULL, NULL);

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
  //clear_map(g_camera);
  delete adventureUIManager;
  delete combatUIManager;
  delete titleUIManager;
  delete lossUIManager;
  close_map_writing();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_FreeSurface(transitionSurface);
  SDL_DestroyTexture(background);
  IMG_Quit();
  Mix_CloseAudio();
  TTF_Quit();
  PHYSFS_deinit();

  return 0;
}

int interact(float elapsed, entity *protag)
{
  SDL_Rect srect;
  switch (protag->animation)
  {

    case 0:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y -= 55;

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 1:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y -= 30;
      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 30;
      }
      else
      {
        srect.x += 30;
      }

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 2:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 55;
      }
      else
      {
        srect.x += 55;
      }

      srect = transformRect(srect);
      // if(drawhitboxes) {
      // 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
      // 	SDL_RenderFillRect(renderer, &srect);
      // 	SDL_RenderPresent(renderer);
      // 	SDL_Delay(500);
      // }
      break;
    case 3:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y += 30;
      if (protag->flip == SDL_FLIP_NONE)
      {
        srect.x -= 30;
      }
      else
      {
        srect.x += 30;
      }

      srect = transformRect(srect);
      break;
    case 4:
      srect.h = protag->bounds.height;
      srect.w = protag->bounds.width;

      srect.x = protag->getOriginX() - srect.w / 2;
      srect.y = protag->getOriginY() - srect.h / 2;

      srect.y += 55;

      srect = transformRect(srect);
      break;
  }

  for (long long unsigned int i = 0; i < g_entities.size(); i++)
  {

    SDL_Rect hisrect = {(int)g_entities[i]->x + g_entities[i]->bounds.x + 10, (int)g_entities[i]->y + g_entities[i]->bounds.y + 10, (int)g_entities[i]->bounds.width - 20, (int)g_entities[i]->bounds.height - 20};
    hisrect = transformRect(hisrect);

    if (g_entities[i] != protag && RectOverlap(hisrect, srect))
    {
      if(g_entities[i]->tangible && g_entities[i]->identity != 0) {
        specialObjectsInteract(g_entities[i]);
        //can do a special object interaction AND execute a script (but I haven't done it yet)
        g_ignoreInput = 1;
        dialogue_cooldown = 500;
      }
      if (g_entities[i]->tangible && g_entities[i]->sayings.size() > 0 && g_entities[i]->inParty == 0)
      {
        if (g_entities[i]->animlimit != 0)
        {
          g_entities[i]->animate = 1;
        }
        // make ent look at player, if they have the frames

        if(g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 7)
        {
          float xvector = (g_entities[i]->getOriginX()) - (protag->getOriginX());
          float yvector = (g_entities[i]->getOriginY()) - (protag->getOriginY());
          float angle = atan2(yvector, xvector);
          g_entities[i]->flip = SDL_FLIP_NONE;
          if(angle < -7 * M_PI / 8 || angle >= 7 * M_PI / 8) {
            g_entities[i]->animation = 6;
          } else if (angle < 7 * M_PI / 8 && angle >= 5 * M_PI / 8) {
            g_entities[i]->animation = 7;
          } else if (angle < 5 * M_PI / 8 && angle >= 3 * M_PI / 8) {
            g_entities[i]->animation = 0;
          } else if (angle < 3 * M_PI / 8 && angle >= M_PI / 8) {
            g_entities[i]->animation = 1;
          } else if (angle < M_PI / 8 && angle >= - M_PI / 8) {
            g_entities[i]->animation = 2;
          } else if (angle < - M_PI / 8 && angle >= - 3 * M_PI / 8) {
            g_entities[i]->animation = 3;
          } else if (angle < - 3 * M_PI / 8 && angle > - 5 * M_PI / 8) {
            g_entities[i]->animation = 4;
          } else if (angle < - 5 * M_PI / 8 && angle > - 7 * M_PI / 8) {
            g_entities[i]->animation = 5;
          }
        }
        else if (g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 5)
        {

          int xdiff = (g_entities[i]->getOriginX()) - (protag->getOriginX());
          int ydiff = (g_entities[i]->getOriginY()) - (protag->getOriginY());
          int axdiff = (abs(xdiff) - abs(ydiff));
          if (axdiff > 0)
          {
            // xaxis is more important
            g_entities[i]->animation = 2;
            if (xdiff > 0)
            {
              g_entities[i]->flip = SDL_FLIP_NONE;
            }
            else
            {
              g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
            }
          }
          else
          {
            // yaxis is more important
            g_entities[i]->flip = SDL_FLIP_NONE;
            if (ydiff > 0)
            {
              g_entities[i]->animation = 0;
            }
            else
            {
              g_entities[i]->animation = 4;
            }
          }
          if (abs(axdiff) < 45)
          {
            if (xdiff > 0)
            {
              g_entities[i]->flip = SDL_FLIP_NONE;
            }
            else
            {
              g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
            }
            if (ydiff > 0)
            {
              g_entities[i]->animation = 1;
            }
            else
            {
              g_entities[i]->animation = 3;
            }
          }
        }

        //adventureUIManager->blip = g_entities[i]->voice;
        adventureUIManager->blip = g_ui_voice;
        //adventureUIManager->sayings = &g_entities[i]->sayings;
        adventureUIManager->talker = g_entities[i];

        adventureUIManager->dialogue_index = -1;
        adventureUIManager->useOwnScriptInsteadOfTalkersScript = 0;
        g_forceEndDialogue = 0;
        adventureUIManager->continueDialogue();
        g_ignoreInput = 1;
        return 0;
      }
    }
  }

  return 0;
}

void getExplorationInput(float &elapsed)
{
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  for (int i = 0; i < 5; i++)
  {
    oldStaticInput[i] = staticInput[i];
  }

  SDL_PollEvent(&event);

  //don't take input the frame after a mapload to prevent
  //talking to an ent instantly
  //even tho the player didnt press anything
  if(g_ignoreInput) {
    g_ignoreInput = 0;
    return;
  }


  //for portability, use the input[] array to drive controls
  //some actions are not bound, e.g. navigation of the settings menu
  

  if(devMode) {
    if(keystate[SDL_SCANCODE_LALT]) { 
      g_holdingTAB = 1;
    } else {
      g_holdingTAB = 0;
    }
  
    if(keystate[SDL_SCANCODE_TAB]) { 
      g_holdingCTRL = 1;
    } else {
      g_holdingCTRL = 0;
    }
  }

  //menu up 
  if(keystate[SDL_SCANCODE_UP] || keystate[SDL_SCANCODE_W])
  {
    staticInput[0] = 1;
  } else 
  {
    staticInput[0] = 0;
  }
  //menu down 
  if(keystate[SDL_SCANCODE_DOWN] || keystate[SDL_SCANCODE_S])
  {
    staticInput[1] = 1;
  } else 
  {
    staticInput[1] = 0;
  }
  //menu left
  if(keystate[SDL_SCANCODE_LEFT] || keystate[SDL_SCANCODE_A])
  {
    staticInput[2] = 1;
  } else 
  {
    staticInput[2] = 0;
  }
  //menu down 
  if(keystate[SDL_SCANCODE_RIGHT] || keystate[SDL_SCANCODE_D])
  {
    staticInput[3] = 1;
  } else 
  {
    staticInput[3] = 0;
  }
  //menu confirm
  if(keystate[SDL_SCANCODE_Z] || keystate[SDL_SCANCODE_SPACE])
  {
    staticInput[4] = 1;
  } else 
  {
    staticInput[4] = 0;
  }

  //triangle toggle from keyboard
  if(keystate[SDL_SCANCODE_W])
  {
    devinput[10] = 1;
  } 

  if(keystate[bindings[0]]) {
    input[0] = 1;
  } else { 
    input[0] = 0;
  }
  if(keystate[bindings[1]]) {
    input[1] = 1;
  } else { 
    input[1] = 0;
  }
  if(keystate[bindings[2]]) {
    input[2] = 1;
  } else { 
    input[2] = 0;
  }
  if(keystate[bindings[3]]) {
    input[3] = 1;
  } else { 
    input[3] = 0;
  }


  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }
  if (keystate[bindings[11]])
  {
    input[11] = 1;
  }
  else
  {
    input[11] = 0;
  }


  if (keystate[SDL_SCANCODE_W])
  {
    camy -= 4;
  }
  if (keystate[SDL_SCANCODE_A])
  {
    camx -= 4;
  }
  if (keystate[SDL_SCANCODE_S])
  {
    camy += 4;
  }
  if (keystate[SDL_SCANCODE_D])
  {
    camx += 4;
  }
  if (keystate[SDL_SCANCODE_G] && !inputRefreshCanSwitchOffDevMode && canSwitchOffDevMode)
  {
    toggleDevmode();
  }
  if (keystate[SDL_SCANCODE_G])
  {
    inputRefreshCanSwitchOffDevMode = 1;
  }
  else
  {
    inputRefreshCanSwitchOffDevMode = 0;
  }

  protag_can_move = !protag_is_talking;
  if (protag_can_move)
  {
    protag->shooting = 0;
    protag->left = 0;
    protag->right = 0;
    protag->up = 0;
    protag->down = 0;
    g_cameraAimingOffsetXTarget = 0;
    g_cameraAimingOffsetYTarget = 0;

    // normalize g_cameraAimingOffsetTargetVector
    float len = pow(pow(g_cameraAimingOffsetXTarget, 2) + pow(g_cameraAimingOffsetYTarget, 2), 0.5);
    if (!isnan(len) && len != 0)
    {
      g_cameraAimingOffsetXTarget /= len;
      g_cameraAimingOffsetYTarget /= len;
    }



    if (keystate[bindings[10]])
    {
      input[10] = 1;
    }
    else
    {
      input[10] = 0;
    }

    if (keystate[bindings[0]])
    {
      if (inPauseMenu && SoldUIUp <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if(inventorySelection - itemsPerRow >= 0) {
          inventorySelection -= itemsPerRow;

        }
        SoldUIUp = (oldUIUp) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move) {
          protag->move_up();
        }
      }
      if(inPauseMenu) {
        oldUIUp = 1;
      }
    }
    else
    {
      if(inPauseMenu) {
        oldUIUp = 0;
        SoldUIUp = 0;
      }
    }
    SoldUIUp--;

    if (keystate[bindings[1]])
    {
      if (inPauseMenu && SoldUIDown <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if(inventorySelection + itemsPerRow < g_itemsInInventory) {

          if (ceil((float)(inventorySelection + 1) / (float)itemsPerRow) < (g_itemsInInventory / g_inventoryRows))
          {
            inventorySelection += itemsPerRow;
          }
        }
        SoldUIDown = (oldUIDown) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move) {
          protag->move_down();
        }
      }
      if(inPauseMenu) {
        oldUIDown = 1;
      }
    }
    else
    {
      if(inPauseMenu) {
        oldUIDown = 0;
        SoldUIDown = 0;
      }
    }
    SoldUIDown--;

    if (keystate[bindings[2]])
    {
      if (inPauseMenu && SoldUILeft <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if (inventorySelection > 0)
        {
          if (inventorySelection % itemsPerRow != 0)
          {
            inventorySelection--;
          }
        }
        SoldUILeft = (oldUILeft) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }
      else
      {
        if(protag_can_move) {
          protag->move_left();
        } 
      }
      oldUILeft = 1;
    }
    else
    {
      oldUILeft = 0;
      SoldUILeft = 0;
    }
    SoldUILeft--;

    if (keystate[bindings[3]])
    {
      if (inPauseMenu && SoldUIRight <= 0)
      {
        //playSound(1, g_menu_manip_sound, 0);
        if (inventorySelection <= g_itemsInInventory)
        {
          // dont want this to wrap around
          if (inventorySelection % itemsPerRow != itemsPerRow - 1)
          {
            inventorySelection++;
          }
        }
        SoldUIRight = (oldUIRight) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
      }


      else
      {
        if(protag_can_move) {
          protag->move_right();
        }
      }
      oldUIRight = 1;
    }
    else
    {
      oldUIRight = 0;
      SoldUIRight = 0;
    }
    SoldUIRight--;

    // check if the stuff is onscreen
    if (inventorySelection >= (g_inventoryRows * itemsPerRow) + (inventoryScroll * itemsPerRow))
    {
      inventoryScroll++;
    }
    else
    {
      if (inventorySelection < (inventoryScroll * itemsPerRow))
      {
        inventoryScroll--;
      }
    }

    // constrain inventorySelection based on itemsInInventory
    if (inventorySelection > g_itemsInInventory - 1)
    {
      // M(g_itemsInInventory - 1);
      inventorySelection = g_itemsInInventory - 1;
    }

    if (inventorySelection < 0)
    {
      inventorySelection = 0;
    }

  }
  else
  {
    //settings menu
    if(g_inSettingsMenu) {
      //menu up
      if (staticInput[0])
      {
        if(SoldUIUp <= 0)
        {

          if(!g_awaitSwallowedKey && !g_settingsUI->modifyingValue) {
            if(g_settingsUI->positionOfCursor == 0) {
              g_settingsUI->positionOfCursor = g_settingsUI->maxPositionOfCursor;
            } else {
              g_settingsUI->positionOfCursor--;
            }
          }
          SoldUIUp = (oldStaticInput[0]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIUp--;
        }
      } else {
        SoldUIUp = 0; 
      }

      //menu down
      if (staticInput[1]) {
        if(SoldUIDown <= 0 )
        {
          if(!g_awaitSwallowedKey && !g_settingsUI->modifyingValue) { 
            if(g_settingsUI->positionOfCursor == g_settingsUI->maxPositionOfCursor) {
              g_settingsUI->positionOfCursor = 0;
            } else {
              g_settingsUI->positionOfCursor++;
            }
          }
          SoldUIDown = (oldStaticInput[1]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIDown--;
        }
      } else {
        SoldUIDown = 0; 
      }


      //menu left
      if (staticInput[2])
      {
        if(SoldUILeft <= 0)
        {
          if(g_settingsUI->modifyingValue) {
            //modify the value left
            switch(g_settingsUI->positionOfCursor) {
              case 9: {
                        g_music_volume -= g_settingsUI->deltaVolume;
                        if(g_music_volume < g_settingsUI->minVolume) {
                          g_music_volume = g_settingsUI->minVolume;
                        }
                        string content = to_string((int)round(g_music_volume * 100)) + "%";
                        g_settingsUI->valueTextboxes[9]->updateText(content, -1, 1);
                        Mix_VolumeMusic(g_music_volume * 128);
                        break;
                      }
              case 10: {
                         g_sfx_volume -= g_settingsUI->deltaVolume;
                         if(g_sfx_volume < g_settingsUI->minVolume) {
                           g_sfx_volume = g_settingsUI->minVolume;
                         }
                         string content = to_string((int)round(g_sfx_volume * 100))+ "%";
                         g_settingsUI->valueTextboxes[10]->updateText(content, -1, 1);
                         break;
                       }
              case 11: {
                         g_graphicsquality -= g_settingsUI->deltaGraphics;
                         if(g_graphicsquality < g_settingsUI->minGraphics) {
                           g_graphicsquality = g_settingsUI->minGraphics;
                         }
                         g_settingsUI->valueTextboxes[11]->updateText(g_graphicsStrings[g_graphicsquality], -1, 1);
                         break;
                       }
              default: {
                         g_brightness -= g_settingsUI->deltaBrightness;
                         if(g_brightness < g_settingsUI->minBrightness) {
                           g_brightness = g_settingsUI->minBrightness;
                         }
                         string content = to_string((int)round(g_brightness)) + "%";
                         g_settingsUI->valueTextboxes[12]->updateText(content, -1, 1);
                         //SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
                         SDL_SetWindowBrightness(window, g_brightness/100.0 );
                         break;
                       }
            }
          } else {
            if(g_settingsUI->cursorIsOnBackButton && !g_awaitSwallowedKey) {
              g_settingsUI->cursorIsOnBackButton = 0;
              g_settingsUI->positionOfCursor = 0;
            }
          }
          SoldUILeft = (oldStaticInput[2]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUILeft --;
        }
      } else {
        SoldUILeft = 0;
      }

      //menu right
      if (staticInput[3])
      {
        if(SoldUIRight <= 0)
        {
          if(g_settingsUI->modifyingValue) {
            //modify the value right
            switch(g_settingsUI->positionOfCursor) {
              case 9: {
                        g_music_volume += g_settingsUI->deltaVolume;
                        if(g_music_volume > g_settingsUI->maxVolume) {
                          g_music_volume = g_settingsUI->maxVolume;
                        }
                        string content = to_string((int)round(g_music_volume * 100)) + "%";
                        g_settingsUI->valueTextboxes[9]->updateText(content, -1, 1);
                        Mix_VolumeMusic(g_music_volume * 128);
                        break;
                      }
              case 10: {
                         g_sfx_volume += g_settingsUI->deltaVolume;
                         if(g_music_volume > g_settingsUI->maxVolume) {
                           g_music_volume = g_settingsUI->maxVolume;
                         }
                         string content = to_string((int)round(g_sfx_volume * 100)) + "%";
                         g_settingsUI->valueTextboxes[10]->updateText(content, -1, 1);
                         break;
                       }
              case 11: {
                         g_graphicsquality += g_settingsUI->deltaGraphics;
                         if(g_graphicsquality > g_settingsUI->maxGraphics) {
                           g_graphicsquality = g_settingsUI->maxGraphics;
                         }
                         g_settingsUI->valueTextboxes[11]->updateText(g_graphicsStrings[g_graphicsquality], -1, 1);
                         break;
                       }
              default: {
                         g_brightness += g_settingsUI->deltaBrightness;
                         if(g_brightness > g_settingsUI->maxBrightness) {
                           g_brightness = g_settingsUI->maxBrightness;
                         }
                         string content = to_string((int)round(g_brightness)) + "%";
                         g_settingsUI->valueTextboxes[12]->updateText(content, -1, 1);
                         //SDL_SetTextureAlphaMod(g_shade, 255 - ( ( g_brightness/100.0 ) * 255));
                         SDL_SetWindowBrightness(window, g_brightness/100.0 );
                         break;
                       }
            }
          } else {
            if(!g_settingsUI->cursorIsOnBackButton && !g_awaitSwallowedKey) {
              g_settingsUI->cursorIsOnBackButton = 1;
            }
          }
          SoldUIRight = (oldStaticInput[3]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIRight --;
        }
      } else {
        SoldUIRight = 0;
      }

      //menu select
      if (staticInput[4] && !oldStaticInput[4] && !g_swallowedAKeyThisFrame)
      {
        if(g_settingsUI->cursorIsOnBackButton) {
          //save config to file
          {
            ofstream outfile("user/configs/" + g_config + ".cfg");
            for(int i = 0; i < 14; i++) {
              outfile << SDL_GetScancodeName(bindings[i]) << endl;
            }
            outfile << "Fullscreen " << g_fullscreen << endl;
            outfile << "BgDarkness " << g_background_darkness << endl;
            outfile << "MusicVolume " << g_music_volume << endl;
            outfile << "SFXVolume " << g_sfx_volume << endl;
            outfile << "Textsize " << g_fontsize << endl;
            outfile << "Minitextsize " << g_minifontsize << endl;
            outfile << "Transitionspeed " << g_transitionSpeed << endl;
            outfile << "Graphics0lowto3high " << g_graphicsquality << endl;
            outfile << "brightness0lowto100high " << g_brightness << endl;
            outfile.close();


          }


          //continue the script which was running 
          g_inventoryUiIsLevelSelect = 0;
          g_inventoryUiIsKeyboard = 0;
          g_inventoryUiIsLoadout = 0;
          g_inSettingsMenu = 0;
          inPauseMenu = 0;
          g_menuTalkReset = 1;
          g_settingsUI->hide();
          protag_is_talking = 0;

        } else {
          for(int i = 0; i < g_settingsUI->valueTextboxes.size(); i++) {
            //g_settingsUI->valueTextboxes[i]->blinking = 0;
          }


          if(g_settingsUI->positionOfCursor == 0) { //rebind Up
            g_pollForThisBinding = 0; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 0;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 1) { //rebind Down
            g_pollForThisBinding = 1; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 1;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 2) { //rebind Left
            g_pollForThisBinding = 2; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 2;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 3) { //rebind Right
            g_pollForThisBinding = 3; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 3;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 4) { //rebind Jump
            g_pollForThisBinding = 8; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 4;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 5) { //rebind Interact
            g_pollForThisBinding = 11; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 5;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 6) { //rebind Inventory
            g_pollForThisBinding = 12; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 6;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 7) { //rebind Spin/use
            g_pollForThisBinding = 13; 
            g_swallowAKey = 1;
            g_awaitSwallowedKey = 1;
            g_whichRebindValue = 7;
            g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText("_", -1, 1);
            g_settingsUI->uiModifying();
          }

          if(g_settingsUI->positionOfCursor == 8) { //fullscreen
            if(!g_fullscreen) {
              g_settingsUI->valueTextboxes[8]->updateText(g_affirmStr, -1, 0.3);
            } else {
              g_settingsUI->valueTextboxes[8]->updateText(g_negStr, -1, 0.3);
            }
            toggleFullscreen();
          }

          if(g_settingsUI->positionOfCursor == 9) { // Music volume
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 10) { // Sounds volume
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 11) { // Graphics Quality
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }

          if(g_settingsUI->positionOfCursor == 12) { // Brightness
            g_settingsUI->modifyingValue = !g_settingsUI->modifyingValue;
            if(g_settingsUI->modifyingValue) {
              g_settingsUI->uiModifying();
            } else {
              g_settingsUI->uiSelecting();
            }
          }
        }
      }

      //did we swallow a keypress?
      if(g_awaitSwallowedKey && !g_swallowAKey) {
        //there might be a problem with this logic
        bindings[g_pollForThisBinding] = g_swallowedKey;
        g_settingsUI->valueTextboxes[g_whichRebindValue]->updateText(SDL_GetScancodeName(g_swallowedKey), -1, 1);
        g_awaitSwallowedKey = 0;
        g_settingsUI->uiSelecting();
      }

      if(g_settingsUI->positionOfCursor > g_settingsUI->maxPositionOfCursor) {
        g_settingsUI->positionOfCursor = g_settingsUI->maxPositionOfCursor;
      }

      if(g_settingsUI->positionOfCursor < g_settingsUI->minPositionOfCursor) {
        g_settingsUI->positionOfCursor = g_settingsUI->minPositionOfCursor;
      } else {

      }
    }
    if(g_inEscapeMenu) {
      //menu up
      if (staticInput[0])
      {
        if(SoldUIUp <= 0)
        {

          if(!g_awaitSwallowedKey && !g_escapeUI->modifyingValue) {
            if(g_escapeUI->positionOfCursor == 0) {
              g_escapeUI->positionOfCursor = g_escapeUI->maxPositionOfCursor;
            } else {
              g_escapeUI->positionOfCursor--;
            }
          }
          SoldUIUp = (oldStaticInput[0]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIUp--;
        }
      } else {
        SoldUIUp = 0; 
      }

      //menu down
      if (staticInput[1]) {
        if(SoldUIDown <= 0 )
        {
          if(!g_awaitSwallowedKey && !g_escapeUI->modifyingValue) { 
            if(g_escapeUI->positionOfCursor == g_escapeUI->maxPositionOfCursor) {
              g_escapeUI->positionOfCursor = 0;
            } else {
              g_escapeUI->positionOfCursor++;
            }
          }
          SoldUIDown = (oldStaticInput[1]) ? g_inputDelayRepeatFrames : g_inputDelayFrames;
        } else {
          SoldUIDown--;
        }
      } else {
        SoldUIDown = 0; 
      }

      //menu select
      if (staticInput[4] && !oldStaticInput[4] && !g_swallowedAKeyThisFrame)
      {
        g_inEscapeMenu = 0;
        //inPauseMenu = 0;
        protag_is_talking = 0;
        g_menuTalkReset = 1;

        g_escapeUI->hide();

        if(g_escapeUI->positionOfCursor == 0) { //Back

        }

        if(g_escapeUI->positionOfCursor == 1) { //levelselect
                                                //clear all behemoths
          for(auto &x : g_dungeonBehemoths) {
            x.ptr->persistentGeneral = 0;
            x.ptr->hisweapon->persistent = 0;
            for(auto &y : x.ptr->spawnlist) {
              y->persistentGeneral = 0;
            }
          }
          g_dungeonBehemoths.clear();
          clear_map(g_camera);
          //load_map(renderer, "resources/maps/base/g.map", "a");
          //instead of loading the base, open the level-select menu
          g_dungeonDarkness = 0;
          g_inventoryUiIsLevelSelect = 1;
          g_inventoryUiIsKeyboard = 0;
          g_inventoryUiIsLoadout = 0;
          inventorySelection = 0;
          inPauseMenu = 1;
          g_firstFrameOfPauseMenu = 1;
          old_z_value = 1;
          adventureUIManager->escText->updateText("", -1, 0.9);
          adventureUIManager->positionInventory();
          adventureUIManager->showInventoryUI();
          adventureUIManager->hideHUD();

        }

        if(g_escapeUI->positionOfCursor == 3) {
          oldStaticInput[4] = 1;
          g_levelFlashing = 0;
          inPauseMenu = 0;
          Mix_FadeOutMusic(1000);
          clear_map(g_camera);
          //transition = 1;
          g_gamemode = gamemode::TITLE;
          titleUIManager->option = 0;
          titleUIManager->showAll();
          return;
        }

        if(g_escapeUI->positionOfCursor == 2) { //Settings
          g_settingsUI->show();
          g_inSettingsMenu = 1;
          g_firstFrameOfSettingsMenu = 1;
          g_settingsUI->positionOfCursor = 0;
          g_settingsUI->cursorIsOnBackButton = 0;
          protag_is_talking = 1;
        }


      }

      //did we swallow a keypress?
      if(g_awaitSwallowedKey && !g_swallowAKey) {
        //there might be a problem with this logic
        bindings[g_pollForThisBinding] = g_swallowedKey;
        g_awaitSwallowedKey = 0;
        g_escapeUI->uiSelecting();
      }

      if(g_escapeUI->positionOfCursor > g_escapeUI->maxPositionOfCursor) {
        g_escapeUI->positionOfCursor = g_escapeUI->maxPositionOfCursor;
      }

      if(g_escapeUI->positionOfCursor < g_escapeUI->minPositionOfCursor) {
        g_escapeUI->positionOfCursor = g_escapeUI->minPositionOfCursor;
      } else {

      }
    }

    // reset shooting offsets
    g_cameraAimingOffsetXTarget = 0;
    g_cameraAimingOffsetYTarget = 0;

    if (keystate[bindings[2]] && !left_ui_refresh)
    {
      if (adventureUIManager->askingQuestion)
      {
        if (adventureUIManager->response_index > 0)
        {
          adventureUIManager->response_index--;
        }
      }
      left_ui_refresh = 1;
    }
    else if (!keystate[bindings[2]])
    {
      left_ui_refresh = 0;
    }
    if (keystate[bindings[3]] && !right_ui_refresh)
    {
      if (adventureUIManager->askingQuestion)
      {
        adventureUIManager->response_index++;
        if (adventureUIManager->response_index > adventureUIManager->responses.size() - 1)
        {
          adventureUIManager->response_index--;
        }
      }
      right_ui_refresh = 1;
    }
    else if (!keystate[bindings[3]])
    {
      right_ui_refresh = 0;
    }
    protag->stop_hori();
    protag->stop_verti();
  }

  if (keystate[bindings[8]])
  {
    input[8] = 1;
  }
  else
  {
    input[8] = 0;
  }

  if (keystate[bindings[9]])
  {
    input[9] = 1;
  }
  else
  {
    input[9] = 0;
  }

  if (keystate[bindings[12]])
  {
    input[12] = 1;
  }
  else
  {
    input[12] = 0;
  }

  if(keystate[bindings[13]]) 
  {
    input[13] = 1;
  }
  else
  {
    input[13] = 0;
  }


  //spinning/using item
  if(protag_can_move && !inPauseMenu) {
    if(1) { //do a spin

      if ( ((input[13] && !oldinput[13]) || (input[13] && storedSpin) ) && protag_can_move && (protag->grounded || g_currentSpinJumpHelpMs > 0 )
          && (g_spin_cooldown <= 0 && g_spinning_duration <= 0 + g_doubleSpinHelpMs && g_afterspin_duration <= 0 )

         )
      {
        storedSpin = 0;
        //propel the protag in the direction of their velocity
        //at high speed, removing control from them
        g_spinning_duration = g_spinning_duration_max;
        g_spinning_xvel = protag->xvel * g_spinning_boost;
        g_spinning_yvel = protag->yvel * g_spinning_boost;
        g_spin_cooldown = g_spin_max_cooldown;
        g_spin_entity->frameInAnimation = 0;
      } else {
        if(input[13] && !oldinput[13] && !protag->grounded) {
          storedSpin = 1;
        }

      }
      g_currentSpinJumpHelpMs-= elapsed;

      if(protag->grounded) {
        g_currentSpinJumpHelpMs = g_spinJumpHelpMs;
      }

      if(g_spin_enabled && g_spinning_duration >= 0 && g_spinning_duration - elapsed < 0 && protag->grounded && !g_protag_jumped_this_frame) {
        g_afterspin_duration = g_afterspin_duration_max;
      }

      g_spin_entity->x = protag->x;
      g_spin_entity->y = protag->y;
      g_spin_entity->z = protag->z; 

    }
  } 

  g_spin_cooldown -= elapsed;
  g_spinning_duration -= elapsed;
  g_afterspin_duration -= elapsed;

  int markeryvel = 0;
  // mapeditor cursor vertical movement for keyboards
  if (keystate[SDL_SCANCODE_KP_PLUS])
  {
    markeryvel = 1;
  }
  else
  {
    keyboard_marker_vertical_modifier_refresh = 1;
  }

  if (keystate[SDL_SCANCODE_KP_MINUS])
  {
    markeryvel = -1;
  }
  else
  {
    keyboard_marker_vertical_modifier_refresh_b = 1;
  }

  if (markeryvel != 0)
  {
    if (g_holdingCTRL)
    {
      if (markeryvel > 0 && keyboard_marker_vertical_modifier_refresh)
      {
        wallstart -= 64;
      }
      else if (markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b)
      {
        wallstart += 64;
      }
      if (wallstart < 0)
      {
        wallstart = 0;
      }
      else
      {
        if (wallstart > 64 * g_layers)
        {
          wallstart = 64 * g_layers;
        }
        if (wallstart > wallheight - 64)
        {
          wallstart = wallheight - 64;
        }
      }
    }
    else
    {
      if (markeryvel > 0 && keyboard_marker_vertical_modifier_refresh)
      {
        wallheight -= 64;
      }
      else if (markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b)
      {
        wallheight += 64;
      }
      if (wallheight < wallstart + 64)
      {
        wallheight = wallstart + 64;
      }
      else
      {
        if (wallheight > 64 * g_layers)
        {
          wallheight = 64 * g_layers;
        }
      }
    }
  }
  if (keystate[SDL_SCANCODE_KP_PLUS])
  {
    keyboard_marker_vertical_modifier_refresh = 0;
  }

  if (keystate[SDL_SCANCODE_KP_MINUS])
  {
    keyboard_marker_vertical_modifier_refresh_b = 0;
  }

  if (input[11] && !oldinput[11] && !inPauseMenu && !g_inSettingsMenu && !g_inEscapeMenu)
  {
    if (protag_is_talking == 1)
    {
      if (!adventureUIManager->typing)
      {
        adventureUIManager->continueDialogue();
      }
    }
  }
  else if (input[11] && !oldinput[11] && inPauseMenu && !g_firstFrameOfPauseMenu)
  {
    if(g_inventoryUiIsLevelSelect == 0) {

      if(g_inventoryUiIsKeyboard) {
        //append this character to the string

        //handle special keys like caps, backspace, and enter
        if(
            g_alphabet[inventorySelection] == '<' 
            || g_alphabet[inventorySelection] == '^' 
            || g_alphabet[inventorySelection] == ';'

          ) {

          //backspace
          if(g_alphabet[inventorySelection] == '<') {
            if(g_keyboardInput.size() > 0) {
              g_keyboardInput = g_keyboardInput.substr(0, g_keyboardInput.size()-1);
            }
          }

          //caps
          if(g_alphabet[inventorySelection] == '^') {
            if(g_alphabet == g_alphabet_lower) {
              g_alphabet = g_alphabet_upper;
              g_alphabet_textures = &g_alphabetUpper_textures;
            } else {
              g_alphabet = g_alphabet_lower;
              g_alphabet_textures = &g_alphabetLower_textures;
            }
          }

          if(g_alphabet[inventorySelection] == ';' && g_keyboardInput != "") {
            writeSaveFieldString(g_keyboardSaveToField, g_keyboardInput);

            //this will continue the script which was running, even if it just instantly terminates

            g_inventoryUiIsLevelSelect = 0;
            g_inventoryUiIsKeyboard = 0;
            inPauseMenu = 0;
            adventureUIManager->hideInventoryUI();


            adventureUIManager->dialogue_index++;
            adventureUIManager->continueDialogue();
          }


        } else {

          if(g_keyboardInput.size() < g_keyboardInputLength) {
            g_keyboardInput += g_alphabet[inventorySelection];
          }

          //take off caps (add sound if it was on)
          g_alphabet = g_alphabet_lower;
          g_alphabet_textures = &g_alphabetLower_textures;

        }

      }
    } else {
      //if this level is unlocked, travel to its map
      if(g_levelSequence->levelNodes[inventorySelection]->locked == 0) {

        string mapName = g_levelSequence->levelNodes[inventorySelection]->mapfilename;
        vector<string> x = splitString(mapName, '/');
        g_mapdir = x[2];



        int numFloors = g_levelSequence->levelNodes[inventorySelection]->dungeonFloors;

        g_dungeonDarkness = g_levelSequence->levelNodes[inventorySelection]->darkness;

        //generate the DUNGEON ( :DD )

        if(numFloors > 0){
          /*
           * Time to have some structured thought about how this should be done.
           * I'll keep it basic, but retain some important functionality.
           * A little bit of "rawness" or "roughness" (e.g., player can be running from a behemoth through
           * a room which they didn't explore alone, which would be annoying and unfair, lol) is good.
           * 
           * But I want this to be simple yet still give me some freedom (some rooms are rarer than others, some rooms
           * tend to spawn later)
           *
           * Maps proceeded with c_, u_, r_, s_, and e_ can be randomly selected to spawn in the dungeon.
           *
           * Rooms with c_ are common, u_ are uncommon and r_ are rare.
           * s_ (special rooms) and e_ (easter-egg rooms) can replace the other types of rooms so long as the player isn't being chased by a behemoth
           * s_ rooms are garanteed to spawn, but e_ rooms are the rarest type of room in the game (perfect for easter eggs)
           *
           *
           * See? That should be simple enough to be doable quickly while still mysterious enough and flexible enough for me 
           * to create many different types of dungeons, including use of the dungeon data params from the levelsequence file
           * (number of floors, length of a rest sequence, length of a chase sequence, first active floor)
           *
           * start.map is first, and finish.map is last
           *
           */

          g_dungeon.clear();
          g_dungeonIndex = 0;
          g_dungeonBehemoths.clear();
          g_dungeonCommonFloors.clear();
          g_dungeonUncommonFloors.clear();
          g_dungeonRareFloors.clear();
          g_dungeonSpecialFloors.clear();
          g_dungeonEggFloors.clear();
          g_dungeonSystemOn = 1;

          //get list of eligible maps in the mapdir
          string dir = "resources/maps/" + g_mapdir;
          char ** entries = PHYSFS_enumerateFiles(dir.c_str());
          char **i;
          for(i = entries; *i != NULL; i++) {
            string fn(*i);
            if(fn.find(".map") != string::npos && fn.size() > 2 && fn[1] == '-') {
              if(fn[0] == 'c') {g_dungeonCommonFloors.push_back(fn);}
              else if(fn[0] == 'u') {g_dungeonUncommonFloors.push_back(fn);}
              else if(fn[0] == 'r') {g_dungeonRareFloors.push_back(fn);}
              else if(fn[0] == 's') {g_dungeonSpecialFloors.push_back(fn);}
              else if(fn[0] == 'e') {g_dungeonEggFloors.push_back(fn);}
            }
          }
          PHYSFS_freeList(entries);

          if(g_dungeonUncommonFloors.size() == 0) {
            for(auto x : g_dungeonCommonFloors) {
              g_dungeonUncommonFloors.push_back(x);
            }
          }

          if(g_dungeonSpecialFloors.size() == 0) {
            for(auto x : g_dungeonCommonFloors) {
              g_dungeonSpecialFloors.push_back(x);
            }
          }

          for(int i = 0; i < numFloors; i++) {

            float random = frng(0,1);
            string mapstring = "";
            char identity = 'a';
            if(random <= 0.55) {
              int random = rng(0, g_dungeonCommonFloors.size()-1);
              mapstring = g_dungeonCommonFloors.at(random);
              identity = 'c';
            } else if(random <= 0.85){
              int random = rng(0, g_dungeonUncommonFloors.size()-1);
              mapstring = g_dungeonUncommonFloors.at(random);
              identity = 'u';
            } else {
              int random = rng(0, g_dungeonRareFloors.size()-1);
              mapstring = g_dungeonRareFloors.at(random);
              identity = 'r';
            }

            dungeonFloorInfo n;
            n.map = mapstring;
            n.identity = identity;
            g_dungeon.push_back(n);

          }

          g_dungeon.at(g_dungeon.size() -1).map = "finish.map";
          g_dungeon.at(g_dungeon.size() -1).identity = 'e';

          g_dungeonMs = 0;
          g_dungeonHits = 0;

        } else {
          g_dungeonSystemOn = 0;
        }

        clear_map(g_camera);

        inPauseMenu = 0;


        //since we are loading a new level, reset the pellets
        g_currentPelletsCollected = 0;

        load_map(renderer, mapName, g_levelSequence->levelNodes[inventorySelection]->waypointname);
        g_levelSequenceIndex = inventorySelection;
        adventureUIManager->hideInventoryUI();


        if (canSwitchOffDevMode)
        {
          init_map_writing(renderer);
        }
        protag_is_talking = 0;
        protag_can_move = 1;
        adventureUIManager->showHUD();

        if(g_dungeonMusic != nullptr) {
          Mix_FreeMusic(g_dungeonMusic);
          g_dungeonMusic = nullptr;
        }

        if(g_dungeonChaseMusic != nullptr) {
          Mix_FreeMusic(g_dungeonChaseMusic);
          g_dungeonChaseMusic = nullptr;
        }


        if(g_levelSequence->levelNodes[inventorySelection]->music != "0") {
          string l = "resources/static/music/" + g_levelSequence->levelNodes[inventorySelection]->music + ".ogg";
          g_dungeonMusic = loadMusic(l);
          l = "resources/static/music/" + g_levelSequence->levelNodes[inventorySelection]->chasemusic + ".ogg";
          g_dungeonChaseMusic = loadMusic(l);

          Mix_VolumeMusic(g_music_volume * 128);
          Mix_PlayMusic(g_dungeonMusic, -1);
        }

        int setFirst = 0;
        for(auto x : g_levelSequence->levelNodes[g_levelSequenceIndex]->behemoths) {
          if(x == "0" || x == "none") {break;}
          dungeonBehemothInfo n;
          n.ptr = new entity(renderer, x);
          n.ptr->x = 0;
          n.ptr->y = 0;
          n.ptr->persistentGeneral = 1;
          n.ptr->hisweapon->persistent = 1;
          n.ptr->tangible = 0;
          if(!setFirst) { 
            n.waitFloors = g_levelSequence->levelNodes[inventorySelection]->firstActiveFloor; 
          } else {
            float waitFloors = g_levelSequence->levelNodes[inventorySelection]->firstActiveFloor + g_levelSequence->levelNodes[g_levelSequenceIndex]->avgRestSequence * frng(0.6,1.4);
            D(waitFloors);
            n.waitFloors = waitFloors; 
          }
          setFirst = 1;

          for(auto &y : n.ptr->spawnlist) {
            y->persistentGeneral = 1;
          }

          g_dungeonBehemoths.push_back(n);
        }



      } 
    }
  }

  dialogue_cooldown -= elapsed;

  if (input[11] && !oldinput[11] && !inPauseMenu && !transition && g_menuTalkReset == 0 && (g_amState == amState::CLOSED || adventureUIManager->keyPrompting))
  {
    if (protag_is_talking == 1)
    { // advance or speedup diaglogue
      text_speed_up = 50;
    }
    if (protag_is_talking == 0)
    {
      if (dialogue_cooldown < 0)
      {
        interact(elapsed, protag);
      }
    }
    old_z_value = 1;
  }
  else
  {
    if(!keystate[bindings[11]]) {
      g_menuTalkReset = 0;
    }

    // reset text_speed_up
    text_speed_up = 1;
    old_z_value = 0;
  }

  if(keystate[bindings[8]] && protag_is_talking && adventureUIManager->keyPromptCancelForceReset <= 0) {
    adventureUIManager->skipText();
  }

  if(!keystate[bindings[8]]) {
    //Used to stop the player from skipping text right after
    //canceling a key item prompt
    adventureUIManager->keyPromptCancelForceReset = 0; 
  }
  if(adventureUIManager->keyPromptCancelForceReset > 0 ) {
    adventureUIManager->keyPromptCancelForceReset --;
  }

  if (keystate[bindings[11]] && inPauseMenu)
  {
    old_z_value = 1;
  }
  else if (inPauseMenu)
  {
    old_z_value = 0;
  }

  if (protag_is_talking == 2)
  {
    protag_is_talking = 0;
    dialogue_cooldown = 500;
  }

  if (keystate[SDL_SCANCODE_SLASH] && devMode)
  {
  }
  // make another entity of the same type as the last spawned
  if (keystate[SDL_SCANCODE_U] && devMode)
  {
    devinput[1] = 1;
  }
  if (keystate[SDL_SCANCODE_V] && devMode)
  {
    devinput[2] = 1;
  }
  //  if (keystate[SDL_SCANCODE_V] && devMode)
  //  {
  //    devinput[3] = 1;
  //  }
  if (keystate[SDL_SCANCODE_B] && devMode)
  {
    // this is make-trigger
    //
    devinput[0] = 1;
  }
  if (keystate[SDL_SCANCODE_N] && devMode)
  {
    devinput[5] = 1;
  }
  if (keystate[SDL_SCANCODE_M] && devMode)
  {
    devinput[6] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_DIVIDE] && devMode)
  {
    // decrease gridsize
    devinput[7] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_MULTIPLY] && devMode)
  {
    // increase gridsize
    devinput[8] = 1;
  }
  if (keystate[SDL_SCANCODE_O] && devMode)
  {
    //make a dungeondoor
    //devinput[9] = 1;
    devinput[41] = 1;
    //boxsenabled = !boxsenabled;
  }
  if (keystate[SDL_SCANCODE_KP_5] && devMode)
  {
    // triangles
    devinput[10] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_3] && devMode)
  {
    // debug hitboxes
    devinput[7] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_8] && devMode)
  {
    devinput[22] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_6] && devMode)
  {
    devinput[23] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_2] && devMode)
  {
    devinput[24] = 1;
  }
  if (keystate[SDL_SCANCODE_KP_4] && devMode)
  {
    devinput[25] = 1;
  }

  if (keystate[SDL_SCANCODE_RETURN] && devMode)
  {
    // stop player first
    protag->stop_hori();
    protag->stop_verti();

    elapsed = 0;
    // pull up console
    devinput[11] = 1;
  }

  if (keystate[SDL_SCANCODE_COMMA] && devMode)
  {
    // make navnode box
    devinput[20] = 1;
  }

  // for testing particles
  if (keystate[SDL_SCANCODE_Y] && devMode)
  {
    devinput[37] = 1;
  }

  if (keystate[SDL_SCANCODE_PERIOD] && devMode)
  {
    // make navnode box
    devinput[21] = 1;
  }

  //make implied slope 
  if (keystate[SDL_SCANCODE_K] && devMode)
  {
    devinput[34] = 1;
  }

  //make triangular implied slope 
  if (keystate[SDL_SCANCODE_J] && devMode)
  {
    devinput[35] = 1;
  }
  if (keystate[SDL_SCANCODE_L] && devMode)
  {
    devinput[36] = 1;
  }

  if(keystate[SDL_SCANCODE_S] && devMode) {
    devinput[38] = 1;
  }

  if(keystate[SDL_SCANCODE_D] && devMode) {
    devinput[39] = 1;
  }

  if(keystate[SDL_SCANCODE_H] && devMode) {
    devinput[40] = 1;
  }

  if (keystate[SDL_SCANCODE_ESCAPE])
  {

    if (devMode)
    {
      quit = 1;
    }
    else
    {
      if(!protag_is_talking && !inPauseMenu) {
        //open escape menu
        g_escapeUI->show();
        g_inEscapeMenu = 1;
        g_firstFrameOfSettingsMenu = 1;
        g_escapeUI->positionOfCursor = 0;
        g_escapeUI->cursorIsOnBackButton = 0;
        protag_is_talking = 1;
      }
    }
  }
  if (devMode)
  {
    if (keystate[SDL_SCANCODE_Q] && devMode && g_holdingCTRL)
    {
      g_update_zoom = 1;
      g_zoom_mod -= 0.001 * elapsed;

      if (g_zoom_mod < min_scale)
      {
        g_zoom_mod = min_scale;
      }
      if (g_zoom_mod > max_scale)
      {
        g_zoom_mod = max_scale;
      }
    }

    if (keystate[SDL_SCANCODE_E] && devMode && g_holdingCTRL)
    {
      g_update_zoom = 1;
      g_zoom_mod += 0.001 * elapsed;

      if (g_zoom_mod < min_scale)
      {
        g_zoom_mod = min_scale;
      }
      if (g_zoom_mod > max_scale)
      {
        g_zoom_mod = max_scale;
      }
    }
  }
  if (keystate[SDL_SCANCODE_BACKSPACE])
  {
    devinput[16] = 1;
  }

  if (keystate[bindings[0]] == keystate[bindings[1]])
  {
    protag->stop_verti();
  }

  if (keystate[bindings[2]] == keystate[bindings[3]])
  {
    protag->stop_hori();
  }

  if (keystate[SDL_SCANCODE_F] && fullscreen_refresh)
  {
    toggleFullscreen();
  }

  if (keystate[SDL_SCANCODE_F])
  {
    fullscreen_refresh = 0;
  }
  else
  {
    fullscreen_refresh = 1;
  }

  if (keystate[SDL_SCANCODE_1] && devMode)
  {
    devinput[16] = 1;
  }

  if (keystate[SDL_SCANCODE_2] && devMode)
  {
    devinput[17] = 1;
  }

  if (keystate[SDL_SCANCODE_3] && devMode)
  {
    devinput[18] = 1;
  }
  if (keystate[SDL_SCANCODE_3] && devMode)
  {
    devinput[18] = 1;
  }
  if (keystate[SDL_SCANCODE_I] && devMode)
  {
    devinput[19] = 1;
  }
}

void toggleFullscreen() {
  g_fullscreen = !g_fullscreen;
  if (g_fullscreen)
  {
    SDL_GetCurrentDisplayMode(0, &DM);

    SDL_GetWindowSize(window, &saved_WIN_WIDTH, &saved_WIN_HEIGHT);

    SDL_SetWindowSize(window, DM.w, DM.h);
    SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);


    // we need to reload some (all?) textures
    for (auto x : g_mapObjects)
    {
      if (x->mask_fileaddress != "&")
      {
        x->reloadTexture();
      }
    }

    // reassign textures for asset-sharers
    for (auto x : g_mapObjects)
    {
      x->reassignTexture();
    }

    // the same must be done for masked tiles
    for (auto t : g_tiles)
    {
      if (t->mask_fileaddress != "&")
      {
        t->reloadTexture();
      }
    }

    // reassign textures for any asset-sharers
    for (auto x : g_tiles)
    {
      x->reassignTexture();
    }
  }
  else
  {

    SDL_SetWindowFullscreen(window, 0);

    // restore saved width/height
    SDL_SetWindowSize(window, saved_WIN_WIDTH, saved_WIN_HEIGHT);
    SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // we need to reload some (all?) textures
    for (auto x : g_mapObjects)
    {
      if (x->mask_fileaddress != "&")
      {
        x->reloadTexture();
        //        I("reloaded a texture of mask");
        //        I(x->mask_fileaddress);
      }
    }

    // reassign textures for asset-sharers
    for (auto x : g_mapObjects)
    {
      x->reassignTexture();
    }

    // the same must be done for masked tiles
    for (auto t : g_tiles)
    {
      if (t->mask_fileaddress != "&")
      {
        t->reloadTexture();
      }
    }

    // reassign textures for any asset-sharers
    for (auto x : g_tiles)
    {
      x->reassignTexture();
    }
  }
}

void toggleDevmode() {
  if (canSwitchOffDevMode)
  {
    devMode = !devMode;
    if(g_camera.upperLimitX != g_camera.lowerLimitX) {
      if(devMode) {g_camera.enforceLimits = 0;} else {g_camera.enforceLimits = 1;}
    }
    g_zoom_mod = 1;
    g_update_zoom = 1;
    marker->x = -1000;
    markerz->x = -1000;
  }
  if (devMode)
  {
    if(drawhitboxes) {
      floortexDisplay->show = 1;
      captexDisplay->show = 1;
      walltexDisplay->show = 1;
    }
    //boxsenabled = 0;
  }
  else
  {
    floortexDisplay->show = 0;
    captexDisplay->show = 0;
    walltexDisplay->show = 0;
    boxsenabled = 1;
    // float scalex = ((float)WIN_WIDTH / 1920) * g_defaultZoom;
    // float scaley = scalex;
    SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
  }
}

//when the protag jumps, uses an item, collects a pellet, or talks to someone
//nearby behemoths will agro on him
//but, nearby isn't defined by g_earshot but rather the behemoth's hearingRadius
void protagMakesNoise() {
  if(g_ninja || devMode) { return;}
  for(auto x : g_ai) {
    float distToProtag = XYWorldDistanceSquared(protag->getOriginX(), protag->getOriginY(), x->getOriginX(), x->getOriginY());
    float maxHearingDist = pow(x->hearingRadius,2);

    if(distToProtag <= maxHearingDist) {
      if(x->useAgro && x->targetFaction == protag->faction) {
        x->agrod = 1;
        x->target = protag;
      }
    }
  }
}


void dungeonFlash() {
  g_usingMsToStunned = 0; //the trick!
  protag->hisStatusComponent.enraged.clearStatuses();
  protag->bonusSpeed = 0;
  protag_is_talking = 2;
  adventureUIManager->executingScript = 0;
  adventureUIManager->mobilize = 0;
  adventureUIManager->hideTalkingUI();

  g_dungeonDoorActivated = 0;
  //oddly, if I put g_dungeon.size() - 1 in the conditional directly it seems to fail inexplicably
  int size = g_dungeon.size();
  size -= 1;
  if(g_dungeonIndex >= size) {

    {
      string field = g_levelSequence->levelNodes[g_levelSequenceIndex]->name + "-time";
      int timeToBeat = checkSaveField(field);
      if(g_dungeonMs < timeToBeat || timeToBeat == -1) {
        //new record
        M("New time record:" + to_string(g_dungeonMs));
        M("Old value was " + to_string(timeToBeat));
        writeSaveField(field, g_dungeonMs);
      }
      field = g_levelSequence->levelNodes[g_levelSequenceIndex]->name + "-hits";
      int hitsToBeat = checkSaveField(field);
      if(g_dungeonHits < hitsToBeat || hitsToBeat == -1) {
        M("New hits record:" + to_string(g_dungeonHits));
        M("Old value was " + to_string(hitsToBeat));
        writeSaveField(field, g_dungeonHits);


      }
    }

    //clear all behemoths
    for(auto &x : g_dungeonBehemoths) {
      x.ptr->persistentGeneral = 0;
      x.ptr->hisweapon->persistent = 0;
      x.ptr->current = nullptr;
      x.ptr->dest = nullptr;
      x.ptr->Destination = nullptr;

      for(auto &y : x.ptr->spawnlist) {
        y->persistentGeneral = 0;
      }
    }
    g_dungeonBehemoths.clear();

    //this dungeon is finished, play the beaten script to probably unlock a level, save the game, and open
    //the menu select, but it might be to initiate a credits sequence or play a cutscene or something cool
    string l = "resources/maps/" + g_mapdir + "/beaten.txt";

    g_levelFlashing = 1; //don't do an effect
    clear_map(g_camera);
    g_levelFlashing = 0;
    transition = 0;

    if (canSwitchOffDevMode)
    {
      init_map_writing(renderer);
    }

    vector<string> beatenScript = loadText(l);
    parseScriptForLabels(beatenScript);

    for(auto x: beatenScript) {
      D(x);
    }
    M("Better do the beaten script");

    adventureUIManager->talker = narrarator;
    adventureUIManager->ownScript = beatenScript;
    adventureUIManager->dialogue_index = -1;
    adventureUIManager->useOwnScriptInsteadOfTalkersScript = 1;
    adventureUIManager->sleepingMS = 0;
    g_forceEndDialogue = 0;
    adventureUIManager->continueDialogue();

  } else {


    int numberOfActiveBehemoths = 0;

    if(!g_dungeonRedo) {
      //decide if we will end any chases
      for(auto &x : g_dungeonBehemoths) {
        if(x.active) {
          x.floorsRemaining -= 1;
          if(x.floorsRemaining < 1) {
            //deactivate this behemoth
            x.active = 0;
            x.waitFloors = g_levelSequence->levelNodes[g_levelSequenceIndex]->avgRestSequence * frng(0.6,1.4);
            x.floorsRemaining = 0;
            //            M("Behemoth sleeps for:");
            //            D(x.waitFloors);
          }

        } else {
          x.waitFloors -= 1;
          if(x.waitFloors < 1) {
            //spawn dungeon behemoth
            //activate this behemoth
            x.active = 1;
            x.floorsRemaining = g_levelSequence->levelNodes[g_levelSequenceIndex]->avgChaseSequence * frng(0.6,1.4);
            //            M("Behemoth active for:");
            //            D(x.floorsRemaining);

          }


        }

      }
    } else {
      //M("Redo floor, not affecting behemoths");
    }

    for(auto x : g_dungeonBehemoths) {
      if(x.active) { numberOfActiveBehemoths++; }

    }



    g_dungeonIndex++;
    g_levelFlashing = 1;
    clear_map(g_camera);
    transition = 1;
    if(g_dungeonIndex == 0) {
      load_map(renderer, "resources/maps/" + g_mapdir + "/start.map", "a");
    } else {
      bool randomCheck = rng(1,20) > 18;
      D(randomCheck);
      if(g_dungeonSpecialFloors.size() > 0 && numberOfActiveBehemoths == 0 && randomCheck && g_dungeonIndex < g_dungeon.size()-1 && !g_dungeonRedo) {
        M("Replace the floor with a special floor");
        D(g_dungeonSpecialFloors.size());
        int randomIndex = rng(0, g_dungeonSpecialFloors.size() - 1);
        string replacestr = "resources/maps/" + g_mapdir + "/" + g_dungeonSpecialFloors[randomIndex];
        g_dungeon.at(g_dungeonIndex).map = g_dungeonSpecialFloors[randomIndex];
        load_map(renderer, replacestr, "a");
        g_dungeonSpecialFloors.erase(g_dungeonSpecialFloors.begin() + randomIndex);

      } else {

        load_map(renderer, "resources/maps/" + g_mapdir + "/" + g_dungeon.at(g_dungeonIndex).map, "a");
      }
    }
    transition = 0;
    protag_is_talking = 0;
    protag_can_move = 1;
    protag->zvel = 0;
    protag->z = 0;
    g_levelFlashing = 0;
    adventureUIManager->showHUD();
    g_dungeonRedo = 0;

    //this could be faster.
    //I added some lines clearing g_behemothx to clear_map() to
    //prevent a memory error, so this is rather safe
    //probably not a big deal
    for(auto x : g_entities) {
      if(!x->isAI) {continue;}
      if(x->aiIndex == 0) {
        g_behemoth0 = x;
        g_behemoths.push_back(x);
      } else if(x->aiIndex == 1) {
        g_behemoth1 = x;
        g_behemoths.push_back(x);
      } else if(x->aiIndex == 2) {
        g_behemoth2 = x;
        g_behemoths.push_back(x);
      } else if(x->aiIndex == 3) {
        g_behemoth3 = x;
        g_behemoths.push_back(x);
      }
    }


    //M(" -- Active behemoths:");
    //try to spawn the second behemoth at waypoint b, and third at c, etc.
    int index = 0;
    for(auto &x : g_dungeonBehemoths) {
      if(g_dungeonBehemoths[0].active) {
        x.ptr->frameInAnimation = 0;

        for (auto &y : x.ptr->spawnlist) {
          y->tangible = 1;
          y->visible = 0;
        }

        for(int i = 0; i < x.ptr->myAbilities.size(); i++) {
          x.ptr->myAbilities[i].ready = 0;
          x.ptr->myAbilities[i].cooldownMS = x.ptr->myAbilities[i].upperCooldownBound;
        }

        //D(x.ptr->name);
        x.ptr->tangible = 1;
        x.ptr->semisolid = 0;
        if(index == 3) {
          if(g_waypoints.size()>3) {
            x.ptr->setOriginX(g_waypoints.at(3)->x);
            x.ptr->setOriginY(g_waypoints.at(3)->y);
          } else {
            x.ptr->tangible = 0;
            x.ptr->x = 0;
            x.ptr->y = 0;
          }
        }

        if(index == 2) {
          if(g_waypoints.size()>2) {
            x.ptr->setOriginX(g_waypoints.at(2)->x);
            x.ptr->setOriginY(g_waypoints.at(2)->y);
          } else {
            x.ptr->tangible = 0;
            x.ptr->x = 0;
            x.ptr->y = 0;
          }
        }

        if(index == 1) {
          if(g_waypoints.size()>1) {
            x.ptr->setOriginX(g_waypoints.at(1)->x);
            x.ptr->setOriginY(g_waypoints.at(1)->y);
          } else {
            x.ptr->tangible = 0;
            x.ptr->x = 0;
            x.ptr->y = 0;
          }
        }
        if(index == 0) {
          if(g_waypoints.size()>0) {
            x.ptr->setOriginX(g_waypoints.at(0)->x);
            x.ptr->setOriginY(g_waypoints.at(0)->y);
          } else {
            x.ptr->tangible = 0;
            x.ptr->x = 0;
            x.ptr->y = 0;
          }
        }

        x.ptr->opacity = -350;
        x.ptr->opacity_delta = 5;
        x.ptr->agrod = 1;
        x.ptr->target = protag;
        x.ptr->shadow->alphamod = -350;
        x.ptr->hisStatusComponent.stunned.addStatus(2000, 1);
      } else {
        x.ptr->tangible = 0;
        x.ptr->x = 0;
        x.ptr->y = 0;

        for (auto &y : x.ptr->spawnlist) {
          y->tangible = 0;
        }

      }
      index++;
    }


    if (canSwitchOffDevMode)
    {
      init_map_writing(renderer);
    }
  }

}
