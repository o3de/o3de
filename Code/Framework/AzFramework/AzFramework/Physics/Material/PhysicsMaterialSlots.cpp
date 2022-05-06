/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Asset/AssetSerializer.h>

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
//#include <PhysX/MeshAsset.h>

// TODO: This file is Work in Progress

namespace Physics
{
    namespace
    {
        const char* const EntireObjectSlotName = "Entire object";
    }

    void MaterialSlots::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialSlots>()
                ->Version(1)
                ->Field("Slots", &MaterialSlots::m_slots)
                ->Field("MaterialAssets", &MaterialSlots::m_materialAssets)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialSlots>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSlots::m_materialAssets, "Physics Materials",
                        "List of slots for Physics materials.")
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &MaterialSlots::GetSlotLabel)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ_CRC_CE("ValueText"), " ")
                        ->ElementAttribute(AZ::Edit::Attributes::ReadOnly, &MaterialSlots::m_slotsReadOnly)
                        ->ElementAttribute(AZ::Edit::Attributes::DefaultAsset, &MaterialSlots::GetDefaultMaterialAssetId)
                        ->ElementAttribute(AZ_CRC_CE("EditButton"), "")
                        ->ElementAttribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                        ->ElementAttribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ;
            }
        }
    }

    MaterialSlots::MaterialSlots()
    {
        SetSlots({}); // Create default slot
    }

    void MaterialSlots::SetSlots(const AZStd::vector<AZStd::string>& slots)
    {
        if (slots.empty())
        {
            m_slots = { EntireObjectSlotName };
        }
        else
        {
            m_slots = slots;
        }

        m_materialAssets.resize(m_slots.size());
    }

    void MaterialSlots::SetSlotsFromPhysicsAsset(const Physics::ShapeConfiguration& shapeConfiguration)
    {
        if (shapeConfiguration.GetShapeType() != Physics::ShapeType::PhysicsAsset)
        {
            return;
        }

        const Physics::PhysicsAssetShapeConfiguration& assetConfiguration =
            static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);

        if (!assetConfiguration.m_asset.GetId().IsValid())
        {
            // Set the default selection if there's no physics asset.
            SetSlots({});
            return;
        }

        if (!assetConfiguration.m_asset.IsReady())
        {
            // The asset is valid but is still loading,
            // Do not set the empty slots in this case to avoid the entity being in invalid state
            return;
        }

        /*Pipeline::MeshAsset* meshAsset = assetConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();
        if (!meshAsset)
        {
            SetSlots({});
            AZ_Warning("Physics", false, "Invalid mesh asset in physics asset shape configuration.");
            return;
        }

        // Set the slots from the mesh asset
        SetSlots(meshAsset->m_assetData.m_materialNames);*/

        // Lastly, check if it has to use the materials assets from the mesh.
        if (assetConfiguration.m_useMaterialsFromAsset)
        {
            // TODO: m_assetData.m_physicsMaterialNames must be replaced with m_assetData.m_physicsMaterialAssets and
            // then here they get set to m_materialAssets

            // Update material IDs in the selection for each slot
            /*const AZStd::vector<AZStd::string>& physicsMaterialNames = meshAsset->m_assetData.m_physicsMaterialNames;
            for (size_t slotIndex = 0; slotIndex < physicsMaterialNames.size(); ++slotIndex)
            {
                const AZStd::string& physicsMaterialNameFromPhysicsAsset = physicsMaterialNames[slotIndex];
                if (physicsMaterialNameFromPhysicsAsset.empty() ||
                    physicsMaterialNameFromPhysicsAsset == Physics::DefaultPhysicsMaterialLabel)
                {
                    materialSelection.SetMaterialId(Physics::MaterialId(), static_cast<int>(slotIndex));
                    continue;
                }

                if (auto it = FindOrCreateMaterial(physicsMaterialNameFromPhysicsAsset);
                    it != m_materials.end())
                {
                    materialSelection.SetMaterialId(Physics::MaterialId::FromUUID(it->first), static_cast<int>(slotIndex));
                }
                else
                {
                    AZ_Warning("Physics", false,
                        "UpdateMaterialSelectionFromPhysicsAsset: Physics material '%s' not found in the material library. Mesh material '%s' will use the default physics material.",
                        physicsMaterialNameFromPhysicsAsset.c_str(),
                        meshAsset->m_assetData.m_materialNames[slotIndex].c_str());
                    materialSelection.SetMaterialId(Physics::MaterialId(), static_cast<int>(slotIndex));
                }
            }*/
        }
    }

    void MaterialSlots::SetSlotsReadOnly(bool readOnly)
    {
        m_slotsReadOnly = readOnly;
    }

    AZStd::string MaterialSlots::GetSlotLabel(int index) const
    {
        if (index >= 0 && index < m_slots.size())
        {
            return m_slots[index];
        }
        else
        {
            return "<error>";
        }
    }

    AZ::Data::AssetId MaterialSlots::GetDefaultMaterialAssetId() const
    {
        return {};
    }
} // namespace Physics
