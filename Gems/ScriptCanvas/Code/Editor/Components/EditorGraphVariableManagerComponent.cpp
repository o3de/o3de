/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <qstring.h>

#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

namespace ScriptCanvasEditor
{
    /////////////////////////////////
    // EditorGraphVariableItemModel
    /////////////////////////////////

    void EditorGraphVariableItemModel::Activate(const ScriptCanvas::ScriptCanvasId& busId)
    {
        m_busId = busId;

        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::GraphVariableManagerNotificationBus::Handler::BusConnect(m_busId);

        removeRows(0, static_cast<int>(m_variableIds.size()));
        m_variableIds.clear();

        const ScriptCanvas::GraphVariableMapping* variableMapping = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableMapping, m_busId, &ScriptCanvas::GraphVariableManagerRequests::GetVariables);
        if (variableMapping)
        {
            beginInsertRows(QModelIndex(), 0, static_cast<int>(variableMapping->size()));

            m_variableIds.reserve(variableMapping->size());

            for (const auto& mapPair : (*variableMapping))
            {
                m_variableIds.emplace_back(mapPair.first);
            }

            endInsertRows();
        }
    }

    ScriptCanvas::VariableId EditorGraphVariableItemModel::FindVariableIdForIndex(const QModelIndex& modelIndex) const
    {
        ScriptCanvas::VariableId variableId;

        if (modelIndex.row() >= 0 && modelIndex.row() < m_variableIds.size())
        {
            variableId = m_variableIds[modelIndex.row()];
        }

        return variableId;
    }

    QModelIndex EditorGraphVariableItemModel::index(int row, int column, [[maybe_unused]] const QModelIndex& parent) const
    {
        return createIndex(row, column);
    }

    QModelIndex EditorGraphVariableItemModel::parent(const QModelIndex&) const
    {
        return QModelIndex();
    }

    int EditorGraphVariableItemModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return 1;
    }

    int EditorGraphVariableItemModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return static_cast<int>(m_variableIds.size());
    }

    QVariant EditorGraphVariableItemModel::data(const QModelIndex& index, int role) const
    {
        ScriptCanvas::VariableId variableId = FindVariableIdForIndex(index);

        if (!variableId.IsValid())
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole
            || role == Qt::EditRole)
        {
            AZStd::string variableName;
            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, m_busId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, variableId);
            return QVariant(variableName.c_str());
        }

        return QVariant();
    }

    void EditorGraphVariableItemModel::OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_variableIds.emplace_back(variableId);
        endInsertRows();
    }

    void EditorGraphVariableItemModel::OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view /*variableName*/)
    {
        int index = -1;

        for (unsigned int i = 0; i < m_variableIds.size(); ++i)
        {
            if (m_variableIds[i] == variableId)
            {
                index = i;
                break;
            }
        }

        if (index >= 0)
        {
            beginRemoveRows(QModelIndex(), index, index);
            m_variableIds.erase(m_variableIds.begin() + index);
            endRemoveRows();
        }
        else
        {
            AZ_Error("Script Canvas", false, "Failed to find index which contains variable id %s. This indicates that the GraphVariableManagerNotification::OnVariableRemoved function"
                " was invoked twice for the same variable without it being added back to the GraphVariableManager. This should not occur and likely indicates an issue in the GraphVariableManager",
                variableId.ToString().data());
        }
    }

    ////////////////////////////////////////
    // EditorGraphVariableManagerComponent
    ////////////////////////////////////////
    void EditorGraphVariableManagerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGraphVariableManagerComponent, ScriptCanvas::GraphVariableManagerComponent>()
                ->Version(0)
                ;
        }
    }

    EditorGraphVariableManagerComponent::EditorGraphVariableManagerComponent(ScriptCanvas::ScriptCanvasId graphScopeId)
        : GraphVariableManagerComponent(graphScopeId)
    {
    }

    void EditorGraphVariableManagerComponent::ConfigureScriptCanvasId(const ScriptCanvas::ScriptCanvasId& executionId)
    {
        ScriptCanvas::GraphVariableManagerComponent::ConfigureScriptCanvasId(executionId);

        m_variableModel.Activate(GetScriptCanvasId());

        EditorSceneVariableManagerRequestBus::Handler::BusDisconnect();
        EditorSceneVariableManagerRequestBus::Handler::BusConnect(GetScriptCanvasId());
    }

    QAbstractItemModel* EditorGraphVariableManagerComponent::GetVariableItemModel()
    {
        return &m_variableModel;
    }
}
