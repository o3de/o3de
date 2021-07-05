/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Core/Node.h>

#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>

#include <ScriptCanvas/Core/GraphBus.h>

#include "PropertyGridBus.h"
#endif

class QSpacerItem;
class QScrollArea;

namespace AzToolsFramework
{
    class PropertiesWidget;
    class EntityPropertyEditor;
    namespace UndoSystem
    {
        class URSequencePoint;
    }
    class ComponentEditor;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class PropertyGrid
            : public AzQtComponents::StyledDockWidget
            , public AzToolsFramework::IPropertyEditorNotify
            , public PropertyGridRequestBus::Handler
            , public ScriptCanvas::EndpointNotificationBus::MultiHandler
            , public ScriptCanvas::NodeNotificationsBus::MultiHandler
            , public GraphCanvas::GraphCanvasPropertyInterfaceNotificationBus::MultiHandler
        {
            Q_OBJECT

        public:

            PropertyGrid(QWidget* parent = nullptr, const char* name = "Properties");
            ~PropertyGrid() override;

            // AzToolsFramework::IPropertyEditorNotify
            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* pNode) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode) override;
            void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& point) override;
            /////

            // NodeNotificationsBus
            void OnSlotDisplayTypeChanged(const ScriptCanvas::SlotId& slotId, const ScriptCanvas::Data::Type& slotType) override;
            ////

            // PropertyGridRequestHandler
            void RefreshPropertyGrid() override;
            void RebuildPropertyGrid() override;
            void SetSelection(const AZStd::vector<AZ::EntityId>& selectedEntityIds) override;
            void ClearSelection() override;
            ////

            // GraphCanvasPropertyInterfaceNotificationBus
            void OnPropertyComponentChanged() override;
            ////

            void SealUndoStack() override {};

            void OnNodeUpdate(const AZ::EntityId&);

            struct InstancesToDisplay
            {
                //! This is ONLY used to get the title of the node.
                //! This entity ISN'T necessarily the owner of m_gcInstances and m_scInstances.
                AZ::EntityId m_gcEntityId;

                AZStd::list<AZ::Component*> m_gcInstances;
                AZStd::list<AZ::Component*> m_scInstances;
            };

            void DisableGrid();
            void EnableGrid();

        private slots:
            void UpdateContents(const AZStd::vector<AZ::EntityId>& selectedEntityIds);

            // ScriptCanvas::EndpointNotificationBus::MultiHandler
            void OnEndpointConnected(const ScriptCanvas::Endpoint& targetEndpoint) override;
            void OnEndpointDisconnected(const ScriptCanvas::Endpoint& targetEndpoint) override;

            void OnEndpointConvertedToValue() override;
            void OnEndpointConvertedToReference() override;
            ////////////////////////////

            void UpdateEndpointVisibility(const ScriptCanvas::Endpoint& endpoint);

        private:

            void SignalUndo(AzToolsFramework::InstanceDataNode* pNode);

            void SetVisibility(const AZStd::vector<AZ::EntityId>& selectedEntityIds);
            void DisplayInstances(const InstancesToDisplay& instances);
            ScriptCanvas::ScriptCanvasId GetScriptCanvasId(AZ::Component* component);

            AzToolsFramework::ComponentEditor* CreateComponentEditor();

            AZStd::vector<AZStd::unique_ptr<AzToolsFramework::ComponentEditor> > m_componentEditors;

            // the spacer's job is to make sure that its always at the end of the list of components.
            QSpacerItem* m_spacer = nullptr;

            QScrollArea* m_scrollArea = nullptr;
            QWidget* m_host = nullptr;

            bool m_propertyModified = false;
        };
    }
}
