#ifndef __PPCGEKKO__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector>

#include "wavefront.h"

/*!
**
** Copyright (c) 2014 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**



*/

#ifdef _WIN32
#    define strcasecmp _stricmp 
#endif

#pragma warning(disable:4996)

typedef std::vector< uint32_t > IntVector;
typedef std::vector< float > FloatVector;

namespace WAVEFRONT
{

/*******************************************************************/
/******************** InParser.h  ********************************/
/*******************************************************************/
class InPlaceParserInterface
{
public:
    virtual uint32_t ParseLine(uint32_t lineno,uint32_t argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
};

enum SeparatorType
{
    ST_DATA,        // is data
    ST_HARD,        // is a hard separator
    ST_SOFT,        // is a soft separator
    ST_EOS          // is a comment symbol, and everything past this character should be ignored
};

class InPlaceParser
{
public:
    InPlaceParser(void)
    {
        Init();
    }

    InPlaceParser(char *data,uint32_t len)
    {
        Init();
        SetSourceData(data,len);
    }

    InPlaceParser(const char *fname)
    {
        Init();
        SetFile(fname);
    }

    ~InPlaceParser(void);

    void Init(void)
    {
        mQuoteChar = 34;
        mData = 0;
        mLen  = 0;
        mMyAlloc = false;
        for (uint32_t i=0; i<256; i++)
        {
            mHard[i] = ST_DATA;
            mHardString[i*2] = (char)i;
            mHardString[i*2+1] = 0;
        }
        mHard[0]  = ST_EOS;
        mHard[32] = ST_SOFT;
        mHard[9]  = ST_SOFT;
        mHard[13] = ST_SOFT;
        mHard[10] = ST_SOFT;
    }

    void SetFile(const char *fname); // use this file as source data to parse.

    void SetSourceData(char *data,uint32_t len)
    {
        mData = data;
        mLen  = len;
        mMyAlloc = false;
    };

    uint32_t  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

    uint32_t ProcessLine(uint32_t lineno,char *line,InPlaceParserInterface *callback);

    const char ** GetArglist(char *source,uint32_t &count); // convert source string into an arg list, this is a destructive parse.

    void SetHardSeparator(char c) // add a hard separator
    {
        mHard[c] = ST_HARD;
    }

    void SetHard(char c) // add a hard separator
    {
        mHard[c] = ST_HARD;
    }


    void SetCommentSymbol(char c) // comment character, treated as 'end of string'
    {
        mHard[c] = ST_EOS;
    }

    void ClearHardSeparator(char c)
    {
        mHard[c] = ST_DATA;
    }


    void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

    bool EOS(char c)
    {
        if ( mHard[c] == ST_EOS )
        {
            return true;
        }
        return false;
    }

    void SetQuoteChar(char c)
    {
        mQuoteChar = c;
    }

private:


    inline char * AddHard(uint32_t &argc,const char **argv,char *foo);
    inline bool   IsHard(char c);
    inline char * SkipSpaces(char *foo);
    inline bool   IsWhiteSpace(char c);
    inline bool   IsNonSeparator(char c); // non seperator,neither hard nor soft

    bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
    char  *mData;  // ascii data to parse.
    uint32_t    mLen;   // length of data
    SeparatorType  mHard[256];
    char   mHardString[256*2];
    char           mQuoteChar;
};

/*******************************************************************/
/******************** InParser.cpp  ********************************/
/*******************************************************************/
void InPlaceParser::SetFile(const char *fname)
{
    if ( mMyAlloc )
    {
        free(mData);
    }
    mData = 0;
    mLen  = 0;
    mMyAlloc = false;

    FILE *fph = fopen(fname,"rb");
    if ( fph )
    {
        fseek(fph,0L,SEEK_END);
        mLen = ftell(fph);
        fseek(fph,0L,SEEK_SET);
        if ( mLen )
        {
            mData = (char *) malloc(sizeof(char)*(mLen+1));
            size_t ok = fread(mData, mLen, 1, fph);
            if ( !ok )
            {
                free(mData);
                mData = 0;
            }
            else
            {
                mData[mLen] = 0; // zero byte terminate end of file marker.
                mMyAlloc = true;
            }
        }
        fclose(fph);
    }

}

InPlaceParser::~InPlaceParser(void)
{
    if ( mMyAlloc )
    {
        free(mData);
    }
}

#define MAXARGS 512

bool InPlaceParser::IsHard(char c)
{
    return mHard[c] == ST_HARD;
}

char * InPlaceParser::AddHard(uint32_t &argc,const char **argv,char *foo)
{
    while ( IsHard(*foo) )
    {
        const char *hard = &mHardString[*foo*2];
        if ( argc < MAXARGS )
        {
            argv[argc++] = hard;
        }
        foo++;
    }
    return foo;
}

bool   InPlaceParser::IsWhiteSpace(char c)
{
    return mHard[c] == ST_SOFT;
}

char * InPlaceParser::SkipSpaces(char *foo)
{
    while ( !EOS(*foo) && IsWhiteSpace(*foo) ) foo++;
    return foo;
}

bool InPlaceParser::IsNonSeparator(char c)
{
    if ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 ) return true;
    return false;
}


uint32_t InPlaceParser::ProcessLine(uint32_t lineno,char *line,InPlaceParserInterface *callback)
{
    uint32_t ret = 0;

    const char *argv[MAXARGS];
    uint32_t argc = 0;

    char *foo = line;

    while ( !EOS(*foo) && argc < MAXARGS )
    {

        foo = SkipSpaces(foo); // skip any leading spaces

        if ( EOS(*foo) ) break;

        if ( *foo == mQuoteChar ) // if it is an open quote
        {
            foo++;
            if ( argc < MAXARGS )
            {
                argv[argc++] = foo;
            }
            while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
            if ( !EOS(*foo) )
            {
                *foo = 0; // replace close quote with zero byte EOS
                foo++;
            }
        }
        else
        {

            foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

            if ( IsNonSeparator(*foo) )  // add non-hard argument.
            {
                bool quote  = false;
                if ( *foo == mQuoteChar )
                {
                    foo++;
                    quote = true;
                }

                if ( argc < MAXARGS )
                {
                    argv[argc++] = foo;
                }

                if ( quote )
                {
                    while (*foo && *foo != mQuoteChar ) foo++;
                    if ( *foo ) *foo = 32;
                }

                // continue..until we hit an eos ..
                while ( !EOS(*foo) ) // until we hit EOS
                {
                    if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
                    {
                        *foo = 0;
                        foo++;
                        break;
                    }
                    else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
                    {
                        const char *hard = &mHardString[*foo*2];
                        *foo = 0;
                        if ( argc < MAXARGS )
                        {
                            argv[argc++] = hard;
                        }
                        foo++;
                        break;
                    }
                    foo++;
                } // end of while loop...
            }
        }
    }

    if ( argc )
    {
        ret = callback->ParseLine(lineno, argc, argv );
    }

    return ret;
}

uint32_t  InPlaceParser::Parse(InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
{
    assert( callback );
    if ( !mData ) return 0;

    uint32_t ret = 0;

    uint32_t lineno = 0;

    char *foo   = mData;
    char *begin = foo;


    while ( *foo )
    {
        if ( *foo == 10 || *foo == 13 )
        {
            lineno++;
            *foo = 0;

            if ( *begin ) // if there is any data to parse at all...
            {
                uint32_t v = ProcessLine(lineno,begin,callback);
                if ( v ) ret = v;
            }

            foo++;
            if ( *foo == 10 ) foo++; // skip line feed, if it is in the carraige-return line-feed format...
            begin = foo;
        }
        else
        {
            foo++;
        }
    }

    lineno++; // lasst line.

    uint32_t v = ProcessLine(lineno,begin,callback);
    if ( v ) ret = v;
    return ret;
}


void InPlaceParser::DefaultSymbols(void)
{
    SetHardSeparator(',');
    SetHardSeparator('(');
    SetHardSeparator(')');
    SetHardSeparator('=');
    SetHardSeparator('[');
    SetHardSeparator(']');
    SetHardSeparator('{');
    SetHardSeparator('}');
    SetCommentSymbol('#');
}


const char ** InPlaceParser::GetArglist(char *line,uint32_t &count) // convert source string into an arg list, this is a destructive parse.
{
    const char **ret = 0;

    static const char *argv[MAXARGS];
    uint32_t argc = 0;

    char *foo = line;

    while ( !EOS(*foo) && argc < MAXARGS )
    {

        foo = SkipSpaces(foo); // skip any leading spaces

        if ( EOS(*foo) ) break;

        if ( *foo == mQuoteChar ) // if it is an open quote
        {
            foo++;
            if ( argc < MAXARGS )
            {
                argv[argc++] = foo;
            }
            while ( !EOS(*foo) && *foo != mQuoteChar ) foo++;
            if ( !EOS(*foo) )
            {
                *foo = 0; // replace close quote with zero byte EOS
                foo++;
            }
        }
        else
        {

            foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

            if ( IsNonSeparator(*foo) )  // add non-hard argument.
            {
                bool quote  = false;
                if ( *foo == mQuoteChar )
                {
                    foo++;
                    quote = true;
                }

                if ( argc < MAXARGS )
                {
                    argv[argc++] = foo;
                }

                if ( quote )
                {
                    while (*foo && *foo != mQuoteChar ) foo++;
                    if ( *foo ) *foo = 32;
                }

                // continue..until we hit an eos ..
                while ( !EOS(*foo) ) // until we hit EOS
                {
                    if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
                    {
                        *foo = 0;
                        foo++;
                        break;
                    }
                    else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
                    {
                        const char *hard = &mHardString[*foo*2];
                        *foo = 0;
                        if ( argc < MAXARGS )
                        {
                            argv[argc++] = hard;
                        }
                        foo++;
                        break;
                    }
                    foo++;
                } // end of while loop...
            }
        }
    }

    count = argc;
    if ( argc )
    {
        ret = argv;
    }

    return ret;
}

/*******************************************************************/
/******************** Obj.h  ********************************/
/*******************************************************************/


class OBJ : public InPlaceParserInterface
{
public:
    uint32_t    LoadMesh(const char *fname);
    uint32_t    LoadMesh(const uint8_t *data,uint32_t dlen);

    uint32_t    LoadOFF(const char *fname);
    uint32_t    LoadOFF(const uint8_t *data, uint32_t dlen);


    uint32_t    ParseLine(uint32_t lineno,uint32_t argc,const char **argv);  // return TRUE to continue parsing, return FALSE to abort parsing process
    IntVector        mTriIndices;
    FloatVector        mVerts;
    bool            mIsOFF{ false };
    bool            mIsValidOFF{ false };
    uint32_t        mVertexCountOFF{ 0 };
    uint32_t        mFaceCountOFF{ 0 };
    uint32_t        mEdgeCountOFF{ 0 };
};


/*******************************************************************/
/******************** Obj.cpp  ********************************/
/*******************************************************************/

uint32_t OBJ::LoadMesh(const char *fname)
{
    uint32_t ret = 0;

    mVerts.clear();
    mTriIndices.clear();
    mIsOFF = false;

    InPlaceParser ipp(fname);

    ipp.Parse(this);


    return ret;
}

uint32_t OBJ::LoadOFF(const char *fname)
{
    uint32_t ret = 0;

    mIsOFF = true;

    mVerts.clear();
    mTriIndices.clear();

    InPlaceParser ipp(fname);

    ipp.Parse(this);


    return ret;
}

uint32_t OBJ::LoadMesh(const uint8_t *data,uint32_t dlen)
{
    uint32_t ret = 0;

    mIsOFF = false;

    mVerts.clear();
    mTriIndices.clear();

    uint8_t *tdata = new uint8_t[dlen+1];
    tdata[dlen] = 0;
    memcpy(tdata,data,dlen);
    InPlaceParser ipp((char *)tdata,dlen);
    ipp.Parse(this);
    delete []tdata;


    return ret;
}

uint32_t OBJ::LoadOFF(const uint8_t *data, uint32_t dlen)
{
    uint32_t ret = 0;

    mIsOFF = true;

    mVerts.clear();
    mTriIndices.clear();

    uint8_t *tdata = new uint8_t[dlen + 1];
    tdata[dlen] = 0;
    memcpy(tdata, data, dlen);
    InPlaceParser ipp((char *)tdata, dlen);
    ipp.Parse(this);
    delete[]tdata;


    return ret;
}


uint32_t OBJ::ParseLine(uint32_t lineno,uint32_t argc,const char **argv)  // return TRUE to continue parsing, return FALSE to abort parsing process
{
  uint32_t ret = 0;

    if (mIsOFF)
    {
        if ( lineno == 1 )
        {
            mIsValidOFF = false;
            if (argc == 1)
            {
                const char *prefix = argv[0];
                if (strcmp(prefix, "OFF") == 0)
                {
                    mIsValidOFF = true;
                }
            }
        }
        else if (lineno == 2)
        {
            if (argc == 3 && mIsValidOFF )
            {
                const char *vcount = argv[0];
                const char *fcount = argv[1];
                const char *ecount = argv[2];
                mVertexCountOFF = atoi(vcount);
                mFaceCountOFF = atoi(fcount);
                mEdgeCountOFF = atoi(ecount);
            }
            else
            {
                mIsValidOFF = false;
            }
        }
        else if (mIsValidOFF)
        {
            uint32_t index = lineno - 3;
            if (index < mVertexCountOFF)
            {
                if (argc == 3)
                {
                    const char *x = argv[0];
                    const char *y = argv[1];
                    const char *z = argv[2];
                    float _x = (float)atof(x);
                    float _y = (float)atof(y);
                    float _z = (float)atof(z);
                    mVerts.push_back(_x);
                    mVerts.push_back(_y);
                    mVerts.push_back(_z);
                }
                else
                {
                    mIsValidOFF = false;
                }
            }
            else
            {
                index -= mVertexCountOFF;
                if (index < mFaceCountOFF)
                {
                    if (argc == 4)
                    {
                        const char *fcount = argv[0];
                        const char *i1 = argv[1];
                        const char *i2 = argv[2];
                        const char *i3 = argv[3];
                        uint32_t _fcount = atoi(fcount);
                        uint32_t _i1 = atoi(i1);
                        uint32_t _i2 = atoi(i2);
                        uint32_t _i3 = atoi(i3);
                        if (_fcount == 3)
                        {
                            mTriIndices.push_back(_i3);
                            mTriIndices.push_back(_i2);
                            mTriIndices.push_back(_i1);
                        }
                        else
                        {
                            mIsValidOFF = false;
                        }
                    }
                    else
                    {
                        mIsValidOFF = false; // I don't support anything but triangles right now..
                    }
                }
            }
        }
        return 0;
    }

  if ( argc >= 1 )
  {
    const char *foo = argv[0];
    if ( *foo != '#' )
    {
      if ( strcasecmp(argv[0],"v") == 0 && argc == 4 )
      {
        float vx = (float) atof( argv[1] );
        float vy = (float) atof( argv[2] );
        float vz = (float) atof( argv[3] );
        mVerts.push_back(vx);
        mVerts.push_back(vy);
        mVerts.push_back(vz);
      }
      else if ( strcasecmp(argv[0],"f") == 0 && argc >= 4 )
      {
        uint32_t vcount = argc-1;

        uint32_t i1 = (uint32_t)atoi(argv[1])-1;
        uint32_t i2 = (uint32_t)atoi(argv[2])-1;
        uint32_t i3 = (uint32_t)atoi(argv[3])-1;

        mTriIndices.push_back(i3);
        mTriIndices.push_back(i2);
        mTriIndices.push_back(i1);


        if ( vcount >=3 ) // do the fan
        {
          for (uint32_t i=2; i<(vcount-1); i++)
          {
              i2 = i3;
              i3 = (uint32_t)atoi(argv[i+2])-1;
              mTriIndices.push_back(i3);
              mTriIndices.push_back(i2);
              mTriIndices.push_back(i1);
          }
        }
      }
    }
  }

  return ret;
}





};

using namespace WAVEFRONT;

WavefrontObj::WavefrontObj(void)
{
    mVertexCount = 0;
    mTriCount    = 0;
    mIndices     = 0;
    mVertices    = NULL;
}

WavefrontObj::~WavefrontObj(void)
{
    releaseMesh();
    delete mIndices;
    delete mVertices;
}

uint32_t WavefrontObj::loadObj(const uint8_t *data,uint32_t dlen)
{
    uint32_t ret = 0;


    OBJ obj;

    obj.LoadMesh(data,dlen);

    mVertexCount = (uint32_t)obj.mVerts.size()/3;
    mTriCount = (uint32_t)obj.mTriIndices.size()/3;

    if ( mVertexCount )
    {
        mVertices = new float[mVertexCount*3];
        memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount*3);
    }

    if ( mTriCount )
    {
        mIndices = new uint32_t[mTriCount*3];
        memcpy(mIndices,&obj.mTriIndices[0],mTriCount*3*sizeof(uint32_t));
    }
    ret = mTriCount;

    return ret;
}

uint32_t WavefrontObj::loadOFF(const uint8_t *data, uint32_t dlen)
{
    uint32_t ret = 0;


    OBJ obj;

    obj.LoadOFF(data, dlen);

    mVertexCount = (uint32_t)obj.mVerts.size() / 3;
    mTriCount = (uint32_t)obj.mTriIndices.size() / 3;

    if (mVertexCount)
    {
        mVertices = new float[mVertexCount * 3];
        memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount * 3);
    }

    if (mTriCount)
    {
        mIndices = new uint32_t[mTriCount * 3];
        memcpy(mIndices, &obj.mTriIndices[0], mTriCount * 3 * sizeof(uint32_t));
    }
    ret = mTriCount;

    return ret;
}

void WavefrontObj::releaseMesh(void)
{
    delete []mVertices;
    mVertices = 0;
    delete []mIndices;
    mIndices = 0;
    mVertexCount = 0;
    mTriCount = 0;
}

uint32_t WavefrontObj::loadObj(const char *fname) // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
{
    uint32_t ret = 0;


    OBJ obj;

    obj.LoadMesh(fname);

    mVertexCount = (uint32_t)obj.mVerts.size()/3;
    mTriCount = (uint32_t)obj.mTriIndices.size()/3;

    if ( mVertexCount )
    {
        mVertices = new float[mVertexCount*3];
        memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount*3);
    }

    if ( mTriCount )
    {
        mIndices = new uint32_t[mTriCount*3];
        memcpy(mIndices,&obj.mTriIndices[0],mTriCount*3*sizeof(uint32_t));
    }
    ret = mTriCount;

    return ret;
}

uint32_t WavefrontObj::loadOFF(const char *fname) // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
{
    uint32_t ret = 0;


    OBJ obj;

    obj.LoadOFF(fname);

    mVertexCount = (uint32_t)obj.mVerts.size() / 3;
    mTriCount = (uint32_t)obj.mTriIndices.size() / 3;

    if (mVertexCount)
    {
        mVertices = new float[mVertexCount * 3];
        memcpy(mVertices, &obj.mVerts[0], sizeof(float)*mVertexCount * 3);
    }

    if (mTriCount)
    {
        mIndices = new uint32_t[mTriCount * 3];
        memcpy(mIndices, &obj.mTriIndices[0], mTriCount * 3 * sizeof(uint32_t));
    }
    ret = mTriCount;

    return ret;
}

bool WavefrontObj::saveObj(const char *fname)
{
    return saveObj(fname, mVertexCount, mVertices, mTriCount, mIndices);
}

bool WavefrontObj::saveObj(const char *fname,uint32_t vcount,const float *vertices,uint32_t tcount,const uint32_t *indices)
{
    bool ret = false;

    FILE *fph = fopen(fname,"wb");
    if ( fph )
    {
        for (uint32_t i=0; i<vcount; i++)
        {
            fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", vertices[0], vertices[1], vertices[2] );
            vertices+=3;
        }
        for (uint32_t i=0; i<tcount; i++)
        {
            fprintf(fph,"f %d %d %d\r\n", indices[2]+1, indices[1]+1, indices[0]+1 );
            indices+=3;
        }
        fclose(fph);
        ret = true;
    }
    return ret;
}

void WavefrontObj::deepCopyScale(WavefrontObj &dest, float scaleFactor,bool centerMesh)
{
    dest.releaseMesh();
    dest.mVertexCount = mVertexCount;
    dest.mTriCount = mTriCount;
    if (mTriCount)
    {
        dest.mIndices = new uint32_t[mTriCount * 3];
        memcpy(dest.mIndices, mIndices, sizeof(uint32_t)*mTriCount * 3);
    }
    if (mVertexCount)
    {
        float adjustX = 0;
        float adjustY = 0;
        float adjustZ = 0;

        if (centerMesh)
        {
            float bmin[3];
            float bmax[3];

            bmax[0] = bmin[0] = mVertices[0];
            bmax[1] = bmin[1] = mVertices[1];
            bmax[2] = bmin[2] = mVertices[2];

            for (uint32_t i = 1; i < mVertexCount; i++)
            {
                const float *p = &mVertices[i * 3];
                if (p[0] < bmin[0])
                {
                    bmin[0] = p[0];
                }
                if (p[1] < bmin[1])
                {
                    bmin[1] = p[1];
                }
                if (p[2] < bmin[2])
                {
                    bmin[2] = p[2];
                }
                if (p[0] > bmax[0])
                {
                    bmax[0] = p[0];
                }
                if (p[1] > bmax[1])
                {
                    bmax[1] = p[1];
                }
                if (p[2] > bmax[2])
                {
                    bmax[2] = p[2];
                }
            }
            adjustX = (bmin[0] + bmax[0])*0.5f;
            adjustY = bmin[1]; // (bmin[1] + bmax[1])*0.5f;
            adjustZ = (bmin[2] + bmax[2])*0.5f;
        }


        dest.mVertices = new float[mVertexCount * 3];
        for (uint32_t i = 0; i < mVertexCount; i++)
        {
            dest.mVertices[i * 3 + 0] = (mVertices[i * 3 + 0]-adjustX) * scaleFactor;
            dest.mVertices[i * 3 + 1] = (mVertices[i * 3 + 1]-adjustY) * scaleFactor;
            dest.mVertices[i * 3 + 2] = (mVertices[i * 3 + 2]-adjustZ) * scaleFactor;
        }
    }
}

#endif
