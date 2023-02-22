/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace PhysX
{
    //! Class for in-editor PhysX Static Rigid Body Component.
    class EditorArticulatedBodyComponent : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorArticulatedBodyComponent, "{7D23169B-3214-4A32-ABFC-FCCE6E31F2CF}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorArticulatedBodyComponent() = default;
        ~EditorArticulatedBodyComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    };
} // namespace PhysX
