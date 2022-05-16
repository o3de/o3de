/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Components/RecastNavigationMeshConfig.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationMeshComponent.
    class EditorRecastNavigationMeshComponent final
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationMeshComponent, "{22D516D4-C98D-4783-85A4-1ABE23CAB4D4}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! EditorComponentBase interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

    private:
        RecastNavigationMeshConfig m_meshConfig;
        bool m_enableDebugDraw = false;
    };
} // namespace RecastNavigation
