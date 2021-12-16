#include <iostream>
#include <vector>
#include <SDL2/SDL.h>        
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <stdlib.h>
#include "globals.cpp"
#include "objects.cpp"
#include "map_editor.cpp"
#include "lightcookietesting.cpp"

using namespace std;



// int compare_ent (entity* a, entity* b) {
//   return a->y+ a->z  + a->sortingOffset < b->y +b->z + b->sortingOffset;
// }

// int compare_ent (actor* a, actor* b) {
//   	return a->getOriginY() + a->z < b->getOriginY() +b->z;
// }

int compare_ent (actor* a, actor* b) {
  	return a->y + a->z + a->sortingOffset < b->y + b->z + b->sortingOffset;
}

void sort_by_y(vector<actor*> &g_entities) {
    stable_sort(g_entities.begin(), g_entities.end(), compare_ent);
}

void getInput(float& elapsed);
 

int main(int argc, char ** argv) {
	//load first arg into variable devmode
	if(argc > 1) {
		devMode = (argv[1][0] == '1');
		canSwitchOffDevMode = devMode;
	}
	if(argc > 2) {
		genericmode = (argv[2][0] == '1');
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	
	window = SDL_CreateWindow("Carbin",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE /*| SDL_WINDOW_ALWAYS_ON_TOP*/);
	renderer = SDL_CreateRenderer(window, -1,  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_SetWindowMinimumSize(window, 100, 100);

	// for( int i = 0; i < SDL_GetNumRenderDrivers(); ++i ) {
	// 	SDL_RendererInfo rendererInfo = {};
	// 	SDL_GetRenderDriverInfo( i, &rendererInfo );
	// 	if( rendererInfo.name != std::string( "opengles2" ) ) {
	// 		//provide info about improper renderer
	// 		continue;
	// 	}

	// 	renderer = SDL_CreateRenderer( window, i, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	// 	break;
	// }

	Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048);
	SDL_RenderSetIntegerScale(renderer, SDL_FALSE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "3"); 
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	
	SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
	//SDL_RenderSetLogicalSize(renderer, 1920, 1080); //for enforcing screen resolution

	//entities will be made here so have them set as created during loadingtime and not arbitrarily during play
	g_loadingATM = 1;

	//set global shadow-texture

	//!!! move these to the engine folder
	SDL_Surface* image = IMG_Load("textures/engine/shadow.bmp");
	g_shadowTexture = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);
	image = IMG_Load("textures/engine/shadow-square.bmp");
	g_shadowTextureAlternate = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

	//narrarator holds scripts caused by things like triggers
	entity* narrarator;
	// !!! reduce first launch overhead by 
	// making the narrarator use a sprite with 1 pixel
	narrarator = new entity(renderer, "fomm");
	narrarator->tangible =0;
	narrarator->persistentHidden = 1;
	//narrarator->name = "sp-narrarator";

	entity* fomm;
	if(devMode) {
		//fomm = new entity(renderer, "fommconstruction"); 
		fomm = new entity(renderer, "fomm"); 
		//g_defaultZoom = 0.85;
	} else {
		fomm = new entity(renderer, "fomm"); 
	}
	
	
	fomm->inParty = 1;
	party.push_back(fomm);
	fomm->footstep = Mix_LoadWAV("sounds/protag-step-1.wav");
	fomm->footstep2 = Mix_LoadWAV("sounds/protag-step-2.wav");
	protag = fomm;
	g_cameraShove = protag->hisweapon->attacks[0]->range/2;
	g_focus = protag;
	
	g_deathsound = Mix_LoadWAV("audio/sounds/game-over.wav");
	
	//protag healthbar
	ui* protagHealthbarA = new ui(renderer, "textures/ui/healthbarA.png", 0,0, 0.05, 0.02, -3);
	protagHealthbarA->persistent = 1;
	ui* protagHealthbarB = new ui(renderer, "textures/ui/healthbarB.png", 0,0, 0.05, 0.02, -2);
	protagHealthbarB->persistent = 1;
	protagHealthbarB->shrinkPixels = 1;
	
	ui* protagHealthbarC = new ui(renderer, "textures/ui/healthbarC.png", 0,0, 0.05, 0.02, -1);
	protagHealthbarC->persistent = 1;
	protagHealthbarC->shrinkPixels = 1;

	//for transition
	SDL_Surface* transitionSurface = IMG_Load("textures/engine/transition.bmp");

	int transitionImageWidth = transitionSurface->w;
	int transitionImageHeight = transitionSurface->h;

	SDL_Texture* transitionTexture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, transitionSurface->w, transitionSurface->h );
	SDL_SetTextureBlendMode(transitionTexture, SDL_BLENDMODE_BLEND);

	void* transitionPixelReference;
	int transitionPitch;

	float transitionDelta = transitionImageHeight;

	//font
	g_font = "fonts/Monda-Bold.ttf";

	//setup UI
	adventureUIManager = new adventureUI(renderer);
	
	if(canSwitchOffDevMode) {
		init_map_writing(renderer);
		//done once, because textboxes aren't cleared during clear_map()
		nodeInfoText = new textbox(renderer, "", g_fontsize * WIN_WIDTH, 0, 0, WIN_WIDTH);
		config = "edit";
		nodeDebug = SDL_CreateTextureFromSurface(renderer, IMG_Load("textures/engine/walker.bmp"));
		
	}
	//set bindings from file
	ifstream bindfile;
	bindfile.open("user/configs/" + config + ".cfg");
	string line;
	for (int i = 0; i < 13; i++) {
		getline(bindfile, line);
		bindings[i] = SDL_GetScancodeFromName(line.c_str());
		//D(bindings[i]);
	}
	//set vsync and g_fullscreen from config
	string valuestr; float value;
	
	//get vsync
	// getline(bindfile, line);
	// valuestr = line.substr(line.find(' '), line.length());
	// value = stoi(valuestr);
	// D(value);
	// g_vsync = value;
	// D(g_vsync);
	
	//get g_fullscreen
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '), line.length());
	value = stoi(valuestr);
	g_fullscreen = value;

	//get bg darkness
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_background_darkness = value;

	//get music volume
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_music_volume = value;

	//get sfx volume
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_sfx_volume = value;

	//get standard textsize
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_fontsize = value;

	//get mini textsize
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_minifontsize = value;

	//transitionspeed
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_transitionSpeed = value;

	//mapdetail
	// 0 -   - ultra low - no lighting, crappy settings for g_tilt_resolution
	// 1 -   - 
	// 2 -
	getline(bindfile, line);
	valuestr = line.substr(line.find(' '),  line.length());
	value = stof(valuestr);
	g_graphicsquality = value;

	switch (g_graphicsquality) {
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
	
	//apply vsync
	SDL_GL_SetSwapInterval(1);

	//hide mouse
	//

	//apply fullscreen
	if(g_fullscreen) {
		SDL_GetCurrentDisplayMode(0, &DM);
		SDL_SetWindowSize(window, DM.w, DM.h);
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowFullscreen(window, 0);
	}	

	//initialize box matrix z
	for (int i = 0; i < g_layers; i++) {
		vector<box*> v = {};
		g_boxs.push_back(v);
	}

	for (int i = 0; i < g_layers; i++) {
		vector<tri*> v = {};
		g_triangles.push_back(v);
	}

	for (int i = 0; i < g_layers; i++) {
		vector<ramp*> v = {};
		g_ramps.push_back(v);
	}

	//init static resources
	g_bulletdestroySound = Mix_LoadWAV("audio/sounds/step.wav");
	g_playerdamage = Mix_LoadWAV("audio/sounds/playerdamage.wav");
	g_enemydamage = Mix_LoadWAV("audio/sounds/enemydamage.wav");
	g_npcdamage = Mix_LoadWAV("audio/sounds/npcdamage.wav");
	g_s_playerdeath = Mix_LoadWAV("audio/sounds/playerdeath.wav");
	g_land = Mix_LoadWAV("audio/sounds/step2.wav");
	g_footstep_a = Mix_LoadWAV("audio/sounds/protag-step-1.wav");
	g_footstep_b = Mix_LoadWAV("audio/sounds/protag-step-1.wav");
	g_menu_open_sound = Mix_LoadWAV("audio/sounds/open-menu.wav");
	g_menu_close_sound = Mix_LoadWAV("audio/sounds/close-menu.wav");
	g_ui_voice = Mix_LoadWAV("audio/sounds/voice-normal.wav");
	g_menu_manip_sound = Mix_LoadWAV("audio/sounds/manip-menu.wav");

	
	//scripts can play sounds completely randomly - e.g. crate gives a random item 
	//this might not be the best way to do it, but it seems fine now
	// std::ifstream infile("staticresources.txt");
	// string soundname = "";
	// while(infile >> soundname) {
	// 	string loadme = "audio/sounds/" + soundname;
	// 	Mix_Chunk* a = Mix_LoadWAV(loadme.c_str());
	// 	static

	// }




	srand(time(NULL));
	if(devMode) {
		g_transitionSpeed = 10000;
		//!!!
		loadSave();
		//empty map or default map for map editing, perhaps a tutorial even
		load_map(renderer, "maps/sp-title/editordefault.map","a");
		g_map = "g";
		protag->x = 100000;
		protag->y = 100000;
		
		
	} else {
		//load the titlescreen
		load_map(renderer, "maps/" + g_map + "/" + g_map + ".map", "a");
		//srand(time(NULL));
	}

	ui* inventoryMarker = new ui(renderer, "textures/ui/non_selector.png", 0, 0, 0.15, 0.15, 2);
	inventoryMarker->show = 0;
	inventoryMarker->persistent = 1;

	textbox* inventoryText = new textbox(renderer, "1", 40, WIN_WIDTH * 0.8, 0, WIN_WIDTH * 0.2);
	inventoryText->show = 0;
	inventoryText->align = 1;

	bool storedJump = 0; //buffer the input from a jump if the player is off the ground, quake-style
	
	//This stuff is for the FoW mechanic
		
    SDL_Surface* SurfaceA = IMG_Load("misc/resolution.bmp");
	SDL_Surface* SurfaceB = IMG_Load("misc/b.png");
	
	SDL_Texture* TextureA = SDL_CreateTextureFromSurface(renderer, SurfaceA);
	//SDL_Texture* TextureA = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIN_WIDTH, WIN_HEIGHT);	
	SDL_Texture* TextureB = SDL_CreateTextureFromSurface(renderer, SurfaceB);

	SDL_FreeSurface(SurfaceA);
	SDL_FreeSurface(SurfaceB);

	SDL_Surface* blackbarSurface = IMG_Load("textures/engine/black.bmp");
	SDL_Texture* blackbarTexture = SDL_CreateTextureFromSurface(renderer, blackbarSurface);

	SDL_FreeSurface(blackbarSurface);

	SDL_Texture* TextureC;


	SDL_Texture* result = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);
	SDL_SetTextureBlendMode(result, SDL_BLENDMODE_MOD);
	SDL_SetTextureBlendMode(TextureA, SDL_BLENDMODE_MOD);
	SDL_SetTextureBlendMode(TextureB, SDL_BLENDMODE_NONE);

	SDL_SetRenderDrawColor(renderer, 0,0,0,0);
	SDL_RenderPresent(renderer);
	SDL_GL_SetSwapInterval(1);

	//textures for adding operation
	SDL_Texture* canvas = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 500, 500);

	SDL_Surface* lightSurface = IMG_Load("misc/light.png");
	SDL_Texture* light = SDL_CreateTextureFromSurface(renderer, lightSurface);
	SDL_FreeSurface(lightSurface);


	
	std::vector<std::vector<int> > g_fogcookies( g_fogwidth, std::vector<int>(g_fogheight));


	//software lifecycle text
	//new textbox(renderer, g_lifecycle.c_str(), 40,WIN_WIDTH * 0.8,0, WIN_WIDTH * 0.2);
	while (!quit) {
		//some event handling
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					switch( event.key.keysym.sym ){
                    	case SDLK_TAB:
							g_holdingCTRL = 1;
							//protag->getItem(a, 1);
							break;
					}
					break;
				case SDL_KEYUP:
					switch( event.key.keysym.sym ){
                    	case SDLK_TAB:
							g_holdingCTRL = 0;
							break;
					}
					break;
				case SDL_MOUSEWHEEL:
				if(g_holdingCTRL) {
					if(event.wheel.y > 0) {
						wallstart -= 64;
					}
					else if(event.wheel.y < 0) {
						wallstart += 64;
					}
					if(wallstart < 0) {
						wallstart = 0;
					} else {
						if(wallstart > 64 * g_layers) {
							wallstart = 64 * g_layers;
						}
						if(wallstart > wallheight - 64) {
							wallstart = wallheight - 64;
						}
					}
				} else {
					if(event.wheel.y > 0) {
						wallheight -= 64;
					}
					else if(event.wheel.y < 0) {
						wallheight += 64;
					}
					if(wallheight < wallstart + 64) {
						wallheight = wallstart + 64;
					} else {
						if(wallheight > 64 * g_layers) {
							wallheight = 64 * g_layers;
						}
					}
					break;
				}
				case SDL_MOUSEBUTTONDOWN:
					if(event.button.button == SDL_BUTTON_LEFT){
						devinput[3] = 1;
					}
					if(event.button.button == SDL_BUTTON_MIDDLE){
						devinput[10] = 1;
					}
					if(event.button.button == SDL_BUTTON_RIGHT){
						devinput[4] = 1;
					}
					break;
				
				case SDL_QUIT:
					quit = 1;
					break;
			}
		}	

		ticks = SDL_GetTicks();
		elapsed = ticks - lastticks;
		lastticks = ticks;

		//I(elapsed);

		//lock time
		elapsed = 16;

		//cooldowns
		halfsecondtimer+=elapsed;
		use_cooldown -= elapsed / 1000;
		musicFadeTimer += elapsed;
		musicUpdateTimer += elapsed;
		g_dash_cooldown -= elapsed;

		if(inPauseMenu) {
			//if we're paused, freeze gametime
			elapsed = 0;
		}

		//INPUT
		getInput(elapsed);

		//spring
		if(input[8] && !oldinput[8] && protag->grounded && protag_can_move || input[8] && storedJump && protag->grounded && protag_can_move) {
			protag->zaccel = 180;
			storedJump = 0;
		} else { 
			if(input[8] && !oldinput[8] && !protag->grounded) {
				storedJump = 1;
			}
		}
		
		//if we die don't worry about not being able to switch because we can't shoot yet
		if(protag->hp <= 0) {playSound(4, g_s_playerdeath, 0); protag->cooldown = 0;}

		//cycle right if the current character dies
		if( (input[9] && !oldinput[9]) || protag->hp <= 0) {
			//keep switching if we switch to a dead partymember
			int i = 0;
			
			if(party.size() > 1 && protag->cooldown <= 0) {
				do {
					M("Cycle party right");
					std::rotate(party.begin(),party.begin()+1,party.end());
					protag->tangible = 0;
					protag->flashingMS = 0;
					party[0]->tangible = 1;
					party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width/2;
					party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height/2;
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
					protag=party[0];
					protag->shadow->x = protag->x + protag->shadow->xoffset;
					protag->shadow->y = protag->y + protag->shadow->yoffset;
					g_focus = protag;
					protag->curheight = 0;
					protag->curwidth = 0;
					g_cameraShove = protag->hisweapon->attacks[0]->range/2;
					//prevent infinite loop
					i++;
					if(i > 600) {M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)"); break; quit = 1;}
				}while(protag->hp <= 0);
			}
		}
		//party swap
		if(input[10] && !oldinput[10]) {
			if(party.size() > 1 && protag->cooldown <= 0) {
				int i = 0;
				do {
					M("Cycle party left");
					std::rotate(party.begin(),party.begin()+party.size()-1,party.end());
					protag->tangible = 0;
					protag->flashingMS = 0;
					party[0]->tangible = 1;
					party[0]->x = protag->getOriginX() - party[0]->bounds.x - party[0]->bounds.width/2;
					party[0]->y = protag->getOriginY() - party[0]->bounds.y - party[0]->bounds.height/2;
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
					protag=party[0];
					protag->shadow->x = protag->x + protag->shadow->xoffset;
					protag->shadow->y = protag->y + protag->shadow->yoffset;
					g_focus = protag;
					protag->curheight = 0;
					protag->curwidth = 0;
					g_cameraShove = protag->hisweapon->attacks[0]->range/2;
					i++;
					if(i > 600) {M("Avoided infinite loop: no living partymembers yet no essential death. (Did the player's party contain at least one essential character?)"); break; quit = 1;}
				} while(protag->hp <= 0);
			}
		}

		//background
		//SDL_SetRenderTarget(renderer, TextureA);
		SDL_RenderClear(renderer);
		if(g_backgroundLoaded && g_useBackgrounds) { //if the level has a background and the user would like to see it
			SDL_RenderCopy(renderer, background, NULL, NULL);
		}
		

		
		
		for(auto n : g_entities) {
			n->cooldown -= elapsed;
		}

		
		




		//listeners
		for (int i = 0; i < g_listeners.size(); i++) {
			if(g_listeners[i]->update()) {
				adventureUIManager->blip = NULL;
				adventureUIManager->sayings = &g_listeners[i]->script;
				adventureUIManager->talker = narrarator;
				narrarator->dialogue_index = -1;
				narrarator->sayings = g_listeners[i]->script;
				adventureUIManager->continueDialogue();
				g_listeners[i]->active = 0;
			}	
		}
		
			

		//update camera
		SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);

		// !!! it might be better to not run this every frame
		if(old_WIN_WIDTH != WIN_WIDTH || g_update_zoom) {
			//user scaled window
			scalex = ((float)WIN_WIDTH / STANDARD_SCREENWIDTH) * g_defaultZoom * g_zoom_mod;
			scaley = scalex;
			if(scalex < min_scale) {
				scalex = min_scale;
			}
			if(scalex > max_scale) {
				scalex = max_scale;
			}
			SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
			SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
		}
		old_WIN_WIDTH = WIN_WIDTH;

		WIN_WIDTH /= scalex;
		WIN_HEIGHT /= scaley;
		if(devMode) {
			
			g_camera.width = WIN_WIDTH / (scalex * g_zoom_mod * 0.2); //the 0.2 is arbitrary. it just makes sure we don't end the camera before the screen
			g_camera.height = WIN_HEIGHT / (scalex * g_zoom_mod * 0.2); 
		} else {
			g_camera.width = WIN_WIDTH;
			g_camera.height = WIN_HEIGHT;
		}
		
		
		

		if(freecamera) {
			g_camera.update_movement(elapsed, camx, camy);
		} else {
			//lerp cameratargets
			g_cameraAimingOffsetY = g_cameraAimingOffsetY*g_cameraAimingOffsetLerpScale + g_cameraAimingOffsetYTarget *(1-(g_cameraAimingOffsetLerpScale));
			g_cameraAimingOffsetX = g_cameraAimingOffsetX*g_cameraAimingOffsetLerpScale + g_cameraAimingOffsetXTarget *(1-(g_cameraAimingOffsetLerpScale));
			float zoomoffsetx = (WIN_WIDTH /2) / g_zoom_mod;
			float zoomoffsety = (WIN_HEIGHT /2) / g_zoom_mod;
			//g_camera.zoom = 0.9;
			
			g_camera.update_movement(elapsed, g_focus->getOriginX() - zoomoffsetx + (g_cameraAimingOffsetX * g_cameraShove), ((g_focus->getOriginY() - XtoZ * g_focus->z) - zoomoffsety - (g_cameraAimingOffsetY * g_cameraShove)));
		}
		// g_camera.zoom = scalex;

		//update ui
		curTextWait += elapsed * text_speed_up;
		if(curTextWait >= textWait) {
			adventureUIManager->updateText();
			curTextWait = 0;
		}
		
		
		//tiles
		for(long long unsigned int i=0; i < g_tiles.size(); i++){
			if(g_tiles[i]->z ==0) {
				g_tiles[i]->render(renderer, g_camera);
			}
		}

		for(long long unsigned int i=0; i < g_tiles.size(); i++){
			if(g_tiles[i]->z ==1) {
				g_tiles[i]->render(renderer, g_camera);
			}
		}

		//sort		
		sort_by_y(g_actors);
		for(long long unsigned int i=0; i < g_actors.size(); i++){
			g_actors[i]->render(renderer, g_camera);
		}

		for(long long unsigned int i=0; i < g_tiles.size(); i++){
			if(g_tiles[i]->z == 2) {
				g_tiles[i]->render(renderer, g_camera);
			}
		}

		
		//map editing
		if(devMode) {
				
			nodeInfoText->textcolor = {0, 0, 0};

			//draw nodes
			for (int i = 0; i < g_worldsounds.size(); i++) {
				SDL_Rect obj = {(g_worldsounds[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_worldsounds[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, worldsoundIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_worldsounds[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_musicNodes.size(); i++) {
				SDL_Rect obj = {(g_musicNodes[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_musicNodes[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, musicIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_musicNodes[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_cueSounds.size(); i++) {
				SDL_Rect obj = {(g_cueSounds[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_cueSounds[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, cueIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_cueSounds[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_waypoints.size(); i++) {
				SDL_Rect obj = {(g_waypoints[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_waypoints[i]->y - 20 - g_camera.y - g_waypoints[i]->z * XtoZ) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, waypointIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_waypoints[i]->name, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			//doors
			for (int i = 0; i < g_doors.size(); i++) {
				SDL_Rect obj = {(g_doors[i]->x -g_camera.x)* g_camera.zoom , ((g_doors[i]->y - g_camera.y - ( g_doors[i]->zeight ) * XtoZ) * g_camera.zoom), (g_doors[i]->width * g_camera.zoom), (g_doors[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj);
				//the wall
				SDL_Rect obj2 = {(g_doors[i]->x -g_camera.x)* g_camera.zoom, ((g_doors[i]->y - g_camera.y - ( g_doors[i]->zeight ) * XtoZ) * g_camera.zoom), (g_doors[i]->width * g_camera.zoom), ( (g_doors[i]->zeight - g_doors[i]->z) * XtoZ * g_camera.zoom) + (g_doors[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, doorIcon->texture, NULL, &obj2);
				nodeInfoText->x = obj.x + 25;
				nodeInfoText->y = obj.y + 25;
				nodeInfoText->updateText(g_doors[i]->to_map + "->" + g_doors[i]->to_point, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			for (int i = 0; i < g_triggers.size(); i++) {
				SDL_Rect obj = {(g_triggers[i]->x -g_camera.x)* g_camera.zoom , ((g_triggers[i]->y - g_camera.y - ( g_triggers[i]->zeight ) * XtoZ) * g_camera.zoom), (g_triggers[i]->width * g_camera.zoom), (g_triggers[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj);
				//the wall
				SDL_Rect obj2 = {(g_triggers[i]->x -g_camera.x)* g_camera.zoom, ((g_triggers[i]->y - g_camera.y - ( g_triggers[i]->zeight ) * XtoZ) * g_camera.zoom), (g_triggers[i]->width * g_camera.zoom), ( (g_triggers[i]->zeight - g_triggers[i]->z) * XtoZ * g_camera.zoom) + (g_triggers[i]->height * g_camera.zoom)};
				SDL_RenderCopy(renderer, triggerIcon->texture, NULL, &obj2);

				nodeInfoText->x = obj.x + 25;
				nodeInfoText->y = obj.y + 25;
				nodeInfoText->updateText(g_triggers[i]->binding, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			//listeners
			for (int i = 0; i < g_listeners.size(); i++) {
				SDL_Rect obj = {(g_listeners[i]->x -g_camera.x - 20)* g_camera.zoom , ((g_listeners[i]->y - g_camera.y - 20) * g_camera.zoom), (40 * g_camera.zoom), (40 * g_camera.zoom)};
				SDL_RenderCopy(renderer, listenerIcon->texture, NULL, &obj);
				nodeInfoText->x = obj.x;
				nodeInfoText->y = obj.y - 20;
				nodeInfoText->updateText(g_listeners[i]->listenList.size() + " of " + g_listeners[i]->entityName, 15, 15);
				nodeInfoText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
			}

			write_map(protag);
			for(int i =0; i < 50; i++) {
				devinput[i]=0;
			}

		}

		//Fogofwar
		if(g_fogofwarEnabled) {

			//this is the worst functional code I've written, with no exceptions

			bool flipper = 0;
			for(int i = 0; i < g_fogcookies.size(); i++) {
				for(int j = 0; j < g_fogcookies[0].size(); j++) {
					flipper = !flipper;
					int xpos = ((i - 10) * 64) + g_focus->getOriginX();
					int ypos = ((j - 9) * 64) + g_focus->getOriginY();
					if(LineTrace(g_focus->getOriginX(), g_focus->getOriginY(), xpos, ypos, true, 30, 0)) {	
						g_fogcookies[i][j] = 1;
					} else {
						g_fogcookies[i][j] = 0;
					}
				}
				flipper = !flipper;
			}

			//these are the corners and the center
			// g_fogcookies[0][0] = 1;
			// g_fogcookies[20][0] = 1;
			// g_fogcookies[20][17] = 1;
			// g_fogcookies[0][17] = 1;
			// g_fogcookies[10][9] = 1;


			int px = -(int)g_focus->getOriginX() % 64;
			
			//offset us to the protag's location
			//int yoffset =  ((g_focus->y- (g_focus->z + g_focus->zeight) * XtoZ)) * g_camera.zoom;
			//the zeight is constant at level 2  for now
			int yoffset =  ((g_focus->getOriginY() - (0) * XtoZ)) * g_camera.zoom;
			
			//and then subtract half of the screen
			yoffset -= yoffset % 55;
			yoffset -= (g_fogheight * 55 + 12)/2;
			yoffset -= g_camera.y;
			

			//we do this nonsense to keep the offset on the grid
			//yoffset -= yoffset % 55;

			//px = 64 - px - 64;
			//py = 55 - py - 55;
			// 50 50
			addTextures(renderer, g_fogcookies, canvas, light, 500, 500, 210, 210);


			TextureC = IlluminateTexture(renderer, TextureA, canvas, result);
			
			//render graphics
			SDL_Rect dstrect = {px - 20, yoffset -8, g_fogwidth * 64 + 50, g_fogheight * 55 + 30};
			SDL_RenderCopy(renderer, TextureC, NULL, &dstrect);
			
			//black bars :/
			SDL_Rect topbar = {px, yoffset - 5000, 1500, 5000};
			SDL_RenderCopy(renderer, blackbarTexture, NULL, &topbar);
			
			SDL_Rect botbar = {px, yoffset +  g_fogheight * 55 + 12, 1500, 5000};
			SDL_RenderCopy(renderer, blackbarTexture, NULL, &botbar);
			
		}

		
		
		//ui
		if(!inPauseMenu && g_showHUD) {
			// !!! segfaults on mapload sometimes
			adventureUIManager->healthText->updateText( to_string(int(protag->hp)) + '/' + to_string(int(protag->maxhp)), WIN_WIDTH * g_minifontsize, 0.9); 
			adventureUIManager->healthText->show = 1;
			
		} else {
			adventureUIManager->healthText->show = 0;
			
		}

		//move the healthbar properly to the protagonist
		rect obj; // = {( , (((protag->y - ((protag->height))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom, (protag->width * g_camera.zoom), (protag->height * g_camera.zoom))};		
		obj.x = ((protag->x -g_camera.x) * g_camera.zoom);
		obj.y = (((protag->y - ((floor(protag->height)))) - protag->z * XtoZ) - g_camera.y) * g_camera.zoom;
		obj.width = (protag->width * g_camera.zoom);
		obj.height = (floor(protag->height) * g_camera.zoom);

		protagHealthbarA->x = (((float)obj.x + obj.width/2) / (float)WIN_WIDTH) - protagHealthbarA->width/2.0;
		protagHealthbarA->y = ((float)obj.y) / (float)WIN_HEIGHT;
		protagHealthbarB->x = protagHealthbarA->x;
		protagHealthbarB->y = protagHealthbarA->y;
		
		protagHealthbarC->x = protagHealthbarA->x;
		protagHealthbarC->y = protagHealthbarA->y;
		protagHealthbarC->width = (protag->hp / protag->maxhp) * 0.05;
		adventureUIManager->healthText->boxX = protagHealthbarA->x + protagHealthbarA->width/2;
		adventureUIManager->healthText->boxY = protagHealthbarA->y - 0.005;
		
		for(long long unsigned int i=0; i < g_ui.size(); i++){
			g_ui[i]->render(renderer, g_camera);
		}	
		for(long long unsigned int i=0; i < g_textboxes.size(); i++){
			g_textboxes[i]->render(renderer, WIN_WIDTH, WIN_HEIGHT);
		}	

		//draw pause screen
		if(inPauseMenu) {
			
			//iterate thru inventory and draw items on screen
			float defaultX = WIN_WIDTH * 0.05;
			float defaultY = WIN_WIDTH * 0.05;
			float x = defaultX;
			float y = defaultY;
			float maxX = WIN_WIDTH * 0.9;
			float maxY = WIN_HEIGHT * 0.60;
			float itemWidth = WIN_WIDTH * 0.07;
			float padding = WIN_WIDTH * 0.01;

	
			int i = 0;
			for(auto it =  mainProtag->inventory.rbegin(); it != mainProtag->inventory.rend(); ++it) {

				if(i < itemsPerRow * inventoryScroll) {
					//this item won't be rendered
					i++;
					continue;
				}

				
				
				SDL_Rect drect = {x, y, itemWidth, itemWidth};
				D(mainProtag->inventory.size());
				if(it->second > 0) {
					SDL_RenderCopy(renderer, it->first->texture, NULL, &drect);
				}
				//draw number
				if(it->second > 1) {
					inventoryText->show = 1;
					inventoryText->updateText( to_string(it->second), 35, 100);
					inventoryText->boxX = (x + (itemWidth * 0.8) ) / WIN_WIDTH;
					inventoryText->boxY = (y + (itemWidth - inventoryText->boxHeight/2) * 0.6 ) / WIN_HEIGHT;
					inventoryText->worldspace = 1;
					inventoryText->render(renderer, WIN_WIDTH, WIN_HEIGHT);
				} else {
					inventoryText->show = 0;
				}


				if(i == inventorySelection) {
					//this item should have the marker
					inventoryMarker->show = 1;
					inventoryMarker->x = x / WIN_WIDTH;
					inventoryMarker->y = y / WIN_HEIGHT;
					inventoryMarker->width = itemWidth / WIN_WIDTH;
					
					float biggen = 0.01; // !!! resolutions : might have problems with diff resolutions
					inventoryMarker->x -= biggen;
					inventoryMarker->y -= biggen * ((float)WIN_WIDTH/(float)WIN_HEIGHT);
					inventoryMarker->width += biggen * 2;
					inventoryMarker->height = inventoryMarker->width * ((float)WIN_WIDTH/(float)WIN_HEIGHT);
				}

				x += itemWidth + padding;
				if(x > maxX) {
					x = defaultX;
					y += itemWidth + padding;
					if(y > maxY) {
						//we filled up the entire inventory, so lets leave
						break;
					}
				}
				i++;
			}
			g_itemsInInventory = mainProtag->inventory.size();

			if(mainProtag->inventory.size() > 0 && mainProtag->inventory.size()- 1 -inventorySelection < mainProtag->inventory.size()) {
			    string description = mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->script[0];
				//first line is a comment so take off the //
				description = description.substr(2);
			    adventureUIManager->escText->updateText( description, WIN_WIDTH *g_fontsize, 0.9);
            } else {
				adventureUIManager->escText->updateText("", WIN_WIDTH * g_fontsize, 0.9);
			}
			
		} else {
			inventoryMarker->show = 0;
			inventoryText->show = 0;
		}

		//sines for item bouncing
		g_elapsed_accumulator += elapsed;
		g_itemsinea = sin(g_elapsed_accumulator / 300) * 10 + 30;
		g_itemsineb = sin((g_elapsed_accumulator - 1400) / 300) * 10 + 30;
		g_itemsinec = sin((g_elapsed_accumulator + 925) / 300) * 10 + 30;

		if(g_elapsed_accumulator > 1800) {
			g_elapsed_accumulator -= 1800;
		}
		

		// ENTITY MOVEMENT
		//dont update movement while transitioning
		if(!transition) {
			for(long long unsigned int i=0; i < g_entities.size(); i++){
				if(g_entities[i]->isWorlditem) {
					//make it bounce
					int index = g_entities[i]->bounceindex;
					if(index == 0) {
						g_entities[i]->floatheight = g_itemsinea;
					} else if (index == 1) {
						g_entities[i]->floatheight = g_itemsineb;
					} else {
						g_entities[i]->floatheight = g_itemsinec;
					} 
					
				}
				door* taken = g_entities[i]->update(g_doors, elapsed);
				//added the !transition because if a player went into a map with a door located in the same place
				//as they are in the old map (before going to the waypoint) they would instantly take that door
				if(taken != nullptr && !transition) {
					//player took this door
					//clear level
					
					//we will now clear the map, so we will save the door's destination map as a string
					const string savemap = "maps/" + taken->to_map + ".map";
					const string dest_waypoint = taken->to_point;

					//render this frame

					clear_map(g_camera);
					load_map(renderer, savemap, dest_waypoint);
					

					//clear_map() will also delete engine tiles, so let's re-load them (but only if the user is map-editing)
					if(canSwitchOffDevMode) { init_map_writing(renderer);}
					


					break;
				}
			}
		}

		

		//did the protag die?
		if(protag->hp <= 0 && protag->essential) {
			playSound(-1, g_deathsound, 0);
			
			if(!canSwitchOffDevMode) {
				clear_map(g_camera);
				SDL_Delay(5000);
				load_map(renderer, "maps/sp-death/sp-death.map","a");
			}
			protag->hp = 0.1;
			//if(canSwitchOffDevMode) { init_map_writing(renderer);}
		}

		//late november 2021 - projectiles are now updated after entities are - that way
		//if a behemoth has trapped the player in a tight corridor, their hitbox will hit the player before being
		//destroyed in the wall
		//update projectiles
		for(auto n : g_projectiles) {
			n->update(elapsed);
		}

		//delete projectiles with expired lifetimes
		for(int i = 0; i < g_projectiles.size(); i ++) {
			if(g_projectiles[i]->lifetime <= 0) {
				delete g_projectiles[i];
				i--;
			}
		}

		//triggers
		for (int i = 0; i < g_triggers.size(); i++) {
			if(!g_triggers[i]->active) {continue;}
			rect trigger = {g_triggers[i]->x, g_triggers[i]->y, g_triggers[i]->width, g_triggers[i]->height};
			entity* checkHim = searchEntities(g_triggers[i]->targetEntity);
			if(checkHim == nullptr) {continue;}
			rect movedbounds = rect(checkHim->bounds.x + checkHim->x, checkHim->bounds.y + checkHim->y, checkHim->bounds.width, checkHim->bounds.height);
			if(RectOverlap(movedbounds, trigger) && (checkHim->z > g_triggers[i]->z && checkHim->z < g_triggers[i]->z + g_triggers[i]->zeight)  ) {
				adventureUIManager->blip = g_ui_voice; 
				adventureUIManager->sayings = &g_triggers[i]->script;
				adventureUIManager->talker = narrarator;
				narrarator->dialogue_index = -1;
				narrarator->sayings = g_triggers[i]->script;				
				adventureUIManager->continueDialogue();
				//we need to break here if we loaded a new map
				//definately definately revisit this if you are having problems
				//with loading maps and memorycorruption
				//!!!
				if(transition) {
					break;		
				}
				
				g_triggers[i]->active = 0;
				
			}	
		}

		//worldsounds
		for (int i = 0; i < g_worldsounds.size(); i++) {
			g_worldsounds[i]->update(elapsed);
		}
		
		//transition
		{
			if (transition) {
				g_forceEndDialogue = 0;
				//onframe things
				SDL_LockTexture(transitionTexture, NULL, &transitionPixelReference, &transitionPitch);
				
				memcpy( transitionPixelReference, transitionSurface->pixels, transitionSurface->pitch * transitionSurface->h);
				Uint32 format = SDL_PIXELFORMAT_ARGB8888;
				SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
				Uint32* pixels = (Uint32*)transitionPixelReference;
				int numPixels = transitionImageWidth * transitionImageHeight;
				Uint32 transparent = SDL_MapRGBA( mappingFormat, 0, 0, 0, 255 );
				// Uint32 halftone = SDL_MapRGBA( mappingFormat, 50, 50, 50, 128);
				transitionDelta += g_transitionSpeed + 0.02 * transitionDelta;
				for(int x = 0;  x < transitionImageWidth; x++) {
					for(int y = 0; y < transitionImageHeight; y++) {
						int dest = (y * transitionImageWidth) + x;
						int src =  (y * transitionImageWidth) + x;
						
						if(pow(pow(transitionImageWidth/2 - x,2) + pow(transitionImageHeight + y,2),0.5) < transitionDelta) {
							pixels[dest] = 0;
							
						} else {
							// if(pow(pow(transitionImageWidth/2 - x,2) + pow(transitionImageHeight + y,2),0.5) < 10 + transitionDelta) {
							// 	pixels[dest] = halftone;
							// } else {
								pixels[dest] = transparent;
							// }
						}
					}
				}

				ticks = SDL_GetTicks();
				elapsed = ticks - lastticks;

				SDL_UnlockTexture(transitionTexture);
				SDL_RenderCopy(renderer, transitionTexture, NULL, NULL);

				if(transitionDelta > transitionImageHeight + pow(pow(transitionImageWidth/2,2) + pow(transitionImageHeight,2),0.5)) {
					transition = 0;
					
				}
			} else {
				transitionDelta = transitionImageHeight;
			}
        
   	 	}

		SDL_RenderPresent(renderer);
		
		//update music
		if(musicUpdateTimer > 500) {
			musicUpdateTimer = 0;
			if(g_musicNodes.size() > 0) {
				newClosest = protag->Get_Closest_Node(g_musicNodes);
				if(closestMusicNode == nullptr && !g_mute) { 
					Mix_PlayMusic(newClosest->blip, -1); 
					Mix_VolumeMusic(g_music_volume * 128);
					closestMusicNode = newClosest; 
				} else { 

					//Segfaults, todo is initialize these musicNodes to have something
					if(newClosest->name != closestMusicNode->name) {
						//D(newClosest->name);
						if(newClosest->name == "silence") {
							Mix_FadeOutMusic(1000);
						}
						closestMusicNode = newClosest;
						//change music
						Mix_FadeOutMusic(1000);
						musicFadeTimer = 0;
						fadeFlag = 1;
						
					}
				}
			}
			//check for any cues
			for (auto x : g_cueSounds) {
				if(x->played == 0 && Distance(x->x, x->y, protag->x + protag->width/2, protag->y) < x->radius) {
					x->played = 1;
					playSound(-1, x->blip, 0);
				}
			}
			
		}
		if(fadeFlag && musicFadeTimer > 1000 && newClosest != 0) {
			fadeFlag = 0;
			Mix_HaltMusic();
			Mix_FadeInMusic(newClosest->blip, -1, 1000);
			
		}

		//wakeup manager if it is sleeping
		if(adventureUIManager->sleepflag) {
			adventureUIManager->continueDialogue();
		}
	}


	clear_map(g_camera);
	delete adventureUIManager;
	close_map_writing();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_FreeSurface(transitionSurface);
	SDL_DestroyTexture(transitionTexture);
	SDL_DestroyTexture(background);
	SDL_DestroyTexture(g_shadowTexture);
	SDL_DestroyTexture(g_shadowTextureAlternate);
	IMG_Quit();
	Mix_CloseAudio();
	TTF_Quit();
	
	return 0;
}

int interact(float elapsed, entity* protag) {
	//M("interact()");
	SDL_Rect srect;
		switch(protag->animation) {
			
		case 0:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

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

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y -= 30;
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 30;
			} else {
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

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 55;
			} else {
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

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y += 30;
			if(protag->flip == SDL_FLIP_NONE) {
				srect.x -= 30;
			} else {
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
		case 4:
			srect.h = protag->bounds.height;
			srect.w = protag->bounds.width;

			srect.x = protag->getOriginX() - srect.w/2;
			srect.y = protag->getOriginY() - srect.h/2;

			srect.y += 55;

			srect = transformRect(srect);
			// if(drawhitboxes) {
			// 	SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
			// 	SDL_RenderFillRect(renderer, &srect);
			// 	SDL_RenderPresent(renderer);	
			// 	SDL_Delay(500);
			// }
			break;
		}


		for (int i = 0; i < g_entities.size(); i++) {

			SDL_Rect hisrect = {g_entities[i]->x+ g_entities[i]->bounds.x, g_entities[i]->y + g_entities[i]->bounds.y, g_entities[i]->bounds.width, g_entities[i]->bounds.height};				
			hisrect = transformRect(hisrect);
			
			if(g_entities[i] != protag && RectOverlap(hisrect, srect)) {
				if(g_entities[i]->isWorlditem) {
					//add item to inventory

					//if the item exists, dont make a new one
					indexItem* a = nullptr;
					for(auto x : g_indexItems) {
						//substr because worlditems have the name "ITEM-" + whatever their file is called
						if(g_entities[i]->name.substr(5) == x->name) {
							a = x;
						}
					}
					//no resource found, so lets just make one
					if(a == nullptr) {
						a = new indexItem(g_entities[i]->name.substr(5), 0);
					}
					
					mainProtag->getItem(a, 1);
					delete g_entities[i];
					return 0;
				}
				if(g_entities[i]->tangible &&g_entities[i]->sayings.size() > 0) {
					if(g_entities[i]->animlimit != 0) {
						g_entities[i]->animate = 1;
					}
					//make ent look at player, if they have the frames
					if(g_entities[i]->turnToFacePlayer && g_entities[i]->yframes >= 5) {
						
						int xdiff = (g_entities[i]->getOriginX()) - (protag->getOriginX());
						int ydiff = (g_entities[i]->getOriginY()) - (protag->getOriginY());
						int axdiff = ( abs(xdiff) - abs(ydiff) );
						if(axdiff > 0) {
							//xaxis is more important
							g_entities[i]->animation = 2;
							if(xdiff > 0) {
								g_entities[i]->flip = SDL_FLIP_NONE;
							} else {
								g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
							}
						} else {
							//yaxis is more important
							g_entities[i]->flip = SDL_FLIP_NONE;
							if(ydiff > 0) {
								g_entities[i]->animation = 0;
							} else {
								g_entities[i]->animation = 4;
							}
						}
						if(abs(axdiff) < 45) {
							if(xdiff > 0) {
								g_entities[i]->flip = SDL_FLIP_NONE;
							} else {
								g_entities[i]->flip = SDL_FLIP_HORIZONTAL;
							}
							if(ydiff > 0) {
								g_entities[i]->animation = 1;
							} else {
								g_entities[i]->animation = 3;
							}
						}
					}
					
					adventureUIManager->blip = g_entities[i]->voice;
					adventureUIManager->sayings = &g_entities[i]->sayings;
					adventureUIManager->talker = g_entities[i];
					adventureUIManager->talker->dialogue_index = -1;
					g_forceEndDialogue = 0;
					adventureUIManager->continueDialogue();
					//removing this in early july to fix problem moving after a script changes map
					//may cause unexpected problems
					//protag_is_talking = 1;
					return 0;
				}
			}
			//no one to talk to, lets do x instead (heres where it goes)
			if(use_cooldown <= 0) {
				// if(inPauseMenu) {
				// 	inPauseMenu = 0;
				// 	adventureUIManager->hideInventoryUI();
				// } else {
				// 	inPauseMenu = 1;
				// 	adventureUIManager->showInventoryUI();
				// }
				
			}

		}

	//we didnt have anything to interact with- lets do a dash
	// if(g_dash_cooldown < 0 && protag_can_move) {
	// 	M("dash");
	// 	//convert frame to angle
	// 	float angle = convertFrameToAngle(protag->frame, protag->flip);
	// 	protag->xvel += 500 * (1 - (protag->friction * 3)) * cos(angle);
	// 	protag->yvel += 500 * (1 - (protag->friction * 3)) * sin(angle);
	// 	protag->spinningMS = 700;
	// 	playSound(-1, g_spin_sound, 0);
	// 	g_dash_cooldown = g_max_dash_cooldown;
	// }
	return 0;
}

void getInput(float &elapsed) {
	for (int i = 0; i < 16; i++) {
		oldinput[i] = input[i];
	}

	SDL_PollEvent(&event);

	if(keystate[SDL_SCANCODE_W]) {
		camy-= 4;
	}
	if(keystate[SDL_SCANCODE_A]) {
		camx-=4;
	}
	if(keystate[SDL_SCANCODE_S]) {
		camy+=4;
	}
	if(keystate[SDL_SCANCODE_D]) {
		camx+=4;
	}
	if(keystate[SDL_SCANCODE_G] && !inputRefreshCanSwitchOffDevMode && canSwitchOffDevMode) {
		if(canSwitchOffDevMode) {
			devMode = !devMode;
			g_zoom_mod = 1;
			g_update_zoom = 1;
			marker->x = -1000;
			markerz-> x = -1000;
		}
		if(devMode) {
			floortexDisplay->show = 1;
			captexDisplay->show = 1;
			walltexDisplay->show = 1;
		} else {
			floortexDisplay->show = 0;
			captexDisplay->show = 0;
			walltexDisplay->show = 0;
			// float scalex = ((float)WIN_WIDTH / 1920) * g_defaultZoom;
			// float scaley = scalex;
			SDL_RenderSetScale(renderer, scalex * g_zoom_mod, scalex * g_zoom_mod);
		}
		
	} 
	if(keystate[SDL_SCANCODE_G]){
		inputRefreshCanSwitchOffDevMode = 1;
	} else {
		inputRefreshCanSwitchOffDevMode = 0;
	}
	
	protag_can_move = !protag_is_talking;
	if(protag_can_move ) {
		protag->shooting = 0;
		protag->left = 0;
		protag->right = 0;
		protag->up = 0;
		protag->down = 0;
		g_cameraAimingOffsetXTarget = 0;
		g_cameraAimingOffsetYTarget = 0;
		

		if(keystate[bindings[4]]&& !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames) {
			protag->shoot_up();
			g_cameraAimingOffsetYTarget = 1;
		}

		if(keystate[bindings[5]]&& !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames) {
			protag->shoot_down();
			g_cameraAimingOffsetYTarget = -1;
		}

		if(keystate[bindings[6]]&& !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames) {
			protag->shoot_left();
			g_cameraAimingOffsetXTarget = -1;
		}

		if(keystate[bindings[7]]&& !inPauseMenu && g_cur_diagonalHelpFrames > g_diagonalHelpFrames) {
			protag->shoot_right();
			g_cameraAimingOffsetXTarget = 1;
		}

		//if we aren't pressing any shooting keys, reset g_cur_diagonalhelpframes
		if( ! (keystate[bindings[4]] || keystate[bindings[5]] || keystate[bindings[6]] || keystate[bindings[7]]) ) {
			g_cur_diagonalHelpFrames = 0;
		} else {
			g_cur_diagonalHelpFrames++;
		}

		//normalize g_cameraAimingOffsetTargetVector
		float len = pow(pow(g_cameraAimingOffsetXTarget,2) + pow(g_cameraAimingOffsetYTarget,2),0.5);
		if(!isnan(len) && len != 0) {
			g_cameraAimingOffsetXTarget/= len;
			g_cameraAimingOffsetYTarget/= len;
		}

		if(keystate[bindings[9]]) {
			input[9] = 1;	
		} else {
			input[9] = 0;
		}

		if(keystate[bindings[10]]) {
			input[10] = 1;
		} else {
			input[10] = 0;
		}
		
		if(keystate[bindings[0]]) {
			if(inPauseMenu && SoldUIUp <= 0) {
				playSound(-1, g_menu_manip_sound, 0);
				//if(inventorySelection - itemsPerRow >= 0) {
					inventorySelection-= itemsPerRow;
					
					
				//}
				SoldUIUp = (oldUIUp) ? 6 : 30;				
			} else {
				protag->move_up();
			}
			oldUIUp = 1;
		} else {
			oldUIUp = 0;
			SoldUIUp = 0;
		}
		SoldUIUp--;
		
		if(keystate[bindings[1]]) {
			if(inPauseMenu && SoldUIDown <= 0) {
				playSound(-1, g_menu_manip_sound, 0);
				//if(inventorySelection + itemsPerRow < g_itemsInInventory) {
					

					if( ceil((float)(inventorySelection+1) / (float)itemsPerRow) < (g_itemsInInventory / g_inventoryRows) ) {
						inventorySelection += itemsPerRow;
					}
				//}
				SoldUIDown = (oldUIDown) ? 6 : 30;
			} else {
				protag->move_down();
			}
			oldUIDown = 1;
		} else {
			oldUIDown = 0;
			SoldUIDown = 0;
		}
		SoldUIDown--;
		
		if(keystate[bindings[2]]) {
			if(inPauseMenu && SoldUILeft <= 0) {
				playSound(-1, g_menu_manip_sound, 0);
				if(inventorySelection > 0) {
					if(inventorySelection % itemsPerRow != 0) {
						inventorySelection--;
					}
				}
				SoldUILeft = (oldUILeft) ? 6 : 30;
			} else {
				protag->move_left();
			}
			oldUILeft = 1;
		} else {
			oldUILeft = 0;
			SoldUILeft = 0;
		}
		SoldUILeft--;
		
		
		
		
		

		if(keystate[bindings[3]]) {
			if(inPauseMenu && SoldUIRight <= 0) {
				playSound(-1, g_menu_manip_sound, 0);
				if(inventorySelection <= g_itemsInInventory) {
					//dont want this to wrap around
					if(inventorySelection % itemsPerRow != itemsPerRow -1 ) { 
						inventorySelection++;
					}
				}
				SoldUIRight = (oldUIRight) ? 6 : 30;
			} else {
				protag->move_right();
			}
			oldUIRight = 1;
		} else {
			oldUIRight = 0;
			SoldUIRight = 0;
		}
		SoldUIRight--;

		
		// //fix inventory input
		// if(inventorySelection < 0) {
		// 	inventorySelection = 0;
		// }

		//check if the stuff is onscreen
		if(inventorySelection >= (g_inventoryRows * itemsPerRow) + (inventoryScroll * itemsPerRow) ) {
			inventoryScroll++;
		} else {
			if(inventorySelection < (inventoryScroll * itemsPerRow) ) {
				inventoryScroll--;
			}
		}

		//constrain inventorySelection based on itemsInInventory
		if(inventorySelection > g_itemsInInventory - 1) {
			//M(g_itemsInInventory - 1);
			inventorySelection = g_itemsInInventory - 1;
		}

		if(inventorySelection < 0) {
			inventorySelection = 0;
		}

		if(keystate[bindings[12]] && !old_pause_value && protag_can_move) {
			//pause menu
			if(inPauseMenu) {
				playSound(-1, g_menu_close_sound, 0);
				inPauseMenu = 0;
				elapsed = 16;
				adventureUIManager->hideInventoryUI();
			
			} else {
				playSound(-1, g_menu_open_sound, 0);
				inPauseMenu = 1;
				inventorySelection = 0;
				adventureUIManager->showInventoryUI();
			}
		}
		
		if(keystate[bindings[12]]) {
			old_pause_value = 1;
		} else {
			old_pause_value = 0;
		}
		
	} else {
		//reset shooting offsets
		g_cameraAimingOffsetXTarget = 0;
		g_cameraAimingOffsetYTarget = 0;
		

		if(keystate[bindings[2]] && !left_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index--;
				if(adventureUIManager->response_index < 0) {
					adventureUIManager->response_index++;
				}
			}
			left_ui_refresh = 1;
		} else if(!keystate[bindings[2]]){ left_ui_refresh = 0;}
		if(keystate[bindings[3]] && !right_ui_refresh) {
			if(adventureUIManager->askingQuestion) {
				adventureUIManager->response_index++;
				if(adventureUIManager->response_index > adventureUIManager->responses.size() - 1) {
					adventureUIManager->response_index--;
				}
			}
			right_ui_refresh = 1;
		} else if(!keystate[bindings[3]]) { right_ui_refresh = 0;}
		protag->stop_hori();
		protag->stop_verti();
	}

	if(keystate[bindings[8]]) {
		input[8] = 1;
	} else {
		input[8] = 0;
	}

	if(keystate[bindings[9]]) {
		input[9] = 1;
	} else {
		input[9] = 0;
	}

	//mapeditor cancel button
	if(keystate[SDL_SCANCODE_X]) {
		devinput[4] = 1;
	}

	int markeryvel = 0;
	//mapeditor cursor vertical movement for keyboards
	if(keystate[SDL_SCANCODE_KP_PLUS]) {
		markeryvel = 1;
	} else {
		keyboard_marker_vertical_modifier_refresh = 1;
	}

	if(keystate[SDL_SCANCODE_KP_MINUS]){
		markeryvel = -1;
	} else {
		keyboard_marker_vertical_modifier_refresh_b = 1;
	}

	if(markeryvel != 0) {
		if(g_holdingCTRL) {
			if(markeryvel > 0 && keyboard_marker_vertical_modifier_refresh) {
				wallstart -= 64;
			}
			else if(markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b) {
				wallstart += 64;
			}
			if(wallstart < 0) {
				wallstart = 0;
			} else {
				if(wallstart > 64 * g_layers) {
					wallstart = 64 * g_layers;
				}
				if(wallstart > wallheight - 64) {
					wallstart = wallheight - 64;
				}
			}
		} else {
			if(markeryvel > 0 && keyboard_marker_vertical_modifier_refresh) {
				wallheight -= 64;
			}
			else if(markeryvel < 0 && keyboard_marker_vertical_modifier_refresh_b) {
				wallheight += 64;
			}
			if(wallheight < wallstart + 64) {
				wallheight = wallstart + 64;
			} else {
				if(wallheight > 64 * g_layers) {
					wallheight = 64 * g_layers;
				}
			}
			
		}
	}
	if(keystate[SDL_SCANCODE_KP_PLUS]){
		keyboard_marker_vertical_modifier_refresh = 0;
	}
	
	if(keystate[SDL_SCANCODE_KP_MINUS]){
		keyboard_marker_vertical_modifier_refresh_b = 0;
	}
	


	if(keystate[bindings[11]] && !old_z_value && !inPauseMenu) {
		if(protag_is_talking == 1) {
			if(!adventureUIManager->typing) {
				adventureUIManager->continueDialogue();
			}
		}	
	} else if (keystate[bindings[11]] && !old_z_value && inPauseMenu){
		//select item in pausemenu
		//only if we arent running a script
		if(protag_can_move && adventureUIManager->sleepingMS <= 0 && mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->script.size() > 0 && mainProtag->inventory.size()  > 0) {
			//call the item's script
			//D(mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->name);
			adventureUIManager->blip = g_ui_voice; 
			adventureUIManager->sayings = &mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->script;
			adventureUIManager->talker = protag;
			protag->dialogue_index = -1;
			protag->sayings = mainProtag->inventory[mainProtag->inventory.size()- 1 -inventorySelection].first->script;
			adventureUIManager->continueDialogue();
			//if we changed maps/died/whatever, close the inventory
			if(transition) {
				inPauseMenu = 0;
				adventureUIManager->hideInventoryUI();
			}
			old_z_value = 1;
		}
	}
	//D(mainProtag->inventory[mainProtag->inventory.size() - 1 -inventorySelection].first->name);
	dialogue_cooldown -= elapsed;
	if(keystate[bindings[11]] && !inPauseMenu) {
		if(protag_is_talking == 1) { //advance or speedup diaglogue 
			text_speed_up = 50;
		}	
		if(protag_is_talking == 0) {
			if(dialogue_cooldown < 0) {
				interact(elapsed, protag);
			}
		}
		old_z_value = 1;
	} else {
		
		//reset text_speed_up
		text_speed_up = 1;
		old_z_value = 0;
	}
	if(keystate[bindings[11]] && inPauseMenu) {
		old_z_value = 1;
	} else if (inPauseMenu){
		old_z_value = 0;
	}

	if(protag_is_talking == 2) {
		protag_is_talking = 0;
		dialogue_cooldown = 500;
	}
	
	if(keystate[SDL_SCANCODE_LSHIFT] && devMode) {
		protag->xmaxspeed = 145;
		protag->ymaxspeed = 145;
	}
	if(keystate[SDL_SCANCODE_LCTRL] && devMode) {
		protag->xmaxspeed = 20;
		protag->ymaxspeed = 20;
	}
	if(keystate[SDL_SCANCODE_CAPSLOCK] && devMode) {
		protag->xmaxspeed = 750;
		protag->ymaxspeed = 750;
	}
	
	if(keystate[SDL_SCANCODE_SLASH] && devMode) {
		
	}
	//make another entity of the same type as the last spawned
	if(keystate[SDL_SCANCODE_K] && devMode) {
		devinput[1] = 1;
	}
	if(keystate[SDL_SCANCODE_C] && devMode) {
		devinput[2] = 1;
	}
	if(keystate[SDL_SCANCODE_V] && devMode) {
		devinput[3] = 1;
	}
	if(keystate[SDL_SCANCODE_B] && devMode) {
		//this is make-trigger
		devinput[0] = 1;
	}
	if(keystate[SDL_SCANCODE_N] && devMode) {
		devinput[5] = 1;
		
	}
	if(keystate[SDL_SCANCODE_M] && devMode) {
		devinput[6] = 1;
		
	}
	if(keystate[SDL_SCANCODE_KP_DIVIDE] && devMode) {
		//decrease gridsize
		devinput[7] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_MULTIPLY] && devMode) {
		//increase gridsize
		devinput[8] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_1] && devMode) {
		//enable/disable collisions
		devinput[9] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_5] && devMode) {
		//triangles
		devinput[10] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_3] && devMode) {
		//debug hitboxes
		devinput[7] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_8] && devMode) {
		devinput[22] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_6] && devMode) {
		devinput[23] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_2] && devMode) {
		devinput[24] = 1;
	}
	if(keystate[SDL_SCANCODE_KP_4] && devMode) {
		devinput[25] = 1;
	}

	if(keystate[SDL_SCANCODE_RETURN] && devMode) {
		//stop player first
		protag->stop_hori();
		protag->stop_verti();
		
		elapsed = 0;
		//pull up console
		devinput[11] = 1;
	
	}

	if(keystate[SDL_SCANCODE_COMMA] && devMode) {
		//make navnode box
		devinput[20] = 1;
	}

	if(keystate[SDL_SCANCODE_PERIOD] && devMode) {
		//make navnode box
		devinput[21] = 1;
	}

	if(keystate[SDL_SCANCODE_ESCAPE]) {
		if(devMode) {
			quit = 1;
		} else {
			quit = 1;
			// if(inPauseMenu) {
			// 	inPauseMenu = 0;
			// 	adventureUIManager->hideInventoryUI();
			// 	clear_map(g_camera);
			// 	load_map(renderer, "maps/sp-title/sp-title.map", "a");
			// }
		}
	}
	if(devMode) {
		g_update_zoom = 0;
		if(keystate[SDL_SCANCODE_Q] && devMode && g_holdingCTRL) {
			g_update_zoom = 1;
			g_zoom_mod-= 0.001 * elapsed;
			
			if(g_zoom_mod < min_scale) {
				g_zoom_mod = min_scale;
			}
			if(g_zoom_mod > max_scale) {
				g_zoom_mod = max_scale;
			}
		}
		
		if(keystate[SDL_SCANCODE_E] && devMode && g_holdingCTRL) {
			g_update_zoom = 1;
			g_zoom_mod+= 0.001 * elapsed;
			
			if(g_zoom_mod < min_scale) {
				g_zoom_mod = min_scale;
			}
			if(g_zoom_mod > max_scale) {
				g_zoom_mod = max_scale;
			}
			
		}
	}
	if(keystate[SDL_SCANCODE_BACKSPACE]) {
		devinput[16] = 1;
	}

	if(keystate[bindings[0]] == keystate[bindings[1]]) {
		protag->stop_verti();
	}

	if(keystate[bindings[2]] == keystate[bindings[3]]) {
		protag->stop_hori();
	}
	

	if(keystate[SDL_SCANCODE_F] && fullscreen_refresh) {
		g_fullscreen = !g_fullscreen;
		if(g_fullscreen) {
			SDL_GetCurrentDisplayMode(0, &DM);

			SDL_GetWindowSize(window, &saved_WIN_WIDTH, &saved_WIN_HEIGHT);	

			SDL_SetWindowSize(window, DM.w, DM.h);
			SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);	
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

			if(devMode) {
				SDL_ShowCursor(0);
			}

		} else {
			
			SDL_SetWindowFullscreen(window, 0);

			//restore saved width/height
			SDL_SetWindowSize(window, saved_WIN_WIDTH, saved_WIN_HEIGHT);
			SDL_GetWindowSize(window, &WIN_WIDTH, &WIN_HEIGHT);
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			
			if(devMode) { 
				SDL_ShowCursor(1);
			}
		}
	}

	if(keystate[SDL_SCANCODE_F]) {
		fullscreen_refresh = 0;
	} else {
		fullscreen_refresh = 1;
	}

	if(keystate[SDL_SCANCODE_1] && devMode) {
		devinput[16] = 1;
	}

	if(keystate[SDL_SCANCODE_2] && devMode) {
		devinput[17] = 1;
	}

	if(keystate[SDL_SCANCODE_3] && devMode) {
		devinput[18] = 1;
	}
	if(keystate[SDL_SCANCODE_3] && devMode) {
		devinput[18] = 1;
	}
	if(keystate[SDL_SCANCODE_L] && devMode) {
		devinput[19] = 1;
	}
}