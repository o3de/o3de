/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace RecastNavigation
{
    //! Editor version of @DetourNavigationComponent.
    class EditorDetourNavigationComponent final
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorDetourNavigationComponent, "{A8D728AB-FC42-42AE-A904-3CF5F1C83D16}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! EditorComponentBase interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

    private:
        AZ::EntityId m_navQueryEntityId;
        float m_nearestDistance = 3.f;
    };
} // namespace RecastNavigation
