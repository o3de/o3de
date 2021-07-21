/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMFX_MORPHTARGETCOMMANDS_H
#define __EMFX_MORPHTARGETCOMMANDS_H

// include the required headers
#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MorphSetupInstance.h>

namespace CommandSystem
{
    // adjust a given morph target of an actor
    MCORE_DEFINECOMMAND_START(CommandAdjustMorphTarget, "Adjust morph target", true)
    float                               mOldWeight;
    float                               mOldRangeMin;
    float                               mOldRangeMax;
    bool                                mOldManualModeEnabled;
    EMotionFX::MorphTarget::EPhonemeSet mOldPhonemeSets;
    bool                                mOldDirtyFlag;

    bool GetMorphTarget(EMotionFX::Actor* actor, EMotionFX::ActorInstance* actorInstance, uint32 lodLevel, const char* morphTargetName, EMotionFX::MorphTarget** outMorphTarget, EMotionFX::MorphSetupInstance::MorphTarget** outMorphTargetInstance, AZStd::string& outResult);
    MCORE_DEFINECOMMAND_END
} // namespace CommandSystem


#endif
