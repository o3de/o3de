/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StdAfx.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Blast/BlastMaterial.h>
#include <NvBlastExtDamageShaders.h>

namespace
{
    const AZ::Data::Asset<Blast::BlastMaterialLibraryAsset> s_invalidMaterialLibrary = {
        AZ::Data::AssetLoadBehavior::NoLoad};
} // namespace

namespace Blast
{
    class BlastMaterialLibraryAssetEventHandler : public AZ::SerializeContext::IEventHandler
    {
        void OnReadBegin(void* classPtr)
        {
            auto matAsset = static_cast<BlastMaterialLibraryAsset*>(classPtr);
            matAsset->GenerateMissingIds();
        }
    };

    void BlastMaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        BlastMaterialId::Reflect(context);
        BlastMaterialFromAssetConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlastMaterialConfiguration>()
                ->Version(1)
                ->Field("MaterialName", &BlastMaterialConfiguration::m_materialName)
                ->Field("Health", &BlastMaterialConfiguration::m_health)
                ->Field("ForceDivider", &BlastMaterialConfiguration::m_forceDivider)
                ->Field("MinDamageThreshold", &BlastMaterialConfiguration::m_minDamageThreshold)
                ->Field("MaxDamageThreshold", &BlastMaterialConfiguration::m_maxDamageThreshold)
                ->Field("StressLinearFactor", &BlastMaterialConfiguration::m_stressLinearFactor)
                ->Field("StressAngularFactor", &BlastMaterialConfiguration::m_stressAngularFactor);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlastMaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Blast Material")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_materialName, "Material name",
                        "Name of the material")
                    ->Attribute(AZ::Edit::Attributes::MaxLength, 64)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_health, "Health",
                        "All damage is subtracted from this value")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_forceDivider, "Force divider",
                        "All damage which originates with force is divided by this amount")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_minDamageThreshold,
                        "Minimum damage threshold", "Incoming damage is discarded if it is less than this value")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_maxDamageThreshold,
                        "Maximum damage threshold", "Incoming damage is capped at this value")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_stressLinearFactor,
                        "Stress linear factor",
                        "Factor with which linear stress such as gravity, direct impulse, collision is applied")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialConfiguration::m_stressAngularFactor,
                        "Stress angular factor", "Factor with which angular stress is applied")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f);
            }
        }
    }

    void BlastMaterialLibraryAsset::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlastMaterialLibraryAsset>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->EventHandler<BlastMaterialLibraryAssetEventHandler>()
                ->Field("Properties", &BlastMaterialLibraryAsset::m_materialLibrary);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlastMaterialLibraryAsset>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialLibraryAsset::m_materialLibrary, "Blast Materials",
                        "List of blast materials")
                    ->Attribute("EditButton", "")
                    ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    void BlastActorConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BlastActorConfiguration>()
                ->Version(1)
                ->Field("CollisionLayer", &BlastActorConfiguration::m_collisionLayer)
                ->Field("CollisionGroupId", &BlastActorConfiguration::m_collisionGroupId)
                ->Field("Simulated", &BlastActorConfiguration::m_isSimulated)
                ->Field("InSceneQueries", &BlastActorConfiguration::m_isInSceneQueries)
                ->Field("CcdEnabled", &BlastActorConfiguration::m_isCcdEnabled)
                ->Field("ColliderTag", &BlastActorConfiguration::m_tag);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BlastActorConfiguration>("BlastActorConfiguration", "Configuration for a collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isSimulated, "Simulated",
                        "If set, this actor's collider will partake in collision in the physical simulation")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isInSceneQueries, "In Scene Queries",
                        "If set, this actor's collider will be visible for scene queries")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_isCcdEnabled, "CCD Enabled",
                        "If set, actor's rigid body will have CCD enabled")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_collisionLayer, "Collision Layer",
                        "The collision layer assigned to the collider")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_collisionGroupId, "Collides With",
                        "The collision group containing the layers this collider collides with")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastActorConfiguration::m_tag, "Tag",
                        "Tag used to identify colliders from one another");
            }
        }
    }

    void BlastMaterialFromAssetConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlastMaterialFromAssetConfiguration>()
                ->Version(1)
                ->Field("Configuration", &BlastMaterialFromAssetConfiguration::m_configuration)
                ->Field("UID", &BlastMaterialFromAssetConfiguration::m_id);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlastMaterialFromAssetConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &BlastMaterialFromAssetConfiguration::m_configuration,
                        "Blast Material", "Blast Material properties")
                    ->Attribute(AZ::Edit::Attributes::ForceAutoExpand, true);
            }
        }
    }

    void BlastMaterialId::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Blast::BlastMaterialId>()->Version(1)->Field(
                "BlastMaterialId", &Blast::BlastMaterialId::m_id);
        }
    }

    BlastMaterial::BlastMaterial(const BlastMaterialConfiguration& configuration)
    {
        m_health = configuration.m_health;
        // This is not an error, health in ExtPxMaterial is actually damage divider and not health
        m_material.health = configuration.m_forceDivider;
        m_material.minDamageThreshold = configuration.m_minDamageThreshold;
        m_material.maxDamageThreshold = configuration.m_maxDamageThreshold;
        m_stressLinearFactor = configuration.m_stressLinearFactor;
        m_stressAngularFactor = configuration.m_stressAngularFactor;
        m_name = configuration.m_materialName;
    }

    const AZStd::string& BlastMaterial::GetMaterialName() const
    {
        return m_name;
    }

    float BlastMaterial::GetHealth() const
    {
        return m_health;
    }

    float BlastMaterial::GetForceDivider() const
    {
        return m_material.health;
    }

    float BlastMaterial::GetMinDamageThreshold() const
    {
        return m_material.minDamageThreshold;
    }

    float BlastMaterial::GetMaxDamageThreshold() const
    {
        return m_material.maxDamageThreshold;
    }

    float BlastMaterial::GetStressLinearFactor() const
    {
        return m_stressLinearFactor;
    }

    float BlastMaterial::GetStressAngularFactor() const
    {
        return m_stressAngularFactor;
    }

    float BlastMaterial::GetNormalizedDamage(const float damage) const
    {
        return m_material.getNormalizedDamage(damage * m_material.health);
    }

    Nv::Blast::ExtStressSolverSettings BlastMaterial::GetStressSolverSettings(uint32_t iterationsCount) const
    {
        auto settings = Nv::Blast::ExtStressSolverSettings();
        settings.hardness = m_material.health;
        settings.stressLinearFactor = m_stressLinearFactor;
        settings.stressAngularFactor = m_stressAngularFactor;
        settings.graphReductionLevel = 0;
        settings.bondIterationsPerFrame = iterationsCount;
        return settings;
    }

    void* BlastMaterial::GetNativePointer()
    {
        return &m_material;
    }

    bool BlastMaterialLibraryAsset::GetDataForMaterialId(
        const BlastMaterialId& materialId, BlastMaterialFromAssetConfiguration& configuration) const
    {
        const auto foundMaterialConfiguration = AZStd::find_if(
            m_materialLibrary.begin(), m_materialLibrary.end(),
            [materialId](const auto& data)
            {
                return data.m_id == materialId;
            });

        if (foundMaterialConfiguration != m_materialLibrary.end())
        {
            configuration = *foundMaterialConfiguration;
            return true;
        }

        return false;
    }

    bool BlastMaterialLibraryAsset::HasDataForMaterialId(const BlastMaterialId& materialId) const
    {
        auto foundMaterialConfiguration = AZStd::find_if(
            m_materialLibrary.begin(), m_materialLibrary.end(),
            [materialId](const auto& data)
            {
                return data.m_id == materialId;
            });
        return foundMaterialConfiguration != m_materialLibrary.end();
    }

    bool BlastMaterialLibraryAsset::GetDataForMaterialName(
        const AZStd::string& materialName, BlastMaterialFromAssetConfiguration& configuration) const
    {
        auto foundMaterialConfiguration = AZStd::find_if(
            m_materialLibrary.begin(), m_materialLibrary.end(),
            [&materialName](const auto& data)
            {
                return data.m_configuration.m_materialName == materialName;
            });

        if (foundMaterialConfiguration != m_materialLibrary.end())
        {
            configuration = *foundMaterialConfiguration;
            return true;
        }

        return false;
    }

    void BlastMaterialLibraryAsset::AddMaterialData(const BlastMaterialFromAssetConfiguration& data)
    {
        BlastMaterialFromAssetConfiguration existingConfiguration;
        if (!data.m_id.IsNull() && GetDataForMaterialId(data.m_id, existingConfiguration))
        {
            AZ_Warning("BlastMaterialLibraryAsset", false, "Trying to add material that already exists");
            return;
        }

        m_materialLibrary.push_back(data);
        GenerateMissingIds();
    }

    void BlastMaterialLibraryAsset::GenerateMissingIds()
    {
        for (auto& materialData : m_materialLibrary)
        {
            if (materialData.m_id.IsNull())
            {
                materialData.m_id = BlastMaterialId::Create();
            }
        }
    }

    BlastMaterialId BlastMaterialId::Create()
    {
        BlastMaterialId id;
        id.m_id = AZ::Uuid::Create();
        return id;
    }
} // namespace Blast
