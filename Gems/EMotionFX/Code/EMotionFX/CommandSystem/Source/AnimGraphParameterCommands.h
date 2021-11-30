/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        bool            m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Remove a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveParameter, "Remove an anim graph parameter", true)
        size_t          m_index;
        AZ::TypeId      m_type;
        AZStd::string   m_name;
        AZStd::string   m_contents;
        AZStd::string   m_parent;
        bool            m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Adjust a given anim graph parameter.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAdjustParameter, "Adjust an anim graph parameter", true)
        AZ::TypeId      m_oldType;
        AZStd::string   m_oldName;
        AZStd::string   m_oldContents;
        bool            m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // Move the parameter to another position.
    MCORE_DEFINECOMMAND_START(CommandAnimGraphMoveParameter, "Move an anim graph parameter", true)
        AZStd::string   m_oldParent;
        size_t          m_oldIndex;
        bool            m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct COMMANDSYSTEM_API ParameterConnectionItem
    {
        void SetParameterNodeName(const char* name)         { m_parameterNodeNameId  = MCore::GetStringIdPool().GenerateIdForString(name); }
        void SetTargetNodeName(const char* name)            { m_targetNodeNameId     = MCore::GetStringIdPool().GenerateIdForString(name); }
        void SetParameterName(const char* name)             { m_parameterNameId      = MCore::GetStringIdPool().GenerateIdForString(name); }

        const char* GetParameterNodeName() const            { return MCore::GetStringIdPool().GetName(m_parameterNodeNameId).c_str(); }
        const char* GetTargetNodeName() const               { return MCore::GetStringIdPool().GetName(m_targetNodeNameId).c_str(); }
        const char* GetParameterName() const                { return MCore::GetStringIdPool().GetName(m_parameterNameId).c_str(); }

    private:
        uint32  m_parameterNodeNameId;
        uint32  m_targetNodeNameId;
        uint32  m_parameterNameId;
    };

    COMMANDSYSTEM_API void RemoveConnectionsForParameter(EMotionFX::AnimGraph* animGraph, const char* parameterName, MCore::CommandGroup& commandGroup);
    COMMANDSYSTEM_API void RemoveConnectionsForParameters(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNames, MCore::CommandGroup& commandGroup);
    COMMANDSYSTEM_API bool BuildRemoveParametersCommandGroup(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNamesToRemove, MCore::CommandGroup* commandGroup = nullptr);
    COMMANDSYSTEM_API void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);

    // Construct the create parameter command string using the the given information.
    COMMANDSYSTEM_API void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, size_t insertAtIndex = InvalidIndex);

} // namespace CommandSystem
