// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}SystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    /*
     * AZ_COMPONENT_IMPL provides the static type information and UUID registration for
     * this system component. The UUID must be unique across the entire project.
     */
    AZ_COMPONENT_IMPL(${SanitizedCppName}SystemComponent, "${SanitizedCppName}SystemComponent", "{${Random_Uuid}}");

    /*
     * Activate is called when the system entity is activated during engine startup,
     * before any game entities are created or activated.
     *
     * System components typically connect to engine-wide buses rather than entity-scoped
     * buses. Use AZ::Interface<T>::Register(this) if exposing a singleton interface, or
     * connect to a broadcast bus (no entity ID) to respond to engine-wide events.
     *
     * For a global system like this, connect without an ID using BusConnect().
     */
    void ${SanitizedCppName}SystemComponent::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect();
    }

    /*
     * Deactivate is called when the engine is shutting down, after all game entities
     * have been deactivated. Disconnect from all buses and release all resources acquired
     * in Activate(). If using AZ::Interface<T>::Register, call Unregister here.
     *
     * The engine can also deactivate and reactivate the system entity without destroying
     * it (for example when re-entering editor mode). This function must leave the component
     * in a clean state that allows Activate() to be called again.
     */
    void ${SanitizedCppName}SystemComponent::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect();
    }

    /*
     * Reflect registers this system component with O3DE's reflection contexts.
     *
     * SerializeContext - Persists the component's configuration data in the system
     *   entity prefab (systementity.cfg). Use ->Field() for each configurable setting.
     *   Bump Version when the serialized layout changes to enable data migration.
     *
     * EditContext - Controls how this component appears in the System Entity Editor
     *   (accessible via Edit > Project Settings > Configure System Components).
     *   AppearsInAddComponentMenu(AZ_CRC_CE("Game")) can be changed to "System" to
     *   restrict this to the system entity's component menu only.
     *
     * BehaviorContext - Exposes this component to Lua scripting and Script Canvas.
     *   System component functionality exposed here is available globally in scripts.
     */
    void ${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}SystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName}SystemComponent", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName} Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "${GemName} Gem Group")
                ;
        }
    }

    /*
     * GetProvidedServices declares the named service this system component provides.
     * Gem-level system components often represent a subsystem (e.g. "AudioService",
     * "PhysicsService") that other components can declare dependencies on.
     */
    void ${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}SystemComponentService"));
    }

    /*
     * GetIncompatibleServices prevents a second instance of this system component from
     * being added to the system entity. System components should almost always declare
     * themselves incompatible with their own service to enforce singleton behavior.
     * Uncomment the line below to enable this guard.
     */
    void ${SanitizedCppName}SystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Remove line to enforce allow multiple instances of this component on the system entity.
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}SystemComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be active before this system component
     * activates. The system entity activation order is determined by these declarations.
     * If a required service is missing, the system emits an error and skips this component.
     */
    void ${SanitizedCppName}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists services that this component should activate after,
     * if they are present. Unlike required services, a missing dependent service does
     * not prevent activation - it only affects ordering when the service is available.
     */
    void ${SanitizedCppName}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
