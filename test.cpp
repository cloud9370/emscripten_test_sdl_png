#include <cstdio>
#include <cstring>
#include <string>
#include <emscripten.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <json/json.h>
#include "data.h"

extern "C"
{
#include <png.h>
}

bool loadPNG(const char *pngFile)
{
    FILE *fp = fopen(pngFile, "rb");
    char buf[8];
    bool result = true;
    if(fp)
    {
        fread(buf, 1, 8, fp);
        int isPng = !png_sig_cmp((png_const_bytep)buf, (png_size_t)0, (png_size_t)8);
        if(isPng)
        {
            printf("is PNG\n");
        }
        else
        {
            printf("not PNG\n");
        }

        png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        if(!png_ptr)
        {
            result = false;
            printf("png_create_read_struct failed\n");
        }
        else
        {
            png_infop info_ptr = png_create_info_struct(png_ptr);
            if(!info_ptr)
            {
                printf("png_create_info_struct failed\n");
            }
            else
            {
                png_init_io(png_ptr, fp);
                png_set_sig_bytes(png_ptr, 8);

                png_read_info(png_ptr, info_ptr);
            }
            png_destroy_read_struct(&png_ptr, 0, 0);
        }
        fclose(fp);
    }

    return result;
}

SDL_Surface *pngSurface;
void asyncOnLoad(const char *file)
{
    EM_ASM
    (
        console.error('asyncOnLoad Test print out on entering function');
    );
    printf("asyncOnLoad [%s] called\n", file);
    if(pngSurface)
    {
        SDL_FreeSurface(pngSurface);
        pngSurface = 0;
    }
    pngSurface = SDL_CreateSurfaceFromPNG(file);
    EM_ASM
    (
        FS.syncfs(function (err) {
            console.error('asyncOnLoad SYNC');
        });
    );
}
void asyncOnErr(const char *file)
{
    printf("Load [%s] ERROR.\n", file);
}

enum
{
    STATE_WAIT_INIT_FS,
    STATE_GET_FILE,
    STATE_RUN,
    STATE_END,
};
int iMainStat = STATE_WAIT_INIT_FS;
extern "C" void initFsDone()
{
    printf("initFsDone CALLED\n");
    iMainStat = STATE_GET_FILE;
    const char *pngFile = "/data/pretty_pal.png";
    FILE *fp = fopen(pngFile, "rb");
    if(fp)
    {
        asyncOnLoad(pngFile);
        fclose(fp);
        printf("Load [%s] from persistence data\n", pngFile);
    }
    else
    {
        printf("Load [%s] asynchronize\n", pngFile);
        emscripten_async_wget("pretty_pal.png", pngFile, asyncOnLoad, asyncOnErr);
    }
}

void mountFS()
{
    printf("mount fs c-call\n");
    EM_ASM
    (
        console.error('Call mount FS');
        FS.mkdir('/data');
        FS.mount(IDBFS, {}, '/data');
        FS.syncfs(true, function (err) {
            console.error('mount path /data');
            ccall('initFsDone');
        });
    );
    printf("mount fs c-call end\n");
}

void unmountFS()
{
    EM_ASM
    (
        FS.syncfs(function (err) {
            console.error('unmountFS');
        });
        FS.umount('/data');
    );
}

SDL_Surface *screen;
TTF_Font *font;
int count = 0;
bool mountFlag = false;
bool checkFile = false;
void callback_test()
{
    //printf("test open file png\n");
    if(!checkFile)
    {
        FILE *testFp = fopen("/data/pretty_pal.png", "rb");
        if(testFp)
        {
            printf("File exists /data/pretty_pal.png\n");
            fclose(testFp);
            checkFile = true;
        }
        else
        {
            printf("File not exists /data/pretty_pal.png\n");
        }
    }
    count ++ ;
    char szBuf[128];
    memset(szBuf, 0, sizeof(szBuf));
    SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_Color color = {255, 255, 255};

    SDL_Rect rect;
    rect.x = 64;
    rect.y = 64;
    rect.w = pngSurface->w;
    rect.h = pngSurface->h;
    if(pngSurface)
    {
        SDL_BlitSurface(pngSurface, 0, screen, &rect);
    }

    // Counter
    sprintf(szBuf, "Count: %d", count);
    SDL_Surface *text = TTF_RenderText_Solid(font, szBuf, color);
    rect.x = 0;
    rect.y = 0;
    rect.w = text->w;
    rect.h = text->h;
    SDL_BlitSurface(text, 0, screen, &rect);
    SDL_FreeSurface(text);

    SDL_Flip(screen);

    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
		switch(event.type)
		{
		case SDL_KEYDOWN:
			printf("Key down\n");
			break;
		}
    }
}

void testEmscripten()
{
    pngSurface = 0;

	SDL_version compiled;
	SDL_VERSION(&compiled);
	printf("SDL compiled: %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);

	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
    TTF_Init();
    Mix_Init(MIX_INIT_OGG);
    font = TTF_OpenFont("FreeSans.ttf", 12);
	screen = SDL_SetVideoMode(640, 480, 32, SDL_FULLSCREEN|SDL_HWSURFACE);
	if(screen)
	{
	}
	else
	{
		SDL_Quit();
	}

    // Test Audio
    int result = Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
    if(result == -1)
    {
        printf("Audio open failed.\n");
    }
    else
    {
        printf("Audio open success.\n");
    }

    #ifdef EMSCRIPTEN
    /*
    emscripten_async_wget2(
        "test.bmp", // url
        "test.bmp", // file
        "POST", // requesttype
        "v=bmp", // param
        0, // arg
        asyncOnLoad2, // onload
        asyncOnErr2, // onerror
        asyncOnProgress2); // onprogress
    */

    //emscripten_async_wget("cgbgm_b0.ogg", "cgbgm_b0.ogg", asyncAudioOnLoad, asyncAudioOnErr);

	//emscripten_set_main_loop(callback_test, 60, 1);
    mountFS();
    emscripten_set_main_loop(callback_test, 0, 1);
    #endif /*EMSCRIPTEN*/
	printf("Finish main\n");

    Mix_Quit();
    TTF_Init();
    SDL_Quit();
}

void testJSON()
{
    Json::Reader reader;
    Json::Value value;
    std::string str = "{\"user\":\"test\",\"age\":30}";
    reader.parse(str, value);
    std::string temp = value[std::string("user")].asString();
    printf("<JSON> user = [%s]\n", temp.c_str());
}

int main(int argc, char *argv[])
{
    printf("argc [%d]\n", argc);
    for(int i=0;i<argc;i++)
    {
        printf("argv[%d] => [%s]\n", i, argv[i]);
    }
    Data d;
    d.test();
    printf("Test\n");
    testJSON();
    testEmscripten();
    /*
    emscripten_set_main_loop(callback_test, 1, 0);
    printf("Finish main\n");
    */
    //unmountFS();

    #ifdef EMSCRIPTEN
    printf("emscripten defined\n");
    #endif /*EMSCRIPTEN*/
    return 0;
}
