/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationPhysXProviderComponent.h"

#include <AzCore/Serialization/EditContext.h>

namespace RecastNavigation
{
    void EditorRecastNavigationPhysXProviderComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationPhysXProviderComponent, BaseClass>()
                ->Version(1);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationPhysXProviderComponent>("Recast Navigation PhysX Provider",
                    "[Collects triangle geometry from PhysX scene for navigation mesh within the area defined by a shape component.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    EditorRecastNavigationPhysXProviderComponent::EditorRecastNavigationPhysXProviderComponent(const RecastNavigationPhysXProviderConfig& config)
        : BaseClass(config)
    {
    }
} // namespace RecastNavigation
