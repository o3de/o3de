/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionCommands.h"
#include "CommandManager.h"

#include <MCore/Source/Compare.h>
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/LogManager.h>

#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>

#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionIdCommandMixin, EMotionFX::CommandAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustMotion, EMotionFX::CommandAllocator, 0)

    const char* CommandStopAllMotionInstances::s_stopAllMotionInstancesCmdName = "StopAllMotionInstances";

    void MotionIdCommandMixin::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<MotionIdCommandMixin>()
            ->Version(1)
            ->Field("motionID", &MotionIdCommandMixin::m_motionID)
        ;
    }

    bool MotionIdCommandMixin::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        m_motionID = parameters.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
        return true;
    }

    CommandStopAllMotionInstances::CommandStopAllMotionInstances(MCore::Command* orgCommand)
        : MCore::Command(s_stopAllMotionInstancesCmdName, orgCommand)
    {
    }

    CommandStopAllMotionInstances::~CommandStopAllMotionInstances()
    {
    }

    //--------------------------------------------------------------------------------
    // CommandPlayMotion
    //--------------------------------------------------------------------------------
    CommandPlayMotion::CommandPlayMotion(MCore::Command* orgCommand)
        : MCore::Command("PlayMotion", orgCommand)
    {
    }


    CommandPlayMotion::~CommandPlayMotion()
    {
    }


    AZStd::string CommandPlayMotion::PlayBackInfoToCommandParameters(const EMotionFX::PlayBackInfo* playbackInfo)
    {
        return AZStd::string::format("-blendInTime %f -blendOutTime %f -playSpeed %f -targetWeight %f -eventWeightThreshold %f -maxPlayTime %f -numLoops %i -priorityLevel %i -blendMode %i -playMode %i -mirrorMotion %s -mix %s -playNow %s -motionExtraction %s -retarget %s -freezeAtLastFrame %s -enableMotionEvents %s -blendOutBeforeEnded %s -canOverwrite %s -deleteOnZeroWeight %s -inPlace %s",
            playbackInfo->mBlendInTime,
            playbackInfo->mBlendOutTime,
            playbackInfo->mPlaySpeed,
            playbackInfo->mTargetWeight,
            playbackInfo->mEventWeightThreshold,
            playbackInfo->mMaxPlayTime,
            playbackInfo->mNumLoops,
            playbackInfo->mPriorityLevel,
            static_cast<AZ::u8>(playbackInfo->mBlendMode),
            static_cast<AZ::u8>(playbackInfo->mPlayMode),
            AZStd::to_string(playbackInfo->mMirrorMotion).c_str(),
            AZStd::to_string(playbackInfo->mMix).c_str(),
            AZStd::to_string(playbackInfo->mPlayNow).c_str(),
            AZStd::to_string(playbackInfo->mMotionExtractionEnabled).c_str(),
            AZStd::to_string(playbackInfo->mRetarget).c_str(),
            AZStd::to_string(playbackInfo->mFreezeAtLastFrame).c_str(),
            AZStd::to_string(playbackInfo->mEnableMotionEvents).c_str(),
            AZStd::to_string(playbackInfo->mBlendOutBeforeEnded).c_str(),
            AZStd::to_string(playbackInfo->mCanOverwrite).c_str(),
            AZStd::to_string(playbackInfo->mDeleteOnZeroWeight).c_str(),
            AZStd::to_string(playbackInfo->mInPlace).c_str());
    }


    // Fill the playback info based on the input parameters.
    void CommandPlayMotion::CommandParametersToPlaybackInfo(Command* command, const MCore::CommandLine& parameters, EMotionFX::PlayBackInfo* outPlaybackInfo)
    {
        if (parameters.CheckIfHasParameter("blendInTime") == true)
        {
            outPlaybackInfo->mBlendInTime = parameters.GetValueAsFloat("blendInTime", command);
        }
        if (parameters.CheckIfHasParameter("blendOutTime"))
        {
            outPlaybackInfo->mBlendOutTime = parameters.GetValueAsFloat("blendOutTime", command);
        }
        if (parameters.CheckIfHasParameter("playSpeed"))
        {
            outPlaybackInfo->mPlaySpeed = parameters.GetValueAsFloat("playSpeed", command);
        }
        if (parameters.CheckIfHasParameter("targetWeight"))
        {
            outPlaybackInfo->mTargetWeight = parameters.GetValueAsFloat("targetWeight", command);
        }
        if (parameters.CheckIfHasParameter("eventWeightThreshold"))
        {
            outPlaybackInfo->mEventWeightThreshold = parameters.GetValueAsFloat("eventWeightThreshold", command);
        }
        if (parameters.CheckIfHasParameter("maxPlayTime"))
        {
            outPlaybackInfo->mMaxPlayTime = parameters.GetValueAsFloat("maxPlayTime", command);
        }
        if (parameters.CheckIfHasParameter("numLoops"))
        {
            outPlaybackInfo->mNumLoops = parameters.GetValueAsInt("numLoops", command);
        }
        if (parameters.CheckIfHasParameter("priorityLevel"))
        {
            outPlaybackInfo->mPriorityLevel = parameters.GetValueAsInt("priorityLevel", command);
        }
        if (parameters.CheckIfHasParameter("blendMode"))
        {
            outPlaybackInfo->mBlendMode = (EMotionFX::EMotionBlendMode)parameters.GetValueAsInt("blendMode", command);
        }
        if (parameters.CheckIfHasParameter("playMode"))
        {
            outPlaybackInfo->mPlayMode = (EMotionFX::EPlayMode)parameters.GetValueAsInt("playMode", command);
        }
        if (parameters.CheckIfHasParameter("mirrorMotion"))
        {
            outPlaybackInfo->mMirrorMotion = parameters.GetValueAsBool("mirrorMotion", command);
        }
        if (parameters.CheckIfHasParameter("mix"))
        {
            outPlaybackInfo->mMix = parameters.GetValueAsBool("mix", command);
        }
        if (parameters.CheckIfHasParameter("playNow"))
        {
            outPlaybackInfo->mPlayNow = parameters.GetValueAsBool("playNow", command);
        }
        if (parameters.CheckIfHasParameter("motionExtraction"))
        {
            outPlaybackInfo->mMotionExtractionEnabled = parameters.GetValueAsBool("motionExtraction", command);
        }
        if (parameters.CheckIfHasParameter("retarget"))
        {
            outPlaybackInfo->mRetarget = parameters.GetValueAsBool("retarget", command);
        }
        if (parameters.CheckIfHasParameter("freezeAtLastFrame"))
        {
            outPlaybackInfo->mFreezeAtLastFrame = parameters.GetValueAsBool("freezeAtLastFrame", command);
        }
        if (parameters.CheckIfHasParameter("enableMotionEvents"))
        {
            outPlaybackInfo->mEnableMotionEvents = parameters.GetValueAsBool("enableMotionEvents", command);
        }
        if (parameters.CheckIfHasParameter("blendOutBeforeEnded"))
        {
            outPlaybackInfo->mBlendOutBeforeEnded = parameters.GetValueAsBool("blendOutBeforeEnded", command);
        }
        if (parameters.CheckIfHasParameter("canOverwrite"))
        {
            outPlaybackInfo->mCanOverwrite = parameters.GetValueAsBool("canOverwrite", command);
        }
        if (parameters.CheckIfHasParameter("deleteOnZeroWeight"))
        {
            outPlaybackInfo->mDeleteOnZeroWeight = parameters.GetValueAsBool("deleteOnZeroWeight", command);
        }
        if (parameters.CheckIfHasParameter("inPlace"))
        {
            outPlaybackInfo->mInPlace = parameters.GetValueAsBool("inPlace", command);
        }
    }


    // execute
    bool CommandPlayMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // clear our old data so that we start fresh in case of a redo
        m_oldData.clear();

        // check if there is any actor instance selected and if not return false so that the command doesn't get called and doesn't get inside the action history
        const uint32 numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();

        // verify if we actually have selected an actor instance
        if (numSelectedActorInstances == 0)
        {
            outResult = "Cannot play motion. No actor instance selected.";
            return false;
        }

        // get the motion
        AZStd::string filename;
        parameters.GetValue("filename", this, &filename);

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (auto fileIoBase{ AZ::IO::FileIOBase::GetInstance() }; fileIoBase && filename.starts_with('@'))
        {
            char resolvedPath[AZ::IO::MaxPathLength];
            if (fileIoBase->ResolvePath(filename.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
            {
                filename = resolvedPath;
            }
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str());
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot find motion '%s' in motion library.", filename.c_str());
            return false;
        }

        // fill the playback info based on the parameters
        EMotionFX::PlayBackInfo playbackInfo;
        CommandParametersToPlaybackInfo(this, parameters, &playbackInfo);

        // iterate through all actor instances and start playing all selected motions
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            UndoObject undoObject;

            // reset the anim graph instance so that the motion will actually play
            undoObject.m_animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (undoObject.m_animGraphInstance)
            {
                undoObject.m_animGraph = undoObject.m_animGraphInstance->GetAnimGraph();
                actorInstance->SetAnimGraphInstance(nullptr);
            }

            // start playing the current motion
            MCORE_ASSERT(actorInstance->GetMotionSystem());
            EMotionFX::MotionInstance* motionInstance = actorInstance->GetMotionSystem()->PlayMotion(motion, &playbackInfo);

            // motion offset
            if (parameters.CheckIfHasParameter("useMotionOffset"))
            {
                if (parameters.CheckIfHasParameter("normalizedMotionOffset"))
                {
                    motionInstance->SetCurrentTimeNormalized(parameters.GetValueAsFloat("normalizedMotionOffset", this));
                    motionInstance->SetPause(true);
                }
                else
                {
                    MCore::LogWarning("Cannot use motion offset. The 'normalizedMotionOffset' parameter is not specified. When using motion offset you need to specify the normalized motion offset value.");
                }
            }

            // store what we did for the undo function
            undoObject.m_actorInstance = actorInstance;
            undoObject.m_motionInstance = motionInstance;
            m_oldData.push_back(undoObject);
        }

        return true;
    }


    // undo the command
    bool CommandPlayMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        bool result = true;

        // iterate through all undo objects
        for (const UndoObject& undoObject : m_oldData)
        {
            EMotionFX::ActorInstance* actorInstance = undoObject.m_actorInstance;
            EMotionFX::MotionInstance* motionInstance = undoObject.m_motionInstance;

            // check if the actor instance is valid
            if (EMotionFX::GetActorManager().CheckIfIsActorInstanceRegistered(actorInstance) == false)
            {
                //outResult = "Cannot stop motion instance. Actor instance is not registered in the actor manager.";
                //result = false;
                continue;
            }

            // check if the motion instance is valid
            MCORE_ASSERT(actorInstance->GetMotionSystem());
            if (actorInstance->GetMotionSystem()->CheckIfIsValidMotionInstance(motionInstance))
            {
                // stop the motion instance and remove it directly from the motion system
                motionInstance->Stop(0.0f);
                actorInstance->GetMotionSystem()->RemoveMotionInstance(motionInstance);
            }
            //else
            //{
            //  outResult = "Motion instance to be stopped is not valid.";
            //  return false;
            //}
        }

        return result;
    }


#define SYNTAX_MOTIONCOMMANDS                                                                                                                                                                                                                                                                                                          \
    GetSyntax().ReserveParameters(30);                                                                                                                                                                                                                                                                                                 \
    GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to play.", MCore::CommandSyntax::PARAMTYPE_STRING);                                                                                                                                                                                                  \
    /*GetSyntax().AddParameter( "mirrorPlaneNormal", "The motion mirror plane normal, which is (1,0,0) on default. This setting is only used when mMirrorMotion is set to true.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "(1, 0, 0)" );*/                                                                                            \
    GetSyntax().AddParameter("blendInTime", "The time, in seconds, which it will take to fully have blended to the target weight.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.3");                                                                                                                                                     \
    GetSyntax().AddParameter("blendOutTime", "The time, in seconds, which it takes to smoothly fadeout the motion, after it has been stopped playing.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.3");                                                                                                                                 \
    GetSyntax().AddParameter("playSpeed", "The playback speed factor. A value of 1 stands for the original speed, while for example 2 means twice the original speed.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");                                                                                                                 \
    GetSyntax().AddParameter("targetWeight", "The target weight, where 1 means fully active, and 0 means not active at all.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");                                                                                                                                                           \
    GetSyntax().AddParameter("eventWeightThreshold", "The motion event weight threshold. If the motion instance weight is lower than this value, no motion events will be executed for this motion instance.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                          \
    GetSyntax().AddParameter("maxPlayTime", "The maximum play time, in seconds. Set to zero or a negative value to disable it.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                                                                                                        \
    GetSyntax().AddParameter("retargetRootOffset", "The retarget root offset. Can be used to prevent actors from floating in the air or going through the ground. Read the manual for more information.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                               \
    GetSyntax().AddParameter("numLoops", "The number of times you want to play this motion. A value of EMFX_LOOPFOREVER (4294967296) means it will loop forever.", MCore::CommandSyntax::PARAMTYPE_INT, "4294967296");   /* 4294967296 == EMFX_LOOPFOREVER */                                                                          \
    GetSyntax().AddParameter("priorityLevel", "The priority level, the higher this value, the higher priority it has on overwriting other motions.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                        \
    GetSyntax().AddParameter("retargetRootIndex", "The retargeting root node index.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                                                                                       \
    GetSyntax().AddParameter("blendMode", "The motion blend mode. Please read the MotionInstance::SetBlendMode(...) method for more information.", MCore::CommandSyntax::PARAMTYPE_INT, "0");   /* 4294967296 == MCORE_INVALIDINDEX32 */                                                                                               \
    GetSyntax().AddParameter("playMode", "The motion playback mode. This means forward or backward playback.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                                                              \
    GetSyntax().AddParameter("mirrorMotion", "Is motion mirroring enabled or not? When set to true, the mMirrorPlaneNormal is used as mirroring axis.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");                                                                                                                             \
    GetSyntax().AddParameter("mix", "Set to true if you want this motion to mix or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");                                                                                                                                                                                           \
    GetSyntax().AddParameter("playNow", "Set to true if you want to start playing the motion right away. If set to false it will be scheduled for later by inserting it into the motion queue.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");                                                                                     \
    GetSyntax().AddParameter("motionExtraction", "Set to true when you want to use motion extraction.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");                                                                                                                                                                              \
    GetSyntax().AddParameter("retarget", "Set to true if you want to enable motion retargeting. Read the manual for more information.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");                                                                                                                                             \
    GetSyntax().AddParameter("freezeAtLastFrame", "Set to true if you like the motion to freeze at the last frame, for example in case of a death motion.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");                                                                                                                         \
    GetSyntax().AddParameter("enableMotionEvents", "Set to true to enable motion events, or false to disable processing of motion events for this motion instance.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");                                                                                                                 \
    GetSyntax().AddParameter("blendOutBeforeEnded", "Set to true if you want the motion to be stopped so that it exactly faded out when the motion/loop fully finished. If set to false it will fade out after the loop has completed (and starts repeating). The default is true.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true"); \
    GetSyntax().AddParameter("canOverwrite", "Set to true if you want this motion to be able to delete other underlaying motion instances when this motion instance reaches a weight of 1.0.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");                                                                                       \
    GetSyntax().AddParameter("deleteOnZeroWeight", "Set to true if you wish to delete this motion instance once it reaches a weight of 0.0.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");                                                                                                                                        \
    GetSyntax().AddParameter("normalizedMotionOffset", "The normalized motion offset time to be used when the useMotionOffset flag is enabled. 0.0 means motion offset is disabled while 1.0 means the motion starts at the end of the motion.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                        \
    GetSyntax().AddParameter("useMotionOffset", "Set to true if you wish to use the motion offset. This will start the motion from the given normalized motion offset value instead of from time=0.0. The motion instance will get paused afterwards.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");                             \
    GetSyntax().AddParameter("inPlace", "Set to true if you wish to play the motion in place. The root of the skeleton will stay at its bind pose value.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");

    // init the syntax of the command
    void CommandPlayMotion::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandPlayMotion::GetDescription() const
    {
        return "This command can be used to start playing the given motion on the selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotionInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionInstance::CommandAdjustMotionInstance(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionInstance", orgCommand)
    {
    }


    // destructor
    CommandAdjustMotionInstance::~CommandAdjustMotionInstance()
    {
    }


    void CommandAdjustMotionInstance::AdjustMotionInstance(Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionInstance* motionInstance)
    {
        //if (parameters.CheckIfHasParameter( "mirrorPlaneNormal" ))    motionInstance->SetMirrorPlaneNormal        = parameters.GetValueAsVector3( "mirrorPlaneNormal", command );
        if (parameters.CheckIfHasParameter("playSpeed"))
        {
            motionInstance->SetPlaySpeed(parameters.GetValueAsFloat("playSpeed", command));
        }
        if (parameters.CheckIfHasParameter("eventWeightThreshold"))
        {
            motionInstance->SetEventWeightThreshold(parameters.GetValueAsFloat("eventWeightThreshold", command));
        }
        if (parameters.CheckIfHasParameter("maxPlayTime"))
        {
            motionInstance->SetMaxPlayTime(parameters.GetValueAsFloat("maxPlayTime", command));
        }
        //if (parameters.CheckIfHasParameter( "retargetRootOffset" ))   motionInstance->SetRetargetRootOffset( parameters.GetValueAsFloat( "retargetRootOffset", command ) );
        if (parameters.CheckIfHasParameter("numLoops"))
        {
            motionInstance->SetNumCurrentLoops(parameters.GetValueAsInt("numLoops", command));
        }
        if (parameters.CheckIfHasParameter("priorityLevel"))
        {
            motionInstance->SetPriorityLevel(parameters.GetValueAsInt("priorityLevel", command));
        }
        //  if (parameters.CheckIfHasParameter( "retargetRootIndex" ))      motionInstance->SetRetargetRootIndex( parameters.GetValueAsInt( "retargetRootIndex", command ) );
        if (parameters.CheckIfHasParameter("blendMode"))
        {
            motionInstance->SetBlendMode(static_cast<EMotionFX::EMotionBlendMode>(parameters.GetValueAsInt("blendMode", command)));
        }
        if (parameters.CheckIfHasParameter("playMode"))
        {
            motionInstance->SetPlayMode((EMotionFX::EPlayMode)parameters.GetValueAsInt("playMode", command));
        }
        if (parameters.CheckIfHasParameter("mirrorMotion"))
        {
            motionInstance->SetMirrorMotion(parameters.GetValueAsBool("mirrorMotion", command));
        }
        if (parameters.CheckIfHasParameter("mix"))
        {
            motionInstance->SetMixMode(parameters.GetValueAsBool("mix", command));
        }
        if (parameters.CheckIfHasParameter("motionExtraction"))
        {
            motionInstance->SetMotionExtractionEnabled(parameters.GetValueAsBool("motionExtraction", command));
        }
        if (parameters.CheckIfHasParameter("retarget"))
        {
            motionInstance->SetRetargetingEnabled(parameters.GetValueAsBool("retarget", command));
        }
        if (parameters.CheckIfHasParameter("freezeAtLastFrame"))
        {
            motionInstance->SetFreezeAtLastFrame(parameters.GetValueAsBool("freezeAtLastFrame", command));
        }
        if (parameters.CheckIfHasParameter("enableMotionEvents"))
        {
            motionInstance->SetMotionEventsEnabled(parameters.GetValueAsBool("enableMotionEvents", command));
        }
        if (parameters.CheckIfHasParameter("blendOutBeforeEnded"))
        {
            motionInstance->SetBlendOutBeforeEnded(parameters.GetValueAsBool("blendOutBeforeEnded", command));
        }
        if (parameters.CheckIfHasParameter("canOverwrite"))
        {
            motionInstance->SetCanOverwrite(parameters.GetValueAsBool("canOverwrite", command));
        }
        if (parameters.CheckIfHasParameter("deleteOnZeroWeight"))
        {
            motionInstance->SetDeleteOnZeroWeight(parameters.GetValueAsBool("deleteOnZeroWeight", command));
        }
        if (parameters.CheckIfHasParameter("inPlace"))
        {
            motionInstance->SetIsInPlace(parameters.GetValueAsBool("inPlace", command));
        }
    }


    // execute
    bool CommandAdjustMotionInstance::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(outResult);

        // iterate through the motion instances and modify them
        const uint32 numSelectedMotionInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedMotionInstances();
        for (uint32 i = 0; i < numSelectedMotionInstances; ++i)
        {
            // get the current selected motion instance and adjust it based on the parameters
            EMotionFX::MotionInstance* selectedMotionInstance = GetCommandManager()->GetCurrentSelection().GetMotionInstance(i);
            AdjustMotionInstance(this, parameters, selectedMotionInstance);
        }

        return true;
    }


    // undo the command
    bool CommandAdjustMotionInstance::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // TODO: not sure yet how to undo this on the best way
        return true;
    }


    // init the syntax of the command
    void CommandAdjustMotionInstance::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandAdjustMotionInstance::GetDescription() const
    {
        return "This command can be used to adjust the selected motion instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustDefaultPlayBackInfo
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustDefaultPlayBackInfo::CommandAdjustDefaultPlayBackInfo(MCore::Command* orgCommand)
        : MCore::Command("AdjustDefaultPlayBackInfo", orgCommand)
    {
    }


    // destructor
    CommandAdjustDefaultPlayBackInfo::~CommandAdjustDefaultPlayBackInfo()
    {
    }


    // execute
    bool CommandAdjustDefaultPlayBackInfo::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion
        AZStd::string filename;
        parameters.GetValue("filename", this, &filename);

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str());
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot find motion '%s' in motion library.", filename.c_str());
            return false;
        }

        // get the default playback info from the motion
        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

        // copy the current playback info to the undo data
        mOldPlaybackInfo = *defaultPlayBackInfo;

        // adjust the playback info based on the parameters
        CommandPlayMotion::CommandParametersToPlaybackInfo(this, parameters, defaultPlayBackInfo);

        // save the current dirty flag and tell the motion that something got changed
        mOldDirtyFlag = motion->GetDirtyFlag();
        return true;
    }


    // undo the command
    bool CommandAdjustDefaultPlayBackInfo::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion
        AZStd::string filename;
        parameters.GetValue("filename", this, &filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str());
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot find motion '%s' in motion library.", filename.c_str());
            return false;
        }

        // get the default playback info from the motion
        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        if (defaultPlayBackInfo == nullptr)
        {
            outResult = AZStd::string::format("Motion '%s' does not have a default playback info. Cannot adjust default playback info.", filename.c_str());
            return false;
        }

        // copy the saved playback info to the actual one
        *defaultPlayBackInfo = mOldPlaybackInfo;

        // set the dirty flag back to the old value
        motion->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustDefaultPlayBackInfo::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandAdjustDefaultPlayBackInfo::GetDescription() const
    {
        return "This command can be used to adjust the default playback info of the given motion.";
    }


    //--------------------------------------------------------------------------------
    // CommandStopMotionInstances
    //--------------------------------------------------------------------------------

    // execute
    bool CommandStopMotionInstances::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // clear our old data so that we start fresh in case of a redo
        //mOldData.Clear();

        // get the number of selected actor instances
        const uint32 numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();

        // check if there is any actor instance selected and if not return false so that the command doesn't get called and doesn't get inside the action history
        if (numSelectedActorInstances == 0)
        {
            return false;
        }

        // get the motion
        AZStd::string filename;
        parameters.GetValue("filename", this, &filename);

        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str());
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot find motion '%s' in motion library.", filename.c_str());
            return false;
        }

        // iterate through all actor instances and stop all selected motion instances
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            // get the actor instance and the corresponding motion system
            EMotionFX::ActorInstance*   actorInstance   = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::MotionSystem*    motionSystem    = actorInstance->GetMotionSystem();

            // stop simulating the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                animGraphInstance->Stop();
            }

            // get the number of motion instances and iterate through them
            const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
            for (uint32 j = 0; j < numMotionInstances; ++j)
            {
                EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);

                // stop the motion instance in case its from the given motion
                if (motion == motionInstance->GetMotion())
                {
                    motionInstance->Stop();
                }
            }
        }

        return true;
    }


    // undo the command
    bool CommandStopMotionInstances::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandStopMotionInstances::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to stop all motion instances for.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandStopMotionInstances::GetDescription() const
    {
        return "Stop all motion instances for the currently selected motions on all selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandStopAllMotionInstances
    //--------------------------------------------------------------------------------

    // execute
    bool CommandStopAllMotionInstances::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // clear our old data so that we start fresh in case of a redo
        //mOldData.Clear();

        // iterate through all actor instances and stop all selected motion instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and the corresponding motion system
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::MotionSystem*    motionSystem    = actorInstance->GetMotionSystem();

            // stop simulating the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                animGraphInstance->Stop();
            }

            // get the number of motion instances and iterate through them
            const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
            for (uint32 j = 0; j < numMotionInstances; ++j)
            {
                // get the motion instance and stop it
                EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
                motionInstance->Stop(0.0f);
            }

            // directly remove the motion instances
            actorInstance->UpdateTransformations(0.0f, true);
        }

        return true;
    }


    // undo the command
    bool CommandStopAllMotionInstances::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandStopAllMotionInstances::InitSyntax()
    {
    }


    // get the description
    const char* CommandStopAllMotionInstances::GetDescription() const
    {
        return "Stop all currently playing motion instances on all selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotion::CommandAdjustMotion(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotion", orgCommand)
    {
    }


    void CommandAdjustMotion::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustMotion, MCore::Command, MotionIdCommandMixin>()
            ->Version(1)
            ->Field("dirtyFlag", &CommandAdjustMotion::m_dirtyFlag)
            ->Field("motionExtractionFlags", &CommandAdjustMotion::m_extractionFlags)
            ->Field("name", &CommandAdjustMotion::m_name)
        ;
    }


    // execute
    bool CommandAdjustMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // check if the motion with the given id exists
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust motion. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // adjust the dirty flag
        if (m_dirtyFlag)
        {
            mOldDirtyFlag = motion->GetDirtyFlag();
            motion->SetDirtyFlag(m_dirtyFlag.value());
        }

        // adjust the name
        if (m_name)
        {
            mOldName = motion->GetName();
            motion->SetName(m_name.value().c_str());

            mOldDirtyFlag = motion->GetDirtyFlag();
            motion->SetDirtyFlag(true);
        }

        // Adjust the motion extraction flags.
        if (m_extractionFlags)
        {
            mOldExtractionFlags = motion->GetMotionExtractionFlags();
            motion->SetMotionExtractionFlags(m_extractionFlags.value());
            mOldDirtyFlag = motion->GetDirtyFlag();
            motion->SetDirtyFlag(true);
        }

        return true;
    }


    // undo the command
    bool CommandAdjustMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        m_motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion with the given id exists
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(m_motionID);
        if (!motion)
        {
            outResult = AZStd::string::format("Cannot adjust motion. Motion with id='%i' does not exist.", m_motionID);
            return false;
        }

        // adjust the dirty flag
        if (m_dirtyFlag)
        {
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        // adjust the name
        if (m_name)
        {
            motion->SetName(mOldName.c_str());
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        if (m_extractionFlags)
        {
            motion->SetMotionExtractionFlags(mOldExtractionFlags);
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandAdjustMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion to adjust.",                                                     MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("dirtyFlag",                "The dirty flag indicates whether the user has made changes to the motion or not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("name",                     "The name of the motion.",                                                             MCore::CommandSyntax::PARAMTYPE_STRING, "Unknown Motion");
        GetSyntax().AddParameter("motionExtractionFlags",    "The motion extraction flags value.",                                                  MCore::CommandSyntax::PARAMTYPE_INT, "0");
    }


    bool CommandAdjustMotion::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        MotionIdCommandMixin::SetCommandParameters(parameters);
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            m_dirtyFlag = true;
        }

        if (parameters.CheckIfHasParameter("name"))
        {
            m_name = AZStd::string();
            parameters.GetValue("name", this, m_name.value());
        }

        if (parameters.CheckIfHasParameter("motionExtractionFlags"))
        {
            m_extractionFlags = static_cast<EMotionFX::EMotionExtractionFlags>(parameters.GetValueAsInt("motionExtractionFlags", this));
        }

        return true;
    }

    // get the description
    const char* CommandAdjustMotion::GetDescription() const
    {
        return "This command can be used to adjust the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotion::CommandRemoveMotion(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotion", orgCommand)
    {
        mOldMotionID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandRemoveMotion::~CommandRemoveMotion()
    {
    }


    // execute
    bool CommandRemoveMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string filename;
        parameters.GetValue("filename", "", filename);
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // find the corresponding motion
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str());
        if (!motion)
        {
            // Make sure potential dangling motions are removed from the selection list. This can happen in a command group where we remove a motion set,
            // which internally destroys the motions, followed by a remove motion command that has been part of the motion set.
            const AZStd::string commandString = AZStd::string::format("Unselect -motionName \"%s\"", filename.c_str());
            GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

            return true;
        }

        if (motion->GetIsOwnedByRuntime())
        {
            outResult = AZStd::string::format("Cannot remove motion. Motion with filename '%s' is being used by the engine runtime.", filename.c_str());
            return false;
        }

        // make sure the motion is not part of any motion set
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // get the current motion set and check if the motion we want to remove is used by it
            EMotionFX::MotionSet*               motionSet   = EMotionFX::GetMotionManager().GetMotionSet(i);
            EMotionFX::MotionSet::MotionEntry*  motionEntry = motionSet->FindMotionEntry(motion);
            if (motionEntry)
            {
                // Unlink the motion from the motion entry so that it is safe to remove it.
                motionEntry->Reset();
            }
        }

        // Remove motion from selection list.
        const AZStd::string commandString = AZStd::string::format("Unselect -motionName \"%s\"", motion->GetName());
        GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult);

        // store the previously used id and remove the motion
        mOldIndex           = EMotionFX::GetMotionManager().FindMotionIndex(motion);
        mOldMotionID        = motion->GetID();
        mOldFileName        = motion->GetFileName();

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        EMotionFX::GetMotionManager().RemoveMotionByID(motion->GetID());
        return true;
    }


    // undo the command
    bool CommandRemoveMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // execute the group command
        AZStd::string commandString;
        commandString = AZStd::string::format("ImportMotion -filename \"%s\" -motionID %i", mOldFileName.c_str(), mOldMotionID);
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveMotion::GetDescription() const
    {
        return "This command can be used to remove the given motion from the motion library.";
    }


    //--------------------------------------------------------------------------------
    // CommandScaleMotionData
    //--------------------------------------------------------------------------------

    // constructor
    CommandScaleMotionData::CommandScaleMotionData(MCore::Command* orgCommand)
        : MCore::Command("ScaleMotionData", orgCommand)
    {
        mMotionID       = MCORE_INVALIDINDEX32;
        mScaleFactor    = 1.0f;
        mOldDirtyFlag   = false;
    }


    // destructor
    CommandScaleMotionData::~CommandScaleMotionData()
    {
    }


    // execute
    bool CommandScaleMotionData::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::Motion* motion;
        if (parameters.CheckIfHasParameter("id"))
        {
            uint32 motionID = parameters.GetValueAsInt("id", MCORE_INVALIDINDEX32);

            motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
            if (motion == nullptr)
            {
                outResult = AZStd::string::format("Cannot get the motion, with ID %d.", motionID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No motion has been selected, please select one first.";
                return false;
            }

            // get the first selected motion
            motion = selection.GetMotion(0);
        }

        if (parameters.CheckIfHasParameter("unitType") == false && parameters.CheckIfHasParameter("scaleFactor") == false)
        {
            outResult = "You have to either specify -unitType or -scaleFactor.";
            return false;
        }

        mMotionID = motion->GetID();
        mScaleFactor = parameters.GetValueAsFloat("scaleFactor", 1.0f);

        AZStd::string targetUnitTypeString;
        parameters.GetValue("unitType", this, &targetUnitTypeString);
        mUseUnitType = parameters.CheckIfHasParameter("unitType");

        MCore::Distance::EUnitType targetUnitType;
        bool stringConvertSuccess = MCore::Distance::StringToUnitType(targetUnitTypeString, &targetUnitType);
        if (mUseUnitType && stringConvertSuccess == false)
        {
            outResult = AZStd::string::format("The passed unitType '%s' is not a valid unit type.", targetUnitTypeString.c_str());
            return false;
        }
        mOldUnitType = MCore::Distance::UnitTypeToString(motion->GetUnitType());

        mOldDirtyFlag = motion->GetDirtyFlag();
        motion->SetDirtyFlag(true);

        // perform the scaling
        if (mUseUnitType == false)
        {
            motion->Scale(mScaleFactor);
        }
        else
        {
            motion->ScaleToUnitType(targetUnitType);
        }

        return true;
    }


    // undo the command
    bool CommandScaleMotionData::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        if (mUseUnitType == false)
        {
            AZStd::string commandString;
            commandString = AZStd::string::format("ScaleMotionData -id %d -scaleFactor %.8f", mMotionID, 1.0f / mScaleFactor);
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);
        }
        else
        {
            AZStd::string commandString;
            commandString = AZStd::string::format("ScaleMotionData -id %d -unitType \"%s\"", mMotionID, mOldUnitType.c_str());
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(mMotionID);
        if (motion)
        {
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandScaleMotionData::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddParameter("id",                      "The identification number of the motion we want to scale.",                    MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("scaleFactor",             "The scale factor, for example 10.0 to make the motion pose 10x as large.",     MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("unitType",                "The unit type to convert to, for example 'meters'.",                           MCore::CommandSyntax::PARAMTYPE_STRING,     "meters");
        GetSyntax().AddParameter("skipInterfaceUpdate",     ".",                                                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
    }


    // get the description
    const char* CommandScaleMotionData::GetDescription() const
    {
        return "This command can be used to scale all internal motion data. This means positional keyframe data will be modified as well as stored pose and bind pose data.";
    }



    //--------------------------------------------------------------------------------
    // Helper Functions
    //--------------------------------------------------------------------------------

    void LoadMotionsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }
        const size_t numFileNames = filenames.size();

        const AZStd::string commandGroupName = AZStd::string::format("%s %zu motion%s", reload ? "Reload" : "Load", numFileNames, (numFileNames > 1) ? "s" : "");
        MCore::CommandGroup commandGroup(commandGroupName, static_cast<uint32>(numFileNames * 2));

        AZStd::string command;
        const EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();
        for (size_t i = 0; i < numFileNames; ++i)
        {
            const AZStd::string& filename = filenames[i];
            const EMotionFX::Motion* motion = motionManager.FindMotionByFileName(filename.c_str());

            if (reload && motion)
            {
                // Remove the old motion first and then load the motion again.
                command = AZStd::string::format("RemoveMotion -filename \"%s\"", filename.c_str());
                commandGroup.AddCommandString(command);

                // Make sure the motion id stays the same after re-importing it.
                command = AZStd::string::format("ImportMotion -filename \"%s\" -motionID %d", filename.c_str(), motion->GetID());
                commandGroup.AddCommandString(command);
            }
            else
            {
                // Just import the motion.
                command = AZStd::string::format("ImportMotion -filename \"%s\"", filename.c_str());
                commandGroup.AddCommandString(command);
            }
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            if (!result.empty())
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }

        // Reset unique datas for nodes that operate with motions.
        const size_t numInst = EMotionFX::GetAnimGraphManager().GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInst; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = EMotionFX::GetAnimGraphManager().GetAnimGraphInstance(i);
            animGraphInstance->RecursiveInvalidateUniqueDatas();
        }
    }


    void ClearMotions(MCore::CommandGroup* commandGroup, bool forceRemove)
    {
        // iterate through the motions and put them into some array
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        AZStd::vector<EMotionFX::Motion*> motionsToRemove;
        motionsToRemove.reserve(numMotions);

        for (uint32 i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            motionsToRemove.push_back(motion);
        }

        // construct the command group and remove the selected motions
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        RemoveMotions(motionsToRemove, &failedRemoveMotions, commandGroup, forceRemove);
    }


    void RemoveMotions(const AZStd::vector<EMotionFX::Motion*>& motions, AZStd::vector<EMotionFX::Motion*>* outFailedMotions, MCore::CommandGroup* commandGroup, bool forceRemove)
    {
        outFailedMotions->clear();

        if (motions.empty())
        {
            return;
        }

        const size_t numMotions = motions.size();

        // Set the command group name.
        AZStd::string commandGroupName;
        if (numMotions == 1)
        {
            commandGroupName = "Remove 1 motion";
        }
        else
        {
            commandGroupName = AZStd::string::format("Remove %zu motions", numMotions);
        }

        // Create the internal command group which is used in case the parameter command group is not specified.
        MCore::CommandGroup internalCommandGroup(commandGroupName);

        // Iterate through all motions and remove them.
        AZStd::string commandString;
        for (uint32 i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = motions[i];

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            // Is the motion part of a motion set?
            bool isUsed = false;
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (uint32 j = 0; j < numMotionSets; ++j)
            {
                EMotionFX::MotionSet*               motionSet   = EMotionFX::GetMotionManager().GetMotionSet(j);
                EMotionFX::MotionSet::MotionEntry*  motionEntry = motionSet->FindMotionEntry(motion);

                if (motionEntry)
                {
                    outFailedMotions->push_back(motionEntry->GetMotion());
                    isUsed = true;
                    break;
                }
            }

            if (!isUsed || forceRemove)
            {
                // Construct the command and add it to the corresponding group
                commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motion->GetFileName());

                if (!commandGroup)
                {
                    internalCommandGroup.AddCommandString(commandString);
                }
                else
                {
                    commandGroup->AddCommandString(commandString);
                }
            }
        }

        if (!commandGroup)
        {
            // Execute the internal command group in case the command group parameter is not specified.
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }
} // namespace CommandSystem
