#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_
#define AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMaterialData, "{4C0E818F-CEE8-48A0-AC3D-AC926811BFE4}", IGraphObject);

                enum class TextureMapType
                {
                    Diffuse,
                    Specular,
                    Bump,
                    Normal,
                    Metallic,
                    Roughness,
                    AmbientOcclusion,
                    Emissive,
                    BaseColor
                };

                ~IMaterialData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override
                {
                    output.Write("MaterialName", GetMaterialName());
                    output.Write("UniqueId", GetUniqueId());
                    output.Write("IsNoDraw", IsNoDraw());
                    output.Write("DiffuseColor", GetDiffuseColor());
                    output.Write("SpecularColor", GetSpecularColor());
                    output.Write("EmissiveColor", GetEmissiveColor());
                    output.Write("Opacity", GetOpacity());
                    output.Write("Shininess", GetShininess());
                    output.Write("UseColorMap", GetUseColorMap());
                    output.Write("BaseColor", GetBaseColor());
                    output.Write("UseMetallicMap", GetUseMetallicMap());
                    output.Write("MetallicFactor", GetMetallicFactor());
                    output.Write("UseRoughnessMap", GetUseRoughnessMap());
                    output.Write("RoughnessFactor", GetRoughnessFactor());
                    output.Write("UseEmissiveMap", GetUseEmissiveMap());
                    output.Write("EmissiveIntensity", GetEmissiveIntensity());
                    output.Write("UseAOMap", GetUseAOMap());
                    output.Write("DiffuseTexture", GetTexture(TextureMapType::Diffuse).c_str());
                    output.Write("SpecularTexture", GetTexture(TextureMapType::Specular).c_str());
                    output.Write("BumpTexture", GetTexture(TextureMapType::Bump).c_str());
                    output.Write("NormalTexture", GetTexture(TextureMapType::Normal).c_str());
                    output.Write("MetallicTexture", GetTexture(TextureMapType::Metallic).c_str());
                    output.Write("RoughnessTexture", GetTexture(TextureMapType::Roughness).c_str());
                    output.Write("AmbientOcclusionTexture", GetTexture(TextureMapType::AmbientOcclusion).c_str());
                    output.Write("EmissiveTexture", GetTexture(TextureMapType::Emissive).c_str());
                    output.Write("BaseColorTexture", GetTexture(TextureMapType::BaseColor).c_str());
                }

                virtual const AZStd::string& GetMaterialName() const = 0;
                virtual const AZStd::string& GetTexture(TextureMapType mapType) const = 0;
                virtual bool IsNoDraw() const = 0;

                virtual const AZ::Vector3& GetDiffuseColor() const = 0;
                virtual const AZ::Vector3& GetSpecularColor() const = 0;
                virtual const AZ::Vector3& GetEmissiveColor() const = 0;
                virtual float GetOpacity() const = 0;
                virtual float GetShininess() const = 0;

                virtual AZStd::optional<bool> GetUseColorMap() const = 0;
                virtual AZStd::optional<AZ::Vector3> GetBaseColor() const = 0;
                virtual AZStd::optional<bool> GetUseMetallicMap() const = 0;
                virtual AZStd::optional<float> GetMetallicFactor() const = 0;
                virtual AZStd::optional<bool> GetUseRoughnessMap() const = 0;
                virtual AZStd::optional<float> GetRoughnessFactor() const = 0;
                virtual AZStd::optional<bool> GetUseEmissiveMap() const = 0;
                virtual AZStd::optional<float> GetEmissiveIntensity() const = 0;
                virtual AZStd::optional<bool> GetUseAOMap() const = 0;
                                
                virtual uint64_t GetUniqueId() const = 0;
            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ

#endif // AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_
