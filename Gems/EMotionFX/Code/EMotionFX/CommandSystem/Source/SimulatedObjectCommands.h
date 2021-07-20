/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;
    class SimulatedObject;
    class SimulatedJoint;

    class CommandSimulatedObjectHelpers
    {
    public:
        static bool AddSimulatedObject(AZ::u32 actorId, AZStd::optional<AZStd::string> name = AZStd::nullopt, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);
        static bool RemoveSimulatedObject(AZ::u32 actorId, size_t objectIndex, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);
        static bool AddSimulatedJoints(AZ::u32 actorId, const AZStd::vector<AZ::u32>& jointIndices, size_t objectIndex, bool addChildren, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);
        static bool RemoveSimulatedJoints(AZ::u32 actorId, const AZStd::vector<AZ::u32>& jointIndices, size_t objectIndex, bool removeChildren, MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

        static void JointIndicesToString(const AZStd::vector<AZ::u32>& jointIndices, AZStd::string& outJointIndicesString);
        static void StringToJointIndices(const AZStd::string& jointIndicesString, AZStd::vector<AZ::u32>& outJointIndices);

        static void ReplaceTag(const Actor* actor, const PhysicsSetup::ColliderConfigType colliderType, const AZStd::string& oldTag, const AZStd::string& newTag, MCore::CommandGroup& outCommandGroup);

    private:
        static bool ReplaceTag(const AZStd::string& oldTag, const AZStd::string& newTag, AZStd::vector<AZStd::string>& outTags);
    };

    class CommandAddSimulatedObject
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAddSimulatedObject, "{07CA04F9-5CDA-4BF0-92CD-D0E22A397C2C}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAddSimulatedObject(MCore::Command* orgCommand = nullptr);
        explicit CommandAddSimulatedObject(AZ::u32 actorId, AZStd::optional<AZStd::string> name = AZStd::nullopt, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add a simulated object to an actor"; }
        const char* GetDescription() const override { return "Add a simulated object to an actor"; }
        MCore::Command* Create() override { return aznew CommandAddSimulatedObject(this); }

        size_t GetObjectIndex() const { return m_objectIndex; }

        static const char* s_commandName;
        static const char* s_objectIndexParameterName;
        static const char* s_nameParameterName;
        static const char* s_contentParameterName;

    private:
        size_t              m_objectIndex;
        AZStd::string       m_contents;
        AZStd::optional<AZStd::string> m_name;
        bool                m_oldDirtyFlag = false;
    };

    class CommandRemoveSimulatedObject
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandRemoveSimulatedObject, "{0EAB9831-62EE-4991-B4BC-4D1222879092}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandRemoveSimulatedObject(MCore::Command* orgCommand = nullptr);
        explicit CommandRemoveSimulatedObject(AZ::u32 actorId, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove a simulated object from an actor"; }
        const char* GetDescription() const override { return "Remove a simulated object from an actor"; }
        MCore::Command* Create() override { return aznew CommandRemoveSimulatedObject(this); }

        size_t GetObjectIndex() const { return m_objectIndex; }

        static const char* s_commandName;
        static const char* s_objectIndexParameterName;

    private:
        size_t             m_objectIndex;
        AZStd::string      m_oldContents;
        bool               m_oldDirtyFlag = false;
    };

    class CommandAdjustSimulatedObject
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAdjustSimulatedObject, "{B0BCB1F0-E678-4189-9ADD-4B14CAF14B76}", MCore::Command, ParameterMixinActorId);
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustSimulatedObject(MCore::Command* orgCommand = nullptr);
        CommandAdjustSimulatedObject(AZ::u32 actorId, size_t objectIndex, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust simulated object attributes"; }
        const char* GetDescription() const override { return "Adjust the attributes of a simulated object"; }
        MCore::Command* Create() override { return aznew CommandAdjustSimulatedObject(this); }

        size_t GetObjectIndex() const;

        void SetObjectName(AZStd::optional<AZStd::string> newObjectName);
        void SetGravityFactor(AZStd::optional<float> newGravityFactor);
        void SetStiffnessFactor(AZStd::optional<float> newStiffnessFactor);
        void SetDampingFactor(AZStd::optional<float> newDampingFactor);
        void SetColliderTags(const AZStd::vector<AZStd::string>& newColliderTags) { m_colliderTags = newColliderTags; }

        void SetOldObjectName(AZStd::optional<AZStd::string> newObjectName);
        void SetOldGravityFactor(AZStd::optional<float> newGravityFactor);
        void SetOldStiffnessFactor(AZStd::optional<float> newStiffnessFactor);
        void SetOldDampingFactor(AZStd::optional<float> newDampingFactor);
        void SetOldColliderTags(const AZStd::vector<AZStd::string>& colliderTags) { m_oldColliderTags = colliderTags; }

        static const char* const s_commandName;
        static const char* const s_objectNameParameterName;
        static const char* const s_gravityFactorParameterName;
        static const char* const s_stiffnessFactorParameterName;
        static const char* const s_dampingFactorParameterName;
        static const char* const s_colliderTagsParameterName;

    private:
        SimulatedObject* GetSimulatedObject(AZStd::string& outResult) const;

        size_t m_objectIndex = ~0UL;
        bool m_oldDirtyFlag = false;

        AZStd::optional<AZStd::string> m_objectName;
        AZStd::optional<float> m_gravityFactor;
        AZStd::optional<float> m_stiffnessFactor;
        AZStd::optional<float> m_dampingFactor;
        AZStd::optional<AZStd::vector<AZStd::string>> m_colliderTags;

        AZStd::optional<AZStd::string> m_oldObjectName;
        AZStd::optional<float> m_oldGravityFactor;
        AZStd::optional<float> m_oldStiffnessFactor;
        AZStd::optional<float> m_oldDampingFactor;
        AZStd::optional<AZStd::vector<AZStd::string>> m_oldColliderTags;
    };

    class CommandAddSimulatedJoints
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAddSimulatedJoints, "{0D12EF58-E732-4915-9D25-1E3953EBAE5F}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAddSimulatedJoints(MCore::Command* orgCommand = nullptr);
        explicit CommandAddSimulatedJoints(AZ::u32 actorId, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add simulated joints to a simulated object"; }
        const char* GetDescription() const override { return "Add simulated joints to a simulated object"; }
        MCore::Command* Create() override { return aznew CommandAddSimulatedJoints(this); }

        const AZStd::vector<AZ::u32>& GetJointIndices() const { return m_jointIndices; }
        void SetJointIndices(AZStd::vector<AZ::u32> newJointIndices) { m_jointIndices = AZStd::move(newJointIndices); }

        size_t GetObjectIndex() { return m_objectIndex; }
        void SetObjectIndex(size_t newObjectIndex ) { m_objectIndex = newObjectIndex; }

        static const char* s_commandName;
        static const char* s_jointIndicesParameterName;
        static const char* s_objectIndexParameterName;
        static const char* s_addChildrenParameterName;
        static const char* s_contentsParameterName;
    private:
        size_t                                  m_objectIndex = MCORE_INVALIDINDEX32;
        AZStd::vector<AZ::u32>                  m_jointIndices;
        AZStd::optional<AZStd::string>          m_contents;
        bool                                    m_addChildren = false;
        bool                                    m_oldDirtyFlag = false;
    };

    class CommandRemoveSimulatedJoints
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandRemoveSimulatedJoints, "{95930C4E-6D0B-43FE-B0B3-22F8C112D6C4}", MCore::Command, ParameterMixinActorId)
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandRemoveSimulatedJoints(MCore::Command* orgCommand = nullptr);
        explicit CommandRemoveSimulatedJoints(AZ::u32 actorId, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove simulated joints from a simulated object"; }
        const char* GetDescription() const override { return "Remove simulated joints from a simulated object"; }
        MCore::Command* Create() override { return aznew CommandRemoveSimulatedJoints(this); }

        const AZStd::vector<AZ::u32>& GetJointIndices() const { return m_jointIndices; }
        size_t GetObjectIndex() { return m_objectIndex; }

        static const char* s_commandName;
        static const char* s_jointIndicesParameterName;
        static const char* s_objectIndexParameterName;

        static const char* s_removeChildrenParameterName;
    private:
        size_t                                  m_objectIndex = MCORE_INVALIDINDEX32;
        AZStd::vector<AZ::u32>                  m_jointIndices;
        AZStd::optional<AZStd::string>          m_oldContents;
        bool                                    m_removeChildren = false;
        bool                                    m_oldDirtyFlag = false;
    };

    class CommandAdjustSimulatedJoint
        : public MCore::Command
        , public ParameterMixinActorId
    {
    public:
        AZ_RTTI(CommandAdjustSimulatedJoint, "{ADCCE17E-6841-46C4-9C46-9C65B2E2C0A0}", MCore::Command, ParameterMixinActorId);
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustSimulatedJoint(MCore::Command* orgCommand = nullptr);
        CommandAdjustSimulatedJoint(AZ::u32 actorId, size_t objectIndex, size_t jointIndex, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust simulated joint attributes"; }
        const char* GetDescription() const override { return "Adjust the attributes of a simulated joint"; }
        MCore::Command* Create() override { return aznew CommandAdjustSimulatedJoint(this); }

        SimulatedJoint* GetSimulatedJoint() const;

        void SetConeAngleLimit(float newConeAngleLimit);
        void SetMass(float newMass);
        void SetStiffness(float newStiffness);
        void SetDamping(float newDamping);
        void SetGravityFactor(float newGravityFactor);
        void SetFriction(float newFriction);
        void SetPinned(bool newPinned);
        void SetColliderExclusionTags(const AZStd::vector<AZStd::string>& exclusionTagList) { m_colliderExclusionTags = exclusionTagList; }
        void SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode newMode);
        void SetGeometricAutoExclusion(bool newEnabled);

        void SetOldConeAngleLimit(float oldConeAngleLimit);
        void SetOldMass(float oldMass);
        void SetOldStiffness(float oldStiffness);
        void SetOldDamping(float oldDamping);
        void SetOldGravityFactor(float oldGravityFactor);
        void SetOldFriction(float oldFriction);
        void SetOldPinned(bool oldPinned);
        void SetOldColliderExclusionTags(const AZStd::vector<AZStd::string>& exclusionTagList) { m_oldColliderExclusionTags = exclusionTagList; }
        void SetOldAutoExcludeMode(SimulatedJoint::AutoExcludeMode oldMode);
        void SetOldGeometricAutoExclusion(bool oldEnabled);

        static const char* const s_commandName;
        static const char* const s_objectIndexParameterName;
        static const char* const s_jointIndexParameterName;
        static const char* const s_coneAngleLimitParameterName;
        static const char* const s_massParameterName;
        static const char* const s_stiffnessParameterName;
        static const char* const s_dampingParameterName;
        static const char* const s_gravityFactorParameterName;
        static const char* const s_frictionParameterName;
        static const char* const s_pinnedParameterName;
        static const char* const s_colliderExclusionTagsParameterName;
        static const char* const s_autoExcludeModeParameterName;
        static const char* const s_geometricAutoExclusionParameterName;

    private:
        SimulatedObject* GetSimulatedObject(AZStd::string& outResult) const;

        size_t m_objectIndex = ~0UL;
        size_t m_jointIndex = ~0UL;
        bool m_oldDirtyFlag = false;

        AZStd::optional<float> m_coneAngleLimit;
        AZStd::optional<float> m_mass;
        AZStd::optional<float> m_stiffness;
        AZStd::optional<float> m_damping;
        AZStd::optional<float> m_gravityFactor;
        AZStd::optional<float> m_friction;
        AZStd::optional<bool> m_pinned;
        AZStd::optional<AZStd::vector<AZStd::string>> m_colliderExclusionTags;
        AZStd::optional<SimulatedJoint::AutoExcludeMode> m_autoExcludeMode;
        AZStd::optional<bool> m_geometricAutoExclusion;

        AZStd::optional<float> m_oldConeAngleLimit;
        AZStd::optional<float> m_oldMass;
        AZStd::optional<float> m_oldStiffness;
        AZStd::optional<float> m_oldDamping;
        AZStd::optional<float> m_oldGravityFactor;
        AZStd::optional<float> m_oldFriction;
        AZStd::optional<bool> m_oldPinned;
        AZStd::optional<AZStd::vector<AZStd::string>> m_oldColliderExclusionTags;
        AZStd::optional<SimulatedJoint::AutoExcludeMode> m_oldAutoExcludeMode;
        AZStd::optional<bool> m_oldGeometricAutoExclusion;
    };
} // namespace EMotionFX
