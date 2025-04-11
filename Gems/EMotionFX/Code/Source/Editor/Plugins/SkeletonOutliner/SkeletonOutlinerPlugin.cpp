/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/ToastNotificationConfiguration.h>
#include <UI/Notifications/ToastNotificationsView.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/ActorInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/MeshInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NamedPropertyStringValue.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/SubMeshInfo.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ReselectingTreeView.h>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QEvent>
#include <QLabel>

namespace EMotionFX
{
    SkeletonOutlinerPlugin::SkeletonOutlinerPlugin()
        : EMStudio::DockWidgetPlugin()
        , m_mainWidget(nullptr)
        , m_noSelectionLabel(nullptr)
    {
    }

    SkeletonOutlinerPlugin::~SkeletonOutlinerPlugin()
    {
        // Reset selection on close.
        if (m_skeletonModel)
        {
            m_skeletonModel->GetSelectionModel().clearSelection();
            m_skeletonModel.reset();
        }

        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_commandCallbacks.clear();

        EMotionFX::SkeletonOutlinerRequestBus::Handler::BusDisconnect();

        delete m_propertyWidget;
        m_propertyWidget = nullptr;
    }

    void SkeletonOutlinerPlugin::Reflect(AZ::ReflectContext* context)
    {
        EMStudio::NamedPropertyStringValue::Reflect(context);
        EMStudio::SubMeshInfo::Reflect(context);
        EMStudio::MeshInfo::Reflect(context);
        EMStudio::NodeInfo::Reflect(context);
        EMStudio::NodeGroupInfo::Reflect(context);
        EMStudio::ActorInfo::Reflect(context);
    }

    bool SkeletonOutlinerPlugin::Init()
    {
        m_mainWidget = new QWidget(m_dock);
        [[maybe_unused]] auto* toastNotificationsView =
            new AzToolsFramework::ToastNotificationsView(m_mainWidget, AZ_CRC_CE("SkeletonOutliner"));

        QVBoxLayout* mainLayout = new QVBoxLayout();
        m_mainWidget->setLayout(mainLayout);

        m_noSelectionLabel = new QLabel("Select an actor instance");
        m_noSelectionLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        mainLayout->addWidget(m_noSelectionLabel, 0, Qt::AlignCenter);

        m_searchWidget = new AzQtComponents::FilteredSearchWidget(m_mainWidget);
        mainLayout->addWidget(m_searchWidget);

        m_headerWidget = new QWidget();
        QHBoxLayout* nodeLayout = new QHBoxLayout();
        m_headerWidget->setLayout(nodeLayout);
        nodeLayout->setMargin(0);
        nodeLayout->setSpacing(0);
        mainLayout->addWidget(m_headerWidget);

        nodeLayout->addSpacerItem(new QSpacerItem(5, 0, QSizePolicy::Fixed));
        QLabel* NodeLabel = new QLabel("Node");
        NodeLabel->setAlignment(Qt::AlignLeft);
        nodeLayout->addWidget(NodeLabel, 0, Qt::AlignLeft);

        nodeLayout->addSpacerItem(new QSpacerItem(150, 0, QSizePolicy::Fixed));
        QLabel* simulationLabel = new QLabel("Simulation");
        nodeLayout->addWidget(simulationLabel, 0, Qt::AlignRight);
        nodeLayout->addSpacerItem(new QSpacerItem(60, 0, QSizePolicy::Fixed));

        m_skeletonModel = AZStd::make_unique<SkeletonModel>();

        m_treeView = new ReselectingTreeView();
        m_treeView->setObjectName("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");

        m_filterProxyModel = new SkeletonSortFilterProxyModel(m_skeletonModel.get(), &m_skeletonModel->GetSelectionModel(), m_treeView);
        m_filterProxyModel->setFilterKeyColumn(-1);
        m_filterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

        m_treeView->setModel(m_filterProxyModel);
        m_treeView->setSelectionModel(m_filterProxyModel->GetSelectionProxyModel());

        m_filterProxyModel->ConnectFilterWidget(m_searchWidget);

        m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_treeView->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_treeView->setExpandsOnDoubleClick(false);
        m_treeView->setMouseTracking(true);

        QHeaderView* header = m_treeView->header();
        header->setStretchLastSection(false);
        for(int i = 1; i < m_skeletonModel->columnCount()-1; ++i)
        {
            header->resizeSection(i, s_iconSize);
        }
        header->resizeSection(m_skeletonModel->columnCount() - 1, s_iconSize + 15);
        header->setSectionResizeMode(0, QHeaderView::Stretch);
        header->hide();

        m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_treeView, &QTreeView::customContextMenuRequested, this, &SkeletonOutlinerPlugin::OnContextMenu);

        connect(m_treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SkeletonOutlinerPlugin::OnSelectionChanged);

        // Connect after the tree view connected to the model.
        connect(m_skeletonModel.get(), &QAbstractItemModel::modelReset, this, &SkeletonOutlinerPlugin::Reinit);

        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &SkeletonOutlinerPlugin::OnTextFilterChanged);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, this, &SkeletonOutlinerPlugin::OnTypeFilterChanged);

        connect(m_treeView, &QAbstractItemView::entered, this, &SkeletonOutlinerPlugin::OnEntered);
        m_treeView->installEventFilter(this);

        mainLayout->addWidget(m_treeView);
        m_dock->setWidget(m_mainWidget);

        EMotionFX::SkeletonOutlinerRequestBus::Handler::BusConnect();
        Reinit();

        m_propertyWidget = new JointPropertyWidget{m_dock};
        m_propertyWidget->hide();

        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddCollider::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandRemoveCollider::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddRagdollJoint::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandRemoveRagdollJoint::s_commandName, m_commandCallbacks.back());

        return true;
    }

    void SkeletonOutlinerPlugin::Reinit()
    {
        ActorInstance* actorInstance = m_skeletonModel ? m_skeletonModel->GetActorInstance() : nullptr;
        if (actorInstance)
        {
            m_headerWidget->setVisible(true);
            m_treeView->setVisible(true);
            m_searchWidget->setVisible(true);
            m_noSelectionLabel->setVisible(false);
        }
        else
        {
            m_headerWidget->setVisible(false);
            m_treeView->setVisible(false);
            m_searchWidget->setVisible(false);
            m_noSelectionLabel->setVisible(true);
        }

        m_treeView->expandAll();
    }

    void SkeletonOutlinerPlugin::OnTextFilterChanged([[maybe_unused]] const QString& text)
    {
        m_treeView->expandAll();
    }

    void SkeletonOutlinerPlugin::OnTypeFilterChanged([[maybe_unused]] const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
    {
        m_treeView->expandAll();
    }

    void SkeletonOutlinerPlugin::OnEntered(const QModelIndex& index)
    {
        Node* hoveredNode = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
        if (hoveredNode)
        {
            SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::JointHoveredChanged, hoveredNode->GetNodeIndex());
        }
    }

    bool SkeletonOutlinerPlugin::eventFilter([[maybe_unused]] QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::Type::Leave)
        {
            SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::JointHoveredChanged, InvalidIndex);
        }
        return false;
    }

    Node* SkeletonOutlinerPlugin::GetSingleSelectedNode()
    {
        const QModelIndexList selectedIndices = m_skeletonModel->GetSelectionModel().selectedRows();
        if (selectedIndices.size() == 1)
        {
            Node* selectedNode = selectedIndices[0].data(SkeletonModel::ROLE_POINTER).value<Node*>();
            return selectedNode;
        }

        return nullptr;
    }

    QModelIndex SkeletonOutlinerPlugin::GetSingleSelectedModelIndex()
    {
        const QModelIndexList selectedIndices = m_skeletonModel->GetSelectionModel().selectedRows();
        if (selectedIndices.size() == 1)
        {
            return selectedIndices[0];
        }

        return QModelIndex();
    }

    SkeletonModel* SkeletonOutlinerPlugin::GetModel()
    {
        return m_skeletonModel.get();
    }

    void SkeletonOutlinerPlugin::DataChanged(const QModelIndex& modelIndex)
    {
        const QModelIndex proxyModelIndex = m_filterProxyModel->mapFromSource(modelIndex);
        const QModelIndex lastColumnProxyModelIndex = proxyModelIndex.sibling(proxyModelIndex.row(), m_filterProxyModel->columnCount() - 1);
        m_filterProxyModel->dataChanged(proxyModelIndex, lastColumnProxyModelIndex);

        const QModelIndex lastColumnModelIndex = modelIndex.sibling(modelIndex.row(), m_skeletonModel->columnCount() - 1);
        m_skeletonModel->dataChanged(modelIndex, lastColumnModelIndex);
    }

    void SkeletonOutlinerPlugin::DataListChanged(const QModelIndexList& modelIndexList)
    {
        for (const QModelIndex& modelIndex : modelIndexList)
        {
            DataChanged(modelIndex);
        }
    }

    AZ::Outcome<QModelIndexList> SkeletonOutlinerPlugin::GetSelectedRowIndices()
    {
        return m_treeView->selectionModel()->selectedRows();
    }

    void SkeletonOutlinerPlugin::OnSelectionChanged([[maybe_unused]] const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        QModelIndexList selectedRows = m_treeView->selectionModel()->selectedRows();
        if (selectedRows.size() == 1)
        {
            const QModelIndex& modelIndex = selectedRows[0];
            Node* selectedNode = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            Actor* selectedActor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::SingleNodeSelectionChanged, selectedActor, selectedNode);
            m_treeView->scrollTo(modelIndex);
        }
        else
        {
            SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::SingleNodeSelectionChanged, m_skeletonModel->GetActor(), nullptr);
        }

        SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::JointSelectionChanged);
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::Update, m_propertyWidget);
    }

    void SkeletonOutlinerPlugin::OnContextMenu(const QPoint& position)
    {
        const QModelIndexList selectedRowIndices = m_skeletonModel->GetSelectionModel().selectedRows();
        if (selectedRowIndices.empty())
        {
            return;
        }

        if (selectedRowIndices.size() == 1 && SkeletonModel::IndexIsRootNode(selectedRowIndices[0]))
        {
            return;
        }

        QMenu* contextMenu = new QMenu(m_mainWidget);
        contextMenu->setObjectName("EMFX.SkeletonOutlinerPlugin.ContextMenu");

        // Allow all external places to plug into the context menu.
        SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::OnContextMenu, contextMenu, selectedRowIndices);

        // Zoom to selected joints
        if (!selectedRowIndices.empty())
        {
            AZStd::vector<Node*> selectedJoints;
            selectedJoints.reserve(selectedRowIndices.size());
            for (const QModelIndex& modelIndex : selectedRowIndices)
            {
                Node* selectedJoint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
                selectedJoints.push_back(selectedJoint);
            }

            ActorInstance* selectedActorInstance = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_INSTANCE_POINTER).value<ActorInstance*>();

            QAction* zoomToJointAction = contextMenu->addAction("Zoom to selected joints");
            connect(zoomToJointAction, &QAction::triggered, this, [=]
                {
                    SkeletonOutlinerNotificationBus::Broadcast(&SkeletonOutlinerNotifications::ZoomToJoints, selectedActorInstance, selectedJoints);
                });
        }

        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(m_treeView->mapToGlobal(position));
        }
        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool SkeletonOutlinerPlugin::DataChanged(AZ::u32 actorId, const AZStd::string& jointName)
    {
        Actor* actor = GetEMotionFX().GetActorManager()->FindActorByID(actorId);
        if (!actor)
        {
            return false;
        }

        const Skeleton* skeleton = actor->GetSkeleton();
        Node* joint = skeleton->FindNodeByName(jointName);
        if (!joint)
        {
            return false;
        }

        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (!skeletonModel || skeletonModel->GetActor() != actor)
        {
            return false;
        }

        QModelIndexList modelIndices;
        modelIndices.push_back(skeletonModel->GetModelIndex(joint));
        SkeletonOutlinerRequestBus::Broadcast(&SkeletonOutlinerRequests::DataListChanged, modelIndices);

        return true;
    }

    bool SkeletonOutlinerPlugin::DataChangedCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        ParameterMixinActorId* actorIdMixin = azdynamic_cast<ParameterMixinActorId*>(command);
        ParameterMixinJointName* jointNameMixin = azdynamic_cast<ParameterMixinJointName*>(command);
        const bool firstLastCommand = commandLine.GetValueAsBool("updateUI", true);
        if (actorIdMixin && jointNameMixin && firstLastCommand)
        {
            DataChanged(actorIdMixin->GetActorId(), jointNameMixin->GetJointName());
        }
        return true;
    }

    bool SkeletonOutlinerPlugin::DataChangedCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(commandLine);
        ParameterMixinActorId* actorIdMixin = azdynamic_cast<ParameterMixinActorId*>(command);
        ParameterMixinJointName* jointNameMixin = azdynamic_cast<ParameterMixinJointName*>(command);
        if (actorIdMixin && jointNameMixin)
        {
            DataChanged(actorIdMixin->GetActorId(), jointNameMixin->GetJointName());
        }
        return true;
    }
} // namespace EMotionFX
