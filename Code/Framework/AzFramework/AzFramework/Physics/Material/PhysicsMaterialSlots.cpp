/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/Asset/AssetSerializer.h>

#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>

namespace Physics
{
    const char* const MaterialSlots::EntireObjectSlotName = "Entire object";

    // Json serializer that clears containers before they are serialized.
    // This is necessary for MaterialSlots class because it adds
    // 1 element to its vector in the constructor to start with valid data,
    // but by default json serializer does not clear containers and this causes
    // it to end up with more elements than it should have.
    class JsonMaterialSlotContainerSerializer
        : public AZ::JsonBasicContainerSerializer
    {
    public:
        AZ_RTTI(Physics::JsonMaterialSlotContainerSerializer, "{1F71E69B-F4A0-46EF-A5BE-98028C931253}", AZ::JsonBasicContainerSerializer);

    protected:
        bool ShouldClearContainer(const AZ::JsonDeserializerContext&) const override
        {
            return true;
        }
    };

    static AZ::Data::AssetId GetDefaultPhysicsMaterialAssetId()
    {
        // Used for Edit Context.
        // When the physics material asset property doesn't have an asset assigned it
        // will show "(default)" to indicate that the default material will be used.
        if (auto* materialManager = AZ::Interface<Physics::MaterialManager>::Get())
        {
            if (AZStd::shared_ptr<Physics::Material> defaultMaterial = materialManager->GetDefaultMaterial())
            {
                return defaultMaterial->GetMaterialAsset().GetId();
            }
        }
        return {};
    }

    void MaterialSlots::MaterialSlot::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialSlots::MaterialSlot>()
                ->Version(1)
                ->Field("Name", &MaterialSlot::m_name)
                ->Field("MaterialAsset", &MaterialSlot::m_materialAsset)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialSlots::MaterialSlot>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSlot::m_materialAsset, "", "")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &MaterialSlot::m_name)
                        ->Attribute(AZ::Edit::Attributes::DefaultAsset, &GetDefaultPhysicsMaterialAssetId)
                        ->Attribute(AZ_CRC_CE("EditButton"), "")
                        ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                        ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &MaterialSlot::m_slotsReadOnly)
                    ;
            }
        }
    }

    void MaterialSlots::Reflect(AZ::ReflectContext* context)
    {
        MaterialSlot::Reflect(context);

        if (auto* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<JsonMaterialSlotContainerSerializer>()
                ->HandlesType<AZStd::vector<MaterialSlot>>();
        }

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialSlots>()
                ->Version(1)
                ->Field("Slots", &MaterialSlots::m_slots)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Physics::MaterialSlots>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialSlots::m_slots, "Physics Materials",
                        "Select which physics materials to use for each slot.")
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ValueText, " ")
                        ->ElementAttribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    MaterialSlots::MaterialSlots()
    {
        SetSlots(MaterialDefaultSlot::Default);
    }

    void MaterialSlots::SetSlots(MaterialDefaultSlot)
    {
        SetSlots(AZStd::vector<AZStd::string>{});
    }

    void MaterialSlots::SetSlots(const AZStd::vector<AZStd::string>& slots)
    {
        // Using resize it's important to not lose the material assets already assigned
        // to the slots, if there were any.
        if (slots.empty())
        {
            m_slots.resize(1);
            m_slots[0].m_name = EntireObjectSlotName;
        }
        else
        {
            m_slots.resize(slots.size());
            for ( size_t i = 0; i < slots.size(); ++i)
            {
                m_slots[i].m_name = slots[i];
            }
        }
    }

    void MaterialSlots::SetMaterialAsset(size_t slotIndex, const AZ::Data::Asset<MaterialAsset>& materialAsset)
    {
        if (slotIndex < m_slots.size())
        {
            m_slots[slotIndex].m_materialAsset = materialAsset;
        }
    }

    size_t MaterialSlots::GetSlotsCount() const
    {
        return m_slots.size();
    }

    AZStd::string_view MaterialSlots::GetSlotName(size_t slotIndex) const
    {
        if (slotIndex < m_slots.size())
        {
            return m_slots[slotIndex].m_name;
        }
        else
        {
            return "<error>";
        }
    }

    AZStd::vector<AZStd::string> MaterialSlots::GetSlotsNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_slots.size());
        for (const auto& slot : m_slots)
        {
            names.push_back(slot.m_name);
        }
        return names;
    }

    const AZ::Data::Asset<MaterialAsset> MaterialSlots::GetMaterialAsset(size_t slotIndex) const
    {
        if (slotIndex < m_slots.size())
        {
            return m_slots[slotIndex].m_materialAsset;
        }
        else
        {
            return {};
        }
    }

    void MaterialSlots::SetSlotsReadOnly(bool readOnly)
    {
        for (auto& slot : m_slots)
        {
            slot.m_slotsReadOnly = readOnly;
        }
    }
} // namespace Physics
