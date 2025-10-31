/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/DebugDraw.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/ColliderWidgets/SimulatedObjectColliderWidget.h>
#include <Editor/ReselectingTreeView.h>
#include <Editor/SimulatedObjectHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Integration/Rendering/RenderActorSettings.h>
#include <MCore/Source/AzCoreConversions.h>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QMessageBox>

namespace EMotionFX
{
    int SimulatedObjectWidget::s_jointLabelSpacing = 17;

    SimulatedObjectWidget::SimulatedObjectWidget()
        : EMStudio::DockWidgetPlugin()
    {
        m_actionManager = AZStd::make_unique<EMStudio::SimulatedObjectActionManager>();
    }

    SimulatedObjectWidget::~SimulatedObjectWidget()
    {
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            CommandSystem::GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_commandCallbacks.clear();

        if (m_simulatedObjectInspectorDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(m_simulatedObjectInspectorDock);
            delete m_simulatedObjectInspectorDock;
        }

        SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
        SimulatedObjectRequestBus::Handler::BusDisconnect();
        ActorEditorNotificationBus::Handler::BusDisconnect();
    }

    bool SimulatedObjectWidget::Init()
    {
        m_noSelectionWidget = new QLabel("Add a simulated object first, then add the joints you want to simulate to the object and customize the simulation settings.");
        m_noSelectionWidget->setWordWrap(true);

        m_simulatedObjectModel = AZStd::make_unique<SimulatedObjectModel>();
        m_treeView = new ReselectingTreeView();
        m_treeView->setObjectName("EMFX.SimulatedObjectWidget.TreeView");
        m_treeView->setModel(m_simulatedObjectModel.get());
        m_treeView->setSelectionModel(m_simulatedObjectModel->GetSelectionModel());
        m_treeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
        m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        m_treeView->setExpandsOnDoubleClick(true);
        m_treeView->expandAll();
        connect(m_treeView, &QTreeView::customContextMenuRequested, this, static_cast<void (SimulatedObjectWidget::*)(const QPoint&)>(&SimulatedObjectWidget::OnContextMenu));
        connect(m_simulatedObjectModel.get(), &QAbstractItemModel::modelReset, m_treeView, &QTreeView::expandAll);
        connect(m_simulatedObjectModel->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            const QModelIndexList& selectedIndices = m_simulatedObjectModel->GetSelectionModel()->selectedRows();
            if (selectedIndices.empty())
            {
                EMStudio::GetManager()->SetSelectedJointIndices({});
            }
            else
            {
                AZStd::unordered_set<size_t> selectedJointIndices;
                for (const QModelIndex& index : selectedIndices)
                {
                    const SimulatedJoint* joint = index.data(SimulatedObjectModel::ROLE_JOINT_PTR).value<SimulatedJoint*>();
                    if (joint)
                    {
                        selectedJointIndices.emplace(joint->GetSkeletonJointIndex());
                    }
                    else
                    {
                        const SimulatedObject* object = index.data(SimulatedObjectModel::ROLE_OBJECT_PTR).value<SimulatedObject*>();
                        if (object)
                        {
                            for (const auto& jointInObject : object->GetSimulatedJoints())
                            {
                                selectedJointIndices.emplace(jointInObject->GetSkeletonJointIndex());
                            }
                        }
                    }
                }
                EMStudio::GetManager()->SetSelectedJointIndices(selectedJointIndices);
            }
        });

        m_addSimulatedObjectButton = new QPushButton("Add simulated object");
        m_addSimulatedObjectButton->setObjectName("addSimulatedObjectButton");

        connect(m_addSimulatedObjectButton, &QPushButton::clicked, this, [this]()
        {
            m_actionManager->OnAddNewObjectAndAddJoints(m_actor, /*selectedJoints=*/{}, /*addChildJoints=*/false, m_dock);
        });

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_selectionWidget = new QWidget();
        QVBoxLayout* selectionLayout = new QVBoxLayout(m_selectionWidget);
        selectionLayout->addWidget(m_treeView);

        m_mainWidget = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(m_mainWidget);
        mainLayout->addWidget(m_addSimulatedObjectButton);


        // Add to simulated object button
        AddToSimulatedObjectButton* addObjectButtonn = new AddToSimulatedObjectButton("Add to simulated object", m_dock);
        selectionLayout->addWidget(addObjectButtonn);

        // Add collider button
        AddColliderButton* addColliderButton = new AddColliderButton("Add simulated object collider", m_dock,
                                                                     PhysicsSetup::ColliderConfigType::SimulatedObjectCollider,
                                                                     { azrtti_typeid<Physics::CapsuleShapeConfiguration>(),
                                                                       azrtti_typeid<Physics::SphereShapeConfiguration>() });
        addColliderButton->setObjectName("EMFX.SimulatedObjectColliderWidget.AddColliderButton");

        connect(addColliderButton, &AddColliderButton::AddCollider, this, &SimulatedObjectWidget::OnAddColliderByType);
        selectionLayout->addWidget(addColliderButton);

        m_instruction1 = new QLabel("To simulated the selected joint, add it to a Simulated Object by clicking on the \"Add to Simulated Object\" button above", m_dock);
        m_instruction1->setWordWrap(true);
        m_instruction2 = new QLabel("If you want the selected joint to collide against a Simulated Object, add a collider to the selected joint, and then set up the \"Collide with\" settings under the Simulated Object", m_dock);
        m_instruction2->setWordWrap(true);
        selectionLayout->addWidget(m_instruction1);
        selectionLayout->addWidget(m_instruction2);

        mainLayout->addWidget(m_noSelectionWidget);
        mainLayout->addWidget(m_selectionWidget, /*stretch=*/1);
        mainLayout->addStretch();

        m_dock->setWidget(m_mainWidget);

        m_simulatedObjectInspectorDock = new AzQtComponents::StyledDockWidget("Simulated Object Inspector", m_dock);
        m_simulatedObjectInspectorDock->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        m_simulatedObjectInspectorDock->setObjectName("EMFX.SimulatedObjectWidget.SimulatedObjectInspectorDock");
        m_simulatedJointWidget = new SimulatedJointWidget(this);
        m_simulatedObjectInspectorDock->setWidget(m_simulatedJointWidget);

        QMainWindow* mainWindow = EMStudio::GetMainWindow();
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_simulatedObjectInspectorDock);

        // Check if there is already an actor selected.
        ActorEditorRequestBus::BroadcastResult(m_actorInstance, &ActorEditorRequests::GetSelectedActorInstance);
        if (m_actorInstance)
        {
            // Only need to set the actor instance on the model, as this function also set the actor.
            m_simulatedObjectModel->SetActorInstance(m_actorInstance);
            m_actor = m_actorInstance->GetActor();
        }
        else
        {
            ActorEditorRequestBus::BroadcastResult(m_actor, &ActorEditorRequests::GetSelectedActor);
            m_simulatedObjectModel->SetActor(m_actor);
        }

        Reinit();

        // Register command callback
        m_commandCallbacks.emplace_back(new DataChangedCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSimulatedObject::s_commandName, m_commandCallbacks.back());
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSimulatedJoints::s_commandName, m_commandCallbacks.back());
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandRemoveSimulatedObject::s_commandName, m_commandCallbacks.back());
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandRemoveSimulatedJoints::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new AddSimulatedObjectCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSimulatedObject::s_commandName, m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new AddSimulatedJointsCallback(/*executePreUndo*/ false));
        CommandSystem::GetCommandManager()->RegisterCommandCallback(CommandAddSimulatedJoints::s_commandName, m_commandCallbacks.back());

        // Buses
        SkeletonOutlinerNotificationBus::Handler::BusConnect();
        SimulatedObjectRequestBus::Handler::BusConnect();
        ActorEditorNotificationBus::Handler::BusConnect();

        return true;
    }

    void SimulatedObjectWidget::ActorSelectionChanged(Actor* actor)
    {
        m_actor = actor;
        m_simulatedObjectModel->SetActor(actor);
        Reinit();
    }

    void SimulatedObjectWidget::ActorInstanceSelectionChanged(EMotionFX::ActorInstance* actorInstance)
    {
        m_actorInstance = actorInstance;
        m_actor = nullptr;
        if (m_actorInstance)
        {
            m_actor = m_actorInstance->GetActor();
        }
        m_simulatedObjectModel->SetActorInstance(actorInstance);
        Reinit();
    }

    void SimulatedObjectWidget::Reinit()
    {
        const bool showSelectionWidget = m_actor ? (m_actor->GetSimulatedObjectSetup()->GetNumSimulatedObjects() != 0) : false;
        m_noSelectionWidget->setVisible(!showSelectionWidget);
        m_selectionWidget->setVisible(showSelectionWidget);
        m_simulatedJointWidget->UpdateDetailsView(QItemSelection(), QItemSelection());
        m_addSimulatedObjectButton->setVisible(m_actorInstance != nullptr);
    }

    QModelIndexList SimulatedObjectWidget::GetSelectedModelIndices() const
    {
        QModelIndexList selectedModelIndices;
        SkeletonModel* skeletonModel = nullptr;
        SkeletonOutlinerRequestBus::BroadcastResult(skeletonModel, &SkeletonOutlinerRequests::GetModel);
        if (skeletonModel)
        {
            selectedModelIndices = skeletonModel->GetSelectionModel().selectedRows();
        }

        return selectedModelIndices;
    }

    SimulatedObjectModel* SimulatedObjectWidget::GetSimulatedObjectModel() const
    {
        return m_simulatedObjectModel.get();
    }

    SimulatedJointWidget* SimulatedObjectWidget::GetSimulatedJointWidget() const
    {
        return m_simulatedJointWidget;
    }

    void SimulatedObjectWidget::ScrollTo(const QModelIndex& index)
    {
        m_treeView->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
    }

    // Called when right-clicked the simulated object widget.
    void SimulatedObjectWidget::OnContextMenu(const QPoint& position)
    {
        const QModelIndexList& selectedIndices = m_treeView->selectionModel()->selectedRows(0);
        const QModelIndex currentIndex = m_treeView->currentIndex();
        if (!currentIndex.isValid())
        {
            return;
        }

        QMenu* contextMenu = new QMenu(m_mainWidget);
        contextMenu->setObjectName("EMFX.SimulatedObjectWidget.ContextMenu");

        const bool isJoint = currentIndex.data(SimulatedObjectModel::ROLE_JOINT_BOOL).toBool();
        if (isJoint)
        {
            if (selectedIndices.count() == 1)
            {
                QAction* removeJoint = contextMenu->addAction("Remove joint");
                connect(removeJoint, &QAction::triggered, [this, currentIndex]() { OnRemoveSimulatedJoint(currentIndex, false); });

                QAction* removeJointAndChildren = contextMenu->addAction("Remove joint and children");
                connect(removeJointAndChildren, &QAction::triggered, [this, currentIndex]() { OnRemoveSimulatedJoint(currentIndex, true); });
            }
            else
            {
                QAction* removeJoints = contextMenu->addAction("Remove joints");
                connect(removeJoints, &QAction::triggered, [this, selectedIndices]() { OnRemoveSimulatedJoints(selectedIndices); });
            }
        }
        else
        {
            QAction* removeObject = contextMenu->addAction("Remove object");
            connect(removeObject, &QAction::triggered, [this, currentIndex]() { OnRemoveSimulatedObject(currentIndex); });
        }

        if (!contextMenu->isEmpty())
        {
            contextMenu->popup(m_treeView->mapToGlobal(position));
        }
        connect(contextMenu, &QMenu::triggered, contextMenu, &QMenu::deleteLater);
    }

    void SimulatedObjectWidget::OnRemoveSimulatedObject(const QModelIndex& objectIndex)
    {
        SimulatedObjectHelpers::RemoveSimulatedObject(objectIndex);
    }

    void SimulatedObjectWidget::OnRemoveSimulatedJoint(const QModelIndex& jointIndex, bool removeChildren)
    {
        SimulatedObjectHelpers::RemoveSimulatedJoint(jointIndex, removeChildren);
    }

    void SimulatedObjectWidget::OnRemoveSimulatedJoints(const QModelIndexList& jointIndices)
    {
        // We don't give the option to remove children when multiple joints are selected.
        SimulatedObjectHelpers::RemoveSimulatedJoints(jointIndices, false);
    }

    void SimulatedObjectWidget::OnAddCollider()
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

        QAction* action = static_cast<QAction*>(sender());
        const QByteArray typeString = action->property("typeId").toString().toUtf8();
        const AZ::TypeId& colliderType = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        ColliderHelpers::AddCollider(selectedRowIndices, PhysicsSetup::SimulatedObjectCollider, colliderType);
    }

    void SimulatedObjectWidget::OnAddColliderByType(const AZ::TypeId& colliderType)
    {
        ColliderHelpers::AddCollider(GetSelectedModelIndices(), PhysicsSetup::SimulatedObjectCollider, colliderType);
    }

    void SimulatedObjectWidget::OnClearColliders()
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

        ColliderHelpers::ClearColliders(selectedRowIndices, PhysicsSetup::SimulatedObjectCollider);
    }

    // Called when right-clicked the skeleton outliner widget.
    void SimulatedObjectWidget::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
    {
        if (selectedRowIndices.empty())
        {
            return;
        }

        const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const SimulatedObjectSetup* simulatedObjectSetup = actor->GetSimulatedObjectSetup().get();
        if (!simulatedObjectSetup)
        {
            AZ_Assert(false, "Expected a simulated object setup on the actor.");
            return;
        }

        AZStd::unordered_set<const SimulatedObject*> addToCandidates;
        for (const QModelIndex& index : selectedRowIndices)
        {
            const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            for (const SimulatedObject* object : simulatedObjectSetup->GetSimulatedObjects())
            {
                if (!object->FindSimulatedJointBySkeletonJointIndex(joint->GetNodeIndex()))
                {
                    addToCandidates.emplace(object);
                }
            }
        }

        QMenu* addToSimulatedObjectMenu = menu->addMenu("Add to simulated object");
        if (!addToCandidates.empty())
        {
            for (const SimulatedObject* object : addToCandidates)
            {
                QAction* openItem = addToSimulatedObjectMenu->addAction(object->GetName().c_str());
                connect(openItem, &QAction::triggered, [this, selectedRowIndices, simulatedObjectSetup, object]() {
                    const bool addChildren = (QMessageBox::question(this->GetDockWidget(),
                        "Add children of joints?", "Add all children of selected joints to the simulated object?") == QMessageBox::Yes);
                    SimulatedObjectHelpers::AddSimulatedJoints(selectedRowIndices, simulatedObjectSetup->FindSimulatedObjectIndex(object).GetValue(), addChildren);
                });
            }
            addToSimulatedObjectMenu->addSeparator();
        }
        connect(addToSimulatedObjectMenu->addAction("New simulated object..."), &QAction::triggered, this, [this, selectedRowIndices]() {
            const bool addChildren = (QMessageBox::question(this->GetDockWidget(),
                "Add children of joints?", "Add all children of selected joints to the simulated object?") == QMessageBox::Yes);
            m_actionManager->OnAddNewObjectAndAddJoints(m_actor, selectedRowIndices, addChildren, m_dock);
        });
        menu->addSeparator();

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        if (!physicsSetup)
        {
            return;
        }

        if (ColliderHelpers::AreCollidersReflected())
        {
            if (selectedRowIndices.count() > 0)
            {
                QMenu* addColliderMenu = menu->addMenu("Add collider");

                QAction* addCapsuleAction = addColliderMenu->addAction("Capsule");
                addCapsuleAction->setProperty("typeId", azrtti_typeid<Physics::CapsuleShapeConfiguration>().ToString<AZStd::string>().c_str());
                connect(addCapsuleAction, &QAction::triggered, this, &SimulatedObjectWidget::OnAddCollider);

                QAction* addSphereAction = addColliderMenu->addAction("Sphere");
                addSphereAction->setProperty("typeId", azrtti_typeid<Physics::SphereShapeConfiguration>().ToString<AZStd::string>().c_str());
                connect(addSphereAction, &QAction::triggered, this, &SimulatedObjectWidget::OnAddCollider);


                ColliderHelpers::AddCopyFromMenu(this, menu, PhysicsSetup::ColliderConfigType::SimulatedObjectCollider, selectedRowIndices);
            }

            const bool anySelectedJointHasCollider = AZStd::any_of(selectedRowIndices.begin(), selectedRowIndices.end(), [](const QModelIndex& modelIndex)
            {
                return modelIndex.data(SkeletonModel::ROLE_SIMULATED_OBJECT_COLLIDER).toBool();
            });

            if (anySelectedJointHasCollider)
            {
                QAction* removeCollidersAction = menu->addAction("Remove colliders");
                removeCollidersAction->setObjectName("EMFX.SimulatedObjectWidget.RemoveCollidersAction");
                connect(removeCollidersAction, &QAction::triggered, this, &SimulatedObjectWidget::OnClearColliders);
            }
            menu->addSeparator();
        }
    }

    void SimulatedObjectWidget::UpdateWidget()
    {
        Reinit();
    }

    bool SimulatedObjectWidget::DataChangedCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        EMotionFX::SimulatedObjectRequestBus::Broadcast(&EMotionFX::SimulatedObjectRequests::UpdateWidget);
        return true;
    }

    bool SimulatedObjectWidget::DataChangedCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        EMotionFX::SimulatedObjectRequestBus::Broadcast(&EMotionFX::SimulatedObjectRequests::UpdateWidget);
        return true;
    }

    bool SimulatedObjectWidget::AddSimulatedObjectCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandAddSimulatedObject* addSimulatedObjectCommand = static_cast<CommandAddSimulatedObject*>(command);
        const size_t objectIndex = addSimulatedObjectCommand->GetObjectIndex();

        SimulatedObjectWidget* simulatedObjectPlugin = static_cast<SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(SimulatedObjectWidget::CLASS_ID));
        if (simulatedObjectPlugin)
        {
            const QModelIndex modelIndex = simulatedObjectPlugin->GetSimulatedObjectModel()->GetModelIndexByObjectIndex(objectIndex);
            simulatedObjectPlugin->GetSimulatedObjectModel()->GetSelectionModel()->select(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            simulatedObjectPlugin->ScrollTo(modelIndex);
        }
        return true;
    }

    bool SimulatedObjectWidget::AddSimulatedObjectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    bool SimulatedObjectWidget::AddSimulatedJointsCallback::Execute(MCore::Command* command, [[maybe_unused]] const MCore::CommandLine& commandLine)
    {
        CommandAddSimulatedJoints* addSimulatedJointsCommand = static_cast<CommandAddSimulatedJoints*>(command);
        const size_t objectIndex = addSimulatedJointsCommand->GetObjectIndex();
        const AZStd::vector<size_t>& jointIndices = addSimulatedJointsCommand->GetJointIndices();

        SimulatedObjectWidget* simulatedObjectPlugin = static_cast<SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(SimulatedObjectWidget::CLASS_ID));
        if (simulatedObjectPlugin)
        {
            QItemSelection selection;
            simulatedObjectPlugin->GetSimulatedObjectModel()->AddJointsToSelection(selection, objectIndex, jointIndices);
            simulatedObjectPlugin->GetSimulatedObjectModel()->GetSelectionModel()->select(selection, QItemSelectionModel::Current | QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (!selection.empty())
            {
                const QModelIndexList list = selection.indexes();
                simulatedObjectPlugin->ScrollTo(list[0]);
            }
        }
        return true;
    }

    bool SimulatedObjectWidget::AddSimulatedJointsCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZ_UNUSED(command);
        AZ_UNUSED(commandLine);
        return true;
    }

    void SimulatedObjectWidget::Render(EMotionFX::ActorRenderFlags renderFlags)
    {
        if (!m_actor || !m_actorInstance)
        {
            return;
        }

        const AZStd::unordered_set<size_t>& selectedJointIndices = EMStudio::GetManager()->GetSelectedJointIndices();
        if (AZ::RHI::CheckBitsAny(renderFlags, EMotionFX::ActorRenderFlags::SimulatedJoints) && !selectedJointIndices.empty())
        {
            // Render the joint radius.
            const size_t actorInstanceCount = GetActorManager().GetNumActorInstances();
            for (size_t actorInstanceIndex = 0; actorInstanceIndex < actorInstanceCount; ++actorInstanceIndex)
            {
                ActorInstance* actorInstance = GetActorManager().GetActorInstance(actorInstanceIndex);
                const Actor* actor = actorInstance->GetActor();
                const SimulatedObjectSetup* setup = actor->GetSimulatedObjectSetup().get();
                if (!setup)
                {
                    AZ_Assert(false, "Expected a simulated object setup on the actor instance.");
                    return;
                }

                const size_t objectCount = setup->GetNumSimulatedObjects();
                for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
                {
                    const SimulatedObject* object = setup->GetSimulatedObject(objectIndex);
                    const size_t simulatedJointCount = object->GetNumSimulatedJoints();
                    for (size_t simulatedJointIndex = 0; simulatedJointIndex < simulatedJointCount; ++simulatedJointIndex)
                    {
                        const SimulatedJoint* simulatedJoint = object->GetSimulatedJoint(simulatedJointIndex);
                        const size_t skeletonJointIndex = simulatedJoint->GetSkeletonJointIndex();
                        if (selectedJointIndices.find(skeletonJointIndex) != selectedJointIndices.end())
                        {
                            RenderJointRadius(simulatedJoint, actorInstance, AZ::Color(1.0f, 0.0f, 1.0f, 1.0f));
                        }
                    }
                }
            }
        }
    }

    void SimulatedObjectWidget::RenderJointRadius(const SimulatedJoint* joint, ActorInstance* actorInstance, const AZ::Color& color)
    {
#ifndef EMFX_SCALE_DISABLED
        const float scale = actorInstance->GetWorldSpaceTransform().m_scale.GetX();
#else
        const float scale = 1.0f;
#endif

        const float radius = joint->GetCollisionRadius() * scale;
        if (radius <= AZ::Constants::FloatEpsilon)
        {
            return;
        }

        AZ_Assert(joint->GetSkeletonJointIndex() != InvalidIndex, "Expected skeletal joint index to be valid.");
        const EMotionFX::Transform jointTransform =
            actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(joint->GetSkeletonJointIndex());

        AZ::s32 viewportId = -1;
        EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, viewportId);
        AzFramework::DebugDisplayRequests* debugDisplay = nullptr;
        debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
        if (!debugDisplay)
        {
            return;
        }

        debugDisplay->SetColor(color);
        debugDisplay->DrawWireSphere(jointTransform.m_position, radius);
    }
} // namespace EMotionFX
