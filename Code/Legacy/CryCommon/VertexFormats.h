/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_VERTEXFORMATS_H
#define CRYINCLUDE_CRYCOMMON_VERTEXFORMATS_H

#pragma once

#include <CryArray.h>
// Stream Configuration options
#define ENABLE_NORMALSTREAM_SUPPORT 1

enum EVertexFormat : uint8
{
    eVF_Unknown,

    // Base stream
    eVF_P3F_C4B_T2F,
    eVF_P3F_C4B_T2F_T2F,
    eVF_P3S_C4B_T2S,
    eVF_P3S_C4B_T2S_T2S, // For UV2 support
    eVF_P3S_N4B_C4B_T2S,

    eVF_P3F_C4B_T4B_N3F2, // Particles.
    eVF_TP3F_C4B_T2F, // Fonts (28 bytes).
    eVF_TP3F_T2F_T3F,  // Miscellaneus.
    eVF_P3F_T3F,       // Miscellaneus. (AuxGeom)
    eVF_P3F_T2F_T3F,   // Miscellaneus.

    // Additional streams
    eVF_T2F,           // Light maps TC (8 bytes).
    eVF_W4B_I4S,  // Skinned weights/indices stream.
    eVF_C4B_C4B,      // SH coefficients.
    eVF_P3F_P3F_I4B,  // Shape deformation stream.
    eVF_P3F,       // Velocity stream.

    eVF_C4B_T2S,     // General (Position is merged with Tangent stream)

    // Lens effects simulation
    eVF_P2F_T4F_C4F,  // primary
    eVF_P2F_T4F_T4F_C4F,

    eVF_P2S_N4B_C4B_T1F,
    eVF_P3F_C4B_T2S,
    eVF_P2F_C4B_T2F_F4B, // UI
    eVF_P3F_C4B,// Auxiliary geometry


    eVF_P3F_C4F_T2F,  //numbering for tracking the new vertex formats and for comparison with testing 23
    eVF_P3F_C4F_T2F_T3F,
    eVF_P3F_C4F_T2F_T3F_T3F,
    eVF_P3F_C4F_T2F_T1F,
    eVF_P3F_C4F_T2F_T1F_T3F,
    eVF_P3F_C4F_T2F_T1F_T3F_T3F,
    eVF_P3F_C4F_T4F_T2F,
    eVF_P3F_C4F_T4F_T2F_T3F,  //30
    eVF_P3F_C4F_T4F_T2F_T3F_T3F,
    eVF_P3F_C4F_T4F_T2F_T1F,
    eVF_P3F_C4F_T4F_T2F_T1F_T3F,
    eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F,
    eVF_P3F_C4F_T2F_T2F_T1F,  //35
    eVF_P3F_C4F_T2F_T2F_T1F_T3F,
    eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F,  //40
    eVF_P4F_T2F_C4F_T4F_T4F,
    eVF_P3F_C4F_T2F_T4F,
    eVF_P3F_C4F_T2F_T3F_T4F,
    eVF_P3F_C4F_T2F_T3F_T3F_T4F,
    eVF_P3F_C4F_T2F_T1F_T4F,  //45
    eVF_P3F_C4F_T2F_T1F_T3F_T4F,
    eVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F,
    eVF_P3F_C4F_T4F_T2F_T4F,
    eVF_P3F_C4F_T4F_T2F_T3F_T4F,
    eVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F,  //50
    eVF_P3F_C4F_T4F_T2F_T1F_T4F,
    eVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F,
    eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F,  //55
    eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F,
    eVF_P4F_T2F_C4F_T4F_T4F_T4F,  //60
    eVF_P3F_C4F_T2F_T4F_T4F,
    eVF_P3F_C4F_T2F_T3F_T4F_T4F,
    eVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F,
    eVF_P3F_C4F_T2F_T1F_T4F_T4F,
    eVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F,  //65
    eVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F,
    eVF_P3F_C4F_T4F_T2F_T4F_T4F,
    eVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F,
    eVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F,
    eVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F,  //70
    eVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F,
    eVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F,  //75
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F,
    eVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F,
    eVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F,

    eVF_Max,
};


typedef Vec4_tpl<int16> Vec4sf;     // Used for tangents only.

struct UCol
{
    union
    {
        uint32 dcolor;
        uint8  bcolor[4];

        struct
        {
            uint8 b, g, r, a;
        };
        struct
        {
            uint8 z, y, x, w;
        };
    };

    // get normal vector from unsigned 8bit integers (can't point up/down and is not normal)
    ILINE Vec3 GetN()
    {
        return Vec3
               (
            (bcolor[0] - 128.0f) / 127.5f,
            (bcolor[1] - 128.0f) / 127.5f,
            (bcolor[2] - 128.0f) / 127.5f
               );
    }

    AUTO_STRUCT_INFO
};

struct Vec3f16
    : public CryHalf4
{
    _inline Vec3f16()
    {
    }
    _inline Vec3f16(f32 _x, f32 _y, f32 _z)
    {
        x = CryConvertFloatToHalf(_x);
        y = CryConvertFloatToHalf(_y);
        z = CryConvertFloatToHalf(_z);
        w = CryConvertFloatToHalf(1.0f);
    }
    float operator[](int i) const
    {
        assert(i <= 3);
        return CryConvertHalfToFloat(((CryHalf*)this)[i]);
    }
    _inline Vec3f16& operator = (const Vec3& sl)
    {
        x = CryConvertFloatToHalf(sl.x);
        y = CryConvertFloatToHalf(sl.y);
        z = CryConvertFloatToHalf(sl.z);
        w = CryConvertFloatToHalf(1.0f);
        return *this;
    }
    _inline Vec3f16& operator = (const Vec4A& sl)
    {
        x = CryConvertFloatToHalf(sl.x);
        y = CryConvertFloatToHalf(sl.y);
        z = CryConvertFloatToHalf(sl.z);
        w = CryConvertFloatToHalf(sl.w);
        return *this;
    }
    _inline Vec3 ToVec3() const
    {
        Vec3 v;
        v.x = CryConvertHalfToFloat(x);
        v.y = CryConvertHalfToFloat(y);
        v.z = CryConvertHalfToFloat(z);
        return v;
    }
};

struct Vec2f16
    : public CryHalf2
{
    _inline Vec2f16()
    {
    }
    _inline Vec2f16(f32 _x, f32 _y)
    {
        x = CryConvertFloatToHalf(_x);
        y = CryConvertFloatToHalf(_y);
    }
    Vec2f16& operator = (const Vec2f16& sl)
    {
        x = sl.x;
        y = sl.y;
        return *this;
    }
    Vec2f16& operator = (const Vec2& sl)
    {
        x = CryConvertFloatToHalf(sl.x);
        y = CryConvertFloatToHalf(sl.y);
        return *this;
    }
    float operator[](int i) const
    {
        assert(i <= 1);
        return CryConvertHalfToFloat(((CryHalf*)this)[i]);
    }
    _inline Vec2 ToVec2() const
    {
        Vec2 v;
        v.x = CryConvertHalfToFloat(x);
        v.y = CryConvertHalfToFloat(y);
        return v;
    }
};


struct SVF_P3F_C4B
{
    Vec3 xyz;
    UCol color;
};

struct SVF_P3F_C4B_T2F
{
    Vec3 xyz;
    UCol color;
    Vec2 st;
};

struct SVF_P3F_C4B_T2F_T2F
{
    Vec3 xyz;
    UCol color;
    Vec2 st;
    Vec2 st2;
};

struct SVF_P2F_C4B_T2F_F4B
{
    Vec2 xy;
    UCol color;
    Vec2 st;
    uint8 texIndex;
    uint8 texHasColorChannel;
    uint8 texIndex2;
    uint8 pad;
};

struct SVF_TP3F_C4B_T2F //Fonts
{
    Vec4 pos;
    UCol color;
    Vec2 st;
};
struct SVF_P3S_C4B_T2S
{
    Vec3f16 xyz;
    UCol color;
    Vec2f16 st;

    AUTO_STRUCT_INFO
};

struct SVF_P3S_C4B_T2S_T2S
{
    Vec3f16 xyz;
    UCol color;
    Vec2f16 st;
    Vec2f16 st2;

    AUTO_STRUCT_INFO
};

struct SVF_P3F_C4B_T2S
{
    Vec3 xyz;
    UCol color;
    Vec2f16 st;
};

struct SVF_P3S_N4B_C4B_T2S
{
    Vec3f16 xyz;
    UCol normal;
    UCol color;
    Vec2f16 st;
};

struct SVF_P2S_N4B_C4B_T1F
{
    CryHalf2 xy;
    UCol normal;
    UCol color;
    float z;
};

struct SVF_T2F
{
    Vec2 st;
};
struct SVF_W4B_I4S
{
    UCol weights;
    uint16 indices[4];
};
struct SVF_C4B_C4B
{
    UCol coef0;
    UCol coef1;
};
struct SVF_P3F_P3F_I4B
{
    Vec3 thin;
    Vec3 fat;
    UCol index;
};
struct SVF_P3F
{
    Vec3 xyz;
};
struct SVF_P3F_T3F
{
    Vec3 p;
    Vec3 st;
};
struct SVF_P3F_T2F_T3F
{
    Vec3 p;
    Vec2 st0;
    Vec3 st1;
};
struct SVF_TP3F_T2F_T3F
{
    Vec4 p;
    Vec2 st0;
    Vec3 st1;
};
struct SVF_P2F_T4F_C4F
{
    Vec2 p;
    Vec4 st;
    Vec4 color;
};

struct SVF_P2F_T4F_T4F_C4F
{
    Vec2 p;
    Vec4 st;
    Vec4 st2;
    Vec4 color;
};

struct SVF_P3F_C4B_I4B_PS4F
{
    Vec3 xyz;
    Vec2 prevXaxis;
    Vec2 prevYaxis;
    UCol color;
    Vec3 prevPos;
    struct SpriteInfo
    {
        uint8       tex_x, tex_y, tex_z, backlight;     // xyzw
    } info;
    Vec2 xaxis;
    Vec2 yaxis;
};

struct SVF_P3F_C4B_T4B_N3F2
{
    Vec3 xyz;
    UCol color;
    UCol st; // st is used as a color, even though st usually refers to a TexCoord
    Vec3 xaxis;
    Vec3 yaxis;
#ifdef PARTICLE_MOTION_BLUR
    Vec3 prevPos;
    Vec3 prevXTan;
    Vec3 prevYTan;
#endif
};

struct SVF_C4B_T2S
{
    UCol color;
    Vec2f16 st;
};

struct SVF_P3F_C4F_T2F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st;
};

struct SVF_P3F_C4F_T2F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
};

struct SVF_P3F_C4F_T2F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
    Vec3 st2;
};

struct SVF_P3F_C4F_T2F_T1F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st;
    float z;
};

struct SVF_P3F_C4F_T2F_T1F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
};

struct SVF_P3F_C4F_T2F_T1F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
    Vec3 st2;
};


struct SVF_P3F_C4F_T4F_T2F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
};

struct SVF_P3F_C4F_T4F_T2F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
};

struct SVF_P3F_C4F_T4F_T2F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
    Vec3 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T1F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
    Vec3 st3;
};

struct SVF_P4F_T2F_C4F_T4F_T4F
{
    Vec4 xyzw;
    Vec2 st0;
    Vec4 color;
    Vec4 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec4 st1;
};

struct SVF_P3F_C4F_T2F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T1F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec4 st1;
};

struct SVF_P3F_C4F_T2F_T1F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T4F_T2F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec4 st2;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
};

struct SVF_P4F_T2F_C4F_T4F_T4F_T4F
{
    Vec4 xyzw;
    Vec2 st0;
    Vec4 color;
    Vec4 st1;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec4 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec3 st1;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T2F_T1F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec4 st1;
    Vec4 st2;
};

struct SVF_P3F_C4F_T2F_T1F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T1F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    float z;
    Vec3 st1;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T4F_T2F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T4F_T2F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
    Vec4 st5;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T4F_T2F_T1F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec4 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
    Vec4 st5;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
    Vec4 st5;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec4 st2;
    Vec4 st3;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
    Vec4 st3;
    Vec4 st4;
};

struct SVF_P3F_C4F_T2F_T2F_T1F_T1F_T3F_T3F_T4F_T4F
{
    Vec3 xyz;
    Vec4 color;
    Vec2 st0;
    Vec2 st1;
    float z0;
    float z1;
    Vec3 st2;
    Vec3 st3;
    Vec4 st4;
    Vec4 st5;
};

struct SVF_P4F_T2F_C4F_T4F_T4F_T4F_T4F
{
    Vec4 xyzw;
    Vec2 st0;
    Vec4 color;
    Vec4 st1;
    Vec4 st2;
    Vec4 st3;
    Vec4 st4;
};


//=============================================================
// Signed norm value packing [-1,+1]

namespace PackingSNorm
{
    ILINE int16 tPackF2B(const float f)
    {
        return (int16)(f * 32767.0f);
    }

    ILINE int16 tPackS2B(const int16 s)
    {
        return (int16)(s * 32767);
    }

    ILINE float tPackB2F(const int16 i)
    {
        return (float)((float)i / 32767.0f);
    }

    ILINE int16 tPackB2S(const int16 s)
    {
        // OPT: "(s >> 15) + !(s >> 15)" works as well
        return (int16)(s / 32767);
    }

    ILINE Vec4sf tPackF2Bv(const Vec4& v)
    {
        Vec4sf vs;

        vs.x = tPackF2B(v.x);
        vs.y = tPackF2B(v.y);
        vs.z = tPackF2B(v.z);
        vs.w = tPackF2B(v.w);

        return vs;
    }

    ILINE Vec4sf tPackF2Bv(const Vec3& v)
    {
        Vec4sf vs;

        vs.x = tPackF2B(v.x);
        vs.y = tPackF2B(v.y);
        vs.z = tPackF2B(v.z);
        vs.w = tPackF2B(1.0f);

        return vs;
    }

    ILINE Vec4 tPackB2F(const Vec4sf& v)
    {
        Vec4 vs;

        vs.x = tPackB2F(v.x);
        vs.y = tPackB2F(v.y);
        vs.z = tPackB2F(v.z);
        vs.w = tPackB2F(v.w);

        return vs;
    }

    ILINE void tPackB2F(const Vec4sf& v, Vec4& vDst)
    {
        vDst.x = tPackB2F(v.x);
        vDst.y = tPackB2F(v.y);
        vDst.z = tPackB2F(v.z);
        vDst.w = 1.0f;
    }

    ILINE void tPackB2FScale(const Vec4sf& v, Vec4& vDst, const Vec3& vScale)
    {
        vDst.x = (float)v.x * vScale.x;
        vDst.y = (float)v.y * vScale.y;
        vDst.z = (float)v.z * vScale.z;
        vDst.w = 1.0f;
    }

    ILINE void tPackB2FScale(const Vec4sf& v, Vec3& vDst, const Vec3& vScale)
    {
        vDst.x = (float)v.x * vScale.x;
        vDst.y = (float)v.y * vScale.y;
        vDst.z = (float)v.z * vScale.z;
    }

    ILINE void tPackB2F(const Vec4sf& v, Vec3& vDst)
    {
        vDst.x = tPackB2F(v.x);
        vDst.y = tPackB2F(v.y);
        vDst.z = tPackB2F(v.z);
    }
};

//=============================================================
// Pip => Graphics Pipeline structures, used for inputs for the GPU's Input Assembler
// These structures are optimized for fast decoding (ALU and bandwidth) and
// might be slow to encode on-the-fly

struct SPipTangents
{
    SPipTangents() {}

private:
    Vec4sf Tangent;
    Vec4sf Bitangent;

public:
    explicit SPipTangents(const Vec4sf& othert, const Vec4sf& otherb, const int16& othersign)
    {
        using namespace PackingSNorm;
        Tangent   = othert;
        Tangent.w   = PackingSNorm::tPackS2B(othersign);
        Bitangent = otherb;
        Bitangent.w = PackingSNorm::tPackS2B(othersign);
    }

    explicit SPipTangents(const Vec4sf& othert, const Vec4sf& otherb, const SPipTangents& othersign)
    {
        Tangent   = othert;
        Tangent.w   = othersign.Tangent.w;
        Bitangent = otherb;
        Bitangent.w = othersign.Bitangent.w;
    }

    explicit SPipTangents(const Vec4sf& othert, const Vec4sf& otherb)
    {
        Tangent   = othert;
        Bitangent = otherb;
    }

    explicit SPipTangents(const Vec3& othert, const Vec3& otherb, const int16& othersign)
    {
        Tangent   = Vec4sf(PackingSNorm::tPackF2B(othert.x), PackingSNorm::tPackF2B(othert.y), PackingSNorm::tPackF2B(othert.z), PackingSNorm::tPackS2B(othersign));
        Bitangent = Vec4sf(PackingSNorm::tPackF2B(otherb.x), PackingSNorm::tPackF2B(otherb.y), PackingSNorm::tPackF2B(otherb.z), PackingSNorm::tPackS2B(othersign));
    }

    explicit SPipTangents(const Vec3& othert, const Vec3& otherb, const SPipTangents& othersign)
    {
        Tangent   = Vec4sf(PackingSNorm::tPackF2B(othert.x), PackingSNorm::tPackF2B(othert.y), PackingSNorm::tPackF2B(othert.z), othersign.Tangent.w);
        Bitangent = Vec4sf(PackingSNorm::tPackF2B(otherb.x), PackingSNorm::tPackF2B(otherb.y), PackingSNorm::tPackF2B(otherb.z), othersign.Bitangent.w);
    }

    explicit SPipTangents(const Quat& other, const int16& othersign)
    {
        Vec3 othert = other.GetColumn0();
        Vec3 otherb = other.GetColumn1();

        Tangent   = Vec4sf(PackingSNorm::tPackF2B(othert.x), PackingSNorm::tPackF2B(othert.y), PackingSNorm::tPackF2B(othert.z), PackingSNorm::tPackS2B(othersign));
        Bitangent = Vec4sf(PackingSNorm::tPackF2B(otherb.x), PackingSNorm::tPackF2B(otherb.y), PackingSNorm::tPackF2B(otherb.z), PackingSNorm::tPackS2B(othersign));
    }

    void ExportTo(Vec4sf& othert, Vec4sf& otherb) const
    {
        othert = Tangent;
        otherb = Bitangent;
    }

    // get normal tangent and bitangent vectors
    void GetTB(Vec4& othert, Vec4& otherb) const
    {
        othert = PackingSNorm::tPackB2F(Tangent);
        otherb = PackingSNorm::tPackB2F(Bitangent);
    }

    // get normal vector (perpendicular to tangent and bitangent plane)
    ILINE Vec3 GetN() const
    {
        Vec4 tng, btg;
        GetTB(tng, btg);

        Vec3 tng3(tng.x, tng.y, tng.z),
        btg3(btg.x, btg.y, btg.z);

        // assumes w 1 or -1
        return tng3.Cross(btg3) * tng.w;
    }

    // get normal vector (perpendicular to tangent and bitangent plane)
    void GetN(Vec3& othern) const
    {
        othern = GetN();
    }

    // get the tangent-space basis as individual normal vectors (tangent, bitangent and normal)
    void GetTBN(Vec3& othert, Vec3& otherb, Vec3& othern) const
    {
        Vec4 tng, btg;
        GetTB(tng, btg);

        Vec3 tng3(tng.x, tng.y, tng.z),
        btg3(btg.x, btg.y, btg.z);

        // assumes w 1 or -1
        othert = tng3;
        otherb = btg3;
        othern = tng3.Cross(btg3) * tng.w;
    }

    // get normal vector sign (reflection)
    ILINE int16 GetR() const
    {
        return PackingSNorm::tPackB2S(Tangent.w);
    }

    // get normal vector sign (reflection)
    void GetR(int16& sign) const
    {
        sign = GetR();
    }

    void TransformBy(const Matrix34& trn)
    {
        Vec4 tng, btg;
        GetTB(tng, btg);

        Vec3 tng3(tng.x, tng.y, tng.z),
        btg3(btg.x, btg.y, btg.z);

        tng3 = trn.TransformVector(tng3);
        btg3 = trn.TransformVector(btg3);

        *this = SPipTangents(tng3, btg3, PackingSNorm::tPackB2S(Tangent.w));
    }

    void TransformSafelyBy(const Matrix34& trn)
    {
        Vec4 tng, btg;
        GetTB(tng, btg);

        Vec3 tng3(tng.x, tng.y, tng.z),
        btg3(btg.x, btg.y, btg.z);

        tng3 = trn.TransformVector(tng3);
        btg3 = trn.TransformVector(btg3);

        // normalize in case "trn" wasn't length-preserving
        tng3.Normalize();
        btg3.Normalize();

        *this = SPipTangents(tng3, btg3, PackingSNorm::tPackB2S(Tangent.w));
    }

    friend struct SMeshTangents;
};

struct SPipQTangents
{
    SPipQTangents() {}

private:
    Vec4sf QTangent;

public:
    explicit SPipQTangents(const Vec4sf& other)
    {
        QTangent = other;
    }

    bool operator ==(const SPipQTangents& other) const
    {
        return
            QTangent[0] == other.QTangent[0] ||
            QTangent[1] == other.QTangent[1] ||
            QTangent[2] == other.QTangent[2] ||
            QTangent[3] == other.QTangent[3];
    }

    bool operator !=(const SPipQTangents& other) const
    {
        return !(*this == other);
    }

    // get quaternion
    ILINE Quat GetQ() const
    {
        Quat q;

        q.v.x = PackingSNorm::tPackB2F(QTangent.x);
        q.v.y = PackingSNorm::tPackB2F(QTangent.y);
        q.v.z = PackingSNorm::tPackB2F(QTangent.z);
        q.w   = PackingSNorm::tPackB2F(QTangent.w);

        return q;
    }

    // get normal vector from quaternion
    ILINE Vec3 GetN() const
    {
        const Quat q = GetQ();
        return q.GetColumn2() * (q.w < 0.0f ? -1.0f : +1.0f);
    }

    friend struct SMeshQTangents;
};

struct SPipNormal
    : public Vec3
{
    SPipNormal() {}

    explicit SPipNormal(const Vec3& othern)
    {
        x = othern.x;
        y = othern.y;
        z = othern.z;
    }

    // get normal vector
    ILINE Vec3 GetN() const
    {
        return *this;
    }

    // get normal vector
    void GetN(Vec3& othern) const
    {
        othern = GetN();
    }

    void TransformBy(const Matrix34& trn)
    {
        *this = SPipNormal(trn.TransformVector(*this));
    }

    void TransformSafelyBy(const Matrix34& trn)
    {
        // normalize in case "trn" wasn't length-preserving
        *this = SPipNormal(trn.TransformVector(*this).normalize());
    }

    friend struct SMeshNormal;
};

//==================================================================================================

typedef SVF_P3F_C4B_T2F SAuxVertex;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex Sizes
//extern const int m_VertexSize[];

//============================================================================
// Custom vertex streams definitions
// NOTE: If you add new stream ID also include vertex declarations creating in
//       CD3D9Renderer::EF_InitD3DVertexDeclarations (D3DRendPipeline.cpp)

// Stream IDs
enum EStreamIDs
{
    VSF_GENERAL,                 // General vertex buffer
    VSF_TANGENTS,                // Tangents buffer
    VSF_QTANGENTS,               // Tangents buffer
    VSF_HWSKIN_INFO,             // HW skinning buffer
    VSF_VERTEX_VELOCITY,                // Velocity buffer
# if ENABLE_NORMALSTREAM_SUPPORT
    VSF_NORMALS,                 // Normals, used for skinning
#endif
    // <- Insert new stream IDs here
    VSF_NUM,                     // Number of vertex streams

    VSF_MORPHBUDDY = 8,          // Morphing (from m_pMorphBuddy)
    VSF_INSTANCED = 9,           // Data is for instance stream
    VSF_MORPHBUDDY_WEIGHTS = 15, // Morphing weights
};

// Stream Masks (Used during updating)
enum EStreamMasks
{
    VSM_GENERAL    = 1 << VSF_GENERAL,
    VSM_TANGENTS   = ((1 << VSF_TANGENTS) | (1 << VSF_QTANGENTS)),
    VSM_HWSKIN     = 1 << VSF_HWSKIN_INFO,
    VSM_VERTEX_VELOCITY   = 1 << VSF_VERTEX_VELOCITY,
# if ENABLE_NORMALSTREAM_SUPPORT
    VSM_NORMALS    = 1 << VSF_NORMALS,
#endif

    VSM_MORPHBUDDY = 1 << VSF_MORPHBUDDY,
    VSM_INSTANCED  = 1 << VSF_INSTANCED,

    VSM_MASK       = ((1 << VSF_NUM) - 1),
};

//==================================================================================================================

#endif // CRYINCLUDE_CRYCOMMON_VERTEXFORMATS_H

