/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Plugins/ColliderWidgets/ClothJointWidget.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionJointWidget.h>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <Editor/Plugins/ColliderWidgets/RagdollNodeWidget.h>
#include <QPushButton>
#endif
#include <QTreeWidget>
#include <QStandardItem>

namespace EMStudio
{
    class ActorInfo;
    class NodeInfo;
}

namespace EMotionFX
{
    class SimulatedObjectColliderWidget;
    class AddCollidersButton;

    //! A Widget in the Inspector Pane displaying Attributes of selected Nodes in a Skeleton
    class JointPropertyWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        JointPropertyWidget(QWidget* parent = nullptr);
        ~JointPropertyWidget();

    public slots:
        void Reset();

    private slots:
        void OnAddCollider(PhysicsSetup::ColliderConfigType configType, AZ::TypeId colliderType);
        void OnAddToRagdoll();
        void OnSearchTextChanged();

    private:
        AzToolsFramework::ReflectedPropertyEditor* m_propertyWidget = nullptr;
        AddCollidersButton* m_addCollidersButton = nullptr;

        ClothJointWidget* m_clothJointWidget = nullptr;
        HitDetectionJointWidget* m_hitDetectionJointWidget = nullptr;
        RagdollNodeWidget* m_ragdollJointWidget = nullptr;
        SimulatedObjectColliderWidget* m_simulatedJointWidget = nullptr;

        AZStd::unique_ptr<EMStudio::ActorInfo> m_actorInfo;
        AZStd::unique_ptr<EMStudio::NodeInfo> m_nodeInfo;

        QLineEdit* m_filterEntityBox = nullptr;
        QString m_filterString;
    };


    //! Button To Add different Colliders
    class AddCollidersButton : public QPushButton
    {
        Q_OBJECT
    public:
        AddCollidersButton(QWidget* parent=nullptr);
        AZStd::string GetNameForColliderType(AZ::TypeId colliderType) const;
    public slots:
        void OnAddColliderActionTriggered(const QModelIndex& index);
    signals:
        void AddCollider(PhysicsSetup::ColliderConfigType configType, AZ::TypeId colliderType);
        void AddToRagdoll();
    protected slots:
        void OnCreateContextMenu();
    protected:
        const AZStd::vector<AZ::TypeId> m_supportedColliderTypes = {
            azrtti_typeid<Physics::BoxShapeConfiguration>(),
            azrtti_typeid<Physics::CapsuleShapeConfiguration>(),
            azrtti_typeid<Physics::SphereShapeConfiguration>()};

        QStandardItemModel* model = nullptr;
    };

} // namespace EMotionFX
