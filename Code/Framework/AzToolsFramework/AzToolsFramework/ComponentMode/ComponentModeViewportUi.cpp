/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeViewportUi.h"

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <QString>
#include <QLabel>
#include <QWidget>
#include <QLayout>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        ComponentModeViewportUi::ComponentModeViewportUi(AZ::Uuid uuid)
            : m_componentType(uuid)
        {
            ComponentModeViewportUiRequestBus::Handler::BusConnect(uuid);
        }

        ComponentModeViewportUi::~ComponentModeViewportUi()
        {
            ComponentModeViewportUiRequestBus::Handler::BusDisconnect();
        }

        void ComponentModeViewportUi::RegisterViewportElementGroup(
            const AZ::EntityComponentIdPair& entityComponentId,
            const AZStd::vector<ViewportUi::ClusterId>& clusterIds)
        {
            if (clusterIds.empty())
            {
                return;
            }

            // check if a widget is already registered with the entity component id pair
            // and that the registered widget has not been deleted
            if (auto widgetMapEntry = m_entityComponentWidgetMap.find(entityComponentId);
                widgetMapEntry != m_entityComponentWidgetMap.end()
                && !widgetMapEntry->second.empty())
            {
                return;
            }

            m_entityComponentWidgetMap.insert({ entityComponentId, clusterIds });
        }

        void ComponentModeViewportUi::ShowActiveViewportElements()
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetClusterGroupVisible,
                m_activeViewportIds, true);
        }

        void ComponentModeViewportUi::HideActiveViewportElements()
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId,
                &ViewportUi::ViewportUiRequestBus::Events::SetClusterGroupVisible,
                m_activeViewportIds, false);
        }

        void ComponentModeViewportUi::SetComponentModeViewportUiActive(bool active)
        {
            // hide all the elements when deactivating
            if (!active)
            {
                if (!m_active)
                {
                    return;
                }

                HideActiveViewportElements();
                m_active = active;
                return;
            }
            
            // broadcast to all other component mode handlers that they should not be active
            // as only one can be active at a time
            ComponentModeViewportUiRequestBus::Broadcast(
                &ComponentModeViewportUiRequestBus::Events::SetComponentModeViewportUiActive, false);

            // once all other handlers are inactive, activate just this handler
            m_active = active;

            if (const auto activeViewportIds = m_entityComponentWidgetMap.find(m_activeEntityComponentId);
                activeViewportIds != m_entityComponentWidgetMap.end())
            {
                // set all of the newly active viewport ids to visible
                m_activeViewportIds = activeViewportIds->second;
                ShowActiveViewportElements();
            }
        }

        void ComponentModeViewportUi::SetViewportUiActiveEntityComponentId(
            const AZ::EntityComponentIdPair& entityComponentId)
        {
            if (m_activeEntityComponentId != entityComponentId)
            {
                HideActiveViewportElements();
            }

            m_activeEntityComponentId = entityComponentId;
            SetComponentModeViewportUiActive(true);
        }
        
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
