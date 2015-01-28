#ifndef __DATA_H__
#define __DATA_H__

#include <SDL.h>

class Data
{
public:
	Data();
	virtual ~Data();
	virtual void test();
};

SDL_Surface *SDL_CreateSurfaceFromPNG(const char *filename);

#endif /*__DATA_H__*/
