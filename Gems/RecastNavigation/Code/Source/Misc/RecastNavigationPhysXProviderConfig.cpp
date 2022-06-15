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
                ->Field("Use Editor Scene", &RecastNavigationPhysXProviderConfig::m_useEditorScene)
                ->Version(1)
                ;
        }
    }
} // namespace RecastNavigation
