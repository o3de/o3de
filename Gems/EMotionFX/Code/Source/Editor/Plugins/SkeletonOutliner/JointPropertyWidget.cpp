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
#include <MCore/Source/ReflectionSerializer.h>
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
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>

namespace EMotionFX
{


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
        m_addCollidersButton->setObjectName("EMotionFX.SkeletonOutlinerPlugin.JointPropertyWidget.addCollidersButton");
        connect(m_addCollidersButton, &AddCollidersButton::AddCollider, this, &JointPropertyWidget::OnAddCollider);
        connect(m_addCollidersButton, &AddCollidersButton::AddToRagdoll, this, &JointPropertyWidget::OnAddToRagdoll);
        auto* marginLayout = new QVBoxLayout;
        marginLayout->setContentsMargins(10, 0, 10, 10);
        marginLayout->addWidget(m_addCollidersButton);
        mainLayout->addLayout(marginLayout);

        mainLayout->addWidget(m_clothJointWidget);
        mainLayout->addWidget(m_hitDetectionJointWidget);
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
        AZ::Outcome<QModelIndexList> indicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(indicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (indicesOutcome.IsSuccess())
        {
            ColliderHelpers::AddCollider(indicesOutcome.GetValue(), configType, colliderType);
        }
    }

    void JointPropertyWidget::OnAddToRagdoll()
    {
        AZ::Outcome<QModelIndexList> indicesOutcome;
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
        m_filterString = m_filterEntityBox->text();

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
        Shape = Qt::UserRole + 1,
        ConfigType = Qt::UserRole + 2,
        CopyFromType = Qt::UserRole + 3,
        PasteCopiedCollider = Qt::UserRole + 4
    };
    struct AddCollidersPallete : public QTreeView
    {
    public:
        AddCollidersPallete(QWidget* parent)
            : QTreeView(parent)
        {
        }
        QSize GetViewportSizeHint()
        {
            return QTreeView::viewportSizeHint();
        }
    };
    void AddCollidersButton::OnCreateContextMenu()
    {
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);

        if(skeletonModel == nullptr)
        {
            return;
        }
        auto selectedIndices = skeletonModel->GetSelectionModel().selectedIndexes();

        delete model;
        model = new QStandardItemModel;
        QFrame* newFrame = new QFrame{this};
        newFrame->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        newFrame->setFixedWidth(this->width());
        newFrame->move({ this->mapToGlobal({ 0, 0 }) });
        auto* treeView = new AddCollidersPallete(newFrame);
        treeView->setModel(model);
        treeView->setObjectName("EMotionFX.SkeletonOutlinerPlugin.AddCollidersButton.TreeView");
        // hide header for dropdown-style, single-column, tree
        treeView->header()->hide();
        connect(treeView, &QTreeView::clicked, this, &AddCollidersButton::OnAddColliderActionTriggered);
        treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        newFrame->setLayout(new QVBoxLayout);
        newFrame->layout()->addWidget(treeView);

        AZStd::string actionName;
        struct ColliderTypeInfo{
            PhysicsSetup::ColliderConfigType type;
            std::string name;
            QIcon icon;
        };
        auto sections = QList<ColliderTypeInfo>{
            {PhysicsSetup::ColliderConfigType::Cloth, "Cloth", QIcon(SkeletonModel::s_clothColliderIconPath)},
            {PhysicsSetup::ColliderConfigType::HitDetection, "Hit Detection", QIcon(SkeletonModel::s_hitDetectionColliderIconPath)}};
        bool nodeInRagdoll = ColliderHelpers::IsInRagdoll(selectedIndices.last());
        if (nodeInRagdoll)
        {
            sections.append({PhysicsSetup::ColliderConfigType::Ragdoll, "Ragdoll", QIcon(SkeletonModel::s_ragdollColliderIconPath)});
        }
        for (const auto& s : sections)
        {
            auto& configType = s.type;
            auto* sectionItem = new QStandardItem{ QString("Add %1 Collider").arg(s.name.data()) };

            for (auto& shape : m_supportedColliderTypes)
            {
                if (shape == azrtti_typeid<Physics::BoxShapeConfiguration>() && configType == PhysicsSetup::ColliderConfigType::Cloth)
                {
                    continue;
                }
                auto capitalColliderTypeName = QString{GetNameForColliderType(shape).c_str()};
                capitalColliderTypeName[0] = capitalColliderTypeName[0].toUpper();
                auto* item = new QStandardItem{ capitalColliderTypeName };
                item->setData(shape.ToString<AZStd::string>().c_str(), ItemRoles::Shape);
                item->setData(static_cast<int>(configType), ItemRoles::ConfigType);
                sectionItem->appendRow(item);
            }
            model->appendRow(sectionItem);
        }

        if (!nodeInRagdoll)
        {
            auto ragdollItem = new QStandardItem{ QIcon{SkeletonModel::s_ragdollColliderIconPath}, "Add to Ragdoll" };
            model->appendRow(ragdollItem);
            ragdollItem->setData(PhysicsSetup::ColliderConfigType::Ragdoll, ItemRoles::ConfigType);
        }


        // copy from other collider type
        for (const auto& s : sections)
        {
            const auto fromType = s.type;
            const bool canCopyFrom = ColliderHelpers::CanCopyFrom(selectedIndices, fromType);
            if (!canCopyFrom)
            {
                continue;
            }
            for (const auto& innerSection: sections)
            {
                if (innerSection.type == fromType)
                {
                    continue;
                }
                const auto toType = innerSection.type;
                const char* visualName = PhysicsSetup::GetVisualNameForColliderConfigType(fromType);
                const char* visualNameTo = PhysicsSetup::GetVisualNameForColliderConfigType(toType);
                actionName = AZStd::string::format("Copy from %s to %s", visualName, visualNameTo);
                auto* item = new QStandardItem{ actionName.c_str() };
                item->setData(static_cast<int>(toType), ItemRoles::ConfigType);
                item->setData(static_cast<int>(fromType), ItemRoles::CopyFromType);
                model->appendRow(item);
            }
        }
        // todo paste (if there is a copied collider)
        /*
        const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData();
        const QByteArray clipboardContents = mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape());

        auto* type = new AzPhysics::ShapeColliderPair;
        MCore::ReflectionSerializer::Deserialize(type, mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape()).data());
        auto copyFromColliderConfiguration = type->first.get();
        //pasteNewColliderAction->setEnabled(!clipboardContents.isEmpty());
        //connect(pasteNewColliderAction, &QAction::triggered, this, [this, index]() { PasteCollider(index, false); } );
        /f (!clipboardContents.isEmpty())
        {
            for (const auto& section : sections)
            {
                if (section.type == copyFromColliderConfiguration->type)
                {
                    break;
                )
                auto pasteNewColliderItem = new QStandardItem(QString{"Paste as %1 collider"}.arg(section.name.c_str()));
                pasteNewColliderItem->setData(true, ItemRoles::PasteCopiedCollider);
                model->appendRow(pasteNewColliderItem);
            }
        }*/

        newFrame->setFixedWidth(width());
        newFrame->show();
        connect(treeView, &QTreeView::clicked, newFrame, &QFrame::deleteLater);
        treeView->expandAll();
        treeView->setFixedHeight(treeView->GetViewportSizeHint().height());
    }

    void AddCollidersButton::OnAddColliderActionTriggered(const QModelIndex& index)
    {
       if (!index.isValid())
        {
            return;
        }

        AZ::Outcome<QModelIndexList> selectedRowIndicesOutcome;
        SkeletonOutlinerRequestBus::BroadcastResult(selectedRowIndicesOutcome, &SkeletonOutlinerRequests::GetSelectedRowIndices);
        if (!selectedRowIndicesOutcome.IsSuccess())
        {
            return;
        }
        const QModelIndexList selectedRowIndices = selectedRowIndicesOutcome.GetValue();
        if (selectedRowIndices.empty())
        {
            return;
        }

        if (index.data(ItemRoles::PasteCopiedCollider).value<bool>())
        {
            // todo
            // PasteCollider(index, false);
            // ColliderHelpers::PasteColliderFromClipboard(selectedRowIndices.first(), );
            return;
        }
        if (index.data(ItemRoles::ConfigType).isNull())
        {
            // TODO collapse/expand
            index.row();
            return;
        }
        auto colliderType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::ConfigType).toInt());
        if (!index.data(ItemRoles::CopyFromType).isNull())
        {
            auto copyFromType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::CopyFromType).toInt());
            // todo check if we could have less

            ColliderHelpers::CopyColliders(selectedRowIndices, copyFromType, colliderType);
            return;
        }
        auto shape = AZ::TypeId{index.data(ItemRoles::Shape).toString().toStdString().c_str()};

        if (colliderType == PhysicsSetup::ColliderConfigType::Ragdoll && index.data(ItemRoles::Shape).isNull())
        {
            emit AddToRagdoll();
            return;
        }

        emit AddCollider(colliderType, shape);
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
