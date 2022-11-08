/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Plugins/SkeletonOutliner/JointPropertyWidget.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/ActorInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <Editor/SkeletonModel.h>
#include <Editor/InspectorBus.h>
#include <Editor/ColliderHelpers.h>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QItemSelectionModel>
#include <QBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QLineEdit>

namespace EMotionFX
{
    struct HLine : public QVBoxLayout
    {
        HLine(QWidget* parent = nullptr)
        {
            setContentsMargins(0, 5, 0, 5);
            auto* frame = new QFrame{parent};
            frame->setFrameShape(QFrame::HLine);
            frame->setFrameShadow(QFrame::Sunken);
            addWidget(frame);
        }
    };


    JointPropertyWidget::JointPropertyWidget(QWidget* parent)
        :QWidget(parent)
    {
        auto* mainLayout = new QVBoxLayout;
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 5, 0, 0);
        auto* propertyCard = new AzQtComponents::Card;
        AzQtComponents::Card::applySectionStyle(propertyCard);
        propertyCard->setTitle("Node Attributes");

        mainLayout->addWidget(propertyCard);

        // add the node attributes widget
        m_propertyWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyWidget->setObjectName("EMFX.Joint.ReflectedPropertyEditor.PropertyWidget");

        propertyCard->setContentWidget(m_propertyWidget);

        setLayout(mainLayout);

        // Connect to the model.
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            connect(skeletonModel, &SkeletonModel::dataChanged, this, &JointPropertyWidget::Reset);
            connect(skeletonModel, &SkeletonModel::modelReset, this, &JointPropertyWidget::Reset);
            connect(&skeletonModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &JointPropertyWidget::Reset);
        }

        m_filterEntityBox = new QLineEdit{ this };
        mainLayout->addWidget(m_filterEntityBox);
        connect(m_filterEntityBox, &QLineEdit::textChanged, this, &JointPropertyWidget::OnSearchTextChanged);

        m_clothJointWidget = new ClothJointWidget;
        m_hitDetectionJointWidget = new HitDetectionJointWidget;
        m_ragdollJointWidget = new RagdollNodeWidget;
        m_clothJointWidget->CreateGUI();
        m_hitDetectionJointWidget->CreateGUI();
        m_ragdollJointWidget->CreateGUI();

        // create Add Component button
        m_addCollidersButton = new AddCollidersButton(propertyCard);
        connect(m_addCollidersButton, &AddCollidersButton::AddCollider, this, &JointPropertyWidget::OnAddCollider);
        connect(m_addCollidersButton, &AddCollidersButton::AddToRagdoll, this, &JointPropertyWidget::OnAddToRagdoll);
        auto* marginLayout = new QVBoxLayout{this};
        marginLayout->setContentsMargins(10, 0, 10, 10);
        marginLayout->addWidget(m_addCollidersButton);
        mainLayout->addLayout(marginLayout);

        mainLayout->addLayout(new HLine);
        mainLayout->addWidget(m_clothJointWidget);
        mainLayout->addLayout(new HLine);
        mainLayout->addWidget(m_hitDetectionJointWidget);
        mainLayout->addLayout(new HLine);
        mainLayout->addWidget(m_ragdollJointWidget);
    }

    JointPropertyWidget::~JointPropertyWidget()
    {
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        delete m_propertyWidget;
    }

    void JointPropertyWidget::Reset()
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (!skeletonModel)
        {
            return;
        }

        auto* actorInstance = skeletonModel->GetActorInstance();
        if (!actorInstance)
        {
            return;
        }

        m_propertyWidget->ClearInstances();
        m_propertyWidget->InvalidateAll();

        Node* node = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(node, &SkeletonOutlinerRequests::GetSingleSelectedNode);

        if (node && node->GetNodeIndex() != InvalidIndex)
        {
            m_nodeInfo.reset(aznew EMStudio::NodeInfo(actorInstance, node));
            m_propertyWidget->AddInstance(m_nodeInfo.get(), azrtti_typeid(m_nodeInfo.get()));
        }
        else if(actorInstance && actorInstance->GetActor())
        {
            m_actorInfo.reset(aznew EMStudio::ActorInfo(actorInstance));
            m_propertyWidget->AddInstance(m_actorInfo.get(), azrtti_typeid(m_actorInfo.get()));
        }
        else
        {
            return;
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        m_propertyWidget->Setup(serializeContext, nullptr, false);
        m_propertyWidget->show();
        m_propertyWidget->ExpandAll();
        m_propertyWidget->InvalidateAll();
    }

    void JointPropertyWidget::OnAddCollider(PhysicsSetup::ColliderConfigType configType, AZ::TypeId colliderType)
    {
        AZ::Outcome<const QModelIndexList&> indicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(indicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (indicesOutcome.IsSuccess())
        {
            ColliderHelpers::AddCollider(indicesOutcome.GetValue(), configType, colliderType);
        }
    }

    void JointPropertyWidget::OnAddToRagdoll()
    {
        AZ::Outcome<const QModelIndexList&> indicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(indicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!indicesOutcome.IsSuccess())
        {
            return;
        }
        auto indices = indicesOutcome.GetValue();

        const AZStd::string groupName = AZStd::string::format("Add joint%s to ragdoll",
                indices.size() > 1 ? "s" : "");
        MCore::CommandGroup commandGroup(groupName);


        AZStd::vector<AZStd::string> jointNames;
        jointNames.reserve(indices.size());

        // All the actor pointers should be the same
        const uint32 actorId = indices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetID();

        for (const QModelIndex& selectedIndex : indices)
        {
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            jointNames.emplace_back(joint->GetNameString());
        }

        CommandRagdollHelpers::AddJointsToRagdoll(actorId, jointNames, &commandGroup);

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
//        // All the actor pointers should be the same
//        const uint32 actorId = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetID();

//        for (const QModelIndex& selectedIndex : modelIndices)
//        {
//            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

//            jointNames.emplace_back(joint->GetNameString());
//        }
//        CommandRagdollHelpers::AddJointsToRagdoll(actorId, jointNames, &commandGroup);

//        AZStd::string result;
//        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
//        {
//            AZ_Error("EMotionFX", false, result.c_str());
//        }
    }

    void JointPropertyWidget::OnSearchTextChanged()
    {
        m_filterString = m_filterEntityBox->text().toLatin1().data();

        m_clothJointWidget->SetFilterString(m_filterString);
        m_hitDetectionJointWidget->SetFilterString(m_filterString);
        m_ragdollJointWidget->SetFilterString(m_filterString);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //    AddCollidersButton
    //
    AddCollidersButton::AddCollidersButton(QWidget *parent)
        :QPushButton(parent)
    {
        setText("Add Collider");
        connect(this, &QPushButton::clicked, this, &AddCollidersButton::OnCreateContextMenu);
    }

    // TODO subclass ItemModel
    enum ItemRoles
    {
        TypeId = Qt::UserRole + 1,
        ConfigType = Qt::UserRole + 2
    };
    void AddCollidersButton::OnCreateContextMenu()
    {
        delete model;
        model = new QStandardItemModel;
        QFrame* newFrame = new QFrame{this};
        newFrame->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        newFrame->setFixedWidth(this->width());
        newFrame->move({ this->mapToGlobal({ 0, 0 }) });
        auto* treeView = new QTreeView(newFrame);
        treeView->setModel(model);
        // hide header for dropdown-style, single-column, tree
        treeView->header()->hide();
        connect(treeView, &QTreeView::clicked, this, &AddCollidersButton::OnAddColliderActionTriggered);
        treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        newFrame->setLayout(new QVBoxLayout);
        newFrame->layout()->addWidget(treeView);

        AZStd::string actionName;
        auto sections = QList<QPair<PhysicsSetup::ColliderConfigType, QString>>{{PhysicsSetup::ColliderConfigType::Cloth, "Cloth"},
                         {PhysicsSetup::ColliderConfigType::HitDetection, "Hit Detection"}};
        for (const auto& s : sections)
        {
            auto& configType = s.first;
            auto* sectionItem = new QStandardItem{ s.second.toStdString().c_str() };

            for (auto& typeId : m_supportedColliderTypes)
            {
                actionName = AZStd::string::format("Add %s %s", s.second.toStdString().c_str(), GetNameForColliderType(typeId).c_str());
                auto* item = new QStandardItem{ actionName.c_str() };
                item->setData(typeId.ToString<AZStd::string>().c_str(), ItemRoles::TypeId);
                item->setData(static_cast<int>(configType), ItemRoles::ConfigType);
                sectionItem->appendRow(item);
            }
            model->appendRow(sectionItem);
        }

        auto ragdollItem = new QStandardItem{ "Ragdoll" };
        model->appendRow(ragdollItem);
        ragdollItem->setData(PhysicsSetup::ColliderConfigType::Ragdoll, ItemRoles::ConfigType);

        // todo copy from other collider type
        // todo paste (if there is a copied collider)

        connect(treeView, &QTreeView::clicked, newFrame, &QFrame::deleteLater);
        treeView->expandAll();
        newFrame->setFixedWidth(width());
        newFrame->show();
    }

    void AddCollidersButton::OnAddColliderActionTriggered(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }
        if (index.data(ItemRoles::ConfigType).isNull())
        {
            // TODO collapse/expand
            index.row();
            return;
        }
        auto configType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::ConfigType).toInt());
        if (!index.data(ItemRoles::CopyFromType).isNull())
        {
            auto copyFromType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::CopyFromType).toInt());
            // todo check if we could have less
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

            ColliderHelpers::CopyColliders(selectedRowIndices, copyFromType, configType);
            return;
        }
        auto colliderType = AZ::TypeId{index.data(ItemRoles::TypeId).toString().toStdString().c_str()};

        if (configType == PhysicsSetup::ColliderConfigType::Ragdoll)
        {
            emit AddToRagdoll();
            return;
        }

        emit AddCollider(configType, colliderType);
    }

    AZStd::string AddCollidersButton::GetNameForColliderType(AZ::TypeId colliderType) const
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

} // ns EMotionFX
