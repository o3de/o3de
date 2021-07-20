/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <PhysXCharacters/Components/CharacterGameplayComponent.h>

namespace PhysX
{
    //! Editor component that allows a PhysX character gameplay component to be edited.
    class EditorCharacterGameplayComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCharacterGameplayComponent, "{3BA7C3CB-C471-4230-8EC9-9EC4C529436F}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        EditorCharacterGameplayComponent() = default;

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        CharacterGameplayConfiguration m_gameplayConfig;
    };
} // namespace PhysX
