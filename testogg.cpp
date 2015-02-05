#include <cstdio>
#include <vorbis/vorbisfile.h>
#include <cstring>
#include "testogg.h"

void decodeOgg(const char *file, DecodeData *outData)
{
    printf("decodeOgg enter\n");
    OggVorbis_File vf;
    FILE *fp = fopen(file, "rb");
    if(ov_open_callbacks(fp, &vf, 0, 0,OV_CALLBACKS_DEFAULT)<0)
    {
        printf("ov_open_callbacks failed\n");
        return;
    }
    char **ptr = ov_comment(&vf, -1)->user_comments;
    vorbis_info *vi = ov_info(&vf, -1);
    while(*ptr)
    {
        printf("%s\n", *ptr);
        ++ptr;
    }
    int channel = vi->channels;
    int rate = vi->rate;
    int totalSample = (int)ov_pcm_total(&vf, -1);
    printf("BitStream is %d channel, %dHz\n", channel, rate);
    printf("Decoded length: %d samples\n", totalSample);
    printf("Encoded by: %s\n", ov_comment(&vf, -1)->vendor);
    if(outData)
    {
        outData->ch = channel;
        outData->len = channel * totalSample * 2; // 2 bytes is 16-bits sample
        outData->buf = new char[outData->len];
    }
    int totalBytes = 0;
    while(true)
    {
        char buf[4096];
        int current_section = 0;
        long ret = ov_read(&vf, buf, sizeof(buf), 0, 2, 1, &current_section);
        if(ret == 0)
        {
            printf("Decode end\n");
            break;
        }
        else if(ret<0)
        {
            printf("Decode ERROR\n");
        }
        else
        {
            //printf("Process decode data length[%d]\n", ret);
            if(outData)
            {
                memcpy(outData->buf+totalBytes, buf, ret);
            }
            totalBytes += ret;
        }
    }
    if(outData)
    {
        if(totalBytes != outData->len) printf("Total bytes[%d] is not match outData->len[%d]\n", totalBytes, outData->len);
    }
    printf("Total decode bytes = [%d]\n", totalBytes);
    ov_clear(&vf);
    printf("decodeOgg end\n");
}

