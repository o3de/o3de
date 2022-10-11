/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/Material/Legacy/LegacyPhysicsMaterialSelection.h>

namespace PhysicsLegacy
{
    void MaterialId::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysicsLegacy::MaterialId>()
                ->Version(1)
                ->Field("MaterialId", &PhysicsLegacy::MaterialId::m_id)
                ;
        }
    }

    void MaterialSelection::Reflect(AZ::ReflectContext* context)
    {
        MaterialId::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysicsLegacy::MaterialSelection>()
                ->Version(4)
                ->Field("MaterialIds", &MaterialSelection::m_materialIdsAssignedToSlots)
                ;
        }
    }
} // namespace PhysicsLegacy
