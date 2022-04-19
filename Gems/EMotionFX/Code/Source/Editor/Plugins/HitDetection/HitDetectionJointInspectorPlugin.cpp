/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/SystemBus.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderViewWidget.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointInspectorPlugin.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointWidget.h>
#include <Integration/Rendering/RenderActorSettings.h>
#include <QScrollArea>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    HitDetectionJointInspectorPlugin::HitDetectionJointInspectorPlugin()
        : EMStudio::DockWidgetPlugin()
        , m_nodeWidget(nullptr)
    {
    }

    HitDetectionJointInspectorPlugin::~HitDetectionJointInspectorPlugin()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
    }

    bool HitDetectionJointInspectorPlugin::Init()
    {
        if (ColliderHelpers::AreCollidersReflected())
        {
            m_nodeWidget = new HitDetectionJointWidget();
            m_nodeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            m_nodeWidget->CreateGUI();

            QScrollArea* scrollArea = new QScrollArea();
            scrollArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            scrollArea->setWidget(m_nodeWidget);
            scrollArea->setWidgetResizable(true);

            m_dock->setWidget(scrollArea);

            EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
        }
        else
        {
            m_dock->setWidget(CreateErrorContentWidget("Hit detection collider editor depends on the PhysX gem. Please enable it in the Project Manager."));
        }

        return true;
    }

    void HitDetectionJointInspectorPlugin::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
    {
        if (selectedRowIndices.empty())
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
            connect(addBoxAction, &QAction::triggered, this, &HitDetectionJointInspectorPlugin::OnAddCollider);

            QAction* addCapsuleAction = addColliderMenu->addAction("Add capsule");
            addCapsuleAction->setProperty("typeId", azrtti_typeid<Physics::CapsuleShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addCapsuleAction, &QAction::triggered, this, &HitDetectionJointInspectorPlugin::OnAddCollider);

            QAction* addSphereAction = addColliderMenu->addAction("Add sphere");
            addSphereAction->setProperty("typeId", azrtti_typeid<Physics::SphereShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addSphereAction, &QAction::triggered, this, &HitDetectionJointInspectorPlugin::OnAddCollider);


            ColliderHelpers::AddCopyFromMenu(this, contextMenu, PhysicsSetup::ColliderConfigType::HitDetection, selectedRowIndices);
        }

        if (numJointsWithColliders > 0)
        {
            QAction* removeCollidersAction = contextMenu->addAction("Remove colliders");
            connect(removeCollidersAction, &QAction::triggered, this, &HitDetectionJointInspectorPlugin::OnClearColliders);
        }
    }

    void HitDetectionJointInspectorPlugin::OnAddCollider()
    {
        AZ::Outcome<const QModelIndexList&> selectedRowIndicesOutcome;
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

    void HitDetectionJointInspectorPlugin::OnClearColliders()
    {
        AZ::Outcome<const QModelIndexList&> selectedRowIndicesOutcome;
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

    void HitDetectionJointInspectorPlugin::LegacyRender(EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo)
    {
        EMStudio::RenderViewWidget* activeViewWidget = renderPlugin->GetActiveViewWidget();
        if (!activeViewWidget)
        {
            return;
        }

        const bool renderColliders = activeViewWidget->GetRenderFlag(EMStudio::RenderViewWidget::RENDER_HITDETECTION_COLLIDERS);
        if (!renderColliders)
        {
            return;
        }

        const EMStudio::RenderOptions* renderOptions = renderPlugin->GetRenderOptions();

        ColliderContainerWidget::LegacyRenderColliders(PhysicsSetup::HitDetection,
            renderOptions->GetHitDetectionColliderColor(),
            renderOptions->GetSelectedHitDetectionColliderColor(),
            renderPlugin,
            renderInfo);
    }
} // namespace EMotionFX
