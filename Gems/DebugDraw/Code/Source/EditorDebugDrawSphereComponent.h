/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include "DebugDrawSphereComponent.h"
#include "EditorDebugDrawComponentCommon.h"

namespace DebugDraw
{
    class EditorDebugDrawSphereComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
        friend class DebugDrawSystemComponent;
    public:
        AZ_EDITOR_COMPONENT(EditorDebugDrawSphereComponent, "{9853D5B5-5028-4B1A-9937-8378F8638F27}", AzToolsFramework::Components::EditorComponentBase);
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

        DebugDrawSphereElement m_element;
        EditorDebugDrawComponentSettings m_settings;
    };
}
