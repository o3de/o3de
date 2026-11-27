/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/conversions.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SocketCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SocketSetup.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddSocket, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveSocket, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustSocket, EMotionFX::CommandAllocator)

    namespace
    {
        // The command line cannot carry quotes inside a quoted value.
        bool IsValidCommandString(const AZStd::string& value)
        {
            return value.find('"') == AZStd::string::npos;
        }

        AZStd::string ToCommandValue(const AZ::Vector3& value)
        {
            return AZStd::string::format("%.6f,%.6f,%.6f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
        }

        AZStd::string ToCommandValue(const AZ::Quaternion& value)
        {
            return AZStd::string::format("%.6f,%.6f,%.6f,%.6f", static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()), static_cast<float>(value.GetW()));
        }

        AZ::Quaternion FromVector4(const AZ::Vector4& value)
        {
            const AZ::Quaternion rotation(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
            return rotation.GetLengthSq() > AZ::Constants::FloatEpsilon ? rotation.GetNormalized() : AZ::Quaternion::CreateIdentity();
        }

        AZStd::string MakeUniqueSocketName(const SocketSetup& setup, const AZStd::string& requestedName)
        {
            const AZStd::string baseName = requestedName.empty() ? AZStd::string("Socket") : requestedName;
            AZStd::string uniqueName = baseName;
            size_t number = 1;
            while (setup.FindSocketIndex(uniqueName) != SocketSetup::InvalidIndex)
            {
                uniqueName = AZStd::string::format("%s %zu", baseName.c_str(), number++);
            }
            return uniqueName;
        }

        bool CheckJointExists(const Actor* actor, const AZStd::string& jointName, AZStd::string& outResult)
        {
            if (!actor->GetSkeleton()->FindNodeByNameNoCase(jointName.c_str()))
            {
                outResult = AZStd::string::format("Joint '%s' does not exist in actor '%s'.", jointName.c_str(), actor->GetName());
                return false;
            }
            return true;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandSocketHelpers
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool CommandSocketHelpers::AddSocket(AZ::u32 actorId, const AZStd::string& name, const AZStd::string& jointName,
        const AZ::Vector3& position, const AZ::Quaternion& rotation, MCore::CommandGroup* commandGroup)
    {
        if (!IsValidCommandString(name) || !IsValidCommandString(jointName))
        {
            return false;
        }

        AZStd::string command = AZStd::string::format("%s -%s %u -%s \"%s\" -%s %s -%s %s",
            CommandAddSocket::s_commandName,
            CommandAddSocket::s_actorIdParameterName, actorId,
            CommandAddSocket::s_jointNameParameterName, jointName.c_str(),
            CommandAddSocket::s_positionParameterName, ToCommandValue(position).c_str(),
            CommandAddSocket::s_rotationParameterName, ToCommandValue(rotation).c_str());

        if (!name.empty())
        {
            command += AZStd::string::format(" -%s \"%s\"", CommandAddSocket::s_nameParameterName, name.c_str());
        }

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup);
    }

    bool CommandSocketHelpers::RemoveSocket(AZ::u32 actorId, const AZStd::string& name, MCore::CommandGroup* commandGroup)
    {
        if (!IsValidCommandString(name))
        {
            return false;
        }

        const AZStd::string command = AZStd::string::format("%s -%s %u -%s \"%s\"",
            CommandRemoveSocket::s_commandName,
            CommandRemoveSocket::s_actorIdParameterName, actorId,
            CommandRemoveSocket::s_nameParameterName, name.c_str());

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup);
    }

    bool CommandSocketHelpers::AdjustSocket(AZ::u32 actorId, const AZStd::string& name, const AZStd::optional<AZStd::string>& newName,
        const AZStd::optional<AZStd::string>& jointName, const AZStd::optional<AZ::Vector3>& position,
        const AZStd::optional<AZ::Quaternion>& rotation, MCore::CommandGroup* commandGroup)
    {
        if (!IsValidCommandString(name) || (newName && !IsValidCommandString(*newName)) || (jointName && !IsValidCommandString(*jointName)))
        {
            return false;
        }

        AZStd::string command = AZStd::string::format("%s -%s %u -%s \"%s\"",
            CommandAdjustSocket::s_commandName,
            CommandAdjustSocket::s_actorIdParameterName, actorId,
            CommandAdjustSocket::s_nameParameterName, name.c_str());

        if (newName)
        {
            command += AZStd::string::format(" -%s \"%s\"", CommandAdjustSocket::s_newNameParameterName, newName->c_str());
        }
        if (jointName)
        {
            command += AZStd::string::format(" -%s \"%s\"", CommandAdjustSocket::s_jointNameParameterName, jointName->c_str());
        }
        if (position)
        {
            command += AZStd::string::format(" -%s %s", CommandAdjustSocket::s_positionParameterName, ToCommandValue(*position).c_str());
        }
        if (rotation)
        {
            command += AZStd::string::format(" -%s %s", CommandAdjustSocket::s_rotationParameterName, ToCommandValue(*rotation).c_str());
        }

        return CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandAddSocket
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAddSocket::s_commandName = "AddSocket";
    const char* CommandAddSocket::s_nameParameterName = "name";
    const char* CommandAddSocket::s_jointNameParameterName = "jointName";
    const char* CommandAddSocket::s_positionParameterName = "position";
    const char* CommandAddSocket::s_rotationParameterName = "rotation";

    CommandAddSocket::CommandAddSocket(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    bool CommandAddSocket::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        if (!CheckJointExists(actor, m_jointName, outResult))
        {
            return false;
        }

        SocketSetup& setup = *actor->GetSocketSetup();
        m_name = MakeUniqueSocketName(setup, m_name);
        if (!setup.AddSocket(m_name, m_jointName, m_position, m_rotation))
        {
            outResult = AZStd::string::format("Cannot add socket '%s'.", m_name.c_str());
            return false;
        }
        setup.ResolveJointIndices(*actor->GetSkeleton());

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAddSocket::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        actor->GetSocketSetup()->RemoveSocket(m_name);
        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAddSocket::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(5);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_jointNameParameterName, "The parent joint of the socket.", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddParameter(s_nameParameterName, "The socket name; made unique if empty or already used.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_positionParameterName, "The position relative to the joint.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        syntax.AddParameter(s_rotationParameterName, "The rotation relative to the joint as a quaternion (x,y,z,w).", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "0.0,0.0,0.0,1.0");
    }

    bool CommandAddSocket::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);

        m_name.clear();
        if (parameters.CheckIfHasParameter(s_nameParameterName))
        {
            parameters.GetValue(s_nameParameterName, this, &m_name);
        }
        parameters.GetValue(s_jointNameParameterName, this, &m_jointName);
        m_position = parameters.GetValueAsVector3(s_positionParameterName, this);
        m_rotation = FromVector4(parameters.GetValueAsVector4(s_rotationParameterName, this));
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandRemoveSocket
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandRemoveSocket::s_commandName = "RemoveSocket";
    const char* CommandRemoveSocket::s_nameParameterName = "name";

    CommandRemoveSocket::CommandRemoveSocket(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    bool CommandRemoveSocket::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SocketSetup& setup = *actor->GetSocketSetup();
        m_oldIndex = setup.FindSocketIndex(m_name);
        if (m_oldIndex == SocketSetup::InvalidIndex)
        {
            outResult = AZStd::string::format("Socket '%s' does not exist.", m_name.c_str());
            return false;
        }

        m_oldSocket = setup.GetSocket(m_oldIndex);
        setup.RemoveSocket(m_name);

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandRemoveSocket::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SocketSetup& setup = *actor->GetSocketSetup();
        if (!setup.InsertSocketAt(m_oldIndex, m_oldSocket))
        {
            outResult = AZStd::string::format("Cannot restore socket '%s'.", m_name.c_str());
            return false;
        }
        setup.ResolveJointIndices(*actor->GetSkeleton());

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandRemoveSocket::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(2);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_nameParameterName, "The name of the socket to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }

    bool CommandRemoveSocket::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);
        parameters.GetValue(s_nameParameterName, this, &m_name);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommandAdjustSocket
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char* CommandAdjustSocket::s_commandName = "AdjustSocket";
    const char* CommandAdjustSocket::s_nameParameterName = "name";
    const char* CommandAdjustSocket::s_newNameParameterName = "newName";
    const char* CommandAdjustSocket::s_jointNameParameterName = "jointName";
    const char* CommandAdjustSocket::s_positionParameterName = "position";
    const char* CommandAdjustSocket::s_rotationParameterName = "rotation";

    CommandAdjustSocket::CommandAdjustSocket(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    bool CommandAdjustSocket::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SocketSetup& setup = *actor->GetSocketSetup();
        const size_t index = setup.FindSocketIndex(m_name);
        if (index == SocketSetup::InvalidIndex)
        {
            outResult = AZStd::string::format("Socket '%s' does not exist.", m_name.c_str());
            return false;
        }

        if (m_newName)
        {
            const size_t existing = setup.FindSocketIndex(*m_newName);
            if (m_newName->empty() || (existing != SocketSetup::InvalidIndex && existing != index))
            {
                outResult = AZStd::string::format("Cannot rename socket '%s' to '%s'; the name is empty or already used.", m_name.c_str(), m_newName->c_str());
                return false;
            }
        }
        if (m_jointName && !CheckJointExists(actor, *m_jointName, outResult))
        {
            return false;
        }

        ActorSocket& socket = setup.GetSocket(index);
        if (!m_oldSocket)
        {
            m_oldSocket = socket;
        }

        if (m_newName)
        {
            socket.SetName(*m_newName);
        }
        if (m_jointName)
        {
            socket.SetJointName(*m_jointName);
        }
        if (m_position)
        {
            socket.SetLocalPosition(*m_position);
        }
        if (m_rotation)
        {
            socket.SetLocalRotation(*m_rotation);
        }
        m_appliedName = socket.GetName();
        setup.ResolveJointIndices(*actor->GetSkeleton());

        m_oldDirtyFlag = actor->GetDirtyFlag();
        actor->SetDirtyFlag(true);
        return true;
    }

    bool CommandAdjustSocket::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        Actor* actor = GetActor(this, outResult);
        if (!actor)
        {
            return false;
        }

        SocketSetup& setup = *actor->GetSocketSetup();
        const size_t index = setup.FindSocketIndex(m_appliedName);
        if (index == SocketSetup::InvalidIndex || !m_oldSocket)
        {
            outResult = AZStd::string::format("Cannot restore socket '%s'.", m_name.c_str());
            return false;
        }

        setup.GetSocket(index) = *m_oldSocket;
        setup.ResolveJointIndices(*actor->GetSkeleton());

        actor->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAdjustSocket::InitSyntax()
    {
        MCore::CommandSyntax& syntax = GetSyntax();
        syntax.ReserveParameters(6);
        ParameterMixinActorId::InitSyntax(syntax);
        syntax.AddRequiredParameter(s_nameParameterName, "The name of the socket to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddParameter(s_newNameParameterName, "The new socket name.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_jointNameParameterName, "The new parent joint.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        syntax.AddParameter(s_positionParameterName, "The new position relative to the joint.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        syntax.AddParameter(s_rotationParameterName, "The new rotation relative to the joint as a quaternion (x,y,z,w).", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "0.0,0.0,0.0,1.0");
    }

    bool CommandAdjustSocket::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinActorId::SetCommandParameters(parameters);

        parameters.GetValue(s_nameParameterName, this, &m_name);

        m_newName.reset();
        m_jointName.reset();
        m_position.reset();
        m_rotation.reset();
        m_oldSocket.reset();

        if (parameters.CheckIfHasParameter(s_newNameParameterName))
        {
            AZStd::string value;
            parameters.GetValue(s_newNameParameterName, this, &value);
            m_newName = value;
        }
        if (parameters.CheckIfHasParameter(s_jointNameParameterName))
        {
            AZStd::string value;
            parameters.GetValue(s_jointNameParameterName, this, &value);
            m_jointName = value;
        }
        if (parameters.CheckIfHasParameter(s_positionParameterName))
        {
            m_position = parameters.GetValueAsVector3(s_positionParameterName, this);
        }
        if (parameters.CheckIfHasParameter(s_rotationParameterName))
        {
            m_rotation = FromVector4(parameters.GetValueAsVector4(s_rotationParameterName, this));
        }
        return true;
    }
} // namespace EMotionFX
