/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Character.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionJointWidget.h>
#include <Editor/Plugins/ColliderWidgets/HitDetectionOutlinerNotificationHandler.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <Editor/SkeletonModel.h>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace EMotionFX
{
    HitDetectionJointWidget::HitDetectionJointWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_handler(this)
    {
        setObjectName("EMotionFX.HitDetectionJointWidget");
    }

    QWidget* HitDetectionJointWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(SkeletonModel::s_hitDetectionColliderIconPath), result);
        connect(m_collidersWidget, &ColliderContainerWidget::CopyCollider, this, &HitDetectionJointWidget::OnCopyCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::PasteCollider, this, &HitDetectionJointWidget::OnPasteCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &HitDetectionJointWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    void HitDetectionJointWidget::InternalReinit()
    {
        m_widgetCount = 0;
        if (GetSelectedModelIndices().size() == 1)
        {
            Physics::CharacterColliderNodeConfiguration* hitDetectionNodeConfig = GetNodeConfig();
            if (hitDetectionNodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(
                    GetActor(),
                    GetNode(),
                    PhysicsSetup::ColliderConfigType::HitDetection,
                    hitDetectionNodeConfig->m_shapes,
                    serializeContext);
                m_collidersWidget->show();
                m_widgetCount = static_cast<int>(hitDetectionNodeConfig->m_shapes.size());
            }
            else
            {
                m_collidersWidget->Reset();
            }
        }
        else
        {
            m_collidersWidget->Reset();
        }
        emit WidgetCountChanged();
    }

    int HitDetectionJointWidget::WidgetCount() const
    {
        return m_widgetCount;
    }

    void HitDetectionJointWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        ColliderHelpers::AddCollider(GetSelectedModelIndices(), PhysicsSetup::HitDetection, colliderType);
    }

    void HitDetectionJointWidget::OnCopyCollider(size_t colliderIndex)
    {
        ColliderHelpers::CopyColliderToClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::HitDetection);
    }

    void HitDetectionJointWidget::OnPasteCollider(size_t colliderIndex, bool replace)
    {
        ColliderHelpers::PasteColliderFromClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::HitDetection, replace);
    }

    void HitDetectionJointWidget::OnRemoveCollider(size_t colliderIndex)
    {
        CommandColliderHelpers::RemoveCollider(GetActor()->GetID(), GetNode()->GetNameString(), PhysicsSetup::HitDetection, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* HitDetectionJointWidget::GetNodeConfig() const
    {
        AZ_Assert(GetSelectedModelIndices().size() == 1, "Get Node config function only return the config when it is single seleted");
        Actor* actor = GetActor();
        Node* node = GetNode();
        if (!actor || !node)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return nullptr;
        }

        const Physics::CharacterColliderConfiguration& hitDetectionConfig = physicsSetup->GetHitDetectionConfig();
        return hitDetectionConfig.FindNodeConfigByName(node->GetNameString());
    }
} // namespace EMotionFX
