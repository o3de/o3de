/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <EMotionFX/CommandSystem/Source/ColliderCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/RagdollCommands.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/SkeletonModel.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QAction>
#include <QClipboard>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMenu>
#include <QMimeData>
#include <QObject>
#include <QPushButton>

namespace EMotionFX
{
    void ColliderHelpers::AddCopyColliderCommandToGroup(
        const Actor* actor,
        const Node* joint,
        PhysicsSetup::ColliderConfigType copyFrom,
        PhysicsSetup::ColliderConfigType copyTo,
        MCore::CommandGroup& commandGroup)
    {
        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::CharacterColliderConfiguration* copyFromColliderConfig = physicsSetup->GetColliderConfigByType(copyFrom);
        if (!copyFromColliderConfig)
        {
            return;
        }

        const Physics::CharacterColliderNodeConfiguration* copyFromNodeConfig =
            copyFromColliderConfig->FindNodeConfigByName(joint->GetNameString());
        if (copyFromNodeConfig)
        {
            for (const AzPhysics::ShapeColliderPair& shapeConfigPair : copyFromNodeConfig->m_shapes)
            {
                const AZStd::string contents = MCore::ReflectionSerializer::Serialize(&shapeConfigPair).GetValue();
                CommandColliderHelpers::AddCollider(
                    actor->GetID(), joint->GetNameString(), copyTo, contents, AZStd::nullopt, &commandGroup);
            }
        }
    }

    void ColliderHelpers::CopyColliders(
        const QModelIndexList& modelIndices,
        PhysicsSetup::ColliderConfigType copyFrom,
        PhysicsSetup::ColliderConfigType copyTo,
        bool removeExistingColliders)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format(
            "Copy %s collider to %s",
            PhysicsSetup::GetStringForColliderConfigType(copyFrom),
            PhysicsSetup::GetStringForColliderConfigType(copyTo));

        MCore::CommandGroup commandGroup(groupName);

        AZStd::string contents;
        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            if (removeExistingColliders)
            {
                CommandColliderHelpers::ClearColliders(actor->GetID(), joint->GetNameString(), copyTo, &commandGroup);
            }

            AddCopyColliderCommandToGroup(actor, joint, copyFrom, copyTo, commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ColliderHelpers::AddCollider(
        const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType addTo, const AZ::TypeId& colliderType)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Add %s colliders", PhysicsSetup::GetStringForColliderConfigType(addTo));

        MCore::CommandGroup commandGroup(groupName);

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            CommandColliderHelpers::AddCollider(actor->GetID(), joint->GetNameString(), addTo, colliderType, &commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ColliderHelpers::ClearColliders(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType removeFrom)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName =
            AZStd::string::format("Remove %s colliders", PhysicsSetup::GetStringForColliderConfigType(removeFrom));

        MCore::CommandGroup commandGroup(groupName);

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            CommandColliderHelpers::ClearColliders(actor->GetID(), joint->GetNameString(), removeFrom, &commandGroup);
        }

        AZStd::string result;
        if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    bool ColliderHelpers::AreCollidersReflected()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            if (serializeContext->FindClassData(azrtti_typeid<Physics::SphereShapeConfiguration>()) &&
                serializeContext->FindClassData(azrtti_typeid<Physics::BoxShapeConfiguration>()) &&
                serializeContext->FindClassData(azrtti_typeid<Physics::CapsuleShapeConfiguration>()))
            {
                return true;
            }
        }

        return false;
    }

    bool ColliderHelpers::CanCopyFrom(const QModelIndexList& modelIndices, PhysicsSetup::ColliderConfigType copyFrom)
    {
        if (modelIndices.isEmpty())
        {
            return false;
        }

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

            const Actor* actor = selectedIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            const Node* joint = selectedIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

            const Physics::CharacterColliderConfiguration* config = actor->GetPhysicsSetup()->GetColliderConfigByType(copyFrom);
            if (config->FindNodeConfigByName(joint->GetNameString()))
            {
                return true;
            }
        }

        return false;
    }

    void ColliderHelpers::AddToRagdoll(const QModelIndexList& modelIndices)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Add joint%s to ragdoll", modelIndices.size() > 1 ? "s" : "");

        MCore::CommandGroup commandGroup(groupName);

        AZStd::vector<AZStd::string> jointNames;
        jointNames.reserve(modelIndices.size());

        // All the actor pointers should be the same
        const uint32 actorId = modelIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>()->GetID();

        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

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

    void ColliderHelpers::RemoveFromRagdoll(const QModelIndexList& modelIndices)
    {
        if (modelIndices.empty())
        {
            return;
        }

        const AZStd::string groupName = AZStd::string::format("Remove joint%s from ragdoll", modelIndices.size() > 1 ? "s" : "");

        MCore::CommandGroup commandGroup(groupName);

        AZStd::vector<AZStd::string> jointNamesToRemove;
        for (const QModelIndex& selectedIndex : modelIndices)
        {
            if (SkeletonModel::IndexIsRootNode(selectedIndex))
            {
                continue;
            }

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

    void ColliderHelpers::AddCopyFromMenu(
        QObject* parent,
        QMenu* parentMenu,
        PhysicsSetup::ColliderConfigType createForType,
        const QModelIndexList& modelIndices,
        const AZStd::function<void(PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)>& copyFunc)
    {
        QMenu* copyFromMenu = parentMenu->addMenu("Copy from existing colliders");

        for (int i = 0; i < PhysicsSetup::ColliderConfigType::Unknown; ++i)
        {
            const PhysicsSetup::ColliderConfigType copyFrom = static_cast<PhysicsSetup::ColliderConfigType>(i);
            if (copyFrom == createForType)
            {
                continue;
            }

            QAction* action = copyFromMenu->addAction(PhysicsSetup::GetVisualNameForColliderConfigType(copyFrom));
            const bool canCopyFrom = CanCopyFrom(modelIndices, copyFrom);
            if (canCopyFrom)
            {
                QObject::connect(
                    action,
                    &QAction::triggered,
                    parent,
                    [copyFunc, copyFrom, createForType]
                    {
                        copyFunc(copyFrom, createForType);
                    });
            }
            else
            {
                action->setEnabled(false);
            }
        }
    }

    void ColliderHelpers::AddCopyFromMenu(
        QObject* parent, QMenu* parentMenu, PhysicsSetup::ColliderConfigType createForType, const QModelIndexList& modelIndices)
    {
        AddCopyFromMenu(
            parent,
            parentMenu,
            createForType,
            modelIndices,
            [modelIndices](PhysicsSetup::ColliderConfigType copyFrom, PhysicsSetup::ColliderConfigType copyTo)
            {
                ColliderHelpers::CopyColliders(modelIndices, copyFrom, copyTo);
            });
    }

    void ColliderHelpers::CopyColliderToClipboard(const QModelIndex& modelIndex, size_t shapeIndex, PhysicsSetup::ColliderConfigType type)
    {
        if (!modelIndex.isValid())
        {
            return;
        }

        const Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const Physics::CharacterColliderConfiguration* copyFromColliderConfig = actor->GetPhysicsSetup()->GetColliderConfigByType(type);
        if (!copyFromColliderConfig)
        {
            return;
        }

        const Physics::CharacterColliderNodeConfiguration* copyFromNodeConfig =
            copyFromColliderConfig->FindNodeConfigByName(joint->GetNameString());
        if (copyFromNodeConfig && shapeIndex < copyFromNodeConfig->m_shapes.size())
        {
            const AzPhysics::ShapeColliderPair* shape = &copyFromNodeConfig->m_shapes[shapeIndex];
            const AZStd::string contents = MCore::ReflectionSerializer::Serialize(shape).GetValue();
            QMimeData* mimeData = new QMimeData();
            mimeData->setData(GetMimeTypeForColliderShape(), QByteArray(contents.data(), static_cast<int>(contents.size())));
            QGuiApplication::clipboard()->setMimeData(mimeData);
        }
    }

    void ColliderHelpers::PasteColliderFromClipboard(
        const QModelIndex& modelIndex, size_t shapeIndex, PhysicsSetup::ColliderConfigType type, bool replace)
    {
        const QClipboard* clipboard = QGuiApplication::clipboard();
        const QMimeData* mimeData = clipboard->mimeData();
        const QByteArray clipboardContents = mimeData->data(GetMimeTypeForColliderShape());

        if (clipboardContents.isEmpty())
        {
            return;
        }

        const Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const Physics::CharacterColliderConfiguration* pasteToColliderConfig = actor->GetPhysicsSetup()->GetColliderConfigByType(type);
        if (!pasteToColliderConfig)
        {
            return;
        }

        const Physics::CharacterColliderNodeConfiguration* pasteToNodeConfig =
            pasteToColliderConfig->FindNodeConfigByName(joint->GetNameString());
        if (!pasteToNodeConfig && !replace && shapeIndex > 0)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Paste collider");

        if (replace && shapeIndex < pasteToNodeConfig->m_shapes.size())
        {
            // Replace the existing one
            CommandColliderHelpers::RemoveCollider(actor->GetID(), joint->GetNameString(), type, shapeIndex, &commandGroup);
        }

        CommandColliderHelpers::AddCollider(
            actor->GetID(),
            joint->GetNameString(),
            type,
            AZStd::string(clipboardContents.data(), clipboardContents.size()),
            shapeIndex,
            &commandGroup);

        AZStd::string result;
        CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result);
    }

    bool ColliderHelpers::NodeHasRagdoll(const QModelIndex& modelIndex)
    {
        const Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const Physics::RagdollConfiguration& ragdollConfig = physicsSetup->GetRagdollConfig();

        return ragdollConfig.FindNodeConfigByName(joint->GetNameString()) != nullptr;
    }

    bool ColliderHelpers::NodeHasClothCollider(const QModelIndex& modelIndex)
    {
        const Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const auto& clothConfig = physicsSetup->GetClothConfig();

        return clothConfig.FindNodeConfigByName(joint->GetNameString()) != nullptr;
    }

    bool ColliderHelpers::NodeHasHitDetection(const QModelIndex& modelIndex)
    {
        const Actor* actor = modelIndex.data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
        const Node* joint = modelIndex.data(SkeletonModel::ROLE_POINTER).value<Node*>();

        const AZStd::shared_ptr<PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
        const auto& hitDetectionConfig = physicsSetup->GetHitDetectionConfig();

        return hitDetectionConfig.FindNodeConfigByName(joint->GetNameString()) != nullptr;
    }
} // namespace EMotionFX
