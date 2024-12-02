#include "title.h"
#include "objects.h"
#include "main.h"
#include "utils.h"
#include "map_editor.h"

void titleUI::hideAll() {
  newText->show = 0;
  continueText->show = 0;
  endText->show = 0;
  creditText->show = 0;
  handMarker->show = 0;
  panel->show = 0;
  title->show = 0;
  titles->show = 0;
  bg->show = 0;
}

void titleUI::showAll() {
  newText->show = 1;
  continueText->show = 1;
  endText->show = 1;
  creditText->show = 1;
  handMarker->show = 1;
  panel->show = 1;
  title->show = 1;
  titles->show = 1;
  bg->show = 1;
}

titleUI::titleUI(SDL_Renderer* renderer) {
  newText = new textbox(renderer, "New", 1600 * g_fontsize, 0, 0, 0.9);
  newText->boxWidth = 1;
  newText->boxHeight = 0.5;
  newText->boxX = 0.5;
  newText->boxY = 0.65;
  newText->align = 2;
  newText->dropshadow = 1;


  continueText = new textbox(renderer, "Continue", 1600 * g_fontsize, 0, 0, 0.9);
  continueText->boxWidth = 1;
  continueText->boxHeight = 0.5;
  continueText->boxX = 0.5;
  continueText->boxY = 0.75;
  continueText->align = 2;
  continueText->dropshadow = 1;

  endText = new textbox(renderer, "Quit", 1600 * g_fontsize, 0, 0, 0.9);
  endText->boxWidth = 1;
  endText->boxHeight = 0.5;
  endText->boxX = 0.5;
  endText->boxY = 0.85;
  endText->align = 2;
  endText->dropshadow = 1;

  creditText = new textbox(renderer, "", 800 * g_fontsize, 0, 0, 0.9);
  creditText->boxWidth = 1;
  creditText->boxHeight = 0.5;
  creditText->boxX = 0.5;
  creditText->boxY = 0.3;
  creditText->align = 2;
  creditText->dropshadow = 1;
  creditText->updateText("by J.M.", 400 * g_fontsize, 0, {235, 235, 235}, g_font);


  handMarker = new ui(renderer, "resources/static/ui/finger_selector_angled.qoi", 0.5, 0.65, 0.1, 1, 2);
  handMarker->persistent = 1;
  handMarker->show = 1;
  handMarker->priority = 3;
  handMarker->heightFromWidthFactor = 1;
  handMarker->renderOverText = 1;

  panel = new ui(renderer, "resources/static/ui/menu9patchblack.qoi", 0.35, 0.6, 1-0.7, 0.375, 0);
  panel->patchwidth = 213;
  panel->patchscale = 0.4;
  panel->is9patch = true;
  panel->persistent = true;

  title = new ui(renderer, "resources/engine/title.qoi", 0.15, 0.13, 1-0.3, 0.13, 0);
  title->persistent = true;

  titles = new ui(renderer, "resources/engine/title_shadow.qoi", 0.15 + 0.005, 0.13 + (0.005*1.6), 1-0.3, 0.13, 0);
  titles->persistent = true;

  bg = new ui(renderer, "resources/engine/titlebg.qoi", 0, 0, 1, 1, 0);
  bg->persistent = true;


}

titleUI::~titleUI() {
  delete newText;
  delete continueText;
  delete endText;
  delete creditText;
  delete panel;
  delete title;
  delete titles;
  delete bg;
}

void getTitleInput() {
  for (int i = 0; i < 16; i++)
  {
    oldinput[i] = input[i];
  }

  SDL_PollEvent(&event);

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


void TitleLoop() {
  getTitleInput();

  SDL_RenderClear(renderer);


  if(input[0] && !oldinput[0]) {
    //up
    if(titleUIManager->option > 0) {
      titleUIManager->option --;
    }
  }

  if(input[1] && !oldinput[1]) {
    //down
    if(titleUIManager->option < 2) {
      titleUIManager->option ++;
    }
  }

  if(titleUIManager->option == 0) {
    titleUIManager->handMarker->targety = titleUIManager->newText->boxY + titleUIManager->handYOffset;
    float ww = WIN_WIDTH;
    float fwidth = titleUIManager->newText->width;
    titleUIManager->handMarker->targetx = titleUIManager->newText->boxX + (fwidth / ww / 2);
  } else if(titleUIManager->option == 1) {
    float ww = WIN_WIDTH;
    float fwidth = titleUIManager->continueText->width;
    titleUIManager->handMarker->targetx = titleUIManager->continueText->boxX + (fwidth / ww / 2);
    titleUIManager->handMarker->targety = titleUIManager->continueText->boxY + titleUIManager->handYOffset;

  } else if(titleUIManager->option == 2) {
    titleUIManager->handMarker->targety = titleUIManager->endText->boxY + titleUIManager->handYOffset;
    float ww = WIN_WIDTH;
    float fwidth = titleUIManager->endText->width;
    titleUIManager->handMarker->targetx = titleUIManager->endText->boxX + (fwidth / ww / 2);

  }

  
  updateWindowResolution();

  titleUIManager->bg->render(renderer, g_camera, elapsed);

  //titleUIManager->panel->render(renderer, g_camera, elapsed);

  titleUIManager->newText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
  titleUIManager->continueText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

  titleUIManager->endText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

  titleUIManager->handMarker->render(renderer, g_camera, elapsed);

  titleUIManager->titles->render(renderer, g_camera, elapsed);
  titleUIManager->title->render(renderer, g_camera, elapsed);

  titleUIManager->creditText->render(renderer, WIN_WIDTH, WIN_HEIGHT);

  if(input[11]) {
    switch(titleUIManager->option) {
      case 0:
      {
        //start new game
        g_saveName = "newsave";
        loadSave();
        g_saveName = "a";
        g_gamemode = gamemode::EXPLORATION;
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
    
        titleUIManager->bg->render(renderer, g_camera, elapsed);
      
        titleUIManager->newText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        titleUIManager->continueText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->endText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->handMarker->render(renderer, g_camera, elapsed);
      
        titleUIManager->titles->render(renderer, g_camera, elapsed);
        titleUIManager->title->render(renderer, g_camera, elapsed);
      
        titleUIManager->creditText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    
    
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
        titleUIManager->hideAll();
        SDL_GL_SetSwapInterval(1);

        if (canSwitchOffDevMode)
        {
          init_map_writing(renderer);
        }
        load_map(renderer, "resources/maps/" + g_mapOfLastSave + ".map", g_waypointOfLastSave);

        break;
      };
      case 1:
      {
        //continue game
        g_gamemode = gamemode::EXPLORATION;
        loadSave();
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
    
        titleUIManager->bg->render(renderer, g_camera, elapsed);
      
        titleUIManager->newText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        titleUIManager->continueText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->endText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->handMarker->render(renderer, g_camera, elapsed);
      
        titleUIManager->titles->render(renderer, g_camera, elapsed);
        titleUIManager->title->render(renderer, g_camera, elapsed);
      
        titleUIManager->creditText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    
    
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
        titleUIManager->hideAll();
        SDL_GL_SetSwapInterval(1);

        if (canSwitchOffDevMode)
        {
          init_map_writing(renderer);
        }
        D(g_mapOfLastSave);
        load_map(renderer, "resources/maps/" + g_mapOfLastSave + ".map", g_waypointOfLastSave);

        break;
      };
      case 2:
      {
        //quit
        quit = 1;
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
    
        titleUIManager->bg->render(renderer, g_camera, elapsed);
      
        titleUIManager->newText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
        titleUIManager->continueText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->endText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
      
        titleUIManager->handMarker->render(renderer, g_camera, elapsed);
      
        titleUIManager->titles->render(renderer, g_camera, elapsed);
        titleUIManager->title->render(renderer, g_camera, elapsed);
      
        titleUIManager->creditText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
    
    
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
        titleUIManager->hideAll();
        SDL_GL_SetSwapInterval(1);
        g_levelFlashing = 1; //skip ending animation, we already did it (better)

        break;
      };
    }
  }


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

  SDL_RenderPresent(renderer);
}
