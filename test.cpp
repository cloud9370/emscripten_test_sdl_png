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
GLubyte gl_atest;
void eventProc()
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
		switch(event.type)
		{
		case SDL_KEYDOWN:
			printf("Key down: %d , Alpha : %d \n",event.key.keysym.scancode,gl_atest);
			int code = event.key.keysym.scancode;
			if ( code == 79)
			{
				gl_atest++;
			}
			else
			{
				gl_atest--;
			}
			
			if ( gl_atest < 0)
			{
				gl_atest = 0;
			}
			if ( gl_atest > 255)
			{
				gl_atest = 255;
			}
            //testChewingKeyDown(&event);
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

GLuint VAOs[2];
GLuint Buffers[2];
GLuint texture[1];
GLuint vertexShaderObject, fragmentShaderObject;
GLuint programObject;

extern GLubyte gl_atest;

GLuint CreateSimpleTexture2D(GLubyte);
void DrawRect(float x , float y , float size_x , float size_y);

void drawGL()
{
	
    glClear(GL_COLOR_BUFFER_BIT);
	//glActiveTexture(GL_TEXTURE0);
	//GLuint textureId =  CreateSimpleTexture2D();
	
	//glUniform1i(textureId , 0);
	
	
    //glBegin(GL_TRIANGLES);
    //glEnd();

    //glBindVertexArray(VAOs[0]);
	//glBindVertexArray(VAOs[1]);
	
	DrawRect(0,0, 1, 1);
	
	DrawRect(-0.5,-0.5, 1, 1);
    
    SDL_GL_SwapBuffers();
}

void callback_test_gl()
{
    drawGL();
	gl_atest++;
    eventProc();
}
#define VERTEX_SHADER " \
    attribute vec4 vp;\n \
	attribute vec2 vt;\n \
	varying vec2 v_TexCoordinate;          \n \
    void main(void)\n \
    {\n \
        gl_Position = vp;\n \
		v_TexCoordinate = vt; \n \
    }\n"

#define FRAGMENT_SHADER " \
	precision mediump float; \n \
	uniform sampler2D u_Texture; \n \
	varying vec2 v_TexCoordinate; \n \
	void main(void)  \
    { \n \
        gl_FragColor =texture2D(u_Texture, v_TexCoordinate) ; \n \
    }"

	
#define VERTEX_INDEX 0
#define COORDINATE_INDEX 1
/*
#define VERTEX_SHADER " \
 uniform mat4 u_MVPMatrix;   \n \
 uniform mat4 u_MVMatrix;     \n attribute vec4 a_Position;   \n \
 attribute vec4 a_Color;      \n attribute vec3 a_Normal;     \n \
 attribute vec2 a_TexCoordinate; \n \
 varying vec3 v_Position;        \n \
 varying vec4 v_Color;          \n \
 varying vec3 v_Normal;         \n \
 varying vec2 v_TexCoordinate;  \n \n\
 void main() \n \
{ \n \
    v_Position = vec3(a_Position); \n  v_Color = a_Color; \n \
    v_TexCoordinate = a_TexCoordinate; \n \
    v_Normal = vec3(u_MVMatrix * vec4(a_Normal, 0.0)); \n gl_Position = u_MVPMatrix * a_Position; \n \
}  \n "

#define FRAGMENT_SHADER " \
precision mediump float; \n \
uniform vec3 u_LightPos; \n \
uniform sampler2D u_Texture; \n \
varying vec3 v_Position;      \n \
varying vec4 v_Color;         \n \
varying vec3 v_Normal;        \n \
varying vec2 v_TexCoordinate; \n \
void main() \n \
{ \n     float distance = length(u_LightPos - v_Position); \n \
    vec3 lightVector = normalize(u_LightPos - v_Position); \n \
    float diffuse = max(dot(v_Normal, lightVector), 0.0); \n \
    diffuse = diffuse * (1.0 / (1.0 + (0.10 * distance))); \n \
    diffuse = diffuse + 0.3;\n \
    gl_FragColor = (v_Color * diffuse * texture2D(u_Texture, v_TexCoordinate)); \n \
}"

*/

GLuint CreateSimpleTexture2D(GLubyte alpha)
{
	GLuint textureId;
	int width = 4 ;
	int height = 4 ; 
	//Image
	GLubyte pixels[ (4 * 4)  * 4] = 
	{
		255,  0 ,   0, alpha,
		255, 255,   0, alpha,
		255, 255,   255, alpha,
		255, 255,   255, alpha,
		
		 0,   0, 255, alpha,
		 0, 255,   0, alpha,
		 0,  70 , 70, alpha,
		 255, 255,   255, alpha,
		 
		70,  70,   0, alpha,
		70,  80, 100, alpha,
		255, 255,   255, alpha,
		255, 0 , 255, alpha,
		
		70,  70,   70, alpha,
		70,  55, 100, alpha,
		255, 255,   22, alpha,
		255, 55 , 255, alpha
		
	};
	GLenum error = glGetError();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1 , &textureId);
	
	glBindTexture( GL_TEXTURE_2D , textureId );
	error = glGetError();
	glTexImage2D(GL_TEXTURE_2D , 0 , GL_RGBA ,  width , height , 0 , GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error Code:%d",error);
	}
	glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	return textureId;
}

struct Vertex {
	GLfloat position[3];
	GLfloat coordinate[2];
};


void DrawRect(float x , float y , float size_x , float size_y)
{
	Vertex vertices[6] = {
		{ {x- (size_x / 2) ,  y - (size_y / 2) , 0.0} , {0,0} },
		{ {x+ (size_x / 2) , y - (size_y / 2) , 0.0} , {1,0} },
		{ {x+ (size_x / 2) , y + (size_y / 2) , 0.0} , {1,1} },
		{ {x+ (size_x / 2) , y + (size_y / 2) , 0.0} , {1,1} },
		{ {x- (size_x / 2) , y + (size_y / 2) , 0.0} , {0,1} },
		{ {x- (size_x / 2) , y - (size_y / 2) , 0.0} , {0,0} }
	};
	
	// Create and bind VAO
    glGenVertexArrays(1, VAOs);
    glBindVertexArray(VAOs[0]);
		
	// Create and Bind Buffers
	glGenBuffers(1, Buffers);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
    glBufferData(GL_ARRAY_BUFFER,  6 * sizeof(Vertex), vertices, GL_STATIC_DRAW);
	
	// Create Texture
	GLuint textureId =  CreateSimpleTexture2D(gl_atest);
	
	glUniform1i(textureId , 0);
	
	// Set Vertex Attribute Value
	glEnableVertexAttribArray(VERTEX_INDEX);
	glVertexAttribPointer(VERTEX_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex,position));
	glEnableVertexAttribArray(COORDINATE_INDEX);
	glVertexAttribPointer(COORDINATE_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex,coordinate));
	
	// Draw Rect 
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glFlush();
	
	glDisableVertexAttribArray(VERTEX_INDEX);
	glDisableVertexAttribArray(COORDINATE_INDEX);
	//Clear Buffers
	glDeleteBuffers( 1, &Buffers[0]);
	glDeleteTextures( 1 , &textureId);
 
}

void testGLInit()
{
    printf("testGLInit\n");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_Init(SDL_INIT_VIDEO);
    //TTF_Init();
    //font = TTF_OpenFont("FreeSans.ttf", 12);
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
	
	gl_atest = 125;
	
    printf("GL version: [%s]\n", glGetString(GL_VERSION));
	glClearColor(0,0,0,0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glEnable(GL_BLEND);
	
	glViewport(0,0,640,480);
	
	//Set up Shader
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
	/*
	GLfloat vertices[5 * 4] = 
    {
		-0.1, -0.1, 0.0 , 1.0,
        0.1, -0.1, 0.0 , 1.0,
        0.1,  0.1, 0.0 , 1.0,
        -0.1, 0.1, 0.0 , 1.0,
		-0.1, -0.1, 0.0 , 1.0
    };
	
	GLfloat color[5 * 4] = 
	{
		 1.0, 0.0, 0.0 , 1.0,
		 0.0, 1.0, 0.0 , 1.0,
		 0.0, 0.0, 1.0 , 1.0,
		 1.0, 1.0, 0.0 , 1.0,
		 1.0, 0.0, 1.0 , 1.0
	};*/
	
	
	
	
	// Create Program
    programObject = glCreateProgram();
    glAttachShader(programObject, vertexShaderObject);
    glAttachShader(programObject, fragmentShaderObject);
	
	// Set Attribute to Program
	glBindAttribLocation(programObject, VERTEX_INDEX , "vp");
	glBindAttribLocation(programObject, COORDINATE_INDEX , "vt");
	
	//Link & Use Program
    glLinkProgram(programObject);
    glUseProgram(programObject);
	
   
	

	//glVertexAttribPointer(VAOs[1], 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(1));
    //glVertexAttribPointer (VERTEX_INDEX,3, GL_FLOAT, GL_FALSE, 0 , vertices);
	//glVertexAttribPointer (COLOR_INDEX,4, GL_FLOAT, GL_FALSE, 0 , color);
	
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	//CreateSimpleTexture2D();
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
