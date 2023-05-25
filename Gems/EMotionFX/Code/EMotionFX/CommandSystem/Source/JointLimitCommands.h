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
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/PhysicsSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <AzFramework/Physics/Configuration/JointConfiguration.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace EMotionFX
{
    class Actor;

    // exists to get around a limit on number of base classes when serializing
    class ParameterMixinActorIdJointName
        : public ParameterMixinActorId
        , public ParameterMixinJointName
    {
    public:
        AZ_RTTI(ParameterMixinActorIdJointName, "{D9569517-8AE0-4CDA-8DE9-1B20E6A4A267}");
        AZ_CLASS_ALLOCATOR_DECL;

        ParameterMixinActorIdJointName() = default;
        ParameterMixinActorIdJointName(AZ::u32 actorId, const AZStd::string& jointName);
        virtual ~ParameterMixinActorIdJointName() = default;

        static void Reflect(AZ::ReflectContext * context);
    };

    //! Provides support for undoing and redoing modifications to joint limit configurations, and recording them in the Action History.
    class CommandAdjustJointLimit
        : public MCore::Command
        , public ParameterMixinActorIdJointName
        , public ParameterMixinSerializedContents
    {
    public:
        AZ_RTTI(
            CommandAdjustJointLimit,
            "{7CB28869-E9DA-4CAB-B63D-F32BD64012D9}",
            MCore::Command,
            ParameterMixinActorIdJointName,
            ParameterMixinSerializedContents);
        AZ_CLASS_ALLOCATOR_DECL

        explicit CommandAdjustJointLimit(MCore::Command* orgCommand = nullptr);
        CommandAdjustJointLimit(AZ::u32 actorId, const AZStd::string& jointName, MCore::Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);

        // MCore::Command overrides ...
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust joint limit"; }
        const char* GetDescription() const override { return "Adjust properties of the given joint limit"; }
        MCore::Command* Create() override { return aznew CommandAdjustJointLimit(this); }

        void SetJointConfiguration(const AzPhysics::JointConfiguration* jointConfiguration);
        void SetOldJointConfiguration(const AzPhysics::JointConfiguration* jointConfiguration);
        
        static constexpr const char* const CommandName = "AdjustJointLimit";

    private:
        AzPhysics::JointConfiguration* GetJointConfiguration(Actor** outActor, AZStd::string& outResult);

        AZStd::optional<AZStd::string> m_oldContents;
        bool m_oldIsDirty = false;
    };

} // namespace EMotionFX


