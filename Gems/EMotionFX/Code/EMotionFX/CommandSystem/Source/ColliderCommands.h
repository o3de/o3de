/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;

    class CommandColliderHelpers
    {
    public:
        static Physics::CharacterColliderNodeConfiguration* GetNodeConfig(const Actor* actor, const AZStd::string& jointName, const Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult);
        static Physics::CharacterColliderNodeConfiguration* GetCreateNodeConfig(const Actor* actor, const AZStd::string& jointName, Physics::CharacterColliderConfiguration& colliderConfig, AZStd::string& outResult);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZ::TypeId& colliderType,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZStd::string& contents, const AZStd::optional<size_t>& insertAtIndex = AZStd::nullopt,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool AddCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            const AZStd::optional<AZ::TypeId>& colliderType, const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAtIndex = AZStd::nullopt,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static bool RemoveCollider(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            size_t colliderIndex,
            MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false, bool firstLastCommand = true);

        static bool ClearColliders(AZ::u32 actorId, const AZStd::string& jointName,
            const PhysicsSetup::ColliderConfigType& configType,
            MCore::CommandGroup* commandGroup = nullptr);
    };

    class CommandAddCollider
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandAddCollider, "{15DF4EB1-5FC2-457D-A2F5-0D94C9702926}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAddCollider(MCore::Command* orgCommand = nullptr);
        CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZ::TypeId& colliderType, MCore::Command* orgCommand = nullptr);
        CommandAddCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, const AZStd::string& contents, size_t insertAtIndex, MCore::Command* orgCommand = nullptr);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_colliderConfigTypeParameterName;
        static const char* s_colliderTypeParameterName;
        static const char* s_contentsParameterName;
        static const char* s_insertAtIndexParameterName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add collider"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAddCollider(this); }

    private:
        PhysicsSetup::ColliderConfigType m_configType;
        AZStd::optional<AZ::TypeId> m_colliderType;
        AZStd::optional<AZStd::string> m_contents;
        AZStd::optional<size_t> m_insertAtIndex;

        bool m_oldIsDirty;
        AZStd::optional<size_t> m_oldColliderIndex;
    };

    class CommandAdjustCollider
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandAdjustCollider, "{F20BE1AB-6058-43EA-B309-465EBFBAE6C3}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName);
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustCollider(MCore::Command* orgCommand = nullptr);
        CommandAdjustCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, size_t colliderIndex, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust collider"; }
        const char* GetDescription() const override { return "Adjust properties of the given collider"; }
        MCore::Command* Create() override { return aznew CommandAdjustCollider(this); }

        void SetColliderConfig(PhysicsSetup::ColliderConfigType configType) { m_configType = configType; }
        void SetColliderIndex(AZ::u32 colliderIndex) { m_index = colliderIndex; }

        // ColliderConfiguration
        void SetCollisionLayer(AzPhysics::CollisionLayer collisionLayer) { m_collisionLayer = collisionLayer; }
        void SetCollisionGroupId(AzPhysics::CollisionGroups::Id collisionGroupId) { m_collisionGroupId = collisionGroupId; }
        void SetIsTrigger(bool isTrigger) { m_isTrigger = isTrigger; }
        void SetPosition(AZ::Vector3 position) { m_position = AZStd::move(position); }
        void SetRotation(AZ::Quaternion rotation) { m_rotation = AZStd::move(rotation); }
        void SetMaterial(Physics::MaterialSelection material) { m_material = AZStd::move(material); }
        void SetTag(AZStd::string tag) { m_tag = AZStd::move(tag); }

        void SetOldCollisionLayer(AzPhysics::CollisionLayer collisionLayer) { m_oldCollisionLayer = collisionLayer; }
        void SetOldCollisionGroupId(AzPhysics::CollisionGroups::Id collisionGroupId) { m_oldCollisionGroupId = collisionGroupId; }
        void SetOldIsTrigger(bool isTrigger) { m_oldIsTrigger = isTrigger; }
        void SetOldPosition(AZ::Vector3 position) { m_oldPosition = AZStd::move(position); }
        void SetOldRotation(AZ::Quaternion rotation) { m_oldRotation = AZStd::move(rotation); }
        void SetOldMaterial(Physics::MaterialSelection material) { m_oldMaterial = AZStd::move(material); }
        void SetOldTag(AZStd::string tag) { m_oldTag = AZStd::move(tag); }

        const AZStd::optional<AZStd::string>& GetOldTag() const { return m_oldTag; }

        // ShapeConfiguration
        void SetRadius(float radius) { m_radius = radius; }
        void SetHeight(float height) { m_height = height; }
        void SetDimensions(AZ::Vector3 dimensions) { m_dimensions = AZStd::move(dimensions); }

        void SetOldRadius(float radius) { m_oldRadius = radius; }
        void SetOldHeight(float height) { m_oldHeight = height; }
        void SetOldDimensions(AZ::Vector3 dimensions) { m_oldDimensions = AZStd::move(dimensions); }

        static const char* s_commandName;

    private:
        AzPhysics::ShapeColliderPair* GetShapeConfigPair(Actor** outActor, AZStd::string& outResult) const;

        AZStd::optional<PhysicsSetup::ColliderConfigType> m_configType;
        AZStd::optional<size_t> m_index;

        // ColliderConfiguration
        AZStd::optional<AzPhysics::CollisionLayer> m_collisionLayer;
        AZStd::optional<AzPhysics::CollisionGroups::Id> m_collisionGroupId;
        AZStd::optional<bool> m_isTrigger;
        AZStd::optional<AZ::Vector3> m_position;
        AZStd::optional<AZ::Quaternion> m_rotation;
        AZStd::optional<Physics::MaterialSelection> m_material;
        AZStd::optional<AZStd::string> m_tag;

        AZStd::optional<AzPhysics::CollisionLayer> m_oldCollisionLayer;
        AZStd::optional<AzPhysics::CollisionGroups::Id> m_oldCollisionGroupId;
        AZStd::optional<bool> m_oldIsTrigger;
        AZStd::optional<AZ::Vector3> m_oldPosition;
        AZStd::optional<AZ::Quaternion> m_oldRotation;
        AZStd::optional<Physics::MaterialSelection> m_oldMaterial;
        AZStd::optional<AZStd::string> m_oldTag;

        // ShapeConfiguration
        AZStd::optional<float> m_radius; // Capsule, Sphere
        AZStd::optional<float> m_height; // Capsule
        AZStd::optional<AZ::Vector3> m_dimensions; // Box

        AZStd::optional<float> m_oldRadius;
        AZStd::optional<float> m_oldHeight;
        AZStd::optional<AZ::Vector3> m_oldDimensions;

        bool m_oldIsDirty;
    };

    class CommandRemoveCollider
        : public MCore::Command
        , public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(CommandRemoveCollider, "{A8F70A9E-2021-473E-BF2D-AE1859D9201D}", MCore::Command, ParameterMixinActorId, ParameterMixinJointName)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandRemoveCollider(MCore::Command* orgCommand = nullptr);
        CommandRemoveCollider(AZ::u32 actorId, const AZStd::string& jointName, PhysicsSetup::ColliderConfigType configType, size_t colliderIndex, MCore::Command* orgCommand = nullptr);
        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_colliderConfigTypeParameterName;
        static const char* s_colliderIndexParameterName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove collider"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandRemoveCollider(this); }

    private:
        PhysicsSetup::ColliderConfigType m_configType;
        AZ::u64 m_colliderIndex;

        bool m_oldIsDirty;
        AZStd::string m_oldContents;
    };
} // namespace EMotionFX
