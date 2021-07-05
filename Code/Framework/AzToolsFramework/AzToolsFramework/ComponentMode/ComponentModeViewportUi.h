/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ComponentModeViewportUiRequestBus.h"

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
         //! Handles adding Viewport UI widgets from a specific ComponentMode onto the render viewport UI.
        class ComponentModeViewportUi
            : public ComponentModeViewportUiRequestBus::Handler
        {
        public:
            explicit ComponentModeViewportUi(AZ::Uuid uuid);
            ComponentModeViewportUi(ComponentModeViewportUi&&) = default;
            ComponentModeViewportUi& operator=(ComponentModeViewportUi&&) = default;
            ~ComponentModeViewportUi();

            //! ComponentModeViewportUiRequestBus ...
            void RegisterViewportElementGroup(
                const AZ::EntityComponentIdPair& entityComponentId,
                const AZStd::vector<ViewportUi::ClusterId>& clusterIds) override;
            void SetComponentModeViewportUiActive(bool active) override;
            void SetViewportUiActiveEntityComponentId(const AZ::EntityComponentIdPair& entityComponentId) override;

            //! The underlying Component type for this ComponentMode.
            AZ::Uuid GetComponentType() const { return m_componentType; }

        private:
            //! Show all active widgets to display on the viewport.
            void ShowActiveViewportElements();

            //! Hide all active widgets displaying on the viewport.
            void HideActiveViewportElements();

            using EntityComponentWidgetMapping = AZStd::unordered_map<AZ::EntityComponentIdPair, AZStd::vector<ViewportUi::ClusterId>>;

            EntityComponentWidgetMapping m_entityComponentWidgetMap; //!< Map of all EntityComponentIdPairs to their respective widgets.
            AZ::EntityComponentIdPair m_activeEntityComponentId; //!< The current active entityComponentId to display on the viewport.
            AZStd::vector<ViewportUi::ClusterId> m_activeViewportIds; //!< Ids of all active viewport widgets displaying on the viewport.
            bool m_active = false; //!< Whether this ComponentMode is active and should display on the viewport.
            AZ::Uuid m_componentType; //!< The component type that this handler is connected to.
        };
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
