/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AzToolsFramework
{
    /// Encompasses all types responsible for providing ComponentMode.
    /// ComponentModeFramework comprises a set of types responsible for providing hooks
    /// to edit Components in the Viewport. It is also responsible for handling how
    /// the Editor transitions in and out of ComponentMode.
    namespace ComponentModeFramework
    {
        /// Foundational interface for any Component wishing to implement ComponentMode.
        /// Components wishing to implement ComponentMode should inherit from
        /// EditorBaseComponentMode, not ComponentMode directly.
        class ComponentMode
        {
        public:
            /// @cond
            ComponentMode() = default;
            ComponentMode(ComponentMode&) = delete;
            ComponentMode& operator=(ComponentMode&) = delete;
            ComponentMode(ComponentMode&&) = default;
            ComponentMode& operator=(ComponentMode&&) = default;
            virtual ~ComponentMode() = default;
            /// @endcond

            /// The type of the underlying Component this mode is for.
            virtual AZ::Uuid GetComponentType() const = 0;

            /// The type of the Component Mode.
            virtual AZ::Uuid GetComponentModeType() const = 0;

            /// The Id of the underlying Component this mode is associated with.
            virtual AZ::ComponentId GetComponentId() const = 0;

            /// Notify ComponentMode that something external to it has
            /// changed and its state should be updated (Manipulators etc).
            virtual void Refresh() = 0;

            /// Notify the ActionManager which actions should be enabled for this Mode.
            virtual AZStd::vector<ActionOverride> PopulateActions() = 0;

            /// Add UI elements for this ComponentMode onto the Viewport UI system. 
            virtual AZStd::vector<ViewportUi::ClusterId> PopulateViewportUi() = 0;

            /// The name for the ComponentMode to be displayed.
            virtual AZStd::string GetComponentModeName() const = 0;
        };

        /// Alias for builder/factory function that is responsible for creating a new ComponentMode.
        using ComponentModeFactoryFunction = AZStd::function<AZStd::unique_ptr<ComponentMode>()>;

        /// Holds a function object to create a ComponentMode for a specific type.
        struct ComponentModeBuilder
        {
            AZ::ComponentId m_componentId; ///< The unique Id of the underlying Component.
            AZ::Uuid m_componentType; ///< The type of the underlying Component.
            ComponentModeFactoryFunction m_componentModeBuilder; ///< Factory function to create a specific ComponentMode.

            /// Constructor to bind a Component type to a concrete ComponentMode.
            ComponentModeBuilder(
                const AZ::ComponentId componentId,
                const AZ::Uuid componentType,
                const ComponentModeFactoryFunction& componentModeBuilder)
                : m_componentId(componentId)
                , m_componentType(componentType)
                , m_componentModeBuilder(componentModeBuilder) {}
        };

        /// Encapsulates an Entity and its active ComponentMode.
        struct EntityAndComponentMode
        {
            AZ::EntityId m_entityId; ///< The Entity Id associated with this ComponentMode.
            AZStd::unique_ptr<ComponentMode> m_componentMode; ///< The ComponentMode currently active for this Entity.

            /// Constructor to bind an EntityId to a ComponentMode.
            EntityAndComponentMode(
                AZ::EntityId entityId,
                AZStd::unique_ptr<ComponentMode> componentMode)
                : m_entityId(entityId)
                , m_componentMode(AZStd::move(componentMode)) {}
        };

        /// Encapsulates a series of ComponentModeBuilders with a single Entity.
        struct EntityAndComponentModeBuilders
        {
            AZ::EntityId m_entityId; ///< The Entity Id associated with this ComponentModeBuilders(s).
            AZStd::vector<ComponentModeBuilder> m_componentModeBuilders; ///< All ComponentModeBuilders that can create modes for this Entity.

            /// Constructor to bind an EntityId to a number of ComponentModeBuilders.
            EntityAndComponentModeBuilders(
                const AZ::EntityId entityId,
                const AZStd::vector<ComponentModeBuilder>& componentModeBuilders)
                : m_entityId(entityId)
                , m_componentModeBuilders(componentModeBuilders) {}

            /// Constructor to bind an EntityId to a single ComponentModeBuilder.
            EntityAndComponentModeBuilders(
                const AZ::EntityId entityId,
                const ComponentModeBuilder& componentModeBuilder)
                : m_entityId(entityId)
                , m_componentModeBuilders(1, componentModeBuilder) {}
        };

         /// Bus to control the overall editor ComponentMode state.
        class ComponentModeSystemRequests
            : public AZ::EBusTraits
        {
        public:
            /// Move the Editor into ComponentMode with a list of Entities and their individual ComponentModes.
            virtual void BeginComponentMode(
                const AZStd::vector<EntityAndComponentModeBuilders>& entityAndComponentModeBuilders) = 0;

            /// Add an Entity and its ComponentModes to an existing or starting up ComponentMode.
            virtual void AddComponentModes(
                const EntityAndComponentModeBuilders& entityAndComponentModeBuilders) = 0;

            /// One single way to leave ComponentMode, will move Editor out of ComponentMode.
            virtual void EndComponentMode() = 0;

            /// Is the Editor currently in ComponentMode or not.
            virtual bool InComponentMode() = 0;

            /// If something about the Component/Entity has changed, Refresh
            /// can be used to update Manipulator positions etc.
            virtual void Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

            /// Is this Component type on this entity currently participating
            /// in the Editor ComponentMode.
            virtual bool AddedToComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType) = 0;

            /// If the user has a multiple selection where each entity in the selection
            /// has the same Component on it, move all Components into ComponentMode.
            virtual void AddSelectedComponentModesOfType(const AZ::Uuid& componentType) = 0;

            /// Switches to the ComponentMode of input component type immediately.
            virtual void ChangeComponentMode(const AZ::Uuid& componentType) = 0;

            /// Move to the next active ComponentMode so the Actions for that mode
            /// become available (it is now 'selected').
            /// Return true if the mode actually changed - the mode will not change if
            /// the componentType requested is the same as the current one.
            virtual bool SelectNextActiveComponentMode() = 0;

            /// Move to the previous active ComponentMode so the Actions for that mode
            /// become available (it is now 'selected').
            /// Return true if the mode actually changed - the mode will not change if
            /// the componentType requested is the same as the current one.
            virtual bool SelectPreviousActiveComponentMode() = 0;

            /// Pick a specific ComponentMode for a Component (by directly selecting a
            /// Component in the EntityInspector - it is now 'selected').
            /// Return true if the mode actually changed - the mode will not change if
            /// the componentType requested is the same as the current one.
            virtual bool SelectActiveComponentMode(const AZ::Uuid& componentType) = 0;

            /// Return the Uuid of the Component Type that is currently active in Component Mode.
            virtual AZ::Uuid ActiveComponentMode() = 0;

            /// Return if the ComponentMode for this specific Entity/Component pair is instantiated.
            virtual bool ComponentModeInstantiated(const AZ::EntityComponentIdPair& entityComponentIdPair) = 0;

            /// Return if there are more than one Component type in Component Mode.
            /// There may be two dependent Component Modes that are not 'active' at the same time
            /// but can be switched between in a Component Mode session (e.g. Tube and Spline Components).
            virtual bool HasMultipleComponentTypes() = 0;

            /// Refresh Actions (shortcuts) for the 'selected' ComponentMode.
            virtual void RefreshActions() = 0;

        protected:
            ~ComponentModeSystemRequests() = default;
        };

        /// Type to inherit to implement ComponentModeSystemRequests.
        using ComponentModeSystemRequestBus = AZ::EBus<ComponentModeSystemRequests>;

        /// Bus traits for Individual ComponentMode mouse viewport requests.
        class ComponentModeMouseViewportRequests
            : public AZ::EBusTraits
        {
        public:
            using BusIdType = AZ::EntityComponentIdPair;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        protected:
            ~ComponentModeMouseViewportRequests() = default;
        };

        /// Interface to ComponentModeDelegate - used to detect if a user has double
        /// clicked to enter or exit ComponentMode.
        class ComponentModeDelegateRequests
        {
        public:
            /// Return true if a mouse event occurred to enter ComponentMode.
            virtual bool DetectEnterComponentModeInteraction(
                const ViewportInteraction::MouseInteractionEvent& mouseInteraction) = 0;

            /// Return true if a mouse event occurred to leave ComponentMode.
            virtual bool DetectLeaveComponentModeInteraction(
                const ViewportInteraction::MouseInteractionEvent& mouseInteraction) = 0;

            /// Attempt to add a ComponentMode for this Delegate if the Component type matches.
            virtual void AddComponentModeOfType(AZ::Uuid componentType) = 0;

        protected:
            ~ComponentModeDelegateRequests() = default;
        };

        /// Type to inherit to implement ComponentModeDelegateRequests.
        using ComponentModeDelegateRequestBus = AZ::EBus<ComponentModeDelegateRequests, ComponentModeMouseViewportRequests>;

         /// Mouse viewport events to be intercepted by individual ComponentModes.
        class ComponentModeRequests
            : public ViewportInteraction::MouseViewportRequests
            , public ComponentMode
        {
        public:
            /// This function is called internally after a mouse interaction returns true.
            /// Implementers of HandleMouseInteraction should not override this function.
            virtual void PostHandleMouseInteraction() = 0;
        };

        /// Type to inherit to implement ComponentModeRequests.
        using ComponentModeRequestBus = AZ::EBus<ComponentModeRequests, ComponentModeMouseViewportRequests>;

        /// Used to notify other systems when ComponentMode events happen.
        class EditorComponentModeNotifications
            : public AZ::EBusTraits
        {
        public:
            using BusIdType = AzFramework::EntityContextId;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            /// Called when Tab is pressed to cycle the 'selected' ComponentMode (which shortcuts/actions are active).
            /// Also called when directly selecting a Component in the EntityOutliner.
            virtual void ActiveComponentModeChanged(const AZ::Uuid& /*componentType*/) {}

        protected:
            ~EditorComponentModeNotifications() = default;
        };

        /// Type to inherit to implement EditorComponentModeNotifications.
        using EditorComponentModeNotificationBus = AZ::EBus<EditorComponentModeNotifications>;

        /// Helper to answer if the Editor is in ComponentMode or not.
        inline bool InComponentMode()
        {
            bool inComponentMode = false;
            ComponentModeSystemRequestBus::BroadcastResult(
                inComponentMode, &ComponentModeSystemRequests::InComponentMode);

            return inComponentMode;
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN(AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests);
DECLARE_EBUS_EXTERN_WITH_TRAITS(AzToolsFramework::ComponentModeFramework::ComponentModeDelegateRequests, AzToolsFramework::ComponentModeFramework::ComponentModeMouseViewportRequests)
