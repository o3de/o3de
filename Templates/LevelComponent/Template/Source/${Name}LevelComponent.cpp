// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}LevelComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    /*
     * AZ_COMPONENT_IMPL provides the static type information and UUID registration for
     * this level component. The UUID must be unique across the entire project.
     */
    AZ_COMPONENT_IMPL(${SanitizedCppName}LevelComponent, "${SanitizedCppName}LevelComponent", "{${Random_Uuid}}");

    /*
     * Activate is called when the level entity is activated, which occurs when the level
     * is loaded and all required services are available. Level components activate after
     * the system entity components and before game entity components in the scene.
     *
     * Connect to the request bus here so that game entities and systems can communicate
     * with this component via ${SanitizedCppName}RequestBus::Event(entityId, ...).
     * The GetEntityId() call returns the level entity's ID, scoping the bus connection
     * so that only callers with that specific entity ID can reach this handler.
     */
    void ${SanitizedCppName}LevelComponent::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect(GetEntityId());
    }

    /*
     * Deactivate is called when the level is being unloaded or shut down.
     * Always disconnect from every bus connected in Activate(). Level components
     * are destroyed during level unload, so dangling connections would cause crashes
     * when the bus fires after the component memory has been freed.
     */
    void ${SanitizedCppName}LevelComponent::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect(GetEntityId());
    }

    /*
     * Reflect registers this level component with O3DE's reflection contexts.
     *
     * SerializeContext - Persists the component's data in the level prefab file.
     *   Register every member variable that should be saved with ->Field().
     *   Bump Version when the serialized layout changes to enable data migration.
     *
     * EditContext - Controls how this component appears in the Editor Inspector
     *   when the level entity is selected.
     *   AppearsInAddComponentMenu(AZ_CRC_CE("Level")) restricts this component to the
     *   level entity only. It will not appear in the Add Component menu for game entities.
     *   Use ->DataElement() to expose individual fields for editing in the Inspector.
     *
     * BehaviorContext - Exposes this component to Lua scripting and Script Canvas.
     *   Register methods and properties you want accessible from scripts using
     *   ->Method() and ->Property().
     */
    void ${SanitizedCppName}LevelComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}LevelComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}LevelComponent>("${SanitizedCppName}LevelComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<${SanitizedCppName}LevelComponent>("${SanitizedCppName} Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "${GemName} Gem Group")
                ;
        }
    }

    /*
     * GetProvidedServices declares the named service this level component provides.
     * Game entity components that depend on level-wide systems can declare a dependency
     * on this service name in their GetRequiredServices() to ensure they activate after
     * this component is ready.
     */
    void ${SanitizedCppName}LevelComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}LevelComponentService"));
    }

    /*
     * GetIncompatibleServices prevents multiple instances of this component on the
     * level entity. Level components are typically singletons - uncomment the line
     * below to enforce that only one instance can exist on the level entity at a time.
     *
     * Called by the system to make sure the level entity contains no components that
     * conflict with each other. Any service names returned here will prevent other
     * components that provide that service name from being added to the level entity
     * if this component is already present.
     */
    void ${SanitizedCppName}LevelComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Uncomment to enforce a single instance of this component on the level entity:
        // incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}LevelComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be present and active on the level
     * entity before this component activates. The system will emit an error and skip
     * activating this component if a required service is missing.
     */
    void ${SanitizedCppName}LevelComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists services that this component should activate after,
     * if they are present, but does not require. Unlike required services, a missing
     * dependent service does not prevent this component from activating.
     */
    void ${SanitizedCppName}LevelComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
