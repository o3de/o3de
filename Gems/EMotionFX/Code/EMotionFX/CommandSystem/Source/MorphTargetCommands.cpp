/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MorphTargetCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <MCore/Source/StringConversions.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAdjustMorphTarget
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMorphTarget::CommandAdjustMorphTarget(MCore::Command* orgCommand)
        : MCore::Command("AdjustMorphTarget", orgCommand)
    {
    }


    // destructor
    CommandAdjustMorphTarget::~CommandAdjustMorphTarget()
    {
    }


    // get the requested morph target and its instance
    bool CommandAdjustMorphTarget::GetMorphTarget(EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, uint32 lodLevel, const char* morphTargetName, EMotionFX::MorphTarget** outMorphTarget, EMotionFX::MorphSetupInstance::MorphTarget** outMorphTargetInstance, AZStd::string& outResult)
    {
        // reset the output results
        *outMorphTarget         = nullptr;
        *outMorphTargetInstance = nullptr;

        // check if either the actor or the actor instance is set
        if (actor == nullptr && actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust morph target '%s'. No actor or actor instance id given.", morphTargetName);
            return false;
        }

        // morph setup
        EMotionFX::MorphTarget* morphTarget = nullptr;
        if (actor)
        {
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(lodLevel);
            if (morphSetup)
            {
                // find the morph target based on the name
                morphTarget = morphSetup->FindMorphTargetByName(morphTargetName);
                if (morphTarget == nullptr)
                {
                    outResult = AZStd::string::format("Cannot adjust morph target '%s'. The morph target does not exist in actor with the id %i.", morphTargetName, actor->GetID());
                    return false;
                }

                // set the output morph target
                *outMorphTarget = morphTarget;
            }
        }

        // morph setup instance
        if (actorInstance && morphTarget)
        {
            EMotionFX::MorphSetupInstance* morphSetupInstance = actorInstance->GetMorphSetupInstance();
            if (morphSetupInstance)
            {
                // find the morph target instance based on the id
                EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
                if (morphTargetInstance == nullptr)
                {
                    outResult = AZStd::string::format("Cannot adjust morph target '%s'. The morph target instance does not exist in the actor instance with id '%i'.", morphTargetName, actorInstance->GetID());
                    return false;
                }

                // set the output morph target instance
                *outMorphTargetInstance = morphTargetInstance;
            }
        }

        return true;
    }


    // execute
    bool CommandAdjustMorphTarget::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the name of the morph target
        AZStd::string valueString;
        parameters.GetValue("name", this, &valueString);

        // get the actor and the actor instance id and find the corresponding objects
        const uint32                actorID         = parameters.GetValueAsInt("actorID", this);
        const uint32                actorInstanceID = parameters.GetValueAsInt("actorInstanceID", this);
        EMotionFX::Actor*           actor           = EMotionFX::GetActorManager().FindActorByID(actorID);
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);

        // make sure to set the actor correctly in case the actor instance is set
        // this is needed to be able to only set the actor instance in the command but have this function still working
        if (actorInstance)
        {
            actor = actorInstance->GetActor();
        }

        if (!actor)
        {
            AZ_Error("EMotionFX", false, "Cannot adjust morph target. Actor with ID %i cannot be found.", actorID);
            return false;
        }

        // get the level of detail to work on
        const uint32 lodLevel = parameters.GetValueAsInt("lodLevel", this);

        // get the morph target and the corresponding morph target instance
        EMotionFX::MorphTarget* morphTarget;
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance;
        GetMorphTarget(actor, actorInstance, lodLevel, valueString.c_str(), &morphTarget, &morphTargetInstance, outResult);

        // set the new weight of the morph target
        if (parameters.CheckIfHasParameter("weight") && morphTargetInstance)
        {
            const float value   = parameters.GetValueAsFloat("weight", this);
            m_oldWeight          = morphTargetInstance->GetWeight();
            morphTargetInstance->SetWeight(value);
        }

        // set the new manual mode
        if (parameters.CheckIfHasParameter("manualMode") && morphTargetInstance)
        {
            const bool value        = parameters.GetValueAsBool("manualMode", this);
            m_oldManualModeEnabled   = morphTargetInstance->GetIsInManualMode();
            morphTargetInstance->SetManualMode(value);
        }

        // set the new range min of the morph target
        if (parameters.CheckIfHasParameter("rangeMin") && morphTarget)
        {
            const float value   = parameters.GetValueAsFloat("rangeMin", this);
            m_oldRangeMin        = morphTarget->GetRangeMin();
            morphTarget->SetRangeMin(value);
        }

        // set the new range max of the morph target
        if (parameters.CheckIfHasParameter("rangeMax") && morphTarget)
        {
            const float value   = parameters.GetValueAsFloat("rangeMax", this);
            m_oldRangeMax        = morphTarget->GetRangeMax();
            morphTarget->SetRangeMax(value);
        }

        // set the phoneme sets
        if (parameters.CheckIfHasParameter("phonemeAction") && morphTarget)
        {
            // get phoneme sets parameters
            AZStd::string phonemeSetsString;
            parameters.GetValue("phonemeAction", this, &valueString);
            parameters.GetValue("phonemeSets", this, &phonemeSetsString);

            // store old phoneme sets
            m_oldPhonemeSets = morphTarget->GetPhonemeSets();

            // remove the phoneme set
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "remove", false /* no case */))
            {
                // split the phoneme sets string and remove them
                AZStd::vector<AZStd::string> phonemeSets;
                AzFramework::StringFunc::Tokenize(phonemeSetsString.c_str(), phonemeSets, MCore::CharacterConstants::comma, true /* keep empty strings */, true /* keep space strings */);

                const size_t numPhonemeSets = phonemeSets.size();
                for (size_t i = 0; i < numPhonemeSets; ++i)
                {
                    EMotionFX::MorphTarget::EPhonemeSet phonemeSet = morphTarget->FindPhonemeSet(phonemeSets[i]);
                    morphTarget->EnablePhonemeSet(phonemeSet, false);
                }
            }
            else
            {
                if (AzFramework::StringFunc::Equal(valueString.c_str(), "clear", false /* no case */)) // clear the set
                {
                    morphTarget->SetPhonemeSets(EMotionFX::MorphTarget::PHONEMESET_NONE);
                }
                else // add the phoneme sets
                {
                    if (AzFramework::StringFunc::Equal(valueString.c_str(), "replace", false /* no case */)) // replace the set
                    {
                        morphTarget->SetPhonemeSets(EMotionFX::MorphTarget::PHONEMESET_NONE);
                    }

                    // split the phoneme sets string and add them
                    AZStd::vector<AZStd::string> phonemeSets;
                    AzFramework::StringFunc::Tokenize(phonemeSetsString.c_str(), phonemeSets, MCore::CharacterConstants::colon, false, true);

                    const size_t numPhonemeSets = phonemeSets.size();
                    for (size_t i = 0; i < numPhonemeSets; ++i)
                    {
                        EMotionFX::MorphTarget::EPhonemeSet phonemeSet = morphTarget->FindPhonemeSet(phonemeSets[i]);
                        morphTarget->EnablePhonemeSet(phonemeSet, true);
                    }
                }
            }
        }

        // save the current dirty flag and tell the actor that something got changed
        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }


    // undo the command
    bool CommandAdjustMorphTarget::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the name of the morph target
        AZStd::string valueString;
        parameters.GetValue("name", this, &valueString);

        // get the actor and the actor instance id and find the corresponding objects
        const uint32                actorID         = parameters.GetValueAsInt("actorID", this);
        const uint32                actorInstanceID = parameters.GetValueAsInt("actorInstanceID", this);
        EMotionFX::Actor*           actor           = EMotionFX::GetActorManager().FindActorByID(actorID);
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);

        // make sure to set the actor correctly in case the actor instance is set
        // this is needed to be able to only set the actor instance in the command but have this function still working
        if (actorInstance)
        {
            actor = actorInstance->GetActor();
        }

        // get the level of detail to work on
        const uint32 lodLevel = parameters.GetValueAsInt("lodLevel", this);

        // get the morph target and the corresponding morph target instance
        EMotionFX::MorphTarget* morphTarget;
        EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance;
        GetMorphTarget(actor, actorInstance, lodLevel, valueString.c_str(), &morphTarget, &morphTargetInstance, outResult);

        // set the old weight of the morph target
        if (parameters.CheckIfHasParameter("weight") && morphTargetInstance)
        {
            morphTargetInstance->SetWeight(m_oldWeight);
        }

        // set the old manual mode
        if (parameters.CheckIfHasParameter("manualMode") && morphTargetInstance)
        {
            morphTargetInstance->SetManualMode(m_oldManualModeEnabled);
        }

        // set the old range min
        if (parameters.CheckIfHasParameter("rangeMin") && morphTarget)
        {
            morphTarget->SetRangeMin(m_oldRangeMin);
        }

        // set the old range max
        if (parameters.CheckIfHasParameter("rangeMax") && morphTarget)
        {
            morphTarget->SetRangeMax(m_oldRangeMax);
        }

        // set the old phoneme sets
        if (parameters.CheckIfHasParameter("phonemeAction") && morphTarget)
        {
            morphTarget->SetPhonemeSets(m_oldPhonemeSets);
        }

        // set the dirty flag back to the old value
        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustMorphTarget::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(10);
        GetSyntax().AddRequiredParameter("name",                 "The name of the morph target.",                                                                                                                                                            MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("lodLevel",             "The level of detail to work on.",                                                                                                                                                          MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("actorID",              "The actor identification number of the actor to work on.",                                                                                                                                 MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("actorInstanceID",      "The actor instance identification number of the actor instance to work on.",                                                                                                               MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("weight",               "The floating point weight value for the morph target (Range [rangeMin, rangeMax]). For a normalized weight 0.0 means the morph target is not active at all, 1.0 means full influence.",    MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("rangeMin",             "The minimum possible weight value.",                                                                                                                                                       MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("rangeMax",             "The maximum possible weight value.",                                                                                                                                                       MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("manualMode",           "Set to true if you want to enable manual mode for the morph target, false if not.",                                                                                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
        GetSyntax().AddParameter("phonemeAction",        "Set to add/remove/clear corresponding to the action wanted.",                                                                                                                              MCore::CommandSyntax::PARAMTYPE_STRING,     "add");
        GetSyntax().AddParameter("phonemeSets",          "List of phoneme sets seperated by ',' char.",                                                                                                                                              MCore::CommandSyntax::PARAMTYPE_STRING,     "");
    }


    // get the description
    const char* CommandAdjustMorphTarget::GetDescription() const
    {
        return "This command can be used to adjust an attribute of a morph target.";
    }
} // namespace CommandSystem
