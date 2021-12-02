//--------------------------------------------------------------------------------------
// File: Color.h
//
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
//--------------------------------------------------------------------------------------

#pragma once


#include <cmath>

class Color
{
public:
    union
    {
        struct
        {
            float m[4];
        };  // r, g, b, a
        struct
        {
            float r, g, b, a;
        };
    };

    Color() :
        r(0), g(0), b(0), a(1)
    {}

    Color(float r, float g, float b, float a = 1.f) :
        m{ r, g, b, a }
    {}

    Color(const Color& other) : 
        m{ other.r, other.g, other.b, other.a }
    {}

    ~Color(){}

    const float& operator[](unsigned int i) const { return m[i]; }
    float& operator[](unsigned int i) { return m[i]; }

    Color& operator=(const Color& other)
    {
        r = other.r; g = other.g;
        b = other.b; a = other.a;
        return *this;
    }
};