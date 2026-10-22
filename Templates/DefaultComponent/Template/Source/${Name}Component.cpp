// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}Component.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    /*
     * AZ_COMPONENT_IMPL provides the static type information and UUID registration for
     * this component. The UUID must be unique across the entire project. It is used by
     * the serialization system to identify this component type when saving and loading
     * entity data, and by the editor to locate the component descriptor.
     */
    AZ_COMPONENT_IMPL(${SanitizedCppName}Component, "${SanitizedCppName}Component", "{${Random_Uuid}}");

    /*
     * Activate is called by the entity system when the owning entity is activated.
     * All services this component depends on are guaranteed to be active before this
     * function is called.
     *
     * Connect to the request bus here so that other components and systems can send
     * messages to this component via ${SanitizedCppName}RequestBus::Event(entityId, ...).
     * Use GetEntityId() to scope the bus connection to this entity only.
     */
    void ${SanitizedCppName}Component::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect(GetEntityId());
    }

    /*
     * Deactivate is called by the entity system when the owning entity is deactivated.
     * This is called before the components this one depends on are deactivated, so the
     * reverse-activation order is guaranteed.
     *
     * Always disconnect from any buses you connected to in Activate(). Failing to do
     * so will cause dangling bus connections and likely crashes when the bus fires after
     * the component has been destroyed.
     *
     * Note that an entity can be deactivated and reactivated without being destroyed.
     * This function must leave the component in a state where Activate() can be called
     * again cleanly.
     */
    void ${SanitizedCppName}Component::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect(GetEntityId());
    }

    /*
     * Reflect registers this component type with O3DE's reflection contexts so that the
     * engine can serialize, deserialize, display, and script this component.
     *
     * SerializeContext - Required for saving and loading component data to/from files.
     *   Register every member variable you want persisted using ->Field("name", &Class::member).
     *   Bump the Version number when you make breaking changes to the serialized layout so
     *   that the VersionConverter can migrate old data.
     *
     * EditContext - Controls how this component appears in the Editor Inspector panel.
     *   The Category attribute places it in the Add Component menu.
     *   The Icon attribute controls the icon shown in the Inspector and Viewport.
     *   AppearsInAddComponentMenu(AZ_CRC_CE("Game")) makes it available on game entities.
     *   Use ->DataElement() to expose individual fields in the Inspector.
     *
     * BehaviorContext - Exposes this component to Lua scripting and Script Canvas.
     *   Register methods and properties you want scriptable using ->Method() and ->Property().
     *   The Category attribute groups the node in the Script Canvas node palette.
     */
    void ${SanitizedCppName}Component::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}Component, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<${SanitizedCppName}Component>("${SanitizedCppName}Component", "[Description of functionality provided by this component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "ComponentCategory")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<${SanitizedCppName}Component>("${SanitizedCppName} Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "${GemName} Gem Group")
                ;
        }
    }

    /*
     * GetProvidedServices declares the named services this component offers to the entity system.
     * Other components that list one of these names in their GetRequiredServices() or
     * GetDependentServices() will be activated after this component.
     *
     * The AZ_CRC_CE macro generates a compile-time CRC32 from the string. The service name
     * should be unique - prefix it with your component name to avoid collisions.
     */
    void ${SanitizedCppName}Component::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}ComponentService"));
    }

    /*
     * GetIncompatibleServices lists services that cannot coexist on the same entity as this
     * component. If an entity already has a component providing one of these services, this
     * component cannot be added, and vice versa.
     *
     * To prevent duplicate instances of this component on the same entity, uncomment the line
     * below. This is a common pattern for components that manage a singleton-style resource
     * per entity (e.g. a physics body, a mesh renderer).
     */
    void ${SanitizedCppName}Component::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Uncomment to prevent more than one instance of this component per entity:
        // incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}ComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be present and active on the same entity
     * before this component can activate. If any required service is missing, the entity
     * system will emit an error and skip activating this component.
     *
     * Example - require a transform component to be present:
     *   required.push_back(AZ_CRC_CE("TransformService"));
     */
    void ${SanitizedCppName}Component::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists services that this component should be activated after,
     * if they are present, but does not require them. If a dependent service is missing
     * the component still activates - unlike GetRequiredServices which causes an error.
     *
     * Use this for optional ordering relationships, such as activating after an audio
     * system component if one happens to exist on the same entity.
     */
    void ${SanitizedCppName}Component::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
