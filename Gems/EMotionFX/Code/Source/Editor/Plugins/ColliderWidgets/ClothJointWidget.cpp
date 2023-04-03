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
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/ColliderWidgets/ClothOutlinerNotificationHandler.h>
#include <Editor/Plugins/ColliderWidgets/ClothJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace EMotionFX
{
    ClothJointWidget::ClothJointWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_handler(this)
    {
        setObjectName("EMotionFX.ClothJointWidget");
    }

    QWidget* ClothJointWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        result->setLayout(layout);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(SkeletonModel::s_clothColliderIconPath), result);
        connect(m_collidersWidget, &ColliderContainerWidget::CopyCollider, this, &ClothJointWidget::OnCopyCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::PasteCollider, this, &ClothJointWidget::OnPasteCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &ClothJointWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    void ClothJointWidget::InternalReinit()
    {
        m_widgetCount = 0;
        if (GetSelectedModelIndices().size() == 1)
        {
            Physics::CharacterColliderNodeConfiguration* nodeConfig = GetNodeConfig();
            if (nodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(GetActor(), GetNode(), PhysicsSetup::ColliderConfigType::Cloth, nodeConfig->m_shapes, serializeContext);
                m_collidersWidget->show();
                m_widgetCount = static_cast<int>(nodeConfig->m_shapes.size());
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

    int ClothJointWidget::WidgetCount() const
    {
        return m_widgetCount;
    }

    void ClothJointWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        ColliderHelpers::AddCollider(GetSelectedModelIndices(), PhysicsSetup::Cloth, colliderType);
    }

    void ClothJointWidget::OnCopyCollider(size_t colliderIndex)
    {
        ColliderHelpers::CopyColliderToClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::Cloth);
    }

    void ClothJointWidget::OnPasteCollider(size_t colliderIndex, bool replace)
    {
        ColliderHelpers::PasteColliderFromClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::Cloth, replace);
    }

    void ClothJointWidget::OnRemoveCollider(size_t colliderIndex)
    {
        CommandColliderHelpers::RemoveCollider(GetActor()->GetID(), GetNode()->GetNameString(), PhysicsSetup::Cloth, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* ClothJointWidget::GetNodeConfig() const
    {
        AZ_Assert(GetSelectedModelIndices().size() == 1, "Get Node config function only return the config when it is single seleted");
        Actor* actor = GetActor();
        Node* joint = GetNode();
        if (!actor || !joint)
        {
            return nullptr;
        }

        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return nullptr;
        }

        const Physics::CharacterColliderConfiguration& clothConfig = physicsSetup->GetClothConfig();
        return clothConfig.FindNodeConfigByName(joint->GetNameString());
    }
} // namespace EMotionFX
