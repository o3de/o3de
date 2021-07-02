/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ObjectEditor.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QVBoxLayout>
#include <Qt>

namespace EMotionFX
{
    ColliderPropertyNotify::ColliderPropertyNotify(ColliderWidget* colliderWidget)
        : m_colliderWidget(colliderWidget)
    {
    }

    void ColliderPropertyNotify::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        if (!m_commandGroup.IsEmpty())
        {
            return;
        }

        const AzToolsFramework::InstanceDataNode* parentDataNode = pNode->GetParent();
        if (!parentDataNode)
        {
            return;
        }

        const AZ::SerializeContext* serializeContext = parentDataNode->GetSerializeContext();
        const AZ::SerializeContext::ClassData* classData = parentDataNode->GetClassMetadata();
        const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();

        const Actor* actor = m_colliderWidget->GetActor();
        const Node* joint = m_colliderWidget->GetJoint();
        if (!actor || !joint)
        {
            return;
        }
        const AZ::u32 actorId = actor->GetID();
        const AZStd::string& jointName = joint->GetNameString();
        const PhysicsSetup::ColliderConfigType colliderType = m_colliderWidget->GetColliderType();
        const size_t colliderIndex = m_colliderWidget->GetColliderIndex();

        const size_t instanceCount = parentDataNode->GetNumInstances();
        m_commandGroup.SetGroupName(AZStd::string::format("Adjust collider%s", instanceCount > 1 ? "s" : ""));
        for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
        {
            CommandAdjustCollider* command = aznew CommandAdjustCollider(actorId, jointName, colliderType, colliderIndex);
            m_commandGroup.AddCommand(command);

            // ColliderConfiguration
            if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::ColliderConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::ColliderConfiguration* colliderConfig = static_cast<Physics::ColliderConfiguration*>(parentDataNode->GetInstance(instanceIndex));

                if (elementData->m_nameCrc == AZ_CRC("CollisionLayer", 0x39931633))
                {
                    command->SetOldCollisionLayer(colliderConfig->m_collisionLayer);
                }
                if (elementData->m_nameCrc == AZ_CRC("CollisionGroupId", 0x84fe4bbe))
                {
                    command->SetOldCollisionGroupId(colliderConfig->m_collisionGroupId);
                }
                if (elementData->m_nameCrc == AZ_CRC("Trigger", 0x1a6b0f5d))
                {
                    command->SetOldIsTrigger(colliderConfig->m_isTrigger);
                }
                if (elementData->m_nameCrc == AZ_CRC("Position", 0x462ce4f5))
                {
                    command->SetOldPosition(colliderConfig->m_position);
                }
                if (elementData->m_nameCrc == AZ_CRC("Rotation", 0x297c98f1))
                {
                    command->SetOldRotation(colliderConfig->m_rotation);
                }
                if (elementData->m_nameCrc == AZ_CRC("MaterialSelection", 0xfebd6d15))
                {
                    command->SetOldMaterial(colliderConfig->m_materialSelection);
                }
                if (elementData->m_nameCrc == AZ_CRC("ColliderTag", 0x5e2963ad))
                {
                    command->SetOldTag(colliderConfig->m_tag);
                }
            }
            // Box
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::BoxShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::BoxShapeConfiguration* boxShapeConfig = static_cast<Physics::BoxShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Configuration", 0xa5e2a5d7))
                {
                    command->SetOldDimensions(boxShapeConfig->m_dimensions);
                }
            }
            // Capsule
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::CapsuleShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::CapsuleShapeConfiguration* capsuleShapeConfig = static_cast<Physics::CapsuleShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Radius", 0x3b7c6e5a))
                {
                    command->SetOldRadius(capsuleShapeConfig->m_radius);
                }
                if (elementData->m_nameCrc == AZ_CRC("Height", 0xf54de50f))
                {
                    command->SetOldHeight(capsuleShapeConfig->m_height);
                }
            }
            // Sphere
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::SphereShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::SphereShapeConfiguration* sphereShapeConfig = static_cast<Physics::SphereShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Radius", 0x3b7c6e5a))
                {
                    command->SetOldRadius(sphereShapeConfig->m_radius);
                }
            }
        }
    }

    void ColliderPropertyNotify::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
    {
        if (m_commandGroup.IsEmpty())
        {
            return;
        }

        const AzToolsFramework::InstanceDataNode* parentDataNode = pNode->GetParent();
        if (!parentDataNode)
        {
            return;
        }

        const AZ::SerializeContext* serializeContext = parentDataNode->GetSerializeContext();
        const AZ::SerializeContext::ClassData* classData = parentDataNode->GetClassMetadata();
        const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();

        const Actor* actor = m_colliderWidget->GetActor();
        const Node* joint = m_colliderWidget->GetJoint();
        if (!actor || !joint)
        {
            return;
        }
        const PhysicsSetup::ColliderConfigType colliderType = m_colliderWidget->GetColliderType();

        const size_t instanceCount = parentDataNode->GetNumInstances();
        for (size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
        {
            CommandAdjustCollider* command = static_cast<CommandAdjustCollider*>(m_commandGroup.GetCommand(instanceIndex));

            // ColliderConfiguration
            if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::ColliderConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::ColliderConfiguration* colliderConfig = static_cast<Physics::ColliderConfiguration*>(parentDataNode->GetInstance(instanceIndex));

                if (elementData->m_nameCrc == AZ_CRC("CollisionLayer", 0x39931633))
                {
                    command->SetCollisionLayer(colliderConfig->m_collisionLayer);
                }
                if (elementData->m_nameCrc == AZ_CRC("CollisionGroupId", 0x84fe4bbe))
                {
                    command->SetCollisionGroupId(colliderConfig->m_collisionGroupId);
                }
                if (elementData->m_nameCrc == AZ_CRC("Trigger", 0x1a6b0f5d))
                {
                    command->SetIsTrigger(colliderConfig->m_isTrigger);
                }
                if (elementData->m_nameCrc == AZ_CRC("Position", 0x462ce4f5))
                {
                    command->SetPosition(colliderConfig->m_position);
                }
                if (elementData->m_nameCrc == AZ_CRC("Rotation", 0x297c98f1))
                {
                    command->SetRotation(colliderConfig->m_rotation);
                }
                if (elementData->m_nameCrc == AZ_CRC("MaterialSelection", 0xfebd6d15))
                {
                    command->SetMaterial(colliderConfig->m_materialSelection);
                }
                if (elementData->m_nameCrc == AZ_CRC("ColliderTag", 0x5e2963ad))
                {
                    command->SetTag(colliderConfig->m_tag);
                    CommandSimulatedObjectHelpers::ReplaceTag(actor, colliderType, /*oldTag=*/command->GetOldTag().value(), /*newTag=*/colliderConfig->m_tag, m_commandGroup);
                }
            }
            // Box
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::BoxShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::BoxShapeConfiguration* boxShapeConfig = static_cast<Physics::BoxShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Configuration", 0xa5e2a5d7))
                {
                    command->SetDimensions(boxShapeConfig->m_dimensions);
                }
            }
            // Capsule
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::CapsuleShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::CapsuleShapeConfiguration* capsuleShapeConfig = static_cast<Physics::CapsuleShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Radius", 0x3b7c6e5a))
                {
                    command->SetRadius(capsuleShapeConfig->m_radius);
                }
                if (elementData->m_nameCrc == AZ_CRC("Height", 0xf54de50f))
                {
                    command->SetHeight(capsuleShapeConfig->m_height);
                }
            }
            // Sphere
            else if (serializeContext->CanDowncast(classData->m_typeId, azrtti_typeid<Physics::SphereShapeConfiguration>(), classData->m_azRtti, nullptr))
            {
                const Physics::SphereShapeConfiguration* sphereShapeConfig = static_cast<Physics::SphereShapeConfiguration*>(parentDataNode->GetInstance(instanceIndex));
                if (elementData->m_nameCrc == AZ_CRC("Radius", 0x3b7c6e5a))
                {
                    command->SetRadius(sphereShapeConfig->m_radius);
                }
            }
        }

        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(m_commandGroup, result);
        m_commandGroup.Clear();
    }

    ///////////////////////////////////////////////////////////////////////////

    ColliderWidget::ColliderWidget(QIcon* icon, QWidget* parent, AZ::SerializeContext* serializeContext)
        : AzQtComponents::Card(parent)
        , m_propertyNotify(AZStd::make_unique<ColliderPropertyNotify>(this))
        , m_icon(icon)
    {
        m_editor = new EMotionFX::ObjectEditor(serializeContext, m_propertyNotify.get(), this);
        setContentWidget(m_editor);
        setExpanded(true);

        connect(this, &AzQtComponents::Card::contextMenuRequested, this, &ColliderWidget::OnCardContextMenu);
    }

    void ColliderWidget::Update(Actor* actor, Node* joint, size_t colliderIndex, PhysicsSetup::ColliderConfigType colliderType, const AzPhysics::ShapeColliderPair& collider)
    {
        m_actor = actor;
        m_joint = joint;
        m_colliderIndex = colliderIndex;
        m_colliderType = colliderType;

        if (!collider.first || !collider.second)
        {
            m_editor->ClearInstances(true);
            m_collider = AzPhysics::ShapeColliderPair();
            return;
        }

        if (collider == m_collider)
        {
            m_editor->InvalidateAll();
            return;
        }

        const AZ::TypeId& shapeType = collider.second->RTTI_GetType();
        m_editor->ClearInstances(false);
        m_editor->AddInstance(collider.second.get(), shapeType);
        m_editor->AddInstance(collider.first.get(), collider.first->RTTI_GetType());

        m_collider = collider;

        AzQtComponents::CardHeader* cardHeader = header();
        cardHeader->setIcon(*m_icon);
        if (shapeType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
        {
            setTitle("Capsule");
        }
        else if (shapeType == azrtti_typeid<Physics::SphereShapeConfiguration>())
        {
            setTitle("Sphere");
        }
        else if (shapeType == azrtti_typeid<Physics::BoxShapeConfiguration>())
        {
            setTitle("Box");
        }
        else
        {
            setTitle("Unknown");
        }

        setProperty("colliderIndex", QVariant::fromValue(colliderIndex));
        setExpanded(true);
    }

    void ColliderWidget::Update()
    {
        if (!m_actor || !m_joint)
        {
            return;
        }

        m_editor->InvalidateValues();
    }

    void ColliderWidget::OnCardContextMenu(const QPoint& position)
    {
        const AzQtComponents::Card* card = static_cast<AzQtComponents::Card*>(sender());
        const size_t colliderIndex = card->property("colliderIndex").value<size_t>();

        QMenu* contextMenu = new QMenu(this);
        contextMenu->setObjectName("EMFX.ColliderContainerWidget.ContextMenu");

        QAction* copyAction = contextMenu->addAction("Copy collider");
        copyAction->setObjectName("EMFX.ColliderContainerWidget.CopyColliderAction");
        copyAction->setProperty("colliderIndex", QVariant::fromValue(colliderIndex));
        connect(copyAction, &QAction::triggered, this, &ColliderWidget::OnCopyCollider);

        QAction* pasteAction = contextMenu->addAction("Paste collider");
        pasteAction->setObjectName("EMFX.ColliderContainerWidget.PasteColliderAction");
        pasteAction->setProperty("colliderIndex", QVariant::fromValue(colliderIndex));
        connect(pasteAction, &QAction::triggered, this, &ColliderWidget::OnPasteCollider);

        const QClipboard* clipboard = QGuiApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        const QByteArray clipboardContents = mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape());
        pasteAction->setEnabled(!clipboardContents.isEmpty());

        QAction* deleteAction = contextMenu->addAction("Delete collider");
        deleteAction->setObjectName("EMFX.ColliderContainerWidget.DeleteColliderAction");
        deleteAction->setProperty("colliderIndex", QVariant::fromValue(colliderIndex));
        connect(deleteAction, &QAction::triggered, this, &ColliderWidget::OnRemoveCollider);

        QObject::connect(contextMenu, &QMenu::triggered, contextMenu, &QObject::deleteLater);

        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(position);
        }
    }

    void ColliderWidget::OnCopyCollider()
    {
        QAction* action = static_cast<QAction*>(sender());
        const size_t colliderIndex = action->property("colliderIndex").value<size_t>();

        emit CopyCollider(colliderIndex);
    }

    void ColliderWidget::OnPasteCollider()
    {
        QAction* action = static_cast<QAction*>(sender());
        const int colliderIndex = action->property("colliderIndex").toInt();

        emit PasteCollider(colliderIndex);
    }

    void ColliderWidget::OnRemoveCollider()
    {
        QAction* action = static_cast<QAction*>(sender());
        const int colliderIndex = action->property("colliderIndex").toInt();

        emit RemoveCollider(colliderIndex);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AddColliderButton::AddColliderButton(const QString& text, QWidget* parent, PhysicsSetup::ColliderConfigType copyToColliderType, const AZStd::vector<AZ::TypeId>& supportedColliderTypes)
        : QPushButton(text, parent)
        , m_supportedColliderTypes(supportedColliderTypes)
        , m_copyToColliderType(copyToColliderType)
    {
        setIcon(MysticQt::GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));
        connect(this, &QPushButton::clicked, this, &AddColliderButton::OnCreateContextMenu);
    }

    void AddColliderButton::OnCreateContextMenu()
    {
        QMenu* contextMenu = new QMenu(this);
        contextMenu->setObjectName("EMFX.AddColliderButton.ContextMenu");

        AZStd::string actionName;
        for (const AZ::TypeId& typeId : m_supportedColliderTypes)
        {
            actionName = AZStd::string::format("Add %s", GetNameForColliderType(typeId).c_str());
            QAction* addBoxAction = contextMenu->addAction(actionName.c_str());
            addBoxAction->setProperty("typeId", typeId.ToString<AZStd::string>().c_str());
            connect(addBoxAction, &QAction::triggered, this, &AddColliderButton::OnAddColliderActionTriggered);
        }

        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);

        // Add the copy from option
        contextMenu->addSeparator();
        if (m_copyToColliderType != PhysicsSetup::ColliderConfigType::Unknown)
        {
            for (int i = 0; i < PhysicsSetup::ColliderConfigType::Unknown; ++i)
            {
                const PhysicsSetup::ColliderConfigType copyFromType = static_cast<PhysicsSetup::ColliderConfigType>(i);
                if (copyFromType == m_copyToColliderType)
                {
                    continue;
                }
                const char* visualName = PhysicsSetup::GetVisualNameForColliderConfigType(copyFromType);
                QAction* copyColliderAction = contextMenu->addAction(QString("Copy from %1").arg(visualName));
                copyColliderAction->setProperty("copyFromType", i);

                const bool canCopyFrom = ColliderHelpers::CanCopyFrom(skeletonModel->GetSelectionModel().selectedIndexes(), copyFromType);
                if (canCopyFrom)
                {
                    connect(copyColliderAction, &QAction::triggered, this, &AddColliderButton::OnCopyColliderActionTriggered);
                }
                else
                {
                    copyColliderAction->setEnabled(false);
                }
            }
        }

        contextMenu->setFixedWidth(width());
        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(mapToGlobal(QPoint(0, height())));
        }
        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
    }

    void AddColliderButton::OnAddColliderActionTriggered()
    {
        QAction* action = static_cast<QAction*>(sender());
        const QByteArray typeString = action->property("typeId").toString().toUtf8();
        const AZ::TypeId& typeId = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        emit AddCollider(typeId);
    }

    void AddColliderButton::OnCopyColliderActionTriggered()
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
        const PhysicsSetup::ColliderConfigType copyFromType = static_cast<PhysicsSetup::ColliderConfigType>(action->property("copyFromType").toInt());

        ColliderHelpers::CopyColliders(selectedRowIndices, copyFromType, m_copyToColliderType);
    }

    AZStd::string AddColliderButton::GetNameForColliderType(AZ::TypeId colliderType) const
    {
        if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
        {
            return "box";
        }
        else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
        {
            return "capsule";
        }
        else if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
        {
            return "sphere";
        }

        return colliderType.ToString<AZStd::string>();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Align the layout spacing with the entity inspector.
    int ColliderContainerWidget::s_layoutSpacing = 13;

    ColliderContainerWidget::ColliderContainerWidget(const QIcon& colliderIcon, QWidget* parent)
        : QWidget(parent)
        , m_colliderIcon(colliderIcon)
    {
        m_layout = new QVBoxLayout(this);
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setMargin(0);
        m_layout->setSpacing(s_layoutSpacing);

        m_commandCallback = new ColliderEditedCallback(this, /*executePreUndo*/false);
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAdjustCollider::s_commandName, m_commandCallback);
    }

    ColliderContainerWidget::~ColliderContainerWidget()
    {
        CommandSystem::GetCommandManager()->RemoveCommandCallback(m_commandCallback, /*delFromMem=*/true);
    }

    void ColliderContainerWidget::Update(Actor* actor, Node* joint, PhysicsSetup::ColliderConfigType colliderType, const AzPhysics::ShapeColliderPairList& colliders, AZ::SerializeContext* serializeContext)
    {
        m_actor = actor;
        m_joint = joint;
        m_colliderType = colliderType;

        const size_t numColliders = colliders.size();
        size_t numAvailableColliderWidgets = m_colliderWidgets.size();

        // Create new collider widgets in case we don't have enough.
        if (numColliders > numAvailableColliderWidgets)
        {
            const int numWidgetsToCreate = static_cast<int>(numColliders) - static_cast<int>(numAvailableColliderWidgets);
            for (int i = 0; i < numWidgetsToCreate; ++i)
            {
                ColliderWidget* colliderWidget = new ColliderWidget(&m_colliderIcon, this, serializeContext);
                connect(colliderWidget, &ColliderWidget::RemoveCollider, this, &ColliderContainerWidget::RemoveCollider);
                connect(colliderWidget, &ColliderWidget::CopyCollider, this, &ColliderContainerWidget::CopyCollider);
                connect(colliderWidget, &ColliderWidget::PasteCollider, this, [this](size_t index) { PasteCollider(index, true); } );
                m_colliderWidgets.emplace_back(colliderWidget);
                m_layout->addWidget(colliderWidget, 0, Qt::AlignTop);
            }
            numAvailableColliderWidgets = m_colliderWidgets.size();
        }
        AZ_Assert(numAvailableColliderWidgets >= numColliders, "Not enough collider widgets available. Something went one with creating new ones.");

        for (size_t i = 0; i < numColliders; ++i)
        {
            ColliderWidget* colliderWidget = m_colliderWidgets[i];
            colliderWidget->Update(m_actor, m_joint, i, m_colliderType, colliders[i]);
            colliderWidget->show();
        }

        // Hide the collider widgets that are too much for the current node.
        for (size_t i = numColliders; i < numAvailableColliderWidgets; ++i)
        {
            m_colliderWidgets[i]->hide();
            m_colliderWidgets[i]->Update(nullptr, nullptr, MCORE_INVALIDINDEX32, PhysicsSetup::ColliderConfigType::Unknown, AzPhysics::ShapeColliderPair());
        }
    }

    void ColliderContainerWidget::Update()
    {
        for (ColliderWidget* colliderWidget : m_colliderWidgets)
        {
            colliderWidget->Update();
        }
    }

    void ColliderContainerWidget::Reset()
    {
        Update(nullptr, nullptr, PhysicsSetup::ColliderConfigType::Unknown, AzPhysics::ShapeColliderPairList(), nullptr);
    }

    void ColliderContainerWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData();
        const QByteArray clipboardContents = mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape());

        int ypos = event->globalY();
        int index = 0;
        int curpos = mapToGlobal({0,0}).y();
        for (const ColliderWidget* card : m_colliderWidgets)
        {
            if (!card->GetActor())
            {
                break;
            }

            curpos = card->mapToGlobal({0,0}).y();
            if (curpos > ypos)
            {
                break;
            }
            ++index;
        }

        auto menu = new QMenu(this);
        connect(menu, &QMenu::triggered, &QObject::deleteLater);

        auto pasteNewColliderAction = new QAction("Paste collider", menu);
        pasteNewColliderAction->setEnabled(!clipboardContents.isEmpty());
        connect(pasteNewColliderAction, &QAction::triggered, this, [this, index]() { PasteCollider(index, false); } );

        menu->addAction(pasteNewColliderAction);
        menu->popup(event->globalPos());
        event->accept();
    }

    QSize ColliderContainerWidget::sizeHint() const
    {
        return QWidget::sizeHint() + QSize(0, s_layoutSpacing);
    }

    void ColliderContainerWidget::RenderColliders(const AzPhysics::ShapeColliderPairList& colliders,
        const ActorInstance* actorInstance,
        const Node* node,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo,
        const MCore::RGBAColor& colliderColor)
    {
        const AZ::u32 nodeIndex = node->GetNodeIndex();
        MCommon::RenderUtil* renderUtil = renderInfo->mRenderUtil;

        for (const auto& collider : colliders)
        {
            #ifndef EMFX_SCALE_DISABLED
                const AZ::Vector3& worldScale = actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex).mScale;
            #else
                const AZ::Vector3 worldScale = AZ::Vector3::CreateOne();
            #endif

            const Transform colliderOffsetTransform(collider.first->m_position, collider.first->m_rotation);
            const Transform& actorInstanceGlobalTransform = actorInstance->GetWorldSpaceTransform();
            const Transform& emfxNodeGlobalTransform = actorInstance->GetTransformData()->GetCurrentPose()->GetModelSpaceTransform(nodeIndex);

            const Transform emfxColliderGlobalTransformNoScale = colliderOffsetTransform * emfxNodeGlobalTransform * actorInstanceGlobalTransform;

            const AZ::TypeId colliderType = collider.second->RTTI_GetType();
            if (colliderType == azrtti_typeid<Physics::SphereShapeConfiguration>())
            {
                Physics::SphereShapeConfiguration* sphere = static_cast<Physics::SphereShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: The maximum component from the node scale will be multiplied by the radius of the sphere.
                const float radius = sphere->m_radius * MCore::Max3<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()), static_cast<float>(worldScale.GetZ()));

                renderUtil->RenderWireframeSphere(radius, emfxColliderGlobalTransformNoScale.ToAZTransform(), colliderColor);
            }
            else if (colliderType == azrtti_typeid<Physics::CapsuleShapeConfiguration>())
            {
                Physics::CapsuleShapeConfiguration* capsule = static_cast<Physics::CapsuleShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: The maximum of the X/Y scale components of the node scale will be multiplied by the radius of the capsule. The Z component of the entity scale will be multiplied by the height of the capsule.
                const float radius = capsule->m_radius * MCore::Max<float>(static_cast<float>(worldScale.GetX()), static_cast<float>(worldScale.GetY()));
                const float height = capsule->m_height * static_cast<float>(worldScale.GetZ());

                renderUtil->RenderWireframeCapsule(radius, height, emfxColliderGlobalTransformNoScale.ToAZTransform(), colliderColor);
            }
            else if (colliderType == azrtti_typeid<Physics::BoxShapeConfiguration>())
            {
                Physics::BoxShapeConfiguration* box = static_cast<Physics::BoxShapeConfiguration*>(collider.second.get());

                // LY Physics scaling rules: Each component of the box dimensions will be scaled by the node's world scale.
                AZ::Vector3 dimensions = box->m_dimensions;
                dimensions *= worldScale;

                renderUtil->RenderWireframeBox(dimensions, emfxColliderGlobalTransformNoScale.ToAZTransform(), colliderColor);
            }
        }
    }

    void ColliderContainerWidget::RenderColliders(PhysicsSetup::ColliderConfigType colliderConfigType,
        const MCore::RGBAColor& defaultColor,
        const MCore::RGBAColor& selectedColor,
        EMStudio::RenderPlugin* renderPlugin,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo)
    {
        if (colliderConfigType == PhysicsSetup::Unknown || !renderPlugin || !renderInfo)
        {
            return;
        }

        MCommon::RenderUtil* renderUtil = renderInfo->mRenderUtil;
        const bool oldLightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableLighting(false);

        const AZStd::unordered_set<AZ::u32>& selectedJointIndices = EMStudio::GetManager()->GetSelectedJointIndices();

        const ActorManager* actorManager = GetEMotionFX().GetActorManager();
        const AZ::u32 actorInstanceCount = actorManager->GetNumActorInstances();
        for (AZ::u32 i = 0; i < actorInstanceCount; ++i)
        {
            const ActorInstance* actorInstance = actorManager->GetActorInstance(i);
            const Actor* actor = actorInstance->GetActor();
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
            const Physics::CharacterColliderConfiguration* colliderConfig = physicsSetup->GetColliderConfigByType(colliderConfigType);

            if (colliderConfig)
            {
                for (const Physics::CharacterColliderNodeConfiguration& nodeConfig : colliderConfig->m_nodes)
                {
                    const Node* joint = actor->GetSkeleton()->FindNodeByName(nodeConfig.m_name.c_str());
                    if (joint)
                    {
                        const bool jointSelected = selectedJointIndices.empty() || selectedJointIndices.find(joint->GetNodeIndex()) != selectedJointIndices.end();
                        const AzPhysics::ShapeColliderPairList& colliders = nodeConfig.m_shapes;
                        RenderColliders(colliders, actorInstance, joint, renderInfo, jointSelected ? selectedColor : defaultColor);
                    }
                }
            }
        }

        renderUtil->RenderLines();
        renderUtil->EnableLighting(oldLightingEnabled);
    }

    ///////////////////////////////////////////////////////////////////////////

    ColliderContainerWidget::ColliderEditedCallback::ColliderEditedCallback(ColliderContainerWidget* parent, bool executePreUndo, bool executePreCommand)
        : MCore::Command::Callback(executePreUndo, executePreCommand)
        , m_widget(parent)
    {
    }

    bool ColliderContainerWidget::ColliderEditedCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_widget->Update();
        return true;
    }

    bool ColliderContainerWidget::ColliderEditedCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        m_widget->Update();
        return true;
    }
} // namespace EMotionFX
