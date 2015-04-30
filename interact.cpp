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

int iChewingState = 0;
int iChewingCount = 0;
int iChewingDownState = 0;
std::string strChewingDownUrl;
std::string strChewingFilePath;

void testChewingInit()
{
    mkdir("/libchewing", S_IRWXU);
    iChewingState = CW_INIT;
    iChewingCount = 0;
    iChewingDownState = 0;
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
        iChewingState = CW_TEST;
        break;
    }
}

void testChewingTest()
{
    printf("=================================================\n");
    printf("Test libchewing\n");
    ChewingContext *ct;
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
        int selKey[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', 0};
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
