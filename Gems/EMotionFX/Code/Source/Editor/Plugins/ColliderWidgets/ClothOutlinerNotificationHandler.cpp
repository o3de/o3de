/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/SystemBus.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/Plugins/ColliderWidgets/ClothJointWidget.h>
#include <Editor/Plugins/ColliderWidgets/ClothOutlinerNotificationHandler.h>
#include <Editor/SkeletonModel.h>
#include <Integration/Rendering/RenderActorSettings.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QScrollArea>

namespace EMotionFX
{
    ClothOutlinerNotificationHandler::ClothOutlinerNotificationHandler(ClothJointWidget* colliderWidget)
        : m_colliderWidget(colliderWidget)
    {
        if (!IsNvClothGemAvailable() || !ColliderHelpers::AreCollidersReflected())
        {
            m_colliderWidget->ErrorNotification(
                "Nvidia Cloth Gem not Available",
                "Cloth collider editor depends on the NVIDIA Cloth gem. Please enable it in the Project Manager.");
            return;
        }
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
    }

    ClothOutlinerNotificationHandler::~ClothOutlinerNotificationHandler()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
    }

    bool ClothOutlinerNotificationHandler::IsNvClothGemAvailable() const
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // TypeId of NvCloth::SystemComponent
        const char* typeIDClothSystem = "{89DF5C48-64AC-4B8E-9E61-0D4C7A7B5491}";

        return serializeContext && serializeContext->FindClassData(AZ::TypeId::CreateString(typeIDClothSystem));
    }

    void ClothOutlinerNotificationHandler::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
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
            const bool hasColliders = modelIndex.data(SkeletonModel::ROLE_CLOTH).toBool();
            if (hasColliders)
            {
                numJointsWithColliders++;
            }
        }

        QMenu* contextMenu = menu->addMenu("Cloth");

        if (numSelectedJoints > 0)
        {
            QMenu* addColliderMenu = contextMenu->addMenu("Add collider");

            QAction* addCapsuleAction = addColliderMenu->addAction("Add capsule");
            addCapsuleAction->setProperty("typeId", azrtti_typeid<Physics::CapsuleShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addCapsuleAction, &QAction::triggered, this, &ClothOutlinerNotificationHandler::OnAddCollider);

            QAction* addSphereAction = addColliderMenu->addAction("Add sphere");
            addSphereAction->setProperty("typeId", azrtti_typeid<Physics::SphereShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addSphereAction, &QAction::triggered, this, &ClothOutlinerNotificationHandler::OnAddCollider);

            ColliderHelpers::AddCopyFromMenu(this, contextMenu, PhysicsSetup::ColliderConfigType::Cloth, selectedRowIndices);
        }

        if (numJointsWithColliders > 0)
        {
            QAction* removeCollidersAction = contextMenu->addAction("Remove colliders");
            connect(removeCollidersAction, &QAction::triggered, this, &ClothOutlinerNotificationHandler::OnClearColliders);
        }
    }

    bool ClothOutlinerNotificationHandler::IsJointInCloth(const QModelIndex& index)
    {
        return index.data(SkeletonModel::ROLE_CLOTH).toBool();
    }

    void ClothOutlinerNotificationHandler::OnAddCollider()
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

        ColliderHelpers::AddCollider(selectedRowIndices, PhysicsSetup::Cloth, colliderType);
    }

    void ClothOutlinerNotificationHandler::OnClearColliders()
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

        ColliderHelpers::ClearColliders(selectedRowIndices, PhysicsSetup::Cloth);
    }
} // namespace EMotionFX
