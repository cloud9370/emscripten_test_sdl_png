#include "interact.h"
#include <emscripten.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <chewing.h>

EMSCRIPTEN_KEEPALIVE extern "C" void interCall()
{
    printf("test\n");
}

#define CHEWING_NEW_TEST 1

const char *szUrl = "http://localhost:6931/data/";

const char *szFile[] =
{
    "ch_index_begin.dat",
    "ch_index_phone.dat",
    "dict.dat",
    "fonetree.dat",
    "ph_index.dat",
    "pinyin.tab",
    "swkb.dat",
    "symbols.dat",
    "us_freq.dat",
    0
};

enum
{
    CW_INIT,
    CW_DOWN,
    CW_TEST,
    CW_END
};

ChewingContext *ct = 0;
int iChewingState = 0;
int iChewingCount = 0;
int iChewingDownState = 0;
std::string strChewingDownUrl;
std::string strChewingFilePath;
int selKey[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 0};

void testChewingInit()
{
    mkdir("/libchewing", S_IRWXU);
    iChewingState = CW_INIT;
    iChewingCount = 0;
    iChewingDownState = 0;
}

void createChewingContext()
{
}

static void cwOnLoad(const char *szFile)
{
    printf("Download [%s] finished\n", szFile);
    iChewingDownState = 2;
}
static void cwOnError(const char *szFile)
{
    printf("Download [%s] error\n", szFile);
}
void testChewingDown()
{
    switch(iChewingDownState)
    {
    case 0: // Start download
        if(szFile[iChewingCount])
        {
            strChewingDownUrl = szUrl;
            strChewingDownUrl += szFile[iChewingCount];
            strChewingFilePath = "/libchewing/";
            strChewingFilePath += szFile[iChewingCount];
            iChewingDownState ++;
            if(emscripten_async_prepare(strChewingFilePath.c_str(), cwOnLoad, cwOnError))
            {
                emscripten_async_wget(strChewingDownUrl.c_str(), strChewingFilePath.c_str(), cwOnLoad, cwOnError);
            }
        }
        else
        {
            // Download finished.
            iChewingDownState = 3;
        }
        break;
    case 1: // Wait for download
        break;
    case 2:
        iChewingCount ++;
        iChewingDownState = 0;
        break;
    case 3: // Download process done;
        #ifdef CHEWING_NEW_TEST
        ct = chewing_new();
        if(ct)
        {
            printf("Init libchewing\n");
            chewing_set_selKey(ct, selKey, 9);
            chewing_set_maxChiSymbolLen(ct, 10);
            chewing_set_candPerPage(ct, 9);
        }
        #endif /*CHEWING_NEW_TEST*/
        iChewingState = CW_TEST;
        break;
    }
}

#ifndef CHEWING_NEW_TEST
void testChewingTest()
{
    printf("=================================================\n");
    printf("Test libchewing\n");
    chewing_Init("/", ".");
    ct = chewing_new();

    if(ct)
    {
        if(chewing_get_ChiEngMode(ct) == CHINESE_MODE)
        {
            printf("Chinese mode\n");
        }
        else
        {
            printf("English mode\n");
            printf("Change to Chinese mode\n");
            chewing_set_ChiEngMode(ct, CHINESE_MODE);
        }
        if(chewing_get_ChiEngMode(ct) == CHINESE_MODE)
        {
            printf("Chinese mode changed\n");
        }
        printf("Set Select Key\n");
        chewing_set_selKey(ct, selKey, 9);

        printf("Set Chinese symbol max length\n");
        chewing_set_maxChiSymbolLen(ct, 10);

        chewing_set_candPerPage(ct, 9);

        chewing_handle_Default(ct, 'x');
        chewing_handle_Default(ct, 'm');
        chewing_handle_Default(ct, '4');
        chewing_handle_Default(ct, 't');
        chewing_handle_Default(ct, '8');
        chewing_handle_Default(ct, '6');
        chewing_handle_Enter(ct);

        char *buf = chewing_commit_String(ct);
        if(buf)
        {
            printf("Buffer : [%s]\n", buf);
            free(buf);
        }
        else
        {
            printf("chewing_commit_String ERROR\n");
        }

        chewing_delete(ct);
        chewing_Terminate();
        ct = 0;
    }
    else
    {
        printf("ERROR!!  ****chewing_new**** failed\n");
        int *pTest = (int *)calloc(1, sizeof(int));
        if(pTest)
        {
            printf("allocate success\n");
            free(pTest);
        }
    }

    iChewingState = CW_END;
}
#else /*CHEWING_NEW_TEST*/
void testChewingTest()
{
}
#endif /*CHEWING_NEW_TEST*/

/**
 * Reference:
 * https://starforcefield.wordpress.com/2012/08/13/%E6%8E%A2%E7%B4%A2%E6%96%B0%E9%85%B7%E9%9F%B3%E8%BC%B8%E5%85%A5%E6%B3%95%EF%BC%9A%E4%BD%BF%E7%94%A8libchewing/
 */
void testChewing()
{
    switch(iChewingState)
    {
    case CW_INIT:
        printf("CW_INIT start\n");
        iChewingState = CW_DOWN;
        break;
    case CW_DOWN:   testChewingDown();          break;
    case CW_TEST:   testChewingTest();          break;
    case CW_END:
    default:
        break;
    }
}

void testChewingKeyDown(SDL_Event *event)
{
    // Web版的SDL似乎有問題，此處手動對應回SDLKey值
    switch(event->key.keysym.sym)
    {
    case 0xba:  event->key.keysym.sym = SDLK_SEMICOLON; break;
    case 0xbb:  event->key.keysym.sym = SDLK_EQUALS;    break;
    case 0xbd:  event->key.keysym.sym = SDLK_MINUS;     break;
    default: break;
    }

    SDL_keysym *pKeySym = &(event->key.keysym);


    if(ct)
    {
        char cKey = 0; // use for keypad.
        SDLKey keySym = pKeySym->sym;
        switch(keySym)
        {
        case SDLK_CAPSLOCK:     chewing_handle_Capslock(ct);  break;
        case SDLK_BACKSPACE:    chewing_handle_Backspace(ct); break;
        case SDLK_UP:           chewing_handle_Up(ct);        break;
        case SDLK_DOWN:         chewing_handle_Down(ct);      break;
        case SDLK_ESCAPE:       chewing_handle_Esc(ct);       break;
        case SDLK_DELETE:       chewing_handle_Del(ct);       break;
        case SDLK_TAB:          chewing_handle_Tab(ct);       break;
        case SDLK_PAGEUP:       chewing_handle_PageUp(ct);    break;
        case SDLK_PAGEDOWN:     chewing_handle_PageDown(ct);  break;
        case SDLK_HOME:         chewing_handle_Home(ct);      break;
        case SDLK_END:          chewing_handle_End(ct);       break;
        //case SDLK_NUMLOCK:      chewing_handle_Numlock(ct);   break;

        case SDLK_KP0:          cKey = '0';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP1:          cKey = '1';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP2:          cKey = '2';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP3:          cKey = '3';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP4:          cKey = '4';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP5:          cKey = '5';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP6:          cKey = '6';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP7:          cKey = '7';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP8:          cKey = '8';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP9:          cKey = '9';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_PERIOD:    cKey = '.';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_DIVIDE:    cKey = '/';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_MULTIPLY:  cKey = '*';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_MINUS:     cKey = '-';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_PLUS:      cKey = '+';     chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_ENTER:     cKey = '\r';    chewing_handle_Numlock(ct, cKey);   break;
        case SDLK_KP_EQUALS:    cKey = '=';     chewing_handle_Numlock(ct, cKey);   break;

        case SDLK_SPACE:
            if(pKeySym->mod & KMOD_SHIFT)
            {
                //printf("Shift Space pressed.\n");
                chewing_handle_ShiftSpace(ct);
                if(chewing_get_ShapeMode(ct))   chewing_set_ShapeMode(ct, 0);
                else                            chewing_set_ShapeMode(ct, 1);
            }
            else if(pKeySym->mod & KMOD_CTRL)
            {
                if(chewing_get_ChiEngMode(ct) == CHINESE_MODE)  chewing_set_ChiEngMode(ct, SYMBOL_MODE);
                else                                            chewing_set_ChiEngMode(ct, CHINESE_MODE);
            }
            else                            chewing_handle_Space(ct);
            break;
        case SDLK_LEFT:
            if(pKeySym->mod & KMOD_SHIFT)   chewing_handle_ShiftLeft(ct);
            else                            chewing_handle_Left(ct);
            break;

        case SDLK_RIGHT:
            if(pKeySym->mod & KMOD_SHIFT)   chewing_handle_ShiftRight(ct);
            else                            chewing_handle_Right(ct);
            break;

        default:
            chewing_handle_Default(ct, (int)keySym);
            break;

        case SDLK_RETURN:
            chewing_handle_Enter(ct);
            {
                /*
                char *szBuf = chewing_commit_String(ct);
                if(szBuf)
                {
                    printf("IME Enter [%s]\n", szBuf);
                    chewing_free(szBuf);
                }
                */
            }
            break;
        }

        Uint32 u32 = pKeySym->unicode;
        int iZuinCount = 0;
        std::string strZuin, strAux, strBuf;
        //================================================
        //printf("chewing_zuin_Check [%d]\n", chewing_zuin_Check(ct));
        //if(!chewing_zuin_Check(ct))
        {
            char *szZuin= chewing_zuin_String(ct, &iZuinCount);
            if(szZuin)
            {
                strZuin = szZuin;
                chewing_free(szZuin);
            }
        }
        //================================================
        if(chewing_aux_Check(ct))
        {
            char *szAux = chewing_aux_String(ct);
            if(szAux)
            {
                strAux = szAux;
                chewing_free(szAux);
            }
        }
        //================================================
        if(chewing_buffer_Check(ct))
        {
            char *szBuf = chewing_buffer_String(ct);
            if(szBuf)
            {
                strBuf = szBuf;
                chewing_free(szBuf);
            }
        }
        //================================================
        printf("[%3d][%3x][%3x]Chewing cursor[%d] zuinCheck[%d] zuinCount[%d] zuin[%s] aux[%s] buffer [%s]\n",
            keySym, keySym, u32, chewing_cursor_Current(ct),
            chewing_zuin_Check(ct),
            iZuinCount, strZuin.c_str(), strAux.c_str(), strBuf.c_str());

        chewing_cand_Enumerate(ct);
        int totalCand = chewing_cand_TotalChoice(ct);
        if(totalCand > 0)
        {
            printf("Candidate [%d]\n", totalCand);
            while(chewing_cand_hasNext(ct))
            {
                char *szCand = chewing_cand_String(ct);
                printf("[%s]", szCand);
                chewing_free(szCand);
            }
            printf("\n");
        }
        printf("cand CD[%d] TP[%d] CPP[%d] CP[%d]\n",
            chewing_cand_CheckDone(ct),
            chewing_cand_TotalPage(ct),
            chewing_cand_ChoicePerPage(ct),
            chewing_cand_CurrentPage(ct));
    }
}

void testChewingDraw(SDL_Surface *screen, TTF_Font *font)
{
    if(ct)
    {
        SDL_Color colorWhite = {255, 255, 255};
        SDL_Color colorRed = {255, 0, 0};
        SDL_Surface *text = 0;
        std::string strBuf;
        SDL_Rect dst = {0, 0, 0, 0};
        char *szOut = 0;

        if(chewing_get_ChiEngMode(ct) == CHINESE_MODE)  strBuf = "注音";
        else                                            strBuf = "英數";
        if(chewing_get_ShapeMode(ct))   strBuf += "/全型";
        else                            strBuf += "/半型";
        dst.x = 240;
        dst.y = 240;
        text = TTF_RenderText_Solid(font, strBuf.c_str(), colorWhite);
        SDL_BlitSurface(text, 0, screen, &dst);
        SDL_FreeSurface(text);

        strBuf = "注音:";
        if(chewing_buffer_Check(ct))
        {
            szOut = chewing_buffer_String(ct);
            if(szOut)
            {
                strBuf += szOut;
                chewing_free(szOut);
                szOut = 0;
            }
        }
        dst.y += 16;
        text = TTF_RenderText_Solid(font, strBuf.c_str(), colorWhite);
        SDL_BlitSurface(text, 0, screen, &dst);
        SDL_FreeSurface(text);

        int iZuinCount;
        strBuf = "符號:";
        szOut = chewing_zuin_String(ct, &iZuinCount);
        if(szOut)
        {
            strBuf += szOut;
            chewing_free(szOut);
            szOut = 0;
        }
        dst.y += 16;
        text = TTF_RenderText_Solid(font, strBuf.c_str(), colorWhite);
        SDL_BlitSurface(text, 0, screen, &dst);
        SDL_FreeSurface(text);

        strBuf = "輸出:";
        if(chewing_commit_Check(ct))
        {
            szOut = chewing_commit_String(ct);
            if(szOut)
            {
                strBuf += szOut;
                chewing_free(szOut);
                szOut = 0;
            }
        }
        dst.y += 16;
        text = TTF_RenderText_Solid(font, strBuf.c_str(), colorWhite);
        SDL_BlitSurface(text, 0, screen, &dst);
        SDL_FreeSurface(text);

        chewing_cand_Enumerate(ct);
        if(chewing_cand_TotalChoice(ct) > 0)
        {
            int iTotalPage      = chewing_cand_TotalPage(ct);
            int iCurrentPage    = chewing_cand_CurrentPage(ct);
            int iChoicePerPage  = chewing_cand_ChoicePerPage(ct);
            int iLoop = 0;
            strBuf = "候選:";
            while(chewing_cand_hasNext(ct))
            {
                char *szCand = chewing_cand_String(ct);
                //printf("[%s]", szCand);
                if(szCand)
                {
                    strBuf += (char)selKey[iLoop];
                    strBuf += szCand;
                    strBuf += " ";
                    chewing_free(szCand);
                }
                iLoop ++ ;
                if(iLoop >= iChoicePerPage) break;
            }
            char temp[256];
            sprintf(temp, "(%d/%d)", iCurrentPage+1, iTotalPage);
            strBuf += temp;
            dst.y += 16;
            text = TTF_RenderText_Solid(font, strBuf.c_str(), colorWhite);
            SDL_BlitSurface(text, 0, screen, &dst);
            SDL_FreeSurface(text);
        }
    }
}
