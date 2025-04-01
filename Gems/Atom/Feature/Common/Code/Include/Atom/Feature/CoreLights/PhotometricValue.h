/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/ReflectContext.h>

namespace AZ
{
    class Vector3;

    namespace Render
    {
        enum class PhotometricUnit : AZ::u8
        {
            Lumen,              // Total amount of luminous power emitted. Since a unit sphere is 4 pi steradians, 1 candela emitting uniformly in all directions is 4 pi lumens.
            Candela,            // Base unit of luminous intensity; luminous power per unit solid angle.
            Lux,                // One lux is one lumen per square meter. The same lux emitting from larger areas emits more lumens than smaller areas.
            Nit,                // Nits are candela per square meter. It can be calculated as Lux / Pi.
            Ev100Luminance,     // Exposure value for luminance - Similar to nits, A measurement of illuminance that grows exponentially. See https://en.wikipedia.org/wiki/Exposure_value
            Ev100Illuminance,   // Exposure value for illuminance - Similar to lux, A measurement of illuminance that grows exponentially. See https://en.wikipedia.org/wiki/Exposure_value
            Unknown,
        };

        // An AZ::Color typed to a specific photometric unit.
        template <PhotometricUnit T>
        class PhotometricColor final : public Color
        {
        public:
            PhotometricColor() = default;
            explicit PhotometricColor(const Color& color)
                : Color(color)
            {}
        };

        //! Stores and converts between photometric data stored in various units like Lux, Lumens, and EV100
        class PhotometricValue final
        {
        public:

            AZ_RTTI(PhotometricValue, "61931C74-75B6-49CA-BE50-ABFFA9D8C4D6");
            static void Reflect(ReflectContext* context);

            static constexpr float Ev100LightMeterConstantIlluminance = 250.0f;
            static constexpr float Ev100LightMeterConstantLuminance = 12.5f;
            static constexpr float Ev100Iso = 100.0f;
            static constexpr float Ev100ShutterSpeed = 1.0f;
            static constexpr float LuxToNitsRatio = 1.0f / Constants::Pi;

            static constexpr float OmnidirectionalSteradians = 4.0f * Constants::Pi;
            static constexpr float DirectionalEffectiveSteradians = Constants::Pi; // Total effective steradians for lambertian emission.

            PhotometricValue() = default;

            //! Creates a new photometric value.
            //! @param intensity - The value of the photometric unit.
            //! @param colorMask - The  0.0 - 1.0 color of the light used as a mask against the value.
            //! @param unit - The Photometric unit of the value.
            PhotometricValue(float intensity, const AZ::Color& colorMask, PhotometricUnit unit);

            //! Gets the characters for an abbreviation of a photometric unit including preceeding space character.
            static const char* GetTypeSuffix(PhotometricUnit type);

            //! Converts intensity from the oldType to the newType given an area in meters squared.
            //!   effectiveSolidAngle is only required when converting between candela and another type.
            //!   area is only required when converting between one photometric unit that is in terms of area and
            //!   another that is not.
            static float ConvertIntensityBetweenUnits(PhotometricUnit fromUnit, PhotometricUnit toUnit, float intensity, float effectiveSolidAngle = OmnidirectionalSteradians, float area = 1.0f);

            //! Returns a single luminance value in linear space from a color in linear sRGB space based on the percieved brightness of red, green, and blue.
            static float GetPerceptualLuminance(AZ::Color color);

            //! Returns a color in the current unit type that is the combination of the chroma and intensity.
            Color GetCombinedRgb() const;
            //! Returns a color in the provided unit type is the combination of the chroma and intensity.
            template <PhotometricUnit T>
            PhotometricColor<T> GetCombinedRgb() const;
            //! Returns an intensity in the current unit type that takes into account the chroma assuming sRGB primaries
            float GetCombinedIntensity() const;
            //! Returns an intensity in the provided unit type that takes into account the chroma assuming sRGB primaries.
            float GetCombinedIntensity(PhotometricUnit type) const;

            //! Set the chroma of the light value.
            void SetChroma(const Color& chroma);
            //! Set the intensity of the light value in the current working photometric unit.
            void SetIntensity(float value);
            //! Sets the area the light covers in meteres squared.
            void SetArea(float area);
            //! Sets the effective number of steradians this light covers taking into account changes in brightness depending on the angle.
            //! For instance, a uniform omnidirectional light will output a full 4pi steradians, but a flat surface with lambertian emission will
            //! output only pi steradians over a hemisphere.
            void SetEffectiveSolidAngle(float steradians);

            //! Returns the chroma of the light value
            const Color& GetChroma() const;
            //! Returns the intensity of the light value in the current working photometric unit.
            float GetIntensity() const;
            //! Returns the area this light value covers in meters squared.
            float GetArea() const;
            //! Returns the effective number of steradians that this light covers. See SetEffectiveSolidAngle() for details.
            float GetEffectiveSolidAngle();

            //! Converts this PhotometricValue to another photometric unit while maintaining the overall physical light intensity.
            void ConvertToPhotometricUnit(PhotometricUnit unit);

            //! Returns which light type this particular light is
            PhotometricUnit GetType() const { return m_unit; };

        private:

            Color m_chroma = AZ::Color::CreateZero();
            float m_intensity = 1.0f;
            float m_area = 0.0f;
            float m_effectiveSolidAngle = OmnidirectionalSteradians; // Affects how candela is converted to other units. 
            PhotometricUnit m_unit = PhotometricUnit::Unknown;
        };

        template <PhotometricUnit T>
        PhotometricColor<T> PhotometricValue::GetCombinedRgb() const
        {
            if (T != m_unit)
            {
                PhotometricValue copy = *this;
                copy.ConvertToPhotometricUnit(T);
                return  PhotometricColor<T> { copy.GetCombinedRgb() };
            }
            return  PhotometricColor<T> { GetCombinedRgb() };
        }
    }
}
