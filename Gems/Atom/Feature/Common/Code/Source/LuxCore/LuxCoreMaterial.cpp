/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include "LuxCoreMaterial.h"
#include <Atom/Feature/LuxCore/LuxCoreBus.h>

#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>

namespace AZ
{
    namespace Render
    {
        LuxCoreMaterial::LuxCoreMaterial(const AZ::Data::Instance<AZ::RPI::Material>& material)
        {
            Init(material);
        }

        LuxCoreMaterial::LuxCoreMaterial(const LuxCoreMaterial &material)
        {
            Init(material.m_material);
        }

        LuxCoreMaterial::~LuxCoreMaterial()
        {
            if (m_material)
            {
                m_material = nullptr;
            }
        }

        void LuxCoreMaterial::Init(const AZ::Data::Instance<AZ::RPI::Material>& material)
        {
            m_material = material;
            m_luxCoreMaterialName = "scene.materials." + m_material->GetId().ToString<AZStd::string>();
            m_luxCoreMaterial = m_luxCoreMaterial << luxrays::Property(std::string(m_luxCoreMaterialName.data()) + ".type")("disney");

            ParseProperty(s_pbrColorGroup, ".basecolor");
            ParseProperty(s_pbrMetallicGroup, ".metallic");
            ParseProperty(s_pbrRoughnessGroup, ".roughness");
            ParseProperty(s_pbrSpecularGroup, ".specular");
            ParseProperty(s_pbrNormalGroup, ".bumptex");
        }

        bool LuxCoreMaterial::ParseTexture(const char* group, AZStd::string propertyName)
        {
            AZ::RPI::MaterialPropertyIndex propertyIndex;

            propertyIndex = m_material->FindPropertyIndex(MakePbrPropertyName(group, s_pbrUseTextureProperty));
            bool useTexture = m_material->GetPropertyValue<bool>(propertyIndex);

            if (useTexture)
            {
                propertyIndex = m_material->FindPropertyIndex(MakePbrPropertyName(group, s_pbrTextureProperty));
                Data::Instance<RPI::Image> texture = m_material->GetPropertyValue<Data::Instance<RPI::Image>>(propertyIndex);

                if (texture)
                {
                    if (group == s_pbrNormalGroup)
                    {
                        AZ::Render::LuxCoreRequestsBus::Broadcast(&AZ::Render::LuxCoreRequestsBus::Events::AddTexture, texture, LuxCoreTextureType::Normal);
                    }
                    else if (group == s_pbrColorGroup)
                    {
                        AZ::Render::LuxCoreRequestsBus::Broadcast(&AZ::Render::LuxCoreRequestsBus::Events::AddTexture, texture, LuxCoreTextureType::Albedo);
                    }
                    else
                    {
                        AZ::Render::LuxCoreRequestsBus::Broadcast(&AZ::Render::LuxCoreRequestsBus::Events::AddTexture, texture, LuxCoreTextureType::Default);
                    }

                    AZStd::string materialProperty = m_luxCoreMaterialName + propertyName;
                    m_luxCoreMaterial = m_luxCoreMaterial << luxrays::Property(std::string(materialProperty.data()))(std::string(texture->GetAssetId().ToString<AZStd::string>().data()));
                    return true;
                }
            }

            return false;
        }

        AZ::Name LuxCoreMaterial::MakePbrPropertyName(const char* groupName, const char* propertyName) const
        {
            return AZ::Name{AZStd::string::format("%s.%s", groupName, propertyName)};
        }

        void LuxCoreMaterial::ParseProperty(const char* group, AZStd::string propertyName)
        {
            if (!ParseTexture(group, propertyName))
            {
                if (group == s_pbrNormalGroup)
                {
                    // Normal should always be texture
                    return;
                }
                else if (group == s_pbrColorGroup)
                {
                    AZ::RPI::MaterialPropertyIndex propertyIndex;
                    propertyIndex = m_material->FindPropertyIndex(MakePbrPropertyName(s_pbrColorGroup, s_pbrColorProperty));
                    Color color = m_material->GetPropertyValue<Color>(propertyIndex);
                    propertyIndex = m_material->FindPropertyIndex(MakePbrPropertyName(s_pbrColorGroup, s_pbrFactorProperty));
                    float factor = m_material->GetPropertyValue<float>(propertyIndex);

                    AZStd::string materialProperty = m_luxCoreMaterialName + propertyName;
                    m_luxCoreMaterial = m_luxCoreMaterial << luxrays::Property(std::string(materialProperty.data()))(float(color.GetR())* factor, float(color.GetG())*factor, float(color.GetB())*factor);
                }
                else
                {
                    AZ::RPI::MaterialPropertyIndex propertyIndex;
                    propertyIndex = m_material->FindPropertyIndex(MakePbrPropertyName(group, s_pbrFactorProperty));
                    float factor = m_material->GetPropertyValue<float>(propertyIndex);

                    AZStd::string materialProperty = m_luxCoreMaterialName + propertyName;
                    m_luxCoreMaterial = m_luxCoreMaterial << luxrays::Property(std::string(materialProperty.data()))(factor);
                }
            }
        }

        luxrays::Properties LuxCoreMaterial::GetLuxCoreMaterialProperties()
        {
            return m_luxCoreMaterial;
        }

        AZ::Data::InstanceId LuxCoreMaterial::GetMaterialId()
        {
            return m_material->GetId();
        }
    }
}

#endif
