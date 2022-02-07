//---------------------------------------------------------------------------------------
//  Header file that defines the .tfx binary file format. This is the format that will
//  be exported by authoring tools, and is the standard file format for hair data. The game
//  can either read this file directly, or further procsessing can be done offline to improve
//  load times.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

namespace AMD
{
    // TressFXTFXFileHeader Structure
    //
    // This structure defines the header of the file. The actual vertex data follows this as specified by the offsets.
    struct TressFXTFXFileHeader
    {
        float version;                      // Specifies TressFX version number
        unsigned int numHairStrands;        // Number of hair strands in this file. All strands in this file are guide strands.
                                            // Follow hair strands are generated procedurally.
        unsigned int numVerticesPerStrand;  // From 4 to 64 inclusive (POW2 only). This should be a fixed value within tfx value. 
                                            // The total vertices from the tfx file is numHairStrands * numVerticesPerStrand.

                                            // Offsets to array data starts here. Offset values are in bytes, aligned on 8 bytes boundaries,
                                            // and relative to beginning of the .tfx file
        unsigned int offsetVertexPosition;  // Array size: FLOAT4[numHairStrands]
        unsigned int offsetStrandUV;         // Array size: FLOAT2[numHairStrands], if 0 no texture coordinates
        unsigned int offsetVertexUV;         // Array size: FLOAT2[numHairStrands * numVerticesPerStrand], if 0, no per vertex texture coordinates
        unsigned int offsetStrandThickness;  // Array size: float[numHairStrands]
        unsigned int offsetVertexColor;      // Array size: FLOAT4[numHairStrands * numVerticesPerStrand], if 0, no vertex colors

        unsigned int reserved[32];           // Reserved for future versions
    };

    struct TressFXTFXBoneFileHeader
    {
        float version;
        unsigned int numHairStrands;
        unsigned int numInfluenceBones;
        unsigned int offsetBoneNames;
        unsigned int offsetSkinningData;
        unsigned int reserved[32];
    };
} // namespace AMD


