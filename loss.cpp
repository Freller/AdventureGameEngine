#include "loss.h"
#include "globals.h"
#include "objects.h"
#include "utils.h"
#include "main.h"
#include "map_editor.h"

void lossUI::hideAll() {
  splat->show = 0;
  yes->show = 0;
  no->show = 0;
  handMarker->show = 0;
}

lossUI::lossUI() {
  this->protag = loadTexture(renderer, "resources/gameover/protag.qoi");
  this->floor = loadTexture(renderer, "resources/gameover/floor.qoi");
  this->shadow = loadTexture(renderer, "resources/engine/shadow.qoi");

  this->splat = new ui(renderer, "resources/gameover/splat.qoi", 0.4, 0.4, 0.16, 1, 0);
  splat->persistent = true;
  splat->show = 1;
  splat->heightFromWidthFactor = 1;
  splat->xframes = 13;
  splat->framewidth = 256;
  splat->frameheight = 256;
  splat->msPerFrame = 50;

//  questionPanel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.3, 0.25, 1-0.6, 0.35, 0);
//  questionPanel->patchwidth = 213;
//  questionPanel->patchscale = 0.4;
//  questionPanel->is9patch = true;
//  questionPanel->persistent = true;

  yes = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
  yes->boxX = 0.50 - 0.07;
  yes->boxY = 0.2;
  yes->boxWidth = 0.01;
  yes->boxHeight = 0.35;
  yes->dropshadow = 1;
  yes->align = 2;

  no = new textbox(renderer, "", 1700 * g_fontsize, 0, 0, 0.9);
  no->boxX = 0.50 + 0.07;
  no->boxY = 0.2;
  no->boxWidth = 0;
  no->boxHeight = 0.35;
  no->dropshadow = 1;
  no->align = 2;

  handMarker = new ui(renderer, "resources/static/ui/finger_selector_angled.qoi", 0.5, 0.65, 0.1, 1, 2);
  handMarker->persistent = 1;
  handMarker->show = 1;
  handMarker->priority = 3;
  handMarker->heightFromWidthFactor = 1;
  handMarker->renderOverText = 1;
  handMarker->y = 0.25;

  SDL_QueryTexture(protag, NULL, NULL, &protagW, &protagH);
  protagW *= 0.52;
  protagH *= 0.52;

  SDL_QueryTexture(shadow, NULL, NULL, &shadowW, &shadowH);
  shadowW *= 0.85;
  shadowH *= 0.85;

  hideAll();
}

lossUI::~lossUI() {
  SDL_DestroyTexture(protag);
  SDL_DestroyTexture(floor);
  delete splat;
  delete yes;
  delete no;
}

void getLossInput() {
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

void LossLoop() {
  getLossInput();
  SDL_RenderClear(renderer);
  updateWindowResolution();

  //D(int(g_lossSub));
  switch(g_lossSub) {
    case lossSub::INWIPE:
    {
      lossUIManager->redness = 255;
      SDL_SetTextureColorMod(lossUIManager->protag, 255, lossUIManager->redness, lossUIManager->redness);
      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);

      lossUIManager->shadowX = (WIN_WIDTH - lossUIManager->shadowW)/2;
      lossUIManager->shadowY = WIN_HEIGHT * 0.492;
      SDL_Rect b = {lossUIManager->shadowX, lossUIManager->shadowY, lossUIManager->shadowW, lossUIManager->shadowH};
      SDL_RenderCopy(renderer, lossUIManager->shadow, NULL, &b);

      lossUIManager->protagX = (WIN_WIDTH - lossUIManager->protagW)/2;
      lossUIManager->protagY = WIN_HEIGHT * 0.45;
      SDL_Rect d = {lossUIManager->protagX, lossUIManager->protagY, lossUIManager->protagW, lossUIManager->protagH};
      SDL_RenderCopy(renderer, lossUIManager->protag, NULL, &d);


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
        g_lossSub = lossSub::SHAKE;
        lossUIManager->timer = 0;
      }

      break;
    }
    case lossSub::SHAKE:
    {
      lossUIManager->timer += elapsed;
      lossUIManager->pMinX = -WIN_WIDTH* 0.002;
      lossUIManager->pMaxX = WIN_WIDTH* 0.002;
      if(lossUIManager->timer < lossUIManager->shake1Ms) {
        lossUIManager->offset += lossUIManager->delta;
        if(lossUIManager->offset > lossUIManager->pMaxX) {
          lossUIManager->delta = lossUIManager->magnitude*-1;
        } else if(lossUIManager->offset < lossUIManager->pMinX) {
          lossUIManager->delta = lossUIManager->magnitude;
        }
      } else if(lossUIManager->timer < lossUIManager->pause1Ms) {
        lossUIManager->offset /=2;
      } else if(lossUIManager->timer < lossUIManager->shake2Ms) {
        lossUIManager->offset += lossUIManager->delta;
        if(lossUIManager->offset > lossUIManager->pMaxX) {
          lossUIManager->delta = lossUIManager->magnitude*-1;
        } else if(lossUIManager->offset < lossUIManager->pMinX) {
          lossUIManager->delta = lossUIManager->magnitude;
        }
      } else if(lossUIManager->timer < lossUIManager->pause2Ms) {
        lossUIManager->offset /=2;
      } else if (lossUIManager->timer < lossUIManager->shake3Ms) {
        lossUIManager->offset += lossUIManager->delta;
        SDL_SetTextureColorMod(lossUIManager->protag, 255, lossUIManager->redness, lossUIManager->redness);
        if(lossUIManager->redness - 2 >0) {
          lossUIManager->redness-=2;
        }
        if(lossUIManager->offset > lossUIManager->pMaxX) {
          lossUIManager->delta = lossUIManager->magnitude*-1;
        } else if(lossUIManager->offset < lossUIManager->pMinX) {
          lossUIManager->delta = lossUIManager->magnitude;
        }
      } else {
        g_lossSub = lossSub::SPLAT;
        lossUIManager->splat->frame = 0;
        lossUIManager->splat->show = 1;
        lossUIManager->splat->msPerFrame = 50;
      }

      
      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);

      lossUIManager->shadowX = (WIN_WIDTH - lossUIManager->shadowW)/2 + lossUIManager->offset;
      lossUIManager->shadowY = WIN_HEIGHT * 0.492;
      SDL_Rect b = {lossUIManager->shadowX, lossUIManager->shadowY, lossUIManager->shadowW, lossUIManager->shadowH};
      SDL_RenderCopy(renderer, lossUIManager->shadow, NULL, &b);

      lossUIManager->protagX = (WIN_WIDTH - lossUIManager->protagW)/2 + lossUIManager->offset;
      lossUIManager->protagY = WIN_HEIGHT * 0.45;
      SDL_Rect d = {lossUIManager->protagX, lossUIManager->protagY, lossUIManager->protagW, lossUIManager->protagH};
      SDL_RenderCopy(renderer, lossUIManager->protag, NULL, &d);
      break;
    }
    case lossSub::SPLAT:
    {
      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
      if(lossUIManager->splat->frame == 12) {
        lossUIManager->splat->msPerFrame = 0;
        lossUIManager->timer = 0;
        g_lossSub = lossSub::PAUSE;
      }
      int pwidth = lossUIManager->splat->width * WIN_WIDTH;
      float pos = (((float)WIN_WIDTH-(float)pwidth)/2);
      pos /= (float)WIN_WIDTH;
      lossUIManager->splat->x = pos;
      lossUIManager->splat->render(renderer, g_camera, elapsed);
      break;
    }
    case lossSub::PAUSE:
    {
      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
      lossUIManager->splat->render(renderer, g_camera, elapsed);
      lossUIManager->timer += elapsed;
      if(lossUIManager->timer >= 1500) {
        g_lossSub = lossSub::TEXT;
        combatUIManager->finalText = "Continue?";
        combatUIManager->currentText = "";
        combatUIManager->mainText->updateText(combatUIManager->currentText, -1, 0.85, g_textcolor, g_font);
      }
      break;
    }
    case lossSub::TEXT:
    {

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
        if(1) {
          //advance dialog
          if(combatUIManager->queuedStrings.size() > 0) {
            combatUIManager->dialogProceedIndicator->y = 0.25;
            combatUIManager->currentText = "";
            combatUIManager->finalText = combatUIManager->queuedStrings.at(0).first;
            combatUIManager->queuedStrings.erase(combatUIManager->queuedStrings.begin());
          } else {
            g_lossSub = lossSub::QUESTION;
            lossUIManager->option = 0;
            lossUIManager->handMarker->show = 1;
            lossUIManager->handMarker->x = lossUIManager->yes->boxX + lossUIManager->yes->boxWidth+ lossUIManager->handOffset;
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



      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
      lossUIManager->splat->render(renderer, g_camera, elapsed);
      combatUIManager->mainPanel->show = 1;
      combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
      combatUIManager->mainText->show = 1;
      combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      break;
    }
    case lossSub::QUESTION:
    {

      SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
      lossUIManager->splat->render(renderer, g_camera, elapsed);
      combatUIManager->mainPanel->show = 1;
      combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
      combatUIManager->mainText->show = 1;
      combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      lossUIManager->yes->show = 1;
      lossUIManager->yes->updateText("Yes", -1, 0.85, g_textcolor, g_font);
      lossUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      lossUIManager->no->show = 1;
      lossUIManager->no->updateText("No", -1, 0.85, g_textcolor, g_font);
      lossUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      lossUIManager->handMarker->render(renderer, g_camera, elapsed);
      if(lossUIManager->option == 0) {
        lossUIManager->handMarker->targetx = lossUIManager->yes->boxX + lossUIManager->yes->boxWidth+ lossUIManager->handOffset;
        lossUIManager->handMarker->y = 0.25;
        if(input[3] && oldinput[3] == 0) {
          lossUIManager->option = 1;
        }

      } else {
        lossUIManager->handMarker->targetx = lossUIManager->no->boxX + lossUIManager->no->boxWidth+ lossUIManager->handOffset;
        lossUIManager->handMarker->y = 0.25;
        if(input[2] && oldinput[2] == 0) {
          lossUIManager->option = 0;
        }
      }
      if(input[11] && !oldinput[11]) {
        if(lossUIManager->option == 0) {
          //Yes
          //Load the player's save and go to exploration mode
          resetUnremarkableData();
          g_gamemode = gamemode::EXPLORATION;


          g_levelFlashing = 1;
          clear_map(g_camera);
          if (canSwitchOffDevMode)
          {
            init_map_writing(renderer);
          }
          g_levelFlashing = 0;
          loadSave();
          Mix_FadeOutMusic(1000);
          g_zoom_mod = 1; //somehow there was a problem where the cam would be zoomed in after continuing from a gameover
          g_update_zoom = 1;
      
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

          SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
          lossUIManager->splat->render(renderer, g_camera, elapsed);
          combatUIManager->mainPanel->show = 1;
          combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
          combatUIManager->mainText->show = 1;
          combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->yes->show = 1;
          lossUIManager->yes->updateText("Yes", -1, 0.85, g_textcolor, g_font);
          lossUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->no->show = 1;
          lossUIManager->no->updateText("No", -1, 0.85, g_textcolor, g_font);
          lossUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->handMarker->render(renderer, g_camera, elapsed);
      
      
      
          SDL_SetRenderTarget(renderer, NULL);
          while (!cont) {
      
            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);
      
            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            //int numPixels = imageWidth * imageHeight;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);
            //Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
      
            offset += g_transitionSpeed + 0.02 * offset;
      
            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {
      
      
                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;
      
                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  // if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < 10 + offset) {
                  // 	pixels[dest] = halftone;
                  // } else {
                  pixels[dest] = 0;
                  // }
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
          transitionDelta = transitionImageHeight;
          combatUIManager->hideAll();
          titleUIManager->hideAll();
          lossUIManager->hideAll();
          adventureUIManager->hideTalkingUI();

          SDL_GL_SetSwapInterval(1);
          g_levelFlashing = 1;
          clear_map(g_camera);
          g_levelFlashing = 0;
          if (canSwitchOffDevMode)
          {
            init_map_writing(renderer);
          }

          load_map(renderer, "resources/maps/" + g_mapOfLastSave + ".map", g_waypointOfLastSave);
          g_fancybox->words.clear();

        } else {
          //No
          //Go to main menu
          g_gamemode = gamemode::TITLE;
          titleUIManager->option = 0;
          if (canSwitchOffDevMode)
          {
            init_map_writing(renderer);
          }
          Mix_FadeOutMusic(1000);
      
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

          SDL_RenderCopy(renderer, lossUIManager->floor, NULL, NULL);
          lossUIManager->splat->render(renderer, g_camera, elapsed);
          combatUIManager->mainPanel->show = 1;
          combatUIManager->mainPanel->render(renderer, g_camera, elapsed);
          combatUIManager->mainText->show = 1;
          combatUIManager->mainText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->yes->show = 1;
          lossUIManager->yes->updateText("Yes", -1, 0.85, g_textcolor, g_font);
          lossUIManager->yes->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->no->show = 1;
          lossUIManager->no->updateText("No", -1, 0.85, g_textcolor, g_font);
          lossUIManager->no->render(renderer, WIN_WIDTH, WIN_HEIGHT);
          lossUIManager->handMarker->render(renderer, g_camera, elapsed);
      
      
      
          SDL_SetRenderTarget(renderer, NULL);
          while (!cont) {
      
            //onframe things
            SDL_LockTexture(transitionTexture, NULL, &pixelReference, &pitch);
      
            memcpy( pixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
            Uint32 format = SDL_PIXELFORMAT_ARGB8888;
            SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
            Uint32* pixels = (Uint32*)pixelReference;
            //int numPixels = imageWidth * imageHeight;
            Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255);
            //Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
      
            offset += g_transitionSpeed + 0.02 * offset;
      
            for(int x = 0;  x < imageWidth; x++) {
              for(int y = 0; y < imageHeight; y++) {
      
      
                int dest = (y * imageWidth) + x;
                //int src =  (y * imageWidth) + x;
      
                if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < offset) {
                  pixels[dest] = transparent;
                } else {
                  // if(pow(pow(imageWidth/2 - x,2) + pow(imageHeight + y,2),0.5) < 10 + offset) {
                  // 	pixels[dest] = halftone;
                  // } else {
                  pixels[dest] = 0;
                  // }
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
          transitionDelta = transitionImageHeight;
          combatUIManager->hideAll();
          lossUIManager->hideAll();
          titleUIManager->showAll();

          g_levelFlashing = 1;
          clear_map(g_camera);
          g_levelFlashing = 0;
          if (canSwitchOffDevMode)
          {
            init_map_writing(renderer);
          }
          g_fancybox->words.clear();


          SDL_GL_SetSwapInterval(1);
        }
      }
      break;
    }
  }


  SDL_RenderPresent(renderer);
}
