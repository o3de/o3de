/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelection.h>

#include <QWidget>

namespace AzToolsFramework
{
    class ViewportEditorModeTrackerInterface;

    //! The default selection/input handler for the editor (includes handling ComponentMode).
    class EditorDefaultSelection
        : public ViewportInteraction::InternalViewportSelectionRequests
        , private ActionOverrideRequestBus::Handler
        , private ComponentModeFramework::ComponentModeSystemRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        //! @cond
        EditorDefaultSelection(const EditorVisibleEntityDataCache* entityDataCache, ViewportEditorModeTrackerInterface* viewportEditorModeTracker);
        EditorDefaultSelection(const EditorDefaultSelection&) = delete;
        EditorDefaultSelection& operator=(const EditorDefaultSelection&) = delete;
        virtual ~EditorDefaultSelection();
        //! @endcond

        //! Override the default widget used to store QActions while in ComponentMode.
        //! @note This should not be necessary during normal operation and is provided
        //! as a customization point to aid with testing.
        void SetOverridePhantomWidget(QWidget* phantomOverrideWidget);

    private:
        // ViewportInteraction::InternalMouseViewportRequests ...
        bool InternalHandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        bool InternalHandleMouseManipulatorInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction) override;
        void DisplayViewportSelection(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
        void DisplayViewportSelection2d(
            const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // ActionOverrideRequestBus ...
        void SetupActionOverrideHandler(QWidget* parent) override;
        void TeardownActionOverrideHandler() override;
        void AddActionOverride(const ActionOverride& actionOverride) override;
        void RemoveActionOverride(AZ::Crc32 actionOverrideUri) override;
        void ClearActionOverrides() override;

        // ComponentModeSystemRequestBus ...
        void BeginComponentMode(
            const AZStd::vector<ComponentModeFramework::EntityAndComponentModeBuilders>& entityAndComponentModeBuilders) override;
        void AddComponentModes(const ComponentModeFramework::EntityAndComponentModeBuilders& entityAndComponentModeBuilders) override;
        void EndComponentMode() override;
        bool InComponentMode() override
        {
            return m_componentModeCollection.InComponentMode();
        }
        void Refresh(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
        bool AddedToComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid& componentType) override;
        void AddSelectedComponentModesOfType(const AZ::Uuid& componentType) override;
        bool SelectNextActiveComponentMode() override;
        bool SelectPreviousActiveComponentMode() override;
        bool SelectActiveComponentMode(const AZ::Uuid& componentType) override;
        AZ::Uuid ActiveComponentMode() override;
        bool ComponentModeInstantiated(const AZ::EntityComponentIdPair& entityComponentIdPair) override;
        bool HasMultipleComponentTypes() override;
        void RefreshActions() override;

        //! Helpers to deal with moving in and out of ComponentMode.
        void TransitionToComponentMode();
        void TransitionFromComponentMode();

        //! Accessor used internally to refer to the phantom widget.
        //! This will either be the default widget or the override if non-null.
        QWidget& PhantomWidget();

        QWidget m_phantomWidget; //!< The phantom widget responsible for holding QActions while in ComponentMode.
        QWidget* m_phantomOverrideWidget = nullptr; //!< It's possible to override the phantom widget in special circumstances (eg testing).
        ComponentModeFramework::ComponentModeCollection m_componentModeCollection; //!< Handles all active ComponentMode types.
        AZStd::unique_ptr<EditorTransformComponentSelection> m_transformComponentSelection =
            nullptr; //!< Viewport selection (responsible for
                     //!< manipulators and transform modifications).
        const EditorVisibleEntityDataCache* m_entityDataCache = nullptr; //!< Reference to cached visible EntityData.

        //! Mapping between passed ActionOverride (AddActionOverride) and allocated QAction.
        struct ActionOverrideMapping
        {
            ActionOverrideMapping(
                const AZ::Crc32 uri, const AZStd::vector<AZStd::function<void()>>& callbacks, AZStd::unique_ptr<QAction> action)
                : m_uri(uri)
                , m_callbacks(callbacks)
                , m_action(AZStd::move(action))
            {
            }

            AZ::Crc32 m_uri; //!< Unique identifier for the Action. (In the form 'com.o3de.action.---").
            AZStd::vector<AZStd::function<void()>> m_callbacks; //!< Callbacks associated with this Action (note: with multi-selections
                                                                //!< there will be a callback per Entity/Component).
            AZStd::unique_ptr<QAction> m_action; //!< The QAction associated with the overrideWidget for all ComponentMode actions.
        };

        AZStd::vector<AZStd::shared_ptr<ActionOverrideMapping>> m_actions; //!< Currently bound actions (corresponding to those set
                                                                           //!< on the override widget).

        AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager; //!< The default manipulator manager.
        ViewportInteraction::MouseInteraction m_currentInteraction; //!< Current mouse interaction to be used for drawing manipulators.
        ViewportEditorModeTrackerInterface* m_viewportEditorModeTracker = nullptr; //!< Tracker for activating/deactivating viewport editor modes.

    };
} // namespace AzToolsFramework
