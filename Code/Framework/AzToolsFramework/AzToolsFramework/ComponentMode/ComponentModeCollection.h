/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/ComponentMode/ComponentModeViewportUi.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>

namespace AzToolsFramework
{
    class EditorMetricsEventsBusTraits;
    class ViewportEditorModeTrackerInterface;

    namespace ComponentModeFramework
    {
        /// Manages all individual ComponentModes for a single instance of Editor wide ComponentMode.
        class ComponentModeCollection
            : public ComponentModeCollectionInterface
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            /// @cond
            explicit ComponentModeCollection(ViewportEditorModeTrackerInterface* viewportEditorModeTracker);
            ~ComponentModeCollection() = default;
            ComponentModeCollection(const ComponentModeCollection&) = delete;
            ComponentModeCollection& operator=(const ComponentModeCollection&) = delete;
            /// @endcond

            /// Add a ComponentMode for a given Component type on the EntityId specified.
            /// An \ref AZ::EntityComponentIdPair is provided so the individual Component on
            /// the Entity can be addressed if there are more than one of the same type.
            void AddComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType,
                const ComponentModeFactoryFunction& componentModeBuilder);

            /// Refresh (update) all active ComponentModes for the specified Entity and Component Id pair.
            void Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair);

            /// Begin Editor-wide ComponentMode.
            /// Notify other systems a ComponentMode is starting and transition the Editor to the correct state.
            void BeginComponentMode();
            /// End Editor-wide ComponentMode
            /// Leave all active ComponentModes and move the Editor back to its normal state.
            void EndComponentMode();

            /// Ensure entire Entity selection is moved to ComponentMode.
            /// For all other selected entities, if they have a matching component that has just entered
            /// ComponentMode, add them too (duplicates will not be added - handled by AddComponentMode)
            void AddOtherSelectedEntityModes();

            /// Return if the Editor-wide ComponentMode state is active.
            bool InComponentMode() const { return m_componentModeState == ComponentModeState::Active; }
            /// Are ComponentModes in the process of being added.
            /// Used to determine if other selected entities with the same Component type should also be added.
            bool ModesAdded() const { return m_adding; }

            /// Return if this Entity and its Component are currently in ComponentMode.
            bool AddedToComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType);

            /// Move to the next active ComponentMode so the Actions for that mode become available (it is now 'selected').
            bool SelectNextActiveComponentMode();

            /// Move to the previous active ComponentMode so the Actions for that mode become available (it is now 'selected').
            bool SelectPreviousActiveComponentMode();

            /// Pick a specific ComponentMode for a Component (by directly selecting a Component in the EntityInspector - is it now 'selected').
            bool SelectActiveComponentMode(const AZ::Uuid& componentType);

            /// Return the Uuid of the Component Type that is currently active in Component Mode.
            AZ::Uuid ActiveComponentMode() const;

            /// Return if the ComponentMode for this specific Entity/Component pair is instantiated.
            bool ComponentModeInstantiated(const AZ::EntityComponentIdPair& entityComponentIdPair) const;

            /// Return if there is more than one Component type in Component Mode.
            bool HasMultipleComponentTypes() const;

            /// Refresh Actions (shortcuts) for the 'selected' ComponentMode.
            void RefreshActions();

            /// Populate Viewport UI elements to use for this ComponentMode.
            /// Called once each time a ComponentMode is added.
            void PopulateViewportUi();

            // ComponentModeCollectionInterface overrides ...
            AZStd::vector<AZ::Uuid> GetComponentTypes() const override;

        private:
            enum class ComponentModeState : uint8_t
            {
                Active,
                Stopping,
                Stopped
            };

            // Internal helper used by Select[|Prev|Next]ActiveComponentMode
            bool ActiveComponentModeChanged(const AZ::Uuid& previousComponentType);

            AZStd::vector<AZ::Uuid> m_activeComponentTypes; ///< What types of ComponentMode are currently active.
            AZStd::vector<ComponentModeViewportUi> m_viewportUiHandlers; ///< Viewport UI handlers for each ComponentMode.
            AZStd::vector<EntityAndComponentMode> m_entitiesAndComponentModes; ///< The active ComponentModes (one per Entity).
            AZStd::vector<EntityAndComponentModeBuilders> m_entitiesAndComponentModeBuilders; ///< Factory functions to re-create specific modes
                                                                                              ///< tied to a particular Entity (for undo/redo).

            size_t m_selectedComponentModeIndex = 0; ///< Index into the array of active ComponentModes, current index is 'selected' ComponentMode.
            bool m_adding = false; ///< Are we currently adding individual ComponentModes to the Editor wide ComponentMode.
            /// Editor (global) ComponentMode state - is ComponentMode active or not.
            ComponentModeState m_componentModeState = ComponentModeState::Stopped; 
            ViewportEditorModeTrackerInterface* m_viewportEditorModeTracker = nullptr; //!< Tracker for activating/deactivating viewport editor modes.
        };
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
