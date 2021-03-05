#ifndef WAVEFRONT_OBJ_H


#define WAVEFRONT_OBJ_H

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

#include <stdint.h>

class WavefrontObj
{
public:

    WavefrontObj(void);
    ~WavefrontObj(void);

    uint32_t loadObj(const uint8_t *data,uint32_t dlen);
    uint32_t loadOFF(const uint8_t *data, uint32_t dlen);
    uint32_t loadObj(const char *fname); // load a wavefront obj returns number of triangles that were loaded.  Data is persists until the class is destructed.
    uint32_t loadOFF(const char *fname);

    void releaseMesh(void);


    void deepCopyScale(WavefrontObj &dest, float scaleFactor,bool centerMesh);

    bool saveObj(const char *fname);

    static bool saveObj(const char *fname,uint32_t vcount,const float *vertices,uint32_t tcount,const uint32_t *indices);

    uint32_t    mVertexCount;
    uint32_t    mTriCount;
    uint32_t    *mIndices;
    float        *mVertices;
};

#endif
