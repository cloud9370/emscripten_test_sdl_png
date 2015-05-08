#ifndef _INTER_ACT_H_
#define _INTER_ACT_H_

#include <SDL.h>
#include <SDL_ttf.h>

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

void interCall();

#ifdef __cplusplus
}
#endif /*__cplusplus*/

void testChewingInit();
void testChewing();
void testChewingKeyDown(SDL_Event *event);
void testChewingDraw(SDL_Surface *screen, TTF_Font *font);

#endif /*_INTER_ACT_H_*/
