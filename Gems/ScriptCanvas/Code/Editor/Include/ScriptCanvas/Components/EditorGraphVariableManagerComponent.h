/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qabstractitemmodel.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>

#include <Editor/Include/ScriptCanvas/Bus/EditorSceneVariableManagerBus.h>

namespace ScriptCanvasEditor
{
    class EditorGraphVariableItemModel
        : public QAbstractItemModel
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorGraphVariableItemModel, AZ::SystemAllocator);
        EditorGraphVariableItemModel() = default;

        void Activate(const ScriptCanvas::ScriptCanvasId& executionId);

        ScriptCanvas::VariableId FindVariableIdForIndex(const QModelIndex& modelIndex) const;

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;

        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        ////

        // SceneVariableManagerNotificationBus
        void OnVariableAddedToGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        void OnVariableRemovedFromGraph(const ScriptCanvas::VariableId& variableId, AZStd::string_view variableName) override;
        ////

    private:

        EditorGraphVariableItemModel(const EditorGraphVariableItemModel&) = delete;
        EditorGraphVariableItemModel& operator=(const EditorGraphVariableItemModel&) = delete;

        AZStd::vector< ScriptCanvas::VariableId > m_variableIds;
        ScriptCanvas::ScriptCanvasId m_busId;
    };

    // Editor version of Variable Component which prevents multiple of them being on the same Entity
    class EditorGraphVariableManagerComponent
        : public ScriptCanvas::GraphVariableManagerComponent
        , public ScriptCanvas::GraphVariableManagerNotificationBus::Handler
        , public EditorSceneVariableManagerRequestBus::Handler        
    {
    public:
        AZ_COMPONENT(EditorGraphVariableManagerComponent, "{86B7CC96-9830-4BD1-85C3-0C0BD0BFBEE7}", ScriptCanvas::GraphVariableManagerComponent);

        static void Reflect(AZ::ReflectContext* context);

        EditorGraphVariableManagerComponent() = default;
        EditorGraphVariableManagerComponent(ScriptCanvas::ScriptCanvasId scriptCanvasId);
        ~EditorGraphVariableManagerComponent() override = default;

        // GraphConfigurationNotificationBus
        void ConfigureScriptCanvasId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) override;
        ////

        // EditorSceneVariableManagerRequestBus
        QAbstractItemModel* GetVariableItemModel() override;
        ////

    protected:
        // prevents a second VariableComponent from being attached to the Entity in the Editor

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            ScriptCanvas::GraphVariableManagerComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC_CE("EditorScriptCanvasVariableService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EditorScriptCanvasVariableService"));
        }

    private:

        EditorGraphVariableManagerComponent(const EditorGraphVariableManagerComponent&) = delete;
        EditorGraphVariableManagerComponent& operator=(const EditorGraphVariableManagerComponent&) = delete;

        EditorGraphVariableItemModel m_variableModel;
    };
}
