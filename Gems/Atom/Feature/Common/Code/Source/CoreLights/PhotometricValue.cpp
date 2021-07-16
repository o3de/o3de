/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/CoreLights/PhotometricValue.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void PhotometricValue::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PhotometricValue>()
                    ->Version(3)
                    ->Field("Chroma", &PhotometricValue::m_chroma)
                    ->Field("Intensity", &PhotometricValue::m_intensity)
                    ->Field("Area", &PhotometricValue::m_area)
                    ->Field("Unit", &PhotometricValue::m_unit)
                    ;
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext
                    ->Enum<(int)PhotometricUnit::Candela>("PhotometricUnit_Candela")
                    ->Enum<(int)PhotometricUnit::Lumen>("PhotometricUnit_Lumen")
                    ->Enum<(int)PhotometricUnit::Lux>("PhotometricUnit_Lux")
                    ->Enum<(int)PhotometricUnit::Ev100Luminance>("PhotometricUnit_Ev100_Luminance")
                    ->Enum<(int)PhotometricUnit::Ev100Illuminance>("PhotometricUnit_Ev100_Illuminance")
                    ->Enum<(int)PhotometricUnit::Nit>("PhotometricUnit_Nit")
                    ->Enum<(int)PhotometricUnit::Unknown>("PhotometricUnit_Unknown")
                    ;
            }

        }

        const char* PhotometricValue::GetTypeSuffix(PhotometricUnit type)
        {
            switch (type)
            {
            case PhotometricUnit::Candela:
                return " cd";
                break;
            case PhotometricUnit::Lumen:
                return " lm";
                break;
            case PhotometricUnit::Ev100Luminance:
            case PhotometricUnit::Ev100Illuminance:
                return " ev";
                break;
            case PhotometricUnit::Lux:
                return " lx";
                break;
            case PhotometricUnit::Nit:
                return " nt";
                break;
            }
            return "";
        }

        float PhotometricValue::ConvertIntensityBetweenUnits(PhotometricUnit fromUnit, PhotometricUnit toUnit, float intensity, float effectiveSolidAngle, float area)
        {
            // Only error if attempting to set the toUnit to Unknown. If only the fromUnit is Unknown then a photometric unit is just being assigned.
            AZ_Error("PhotometricValue", toUnit != PhotometricUnit::Unknown, "Can't convert intensity to PhotometricUnit::Unknown.");

            if (fromUnit == toUnit || fromUnit == PhotometricUnit::Unknown || toUnit == PhotometricUnit::Unknown)
            {
                // Same type or Unknown type, nothing to do.
                return intensity;
            }

            // First convert to lumens
            switch (fromUnit)
            {
            case PhotometricUnit::Candela:
                intensity *= effectiveSolidAngle;
                break;
            case PhotometricUnit::Lumen:
                break; // Do nothing, already in lumens
            case PhotometricUnit::Ev100Luminance:
                // First convert to nits then fallthrough
                intensity = (Ev100LightMeterConstantLuminance * pow(2.0f, intensity)) / (Ev100Iso * Ev100ShutterSpeed);
                [[fallthrough]];
            case PhotometricUnit::Nit:
                intensity *= Constants::Pi;
                intensity = area > 0.0f ? intensity * area : intensity;
                break;
            case PhotometricUnit::Ev100Illuminance:
                // First convert to lux then fallthrough
                intensity = (Ev100LightMeterConstantIlluminance * pow(2.0f, intensity)) / (Ev100Iso * Ev100ShutterSpeed);
                [[fallthrough]];
            case PhotometricUnit::Lux:
                intensity = area > 0.0f ? intensity * area : intensity;
                break;
            }

            // Convert from lumens to final type
            switch (toUnit)
            {
            case PhotometricUnit::Candela:
                intensity /= effectiveSolidAngle;
                break;
            case PhotometricUnit::Lumen:
                // Do nothing, already in lumens
                break;
            case PhotometricUnit::Ev100Luminance:
                // Convert to nits first
                intensity /= Constants::Pi;
                intensity = area > 0.0f ? intensity / area : intensity;
                // Now convert to Ev100
                intensity = log2((Ev100Iso * Ev100ShutterSpeed * intensity) / Ev100LightMeterConstantLuminance);
                break;
            case PhotometricUnit::Nit:
                intensity /= Constants::Pi;
                intensity = area > 0.0f ? intensity / area : intensity;
                break;
            case PhotometricUnit::Ev100Illuminance:
                // Convert to lux first
                intensity = area > 0.0f ? intensity / area : intensity;
                // Now convert to Ev100
                intensity = log2((Ev100Iso * Ev100ShutterSpeed * intensity) / Ev100LightMeterConstantIlluminance);
                break;
            case PhotometricUnit::Lux:
                intensity = area > 0.0f ? intensity / area : intensity;
                break;
            }

            return intensity;
        }

        float PhotometricValue::GetPerceptualLuminance(AZ::Color color)
        {
            // Assumes rec 709 sRGB primaries.
            // https://en.wikipedia.org/wiki/Luma_(video)
            // https://en.wikipedia.org/wiki/Relative_luminance
            return color.GetR() * 0.2126f + color.GetG() * 0.7152f + color.GetB() * 0.0722f;
        }

        PhotometricValue::PhotometricValue(float intensity, const AZ::Color& color, PhotometricUnit unit)
            : m_chroma(color)
            , m_intensity(intensity)
            , m_unit(unit)
        {
        }

        AZ::Color PhotometricValue::GetCombinedRgb() const
        {
            return m_chroma * m_intensity;
        }

        float PhotometricValue::GetCombinedIntensity() const
        {
            return GetPerceptualLuminance(GetCombinedRgb());
        }

        float PhotometricValue::GetCombinedIntensity(PhotometricUnit unit) const
        {
            if (unit != m_unit)
            {
                PhotometricValue copy = *this;
                copy.ConvertToPhotometricUnit(unit);
                return copy.GetCombinedIntensity();
            }
            return GetCombinedIntensity();
        }

        void PhotometricValue::SetChroma(const AZ::Color& color)
        {
            m_chroma = color;
        }

        void PhotometricValue::SetArea(float area)
        {
            m_area = area;
        }

        void PhotometricValue::SetEffectiveSolidAngle(float steradians)
        {
            m_effectiveSolidAngle = steradians;
        }

        void PhotometricValue::SetIntensity(float intensity)
        {
            m_intensity = intensity;
        }

        const AZ::Color& PhotometricValue::GetChroma() const
        {
            return m_chroma;
        }

        float PhotometricValue::GetArea() const
        {
            return m_area;
        }

        float PhotometricValue::GetEffectiveSolidAngle()
        {
            return m_effectiveSolidAngle;
        }

        float PhotometricValue::GetIntensity() const
        {
            return m_intensity;
        }

        void PhotometricValue::ConvertToPhotometricUnit(PhotometricUnit unit)
        {
            m_intensity = ConvertIntensityBetweenUnits(m_unit, unit, m_intensity, m_effectiveSolidAngle, m_area);
            m_unit = unit;
        }
    }
}
