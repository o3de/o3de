/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_MESHUTIL_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_MESHUTIL_H
#pragma once

#include "../../../CryCommon/VertexFormats.h"

namespace stable_rand
{
    void setSeed(uint32 seed);
    float randUnit();
    float randPositive();
    float randBias(float noise);
}

struct SpritePoint
{
    Vec2 pos;
    float size;
    float brightness;
    float rotation;
    ColorF color;

    SpritePoint(const Vec2& _pos, float _brightness)
        : pos(_pos)
        , brightness(_brightness)
        , size(0.1f)
        , rotation(0)
    {
        color.set(1, 1, 1, 1);
    }
    SpritePoint()
        : brightness(1)
        , size(0.1f)
        , rotation(0)
    {
        pos.set(0, 0);
        color.set(1, 1, 1, 1);
    }
};

class MeshUtil
{
public:
    static void GenDisk(float radius, int polySides, int ringCount, bool capInnerHole, const ColorF& clr, float* ringPosArray, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);

    static void GenHoop(float radius, int polySides, float thickness, int ringCount, const ColorF& clr, float noiseStrength, int noiseSeed, float startAngle, float endAngle, float fadeAngle, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
    static void GenTrapezoidFan(int numSideVert, float radius, float startAngleDegree, float endAngleDegree, float centerThickness, const ColorF& clr, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
    static void GenFan(int numSideVert, float radius, float startAngleDegree, float endAngleDegree, const ColorF& clr, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);

    //A falloff fan is a simple coarse fan-shaped mesh with odd number of side-vertices.
    //It's UV mapping spans a strict rectanglular shape (x:[0,1,0]  y:[0,1]).
    //On top of the fan shape, there's a concentric beam which simulates the spike effect.
    static void GenShaft(float radius, float centerThickness, int complexity, float startAngleDegree, float endAngleDegree, const ColorF& clr, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
    static void GenStreak(float dir, float radius, float thickness, const ColorF& clr, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
    static void GenSprites(std::vector<SpritePoint>& spriteList, float aspectRatio,  bool packPivotPos, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
    static void TrianglizeQuadIndices(int quadCount, std::vector<uint16>& idxOut);
    static void GenScreenTile(float x0, float y0, float x1, float y1, ColorF clr, int rowCount, int columnCount, std::vector<SVF_P3F_C4B_T2F>& vertOut, std::vector<uint16>& idxOut);
};

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_COMMON_RENDELEMENTS_MESHUTIL_H
