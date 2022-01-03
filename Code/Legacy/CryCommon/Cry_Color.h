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
