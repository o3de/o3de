// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "Editor${SanitizedCppName}SystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    /*
     * AZ_COMPONENT_IMPL provides the static type information and UUID registration for
     * the editor component. This UUID must differ from the runtime component's UUID.
     * The editor component and runtime component are separate types with separate identities
     * in the serialization system.
     */
    AZ_COMPONENT_IMPL(Editor${SanitizedCppName}SystemComponent, "Editor${SanitizedCppName}SystemComponent", "{${Random_Uuid}}");

    /*
     * Reflect registers the editor component with O3DE's reflection contexts.
     *
     * SerializeContext - Persists editor-side data when the level is saved. Any fields
     *   you want carried into the exported game (via BuildGameEntity) must be serialized here.
     *   Bump the Version number when you make breaking serialization changes.
     *
     * EditContext - Controls how this component appears in the Editor Inspector.
     *   AutoExpand(true) causes the component's property section to be open by default.
     *   ViewportIcon sets the icon rendered in the 3D viewport when this component is selected.
     *   Category places the component in the Add Component menu.
     *   AppearsInAddComponentMenu(AZ_CRC_CE("Game")) makes it available on game entities.
     *   Use ->DataElement() for each member variable you want editable in the Inspector.
     *
     * IMPORTANT: Because this editor component exists, the runtime ${SanitizedCppName}SystemComponent
     * must be hidden from the Editor's Add Component menu to avoid confusion. In the runtime
     * component's Reflect(), set its AppearsInAddComponentMenu attribute to an empty string:
     *   ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE(""))
     * Without this, both the editor and runtime versions will appear in the menu and a user
     * adding the runtime component directly will bypass BuildGameEntity entirely.
     *
     * Note: The BehaviorContext is intentionally not reflected here. Editor components are
     * not exposed to scripting - only the runtime component is. If you need script access
     * to editor properties, expose them through the runtime component's BehaviorContext.
     */
    void Editor${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Editor${SanitizedCppName}SystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Editor${SanitizedCppName}SystemComponent>("${SanitizedCppName}SystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    /*
     * GetProvidedServices declares the services this editor component provides.
     * The service names should match the runtime component's provided services so that
     * the dependency system treats the editor and runtime components consistently.
     */
    void Editor${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}SystemComponentService"));
    }

    /*
     * GetIncompatibleServices prevents duplicate editor components from being added to
     * the same entity. Uncomment the line below to enforce a single-instance constraint.
     */
    void Editor${SanitizedCppName}SystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Uncomment to prevent more than one instance of this component per entity:
        // incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}SystemComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be active on the entity before this
     * editor component activates. Mirror the runtime component's requirements here.
     */
    void Editor${SanitizedCppName}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists optional ordering dependencies. The editor component
     * activates after these services if they are present, but does not fail without them.
     */
    void Editor${SanitizedCppName}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    /*
     * BuildGameEntity is called when exporting the level or entering game mode in the Editor.
     * This is where the editor component creates and configures its corresponding runtime
     * component on the exported game entity.
     *
     * Any serialized data that the runtime component needs must be transferred here.
     * Use gameEntity->CreateComponent<${SanitizedCppName}SystemComponent>(...) and pass your
     * editor-side property values into the runtime component's constructor or setters.
     *
     * The gameEntity passed in is a separate entity from the editor entity. It will be
     * activated in game mode with the runtime component you create here.
     */
    void Editor${SanitizedCppName}SystemComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<${SanitizedCppName}SystemComponent>();
    }

} // namespace ${GemName}
