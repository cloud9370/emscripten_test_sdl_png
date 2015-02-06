#include <cstdio>
#include <cstring>
#include <string>
#include <emscripten.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <json/json.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "data.h"
#include "testogg.h"

extern "C"
{
#include <png.h>
}

#define OPENAL_MAX_LENGTH 10
ALCdevice *device;
ALCcontext *context;
ALuint buffer[OPENAL_MAX_LENGTH];
ALuint source[OPENAL_MAX_LENGTH];
void initOpenAL()
{
    printf("OpenAL Init\n");
    device = alcOpenDevice(0);
    context = 0;
    if(device)
    {
        printf("Open Device success\n");
        context = alcCreateContext(device, 0);
        if(context)
        {
            printf("Make context\n");
            alcMakeContextCurrent(context);
        }
    }
    alGenBuffers(OPENAL_MAX_LENGTH, buffer);
    alGenSources(OPENAL_MAX_LENGTH, source);

    ALfloat listenerPos[]={0.0,0.0,4.0};
    ALfloat listenerVel[]={0.0,0.0,0.0};
    ALfloat listenerOri[]={0.0,0.0,1.0, 0.0,1.0,0.0};

    ALfloat source0Pos[]={ -2.0, 0.0, 0.0};
    ALfloat source0Vel[]={ 0.0, 0.0, 0.0};

    alListenerfv(AL_POSITION,listenerPos);
    alListenerfv(AL_VELOCITY,listenerVel);
    alListenerfv(AL_ORIENTATION,listenerOri);

    alSourcef(source[0], AL_PITCH, 1.0f);
    alSourcef(source[0], AL_GAIN, 1.0f);
    alSourcefv(source[0], AL_POSITION, source0Pos);
    alSourcefv(source[0], AL_VELOCITY, source0Vel);
    alSourcei(source[0], AL_BUFFER,buffer[0]);
    alSourcei(source[0], AL_LOOPING, AL_TRUE);
}
void endOpenAL()
{
    alDeleteSources(OPENAL_MAX_LENGTH, source);
    alDeleteBuffers(OPENAL_MAX_LENGTH, buffer);
    alcMakeContextCurrent(0);
    if(context) alcDestroyContext(context);
    context = 0;
    if(device) alcCloseDevice(device);
    device = 0;
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

//============================================================================
void AsyncPrepareOnLoad(const char *file)
{
    printf("AsyncPrepareOnLoad called [%s]\n", file);
}
void AsyncPrepareOnError(const char *file)
{
    printf("AsyncPrepareOnError called [%s]\n", file);
}
void testAsyncPrepare()
{
    printf("Call async prepare\n");
    emscripten_async_prepare("/data/pretty_pal.png", AsyncPrepareOnLoad, AsyncPrepareOnError);
}
//============================================================================

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
        testAsyncPrepare();
    }
    else
    {
        printf("Load [%s] asynchronize\n", pngFile);
        emscripten_async_wget("pretty_pal.png", pngFile, asyncOnLoad, asyncOnErr);
        emscripten_async_wget("cgbgm_b0.ogg", "/data/cgbgm_b0.ogg", asyncOnLoad, asyncOnErr);
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
bool checkSound = false;
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
            if(pngSurface)
            {
                SDL_FreeSurface(pngSurface);
                pngSurface = 0;
            }
            pngSurface = SDL_CreateSurfaceFromPNG("/data/pretty_pal.png");
        }
        else
        {
            //printf("File not exists /data/pretty_pal.png\n");
        }
    }
    if(!checkSound)
    {
        FILE *testFp = fopen("/data/cgbgm_b0.ogg", "rb");
        if(testFp)
        {
            printf("File exists /data/cgbgm_b0.ogg\n");
            fclose(testFp);
            checkSound = true;
            Mix_Chunk *pChunk = Mix_LoadWAV("/data/cgbgm_b0.ogg");
            if(pChunk)
            {
                if(alIsSource(source[0])) printf("source[0] is source\n");
                else printf("source[0] is NOT source\n");
                ALint state;
                alGetSourcei(source[0], AL_SOURCE_STATE, &state);
                if(state==AL_STOPPED)
                {
                    printf("Sound stopped\n");
                }
                else if(state==AL_PLAYING)
                {
                    alSourceStop(source[0]);
                    printf("Sound playing\n");
                }
                else
                {
                    printf("Sound unknown state\n");
                }
                static DecodeData decData;
                decodeOgg("/data/cgbgm_b0.ogg", &decData);
                alBufferData(buffer[0], AL_FORMAT_STEREO16, decData.buf, decData.len, 22050);
                //alBufferData(buffer[0], AL_FORMAT_STEREO16, pChunk->abuf, pChunk->alen, 22050);
                alSourcei(source[0], AL_BUFFER,buffer[0]);
                printf("Has Chunk\n");
                Mix_FreeChunk(pChunk);
                alSourcePlay(source[0]);
            }
            else
            {
                printf("Mix Error [%s]\n", Mix_GetError());
            }
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

void asyncOnLoad2(void *arg, const char *file)
{
    printf("asyncOnLoad2 called\n");
}
void asyncOnErr2(void *arg, int status)
{
    printf("asyncOnErr2\n");
}
void asyncOnProgress2(void *arg, int status)
{
    printf("asyncOnProgress2\n");
}
void testAsyncGet()
{
    /*
    emscripten_async_wget2(
        "pretty_pal.png", // url
        "pretty_pal.png", // file
        "POST", // requesttype
        "v=bmp", // param
        0, // arg
        asyncOnLoad2, // onload
        asyncOnErr2, // onerror
        asyncOnProgress2); // onprogress
    */
}

void testEmscripten()
{
    initOpenAL();
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
    int result = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 1024);
    if(result == -1)
    {
        printf("Audio open failed.\n");
    }
    else
    {
        printf("Audio open success.\n");
    }

    #ifdef EMSCRIPTEN

    //emscripten_async_wget("cgbgm_b0.ogg", "cgbgm_b0.ogg", asyncAudioOnLoad, asyncAudioOnErr);

	//emscripten_set_main_loop(callback_test, 60, 1);
    testAsyncPrepare();
    mountFS();
    emscripten_set_main_loop(callback_test, 0, 1);
    #endif /*EMSCRIPTEN*/
	printf("Finish main\n");

    Mix_Quit();
    TTF_Init();
    SDL_Quit();
    endOpenAL();
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
