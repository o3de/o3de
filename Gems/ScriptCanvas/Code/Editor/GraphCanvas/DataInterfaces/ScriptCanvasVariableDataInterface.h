/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QString>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/Connections/ConnectionBus.h>

#include <GraphCanvas/Components/NodePropertyDisplay/ComboBoxDataInterface.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModels.h>
#include <GraphCanvas/Utils/QtMimeUtils.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorSceneVariableManagerBus.h>
#include <Editor/Include/ScriptCanvas/Bus/RequestBus.h>
#include <Editor/Include/ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include "ScriptCanvasDataInterface.h"

#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Utils/DataUtils.h>

namespace ScriptCanvasEditor
{
    class VariableComboBoxDataModel
        : public GraphCanvas::GraphCanvasListComboBoxModel<ScriptCanvas::VariableId>
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public GeneralEditorNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableComboBoxDataModel, AZ::SystemAllocator, 0);

        VariableComboBoxDataModel()
        {
        }

        ~VariableComboBoxDataModel() override
        {
            ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
            GeneralEditorNotificationBus::Handler::BusDisconnect();
        }

        void Activate(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
        {
            m_scriptCanvasId = scriptCanvasId;
            if (m_scriptCanvasId.IsValid())
            {
                GeneralEditorNotificationBus::Handler::BusConnect(m_scriptCanvasId);

                if (!IsInUndo())
                {
                    FinalizeActivation();
                }
            }
        }

        // ScriptCanvas::GraphVariableManagerNotifications
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override
        {
            QString displayName = QString::fromUtf8(variableName.data(), static_cast<int>(variableName.size()));
            
            AddElement(variableId, displayName);
        }

        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, [[maybe_unused]] AZStd::string_view variableName) override
        {
            RemoveElement(variableId);
        }

        void OnVariableNameChangedInGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override
        {
            RemoveElement(variableId);
            OnVariableAddedToGraph(variableId, variableName);
        }
        ////

        // GeneralEditorNotifications
        void OnUndoRedoBegin() override
        {
            ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
        }

        void OnUndoRedoEnd() override
        {
            FinalizeActivation();
        }
        ////

        const ScriptCanvas::GraphVariable* GetGraphVariable(const ScriptCanvas::VariableId& variableId) const
        {
            const ScriptCanvas::GraphVariable* graphVariable = nullptr;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

            return graphVariable;
        }

        const ScriptCanvas::GraphVariable* GetGraphVariableForIndex(const QModelIndex& index) const
        {
            return GetGraphVariable(GetValueForIndex(index));
        }

    private:

        void FinalizeActivation()
        {
            if (m_scriptCanvasId.IsValid())
            {
                ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_scriptCanvasId);

                const ScriptCanvas::GraphVariableMapping* graphVariables = nullptr;
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariables, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);            

                if (graphVariables)
                {
                    ClearElements();

                    for (auto variablePair : (*graphVariables))
                    {
                        OnVariableAddedToGraph(variablePair.first, variablePair.second.GetVariableName());
                    }
                }
            }
        }

        bool IsInUndo() const
        {
            bool isInUndo = false;
            GeneralRequestBus::BroadcastResult(isInUndo, &GeneralRequests::IsScriptCanvasInUndoRedo, m_scriptCanvasId);

            return isInUndo;
        }

        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;
    };

    class VariableTypeComboBoxFilterModel
        : public GraphCanvas::GraphCanvasSortFilterComboBoxProxyModel
    {
    public:
        AZ_CLASS_ALLOCATOR(VariableTypeComboBoxFilterModel, AZ::SystemAllocator, 0);

        VariableTypeComboBoxFilterModel(const VariableComboBoxDataModel* sourceModel, ScriptCanvas::Slot* slot = nullptr)
            : m_sourceModel(sourceModel)
            , m_slotFilter(slot)
        {
            SetModelInterface(const_cast<VariableComboBoxDataModel*>(sourceModel));
        }

        void SetSlotFilter(ScriptCanvas::Slot* slotFilter)
        {
            if (m_slotFilter != slotFilter)
            {
                m_slotFilter = slotFilter;

                if (sourceModel())
                {
                    QSortFilterProxyModel::invalidateFilter();
                }
            }
        }

        void RefreshFilter()
        {
            if (sourceModel())
            {
                QSortFilterProxyModel::invalidateFilter();
            }
        }

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
        {
            if (m_slotFilter == nullptr)
            {
                return true;
            }

            QModelIndex sourceIndex = m_sourceModel->index(sourceRow, VariableComboBoxDataModel::ColumnIndex::Name, sourceParent);
            const ScriptCanvas::GraphVariable* variable = m_sourceModel->GetGraphVariableForIndex(sourceIndex);

            if (variable)
            {
                auto dataType = variable->GetDatum()->GetType();

                return m_slotFilter->GetNode()->IsValidTypeForSlot(m_slotFilter->GetId(), dataType).IsSuccess();
            }

            return false;
        }

        ScriptCanvas::VariableId GetValueForIndex(const QModelIndex& modelIndex) const
        {
            return m_sourceModel->GetValueForIndex(RemapToSourceIndex(modelIndex));
        }

        QModelIndex GetIndexForValue(const ScriptCanvas::VariableId& variableId) const
        {
            return RemapFromSourceIndex(m_sourceModel->GetIndexForValue(variableId));
        }

        const QString& GetDisplayName(const ScriptCanvas::VariableId& variableId) const
        {
            return m_sourceModel->GetNameForValue(variableId);
        }

        const ScriptCanvas::GraphVariable* GetGraphVariable(const ScriptCanvas::VariableId& variableId) const
        {
            return m_sourceModel->GetGraphVariable(variableId);
        }

    private:

        const VariableComboBoxDataModel* m_sourceModel;

        ScriptCanvas::Slot* m_slotFilter;
    };

    class ScriptCanvasGraphScopedVariableDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>
        , public ScriptCanvas::VariableNotificationBus::Handler        
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasGraphScopedVariableDataInterface, AZ::SystemAllocator, 0);

        ScriptCanvasGraphScopedVariableDataInterface(const VariableComboBoxDataModel* variableDataModel, const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId)
            : ScriptCanvasDataInterface(scriptCanvasNodeId, scriptCanvasSlotId)
            , m_scriptCanvasGraphId(scriptCanvasGraphId)
            , m_variableTypeModel(variableDataModel)
        {
            RegisterBus();
        }

        ~ScriptCanvasGraphScopedVariableDataInterface()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
        }

        // SystemTickBus
        void OnSystemTick() override
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AssignIndex(m_variableTypeModel.GetDefaultIndex());
            SignalValueChanged();
        }
        ////

        // VariableNotificationBus
        void OnVariableRenamed([[maybe_unused]] AZStd::string_view newName) override
        {
            SignalValueChanged();
        }

        void OnVariableRemoved() override
        {
            // Delay this since its possible the model hasn't updated yet
            AZ::SystemTickBus::Handler::BusConnect();
        }
        ////

        // ScriptCanvas::NodeNotificationsBus::Handler
        void OnSlotInputChanged(const ScriptCanvas::SlotId& slotId) override
        {
            if (slotId == GetSlotId())
            {
                RegisterBus();
            }

            ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>::OnSlotInputChanged(slotId);
        }
        ////

        // GraphCanvas::ComboBoxModelInterface
        GraphCanvas::ComboBoxItemModelInterface* GetItemInterface() override
        {
            return &m_variableTypeModel;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            if (!index.isValid())
            {
                return;
            }

            ScriptCanvas::VariableId variableId = m_variableTypeModel.GetValueForIndex(index);

            SetVariableId(variableId);
        }

        QModelIndex GetAssignedIndex() const override
        {
            const ScriptCanvas::Datum* datum = GetSlotObject();

            const ScriptCanvas::GraphScopedVariableId* variableId = (datum ? datum->GetAs<ScriptCanvas::GraphScopedVariableId>() : nullptr);

            if (variableId)
            {
                return m_variableTypeModel.GetIndexForValue(variableId->m_identifier);
            }

            return QModelIndex();
        }

        // Returns the string used to display the currently selected value[Used in the non-editable format]
        QString GetDisplayString() const override
        {
            const ScriptCanvas::Datum* datum = GetSlotObject();

            const ScriptCanvas::GraphScopedVariableId* variableId = (datum ? datum->GetAs<ScriptCanvas::GraphScopedVariableId>() : nullptr);

            if (variableId)
            {       
                return m_variableTypeModel.GetDisplayName(variableId->m_identifier);
            }

            return ComboBoxDataInterface::GetDisplayString();
        }

        void SetVariableId(const ScriptCanvas::VariableId& variableId)
        {
            ScriptCanvas::GraphScopedVariableId scopedVariableId;
            scopedVariableId.m_identifier = variableId;

            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            datumView.SetAs<ScriptCanvas::GraphScopedVariableId>(scopedVariableId);

            if (ScriptCanvas::VariableNotificationBus::Handler::BusIsConnected())
            {
                ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
            }

            scopedVariableId.m_scriptCanvasId = GetScriptCanvasId();
            ScriptCanvas::VariableNotificationBus::Handler::BusConnect(scopedVariableId);

            PostUndoPoint();
            PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
        }

        // DataInterfaceOverrides
        bool EnableDropHandling() const override
        {
            return true;
        }

        AZ::Outcome<GraphCanvas::DragDropState> ShouldAcceptMimeData(const QMimeData* mimeData) override
        {
            if (mimeData->hasFormat(GraphCanvas::k_ReferenceMimeType))
            {
                return AZ::Success(GraphCanvas::DragDropState::Valid);
            }

            return AZ::Failure();
        }

        bool HandleMimeData(const QMimeData* mimeData) override
        {
            ScriptCanvas::VariableId variableId = GraphCanvas::QtMimeUtils::ExtractTypeFromMimeData<ScriptCanvas::VariableId>(mimeData, GraphCanvas::k_ReferenceMimeType);
            SetVariableId(variableId);
            return true;
        }
        //

    private:

        void RegisterBus()
        {
            if (ScriptCanvas::VariableNotificationBus::Handler::BusIsConnected())
            {
                ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
            }

            const ScriptCanvas::Datum* datum = GetSlotObject();
            const ScriptCanvas::GraphScopedVariableId* variableId = (datum ? datum->GetAs<ScriptCanvas::GraphScopedVariableId>() : nullptr);

            if (variableId)
            {
                ScriptCanvas::GraphScopedVariableId scopedVariableId = (*variableId);
                scopedVariableId.m_scriptCanvasId = GetScriptCanvasId();

                ScriptCanvas::VariableNotificationBus::Handler::BusConnect(scopedVariableId);
            }
        }

        VariableTypeComboBoxFilterModel m_variableTypeModel;

        AZ::EntityId m_scriptCanvasGraphId;
    };

    class ScriptCanvasVariableReferenceDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>
        , public ScriptCanvas::VariableNotificationBus::Handler
        , public ScriptCanvas::EndpointNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasVariableReferenceDataInterface, AZ::SystemAllocator, 0);

        ScriptCanvasVariableReferenceDataInterface(const VariableComboBoxDataModel* variableDataModel, const AZ::EntityId& scriptCanvasGraphId, const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId)
            : ScriptCanvasDataInterface(scriptCanvasNodeId, scriptCanvasSlotId)
            , m_scriptCanvasGraphId(scriptCanvasGraphId)
            , m_variableTypeModel(variableDataModel)
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                m_variableTypeModel.SetSlotFilter(slot);

                ScriptCanvas::VariableId variableId = slot->GetVariableReference();

                if (variableId.IsValid())
                {
                    ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(m_scriptCanvasGraphId, variableId));
                }
            }

            ScriptCanvas::EndpointNotificationBus::Handler::BusConnect(ScriptCanvas::Endpoint(scriptCanvasNodeId, scriptCanvasSlotId));
        }

        ~ScriptCanvasVariableReferenceDataInterface()
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
            ScriptCanvas::EndpointNotificationBus::Handler::BusDisconnect();
        }

        // SystemTickBus
        void OnSystemTick() override
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            AssignIndex(m_variableTypeModel.GetDefaultIndex());
            SignalValueChanged();
        }
        ////

        // NodeNotificationBus
        void OnSlotDisplayTypeChanged(const ScriptCanvas::SlotId& slotId, [[maybe_unused]] const ScriptCanvas::Data::Type& slotType) override
        {
            if (slotId == GetSlotId())
            {
                m_variableTypeModel.RefreshFilter();
            }
        }

        void OnSlotInputChanged(const ScriptCanvas::SlotId& slotId) override
        {
            if (slotId == GetSlotId())
            {
                RegisterBus();
            }

            ScriptCanvasDataInterface<GraphCanvas::ComboBoxDataInterface>::OnSlotInputChanged(slotId);
        }
        ////

        // VariableNotificationBus
        void OnVariableRenamed([[maybe_unused]] AZStd::string_view newName) override
        {
            SignalValueChanged();
        }

        void OnVariableRemoved() override
        {
            // Delay this since its possible the model hasn't updated yet
            AZ::SystemTickBus::Handler::BusConnect();
        }
        ////

        // EndpointNotificationBus
        void OnEndpointReferenceChanged(const ScriptCanvas::VariableId& variableId) override
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
            ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(GetScriptCanvasId(), variableId));

            SignalValueChanged();
        }

        void OnSlotRecreated() override
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                m_variableTypeModel.SetSlotFilter(slot);
            }
        }
        ////

        // GraphCanvas::ComboBoxModelInterface
        GraphCanvas::ComboBoxItemModelInterface* GetItemInterface() override
        {
            return &m_variableTypeModel;
        }

        void AssignIndex(const QModelIndex& index) override
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                if (slot->IsVariableReference())
                {
                    // This will trigger an ebus call regarding the modification, which will trigger the value changed.
                    ScriptCanvas::VariableId variableId = m_variableTypeModel.GetValueForIndex(index);
                    slot->SetVariableReference(variableId);

                    PostUndoPoint();
                }
            }
        }

        QModelIndex GetAssignedIndex() const override
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                if (slot->IsVariableReference())
                {
                    return m_variableTypeModel.GetIndexForValue(slot->GetVariableReference());
                }
            }

            return QModelIndex();
        }

        // Returns the string used to display the currently selected value[Used in the non-editable format]
        QString GetDisplayString() const override
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                if (slot->IsVariableReference())
                {
                    const ScriptCanvas::GraphVariable* variable = m_variableTypeModel.GetGraphVariable(slot->GetVariableReference());

                    if (variable)
                    {
                        return m_variableTypeModel.GetDisplayName(variable->GetVariableId());
                    }
                }
            }

            return ComboBoxDataInterface::GetDisplayString();
        }

    private:

        void RegisterBus()
        {
            ScriptCanvas::Slot* slot = GetSlot();

            if (slot)
            {
                ScriptCanvas::VariableId variableId = slot->GetVariableReference();

                if (ScriptCanvas::VariableNotificationBus::Handler::BusIsConnected())
                {
                    ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();
                }

                if (variableId.IsValid())
                {
                    ScriptCanvas::VariableNotificationBus::Handler::BusConnect(ScriptCanvas::GraphScopedVariableId(m_scriptCanvasGraphId, variableId));
                }                
            }
        }

        ScriptCanvas::Slot* GetSlot() const
        {
            ScriptCanvas::Slot* slot = nullptr;
            ScriptCanvas::NodeRequestBus::EventResult(slot, GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, GetSlotId());

            return slot;
        }

        VariableTypeComboBoxFilterModel m_variableTypeModel;

        ScriptCanvas::Data::Type m_displayType = ScriptCanvas::Data::Type::Invalid();
        AZ::EntityId m_scriptCanvasGraphId;
    };
}
