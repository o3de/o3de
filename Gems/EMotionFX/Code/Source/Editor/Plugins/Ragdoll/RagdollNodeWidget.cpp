/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace EMotionFX
{
    RagdollCardHeader::RagdollCardHeader(QWidget* parent)
        : AzQtComponents::CardHeader(parent)
    {
        m_backgroundFrame->setObjectName("");
    }

    RagdollCard::RagdollCard(QWidget* parent)
        : AzQtComponents::Card(new RagdollCardHeader(), parent)
    {
        hideFrame();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    RagdollNodeWidget::RagdollNodeWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
        , m_ragdollNodeCard(nullptr)
        , m_ragdollNodeEditor(nullptr)
        , m_addRemoveButton(nullptr)
        , m_jointLimitWidget(nullptr)
        , m_addColliderButton(nullptr)
        , m_collidersWidget(nullptr)
    {
    }

    QWidget* RagdollNodeWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Ragdoll node widget
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

        m_ragdollNodeEditor = new EMotionFX::ObjectEditor(serializeContext, result);
        m_ragdollNodeCard = new RagdollCard(result);
        m_ragdollNodeCard->setTitle("Ragdoll properties");
        m_ragdollNodeCard->setContentWidget(m_ragdollNodeEditor);
        m_ragdollNodeCard->setExpanded(true);
        AzQtComponents::CardHeader* cardHeader = m_ragdollNodeCard->header();
        cardHeader->setHasContextMenu(false);
        layout->addWidget(m_ragdollNodeCard);

        // Buttons
        QVBoxLayout* buttonLayout = new QVBoxLayout();
        layout->addLayout(buttonLayout);

        m_addColliderButton = new AddColliderButton("Add ragdoll collider", result, PhysicsSetup::ColliderConfigType::Ragdoll);
        connect(m_addColliderButton, &AddColliderButton::AddCollider, this, &RagdollNodeWidget::OnAddCollider);
        buttonLayout->addWidget(m_addColliderButton);

        m_addRemoveButton = new QPushButton(result);
        m_addRemoveButton->setObjectName("EMFX.RagdollNodeWidget.PushButton.RagdollAddRemoveButton");
        connect(m_addRemoveButton, &QPushButton::clicked, this, &RagdollNodeWidget::OnAddRemoveRagdollNode);
        buttonLayout->addWidget(m_addRemoveButton);

        // Joint limit
        m_jointLimitWidget = new RagdollJointLimitWidget(m_copiedJointLimit, result);
        connect(m_jointLimitWidget, &RagdollJointLimitWidget::JointLimitCopied, [this](const AZStd::string& serializedJointLimits)
        {
            m_copiedJointLimit = serializedJointLimits;
        });
        layout->addWidget(m_jointLimitWidget);

        // Colliders
        m_collidersWidget = new ColliderContainerWidget(QIcon(SkeletonModel::s_ragdollColliderIconPath), result);
        connect(m_collidersWidget, &ColliderContainerWidget::CopyCollider, this, &RagdollNodeWidget::OnCopyCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::PasteCollider, this,  &RagdollNodeWidget::OnPasteCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &RagdollNodeWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    QWidget* RagdollNodeWidget::CreateNoSelectionWidget(QWidget* parent)
    {
        QLabel* noSelectionLabel = new QLabel("Select joints from the Skeleton Outliner and add it to the ragdoll using the right-click menu", parent);
        noSelectionLabel->setWordWrap(true);

        return noSelectionLabel;
    }

    void RagdollNodeWidget::InternalReinit()
    {
        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        if (selectedModelIndices.size() == 1)
        {
            m_ragdollNodeEditor->ClearInstances(false);

            Physics::CharacterColliderNodeConfiguration* colliderNodeConfig = GetRagdollColliderNodeConfig();
            Physics::RagdollNodeConfiguration* ragdollNodeConfig = GetRagdollNodeConfig();
            if (ragdollNodeConfig)
            {
                m_addColliderButton->show();
                m_addRemoveButton->setText("Remove from ragdoll");

                m_ragdollNodeEditor->AddInstance(ragdollNodeConfig, azrtti_typeid(ragdollNodeConfig));

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                if (colliderNodeConfig)
                {
                    m_collidersWidget->Update(GetActor(), GetNode(), PhysicsSetup::ColliderConfigType::Ragdoll, colliderNodeConfig->m_shapes, serializeContext);
                }
                else
                {
                    m_collidersWidget->Reset();
                }

                m_jointLimitWidget->Update(selectedModelIndices[0]);
                m_ragdollNodeCard->setExpanded(true);
                m_ragdollNodeCard->show();
                m_jointLimitWidget->show();
                m_collidersWidget->show();
            }
            else
            {
                m_addColliderButton->hide();
                m_addRemoveButton->setText("Add to ragdoll");
                m_collidersWidget->Reset();
                m_ragdollNodeCard->hide();
                m_jointLimitWidget->Update(QModelIndex());
                m_jointLimitWidget->hide();
                m_collidersWidget->hide();
            }
        }
        else
        {
            m_ragdollNodeEditor->ClearInstances(true);
            m_jointLimitWidget->Update(QModelIndex());
            m_collidersWidget->Reset();
        }
    }

    void RagdollNodeWidget::OnAddRemoveRagdollNode()
    {
        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        if (GetRagdollNodeConfig())
        {
            // The node is present in the ragdoll, remove it.
            RagdollNodeInspectorPlugin::RemoveFromRagdoll(selectedModelIndices);
        }
        else
        {
            // The node is not part of the ragdoll, add it.
            RagdollNodeInspectorPlugin::AddToRagdoll(selectedModelIndices);
        }
    }

    void RagdollNodeWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        ColliderHelpers::AddCollider(GetSelectedModelIndices(), PhysicsSetup::Ragdoll, colliderType);
    }

    void RagdollNodeWidget::OnCopyCollider(size_t colliderIndex)
    {
        ColliderHelpers::CopyColliderToClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::Ragdoll);
    }

    void RagdollNodeWidget::OnPasteCollider(size_t colliderIndex, bool replace)
    {
        ColliderHelpers::PasteColliderFromClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::Ragdoll, replace);
    }

    void RagdollNodeWidget::OnRemoveCollider(size_t colliderIndex)
    {
        CommandColliderHelpers::RemoveCollider(GetActor()->GetID(), GetNode()->GetNameString(), PhysicsSetup::Ragdoll, colliderIndex);
    }

    Physics::RagdollConfiguration* RagdollNodeWidget::GetRagdollConfig() const
    {
        Actor* actor = GetActor();
        Node* node = GetNode();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                return &physicsSetup->GetRagdollConfig();
            }
        }

        return nullptr;
    }

    Physics::CharacterColliderNodeConfiguration* RagdollNodeWidget::GetRagdollColliderNodeConfig() const
    {
        Actor* actor = GetActor();
        Node* node = GetNode();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::CharacterColliderConfiguration& colliderConfig = physicsSetup->GetRagdollConfig().m_colliders;
                return colliderConfig.FindNodeConfigByName(node->GetNameString());
            }
        }

        return nullptr;
    }

    Physics::RagdollNodeConfiguration* RagdollNodeWidget::GetRagdollNodeConfig() const
    {
        Actor* actor = GetActor();
        Node* node = GetNode();
        if (actor && node)
        {
            const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            if (physicsSetup)
            {
                const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
                return ragdollConfig.FindNodeConfigByName(node->GetNameString());
            }
        }

        return nullptr;
    }
} // namespace EMotionFX
