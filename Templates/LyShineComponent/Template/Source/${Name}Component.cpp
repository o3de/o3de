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
     * this UI component. The UUID must be unique across the entire project.
     */
    AZ_COMPONENT_IMPL(${SanitizedCppName}Component, "${SanitizedCppName}Component", "{${Random_Uuid}}");

    /*
     * Activate is called when the UI canvas element entity is activated.
     * This happens when the canvas is loaded, but NOT yet fully initialized - other
     * components on the canvas may not be ready to receive events yet at this point.
     *
     * Connect to the request bus here to receive incoming requests from other systems.
     * Also connect to UiInitializationBus so the engine calls InGamePostActivate() once
     * the entire canvas has finished activating. This is the safe point to begin
     * sending events to other canvas elements and components.
     */
    void ${SanitizedCppName}Component::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect(GetEntityId());
        UiInitializationBus::Handler::BusConnect(GetEntityId());
    }

    /*
     * InGamePostActivate is called by LyShine after the entire UI canvas has been
     * activated and all element components are ready. This is the correct place to:
     *   - Send initial events or state to other canvas elements.
     *   - Query other components on the canvas (e.g. UiTextBus, UiImageBus).
     *   - Start animations or set initial visibility states.
     *
     * After InGamePostActivate returns, disconnect from UiInitializationBus because
     * this callback is only needed once per canvas load. Remaining connected would
     * cause InGamePostActivate to be called again if the canvas is re-activated.
     */
    void ${SanitizedCppName}Component::InGamePostActivate()
    {
        UiInitializationBus::Handler::BusDisconnect();
    }

    /*
     * Deactivate is called when the canvas is unloaded or the element is destroyed.
     * Disconnect from all buses connected in Activate(). Note that UiInitializationBus
     * is disconnected in InGamePostActivate(), so only the request bus needs cleanup here
     * (unless InGamePostActivate was never called, in which case it is already safe to
     * call BusDisconnect on an already-disconnected handler).
     */
    void ${SanitizedCppName}Component::Deactivate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect(GetEntityId());
    }

    /*
     * Reflect registers this UI component with O3DE's reflection contexts.
     *
     * SerializeContext - Persists component data in the .uicanvas file.
     *   Use ->Field() for each member variable to be saved with the canvas.
     *   Bump Version when the serialized layout changes.
     *
     * EditContext - Controls how this component appears in the UI Editor Inspector.
     *   AppearsInAddComponentMenu(AZ_CRC_CE("UI")) restricts this component to the
     *   UI canvas editor and prevents it from appearing on game entities.
     *   Use ->DataElement() to expose fields for editing in the UI Editor.
     *
     * BehaviorContext - Exposes this UI component to Lua scripting.
     *   Script Canvas is rarely used with UI components; Lua is more common in UI.
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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
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
     * GetProvidedServices declares the named service this UI component provides.
     * Other UI components on the same element that depend on this service will
     * activate after this component.
     */
    void ${SanitizedCppName}Component::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}ComponentService"));
    }

    /*
     * GetIncompatibleServices prevents duplicate instances of this component on the
     * same UI element. Uncomment the line below to enforce a single-instance constraint.
     */
    void ${SanitizedCppName}Component::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        // Uncomment to prevent more than one instance of this component per UI element:
        // incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}ComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be present on the UI element before
     * this component activates. A missing required service causes an activation error.
     * UI components commonly require "UiTransformService" from UiTransform2dComponent.
     */
    void ${SanitizedCppName}Component::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists optional ordering dependencies. This component activates
     * after these services if present but does not fail if they are absent.
     */
    void ${SanitizedCppName}Component::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
