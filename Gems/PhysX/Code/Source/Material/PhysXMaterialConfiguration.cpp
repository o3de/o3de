/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Physics/NameConstants.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>

namespace PhysX
{
    void MaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::MaterialConfiguration>()
                ->Version(1)
                ->Field("DynamicFriction", &MaterialConfiguration::m_dynamicFriction)
                ->Field("StaticFriction", &MaterialConfiguration::m_staticFriction)
                ->Field("Restitution", &MaterialConfiguration::m_restitution)
                ->Field("FrictionCombine", &MaterialConfiguration::m_frictionCombine)
                ->Field("RestitutionCombine", &MaterialConfiguration::m_restitutionCombine)
                ->Field("Density", &MaterialConfiguration::m_density)
                ->Field("DebugColor", &MaterialConfiguration::m_debugColor)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {

                editContext->Class<PhysX::MaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "PhysX Material")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_staticFriction, "Static friction", "Friction coefficient when object is still")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_dynamicFriction, "Dynamic friction", "Friction coefficient when object is moving")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_restitution, "Restitution", "Restitution coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_frictionCombine, "Friction combine", "How the friction is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_restitutionCombine, "Restitution combine", "How the restitution is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_density, "Density", "Material density")
                        ->Attribute(AZ::Edit::Attributes::Min, &MaterialConfiguration::GetMinDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Max, &MaterialConfiguration::GetMaxDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetDensityUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Color, &MaterialConfiguration::m_debugColor, "Debug Color", "Debug color to use for this material")
                    ;
            }
        }
    }

    AZ::Data::Asset<Physics::MaterialAsset> MaterialConfiguration::CreateMaterialAsset() const
    {
        AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
            AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(
                AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        const Physics::MaterialAsset::MaterialProperties materialProperties =
        {
            {MaterialConstants::DynamicFrictionName, m_dynamicFriction},
            {MaterialConstants::StaticFrictionName, m_staticFriction},
            {MaterialConstants::RestitutionName, m_restitution},
            {MaterialConstants::DensityName, m_density},
            {MaterialConstants::RestitutionCombineModeName, static_cast<AZ::u32>(m_restitutionCombine)},
            {MaterialConstants::FrictionCombineModeName, static_cast<AZ::u32>(m_frictionCombine)},
            {MaterialConstants::DebugColorName, m_debugColor}
        };

        materialAsset->SetData(
            MaterialConstants::MaterialAssetType,
            MaterialConstants::MaterialAssetVersion,
            materialProperties);

        return materialAsset;
    }

    void MaterialConfiguration::ValidateMaterialAsset(
        [[maybe_unused]] AZ::Data::Asset<Physics::MaterialAsset> materialAsset)
    {
#if !defined(AZ_RELEASE_BUILD)
        if (!materialAsset)
        {
            AZ_Error("MaterialConfiguration", false, "Invalid material asset");
            return;
        }

        AZ_Error("MaterialConfiguration", materialAsset->GetMaterialType() == MaterialConstants::MaterialAssetType,
            "Material asset '%s' has unexpected material type ('%s'). Expected type is '%.*s'.",
            materialAsset.GetHint().c_str(), materialAsset->GetMaterialType().c_str(), AZ_STRING_ARG(MaterialConstants::MaterialAssetType));

        AZ_Error("MaterialConfiguration", materialAsset->GetVersion() == MaterialConstants::MaterialAssetVersion,
            "Material asset '%s' has unexpected material version (%u). Expected version is '%u'.",
            materialAsset.GetHint().c_str(), materialAsset->GetVersion(), MaterialConstants::MaterialAssetVersion);

        const AZStd::fixed_vector materialPropertyNames =
        {
            MaterialConstants::DynamicFrictionName,
            MaterialConstants::StaticFrictionName,
            MaterialConstants::RestitutionName,
            MaterialConstants::DensityName,
            MaterialConstants::RestitutionCombineModeName,
            MaterialConstants::FrictionCombineModeName,
            MaterialConstants::DebugColorName
        };

        const auto& materialProperties = materialAsset->GetMaterialProperties();

        for (const auto& materialPropertyName : materialPropertyNames)
        {
            AZ_Error("MaterialConfiguration", materialProperties.find(materialPropertyName) != materialProperties.end(),
                "Material asset '%s' does not have property '%.*s'.",
                materialAsset.GetHint().c_str(), AZ_STRING_ARG(materialPropertyName));
        }
#endif
    }

    float MaterialConfiguration::GetMinDensityLimit()
    {
        return MaterialConstants::MinDensityLimit;
    }

    float MaterialConfiguration::GetMaxDensityLimit()
    {
        return MaterialConstants::MaxDensityLimit;
    }
} // namespace PhysX
