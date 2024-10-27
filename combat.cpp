#include "combat.h"
#include "objects.h"
#include "main.h"

combatUI::combatUI(SDL_Renderer* renderer) {

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
  //partyText->layer0 = 1;
  partyText->show = 1;


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

  targetText = new textbox(renderer, "Disaster", 1600 * g_fontsize, 0, 0, 0.9);
  targetText->boxWidth = 0.3;
  targetText->boxHeight = 0.12;
  targetText->boxX = 0.45;
  targetText->boxY = 0.22;
  targetText->dropshadow = 1;

}

combatUI::~combatUI() {
  delete mainPanel;
  delete dialogProceedIndicator;
  delete mainText;
  delete optionsPanel;
  delete optionsText;
  delete menuPicker;
}

void combatUI::hideAll() {
  mainPanel->show = 0;
  dialogProceedIndicator->show = 0;
  mainText->show = 0;
  optionsPanel->show = 0;
  optionsText->show = 0;
  menuPicker->show = 0;
  targetPanel->show = 0;
  targetText->show = 0;
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
  int gap = 0.01 * WIN_WIDTH; // Space between combatants
  
  for (int i = 0; i < count; ++i) {
      combatant* combatant = g_enemyCombatants[i];
  
      // Convert percentage-based width and height to actual pixel values
      int actual_width = static_cast<int>(WIN_WIDTH * combatant->width);
      int actual_height = static_cast<int>(WIN_WIDTH * combatant->height);
  
      // Calculate X position to arrange horizontally
      int total_width = (count * actual_width) + ((count - 1) * gap);
      int x = (WIN_WIDTH - total_width) / 2 + i * (actual_width + gap);
      int y = ((WIN_HEIGHT - actual_height) / 2); // Centering vertically
  
      SDL_Rect renderQuad = { x, y, actual_width, actual_height };

      if(g_submode == submode::TARGETING && i == combatUIManager->currentTarget) {
        SDL_SetTextureColorMod(combatant->texture, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod, combatUIManager->targetingColorMod);
      } else {
        SDL_SetTextureColorMod(combatant->texture, 255, 255, 255);
      }

      SDL_RenderCopy(renderer, combatant->texture, nullptr, &renderQuad);
  }

  count = g_partyCombatants.size();
  combatUIManager->partyHealthBox->show = 1;
  gap = 0.1;

  for (int i = 0; i < count; ++i) {
      combatant* combatant = g_partyCombatants[i];
  
      // Convert percentage-based width and height to actual pixel values
      float actual_width = 0.3 *  WIN_HEIGHT / WIN_WIDTH;
      float actual_height = 0.3;
  
      // Calculate X position to arrange horizontally
      float total_width = (count * actual_width) + ((count - 1) * gap);
      float x = (1 - total_width) / 2 + i * (actual_width + gap);
      float y = (1 - actual_height) / 2; // Centering vertically

      combatUIManager->partyHealthBox->x = x;
      combatUIManager->partyHealthBox->y = 0.7;
      combatUIManager->partyHealthBox->width = actual_width;
      combatUIManager->partyHealthBox->height = actual_height;

      combatUIManager->partyHealthBox->render(renderer, g_camera, elapsed);

      combatUIManager->partyText->boxX = x + 0.02;
      combatUIManager->partyText->boxY = 0.7 + 0.02;
      combatUIManager->partyText->boxWidth = actual_width;
      combatUIManager->partyText->boxHeight = actual_height;
      combatUIManager->partyText->updateText(combatant->name, -1, 34);
      combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

      combatUIManager->partyText->boxY += 0.07;
      combatUIManager->partyText->updateText(to_string(combatant->health) + '/' + to_string(combatant->maxHealth), -1, 34);
      combatUIManager->partyText->render(renderer, WIN_WIDTH, WIN_HEIGHT);



  
  }
  combatUIManager->partyHealthBox->show = 0;

}

void CombatLoop() {
  getCombatInput();

  SDL_RenderClear(renderer);
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
          combatUIManager->currentText += combatUIManager->finalText.at(combatUIManager->currentText.size());
          combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
  
        } else {
          if(input[11] && !oldinput[11]) {
            //advance dialog
            if(combatUIManager->queuedStrings.size() > 0) {
              combatUIManager->dialogProceedIndicator->y = 0.25;
              combatUIManager->currentText = "";
              combatUIManager->finalText = combatUIManager->queuedStrings.at(0);
              combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
            } else {
              g_submode = submode::MAIN;
            }
          }
        }
        curTextWait = 0;
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


      combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
      combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      combatUIManager->dialogProceedIndicator->render(renderer, g_camera, elapsed);
       

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
            M("Attack");
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

            //now, choose a target
            g_submode = submode::ITEMCHOOSE;

            break;
          }
          case 3:
          {
            //Defend
            g_partyCombatants[curCombatantIndex]->serial.action = turnAction::DEFEND;
            g_partyCombatants[curCombatantIndex]->serial.actionIndex = 0;

            //now, choose a target
            g_submode = submode::EXECUTE;
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


      combatUIManager->optionsPanel->show = 1;
      combatUIManager->menuPicker->show = 1;
      combatUIManager->optionsText->show = 1;

      combatUIManager->optionsPanel->render(renderer, g_camera, elapsed);
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

      combatUIManager->targetingColorMod = (sin(combatUIManager->tcm_accumulator) + 1) * 128;

      if(input[2] && !oldinput[2]) {
        if(combatUIManager->currentTarget > 0) {
          combatUIManager->currentTarget --;
        }
      }

      if(input[3] && !oldinput[3]) {
        if(combatUIManager->currentTarget < g_enemyCombatants.size()) {
          combatUIManager->currentTarget ++;
        }
      }

      if(input[11] && !oldinput[11]) {
        g_partyCombatants[curCombatantIndex]->serial.target = combatUIManager->currentTarget;
        g_submode = submode::CONTINUE;
      }

      if(input[10] && !oldinput[10]) {
        M("back button");

      }


      combatUIManager->optionsPanel->render(renderer, g_camera, elapsed);
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

      combatUIManager->targetText->updateText("To " + g_enemyCombatants.at(combatUIManager->currentTarget)->name, -1, 34);

      combatUIManager->targetPanel->render(renderer, g_camera, elapsed);
      combatUIManager->targetText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      


      break;
    }
  }





  drawCombatants();

  updateWindowResolution();

  SDL_RenderPresent(renderer);
}

