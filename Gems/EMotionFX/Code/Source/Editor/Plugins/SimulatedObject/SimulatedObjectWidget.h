/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectActionManager.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/SimulatedObjectBus.h>
#include <Editor/SimulatedObjectModel.h>
#include <MCore/Source/Command.h>
#include <Source/Editor/ObjectEditor.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QTreeView)

namespace EMotionFX
{
    class Actor;
    class ActorInstance;

    class SimulatedObjectWidget
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
        , private EMotionFX::SimulatedObjectRequestBus::Handler
        , private EMotionFX::ActorEditorNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x00861164
        };

        SimulatedObjectWidget();
        ~SimulatedObjectWidget() override;
        SimulatedObjectWidget(const SimulatedObjectWidget&) = delete;
        SimulatedObjectWidget(SimulatedObjectWidget&&) = delete;
        SimulatedObjectWidget& operator=(const SimulatedObjectWidget&) = delete;
        SimulatedObjectWidget& operator=(SimulatedObjectWidget&&) = delete;

        // EMStudioPlugin overrides
        const char* GetName() const override { return "Simulated Object"; }
        uint32 GetClassID() const override { return CLASS_ID; }
        bool GetIsClosable() const override { return true; }
        bool GetIsFloatable() const override { return true; }
        bool GetIsVertical() const override { return false; }
        EMStudioPlugin* Clone() const override { return new SimulatedObjectWidget(); }
        bool Init() override;
        void Reinit();

        void Render(EMotionFX::ActorRenderFlags renderFlags) override;
        void RenderJointRadius(const SimulatedJoint* joint, ActorInstance* actorInstance, const AZ::Color& color);

        SimulatedObjectModel* GetSimulatedObjectModel() const;
        SimulatedJointWidget* GetSimulatedJointWidget() const;

        void ScrollTo(const QModelIndex& index);

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        // SimulatedObjectRequestBus overrides
        void UpdateWidget() override;

        // ActorEditorNotificationBus overrides
        void ActorSelectionChanged(Actor* actor) override;
        void ActorInstanceSelectionChanged(EMotionFX::ActorInstance* actorInstance) override;

        EMStudio::SimulatedObjectActionManager* GetActionManager() const { return m_actionManager.get(); }

        Actor* GetActor() const;
        ActorInstance* GetActorInstance() const;
        Node* GetNode() const;
        Physics::CharacterColliderNodeConfiguration* GetNodeConfig() const;
        QModelIndexList GetSelectedModelIndices() const;

    public slots:
        void OnContextMenu(const QPoint& position);
        void OnRemoveSimulatedObject(const QModelIndex& objectIndex);
        void OnRemoveSimulatedJoint(const QModelIndex& jointIndex, bool removeChildren);
        void OnRemoveSimulatedJoints(const QModelIndexList& jointIndices);

        void OnAddCollider();
        void OnAddColliderByType(const AZ::TypeId& colliderType);
        void OnClearColliders();

    private:
        EMotionFX::Actor* m_actor = nullptr;
        EMotionFX::ActorInstance* m_actorInstance = nullptr;
        QWidget* m_mainWidget = nullptr;
        QLabel* m_noSelectionWidget = nullptr;
        QWidget* m_selectionWidget = nullptr;
        QTreeView* m_treeView = nullptr;
        AZStd::unique_ptr<SimulatedObjectModel> m_simulatedObjectModel = nullptr;
        AZStd::unique_ptr<EMStudio::SimulatedObjectActionManager> m_actionManager;
        QWidget* m_contentsWidget = nullptr;
        QDockWidget* m_simulatedObjectInspectorDock = nullptr;
        SimulatedJointWidget* m_simulatedJointWidget = nullptr;
        QPushButton* m_addSimulatedObjectButton = nullptr;

        QLabel* m_instruction1 = nullptr;
        QLabel* m_instruction2 = nullptr;

        // Rendering
        AZStd::vector<AZ::Vector3> m_vertexBuffer;
        AZStd::vector<AZ::u32> m_indexBuffer;
        AZStd::vector<AZ::Vector3> m_lineBuffer;
        AZStd::vector<bool> m_lineValidityBuffer;

        // Callbacks
        MCORE_DEFINECOMMANDCALLBACK(DataChangedCallback);
        MCORE_DEFINECOMMANDCALLBACK(AddSimulatedObjectCallback);
        MCORE_DEFINECOMMANDCALLBACK(AddSimulatedJointsCallback);
        // static bool DataChanged(AZ::u32 actorId);
        AZStd::vector<MCore::Command::Callback*> m_commandCallbacks;

        static int s_jointLabelSpacing;
    };
} // namespace EMotionFX
