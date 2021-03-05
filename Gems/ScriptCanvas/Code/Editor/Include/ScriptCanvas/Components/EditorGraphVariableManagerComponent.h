/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        AZ_CLASS_ALLOCATOR(EditorGraphVariableItemModel, AZ::SystemAllocator, 0);
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
            provided.push_back(AZ_CRC("EditorScriptCanvasVariableService", 0x3df3d54e));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("EditorScriptCanvasVariableService", 0x3df3d54e));
        }

    private:

        EditorGraphVariableManagerComponent(const EditorGraphVariableManagerComponent&) = delete;
        EditorGraphVariableManagerComponent& operator=(const EditorGraphVariableManagerComponent&) = delete;

        EditorGraphVariableItemModel m_variableModel;
    };
}