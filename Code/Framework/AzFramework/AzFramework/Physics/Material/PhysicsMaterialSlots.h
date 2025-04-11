/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    enum class MaterialDefaultSlot
    {
        Default
    };

    //! The class is used to store a list of material assets.
    //! Each material will be assigned to a slot and when reflected
    //! to edit context it will show it for each slot entry.
    class MaterialSlots
    {
    public:
        AZ_CLASS_ALLOCATOR(Physics::MaterialSlots, AZ::SystemAllocator);
        AZ_RTTI(Physics::MaterialSlots, "{8A0D64CB-C98E-42E3-96A9-B81D7118CA6F}");

        static void Reflect(AZ::ReflectContext* context);

        static const char* const EntireObjectSlotName;

        //! Contstructor will already provide a valid default slot.
        MaterialSlots();
        virtual ~MaterialSlots() = default;

        //! Sets one material slot with the default label "Entire Object".
        //! It will resize the material slots but without reassigning the material assets in them.
        void SetSlots(MaterialDefaultSlot);

        //! Sets an array of material slots.
        //! It will resize the material slots but without reassigning the material assets in them.
        //! @param slots List of labels for slots. It can be empty, in which case it's the same as calling SetSlots(MaterialDefaultSlot::Default).
        void SetSlots(const AZStd::vector<AZStd::string>& slots);

        //! Sets a material asset to a specific slot.
        void SetMaterialAsset(size_t slotIndex, const AZ::Data::Asset<MaterialAsset>& materialAsset);

        //! Returns the number of slots.
        size_t GetSlotsCount() const;

        //! Returns the name of a specific slot.
        AZStd::string_view GetSlotName(size_t slotIndex) const;

        //! Returns the names of all slots.
        AZStd::vector<AZStd::string> GetSlotsNames() const;

        //! Returns the material assigned assigned to a specific slots.
        //! A slot can have no asset assigned, in which case it will return
        //! a null asset.
        const AZ::Data::Asset<MaterialAsset> GetMaterialAsset(size_t slotIndex) const;

        //! Set if the material slots are editable in the edit context.
        void SetSlotsReadOnly(bool readOnly);

    protected:
        struct MaterialSlot
        {
            AZ_TYPE_INFO(Physics::MaterialSlots::MaterialSlot, "{B5AA3CC9-637F-44FB-A4AE-4621D37884BA}");

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_name;
            AZ::Data::Asset<MaterialAsset> m_materialAsset;

            // Used for the ReadOnly attribute of m_materialAsset in edit context.
            bool m_slotsReadOnly = false;
        };

        AZStd::vector<MaterialSlot> m_slots;
    };
} // namespace Physics
