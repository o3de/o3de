/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/ActorInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeInfo.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/InspectorBus.h>
#include <Editor/Plugins/ColliderWidgets/JointPropertyWidget.h>
#include <Editor/Plugins/ColliderWidgets/SimulatedObjectColliderWidget.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QBoxLayout>
#include <QClipboard>
#include <QGuiApplication>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace EMotionFX
{

    JointPropertyWidget::JointPropertyWidget(QWidget* parent)
        : QWidget(parent)
    {
        auto* mainLayout = new QVBoxLayout;
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 5, 0, 0);
        mainLayout->setSpacing(0);
        auto* propertyCard = new AzQtComponents::Card;
        AzQtComponents::Card::applySectionStyle(propertyCard);
        propertyCard->setTitle("Node Properties");

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

        // create Add Component button
        m_addCollidersButton = new AddCollidersButton(propertyCard);
        m_addCollidersButton->setObjectName("EMotionFX.SkeletonOutlinerPlugin.JointPropertyWidget.addCollidersButton");
        connect(m_addCollidersButton, &AddCollidersButton::AddCollider, this, &JointPropertyWidget::OnAddCollider);
        connect(m_addCollidersButton, &AddCollidersButton::AddToRagdoll, this, &JointPropertyWidget::OnAddToRagdoll);
        auto* marginLayout = new QVBoxLayout;
        marginLayout->setContentsMargins(10, 10, 10, 10);
        marginLayout->addWidget(m_addCollidersButton);
        mainLayout->addLayout(marginLayout);

        m_filterEntityBox = new QLineEdit{ this };
        m_filterEntityBox->setPlaceholderText(tr("Search..."));
        AzQtComponents::LineEdit::applySearchStyle(m_filterEntityBox);

        auto* marginFilterEntityBoxLayout = new QHBoxLayout;
        marginFilterEntityBoxLayout->setContentsMargins(10, 10, 10, 10);
        marginFilterEntityBoxLayout->addWidget(m_filterEntityBox);
        mainLayout->addLayout(marginFilterEntityBoxLayout);

        connect(m_filterEntityBox, &QLineEdit::textChanged, this, &JointPropertyWidget::OnSearchTextChanged);

        m_clothJointWidget = new ClothJointWidget;
        m_hitDetectionJointWidget = new HitDetectionJointWidget;
        m_ragdollJointWidget = new RagdollNodeWidget;
        m_simulatedJointWidget = new SimulatedObjectColliderWidget;
        m_clothJointWidget->CreateGUI();
        m_hitDetectionJointWidget->CreateGUI();
        m_ragdollJointWidget->CreateGUI();
        m_simulatedJointWidget->CreateGUI();

        mainLayout->addWidget(m_clothJointWidget);
        mainLayout->addWidget(m_hitDetectionJointWidget);
        mainLayout->addWidget(m_ragdollJointWidget);
        mainLayout->addWidget(m_simulatedJointWidget);
    }

    JointPropertyWidget::~JointPropertyWidget()
    {
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        delete m_propertyWidget;
    }

    void JointPropertyWidget::Reset()
    {
        hide();
        m_propertyWidget->ClearInstances();
        m_propertyWidget->InvalidateAll();

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

        Node* node = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(node, &SkeletonOutlinerRequests::GetSingleSelectedNode);

        if (node && node->GetNodeIndex() != InvalidIndex)
        {
            m_nodeInfo.reset(aznew EMStudio::NodeInfo(actorInstance, node));
            m_propertyWidget->AddInstance(m_nodeInfo.get(), azrtti_typeid(m_nodeInfo.get()));
        }
        else if (actorInstance && actorInstance->GetActor())
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

        show();
        m_propertyWidget->Setup(serializeContext, nullptr, false);
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

        const AZStd::string groupName = AZStd::string::format("Add joint%s to ragdoll", indices.size() > 1 ? "s" : "");
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
    }

    void JointPropertyWidget::OnSearchTextChanged()
    {
        m_filterString = m_filterEntityBox->text();

        m_clothJointWidget->SetFilterString(m_filterString);
        m_hitDetectionJointWidget->SetFilterString(m_filterString);
        m_ragdollJointWidget->SetFilterString(m_filterString);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    //    AddCollidersButton
    //
    AddCollidersButton::AddCollidersButton(QWidget* parent)
        : QPushButton(parent)
    {
        setText("Add Property \342\226\276");
        connect(this, &QPushButton::clicked, this, &AddCollidersButton::OnCreateContextMenu);
    }

    // TODO subclass ItemModel
    enum ItemRoles
    {
        Shape = Qt::UserRole + 1,
        ConfigType = Qt::UserRole + 2,
        CopyFromType = Qt::UserRole + 3,
        PasteCopiedCollider = Qt::UserRole + 4,
        CopyToType = Qt::UserRole + 5
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

        if (skeletonModel == nullptr)
        {
            return;
        }

        auto selectedIndices = skeletonModel->GetSelectionModel().selectedIndexes();
        if (selectedIndices.empty())
        {
            AZ_Assert(false, "The Add Collider Button in JointPropertyWidget is being clicked on while there is empty selection. This button should be hidden.");
            return;
        }

        delete model;
        model = new QStandardItemModel;
        QFrame* newFrame = new QFrame{ this };
        newFrame->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
        newFrame->setFixedWidth(this->width());
        newFrame->move({ this->mapToGlobal({ 0, 0 + height() }) });
        auto* treeView = new AddCollidersPallete(newFrame);
        treeView->setModel(model);
        treeView->setObjectName("EMotionFX.SkeletonOutlinerPlugin.AddCollidersButton.TreeView");
        // Hide header for dropdown-style, single-column, tree
        treeView->header()->hide();
        connect(treeView, &QTreeView::clicked, this, &AddCollidersButton::OnAddColliderActionTriggered);
        treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        newFrame->setLayout(new QVBoxLayout);
        newFrame->layout()->addWidget(treeView);

        AZStd::string actionName;
        struct ColliderTypeInfo
        {
            PhysicsSetup::ColliderConfigType type;
            std::string name;
            QIcon icon;
        };
        auto sections = QList<ColliderTypeInfo>{
            { PhysicsSetup::ColliderConfigType::Cloth, "Cloth", QIcon(SkeletonModel::s_clothColliderIconPath) },
            { PhysicsSetup::ColliderConfigType::HitDetection, "Hit Detection", QIcon(SkeletonModel::s_hitDetectionColliderIconPath) }
        };
        bool nodeInRagdoll = ColliderHelpers::NodeHasRagdoll(selectedIndices.last());
        if (nodeInRagdoll)
        {
            sections.append({ PhysicsSetup::ColliderConfigType::Ragdoll, "Ragdoll", QIcon(SkeletonModel::s_ragdollColliderIconPath) });
        }
        for (const auto& section : sections)
        {
            auto& configType = section.type;
            auto* sectionItem = new QStandardItem{QString("%1 Collider").arg(section.name.data()) };

            for (auto& shape : m_supportedColliderTypes)
            {
                if (configType == PhysicsSetup::ColliderConfigType::Cloth)
                {
                    const bool clothSupportedShapes = shape == azrtti_typeid<Physics::SphereShapeConfiguration>() ||
                        shape == azrtti_typeid<Physics::CapsuleShapeConfiguration>();
                    if (!clothSupportedShapes)
                    {
                        continue;
                    }
                }

                auto capitalColliderTypeName = QString{ GetNameForColliderType(shape).c_str() };
                capitalColliderTypeName[0] = capitalColliderTypeName[0].toUpper();
                auto* item = new QStandardItem{ section.icon, capitalColliderTypeName };
                item->setData(shape.ToString<AZStd::string>().c_str(), ItemRoles::Shape);
                item->setData(static_cast<int>(configType), ItemRoles::ConfigType);
                sectionItem->appendRow(item);
            }
            model->appendRow(sectionItem);
        }

        if (!nodeInRagdoll)
        {
            auto ragdollItem = new QStandardItem{ QIcon{ SkeletonModel::s_ragdollColliderIconPath }, "Add to Ragdoll" };
            model->appendRow(ragdollItem);
            ragdollItem->setData(PhysicsSetup::ColliderConfigType::Ragdoll, ItemRoles::ConfigType);
        }

        // Copy from other collider type
        for (const auto& section : sections)
        {
            const auto fromType = section.type;
            const bool canCopyFrom = ColliderHelpers::CanCopyFrom(selectedIndices, fromType);
            if (!canCopyFrom)
            {
                continue;
            }
            for (const auto& innerSection : sections)
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

        // Paste a copied collider
        const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData();
        const QByteArray clipboardContents = mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape());

        if (!clipboardContents.isEmpty())
        {
            AzPhysics::ShapeColliderPair colliderPair;
            MCore::ReflectionSerializer::Deserialize(&colliderPair, mimeData->data(ColliderHelpers::GetMimeTypeForColliderShape()).data());

            for (const auto& section : sections)
            {
                auto pasteNewColliderItem = new QStandardItem(QString{"Paste as %1 Collider"}.arg(section.name.c_str()));
                pasteNewColliderItem->setData(true, ItemRoles::PasteCopiedCollider);
                pasteNewColliderItem->setData(static_cast<int>(section.type), ItemRoles::CopyToType);
                model->appendRow(pasteNewColliderItem);
            }
        }

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
            auto copyToType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::CopyToType).toInt());
            ColliderHelpers::PasteColliderFromClipboard(selectedRowIndices.last(), 0, copyToType, false);
            return;
        }
        if (index.data(ItemRoles::ConfigType).isNull())
        {
            return;
        }
        auto colliderType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::ConfigType).toInt());
        if (!index.data(ItemRoles::CopyFromType).isNull())
        {
            auto copyFromType = static_cast<PhysicsSetup::ColliderConfigType>(index.data(ItemRoles::CopyFromType).toInt());

            ColliderHelpers::CopyColliders(selectedRowIndices, copyFromType, colliderType);
            return;
        }
        auto shape = AZ::TypeId{ index.data(ItemRoles::Shape).toString().toStdString().c_str() };

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

} // namespace EMotionFX
