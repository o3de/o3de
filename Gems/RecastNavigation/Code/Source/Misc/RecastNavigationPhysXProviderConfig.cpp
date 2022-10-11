/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Misc/RecastNavigationPhysXProviderConfig.h>

namespace RecastNavigation
{
    void RecastNavigationPhysXProviderConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RecastNavigationPhysXProviderConfig>()
                ->Version(2)
                ->Field("Use Editor Scene", &RecastNavigationPhysXProviderConfig::m_useEditorScene)
                ->Field("Collision Group", &RecastNavigationPhysXProviderConfig::m_collisionGroupId)
                ;
        }
    }
} // namespace RecastNavigation
