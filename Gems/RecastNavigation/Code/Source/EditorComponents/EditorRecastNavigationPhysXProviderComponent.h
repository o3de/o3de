/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Misc/RecastNavigationPhysXProviderCommon.h>
#include <RecastNavigation/RecastNavigationProviderBus.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationPhysXProviderComponent
    class EditorRecastNavigationPhysXProviderComponent final
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationPhysXProviderComponent,
            "{F1E57D0B-11A1-46C2-876D-720DD40CB14D}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! EditorComponentBase overrides ...
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

    private:
        //! If enabled, debug draw is enabled to show the triangles collected in the PhysX scene for the navigation mesh.
        bool m_debugDrawInputData = false;
    };
} // namespace RecastNavigation
