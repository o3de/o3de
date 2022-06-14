/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>

namespace RecastNavigation
{
    //! Provides configuration for @RecastNavigationPhysXProviderComponent and @EditorRecastNavigationPhysXProviderComponent.
    class RecastNavigationPhysXProviderConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(RecastNavigationPhysXProviderConfig, "{310A3F15-6212-445E-A543-FB3BD6D47768}");

        static void Reflect(AZ::ReflectContext* context);

        //! Either use Editor PhysX world or game PhysX world.
        float m_useEditorScene = false;
    };
} // namespace RecastNavigation
