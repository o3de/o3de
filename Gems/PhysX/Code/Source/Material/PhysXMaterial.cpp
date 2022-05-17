/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

#include <PhysX/Material/PhysXMaterial.h>

namespace PhysX
{
    static Physics::CombineMode FromPxCombineMode(physx::PxCombineMode::Enum pxMode)
    {
        switch (pxMode)
        {
        case physx::PxCombineMode::eAVERAGE:
            return Physics::CombineMode::Average;
        case physx::PxCombineMode::eMULTIPLY:
            return Physics::CombineMode::Multiply;
        case physx::PxCombineMode::eMAX:
            return Physics::CombineMode::Maximum;
        case physx::PxCombineMode::eMIN:
            return Physics::CombineMode::Minimum;
        default:
            return Physics::CombineMode::Average;
        }
    }

    static physx::PxCombineMode::Enum ToPxCombineMode(Physics::CombineMode mode)
    {
        switch (mode)
        {
        case Physics::CombineMode::Average:
            return physx::PxCombineMode::eAVERAGE;
        case Physics::CombineMode::Multiply:
            return physx::PxCombineMode::eMULTIPLY;
        case Physics::CombineMode::Maximum:
            return physx::PxCombineMode::eMAX;
        case Physics::CombineMode::Minimum:
            return physx::PxCombineMode::eMIN;
        default:
            return physx::PxCombineMode::eAVERAGE;
        }
    }

    AZStd::shared_ptr<Material> Material::FindOrCreateMaterial(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::rtti_pointer_cast<Material>(
            AZ::Interface<Physics::MaterialManager>::Get()->FindOrCreateMaterial(
                Physics::MaterialId::CreateFromAssetId(materialAsset.GetId()),
                materialAsset));
    }

    AZStd::vector<AZStd::shared_ptr<Material>> Material::FindOrCreateMaterials(const Physics::MaterialSlots& materialSlots)
    {
        AZStd::shared_ptr<Material> defaultMaterial =
            AZStd::rtti_pointer_cast<Material>(
                AZ::Interface<Physics::MaterialManager>::Get()->GetDefaultMaterial());

        const size_t slotsCount = materialSlots.GetSlotsCount();

        AZStd::vector<AZStd::shared_ptr<Material>> materials;
        materials.reserve(slotsCount);

        for (size_t slotIndex = 0; slotIndex < slotsCount; ++slotIndex)
        {
            if (const auto materialAsset = materialSlots.GetMaterialAsset(slotIndex);
                materialAsset.GetId().IsValid())
            {
                auto material = Material::FindOrCreateMaterial(materialAsset);
                if (material)
                {
                    materials.push_back(material);
                }
                else
                {
                    materials.push_back(defaultMaterial);
                }
            }
            else
            {
                materials.push_back(defaultMaterial);
            }
        }

        return materials;
    }

    AZStd::shared_ptr<Material> Material::CreateMaterialWithRandomId(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::rtti_pointer_cast<Material>(
            AZ::Interface<Physics::MaterialManager>::Get()->FindOrCreateMaterial(
                Physics::MaterialId::CreateRandom(),
                materialAsset));
    }

    Material::Material(
        const Physics::MaterialId& id,
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
        : Physics::Material(id, materialAsset)
    {
        const Physics::MaterialConfiguration defaultMaterialConf;

        m_pxMaterial = PxMaterialUniquePtr(
            PxGetPhysics().createMaterial(
                defaultMaterialConf.m_staticFriction, defaultMaterialConf.m_dynamicFriction, defaultMaterialConf.m_restitution),
            [](physx::PxMaterial* pxMaterial)
            {
                pxMaterial->release();
                pxMaterial->userData = nullptr;
            });
        AZ_Assert(m_pxMaterial, "Failed to create physx material");
        m_pxMaterial->userData = this;

        const auto& materialConfiguration = m_materialAsset->GetMaterialConfiguration();

        SetStaticFriction(materialConfiguration.m_staticFriction);
        SetDynamicFriction(materialConfiguration.m_dynamicFriction);
        SetRestitution(materialConfiguration.m_restitution);
        SetFrictionCombineMode(materialConfiguration.m_frictionCombine);
        SetRestitutionCombineMode(materialConfiguration.m_restitutionCombine);
        SetDensity(materialConfiguration.m_density);
        SetDebugColor(materialConfiguration.m_debugColor);

        // Connect to asset bus to listen to asset reloads notifications
        AZ::Data::AssetBus::Handler::BusConnect(m_materialAsset.GetId());
    }

    Material::~Material()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    float Material::GetDynamicFriction() const
    {
        return m_pxMaterial->getDynamicFriction();
    }

    void Material::SetDynamicFriction(float dynamicFriction)
    {
        AZ_Warning(
            "PhysX Material", dynamicFriction >= 0.0f, "Dynamic friction value %f is out of range, 0 will be used.", dynamicFriction);

        m_pxMaterial->setDynamicFriction(AZ::GetMax(0.0f, dynamicFriction));
    }

    float Material::GetStaticFriction() const
    {
        return m_pxMaterial->getStaticFriction();
    }

    void Material::SetStaticFriction(float staticFriction)
    {
        AZ_Warning("PhysX Material", staticFriction >= 0.0f, "Static friction value %f is out of range, 0 will be used.", staticFriction);

        m_pxMaterial->setStaticFriction(AZ::GetMax(0.0f, staticFriction));
    }

    float Material::GetRestitution() const
    {
        return m_pxMaterial->getRestitution();
    }

    void Material::SetRestitution(float restitution)
    {
        AZ_Warning(
            "PhysX Material", restitution >= 0.0f && restitution <= 1.0f, "Restitution value %f will be clamped into range [0, 1]",
            restitution);

        m_pxMaterial->setRestitution(AZ::GetClamp(restitution, 0.0f, 1.0f));
    }

    Physics::CombineMode Material::GetFrictionCombineMode() const
    {
        return FromPxCombineMode(m_pxMaterial->getFrictionCombineMode());
    }

    void Material::SetFrictionCombineMode(Physics::CombineMode mode)
    {
        m_pxMaterial->setFrictionCombineMode(ToPxCombineMode(mode));
    }

    Physics::CombineMode Material::GetRestitutionCombineMode() const
    {
        return FromPxCombineMode(m_pxMaterial->getRestitutionCombineMode());
    }

    void Material::SetRestitutionCombineMode(Physics::CombineMode mode)
    {
        m_pxMaterial->setRestitutionCombineMode(ToPxCombineMode(mode));
    }

    float Material::GetDensity() const
    {
        return m_density;
    }

    void Material::SetDensity(float density)
    {
        AZ_Warning(
            "PhysX Material", density >= Physics::MaterialConfiguration::MinDensityLimit && density <= Physics::MaterialConfiguration::MaxDensityLimit,
            "Density value %f will be clamped into range [%f, %f].", density, Physics::MaterialConfiguration::MinDensityLimit,
            Physics::MaterialConfiguration::MaxDensityLimit);

        m_density = AZ::GetClamp(density, Physics::MaterialConfiguration::MinDensityLimit, Physics::MaterialConfiguration::MaxDensityLimit);
    }

    const AZ::Color& Material::GetDebugColor() const
    {
        return m_debugColor;
    }

    void Material::SetDebugColor(const AZ::Color& debugColor)
    {
        m_debugColor = debugColor;
    }

    const physx::PxMaterial* Material::GetPxMaterial() const
    {
        return m_pxMaterial.get();
    }

    void Material::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_materialAsset = asset;

        const auto& materialConfiguration = m_materialAsset->GetMaterialConfiguration();

        SetStaticFriction(materialConfiguration.m_staticFriction);
        SetDynamicFriction(materialConfiguration.m_dynamicFriction);
        SetRestitution(materialConfiguration.m_restitution);
        SetFrictionCombineMode(materialConfiguration.m_frictionCombine);
        SetRestitutionCombineMode(materialConfiguration.m_restitutionCombine);
        SetDensity(materialConfiguration.m_density);
        SetDebugColor(materialConfiguration.m_debugColor);
    }
} // namespace PhysX
