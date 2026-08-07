// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include "${SanitizedCppName}Component.h"

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace ${GemName}
{
    /*
    * TODO: Register this component in your Gem's Editor AZ::Module interface by inserting the following:
    *       Editor${SanitizedCppName}Component::CreateDescriptor(),
    *
    * Editor components wrap the corresponding runtime component. In the editor, this component
    * is placed on entities. At export/game-mode, BuildGameEntity() instantiates the runtime component.
    */

    class Editor${SanitizedCppName}Component
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(Editor${SanitizedCppName}Component, "{${Random_Uuid}}", AzToolsFramework::Components::EditorComponentBase);

        /*
        * Reflects component data into the reflection contexts.
        * The edit context reflection here controls how the component appears in the Editor Inspector.
        */
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        /*
        * Called at game export and when entering game mode.
        * Add the runtime component to the game entity, transferring any serialized data as needed.
        */
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    };

} // namespace ${GemName}
