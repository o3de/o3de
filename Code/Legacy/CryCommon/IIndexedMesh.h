/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Cry_Color.h"
#include <Vertex.h>

// Description:
//    2D Texture coordinates used by CMesh.
struct SMeshTexCoord
{
    SMeshTexCoord() {}

private:
    float s, t;

public:
    bool IsEquivalent(const SMeshTexCoord& other, float epsilon = 0.00005f) const
    {
        return
            (fabs_tpl(s - other.s) <= epsilon) &&
            (fabs_tpl(t - other.t) <= epsilon);
    }
    ILINE Vec2 GetUV() const
    {
        return Vec2(s, t);
    }
};

// Description:
//    RGBA Color description structure used by CMesh.
struct SMeshColor
{
    SMeshColor() {}

private:
    uint8 r, g, b, a;

public:
    explicit SMeshColor(uint8 otherr, uint8 otherg, uint8 otherb, uint8 othera)
    {
        r = otherr;
        g = otherg;
        b = otherb;
        a = othera;
    }
};
