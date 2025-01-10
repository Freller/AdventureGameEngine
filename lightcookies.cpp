#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <array>

#include "lightcookies.h"
#include "globals.h"

using namespace std;

SDL_Texture *addTextures(SDL_Renderer *renderer, array<array<int, g_fogheight>, g_fogwidth> fogcookies, SDL_Texture *&illuminateMe, SDL_Texture *&lightspot, int widthOfIlluminateMe, int heightOfIlluminateMe, int paddingx, int paddingy, int layer)
{
	SDL_SetRenderTarget(renderer, illuminateMe);
	SDL_RenderClear(renderer);
	paddingy = paddingx * (heightOfIlluminateMe / widthOfIlluminateMe);
	SDL_Texture *usingThisTexture = lightspot;

	for (long long unsigned int i = 0; i < fogcookies.size(); i++)
	{
		for (long long unsigned int j = 0; j < fogcookies[0].size(); j++)
		{
			if (fogcookies[i][j])
			{
				usingThisTexture = lightspot;

				if (layer)
				{
					if (g_shc[i][j] == 0)
					{
						usingThisTexture = lightc;
					}
					else if (g_shc[i][j] == 1)
					{
						usingThisTexture = lightb;
					}
					else if (g_shc[i][j] == 2)
					{
						usingThisTexture = lighta;
					}
					else if (g_shc[i][j] == 3)
					{
						usingThisTexture = lightd;
					}
					else if (g_shc[i][j] == 4)
					{
						usingThisTexture = lightcro;
					}
					else if (g_shc[i][j] == 5)
					{
						usingThisTexture = lightbro;
					}
					else if (g_shc[i][j] == 6)
					{
						usingThisTexture = lightaro;
					}
					else if (g_shc[i][j] == 7)
					{
						usingThisTexture = lightdro;
					}
					else if (g_shc[i][j] == 8)
					{
						usingThisTexture = lightcri;
					}
					else if (g_shc[i][j] == 9)
					{
						usingThisTexture = lightbri;
					}
					else if (g_shc[i][j] == 10)
					{
						usingThisTexture = lightari;
					}
					else if (g_shc[i][j] == 11)
					{
						usingThisTexture = lightdri;
					}
				}

				// render the lightspot
				SDL_Rect dstrect = {(int)(i * ((widthOfIlluminateMe) / fogcookies.size())),
														(int)(j * ((heightOfIlluminateMe) / fogcookies[0].size())),
														(int)((widthOfIlluminateMe + paddingx) / fogcookies.size()),
														(int)((heightOfIlluminateMe + paddingy) / fogcookies[0].size())};

				if (g_graphicsquality != 0)
				{
					SDL_SetTextureAlphaMod(usingThisTexture, fogcookies[i][j]);
				}

				SDL_RenderCopy(renderer, usingThisTexture, NULL, &dstrect); //no error
			}
		}
	}

	// illuminateMe is properly lit
	SDL_SetRenderTarget(renderer, NULL);
	return illuminateMe;
}

SDL_Texture *IlluminateTexture(SDL_Renderer *renderer, SDL_Texture *&mask, SDL_Texture *&diffuse, SDL_Texture *&result)
{
	SDL_SetRenderTarget(renderer, result);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, diffuse, NULL, NULL);
	SDL_RenderCopy(renderer, mask, NULL, NULL);
	SDL_SetRenderTarget(renderer, NULL);
	return result;
}

SDL_Texture* animateWater(SDL_Renderer* renderer, SDL_Texture* wtex, SDL_Surface* wsurf, float acc) 
{
  const int imageHeight = 440;
  const int imageWidth = 512;
  int pitch;
  void* pixelReference;

  SDL_LockTexture(wtex, NULL, &pixelReference, &pitch);
  memcpy(pixelReference, g_waterSurface->pixels, g_waterSurface->pitch * g_waterSurface->h);
//  Uint32 format = SDL_PIXELFORMAT_ARGB8888;
//  SDL_PixelFormat* mappingFormat = SDL_AllocFormat( format );
  Uint32* pixels = (Uint32*) pixelReference;
  int numPixels = imageWidth * imageHeight;
  
  for(int x = 0; x < imageWidth; x++) {
    for(int y = 0; y < imageHeight; y++) {
      int xt = 0;
      int yt = 0;
      
      int xp = x + 100 + 10 * acc;
      xp = xp % imageWidth;
      
      Uint32 v = getPixelOfSurface(g_wDistort, xp, y);
      Uint8* p = (Uint8*)&v;
      xt = p[0]/10;
      yt = p[1]/10;


      int dest = (y*imageWidth) + x;
      int src = (((int)(y+yt) % imageHeight) * imageWidth) + (int(x + xt) % imageWidth);


      g_wPixels[dest] = pixels[src];

    }
  }
  for(int i = 0; i < g_wNumPixels; i++) {
    pixels[i] = g_wPixels[i];
  }
  
  SDL_UnlockTexture(wtex);

  return wtex;

}

Uint32 getPixelOfSurface(SDL_Surface *surface, int x, int y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
    case 1:
      return *p;
      //break;

    case 2:
      return *(Uint16 *)p;
      //break;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      //break;

    case 4:
      return *(Uint32 *)p;
      //break;

    default:
      return 0;
  }
}
