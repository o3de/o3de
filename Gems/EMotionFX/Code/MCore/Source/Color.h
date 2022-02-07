/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "StandardHeaders.h"
#include "Algorithms.h"
#include "FastMath.h"
#include <AzCore/Math/Color.h>


namespace MCore
{
    // color component extraction
    MCORE_INLINE uint8 ExtractRed  (uint32 col)     { return ((col >> 16) & 0xff); }
    MCORE_INLINE uint8 ExtractGreen(uint32 col)     { return ((col >> 8)  & 0xff); }
    MCORE_INLINE uint8 ExtractBlue (uint32 col)     { return (col & 0xff); }
    MCORE_INLINE uint8 ExtractAlpha(uint32 col)     { return (col >> 24); }

    /**
     * Construct a 32bit value (unsigned long) from four bytes (0..255) for each color component.
     * This actually stores the value as ABGR inside the integer, so like: 0xAABBGGRR.
     * @param r The value of the red component.
     * @param g The value of the green component.
     * @param b The value of the blue component.
     * @param a the value of the alpha component.
     * @result A packed 32-bit integer, containing the four color components.
     */
    MCORE_INLINE uint32 RGBA(uint8 r, uint8 g, uint8 b, uint8 a = 255)            { return ((uint32)((((a) & 0xff) << 24) | (((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))); }

    /**
     * The high precision color class.
     * This template should be used with floats or doubles.
     * Default predefined types are: RGBAColor, FRGBAColor, DRGBAColor.
     * The component values go from 0 to 1, where 0 is black and 1 is white in case all components had this same value.
     * It however is possible to go higher than 1. Color component values can also be clamped, normalized and affected by exposure control.
     */
    class MCORE_API RGBAColor
    {
    public:
        /**
         * Default constructor. Color will be set to black (0,0,0,0).
         */
        MCORE_INLINE RGBAColor()
            : m_r(0.0f)
            , m_g(0.0f)
            , m_b(0.0f)
            , m_a(1.0f) {}

        /**
         * Constructor which sets all components to the same given value.
         * @param value The value to put in all components (r,g,b,a).
         */
        MCORE_INLINE RGBAColor(float value)
            : m_r(value)
            , m_g(value)
            , m_b(value)
            , m_a(value) {}

        /**
         * Constructor which sets each color component.
         * @param cR The value for red.
         * @param cG The value for green.
         * @param cB The value for blue.
         * @param cA The value for alpha [default=1.0]
         */
        MCORE_INLINE RGBAColor(float cR, float cG, float cB, float cA = 1.0f)
            : m_r(cR)
            , m_g(cG)
            , m_b(cB)
            , m_a(cA) {}

        /**
         * Copy constructor.
         * @param col The color to copy the component values from.
         */
        MCORE_INLINE RGBAColor(const RGBAColor& col)
            : m_r(col.m_r)
            , m_g(col.m_g)
            , m_b(col.m_b)
            , m_a(col.m_a) {}

        /**
         * Constructor to convert a 32-bits DWORD to a high precision color.
         * @param col The 32-bits DWORD, for example constructed using the MCore::RGBA(...) function.
         */
        RGBAColor(uint32 col)
            : m_r(ExtractRed(col) / 255.0f)
            , m_g(ExtractGreen(col) / 255.0f)
            , m_b(ExtractBlue(col) / 255.0f)
            , m_a(ExtractAlpha(col) / 255.0f) {}

        /**
        * Constructor to convert from AZ::Color. This constructor is convenient until we replace the usage of this class with AZ::Color
        * @param color The AZ::Color to construct from
        */
        RGBAColor(const AZ::Color& color)
            : m_r(color.GetR())
            , m_g(color.GetG())
            , m_b(color.GetB())
            , m_a(color.GetA())
        {}

        /**
        * Automatic conversion to AZ::Color until we remove the u sage of this class with AZ::Color
        */
        operator AZ::Color() const
        {
            return AZ::Color(m_r, m_g, m_b, m_a);
        }

        /**
         * Set the color component values.
         * @param cR The value for red.
         * @param cG The value for green.
         * @param cB The value for blue.
         * @param cA The value for alpha.
         */
        MCORE_INLINE void Set(float cR, float cG, float cB, float cA)                           { m_r = cR; m_g = cG; m_b = cB; m_a = cA; }

        /**
         * Set the color component values.
         * @param color The color to set.
         */
        MCORE_INLINE void Set(const RGBAColor& color)                                           { m_r = color.m_r; m_g = color.m_g; m_b = color.m_b; m_a = color.m_a; }

        /**
         * Clear the color component values. Set them all to zero, so the color turns into black.
         */
        MCORE_INLINE void Zero()                                                                { m_r = m_g = m_b = m_a = 0.0f; }

        /**
         * Clamp all color component values in a range of 0..1
         * This can screw up your colors of course. Unless you just want to clamp we advise you to use
         * the Exposure method for exposure control or the Normalize method.
         * @result The clamped color.
         */
        MCORE_INLINE RGBAColor& Clamp()                                                         { m_r = MCore::Clamp<float>(m_r, 0.0f, 1.0f); m_g = MCore::Clamp<float>(m_g, 0.0f, 1.0f); m_b = MCore::Clamp<float>(m_b, 0.0f, 1.0f); m_a = MCore::Clamp<float>(m_a, 0.0f, 1.0f); return *this; }

        /**
         * Returns the length of the color components (r, g, b), just like you calculate the length of a vector.
         * The higher the length value, the more bright the color will be.
         * @result The length of the vector constructed by the r, g and b components.
         */
        MCORE_INLINE float CalcLength() const                                                   { return Math::Sqrt(m_r * m_r + m_g * m_g + m_b * m_b); }

        /**
         * Calculates and returns the intensity of the color. This gives an idea of how bright the color would be on the screen.
         * @result The intensity.
         */
        MCORE_INLINE float CalcIntensity() const                                                { return m_r * 0.212671f + m_g * 0.715160f + m_b * 0.072169f; }

        /**
         * Checks if this color is close to another given color.
         * @param col The other color to compare with.
         * @param distSq The square distance of maximum tollerance of the difference. A value of 0 would mean the colors have to be exactly the same.
         * @result Returns true when the colors are the same (or close enough to eachother). Otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfIsClose(const RGBAColor& col, float distSq = 0.0001f) const
        {
            float cR = (m_r - col.m_r);
            cR *= cR;
            if (cR > distSq)
            {
                return false;
            }
            float cG = (m_g - col.m_g);
            cR += cG * cG;
            if (cR > distSq)
            {
                return false;
            }
            float cB = (m_b - col.m_b);
            cR += cB * cB;
            if (cR > distSq)
            {
                return false;
            }
            float cA = (m_a - col.m_a);
            cR += cA * cA;
            return (cR < distSq);
        }

        /**
         * Convert this high precision color to a 32-bit DWORD value.
         * In order to work correctly, the color component values must be in range of 0..1. So they have to be clamped, normalized or exposure controlled before calling this method.
         * @result The 32-bit integer value where each byte is a color component.
         */
        MCORE_INLINE uint32 ToInt() const                                                       { return MCore::RGBA((uint8)(m_r * 255), (uint8)(m_g * 255), (uint8)(m_b * 255), (uint8)(m_a * 255)); }

        /**
         * Perform exposure control on the color components.
         * This will give much better results than just clamping the values between 0 and 1.
         * @param exposure The exposure value [default=1.5]
         * @result Returns itself, but now with exposure control performed on it.
         */
        MCORE_INLINE RGBAColor& Exposure(float exposure = 1.5f)
        {
            m_r = 1.0f - Math::Exp(-m_r * exposure);
            m_g = 1.0f - Math::Exp(-m_g * exposure);
            m_b = 1.0f - Math::Exp(-m_b * exposure);
            return *this;
        }

        /**
         * Smart normalizes the color. This means it will scale the values in a range of 0..1 if they are out of range.
         * It picks the value of the component with the highest value. And makes this into a value of 1, while scaling
         * the others down with the correct amounts. So it's not really normalizing like you normalize a vector, but it
         * is a little bit more intelligent and will only perform it when needed, so it won't touch the colors if
         * they are already in range of 0..1.
         * @result The normalized color.
         */
        MCORE_INLINE RGBAColor& Normalize()
        {
            float maxVal = 1.0f;

            if (m_r > maxVal)
            {
                maxVal = m_r;
            }
            if (m_g > maxVal)
            {
                maxVal = m_g;
            }
            if (m_b > maxVal)
            {
                maxVal = m_b;
            }

            float mul = 1.0f / maxVal;

            m_r *= mul;
            m_g *= mul;
            m_b *= mul;

            return *this;
        }

        // operators
        MCORE_INLINE bool               operator==(const RGBAColor& col) const              { return ((m_r == col.m_r) && (m_g == col.m_g) && (m_b == col.m_b) && (m_a == col.m_a)); }
        MCORE_INLINE const RGBAColor&   operator*=(const RGBAColor& col)                    { m_r *= col.m_r; m_g *= col.m_g; m_b *= col.m_b; m_a *= col.m_a; return *this; }
        MCORE_INLINE const RGBAColor&   operator+=(const RGBAColor& col)                    { m_r += col.m_r; m_g += col.m_g; m_b += col.m_b; m_a += col.m_a; return *this; }
        MCORE_INLINE const RGBAColor&   operator-=(const RGBAColor& col)                    { m_r -= col.m_r; m_g -= col.m_g; m_b -= col.m_b; m_a -= col.m_a; return *this; }
        MCORE_INLINE const RGBAColor&   operator*=(float m)                                 { m_r *= m; m_g *= m; m_b *= m; m_a *= m; return *this; }
        //MCORE_INLINE const RGBAColor& operator*=(double m)                                { r*=m; g*=m; b*=m; a*=m; return *this; }
        MCORE_INLINE const RGBAColor&   operator/=(float d)                                 { float ooD = 1.0f / d; m_r *= ooD; m_g *= ooD; m_b *= ooD; m_a *= ooD; return *this; }
        //MCORE_INLINE const RGBAColor& operator/=(double d)                                { float ooD=1.0f/d; r*=ooD; g*=ooD; b*=ooD; a*=ooD; return *this; }
        MCORE_INLINE const RGBAColor&   operator= (const RGBAColor& col)                    { m_r = col.m_r; m_g = col.m_g; m_b = col.m_b; m_a = col.m_a; return *this; }
        MCORE_INLINE const RGBAColor&   operator= (float colorValue)                        { m_r = colorValue; m_g = colorValue; m_b = colorValue; m_a = colorValue; return *this; }

        MCORE_INLINE float&             operator[](int32 row)                               { return ((float*)&m_r)[row]; }
        MCORE_INLINE operator           float*()                                            { return (float*)&m_r; }
        MCORE_INLINE operator           const float*() const                                { return (const float*)&m_r; }

        static uint32 s_colorTable[128];

        // attributes
        float   m_r;  /**< Red component. */
        float   m_g;  /**< Green component. */
        float   m_b;  /**< Blue component. */
        float   m_a;  /**< Alpha component. */
    };

    /**
     * Picks a random color from a table of 128 different colors.
     * @result The generated color.
     */
    MCORE_INLINE uint32 GenerateColor()                                                         { return RGBAColor::s_colorTable[rand() % 128]; }

    // operators
    MCORE_INLINE RGBAColor operator*(const RGBAColor& colA, const RGBAColor& colB)              { return RGBAColor(colA.m_r * colB.m_r, colA.m_g * colB.m_g, colA.m_b * colB.m_b, colA.m_a * colB.m_a); }
    MCORE_INLINE RGBAColor operator+(const RGBAColor& colA, const RGBAColor& colB)              { return RGBAColor(colA.m_r + colB.m_r, colA.m_g + colB.m_g, colA.m_b + colB.m_b, colA.m_a + colB.m_a); }
    MCORE_INLINE RGBAColor operator-(const RGBAColor& colA, const RGBAColor& colB)              { return RGBAColor(colA.m_r - colB.m_r, colA.m_g - colB.m_g, colA.m_b - colB.m_b, colA.m_b - colB.m_b); }
    MCORE_INLINE RGBAColor operator*(const RGBAColor& colA, float m)                            { return RGBAColor(colA.m_r * m, colA.m_g * m, colA.m_b * m, colA.m_a * m); }
    //MCORE_INLINE RGBAColor operator*(const RGBAColor& colA,   double m)                           { return RGBAColor(colA.r*m, colA.g*m, colA.b*m, colA.a*m); }
    MCORE_INLINE RGBAColor operator*(float m,               const RGBAColor& colB)              { return RGBAColor(m * colB.m_r, m * colB.m_g, m * colB.m_b, m * colB.m_a); }
    //MCORE_INLINE RGBAColor operator*(double m,                const RGBAColor& colB)              { return RGBAColor(m*colB.r, m*colB.g, m*colB.b, m*colB.a); }
    MCORE_INLINE RGBAColor operator/(const RGBAColor& colA, float d)                            { float ooD = 1.0f / d; return RGBAColor(colA.m_r * ooD, colA.m_g * ooD, colA.m_b * ooD, colA.m_a * ooD); }
    //MCORE_INLINE RGBAColor operator/(const RGBAColor& colA,   double d)                           { float ooD=1.0f/d; return RGBAColor(colA.r*ooD, colA.g*ooD, colA.b*ooD, colA.a*ooD); }
}   // namespace MCore
