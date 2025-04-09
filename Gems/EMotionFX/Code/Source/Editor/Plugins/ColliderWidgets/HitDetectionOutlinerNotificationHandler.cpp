/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/SystemBus.h>
#include <MCore/Source/AzCoreConversions.h>
#include <UI/Notifications/ToastBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionJointWidget.h>
#include <Editor/SkeletonModel.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionOutlinerNotificationHandler.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Integration/Rendering/RenderActorSettings.h>
#include <QScrollArea>
#include <QTimer>


namespace EMotionFX
{
    HitDetectionOutlinerNotificationHandler::HitDetectionOutlinerNotificationHandler(HitDetectionJointWidget* jointWidget)
        :m_nodeWidget(jointWidget)
    {

        if (!ColliderHelpers::AreCollidersReflected())
        {
            m_nodeWidget->ErrorNotification("PhysX disabled",
                                            "Hit detection collider editor depends on the PhysX gem. Please enable it in the Project Manager.");

            return;
        }
         EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();

    }
    HitDetectionOutlinerNotificationHandler::~HitDetectionOutlinerNotificationHandler()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
    }

    void HitDetectionOutlinerNotificationHandler::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
    {
        if (selectedRowIndices.empty())
        {
            return;
        }

        if (selectedRowIndices.size() == 1 && SkeletonModel::IndexIsRootNode(selectedRowIndices[0]))
        {
            return;
        }

        const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return;
        }

        const int numSelectedJoints = selectedRowIndices.count();
        int numJointsWithColliders = 0;
        for (const QModelIndex& modelIndex : selectedRowIndices)
        {
            const bool hasColliders = modelIndex.data(SkeletonModel::ROLE_HITDETECTION).toBool();
            if (hasColliders)
            {
                numJointsWithColliders++;
            }
        }

        QMenu* contextMenu = menu->addMenu("Hit detection");

        if (numSelectedJoints > 0)
        {
            QMenu* addColliderMenu = contextMenu->addMenu("Add collider");

            QAction* addBoxAction = addColliderMenu->addAction("Add box");
            addBoxAction->setProperty("typeId", azrtti_typeid<Physics::BoxShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addBoxAction, &QAction::triggered, this, &HitDetectionOutlinerNotificationHandler::OnAddCollider);
            QAction* addCapsuleAction = addColliderMenu->addAction("Add capsule");
            addCapsuleAction->setProperty("typeId", azrtti_typeid<Physics::CapsuleShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addCapsuleAction, &QAction::triggered, this, &HitDetectionOutlinerNotificationHandler::OnAddCollider);

            QAction* addSphereAction = addColliderMenu->addAction("Add sphere");
            addSphereAction->setProperty("typeId", azrtti_typeid<Physics::SphereShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addSphereAction, &QAction::triggered, this, &HitDetectionOutlinerNotificationHandler::OnAddCollider);


            ColliderHelpers::AddCopyFromMenu(this, contextMenu, PhysicsSetup::ColliderConfigType::HitDetection, selectedRowIndices);
        }

        if (numJointsWithColliders > 0)
        {
            QAction* removeCollidersAction = contextMenu->addAction("Remove colliders");
            connect(removeCollidersAction, &QAction::triggered, this, &HitDetectionOutlinerNotificationHandler::OnClearColliders);
        }
    }

    void HitDetectionOutlinerNotificationHandler::OnAddCollider()
    {
        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!selectedRowIndicesOutcome.IsSuccess())
        {
            return;
        }

        const QModelIndexList& selectedRowIndices = selectedRowIndicesOutcome.GetValue();
        if (selectedRowIndices.empty())
        {
            return;
        }

        QAction* action = static_cast<QAction*>(sender());
        const QByteArray typeString = action->property("typeId").toString().toUtf8();
        const AZ::TypeId& colliderType = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        ColliderHelpers::AddCollider(selectedRowIndices, PhysicsSetup::HitDetection, colliderType);
    }

    void HitDetectionOutlinerNotificationHandler::OnClearColliders()
    {
        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!selectedRowIndicesOutcome.IsSuccess())
        {
            return;
        }

        const QModelIndexList& selectedRowIndices = selectedRowIndicesOutcome.GetValue();
        if (selectedRowIndices.empty())
        {
            return;
        }

        ColliderHelpers::ClearColliders(selectedRowIndices, PhysicsSetup::HitDetection);
    }

} // namespace EMotionFX
