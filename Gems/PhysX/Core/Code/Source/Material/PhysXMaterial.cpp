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
#include <PhysX/Material/PhysXMaterialConfiguration.h>

namespace PhysX
{
    static CombineMode FromPxCombineMode(physx::PxCombineMode::Enum pxMode)
    {
        switch (pxMode)
        {
        case physx::PxCombineMode::eAVERAGE:
            return CombineMode::Average;
        case physx::PxCombineMode::eMULTIPLY:
            return CombineMode::Multiply;
        case physx::PxCombineMode::eMAX:
            return CombineMode::Maximum;
        case physx::PxCombineMode::eMIN:
            return CombineMode::Minimum;
        default:
            return CombineMode::Average;
        }
    }

    static physx::PxCombineMode::Enum ToPxCombineMode(CombineMode mode)
    {
        switch (mode)
        {
        case CombineMode::Average:
            return physx::PxCombineMode::eAVERAGE;
        case CombineMode::Multiply:
            return physx::PxCombineMode::eMULTIPLY;
        case CombineMode::Maximum:
            return physx::PxCombineMode::eMAX;
        case CombineMode::Minimum:
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
        const MaterialConfiguration defaultMaterialConfiguration;

        // Create the PxMaterial with default values
        m_pxMaterial = PxMaterialUniquePtr(
            PxGetPhysics().createMaterial(
                defaultMaterialConfiguration.m_staticFriction, defaultMaterialConfiguration.m_dynamicFriction, defaultMaterialConfiguration.m_restitution),
            [](physx::PxMaterial* pxMaterial)
            {
                pxMaterial->release();
                pxMaterial->userData = nullptr;
            });
        AZ_Assert(m_pxMaterial, "Failed to create physx material");
        m_pxMaterial->userData = this;

        // Assign default values to members
        m_restitution = defaultMaterialConfiguration.m_restitution;
        m_compliantContactModeDamping = defaultMaterialConfiguration.m_compliantContactMode.m_damping;
        m_compliantContactModeStiffness = defaultMaterialConfiguration.m_compliantContactMode.m_stiffness;
        m_density = defaultMaterialConfiguration.m_density;
        m_debugColor = defaultMaterialConfiguration.m_debugColor;

        // When OnAssetReady is called, it will set all the properties from the material asset
        AZ::Data::AssetBus::Handler::BusConnect(m_materialAsset.GetId());
    }

    Material::~Material()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    Physics::MaterialPropertyValue Material::GetProperty(AZStd::string_view propertyName) const
    {
        if (propertyName == MaterialConstants::DynamicFrictionName)
        {
            return GetDynamicFriction();
        }
        else if (propertyName == MaterialConstants::StaticFrictionName)
        {
            return GetStaticFriction();
        }
        else if (propertyName == MaterialConstants::RestitutionName)
        {
            return GetRestitution();
        }
        else if (propertyName == MaterialConstants::DensityName)
        {
            return GetDensity();
        }
        else if (propertyName == MaterialConstants::RestitutionCombineModeName)
        {
            return static_cast<AZ::u32>(GetRestitutionCombineMode());
        }
        else if (propertyName == MaterialConstants::FrictionCombineModeName)
        {
            return static_cast<AZ::u32>(GetFrictionCombineMode());
        }
        else if (propertyName == MaterialConstants::CompliantContactModeEnabledName)
        {
            return IsCompliantContactModeEnabled();
        }
        else if (propertyName == MaterialConstants::CompliantContactModeDampingName)
        {
            return GetCompliantContactModeDamping();
        }
        else if (propertyName == MaterialConstants::CompliantContactModeStiffnessName)
        {
            return GetCompliantContactModeStiffness();
        }
        else if (propertyName == MaterialConstants::DebugColorName)
        {
            return GetDebugColor();
        }
        else
        {
            AZ_Error("PhysX::Material", false, "Unknown property '%.*s'", AZ_STRING_ARG(propertyName));
            return 0.0f;
        }
    }

    void Material::SetProperty(AZStd::string_view propertyName, Physics::MaterialPropertyValue value)
    {
        if (propertyName == MaterialConstants::DynamicFrictionName)
        {
            SetDynamicFriction(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::StaticFrictionName)
        {
            SetStaticFriction(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::RestitutionName)
        {
            SetRestitution(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::DensityName)
        {
            SetDensity(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::RestitutionCombineModeName)
        {
            SetRestitutionCombineMode(static_cast<CombineMode>(value.GetValue<AZ::u32>()));
        }
        else if (propertyName == MaterialConstants::FrictionCombineModeName)
        {
            SetFrictionCombineMode(static_cast<CombineMode>(value.GetValue<AZ::u32>()));
        }
        else if (propertyName == MaterialConstants::CompliantContactModeEnabledName)
        {
            EnableCompliantContactMode(value.GetValue<bool>());
        }
        else if (propertyName == MaterialConstants::CompliantContactModeDampingName)
        {
            SetCompliantContactModeDamping(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::CompliantContactModeStiffnessName)
        {
            SetCompliantContactModeStiffness(value.GetValue<float>());
        }
        else if (propertyName == MaterialConstants::DebugColorName)
        {
            SetDebugColor(value.GetValue<AZ::Color>());
        }
        else
        {
            AZ_Error("PhysX::Material", false, "Unknown property '%.*s'", AZ_STRING_ARG(propertyName));
        }
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
        return m_restitution;
    }

    void Material::SetRestitution(float restitution)
    {
        AZ_Warning(
            "PhysX Material", restitution >= 0.0f && restitution <= 1.0f, "Restitution value %f will be clamped into range [0, 1]",
            restitution);

        m_restitution = AZ::GetClamp(restitution, 0.0f, 1.0f);

        // Restitution property in a PxMaterial is reused for spring stiffness when compliant contact mode is enabled.
        if (!IsCompliantContactModeEnabled())
        {
            m_pxMaterial->setRestitution(m_restitution);
        }
    }

    CombineMode Material::GetFrictionCombineMode() const
    {
        return FromPxCombineMode(m_pxMaterial->getFrictionCombineMode());
    }

    void Material::SetFrictionCombineMode(CombineMode mode)
    {
        m_pxMaterial->setFrictionCombineMode(ToPxCombineMode(mode));
    }

    CombineMode Material::GetRestitutionCombineMode() const
    {
        return FromPxCombineMode(m_pxMaterial->getRestitutionCombineMode());
    }

    void Material::SetRestitutionCombineMode(CombineMode mode)
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
            "PhysX Material", density >= MaterialConstants::MinDensityLimit && density <= MaterialConstants::MaxDensityLimit,
            "Density value %f will be clamped into range [%f, %f].", density, MaterialConstants::MinDensityLimit, MaterialConstants::MaxDensityLimit);

        m_density = AZ::GetClamp(density, MaterialConstants::MinDensityLimit, MaterialConstants::MaxDensityLimit);
    }

    bool Material::IsCompliantContactModeEnabled() const
    {
#if (PX_PHYSICS_VERSION_MAJOR >= 5)
        return m_pxMaterial->getFlags().isSet(physx::PxMaterialFlag::eCOMPLIANT_CONTACT);
#else
        return false;
#endif
    }

    void Material::EnableCompliantContactMode([[maybe_unused]] bool enabled)
    {
#if (PX_PHYSICS_VERSION_MAJOR >= 5)
        m_pxMaterial->setFlag(physx::PxMaterialFlag::eCOMPLIANT_CONTACT, enabled);
        if (enabled)
        {
            m_pxMaterial->setDamping(m_compliantContactModeDamping);
            // PxMaterial uses negative values in the restitution property for the stiffness of Compliant Contacts
            m_pxMaterial->setRestitution(-m_compliantContactModeStiffness);
        }
        else
        {
            m_pxMaterial->setDamping(0.0f);
            // Restores restitution value when Compliant Contact Modde is disabled
            m_pxMaterial->setRestitution(m_restitution);
        }
#endif
    }

    float Material::GetCompliantContactModeDamping() const
    {
        return m_compliantContactModeDamping;
    }

    void Material::SetCompliantContactModeDamping([[maybe_unused]] float damping)
    {
#if (PX_PHYSICS_VERSION_MAJOR >= 5)
        AZ_Warning("PhysX Material", damping >= 0.0f, "Compliant Contact Mode Damping value %f is out of range, 0 will be used.", damping);

        m_compliantContactModeDamping = AZ::GetMax(0.0f, damping);

        if (IsCompliantContactModeEnabled())
        {
            m_pxMaterial->setDamping(m_compliantContactModeDamping);
        }
#endif
    }

    float Material::GetCompliantContactModeStiffness() const
    {
        return m_compliantContactModeStiffness;
    }

    void Material::SetCompliantContactModeStiffness([[maybe_unused]] float stiffness)
    {
#if (PX_PHYSICS_VERSION_MAJOR >= 5)
        AZ_Warning(
            "PhysX Material", stiffness >= 0.0f, "Compliant Contact Mode Stiffness value %f is out of range, 0 will be used.", stiffness);

        m_compliantContactModeStiffness = AZ::GetMax(0.0f, stiffness);

        if (IsCompliantContactModeEnabled())
        {
            // PxMaterial uses negative values in the restitution property for the stiffness of Compliant Contacts
            m_pxMaterial->setRestitution(-m_compliantContactModeStiffness);
        }
#endif
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

    void Material::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_materialAsset = asset;

        MaterialConfiguration::ValidateMaterialAsset(m_materialAsset);

        for (const auto& materialProperty : m_materialAsset->GetMaterialProperties())
        {
            SetProperty(materialProperty.first, materialProperty.second);
        }
    }

    void Material::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }
} // namespace PhysX
