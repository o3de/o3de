/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <EMotionFX/CommandSystem/Source/CommandSystemConfig.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>


EMFX_FORWARD_DECLARE(AttributeParameterMask);
MCORE_FORWARD_DECLARE(Attribute);

namespace CommandSystem
{
    // Create a new anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateParameter, "Create an anim graph parameter", true)
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Remove a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveParameter, "Remove an anim graph parameter", true)
        size_t          mIndex;
        AZ::TypeId      mType;
        AZStd::string   mName;
        AZStd::string   mContents;
        AZStd::string   mParent;
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Adjust a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustParameter, "Adjust an anim graph parameter", true)
        AZ::TypeId      mOldType;
        AZStd::string   mOldName;
        AZStd::string   mOldContents;
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Move the parameter to another position.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphMoveParameter, "Move an anim graph parameter", true)
        AZStd::string   mOldParent;
        size_t          mOldIndex;
        bool            mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct COMMANDSYSTEM_API ParameterConnectionItem
    {
        uint32  mTargetNodePort;

        void SetParameterNodeName(const char* name)         { mParameterNodeNameID  = MCore::GetStringIdPool().GenerateIdForString(name); }
        void SetTargetNodeName(const char* name)            { mTargetNodeNameID     = MCore::GetStringIdPool().GenerateIdForString(name); }
        void SetParameterName(const char* name)             { mParameterNameID      = MCore::GetStringIdPool().GenerateIdForString(name); }

        const char* GetParameterNodeName() const            { return MCore::GetStringIdPool().GetName(mParameterNodeNameID).c_str(); }
        const char* GetTargetNodeName() const               { return MCore::GetStringIdPool().GetName(mTargetNodeNameID).c_str(); }
        const char* GetParameterName() const                { return MCore::GetStringIdPool().GetName(mParameterNameID).c_str(); }

    private:
        uint32  mParameterNodeNameID;
        uint32  mTargetNodeNameID;
        uint32  mParameterNameID;
    };

    COMMANDSYSTEM_API void RemoveConnectionsForParameter(EMotionFX::AnimGraph* animGraph, const char* parameterName, MCore::CommandGroup& commandGroup);
    COMMANDSYSTEM_API void RemoveConnectionsForParameters(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNames, MCore::CommandGroup& commandGroup);
    COMMANDSYSTEM_API bool BuildRemoveParametersCommandGroup(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNamesToRemove, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);

    // Construct the create parameter command string using the the given information.
    COMMANDSYSTEM_API void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, uint32 insertAtIndex = MCORE_INVALIDINDEX32);

} // namespace CommandSystem
