#ifndef _TEST_OGG_H_
#define _TEST_OGG_H_

class DecodeData
{
public:
    DecodeData():ch(0), len(0), buf(0) {}
    ~DecodeData()
    {
        delete [] buf;
    }
    int ch;
    int len;
    char *buf;
};

void decodeOgg(const char *file, DecodeData *outData);

#endif /*_TEST_OGG_H_*/

