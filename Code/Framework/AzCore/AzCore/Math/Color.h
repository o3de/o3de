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
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_floating_point.h>

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

        //! Constructs a color from the given floating point RGBA values in the range [0..1].
        //! This must be a template function so that constructing a Color with mixed floating point and integer arguments is not possible.
        template<typename T> requires AZStd::is_floating_point_v<T>
        Color(T r, T g, T b, T a = T(1));

        //! Constructs a color from the given integer RGBA values in the range [0..255].
        //! This must be a template function so that calling the constructor with non-u8 integer types such as int/uint is not ambiguous.
        template<typename T> requires AZStd::is_integral_v<T>
        Color(T r, T g, T b, T a = T(255));

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

        //! Clamps the color to the range [0..1]
        void Saturate();

        //! Returns a color which was clamped in the range [0..1]
        Color GetSaturated() const;

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
        const Color White                { 255, 255, 255 };
        const Color Silver               { 192, 192, 192 };
        const Color Gray                 { 128, 128, 128 };
        const Color Black                {   0,   0,   0 };
        const Color Red                  { 255,   0,   0 };
        const Color Maroon               { 128,   0,   0 };
        const Color Lime                 {   0, 255,   0 };
        const Color Green                {   0, 128,   0 };
        const Color Blue                 {   0,   0, 255 };
        const Color Navy                 {   0,   0, 128 };
        const Color Yellow               { 255, 255,   0 };
        const Color Orange               { 255, 165,   0 };
        const Color Olive                { 128, 128,   0 };
        const Color Purple               { 128,   0, 128 };
        const Color Fuchsia              { 255,   0, 255 };
        const Color Teal                 {   0, 128, 128 };
        const Color Aqua                 {   0, 255, 255 };
        // CSS3 colors
        // Reds
        const Color IndianRed            { 205,  92,  92 };
        const Color LightCoral           { 240, 128, 128 };
        const Color Salmon               { 250, 128, 114 };
        const Color DarkSalmon           { 233, 150, 122 };
        const Color LightSalmon          { 255, 160, 122 };
        const Color Crimson              { 220,  20,  60 };
        const Color FireBrick            { 178,  34,  34 };
        const Color DarkRed              { 139,   0,   0 };
        // Pinks
        const Color Pink                 { 255, 192, 203 };
        const Color LightPink            { 255, 182, 193 };
        const Color HotPink              { 255, 105, 180 };
        const Color DeepPink             { 255,  20, 147 };
        const Color MediumVioletRed      { 199,  21, 133 };
        const Color PaleVioletRed        { 219, 112, 147 };
        // Oranges
        const Color Coral                { 255, 127,  80 };
        const Color Tomato               { 255,  99,  71 };
        const Color OrangeRed            { 255,  69,   0 };
        const Color DarkOrange           { 255, 140,   0 };
        // Yellows
        const Color Gold                 { 255, 215,   0 };
        const Color LightYellow          { 255, 255, 224 };
        const Color LemonChiffon         { 255, 250, 205 };
        const Color LightGoldenrodYellow { 250, 250, 210 };
        const Color PapayaWhip           { 255, 239, 213 };
        const Color Moccasin             { 255, 228, 181 };
        const Color PeachPuff            { 255, 218, 185 };
        const Color PaleGoldenrod        { 238, 232, 170 };
        const Color Khaki                { 240, 230, 140 };
        const Color DarkKhaki            { 189, 183, 107 };
        // Purples
        const Color Lavender             { 230, 230, 250 };
        const Color Thistle              { 216, 191, 216 };
        const Color Plum                 { 221, 160, 221 };
        const Color Violet               { 238, 130, 238 };
        const Color Orchid               { 218, 112, 214 };
        const Color Magenta              { 255,   0, 255 };
        const Color MediumOrchid         { 186,  85, 211 };
        const Color MediumPurple         { 147, 112, 219 };
        const Color BlueViolet           { 138,  43, 226 };
        const Color DarkViolet           { 148,   0, 211 };
        const Color DarkOrchid           { 153,  50, 204 };
        const Color DarkMagenta          { 139,   0, 139 };
        const Color RebeccaPurple        { 102,  51, 153 };
        const Color Indigo               {  75,   0, 130 };
        const Color MediumSlateBlue      { 123, 104, 238 };
        const Color SlateBlue            { 106,  90, 205 };
        const Color DarkSlateBlue        {  72,  61, 139 };
        // Greens
        const Color GreenYellow          { 173, 255,  47 };
        const Color Chartreuse           { 127, 255,   0 };
        const Color LawnGreen            { 124, 252,   0 };
        const Color LimeGreen            {  50, 205,  50 };
        const Color PaleGreen            { 152, 251, 152 };
        const Color LightGreen           { 144, 238, 144 };
        const Color MediumSpringGreen    {   0, 250, 154 };
        const Color SpringGreen          {   0, 255, 127 };
        const Color MediumSeaGreen       {  60, 179, 113 };
        const Color SeaGreen             {  46, 139,  87 };
        const Color ForestGreen          {  34, 139,  34 };
        const Color DarkGreen            {   0, 100,   0 };
        const Color YellowGreen          { 154, 205,  50 };
        const Color OliveDrab            { 107, 142,  35 };
        const Color DarkOliveGreen       {  85, 107,  47 };
        const Color MediumAquamarine     { 102, 205, 170 };
        const Color DarkSeaGreen         { 143, 188, 143 };
        const Color LightSeaGreen        {  32, 178, 170 };
        const Color DarkCyan             {   0, 139, 139 };
        // Blues
        const Color Cyan                 {   0, 255, 255 };
        const Color LightCyan            { 224, 255, 255 };
        const Color PaleTurquoise        { 175, 238, 238 };
        const Color Aquamarine           { 127, 255, 212 };
        const Color Turquoise            {  64, 224, 208 };
        const Color MediumTurquoise      {  72, 209, 204 };
        const Color DarkTurquoise        {   0, 206, 209 };
        const Color CadetBlue            {  95, 158, 160 };
        const Color SteelBlue            {  70, 130, 180 };
        const Color LightSteelBlue       { 176, 196, 222 };
        const Color PowderBlue           { 176, 224, 230 };
        const Color LightBlue            { 173, 216, 230 };
        const Color SkyBlue              { 135, 206, 235 };
        const Color LightSkyBlue         { 135, 206, 250 };
        const Color DeepSkyBlue          {   0, 191, 255 };
        const Color DodgerBlue           {  30, 144, 255 };
        const Color CornflowerBlue       { 100, 149, 237 };
        const Color RoyalBlue            {  65, 105, 225 };
        const Color MediumBlue           {   0,   0, 205 };
        const Color DarkBlue             {   0,   0, 139 };
        const Color MidnightBlue         {  25,  25, 112 };
        // Browns
        const Color Cornsilk             { 255, 248, 220 };
        const Color BlanchedAlmond       { 255, 235, 205 };
        const Color Bisque               { 255, 228, 196 };
        const Color NavajoWhite          { 255, 222, 173 };
        const Color Wheat                { 245, 222, 179 };
        const Color BurlyWood            { 222, 184, 135 };
        const Color Tan                  { 210, 180, 140 };
        const Color RosyBrown            { 188, 143, 143 };
        const Color SandyBrown           { 244, 164,  96 };
        const Color Goldenrod            { 218, 165,  32 };
        const Color DarkGoldenrod        { 184, 134,  11 };
        const Color Peru                 { 205, 133,  63 };
        const Color Chocolate            { 210, 105,  30 };
        const Color SaddleBrown          { 139,  69,  19 };
        const Color Sienna               { 160,  82,  45 };
        const Color Brown                { 165,  42,  42 };
        // Whites
        const Color Snow                 { 255, 250, 250 };
        const Color Honeydew             { 240, 255, 240 };
        const Color MintCream            { 245, 255, 250 };
        const Color Azure                { 240, 255, 255 };
        const Color AliceBlue            { 240, 248, 255 };
        const Color GhostWhite           { 248, 248, 255 };
        const Color WhiteSmoke           { 245, 245, 245 };
        const Color Seashell             { 255, 245, 238 };
        const Color Beige                { 245, 245, 220 };
        const Color OldLace              { 253, 245, 230 };
        const Color FloralWhite          { 255, 250, 240 };
        const Color Ivory                { 255, 255, 240 };
        const Color AntiqueWhite         { 250, 235, 215 };
        const Color Linen                { 250, 240, 230 };
        const Color LavenderBlush        { 255, 240, 245 };
        const Color MistyRose            { 255, 228, 225 };
        // Grays
        const Color Gainsboro            { 220, 220, 220 };
        const Color LightGray            { 211, 211, 211 };
        const Color LightGrey            { 211, 211, 211 };
        const Color DarkGray             { 169, 169, 169 };
        const Color DarkGrey             { 169, 169, 169 };
        const Color Grey                 { 128, 128, 128 };
        const Color DimGray              { 105, 105, 105 };
        const Color DimGrey              { 105, 105, 105 };
        const Color LightSlateGray       { 119, 136, 153 };
        const Color LightSlateGrey       { 119, 136, 153 };
        const Color SlateGray            { 112, 128, 144 };
        const Color SlateGrey            { 112, 128, 144 };
        const Color DarkSlateGray        {  47,  79,  79 };
        const Color DarkSlateGrey        {  47,  79,  79 };
    }
}

#include <AzCore/Math/Color.inl>
