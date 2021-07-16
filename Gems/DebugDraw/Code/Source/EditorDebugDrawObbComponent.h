/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include "DebugDrawObbComponent.h"
#include "EditorDebugDrawComponentCommon.h"

namespace DebugDraw
{
    class EditorDebugDrawObbComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_EDITOR_COMPONENT(EditorDebugDrawObbComponent, "{602AF187-693F-4E04-96A7-0B2D2028A937}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    protected:

        void OnPropertyUpdate();

        DebugDrawObbElement m_element;
        EditorDebugDrawComponentSettings m_settings;
    };
}
