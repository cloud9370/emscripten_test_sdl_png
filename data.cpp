#include "data.h"
#include <cstdio>
extern "C"
{
#include <png.h>
}

Data::Data()
{
	printf("Data Constructor called\n");
}

Data::~Data()
{
	printf("Data DeConstructor called\n");
}

void Data::test()
{
	printf("Data::test called\n");
}

SDL_Surface *SDL_CreateSurfaceFromPNG(const char *filename)
{
    SDL_Surface *pSurface = 0;

    FILE *fp = fopen(filename, "rb");
    if(!fp)
    {
        printf("SDL_SurfaceFromPNG File [%s] open ERROR\n", filename);
        return 0;
    }

    png_byte header[8];
    fread(header, 1, 8, fp);
    if(png_sig_cmp(header, 0, 8))
        printf("Not PNG\n");
    else
        printf("Is PNG\n");

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(png_ptr)
    {
        png_infop png_info = png_create_info_struct(png_ptr);
        if(png_info)
        {
            if(setjmp(png_jmpbuf(png_ptr)))
            {
                printf("setjmp ERROR\n");
            }
            else
            {
                png_init_io(png_ptr, fp);
                png_set_sig_bytes(png_ptr, 8);
                png_read_info(png_ptr, png_info);

                int width = png_get_image_width(png_ptr, png_info);
                int height = png_get_image_height(png_ptr, png_info);
                int colorType = png_get_color_type(png_ptr, png_info);
                int bitDepth = png_get_bit_depth(png_ptr, png_info);
                int number_of_passes = png_set_interlace_handling(png_ptr);
                png_read_update_info(png_ptr, png_info);

                png_colorp palette = 0;;
                int num_palette = 0;
                if(png_get_PLTE(png_ptr, png_info, &palette, &num_palette))
                {
                    printf("Read PALETTE success num_palette[%d]\n", num_palette);
                }

                png_bytep *data = new png_bytep[height];
                int rowBytes = png_get_rowbytes(png_ptr, png_info);
                int pixelBytes = rowBytes / width;
                printf("Pixel bytes [%d]\n", pixelBytes);
                for(int i=0;i<height;i++)
                {
                    data[i] = new png_byte[rowBytes];
                }
                png_read_image(png_ptr, data);
                pSurface = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY|SDL_SRCALPHA, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
                if(SDL_MUSTLOCK(pSurface)) SDL_LockSurface(pSurface);
                Uint8 bytesPerPixel = pSurface->format->BytesPerPixel;
                if(bytesPerPixel == 4)
                {
                    for(int i=0;i<height;i++)
                    {
                        Uint8 *pDst = ((Uint8 *)pSurface->pixels) + (bytesPerPixel * width * i);
                        for(int j=0;j<width;j++)
                        {
                            Uint32 *pRow = (Uint32 *)pDst;
                            Uint8 r=0, g=0, b=0;
                            if(palette)
                            {
                                int index = (int)data[i][j];
                                if(index < 256)
                                {
                                    r = (Uint8)palette[index].red;
                                    g = (Uint8)palette[index].green;
                                    b = (Uint8)palette[index].blue;
                                }
                            }
                            else
                            {
                                r = (Uint8)data[i][j*pixelBytes];
                                g = (Uint8)data[i][j*pixelBytes+1];
                                b = (Uint8)data[i][j*pixelBytes+2];
                            }
                            pRow[j] = SDL_MapRGB(pSurface->format, r, g, b);
                        }
                    }
                }
                else
                {
                    printf("ERROR, bytesPerPixel[%d] != 4", (int)bytesPerPixel);
                }
                if(SDL_MUSTLOCK(pSurface)) SDL_UnlockSurface(pSurface);

                for(int i=0;i<height;i++)
                {
                    delete [] data[i];
                }
                delete [] data;
                png_read_end(png_ptr, png_info);
            }
            png_destroy_info_struct(png_ptr, &png_info);
        }
        png_destroy_read_struct(&png_ptr, 0, 0);
    }
    fclose(fp);

    return pSurface;
}
