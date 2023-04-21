/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : 4D Color template.
#pragma once

#include <platform.h>
#include <AzCore/std/containers/array.h>
#include "Cry_Math.h"

template <class T>
struct Color_tpl;

typedef Color_tpl<uint8> ColorB; // [ 0,  255]
typedef Color_tpl<float> ColorF; // [0.0, 1.0]

//////////////////////////////////////////////////////////////////////////////////////////////
// RGBA Color structure.
//////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
struct Color_tpl
{
    T   r, g, b, a;

    ILINE Color_tpl() {};

    ILINE Color_tpl(T _x, T _y, T _z, T _w);
    ILINE Color_tpl(T _x, T _y, T _z);

    /*  inline Color_tpl(const Color_tpl<T> & v)    {
    r = v.r; g = v.g; b = v.b; a = v.a;
    }*/

    // works together with pack_abgr8888
    ILINE Color_tpl(const unsigned int abgr);
    ILINE Color_tpl(const f32 c);
    ILINE Color_tpl(const ColorF& c);
    ILINE Color_tpl(const ColorF& c, float fAlpha);
    ILINE Color_tpl(const Vec3& c, float fAlpha);
    ILINE Color_tpl(const Vec4& c);

    ILINE Color_tpl(const Vec3& vVec)
    {
        r = (T)vVec.x;
        g = (T)vVec.y;
        b = (T)vVec.z;
        a = (T)1.f;
    }

    ILINE Color_tpl& operator = (const Vec3& v) { r = (T)v.x; g = (T)v.y; b = (T)v.z; a = (T)1.0f; return *this; }
    ILINE Color_tpl& operator = (const Color_tpl& c) { r = (T)c.r; g = (T)c.g; b = (T)c.b; a = (T)c.a; return *this; }

    ILINE T& operator [] (int index)          { assert(index >= 0 && index <= 3); return ((T*)this)[index]; }
    ILINE T operator [] (int index) const { assert(index >= 0 && index <= 3); return ((T*)this)[index]; }

    ILINE void Set(float fR, float fG, float fB, float fA = 1.0f)
    {
        r = static_cast<T>(fR);
        g = static_cast<T>(fG);
        b = static_cast<T>(fB);
        a = static_cast<T>(fA);
    }

    ILINE Color_tpl operator + () const
    {
        return *this;
    }
    ILINE Color_tpl operator - () const
    {
        return Color_tpl<T>(-r, -g, -b, -a);
    }

    ILINE Color_tpl& operator += (const Color_tpl& v)
    {
        T _r = r,       _g = g,         _b = b,         _a = a;
        _r += v.r;
        _g += v.g;
        _b += v.b;
        _a += v.a;
        r = _r;
        g = _g;
        b = _b;
        a = _a;
        return *this;
    }
    ILINE Color_tpl& operator -= (const Color_tpl& v)
    {
        r -= v.r;
        g -= v.g;
        b -= v.b;
        a -= v.a;
        return *this;
    }
    ILINE Color_tpl& operator *= (const Color_tpl& v)
    {
        r *= v.r;
        g *= v.g;
        b *= v.b;
        a *= v.a;
        return *this;
    }
    ILINE Color_tpl& operator /= (const Color_tpl& v)
    {
        r /= v.r;
        g /= v.g;
        b /= v.b;
        a /= v.a;
        return *this;
    }
    ILINE Color_tpl& operator *= (T s)
    {
        r *= s;
        g *= s;
        b *= s;
        a *= s;
        return *this;
    }
    ILINE Color_tpl& operator /= (T s)
    {
        s = 1.0f / s;
        r *= s;
        g *= s;
        b *= s;
        a *= s;
        return *this;
    }

    ILINE Color_tpl operator + (const Color_tpl& v) const
    {
        return Color_tpl(r + v.r, g + v.g, b + v.b, a + v.a);
    }
    ILINE Color_tpl operator - (const Color_tpl& v) const
    {
        return Color_tpl(r - v.r, g - v.g, b - v.b, a - v.a);
    }
    ILINE Color_tpl operator * (const Color_tpl& v) const
    {
        return Color_tpl(r * v.r, g * v.g, b * v.b, a * v.a);
    }
    ILINE Color_tpl operator / (const Color_tpl& v) const
    {
        return Color_tpl(r / v.r, g / v.g, b / v.b, a / v.a);
    }
    ILINE Color_tpl operator * (T s) const
    {
        return Color_tpl(r * s, g * s, b * s, a * s);
    }
    ILINE Color_tpl operator / (T s) const
    {
        s = 1.0f / s;
        return Color_tpl(r * s, g * s, b * s, a * s);
    }

    ILINE bool operator == (const Color_tpl& v) const
    {
        return (r == v.r) && (g == v.g) && (b == v.b) && (a == v.a);
    }
    ILINE bool operator != (const Color_tpl& v) const
    {
        return (r != v.r) || (g != v.g) || (b != v.b) || (a != v.a);
    }

    ILINE unsigned int pack_abgr8888() const;
    ILINE unsigned int pack_argb8888() const;
    inline Vec3 toVec3() const { return Vec3(r, g, b); }

    inline void clamp(T bottom = 0.0f, T top = 1.0f);

    void srgb2rgb()
    {
        for (int i = 0; i < 3; i++)
        {
            T& c = (*this)[i];

            if (c <= 0.040448643f)
            {
                c = c / 12.92f;
            }
            else
            {
                c = pow((c + 0.055f) / 1.055f, 2.4f);
            }
        }
    }

    AZStd::array<T, 4> GetAsArray() const
    {
        AZStd::array<T, 4> primitiveArray = { { r, g, b, a } };
        return primitiveArray;
    }
};


//////////////////////////////////////////////////////////////////////////////////////////////
// template specialization
///////////////////////////////////////////////

template<>
ILINE Color_tpl<f32>::Color_tpl(f32 _x, f32 _y, f32 _z, f32 _w)
{
    r = _x;
    g = _y;
    b = _z;
    a = _w;
}

template<>
ILINE Color_tpl<f32>::Color_tpl(f32 _x, f32 _y, f32 _z)
{
    r = _x;
    g = _y;
    b = _z;
    a = 1.f;
}

template<>
ILINE Color_tpl<uint8>::Color_tpl(uint8 _x, uint8 _y, uint8 _z, uint8 _w)
{
    r = _x;
    g = _y;
    b = _z;
    a = _w;
}

template<>
ILINE Color_tpl<uint8>::Color_tpl(uint8 _x, uint8 _y, uint8 _z)
{
    r = _x;
    g = _y;
    b = _z;
    a = 255;
}

//-----------------------------------------------------------------------------

template<>
ILINE Color_tpl<f32>::Color_tpl(const unsigned int abgr)
{
    r = (abgr & 0xff) / 255.0f;
    g = ((abgr >> 8) & 0xff) / 255.0f;
    b = ((abgr >> 16) & 0xff) / 255.0f;
    a = ((abgr >> 24) & 0xff) / 255.0f;
}

template<>
ILINE Color_tpl<uint8>::Color_tpl(const unsigned int c)
{
    *(unsigned int*)(&r) = c;
} //use this with RGBA8 macro!

//-----------------------------------------------------------------------------

template<>
ILINE Color_tpl<f32>::Color_tpl(const float c)
{
    r = c;
    g = c;
    b = c;
    a = c;
}
template<>
ILINE Color_tpl<uint8>::Color_tpl(const float c)
{
    r = (uint8)(c * 255);
    g = (uint8)(c * 255);
    b = (uint8)(c * 255);
    a = (uint8)(c * 255);
}
//-----------------------------------------------------------------------------

template<>
ILINE Color_tpl<f32>::Color_tpl(const ColorF& c)
{
    r = c.r;
    g = c.g;
    b = c.b;
    a = c.a;
}
template<>
ILINE Color_tpl<uint8>::Color_tpl(const ColorF& c)
{
    r = (uint8)(c.r * 255);
    g = (uint8)(c.g * 255);
    b = (uint8)(c.b * 255);
    a = (uint8)(c.a * 255);
}

template<>
ILINE Color_tpl<f32>::Color_tpl(const ColorF& c, float fAlpha)
{
    r = c.r;
    g = c.g;
    b = c.b;
    a = fAlpha;
}

template<>
ILINE Color_tpl<f32>::Color_tpl(const Vec3& c, float fAlpha)
{
    r = c.x;
    g = c.y;
    b = c.z;
    a = fAlpha;
}

template<>
ILINE Color_tpl<f32>::Color_tpl(const Vec4& c)
{
    r = c.x;
    g = c.y;
    b = c.z;
    a = c.w;
}

template<>
ILINE Color_tpl<uint8>::Color_tpl(const ColorF& c, float fAlpha)
{
    r = (uint8)(c.r * 255);
    g = (uint8)(c.g * 255);
    b = (uint8)(c.b * 255);
    a = (uint8)(fAlpha * 255);
}
template<>
ILINE Color_tpl<uint8>::Color_tpl(const Vec3& c, float fAlpha)
{
    r = (uint8)(c.x * 255);
    g = (uint8)(c.y * 255);
    b = (uint8)(c.z * 255);
    a = (uint8)(fAlpha * 255);
}

template<>
ILINE Color_tpl<uint8>::Color_tpl(const Vec4& c)
{
    r = (uint8)(c.x * 255);
    g = (uint8)(c.y * 255);
    b = (uint8)(c.z * 255);
    a = (uint8)(c.w * 255);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// functions implementation
///////////////////////////////////////////////

///////////////////////////////////////////////
///////////////////////////////////////////////
template <class T>
ILINE Color_tpl<T> operator * (T s, const Color_tpl<T>& v)
{
    return Color_tpl<T>(v.r * s, v.g * s, v.b * s, v.a * s);
}

///////////////////////////////////////////////
template <class T>
ILINE unsigned int Color_tpl<T>::pack_abgr8888() const
{
    unsigned char cr;
    unsigned char cg;
    unsigned char cb;
    unsigned char ca;
    if constexpr (sizeof(r) == 1) // char and unsigned char
    {
        cr = (unsigned char)r;
        cg = (unsigned char)g;
        cb = (unsigned char)b;
        ca = (unsigned char)a;
    }
    else if constexpr (sizeof(r) == 2) // short and unsigned short
    {
        cr = (unsigned short)(r) >> 8;
        cg = (unsigned short)(g) >> 8;
        cb = (unsigned short)(b) >> 8;
        ca = (unsigned short)(a) >> 8;
    }
    else // float or double
    {
        cr = (unsigned char)(r * 255.0f);
        cg = (unsigned char)(g * 255.0f);
        cb = (unsigned char)(b * 255.0f);
        ca = (unsigned char)(a * 255.0f);
    }
    return (ca << 24) | (cb << 16) | (cg << 8) | cr;
}


///////////////////////////////////////////////
template <class T>
ILINE unsigned int Color_tpl<T>::pack_argb8888() const
{
    unsigned char cr;
    unsigned char cg;
    unsigned char cb;
    unsigned char ca;
    if constexpr (sizeof(r) == 1) // char and unsigned char
    {
        cr = (unsigned char)r;
        cg = (unsigned char)g;
        cb = (unsigned char)b;
        ca = (unsigned char)a;
    }
    else if constexpr (sizeof(r) == 2) // short and unsigned short
    {
        cr = (unsigned short)(r) >> 8;
        cg = (unsigned short)(g) >> 8;
        cb = (unsigned short)(b) >> 8;
        ca = (unsigned short)(a) >> 8;
    }
    else // float or double
    {
        cr = (unsigned char)(r * 255.0f);
        cg = (unsigned char)(g * 255.0f);
        cb = (unsigned char)(b * 255.0f);
        ca = (unsigned char)(a * 255.0f);
    }
    return (ca << 24) | (cr << 16) | (cg << 8) | cb;
}

///////////////////////////////////////////////
template <class T>
inline void Color_tpl<T>::clamp(T bottom, T top)
{
    r = min(top, max(bottom, r));
    g = min(top, max(bottom, g));
    b = min(top, max(bottom, b));
    a = min(top, max(bottom, a));
}

#define Col_TrackviewDefault    ColorF (0.187820792f, 0.187820792f, 1.0f)
#define Clr_Empty                               ColorF(0.0f, 0.0f, 0.0f, 1.0f)

// Gruer patch begin aoreshko: MADPORT-12
//////////////////////////////////////////////////////////////////////////////////////////////
//#define RGBA8(r,g,b,a)   (ColorB((uint8)(r),(uint8)(g),(uint8)(b),(uint8)(a)))
#if defined(NEED_ENDIAN_SWAP)
#define RGBA8(a, b, g, r)                                                                                                                  \
    ((uint32)(((uint8)(r) | ((uint16)((uint8)(g)) << 8)) | (((uint32)(uint8)(b)) << 16)) | (((uint32)(uint8)(a)) << 24))
#else
#define RGBA8(r, g, b, a)                                                                                                                  \
    ((uint32)(((uint8)(r) | ((uint16)((uint8)(g)) << 8)) | (((uint32)(uint8)(b)) << 16)) | (((uint32)(uint8)(a)) << 24))
#endif
#define Col_Black ColorF(0.000f, 0.000f, 0.000f) // 0xFF000000   RGB: 0, 0, 0
#define Col_White ColorF(1.000f, 1.000f, 1.000f) // 0xFFFFFFFF   RGB: 255, 255, 255
#define Col_Aquamarine ColorF(0.498f, 1.000f, 0.831f) // 0xFF7FFFD4   RGB: 127, 255, 212
#define Col_Azure ColorF(0.000f, 0.498f, 1.000f) // 0xFF007FFF       RGB: 0, 127, 255
#define Col_Blue ColorF(0.000f, 0.000f, 1.000f) // 0xFF0000FF   RGB: 0, 0, 255
#define Col_BlueViolet ColorF(0.541f, 0.169f, 0.886f) // 0xFF8A2BE2   RGB: 138, 43, 226
#define Col_Brown ColorF(0.647f, 0.165f, 0.165f) // 0xFFA52A2A   RGB: 165, 42, 42
#define Col_CadetBlue ColorF(0.373f, 0.620f, 0.627f) // 0xFF5F9EA0   RGB: 95, 158, 160
#define Col_Coral ColorF(1.000f, 0.498f, 0.314f) // 0xFFFF7F50   RGB: 255, 127, 80
#define Col_CornflowerBlue ColorF(0.392f, 0.584f, 0.929f) // 0xFF6495ED   RGB: 100, 149, 237
#define Col_Cyan ColorF(0.000f, 1.000f, 1.000f) // 0xFF00FFFF   RGB: 0, 255, 255
#define Col_DarkGray ColorF(0.663f, 0.663f, 0.663f) // 0xFFA9A9A9   RGB: 169, 169, 169
#define Col_DarkGrey ColorF(0.663f, 0.663f, 0.663f) // 0xFFA9A9A9   RGB: 169, 169, 169
#define Col_DarkGreen ColorF(0.000f, 0.392f, 0.000f) // 0xFF006400   RGB: 0, 100, 0
#define Col_DarkOliveGreen ColorF(0.333f, 0.420f, 0.184f) // 0xFF556B2F   RGB: 85, 107, 47
#define Col_DarkOrchid ColorF(0.600f, 0.196f, 0.800f) // 0xFF9932CC   RGB: 153, 50, 204
#define Col_DarkSlateBlue ColorF(0.282f, 0.239f, 0.545f) // 0xFF483D8B   RGB: 72, 61, 139
#define Col_DarkSlateGray ColorF(0.184f, 0.310f, 0.310f) // 0xFF2F4F4F   RGB: 47, 79, 79
#define Col_DarkSlateGrey ColorF(0.184f, 0.310f, 0.310f) // 0xFF2F4F4F   RGB: 47, 79, 79
#define Col_DarkTurquoise ColorF(0.000f, 0.808f, 0.820f) // 0xFF00CED1       RGB: 0, 206, 209
#define Col_DarkWood ColorF(0.050f, 0.010f, 0.005f) // 0xFF0D0301       RGB: 13, 3, 1
#define Col_DeepPink ColorF(1.000f, 0.078f, 0.576f) // 0xFFFF1493       RGB: 255, 20, 147
#define Col_DimGray ColorF(0.412f, 0.412f, 0.412f) // 0xFF696969   RGB: 105, 105, 105
#define Col_DimGrey ColorF(0.412f, 0.412f, 0.412f) // 0xFF696969   RGB: 105, 105, 105
#define Col_FireBrick ColorF(0.698f, 0.133f, 0.133f) // 0xFFB22222   RGB: 178, 34, 34
#define Col_ForestGreen ColorF(0.133f, 0.545f, 0.133f) // 0xFF228B22   RGB: 34, 139, 34
#define Col_Gold ColorF(1.000f, 0.843f, 0.000f) // 0xFFFFD700   RGB: 255, 215, 0
#define Col_Goldenrod ColorF(0.855f, 0.647f, 0.125f) // 0xFFDAA520   RGB: 218, 165, 32
#define Col_Gray ColorF(0.502f, 0.502f, 0.502f) // 0xFF808080   RGB: 128, 128, 128
#define Col_Grey ColorF(0.502f, 0.502f, 0.502f) // 0xFF808080   RGB: 128, 128, 128
#define Col_Green ColorF(0.000f, 0.502f, 0.000f) // 0xFF008000   RGB: 0, 128, 0
#define Col_GreenYellow ColorF(0.678f, 1.000f, 0.184f) // 0xFFADFF2F   RGB: 173, 255, 47
#define Col_IndianRed ColorF(0.804f, 0.361f, 0.361f) // 0xFFCD5C5C   RGB: 205, 92, 92
#define Col_Khaki ColorF(0.941f, 0.902f, 0.549f) // 0xFFF0E68C   RGB: 240, 230, 140
#define Col_LightBlue ColorF(0.678f, 0.847f, 0.902f) // 0xFFADD8E6   RGB: 173, 216, 230
#define Col_LightGray ColorF(0.827f, 0.827f, 0.827f) // 0xFFD3D3D3   RGB: 211, 211, 211
#define Col_LightGrey ColorF(0.827f, 0.827f, 0.827f) // 0xFFD3D3D3   RGB: 211, 211, 211
#define Col_LightSteelBlue ColorF(0.690f, 0.769f, 0.871f) // 0xFFB0C4DE   RGB: 176, 196, 222
#define Col_LightWood ColorF(0.600f, 0.240f, 0.100f) // 0xFF993D1A       RGB: 153, 61, 26
#define Col_Lime ColorF(0.000f, 1.000f, 0.000f) // 0xFF00FF00   RGB: 0, 255, 0
#define Col_LimeGreen ColorF(0.196f, 0.804f, 0.196f) // 0xFF32CD32   RGB: 50, 205, 50
#define Col_Magenta ColorF(1.000f, 0.000f, 1.000f) // 0xFFFF00FF   RGB: 255, 0, 255
#define Col_Maroon ColorF(0.502f, 0.000f, 0.000f) // 0xFF800000   RGB: 128, 0, 0
#define Col_MedianWood ColorF(0.300f, 0.120f, 0.030f) // 0xFF4D1F09       RGB: 77, 31, 9
#define Col_MediumAquamarine ColorF(0.400f, 0.804f, 0.667f) // 0xFF66CDAA   RGB: 102, 205, 170
#define Col_MediumBlue ColorF(0.000f, 0.000f, 0.804f) // 0xFF0000CD   RGB: 0, 0, 205
#define Col_MediumForestGreen ColorF(0.420f, 0.557f, 0.137f) // 0xFF6B8E23       RGB: 107, 142, 35
#define Col_MediumGoldenrod ColorF(0.918f, 0.918f, 0.678f) // 0xFFEAEAAD       RGB: 234, 234, 173
#define Col_MediumOrchid ColorF(0.729f, 0.333f, 0.827f) // 0xFFBA55D3   RGB: 186, 85, 211
#define Col_MediumSeaGreen ColorF(0.235f, 0.702f, 0.443f) // 0xFF3CB371   RGB: 60, 179, 113
#define Col_MediumSlateBlue ColorF(0.482f, 0.408f, 0.933f) // 0xFF7B68EE   RGB: 123, 104, 238
#define Col_MediumSpringGreen ColorF(0.000f, 0.980f, 0.604f) // 0xFF00FA9A   RGB: 0, 250, 154
#define Col_MediumTurquoise ColorF(0.282f, 0.820f, 0.800f) // 0xFF48D1CC   RGB: 72, 209, 204
#define Col_MediumVioletRed ColorF(0.780f, 0.082f, 0.522f) // 0xFFC71585   RGB: 199, 21, 133
#define Col_MidnightBlue ColorF(0.098f, 0.098f, 0.439f) // 0xFF191970   RGB: 25, 25, 112
#define Col_Navy ColorF(0.000f, 0.000f, 0.502f) // 0xFF000080   RGB: 0, 0, 128
#define Col_NavyBlue ColorF(0.137f, 0.137f, 0.557f) // 0xFF23238E       RGB: 35, 35, 142
#define Col_Orange ColorF(1.000f, 0.647f, 0.000f) // 0xFFFFA500   RGB: 255, 165, 0
#define Col_OrangeRed ColorF(1.000f, 0.271f, 0.000f) // 0xFFFF4500   RGB: 255, 69, 0
#define Col_Orchid ColorF(0.855f, 0.439f, 0.839f) // 0xFFDA70D6   RGB: 218, 112, 214
#define Col_PaleGreen ColorF(0.596f, 0.984f, 0.596f) // 0xFF98FB98   RGB: 152, 251, 152
#define Col_Pink ColorF(1.000f, 0.753f, 0.796f) // 0xFFFFC0CB   RGB: 255, 192, 203
#define Col_Plum ColorF(0.867f, 0.627f, 0.867f) // 0xFFDDA0DD   RGB: 221, 160, 221
#define Col_Red ColorF(1.000f, 0.000f, 0.000f) // 0xFFFF0000   RGB: 255, 0, 0
#define Col_Salmon ColorF(0.980f, 0.502f, 0.447f) // 0xFFFA8072   RGB: 250, 128, 114
#define Col_SeaGreen ColorF(0.180f, 0.545f, 0.341f) // 0xFF2E8B57   RGB: 46, 139, 87
#define Col_Sienna ColorF(0.627f, 0.322f, 0.176f) // 0xFFA0522D   RGB: 160, 82, 45
#define Col_SkyBlue ColorF(0.529f, 0.808f, 0.922f) // 0xFF87CEEB   RGB: 135, 206, 235
#define Col_SlateBlue ColorF(0.416f, 0.353f, 0.804f) // 0xFF6A5ACD   RGB: 106, 90, 205
#define Col_SpringGreen ColorF(0.000f, 1.000f, 0.498f) // 0xFF00FF7F   RGB: 0, 255, 127
#define Col_SteelBlue ColorF(0.275f, 0.510f, 0.706f) // 0xFF4682B4   RGB: 70, 130, 180
#define Col_Tan ColorF(0.824f, 0.706f, 0.549f) // 0xFFD2B48C   RGB: 210, 180, 140
#define Col_Thistle ColorF(0.847f, 0.749f, 0.847f) // 0xFFD8BFD8   RGB: 216, 191, 216
#define Col_Transparent ColorF(0.0f, 0.0f, 0.0f, 0.0f) // 0x00000000       RGB: 0, 0, 0
#define Col_Turquoise ColorF(0.251f, 0.878f, 0.816f) // 0xFF40E0D0   RGB: 64, 224, 208
#define Col_Violet ColorF(0.933f, 0.510f, 0.933f) // 0xFFEE82EE   RGB: 238, 130, 238
#define Col_VioletRed ColorF(0.800f, 0.196f, 0.600f) // 0xFFCC3299       RGB: 204, 50, 153
#define Col_Wheat ColorF(0.961f, 0.871f, 0.702f) // 0xFFF5DEB3   RGB: 245, 222, 179
#define Col_Yellow ColorF(1.000f, 1.000f, 0.000f) // 0xFFFFFF00   RGB: 255, 255, 0
#define Col_YellowGreen ColorF(0.604f, 0.804f, 0.196f) // 0xFF9ACD32   RGB: 154, 205, 50

#define Clr_Empty ColorF(0.0f, 0.0f, 0.0f, 1.0f)
#define Clr_Dark ColorF(0.15f, 0.15f, 0.15f, 1.0f)
#define Clr_White ColorF(1.0f, 1.0f, 1.0f, 1.0f)
#define Clr_WhiteTrans ColorF(1.0f, 1.0f, 1.0f, 0.0f)
#define Clr_Full ColorF(1.0f, 1.0f, 1.0f, 1.0f)
#define Clr_Neutral ColorF(1.0f, 1.0f, 1.0f, 1.0f)
#define Clr_Transparent ColorF(0.0f, 0.0f, 0.0f, 0.0f)
#define Clr_FrontVector ColorF(0.0f, 0.0f, 0.5f, 1.0f)
#define Clr_Static ColorF(127.0f / 255.0f, 127.0f / 255.0f, 0.0f, 0.0f)
#define Clr_Median ColorF(0.5f, 0.5f, 0.5f, 0.0f)
#define Clr_MedianHalf ColorF(0.5f, 0.5f, 0.5f, 0.5f)
#define Clr_FarPlane ColorF(1.0f, 0.0f, 0.0f, 0.0f)
#define Clr_FarPlane_R ColorF(bReverseDepth ? 0.0f : 1.0f, 0.0f, 0.0f, 0.0f)
#define Clr_Unknown ColorF(0.0f, 0.0f, 0.0f, 0.0f)
#define Clr_Unused ColorF(0.0f, 0.0f, 0.0f, 0.0f)
#define Clr_Debug ColorF(1.0f, 0.0f, 0.0f, 1.0f)
// Gruer patch end aoreshko
