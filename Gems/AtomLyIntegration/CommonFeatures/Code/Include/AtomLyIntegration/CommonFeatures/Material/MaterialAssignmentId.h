/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <Atom/RPI.Reflect/Model/ModelMaterialSlot.h>

namespace AZ
{
    namespace Render
    {
        using MaterialAssignmentLodIndex = AZ::u64;

        //! MaterialAssignmentId is used to address available and overridable material slots on a model.
        //! The LOD and one of the model's original material slot IDs are used as coordinates that identify
        //! a specific material slot or a set of slots matching either.
        struct MaterialAssignmentId final
        {
            AZ_RTTI(AZ::Render::MaterialAssignmentId, "{EB603581-4654-4C17-B6DE-AE61E79EDA97}");
            AZ_CLASS_ALLOCATOR(AZ::Render::MaterialAssignmentId, SystemAllocator);
            static void Reflect(ReflectContext* context);
            static bool ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            MaterialAssignmentId() = default;

            MaterialAssignmentId(MaterialAssignmentLodIndex lodIndex, RPI::ModelMaterialSlot::StableId materialSlotStableId);

            //! Create an ID that maps to all material slots, regardless of slot ID or LOD, effectively applying to an entire model.
            static MaterialAssignmentId CreateDefault();

            //! Create an ID that maps to all material slots with a corresponding slot ID, regardless of LOD.
            static MaterialAssignmentId CreateFromStableIdOnly(RPI::ModelMaterialSlot::StableId materialSlotStableId);

            //! Create an ID that maps to a specific material slot with a corresponding stable ID and LOD.
            static MaterialAssignmentId CreateFromLodAndStableId(MaterialAssignmentLodIndex lodIndex, RPI::ModelMaterialSlot::StableId materialSlotStableId);

            //! Returns true if the slot stable ID and LOD are invalid, meaning this assignment applies to the entire model.
            bool IsDefault() const;

            //! Returns true if the slot stable ID is valid and LOD is invalid, meaning this assignment applies to every LOD.
            bool IsSlotIdOnly() const;

            //! Returns true if the slot stable ID and LOD are both valid, meaning this assignment applies to a single material slot on a specific LOD.
            bool IsLodAndSlotId() const;

            //! Creates a string describing all the details of the assignment ID
            AZStd::string ToString() const;

            //! Creates a hash composed of all elements of the assignment ID
            size_t GetHash() const;

            bool operator==(const MaterialAssignmentId& rhs) const;
            bool operator!=(const MaterialAssignmentId& rhs) const;

            static constexpr MaterialAssignmentLodIndex NonLodIndex = std::numeric_limits<MaterialAssignmentLodIndex>::max();

            MaterialAssignmentLodIndex m_lodIndex = NonLodIndex;
            RPI::ModelMaterialSlot::StableId m_materialSlotStableId = RPI::ModelMaterialSlot::InvalidStableId;
        };

    } // namespace Render
} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::Render::MaterialAssignmentId>
    {
        size_t operator()(const AZ::Render::MaterialAssignmentId& id) const
        {
            return id.GetHash();
        }
    };
} // namespace AZStd
