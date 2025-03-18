/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/AzCoreAPI.h>

namespace AZ
{
    //! A color class with 4 components, RGBA.
    class AZCORE_API Color
    {
    public:

        AZ_TYPE_INFO(Color, "{7894072A-9050-4F0F-901B-34B1A0D29417}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, components are uninitialized.
        Color() = default;
        Color(const Vector4& v)   { m_color = v; }

        explicit Color(const Vector2& source);

        explicit Color(const Vector3& source);

        //! Constructs vector with all components set to the same specified value.
        explicit Color(float rgba);

        Color(float r, float g, float b, float a);

        Color(u8 r, u8 g, u8 b, u8 a);

        //! Creates a vector with all components set to zero, more efficient than calling Color(0.0f).
        static Color CreateZero();

        //! Creates a vector with all components set to one.
        static Color CreateOne();

        //! Sets components from rgba.
        static Color CreateFromRgba(u8 r, u8 g, u8 b, u8 a);

        //! Sets components from an array of 4 floats, stored in xyzw order.
        static Color CreateFromFloat4(const float* values);

        //! Copies r,g,b components from a Vector3, sets w to 1.0.
        static Color CreateFromVector3(const Vector3& v);

        //! Copies r,g,b components from a Vector3, specify w separately.
        static Color CreateFromVector3AndFloat(const Vector3& v, float w);

        //! r,g,b,a to u32 => 0xAABBGGRR (COLREF format).
        static u32 CreateU32(u8 r, u8 g, u8 b, u8 a);

        //! Stores the vector to an array of 4 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToFloat4(float* values) const;

        u8 GetR8() const;
        u8 GetG8() const;
        u8 GetB8() const;
        u8 GetA8() const;

        void SetR8(u8 r);
        void SetG8(u8 g);
        void SetB8(u8 b);
        void SetA8(u8 a);

        float GetR() const;
        float GetG() const;
        float GetB() const;
        float GetA() const;

        void SetR(float r);
        void SetG(float g);
        void SetB(float b);
        void SetA(float a);

        float GetElement(int32_t index) const;
        void SetElement(int32_t index, float v);

        Vector3 GetAsVector3() const;

        Vector4 GetAsVector4() const;

        //! Sets all components to the same specified value.
        void Set(float x);

        void Set(float r, float g, float b, float a);

        //! Sets components from an array of 4 floats, stored in rgba order.
        void Set(const float values[4]);

        //! Sets r,g,b components from a Vector3, sets a to 1.0.
        void Set(const Vector3& v);

        //! Sets r,g,b components from a Vector3, specify a separately.
        void Set(const Vector3& v, float a);

        //! Sets the RGB values of this Color based on a passed in hue, saturation, and value. Alpha is unchanged.
        void SetFromHSVRadians(float hueRadians, float saturation, float value);

        //! Checks the color is equal to another within a floating point tolerance.
        bool IsClose(const Color& v, float tolerance = Constants::Tolerance) const;

        bool IsZero(float tolerance = Constants::FloatEpsilon) const;

        //! Checks whether all components are finite.
        bool IsFinite() const;

        bool operator==(const Color& rhs) const;
        bool operator!=(const Color& rhs) const;

        explicit operator Vector3() const;
        explicit operator Vector4() const;

        Color& operator=(const Vector3& rhs);

        //! Color to u32 => 0xAABBGGRR.
        u32 ToU32() const;

        //! Color to u32 => 0xAABBGGRR, RGB convert from Linear to Gamma corrected values.
        u32 ToU32LinearToGamma() const;

        //! Color from u32 => 0xAABBGGRR.
        void FromU32(u32 c);

        //! Color from u32 => 0xAABBGGRR, RGB convert from Gamma corrected to Linear values.
        void FromU32GammaToLinear(u32 c);

        //! Convert SRGB gamma space to linear space
        static float ConvertSrgbGammaToLinear(float x);

        //! Convert SRGB linear space to gamma space
        static float ConvertSrgbLinearToGamma(float x);

        //! Convert color from linear to gamma corrected space.
        Color LinearToGamma() const;

        //! Convert color from gamma corrected to linear space.
        Color GammaToLinear() const;

        //! Comparison functions, not implemented as operators since that would probably be a little dangerous. These
        //! functions return true only if all components pass the comparison test.
        //! @{
        bool IsLessThan(const Color& rhs) const;
        bool IsLessEqualThan(const Color& rhs) const;
        bool IsGreaterThan(const Color& rhs) const;
        bool IsGreaterEqualThan(const Color& rhs) const;
        //! @}

        //! Linear interpolation between this color and a destination.
        //! @return (*this)*(1-t) + dest*t
        Color Lerp(const Color& dest, float t) const;

        //! Dot product of two colors, uses all 4 components.
        float Dot(const Color& rhs) const;

        //! Dot product of two colors, using only the r,g,b components.
        float Dot3(const Color& rhs) const;

        Color operator-() const;
        Color operator+(const Color& rhs) const;
        Color operator-(const Color& rhs) const;
        Color operator*(const Color& rhs) const;
        Color operator/(const Color& rhs) const;
        Color operator*(float multiplier) const;
        Color operator/(float divisor) const;

        Color& operator+=(const Color& rhs);
        Color& operator-=(const Color& rhs);
        Color& operator*=(const Color& rhs);
        Color& operator/=(const Color& rhs);
        Color& operator*=(float multiplier);
        Color& operator/=(float divisor);

    private:

        Vector4 m_color;

    };

    // Named colors, from CSS specification: https://www.w3.org/TR/2011/REC-SVG11-20110816/types.html#ColorKeywords
    namespace Colors
    {
        // Basic Colors (CSS 1 standard)
        const Color White                     { 1.000f, 1.000f, 1.000f, 1.000f }; // RGB: (255, 255, 255)
        const Color Silver                    { 0.753f, 0.753f, 0.753f, 1.000f }; // RGB: (192, 192, 192)
        const Color Gray                      { 0.500f, 0.500f, 0.500f, 1.000f }; // RGB: (128, 128, 128)
        const Color Black                     { 0.000f, 0.000f, 0.000f, 1.000f }; // RGB: (0, 0, 0)
        const Color Red                       { 1.000f, 0.000f, 0.000f, 1.000f }; // RGB: (255, 0, 0)
        const Color Maroon                    { 0.500f, 0.000f, 0.000f, 1.000f }; // RGB: (128, 0, 0)
        const Color Lime                      { 0.000f, 1.000f, 0.000f, 1.000f }; // RGB: (0, 255, 0)
        const Color Green                     { 0.000f, 0.500f, 0.000f, 1.000f }; // RGB: (0, 128, 0)
        const Color Blue                      { 0.000f, 0.000f, 1.000f, 1.000f }; // RGB: (0, 0, 255)
        const Color Navy                      { 0.000f, 0.000f, 0.500f, 1.000f }; // RGB: (0, 0, 128)
        const Color Yellow                    { 1.000f, 1.000f, 0.000f, 1.000f }; // RGB: (255, 255, 0)
        const Color Orange                    { 1.000f, 0.647f, 0.000f, 1.000f }; // RGB: (255, 165, 0)
        const Color Olive                     { 0.500f, 0.500f, 0.000f, 1.000f }; // RGB: (128, 128, 0)
        const Color Purple                    { 0.500f, 0.000f, 0.500f, 1.000f }; // RGB: (128, 0, 128)
        const Color Fuchsia                   { 1.000f, 0.000f, 1.000f, 1.000f }; // RGB: (255, 0, 255)
        const Color Teal                      { 0.000f, 0.500f, 0.500f, 1.000f }; // RGB: (0, 128, 128)
        const Color Aqua                      { 0.000f, 1.000f, 1.000f, 1.000f }; // RGB: (0, 255, 255)
        // CSS3 colors
        // Reds
        const Color IndianRed                 { 0.804f, 0.361f, 0.361f, 1.000f }; // RGB: (205, 92, 92)
        const Color LightCoral                { 0.941f, 0.502f, 0.502f, 1.000f }; // RGB: (240, 128, 128)
        const Color Salmon                    { 0.980f, 0.502f, 0.447f, 1.000f }; // RGB: (250, 128, 114)
        const Color DarkSalmon                { 0.914f, 0.588f, 0.478f, 1.000f }; // RGB: (233, 150, 122)
        const Color LightSalmon               { 1.000f, 0.627f, 0.478f, 1.000f }; // RGB: (255, 160, 122)
        const Color Crimson                   { 0.863f, 0.078f, 0.235f, 1.000f }; // RGB: (220, 20, 60)
        const Color FireBrick                 { 0.698f, 0.133f, 0.133f, 1.000f }; // RGB: (178, 34, 34)
        const Color DarkRed                   { 0.545f, 0.000f, 0.000f, 1.000f }; // RGB: (139, 0, 0)
        // Pinks                                                              
        const Color Pink                      { 1.000f, 0.753f, 0.796f, 1.000f }; // RGB: (255, 192, 203)
        const Color LightPink                 { 1.000f, 0.714f, 0.757f, 1.000f }; // RGB: (255, 182, 193)
        const Color HotPink                   { 1.000f, 0.412f, 0.706f, 1.000f }; // RGB: (255, 105, 180)
        const Color DeepPink                  { 1.000f, 0.078f, 0.576f, 1.000f }; // RGB: (255, 20, 147)
        const Color MediumVioletRed           { 0.780f, 0.082f, 0.522f, 1.000f }; // RGB: (199, 21, 133)
        const Color PaleVioletRed             { 0.859f, 0.439f, 0.576f, 1.000f }; // RGB: (219, 112, 147)
        // Oranges
        const Color Coral                     { 1.000f, 0.498f, 0.314f, 1.000f }; // RGB: (255, 127, 80)
        const Color Tomato                    { 1.000f, 0.388f, 0.278f, 1.000f }; // RGB: (255, 99, 71)
        const Color OrangeRed                 { 1.000f, 0.271f, 0.000f, 1.000f }; // RGB: (255, 69, 0)
        const Color DarkOrange                { 1.000f, 0.549f, 0.000f, 1.000f }; // RGB: (255, 140, 0)
        // Yellows
        const Color Gold                      { 1.000f, 0.843f, 0.000f, 1.000f }; // RGB: (255, 215, 0)
        const Color LightYellow               { 1.000f, 1.000f, 0.878f, 1.000f }; // RGB: (255, 255, 224)
        const Color LemonChiffon              { 1.000f, 0.980f, 0.804f, 1.000f }; // RGB: (255, 250, 205)
        const Color LightGoldenrodYellow      { 0.980f, 0.980f, 0.824f, 1.000f }; // RGB: (250, 250, 210)
        const Color PapayaWhip                { 1.000f, 0.937f, 0.835f, 1.000f }; // RGB: (255, 239, 213)
        const Color Moccasin                  { 1.000f, 0.894f, 0.710f, 1.000f }; // RGB: (255, 228, 181)
        const Color PeachPuff                 { 1.000f, 0.855f, 0.725f, 1.000f }; // RGB: (255, 218, 185)
        const Color PaleGoldenrod             { 0.933f, 0.910f, 0.667f, 1.000f }; // RGB: (238, 232, 170)
        const Color Khaki                     { 0.941f, 0.902f, 0.549f, 1.000f }; // RGB: (240, 230, 140)
        const Color DarkKhaki                 { 0.741f, 0.718f, 0.420f, 1.000f }; // RGB: (189, 183, 107)
        // Purples                                                                    
        const Color Lavender                  { 0.902f, 0.902f, 0.980f, 1.000f }; // RGB: (230, 230, 250)
        const Color Thistle                   { 0.847f, 0.749f, 0.847f, 1.000f }; // RGB: (216, 191, 216)
        const Color Plum                      { 0.867f, 0.627f, 0.867f, 1.000f }; // RGB: (221, 160, 221)
        const Color Violet                    { 0.933f, 0.510f, 0.933f, 1.000f }; // RGB: (238, 130, 238)
        const Color Orchid                    { 0.855f, 0.439f, 0.839f, 1.000f }; // RGB: (218, 112, 214)
        const Color Magenta                   { 1.000f, 0.000f, 1.000f, 1.000f }; // RGB: (255, 0, 255)
        const Color MediumOrchid              { 0.729f, 0.333f, 0.827f, 1.000f }; // RGB: (186, 85, 211)
        const Color MediumPurple              { 0.576f, 0.439f, 0.859f, 1.000f }; // RGB: (147, 112, 219)
        const Color BlueViolet                { 0.541f, 0.169f, 0.886f, 1.000f }; // RGB: (138, 43, 226)
        const Color DarkViolet                { 0.580f, 0.000f, 0.827f, 1.000f }; // RGB: (148, 0, 211)
        const Color DarkOrchid                { 0.600f, 0.196f, 0.800f, 1.000f }; // RGB: (153, 50, 204)
        const Color DarkMagenta               { 0.545f, 0.000f, 0.545f, 1.000f }; // RGB: (139, 0, 139)
        const Color RebeccaPurple             { 0.400f, 0.200f, 0.600f, 1.000f }; // RGB: (102, 51, 153)
        const Color Indigo                    { 0.294f, 0.000f, 0.510f, 1.000f }; // RGB: (75, 0, 130)
        const Color MediumSlateBlue           { 0.482f, 0.408f, 0.933f, 1.000f }; // RGB: (123, 104, 238)
        const Color SlateBlue                 { 0.416f, 0.353f, 0.804f, 1.000f }; // RGB: (106, 90, 205)
        const Color DarkSlateBlue             { 0.282f, 0.239f, 0.545f, 1.000f }; // RGB: (72, 61, 139)
        // Greens
        const Color GreenYellow               { 0.678f, 1.000f, 0.184f, 1.000f }; // RGB: (173, 255, 47)
        const Color Chartreuse                { 0.498f, 1.000f, 0.000f, 1.000f }; // RGB: (127, 255, 0)
        const Color LawnGreen                 { 0.486f, 0.988f, 0.000f, 1.000f }; // RGB: (124, 252, 0)
        const Color LimeGreen                 { 0.196f, 0.804f, 0.196f, 1.000f }; // RGB: (50, 205, 50)
        const Color PaleGreen                 { 0.596f, 0.984f, 0.596f, 1.000f }; // RGB: (152, 251, 152)
        const Color LightGreen                { 0.565f, 0.933f, 0.565f, 1.000f }; // RGB: (144, 238, 144)
        const Color MediumSpringGreen         { 0.000f, 0.980f, 0.604f, 1.000f }; // RGB: (0, 250, 154)
        const Color SpringGreen               { 0.000f, 1.000f, 0.498f, 1.000f }; // RGB: (0, 255, 127)
        const Color MediumSeaGreen            { 0.235f, 0.702f, 0.443f, 1.000f }; // RGB: (60, 179, 113)
        const Color SeaGreen                  { 0.180f, 0.545f, 0.341f, 1.000f }; // RGB: (46, 139, 87)
        const Color ForestGreen               { 0.133f, 0.545f, 0.133f, 1.000f }; // RGB: (34, 139, 34)
        const Color DarkGreen                 { 0.000f, 0.392f, 0.000f, 1.000f }; // RGB: (0, 100, 0)
        const Color YellowGreen               { 0.604f, 0.804f, 0.196f, 1.000f }; // RGB: (154, 205, 50)
        const Color OliveDrab                 { 0.420f, 0.557f, 0.137f, 1.000f }; // RGB: (107, 142, 35)
        const Color DarkOliveGreen            { 0.333f, 0.420f, 0.184f, 1.000f }; // RGB: (85, 107, 47)
        const Color MediumAquamarine          { 0.400f, 0.804f, 0.667f, 1.000f }; // RGB: (102, 205, 170)
        const Color DarkSeaGreen              { 0.561f, 0.737f, 0.561f, 1.000f }; // RGB: (143, 188, 143)
        const Color LightSeaGreen             { 0.125f, 0.698f, 0.667f, 1.000f }; // RGB: (32, 178, 170)
        const Color DarkCyan                  { 0.000f, 0.545f, 0.545f, 1.000f }; // RGB: (0, 139, 139)
        // Blues
        const Color Cyan                      { 0.000f, 1.000f, 1.000f, 1.000f }; // RGB: (0, 255, 255)
        const Color LightCyan                 { 0.878f, 1.000f, 1.000f, 1.000f }; // RGB: (224, 255, 255)
        const Color PaleTurquoise             { 0.686f, 0.933f, 0.933f, 1.000f }; // RGB: (175, 238, 238)
        const Color Aquamarine                { 0.498f, 1.000f, 0.831f, 1.000f }; // RGB: (127, 255, 212)
        const Color Turquoise                 { 0.251f, 0.878f, 0.816f, 1.000f }; // RGB: (64, 224, 208)
        const Color MediumTurquoise           { 0.282f, 0.820f, 0.800f, 1.000f }; // RGB: (72, 209, 204)
        const Color DarkTurquoise             { 0.000f, 0.808f, 0.820f, 1.000f }; // RGB: (0, 206, 209)
        const Color CadetBlue                 { 0.373f, 0.620f, 0.627f, 1.000f }; // RGB: (95, 158, 160)
        const Color SteelBlue                 { 0.275f, 0.510f, 0.706f, 1.000f }; // RGB: (70, 130, 180)
        const Color LightSteelBlue            { 0.690f, 0.769f, 0.871f, 1.000f }; // RGB: (176, 196, 222)
        const Color PowderBlue                { 0.690f, 0.878f, 0.902f, 1.000f }; // RGB: (176, 224, 230)
        const Color LightBlue                 { 0.678f, 0.847f, 0.902f, 1.000f }; // RGB: (173, 216, 230)
        const Color SkyBlue                   { 0.529f, 0.808f, 0.922f, 1.000f }; // RGB: (135, 206, 235)
        const Color LightSkyBlue              { 0.529f, 0.808f, 0.980f, 1.000f }; // RGB: (135, 206, 250)
        const Color DeepSkyBlue               { 0.000f, 0.749f, 1.000f, 1.000f }; // RGB: (0, 191, 255)
        const Color DodgerBlue                { 0.118f, 0.565f, 1.000f, 1.000f }; // RGB: (30, 144, 255)
        const Color CornflowerBlue            { 0.392f, 0.584f, 0.929f, 1.000f }; // RGB: (100, 149, 237)
        const Color RoyalBlue                 { 0.255f, 0.412f, 0.882f, 1.000f }; // RGB: (65, 105, 225)
        const Color MediumBlue                { 0.000f, 0.000f, 0.804f, 1.000f }; // RGB: (0, 0, 205)
        const Color DarkBlue                  { 0.000f, 0.000f, 0.545f, 1.000f }; // RGB: (0, 0, 139)
        const Color MidnightBlue              { 0.098f, 0.098f, 0.439f, 1.000f }; // RGB: (25, 25, 112)
        // Browns
        const Color Cornsilk                  { 1.000f, 0.973f, 0.863f, 1.000f }; // RGB: (255, 248, 220)
        const Color BlanchedAlmond            { 1.000f, 0.922f, 0.804f, 1.000f }; // RGB: (255, 235, 205)
        const Color Bisque                    { 1.000f, 0.894f, 0.769f, 1.000f }; // RGB: (255, 228, 196)
        const Color NavajoWhite               { 1.000f, 0.871f, 0.678f, 1.000f }; // RGB: (255, 222, 173)
        const Color Wheat                     { 0.961f, 0.871f, 0.702f, 1.000f }; // RGB: (245, 222, 179)
        const Color BurlyWood                 { 0.871f, 0.722f, 0.529f, 1.000f }; // RGB: (222, 184, 135)
        const Color Tan                       { 0.824f, 0.706f, 0.549f, 1.000f }; // RGB: (210, 180, 140)
        const Color RosyBrown                 { 0.737f, 0.561f, 0.561f, 1.000f }; // RGB: (188, 143, 143)
        const Color SandyBrown                { 0.957f, 0.643f, 0.376f, 1.000f }; // RGB: (244, 164, 96)
        const Color Goldenrod                 { 0.855f, 0.647f, 0.125f, 1.000f }; // RGB: (218, 165, 32)
        const Color DarkGoldenrod             { 0.722f, 0.525f, 0.043f, 1.000f }; // RGB: (184, 134, 11)
        const Color Peru                      { 0.804f, 0.522f, 0.247f, 1.000f }; // RGB: (205, 133, 63)
        const Color Chocolate                 { 0.824f, 0.412f, 0.118f, 1.000f }; // RGB: (210, 105, 30)
        const Color SaddleBrown               { 0.545f, 0.271f, 0.075f, 1.000f }; // RGB: (139, 69, 19)
        const Color Sienna                    { 0.627f, 0.322f, 0.176f, 1.000f }; // RGB: (160, 82, 45)
        const Color Brown                     { 0.647f, 0.165f, 0.165f, 1.000f }; // RGB: (165, 42, 42)
        // Whites
        const Color Snow                      { 1.000f, 0.980f, 0.980f, 1.000f }; // RGB: (255, 250, 250)
        const Color Honeydew                  { 0.941f, 1.000f, 0.941f, 1.000f }; // RGB: (240, 255, 240)
        const Color MintCream                 { 0.961f, 1.000f, 0.980f, 1.000f }; // RGB: (245, 255, 250)
        const Color Azure                     { 0.941f, 1.000f, 1.000f, 1.000f }; // RGB: (240, 255, 255)
        const Color AliceBlue                 { 0.941f, 0.973f, 1.000f, 1.000f }; // RGB: (240, 248, 255)
        const Color GhostWhite                { 0.973f, 0.973f, 1.000f, 1.000f }; // RGB: (248, 248, 255)
        const Color WhiteSmoke                { 0.961f, 0.961f, 0.961f, 1.000f }; // RGB: (245, 245, 245)
        const Color Seashell                  { 1.000f, 0.961f, 0.933f, 1.000f }; // RGB: (255, 245, 238)
        const Color Beige                     { 0.961f, 0.961f, 0.863f, 1.000f }; // RGB: (245, 245, 220)
        const Color OldLace                   { 0.992f, 0.961f, 0.902f, 1.000f }; // RGB: (253, 245, 230)
        const Color FloralWhite               { 1.000f, 0.980f, 0.941f, 1.000f }; // RGB: (255, 250, 240)
        const Color Ivory                     { 1.000f, 1.000f, 0.941f, 1.000f }; // RGB: (255, 255, 240)
        const Color AntiqueWhite              { 0.980f, 0.922f, 0.843f, 1.000f }; // RGB: (250, 235, 215)
        const Color Linen                     { 0.980f, 0.941f, 0.902f, 1.000f }; // RGB: (250, 240, 230)
        const Color LavenderBlush             { 1.000f, 0.941f, 0.961f, 1.000f }; // RGB: (255, 240, 245)
        const Color MistyRose                 { 1.000f, 0.894f, 0.882f, 1.000f }; // RGB: (255, 228, 225)
        // Grays
        const Color Gainsboro                 { 0.863f, 0.863f, 0.863f, 1.000f }; // RGB: (220, 220, 220)
        const Color LightGray                 { 0.827f, 0.827f, 0.827f, 1.000f }; // RGB: (211, 211, 211)
        const Color LightGrey                 { 0.827f, 0.827f, 0.827f, 1.000f }; // RGB: (211, 211, 211)
        const Color DarkGray                  { 0.663f, 0.663f, 0.663f, 1.000f }; // RGB: (169, 169, 169)
        const Color DarkGrey                  { 0.663f, 0.663f, 0.663f, 1.000f }; // RGB: (169, 169, 169)
        const Color Grey                      { 0.502f, 0.502f, 0.502f, 1.000f }; // RGB: (128, 128, 128)
        const Color DimGray                   { 0.412f, 0.412f, 0.412f, 1.000f }; // RGB: (105, 105, 105)
        const Color DimGrey                   { 0.412f, 0.412f, 0.412f, 1.000f }; // RGB: (105, 105, 105)
        const Color LightSlateGray            { 0.467f, 0.533f, 0.600f, 1.000f }; // RGB: (119, 136, 153)
        const Color LightSlateGrey            { 0.467f, 0.533f, 0.600f, 1.000f }; // RGB: (119, 136, 153)
        const Color SlateGray                 { 0.439f, 0.502f, 0.565f, 1.000f }; // RGB: (112, 128, 144)
        const Color SlateGrey                 { 0.439f, 0.502f, 0.565f, 1.000f }; // RGB: (112, 128, 144)
        const Color DarkSlateGray             { 0.184f, 0.310f, 0.310f, 1.000f }; // RGB: (47, 79, 79)
        const Color DarkSlateGrey             { 0.184f, 0.310f, 0.310f, 1.000f }; // RGB: (47, 79, 79)
    }
}

#include <AzCore/Math/Color.inl>
