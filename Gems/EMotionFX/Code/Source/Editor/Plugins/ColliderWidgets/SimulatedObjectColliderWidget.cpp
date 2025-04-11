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
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/NotificationWidget.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/ColliderWidgets/SimulatedObjectColliderWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QLabel>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>


namespace EMotionFX
{
    SimulatedObjectColliderWidget::SimulatedObjectColliderWidget(QWidget* parent)
        : SkeletonModelJointWidget(parent)
    {
    }

    QWidget* SimulatedObjectColliderWidget::CreateContentWidget(QWidget* parent)
    {
        QWidget* result = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(ColliderContainerWidget::s_layoutSpacing);
        result->setLayout(layout);

        // Object ownership label
        {
            m_ownershipWidget = new QWidget(result);
            QHBoxLayout* ownershipLayout = new QHBoxLayout(m_ownershipWidget);
            ownershipLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            ownershipLayout->setMargin(0);
            ownershipLayout->setSpacing(0);
            m_ownershipWidget->setLayout(ownershipLayout);

            ownershipLayout->addSpacerItem(new QSpacerItem(s_jointLabelSpacing, 0, QSizePolicy::Fixed));
            QLabel* tempLabel = new QLabel("Part of Simulated Objects");
            tempLabel->setStyleSheet("font-weight: bold;");
            ownershipLayout->addWidget(tempLabel);

            ownershipLayout->addSpacerItem(new QSpacerItem(44, 0, QSizePolicy::Fixed));
            m_ownershipLabel = new QLabel();
            m_ownershipLabel->setWordWrap(true);
            ownershipLayout->addWidget(m_ownershipLabel);
            ownershipLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored));
            layout->addWidget(m_ownershipWidget);
        }

        // Collide with object label
        {
            m_collideWithWidget = new QWidget(result);
            QHBoxLayout* collideWithLayout = new QHBoxLayout(m_collideWithWidget);
            collideWithLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            collideWithLayout->setMargin(0);
            collideWithLayout->setSpacing(0);
            m_collideWithWidget->setLayout(collideWithLayout);

            collideWithLayout->addSpacerItem(new QSpacerItem(s_jointLabelSpacing, 0, QSizePolicy::Fixed));
            QLabel* tempLabel = new QLabel("Collide with Simulated Objects");
            tempLabel->setStyleSheet("font-weight: bold;");
            collideWithLayout->addWidget(tempLabel);

            collideWithLayout->addSpacerItem(new QSpacerItem(13, 0, QSizePolicy::Fixed));
            m_collideWithLabel = new QLabel();
            m_collideWithLabel->setWordWrap(true);
            collideWithLayout->addWidget(m_collideWithLabel);
            collideWithLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::Ignored));
            layout->addWidget(m_collideWithWidget);
        }

        // Collider notification
        m_colliderNotif = new NotificationWidget(result, "Currently, this collider doesn't collide against any simulated object. Select the Simulated Object you want to collide with from the Simulated Object Window, and choose this collider in the \"Collide with\" setting.");
        layout->addWidget(m_colliderNotif);
        m_colliderNotif->hide();

        // Colliders widget
        m_collidersWidget = new ColliderContainerWidget(QIcon(SkeletonModel::s_simulatedColliderIconPath), result); // use the ragdoll white collider icon because it's generic to all colliders.
        m_collidersWidget->setObjectName("EMFX.SimulatedObjectColliderWidget.ColliderContainerWidget");
        connect(m_collidersWidget, &ColliderContainerWidget::CopyCollider, this, &SimulatedObjectColliderWidget::OnCopyCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::PasteCollider, this, &SimulatedObjectColliderWidget::OnPasteCollider);
        connect(m_collidersWidget, &ColliderContainerWidget::RemoveCollider, this, &SimulatedObjectColliderWidget::OnRemoveCollider);
        layout->addWidget(m_collidersWidget);

        return result;
    }

    void SimulatedObjectColliderWidget::InternalReinit()
    {
        m_widgetCount = 0;

        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        if (selectedModelIndices.size() == 1)
        {
            Physics::CharacterColliderNodeConfiguration* nodeConfig = GetNodeConfig();
            if (nodeConfig)
            {
                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                AZ_Error("EMotionFX", serializeContext, "Can't get serialize context from component application.");

                m_collidersWidget->Update(GetActor(), GetNode(), PhysicsSetup::ColliderConfigType::SimulatedObjectCollider, nodeConfig->m_shapes, serializeContext);
                m_content->show();
                m_collidersWidget->show();
                m_widgetCount = static_cast<int>(1 + nodeConfig->m_shapes.size()); // on Mac unsigned long (size_t) can't silent downcast to int, added static_cast
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

        UpdateOwnershipLabel();
        UpdateColliderNotification();

        emit WidgetCountChanged();
    }

    int SimulatedObjectColliderWidget::WidgetCount() const
    {
        return m_widgetCount;
    }

    void SimulatedObjectColliderWidget::UpdateOwnershipLabel()
    {
        Actor* actor = GetActor();
        if (!actor)
        {
            return;
        }

        AZStd::string labelText;
        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        const AZStd::vector<SimulatedObject*>& simObjs = actor->GetSimulatedObjectSetup()->GetSimulatedObjects();
        for (const SimulatedObject* obj : simObjs)
        {
            for (int i = 0; i < selectedModelIndices.size(); ++i)
            {
                Node* node = selectedModelIndices[i].data(SkeletonModel::ROLE_POINTER).value<Node*>();
                if (obj->FindSimulatedJointBySkeletonJointIndex(node->GetNodeIndex()))
                {
                    if (!labelText.empty())
                    {
                        labelText += ", ";
                    }
                    labelText += obj->GetName();
                    break;
                }
            }
        }
        if (labelText.empty())
        {
            labelText = "N/A";
        }
        m_ownershipLabel->setText(labelText.c_str());
    }

    void SimulatedObjectColliderWidget::UpdateColliderNotification()
    {
        m_colliderNotif->hide();
        m_collideWithWidget->hide();

        Actor* actor = GetActor();
        Node* joint = GetNode();
        if (!actor || !joint)
        {
            return;
        }

        const QModelIndexList& selectedModelIndices = GetSelectedModelIndices();
        // Only show the notification when it is single selection.
        if (selectedModelIndices.size() != 1)
        {
            return;
        }

        Physics::CharacterColliderNodeConfiguration* nodeConfig = GetNodeConfig();
        if (!nodeConfig)
        {
            return;
        }

        m_collideWithWidget->show();

        AZStd::string collideWith;
        const AZStd::vector<SimulatedObject*>& simObjs = actor->GetSimulatedObjectSetup()->GetSimulatedObjects();
        for (const SimulatedObject* obj : simObjs)
        {
            if (AZStd::find(obj->GetColliderTags().begin(), obj->GetColliderTags().end(), joint->GetName()) != obj->GetColliderTags().end())
            {
                if (!collideWith.empty())
                {
                    collideWith += ", ";
                }
                collideWith += obj->GetName();
            }
        }

        if (collideWith.empty())
        {
            m_colliderNotif->show();
            m_collideWithLabel->setText("N/A");
        }
        else
        {
            m_colliderNotif->hide();
            m_collideWithLabel->setText(collideWith.c_str());
        }
    }

    void SimulatedObjectColliderWidget::OnAddCollider(const AZ::TypeId& colliderType)
    {
        ColliderHelpers::AddCollider(GetSelectedModelIndices(), PhysicsSetup::SimulatedObjectCollider, colliderType);
    }

    void SimulatedObjectColliderWidget::OnCopyCollider(size_t colliderIndex)
    {
        ColliderHelpers::CopyColliderToClipboard(GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::SimulatedObjectCollider);
    }

    void SimulatedObjectColliderWidget::OnPasteCollider(size_t colliderIndex, bool replace)
    {
        ColliderHelpers::PasteColliderFromClipboard(
            GetSelectedModelIndices().first(), colliderIndex, PhysicsSetup::SimulatedObjectCollider, replace);
    }

    void SimulatedObjectColliderWidget::OnRemoveCollider(size_t colliderIndex)
    {
        CommandColliderHelpers::RemoveCollider(GetActor()->GetID(), GetNode()->GetNameString(), PhysicsSetup::SimulatedObjectCollider, colliderIndex);
    }

    Physics::CharacterColliderNodeConfiguration* SimulatedObjectColliderWidget::GetNodeConfig() const
    {
        AZ_Assert(GetSelectedModelIndices().size() == 1, "Get Node config function only return the config when it is single selected");
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

        const Physics::CharacterColliderConfiguration& simulatedObjectColliderConfig = physicsSetup->GetSimulatedObjectColliderConfig();
        return simulatedObjectColliderConfig.FindNodeConfigByName(joint->GetNameString());
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AddToSimulatedObjectButton::AddToSimulatedObjectButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent)
    {
        m_actionManager = AZStd::make_unique<EMStudio::SimulatedObjectActionManager>();
        setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));
        connect(this, &QPushButton::clicked, this, &AddToSimulatedObjectButton::OnCreateContextMenu);
    }

    void AddToSimulatedObjectButton::OnCreateContextMenu()
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

        const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        if (!actor || !actor->GetSimulatedObjectSetup())
        {
            return;
        }

        const SimulatedObjectSetup* simObjSetup = actor->GetSimulatedObjectSetup().get();
        const size_t simObjCounts = simObjSetup->GetNumSimulatedObjects();

        // Find the object we can add the joints to, excluded the one that already contains all the selected joints.
        AZStd::vector<bool> flags(simObjCounts, false);
        for (const QModelIndex& index : selectedRowIndices)
        {
            const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            for (size_t i = 0; i < simObjCounts; ++i)
            {
                const SimulatedObject* object = simObjSetup->GetSimulatedObject(i);
                if (!object->FindSimulatedJointBySkeletonJointIndex(joint->GetNodeIndex()))
                {
                    flags[i] = true;
                }
            }
        }

        QMenu* contextMenu = new QMenu(this);
        if (simObjCounts == 0)
        {
            QAction* action = contextMenu->addAction("0 simulated objects created.");
            action->setEnabled(false);
            contextMenu->addSeparator();
        }

        // Add all the object that we can add joints to in the menu.
        for (size_t i = 0; i < simObjCounts; ++i)
        {
            if (!flags[i])
            {
                continue;
            }

            const SimulatedObject* obj = simObjSetup->GetSimulatedObject(i);
            QAction* action = contextMenu->addAction(obj->GetName().c_str());
            action->setProperty("simObjName", obj->GetName().c_str());
            action->setProperty("simObjIndex", QVariant::fromValue(i));
            connect(action, &QAction::triggered, this, &AddToSimulatedObjectButton::OnAddJointsToObjectActionTriggered);
        }

        contextMenu->addSeparator();
        // Add the action to add simulated object, then add the joint to the object.
        QAction* addObjectAction = contextMenu->addAction("New simulated object...");
        connect(addObjectAction, &QAction::triggered, this, &AddToSimulatedObjectButton::OnCreateObjectAndAddJointsActionTriggered);

        contextMenu->setFixedWidth(width());
        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(mapToGlobal(QPoint(0, height())));
        }
        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
    }

    void AddToSimulatedObjectButton::OnAddJointsToObjectActionTriggered([[maybe_unused]] bool checked)
    {
        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!selectedRowIndicesOutcome.IsSuccess())
        {
            return;
        }

        QAction* action = static_cast<QAction*>(sender());
        size_t objIndex = static_cast<size_t>(action->property("simObjIndex").toInt());
        SimulatedObjectHelpers::AddSimulatedJoints(selectedRowIndicesOutcome.GetValue(), objIndex, false);
    }

    void AddToSimulatedObjectButton::OnCreateObjectAndAddJointsActionTriggered()
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

        Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        if (!actor || !actor->GetSimulatedObjectSetup())
        {
            return;
        }

        const bool addChildren = (QMessageBox::question(this,
            "Add children of joints?", "Add all children of selected joints to the simulated object?") == QMessageBox::Yes);
        m_actionManager->OnAddNewObjectAndAddJoints(actor, selectedRowIndices, addChildren, this);
    }
} // namespace EMotionFX
