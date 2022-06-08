/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace PhysicsLegacy
{
    // O3DE_DEPRECATION_NOTICE(GHI-9840)
    // Legacy Physics material Id class used to identify the material in the collection of materials.
    // Used when converting old material asset to new one.
    class MaterialId
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialId, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PhysicsLegacy::MaterialId, "{744CCE6C-9F69-4E2F-B950-DAB8514F870B}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid m_id = AZ::Uuid::CreateNull();
    };

    // O3DE_DEPRECATION_NOTICE(GHI-9840)
    // Legacy class to store a vector of MaterialIds selected from the library.
    // Used when converting old material asset to new one.
    class MaterialSelection
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialSelection, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PhysicsLegacy::MaterialSelection, "{F571AFF4-C4BB-4590-A204-D11D9EEABBC4}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<MaterialId> m_materialIdsAssignedToSlots;
    };
} // namespace PhysicsLegacy

