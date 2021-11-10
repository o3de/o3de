/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CommandSystemConfig.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>


namespace EMotionFX
{
    class AnimGraph;
    class AnimGraphInstance;
}

namespace CommandSystem
{
    class MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::MotionIdCommandMixin, "{968E9513-3159-4469-B5FA-97D0920456E3}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~MotionIdCommandMixin() = default;

        static void Reflect(AZ::ReflectContext* context);

        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetMotionID(uint32 motionID) { m_motionID = motionID; }
    protected:
        uint32 m_motionID = 0;
    };

    // Adjust motion command.
    class DEFINECOMMAND_API CommandAdjustMotion
        : public MCore::Command, public MotionIdCommandMixin
    {
    public:
        AZ_RTTI(CommandSystem::CommandAdjustMotion, "{A8977553-4011-4BEB-97C8-6AE44B07C7A8}", MCore::Command, MotionIdCommandMixin)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAdjustMotion(MCore::Command * orgCommand = nullptr);
        ~CommandAdjustMotion() override = default;

        static void Reflect(AZ::ReflectContext* context);

        bool Execute(const MCore::CommandLine & parameters, AZStd::string & outResult) override;
        bool Undo(const MCore::CommandLine & parameters, AZStd::string & outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust motion"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAdjustMotion(this); }

        void SetMotionExtractionFlags(const EMotionFX::EMotionExtractionFlags flags) { m_extractionFlags = flags; }

    private:
        AZStd::optional<bool> m_dirtyFlag;
        bool m_oldDirtyFlag;
        AZStd::optional<EMotionFX::EMotionExtractionFlags> m_extractionFlags;
        EMotionFX::EMotionExtractionFlags m_oldExtractionFlags;
        AZStd::optional<AZStd::string> m_name;
        AZStd::string m_oldName;
        AZStd::string m_oldMotionExtractionNodeName;
    };


    // Remove motion command.
    MCORE_DEFINECOMMAND_START(CommandRemoveMotion, "Remove motion", true)
    public:
        uint32          m_oldMotionId;
        AZStd::string   m_oldFileName;
        size_t          m_oldIndex;
        bool            m_oldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Scale motion data.
    MCORE_DEFINECOMMAND_START(CommandScaleMotionData, "Scale motion data", true)
    public:
        AZStd::string   m_oldUnitType;
        uint32          m_motionId;
        float           m_scaleFactor;
        bool            m_oldDirtyFlag;
        bool            m_useUnitType;
    MCORE_DEFINECOMMAND_END


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Playback
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Play motion command.
    MCORE_DEFINECOMMAND_START(CommandPlayMotion, "Play motion", true)
    public:
        struct UndoObject
        {
            EMotionFX::ActorInstance*       m_actorInstance = nullptr;         /**< The old selected actor on which the motion got started. */
            EMotionFX::MotionInstance*      m_motionInstance = nullptr;        /**< The old motion instance to be stopped by the undo process. */
            EMotionFX::AnimGraph*           m_animGraph = nullptr;             /**< The old anim graph that was playing on the actor instance before playing the motion. */
            EMotionFX::AnimGraphInstance*   m_animGraphInstance = nullptr;     /**< The old anim graph instance. This pointer won't be valid anymore at undo but is needed for the anim graph model callbacks. */
        };
        
        AZStd::vector<UndoObject> m_oldData; /**< Array of undo items. Each item means we started a motion on an actor and have to stop it again in the undo process. */
    
        static void CommandParametersToPlaybackInfo(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::PlayBackInfo* outPlaybackInfo);
        static AZStd::string PlayBackInfoToCommandParameters(const EMotionFX::PlayBackInfo* playbackInfo);
    MCORE_DEFINECOMMAND_END


    // Adjust motion instance command.
    MCORE_DEFINECOMMAND_START(CommandAdjustMotionInstance, "Adjust motion instance", true)
    void AdjustMotionInstance(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionInstance* motionInstance);
    MCORE_DEFINECOMMAND_END


    // Adjust default playback info command.
    MCORE_DEFINECOMMAND_START(CommandAdjustDefaultPlayBackInfo, "Adjust default playback info", true)
    EMotionFX::PlayBackInfo m_oldPlaybackInfo;
    bool                    m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Stop motion instances command.
    MCORE_DEFINECOMMAND(CommandStopMotionInstances, "StopMotionInstances", "Stop motion instances", false)


    // Stop all motion instances command.
    MCORE_DEFINECOMMAND_START(CommandStopAllMotionInstances, "Stop all motion instances", false)
public:
    static const char* s_stopAllMotionInstancesCmdName;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper Functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API LoadMotionsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload = false);
    void COMMANDSYSTEM_API RemoveMotions(const AZStd::vector<EMotionFX::Motion*>& motions, AZStd::vector<EMotionFX::Motion*>* outFailedMotions, MCore::CommandGroup* commandGroup = nullptr, bool forceRemove = false);
    void COMMANDSYSTEM_API ClearMotions(MCore::CommandGroup* commandGroup = nullptr, bool forceRemove = false);
} // namespace CommandSystem
