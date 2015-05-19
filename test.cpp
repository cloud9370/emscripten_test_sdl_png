#include <cstdio>
#include <cstring>
#include <string>
#include <emscripten.h>
#include <SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <json/json.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "data.h"
#include "testogg.h"
#include "interact.h"

#define BUFFER_OFFSET(offset)  ((GLvoid*) (NULL + offset))
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

void eventProc()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
		switch(event.type)
		{
		case SDL_KEYDOWN:
			//printf("Key down\n");
            testChewingKeyDown(&event);
			break;
		}
    }
}

//#define DEF_SHOW_PNG
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
        #ifdef DEF_SHOW_PNG
        SDL_BlitSurface(pngSurface, 0, screen, &rect);
        #endif /*DEF_SHOW_PNG*/
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

    //========================================================================
    // Testing chewing
    testChewing();
    testChewingDraw(screen, font);
    //========================================================================

    SDL_Flip(screen);

    eventProc();
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
    TTF_Quit();
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

#define TEST_FOR_GL

GLuint VAOs[1];
GLuint Buffers[1];
GLuint vertexShaderObject, fragmentShaderObject;
GLuint programObject;
void drawGL()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    //glBegin(GL_TRIANGLES);
    //glEnd();

    glBindVertexArray(VAOs[0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glFlush();
    SDL_GL_SwapBuffers();
}

void callback_test_gl()
{
    drawGL();
    eventProc();
}

#define VERTEX_SHADER " \
    attribute vec3 vp;\n \
    void main(void)\n \
    {\n \
        gl_Position = vec4(vp, 1.0);\n \
    }\n"

#define FRAGMENT_SHADER "void main(void) \
    { \
        gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0); \
    }"

void testGLInit()
{
    printf("testGLInit\n");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    font = TTF_OpenFont("FreeSans.ttf", 12);
	screen = SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
	if(screen)
	{
	}
	else
	{
		SDL_Quit();
	}

    //GLfloat ratio = (GLfloat)640/(GLfloat)480;
    //glShadeModel(GL_SMOOTH);

    //glClearColor(0, 0, 0, 0);

    //glViewport(0, 0, 640, 480);
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();

    //gluPerspective(45.0f, ratio, 1.0f, 100.0f);

    printf("GL version: [%s]\n", glGetString(GL_VERSION));
    glGenVertexArrays(1, VAOs);
    glBindVertexArray(VAOs[0]);

    GLfloat vertices[6][2] = 
    {
        { -0.90, -0.90 },
        {  0.85, -0.90 },
        { -0.90,  0.85 },
        {  0.90, -0.85 },
        {  0.90,  0.90 },
        { -0.85,  0.90 }
    };
    glGenBuffers(1, Buffers);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
    fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar *VertexShaderSource = (const GLchar *)VERTEX_SHADER;
    const GLchar *FragmentShaderSource = (const GLchar *)FRAGMENT_SHADER;
    GLint vlength = (GLint)strlen(VERTEX_SHADER);
    GLint flength = (GLint)strlen(FRAGMENT_SHADER);
    glShaderSource(vertexShaderObject, 1, &VertexShaderSource, &vlength);
    glShaderSource(fragmentShaderObject, 1, &FragmentShaderSource, &flength);
    glCompileShader(vertexShaderObject);
    glCompileShader(fragmentShaderObject);
	GLint Result = GL_FALSE;
	int InfoLogLength;
    glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if(Result == GL_FALSE)
    {
        if ( InfoLogLength > 0 ){
            GLchar *szTemp = new GLchar[InfoLogLength + 1];
            //std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
            glGetShaderInfoLog(vertexShaderObject, InfoLogLength, NULL, szTemp);
            printf("[%d][%s]\n", Result, szTemp);
            delete [] szTemp;
        }
    }
    glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if(Result == GL_FALSE)
    {
        if ( InfoLogLength > 0 ){
            GLchar *szTemp = new GLchar[InfoLogLength + 1];
            //std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
            glGetShaderInfoLog(fragmentShaderObject, InfoLogLength, NULL, szTemp);
            printf("[%d][%s]\n", Result, szTemp);
            delete [] szTemp;
        }
    }
    programObject = glCreateProgram();
    glAttachShader(programObject, vertexShaderObject);
    glAttachShader(programObject, fragmentShaderObject);
    glLinkProgram(programObject);
    glUseProgram(programObject);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    #ifdef EMSCRIPTEN
    emscripten_set_main_loop(callback_test_gl, 0, 1);
    #endif /*EMSCRIPTEN*/

    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
}


int main(int argc, char *argv[])
{
    #ifdef TEST_FOR_GL
    testGLInit();
    #else /*TEST_FOR_GL*/
    printf("argc [%d]\n", argc);
    for(int i=0;i<argc;i++)
    {
        printf("argv[%d] => [%s]\n", i, argv[i]);
    }
    Data d;
    d.test();
    printf("Test\n");
    testChewingInit();
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
    #endif /*TEST_FOR_GL*/
    return 0;
}
