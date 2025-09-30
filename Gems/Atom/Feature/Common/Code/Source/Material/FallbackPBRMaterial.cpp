/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Material/ConvertEmissiveUnitFunctor.h>
#include <Atom/Feature/Material/FallbackPBRMaterial.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ::Render
{
    namespace FallbackPBR
    {
        class MaterialConversionUtil
        {
        public:
            MaterialConversionUtil(RPI::Material* sourceMaterial)
                : m_material(sourceMaterial)
            {
            }

            template<typename T>
            T GetProperty(const AZ::Name& propertyName, const T defaultValue = T{})
            {
                auto propertyIndex = m_material->FindPropertyIndex(propertyName);
                if (propertyIndex.IsValid())
                {
                    return m_material->GetPropertyValue<T>(propertyIndex);
                }
                return defaultValue;
            }

            template<typename T, size_t N>
            T GetProperty(const AZStd::array<AZ::Name, N> propertyNames, const T defaultValue = T{})
            {
                for (const auto& propertyName : propertyNames)
                {
                    auto propertyIndex = m_material->FindPropertyIndex(propertyName);
                    if (propertyIndex.IsValid())
                    {
                        return m_material->GetPropertyValue<T>(propertyIndex);
                    }
                }
                return defaultValue;
            }

            template<>
            RHI::Ptr<const RHI::ImageView> GetProperty(const AZ::Name& propertyName, const RHI::Ptr<const RHI::ImageView> defaultValue)
            {
                auto image = GetProperty<Data::Instance<RPI::Image>>(propertyName, nullptr);
                if (image)
                {
                    return image->GetImageView();
                }
                return defaultValue;
            }

            template<typename T>
            RHI::Ptr<T> GetFunctor()
            {
                for (auto& functor : m_material->GetAsset()->GetMaterialFunctors())
                {
                    auto convertedFunctor = azdynamic_cast<T*>(functor.get());
                    if (convertedFunctor != nullptr)
                    {
                        return convertedFunctor;
                        // intensity = emissiveFunctor->GetProcessedValue(intensity, unit);
                    }
                }
                return nullptr;
            }

            AZ::Name GetEnumValueName(const AZ::Name& propertyName, const AZ::Name defaultValueName)
            {
                auto propertyIndex = m_material->FindPropertyIndex(propertyName);
                if (propertyIndex.IsValid())
                {
                    uint32_t enumVal = m_material->GetPropertyValue<uint32_t>(propertyIndex);
                    return m_material->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex)->GetEnumName(enumVal);
                }
                return defaultValueName;
            }

        private:
            AZ::RPI::Material* m_material;
        };

        void ConvertMaterial(RPI::Material* material, MaterialParameters& convertedMaterial)
        {
            if (material == nullptr)
            {
                return;
            }
            MaterialConversionUtil util{ material };

            convertedMaterial.m_baseColor = util.GetProperty<AZ::Color>(AZ_NAME_LITERAL("baseColor.color"));
            convertedMaterial.m_baseColor *= util.GetProperty<float>(AZ_NAME_LITERAL("baseColor.factor"), 1.0f);

            convertedMaterial.m_metallicFactor = util.GetProperty<float>(AZ_NAME_LITERAL("metallic.factor"), 0.5f);
            convertedMaterial.m_roughnessFactor = util.GetProperty<float>(AZ_NAME_LITERAL("roughness.factor"), 0.5f);

            bool emissiveEnable = util.GetProperty<bool>(AZ_NAME_LITERAL("emissive.enable"), false);
            if (emissiveEnable)
            {
                convertedMaterial.m_emissiveColor = util.GetProperty<AZ::Color>(AZ_NAME_LITERAL("emissive.color"));
                float intensity = util.GetProperty<float>(AZ_NAME_LITERAL("emissive.intensity"), 1.0f);
                uint32_t unit = util.GetProperty<uint32_t>(AZ_NAME_LITERAL("emissive.unit"), 0);
                auto emissiveFunctor = util.GetFunctor<ConvertEmissiveUnitFunctor>();
                if (emissiveFunctor)
                {
                    intensity = emissiveFunctor->GetProcessedValue(intensity, unit);
                    convertedMaterial.m_emissiveColor *= intensity;
                }
                else
                {
                    AZ_WarningOnce(
                        "MaterialManager",
                        false,
                        "Could not find ConvertEmissiveUnitFunctor for material %s",
                        material->GetAsset()->GetId().ToFixedString().c_str());
                }
            }

            convertedMaterial.m_baseColorImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("baseColor.textureMap"));
            convertedMaterial.m_normalImageView = util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("normal.textureMap"));
            convertedMaterial.m_metallicImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("metallic.textureMap"));
            convertedMaterial.m_roughnessImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("roughness.textureMap"));
            convertedMaterial.m_emissiveImageView =
                util.GetProperty<RHI::Ptr<const RHI::ImageView>>(AZ_NAME_LITERAL("emissive.textureMap"));

            auto irradianceColorSource =
                util.GetEnumValueName(AZ_NAME_LITERAL("irradiance.irradianceColorSource"), AZ_NAME_LITERAL("Manual"));
            if (irradianceColorSource == AZ_NAME_LITERAL("Manual"))
            {
                auto propertyNames = AZStd::array{ AZ_NAME_LITERAL("irradiance.manualColor"), AZ_NAME_LITERAL("irradiance.color") };
                convertedMaterial.m_irradianceColor = util.GetProperty<AZ::Color>(propertyNames, AZ::Colors::White);
            }
            else if (irradianceColorSource == AZ_NAME_LITERAL("BaseColorTint"))
            {
                convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
            }
            else if (irradianceColorSource == AZ_NAME_LITERAL("BaseColor"))
            {
                // if we can't find the useTexture - switch, assume we want to use a texture if it's assigned
                bool useTexture = util.GetProperty(AZ_NAME_LITERAL("baseColor.useTexture"), true);
                Data::Instance<RPI::Image> baseColorImage =
                    util.GetProperty<Data::Instance<RPI::Image>>(AZ_NAME_LITERAL("baseColor.textureMap"));
                if (useTexture && baseColorImage)
                {
                    auto baseColorStreamingImg = azdynamic_cast<RPI::StreamingImage*>(baseColorImage.get());
                    if (baseColorStreamingImg)
                    {
                        // Note: there are quite a few hidden assumptions in using the average
                        // texture color. For instance, (1) it assumes that every texel in the
                        // texture actually gets mapped to the surface (or non-mapped regions are
                        // colored with a meaningful 'average' color, or have zero opacity); (2) it
                        // assumes that the mapping from uv space to the mesh surface is
                        // (approximately) area-preserving to get a properly weighted average; and
                        // mostly, (3) it assumes that a single 'average color' is a meaningful
                        // characterisation of the full material.
                        Color avgColor = baseColorStreamingImg->GetAverageColor();

                        // We do a simple 'multiply' blend with the base color
                        // Note: other blend modes are currently not supported
                        convertedMaterial.m_irradianceColor = avgColor * convertedMaterial.m_baseColor;
                    }
                    else
                    {
                        AZ_Warning(
                            "MeshFeatureProcessor",
                            false,
                            "Using BaseColor as irradianceColorSource "
                            "is currently only supported for textures of type StreamingImage");
                        // Default to the flat base color
                        convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
                    }
                }
                else
                {
                    // No texture, simply copy the baseColor
                    convertedMaterial.m_irradianceColor = convertedMaterial.m_baseColor;
                }
            }
            else
            {
                AZ_Warning(
                    "MaterialManager",
                    false,
                    "Unknown irradianceColorSource value: %s, defaulting to white",
                    irradianceColorSource.GetCStr());
                convertedMaterial.m_irradianceColor = AZ::Colors::White;
            }

            // Overall scale factor
            float irradianceScale = util.GetProperty(AZ_NAME_LITERAL("irradiance.factor"), 1.0f);
            convertedMaterial.m_irradianceColor *= irradianceScale;

            // set the transparency from the material opacity factor
            auto opacityMode = util.GetEnumValueName(AZ_NAME_LITERAL("opacity.mode"), AZ_NAME_LITERAL("Opaque"));
            if (opacityMode != AZ_NAME_LITERAL("Opaque"))
            {
                convertedMaterial.m_irradianceColor.SetA(util.GetProperty(AZ_NAME_LITERAL("opacity.factor"), 1.0f));
            }
        }
    } // namespace FallbackPBR
} // namespace AZ::Render
