/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/SystemBus.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/RagdollInstance.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderViewWidget.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Editor/ColliderContainerWidget.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <Editor/Plugins/Ragdoll/RagdollJointLimitWidget.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeInspectorPlugin.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <QScrollArea>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    float RagdollNodeInspectorPlugin::s_scale = 0.1f;
    AZ::u32 RagdollNodeInspectorPlugin::s_angularSubdivisions = 32;
    AZ::u32 RagdollNodeInspectorPlugin::s_radialSubdivisions = 2;

    RagdollNodeInspectorPlugin::RagdollNodeInspectorPlugin()
        : EMStudio::DockWidgetPlugin()
        , m_nodeWidget(nullptr)
    {
    }

    RagdollNodeInspectorPlugin::~RagdollNodeInspectorPlugin()
    {
        EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusDisconnect();
    }

    EMStudio::EMStudioPlugin* RagdollNodeInspectorPlugin::Clone()
    {
        RagdollNodeInspectorPlugin* newPlugin = new RagdollNodeInspectorPlugin();
        return newPlugin;
    }

    bool RagdollNodeInspectorPlugin::IsPhysXGemAvailable() const
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // TypeId of PhysX::SystemComponent
        const char* typeIDPhysXSystem = "{85F90819-4D9A-4A77-AB89-68035201F34B}";

        return serializeContext
            && serializeContext->FindClassData(AZ::TypeId::CreateString(typeIDPhysXSystem));
    }

    bool RagdollNodeInspectorPlugin::Init()
    {
        if (IsPhysXGemAvailable() && ColliderHelpers::AreCollidersReflected())
        {
            m_nodeWidget = new RagdollNodeWidget();
            m_nodeWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            m_nodeWidget->CreateGUI();

            QScrollArea* scrollArea = new QScrollArea();
            scrollArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
            scrollArea->setWidget(m_nodeWidget);
            scrollArea->setWidgetResizable(true);

            m_dock->setWidget(scrollArea);

            EMotionFX::SkeletonOutlinerNotificationBus::Handler::BusConnect();
        }
        else
        {
            m_dock->setWidget(CreateErrorContentWidget("Ragdoll editor depends on the PhysX gem. Please enable it in the Project Manager."));
        }

        return true;
    }

    void RagdollNodeInspectorPlugin::OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices)
    {
        if (selectedRowIndices.empty())
        {
            return;
        }

        const int numSelectedNodes = selectedRowIndices.count();
        int ragdollNodeCount = 0;
        for (const QModelIndex& modelIndex : selectedRowIndices)
        {
            const bool partOfRagdoll = modelIndex.data(SkeletonModel::ROLE_RAGDOLL).toBool();

            if (partOfRagdoll)
            {
                ragdollNodeCount++;
            }
        }

        QMenu* contextMenu = menu->addMenu("Ragdoll");

        if (ragdollNodeCount < numSelectedNodes)
        {
            QAction* addToRagdollAction = contextMenu->addAction("Add to ragdoll");
            connect(addToRagdollAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnAddToRagdoll);
        }

        if (ragdollNodeCount == numSelectedNodes)
        {
            QMenu* addColliderMenu = contextMenu->addMenu("Add collider");

            QAction* addBoxAction = addColliderMenu->addAction("Add box");
            addBoxAction->setProperty("typeId", azrtti_typeid<Physics::BoxShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addBoxAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnAddCollider);

            QAction* addCapsuleAction = addColliderMenu->addAction("Add capsule");
            addCapsuleAction->setProperty("typeId", azrtti_typeid<Physics::CapsuleShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addCapsuleAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnAddCollider);

            QAction* addSphereAction = addColliderMenu->addAction("Add sphere");
            addSphereAction->setProperty("typeId", azrtti_typeid<Physics::SphereShapeConfiguration>().ToString<AZStd::string>().c_str());
            connect(addSphereAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnAddCollider);
        }

        ColliderHelpers::AddCopyFromMenu(this, contextMenu, PhysicsSetup::ColliderConfigType::Ragdoll, selectedRowIndices,
            [=](PhysicsSetup::ColliderConfigType copyFrom, [[maybe_unused]] PhysicsSetup::ColliderConfigType copyTo)
            {
                CopyColliders(selectedRowIndices, copyFrom);
            });

        if (ragdollNodeCount > 0)
        {
            QAction* removeCollidersAction = contextMenu->addAction("Remove colliders");
            connect(removeCollidersAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnClearColliders);

            QAction* removeToRagdollAction = contextMenu->addAction("Remove from ragdoll");
            connect(removeToRagdollAction, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnRemoveFromRagdoll);

            QAction* pasteJointLimits = contextMenu->addAction("Paste joint limits");
            pasteJointLimits->setObjectName("EMFX.RagdollNodeInspectorPlugin.PasteJointLimitsAction");
            connect(pasteJointLimits, &QAction::triggered, this, &RagdollNodeInspectorPlugin::OnPasteJointLimits);
            pasteJointLimits->setEnabled(m_nodeWidget->HasCopiedJointLimits());
        }
    }

    bool RagdollNodeInspectorPlugin::IsNodeInRagdoll(const QModelIndex& index)
    {
        const Actor* actor = index.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        return ragdollConfig.FindNodeConfigByName(joint->GetNameString()) != nullptr;
    }


    void RagdollNodeInspectorPlugin::AddToRagdoll(const QModelIndexList& modelIndices)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Add joint%s to ragdoll",
                modelIndices.size() > 1 ? "s" : "");

        MCore::CommandGroup commandGroup(groupName);

        AZStd::vector<AZStd::string> jointNames;
        jointNames.reserve(modelIndices.size());

        // All the actor pointers should be the same
        const uint32 actorId = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetID();

        for (const QModelIndex& selectedIndex : modelIndices)
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

    void RagdollNodeInspectorPlugin::RemoveFromRagdoll(const QModelIndexList& modelIndices)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Remove joint%s from ragdoll",
                modelIndices.size() > 1 ? "s" : "");

        MCore::CommandGroup commandGroup(groupName);

        AZStd::vector<AZStd::string> jointNamesToRemove;
        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Node* selectedJoint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            jointNamesToRemove.emplace_back(selectedJoint->GetNameString());
        }
        const Actor* actor = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();

        CommandRagdollHelpers::RemoveJointsFromRagdoll(actor->GetID(), jointNamesToRemove, &commandGroup);

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void RagdollNodeInspectorPlugin::AddCollider(const QModelIndexList& modelIndices, const AZ::TypeId& colliderType)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Add collider%s to ragdoll",
                modelIndices.size() > 1 ? "s" : "");

        MCore::CommandGroup commandGroup(groupName);

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* selectedJoint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            CommandColliderHelpers::AddCollider(actor->GetID(), selectedJoint->GetNameString(), PhysicsSetup::Ragdoll, colliderType, &commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void RagdollNodeInspectorPlugin::CopyColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Copy %s collider%s to ragdoll",
                PhysicsSetup::GetStringForColliderConfigType(copyFrom),
                modelIndices.size() > 1 ? "s" : "");
        MCore::CommandGroup commandGroup(groupName);

        AZStd::vector<AZStd::string> jointNamesToAdd;
        AZStd::vector<const Node*> jointsToAdd;
        const Actor* actor = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        for (const QModelIndex& selectedIndex : modelIndices)
        {
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();

            const Physics::CharacterColliderConfiguration* copyFromColliderConfig = physicsSetup->GetColliderConfigByType(copyFrom);
            if (!copyFromColliderConfig)
            {
                continue;
            }

            Physics::CharacterColliderNodeConfiguration* copyFromNodeConfig = copyFromColliderConfig->FindNodeConfigByName(joint->GetNameString());
            if (!copyFromNodeConfig || copyFromNodeConfig->m_shapes.empty())
            {
                continue;
            }

            jointNamesToAdd.emplace_back(joint->GetNameString());
            jointsToAdd.emplace_back(joint);
        }
        CommandRagdollHelpers::AddJointsToRagdoll(actor->GetID(), jointNamesToAdd, &commandGroup);

        for (const Node* joint : jointsToAdd)
        {
            // 2. Remove the auto-added capsule and former colliders.
            CommandColliderHelpers::ClearColliders(actor->GetID(), joint->GetNameString(), PhysicsSetup::Ragdoll, &commandGroup);

            // 3. Copy colliders
            ColliderHelpers::AddCopyColliderCommandToGroup(actor, joint, copyFrom, PhysicsSetup::Ragdoll, commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void RagdollNodeInspectorPlugin::OnAddToRagdoll()
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

        AddToRagdoll(selectedRowIndices);
    }

    void RagdollNodeInspectorPlugin::OnAddCollider()
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
        const QByteArray typeString = action->property("typeId").toString().toUtf8();
        const AZ::TypeId& colliderType = AZ::TypeId::CreateString(typeString.data(), typeString.size());

        AddCollider(selectedRowIndices, colliderType);
    }

    void RagdollNodeInspectorPlugin::OnRemoveFromRagdoll()
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

        RemoveFromRagdoll(selectedRowIndices);
    }

    void RagdollNodeInspectorPlugin::OnClearColliders()
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

        ColliderHelpers::ClearColliders(selectedRowIndices, PhysicsSetup::Ragdoll);
    }

    void RagdollNodeInspectorPlugin::OnPasteJointLimits()
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

        MCore::CommandGroup group("Paste joint limits");
        for (const QModelIndex& index : selectedRowIndices)
        {
            const Actor* actor = index.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
            auto command = aznew CommandAdjustRagdollJoint(actor->GetID(), joint->GetName(), m_nodeWidget->GetCopiedJointLimits());
            group.AddCommand(command);
        }
        AZStd::string result;
        if(!CommandSystem::GetCommandManager()->ExecuteCommandGroup(group, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void RagdollNodeInspectorPlugin::Render(EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo)
    {
        EMStudio::RenderViewWidget* activeViewWidget = renderPlugin->GetActiveViewWidget();
        if (!activeViewWidget)
        {
            return;
        }

        const bool renderColliders = activeViewWidget->GetRenderFlag(EMStudio::RenderViewWidget::RENDER_RAGDOLL_COLLIDERS);
        const bool renderJointLimits = activeViewWidget->GetRenderFlag(EMStudio::RenderViewWidget::RENDER_RAGDOLL_JOINTLIMITS);
        if (!renderColliders && !renderJointLimits)
        {
            return;
        }

        MCommon::RenderUtil* renderUtil = renderInfo->m_renderUtil;
        const bool oldLightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableLighting(false);

        const size_t actorInstanceCount = GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < actorInstanceCount; ++i)
        {
            ActorInstance* actorInstance = GetActorManager().GetActorInstance(i);
            RenderRagdoll(actorInstance, renderColliders, renderJointLimits, renderPlugin, renderInfo);
        }

        renderUtil->RenderLines();
        renderUtil->EnableLighting(oldLightingEnabled);
    }

    void RagdollNodeInspectorPlugin::RenderRagdoll(ActorInstance* actorInstance, bool renderColliders, bool renderJointLimits, EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo)
    {
        const Actor* actor = actorInstance->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t numNodes = skeleton->GetNumNodes();
        const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();
        const AZStd::vector<Physics::RagdollNodeConfiguration>& ragdollNodes = ragdollConfig.m_nodes;
        const Physics::CharacterColliderConfiguration& colliderConfig = ragdollConfig.m_colliders;
        const RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();

        EMStudio::RenderOptions* renderOptions = renderPlugin->GetRenderOptions();
        const MCore::RGBAColor defaultColor = renderOptions->GetRagdollColliderColor();
        const MCore::RGBAColor selectedColor = renderOptions->GetSelectedRagdollColliderColor();

        const AZStd::unordered_set<size_t>& selectedJointIndices = EMStudio::GetManager()->GetSelectedJointIndices();

        for (size_t nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            const Node* joint = skeleton->GetNode(nodeIndex);
            const size_t jointIndex = joint->GetNodeIndex();

            AZ::Outcome<size_t> ragdollNodeIndex = AZ::Failure();
            if (ragdollInstance)
            {
                ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);
            }
            else
            {
                ragdollNodeIndex = ragdollConfig.FindNodeConfigIndexByName(joint->GetNameString());
            }

            if (!ragdollNodeIndex.IsSuccess())
            {
                continue;
            }

            const bool jointSelected = selectedJointIndices.empty() || selectedJointIndices.find(jointIndex) != selectedJointIndices.end();

            MCore::RGBAColor finalColor;
            if (jointSelected)
            {
                finalColor = selectedColor;
            }
            else
            {
                finalColor = defaultColor;
            }

            const Physics::RagdollNodeConfiguration& ragdollNode = ragdollNodes[ragdollNodeIndex.GetValue()];

            if (renderColliders)
            {
                const Physics::CharacterColliderNodeConfiguration* colliderNodeConfig = colliderConfig.FindNodeConfigByName(joint->GetNameString());
                if (colliderNodeConfig)
                {
                    const AzPhysics::ShapeColliderPairList& colliders = colliderNodeConfig->m_shapes;
                    ColliderContainerWidget::RenderColliders(colliders, actorInstance, joint, renderInfo, finalColor);
                }
            }

            if (renderJointLimits && jointSelected)
            {
                const AZStd::shared_ptr<AzPhysics::JointConfiguration>& jointLimitConfig = ragdollNode.m_jointConfig;
                if (jointLimitConfig)
                {
                    const Node* ragdollParentNode = physicsSetup->FindRagdollParentNode(joint);
                    if (ragdollParentNode)
                    {
                        RenderJointLimit(*jointLimitConfig, actorInstance, joint, ragdollParentNode, renderPlugin, renderInfo, finalColor);
                        RenderJointFrame(*jointLimitConfig, actorInstance, joint, ragdollParentNode, renderInfo, finalColor);
                    }
                }
            }
        }
    }

    void RagdollNodeInspectorPlugin::RenderJointLimit(
        const AzPhysics::JointConfiguration& configuration,
        const ActorInstance* actorInstance,
        const Node* node,
        const Node* parentNode,
        EMStudio::RenderPlugin* renderPlugin,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo,
        const MCore::RGBAColor& color)
    {
        const EMStudio::RenderOptions* renderOptions = renderPlugin->GetRenderOptions();
        const MCore::RGBAColor violatedColor = renderOptions->GetViolatedJointLimitColor();
        const size_t nodeIndex = node->GetNodeIndex();
        const size_t parentNodeIndex = parentNode->GetNodeIndex();
        const Transform& actorInstanceWorldTransform = actorInstance->GetWorldSpaceTransform();
        const Pose* currentPose = actorInstance->GetTransformData()->GetCurrentPose();
        const AZ::Quaternion& parentOrientation = currentPose->GetModelSpaceTransform(parentNodeIndex).m_rotation;
        const AZ::Quaternion& childOrientation = currentPose->GetModelSpaceTransform(nodeIndex).m_rotation;

        m_vertexBuffer.clear();
        m_indexBuffer.clear();
        m_lineBuffer.clear();
        m_lineValidityBuffer.clear();
        if(auto* jointHelpers = AZ::Interface<AzPhysics::JointHelpersInterface>::Get())
        {
            jointHelpers->GenerateJointLimitVisualizationData(
                configuration, parentOrientation, childOrientation, s_scale, s_angularSubdivisions, s_radialSubdivisions, m_vertexBuffer,
                m_indexBuffer, m_lineBuffer, m_lineValidityBuffer);
        }           

        Transform jointModelSpaceTransform = currentPose->GetModelSpaceTransform(parentNodeIndex);
        jointModelSpaceTransform.m_position = currentPose->GetModelSpaceTransform(nodeIndex).m_position;
        const Transform jointGlobalTransformNoScale = jointModelSpaceTransform * actorInstanceWorldTransform;

        MCommon::RenderUtil* renderUtil = renderInfo->m_renderUtil;
        const size_t numLineBufferEntries = m_lineBuffer.size();
        if (m_lineValidityBuffer.size() * 2 != numLineBufferEntries)
        {
            AZ_ErrorOnce("EMotionFX", false, "Unexpected buffer size in joint limit visualization for node %s", node->GetName());
            return;
        }

        for (size_t i = 0; i < numLineBufferEntries; i += 2)
        {
            renderUtil->RenderLine(jointGlobalTransformNoScale.TransformPoint(m_lineBuffer[i]),
                jointGlobalTransformNoScale.TransformPoint(m_lineBuffer[i + 1]), m_lineValidityBuffer[i / 2] ? color : violatedColor);
        }
    }

    void RagdollNodeInspectorPlugin::RenderJointFrame(
        const AzPhysics::JointConfiguration& configuration,
        const ActorInstance* actorInstance,
        const Node* node,
        const Node* parentNode,
        EMStudio::EMStudioPlugin::RenderInfo* renderInfo,
        const MCore::RGBAColor& color)
    {
        AZ_UNUSED(parentNode);

        const Transform& actorInstanceWorldSpaceTransform = actorInstance->GetWorldSpaceTransform();
        const Pose* currentPose = actorInstance->GetTransformData()->GetCurrentPose();
        const Transform childJointLocalSpaceTransform(AZ::Vector3::CreateZero(), configuration.m_childLocalRotation);
        const Transform childModelSpaceTransform = childJointLocalSpaceTransform * currentPose->GetModelSpaceTransform(node->GetNodeIndex());
        const Transform jointChildWorldSpaceTransformNoScale = (childModelSpaceTransform * actorInstanceWorldSpaceTransform);

        renderInfo->m_renderUtil->RenderArrow(0.1f, jointChildWorldSpaceTransformNoScale.m_position, MCore::GetRight(jointChildWorldSpaceTransformNoScale.ToAZTransform()), color);
    }
} // namespace EMotionFX
