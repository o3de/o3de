/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>

#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void IndirectBufferViewArguments::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IndirectBufferViewArguments>()
                ->Version(0)
                ->Field("m_slot", &IndirectBufferViewArguments::m_slot)
                ;
        }
    }

    void IndirectCommandDescriptor::Reflect(ReflectContext* context)
    {
        IndirectCommandIndex::Reflect(context);
        IndirectBufferViewArguments::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IndirectCommandDescriptor>()
                ->Version(0)
                ->Field("m_type", &IndirectCommandDescriptor::m_type)
                ->Field("m_vertexBufferArgs", &IndirectCommandDescriptor::m_vertexBufferArgs)
                ;
        }
    }

    HashValue64 IndirectCommandDescriptor::GetHash(HashValue64 seed /*= 0*/) const
    {
        return TypeHash64(*this, seed);
    }

    void IndirectBufferLayout::Reflect(AZ::ReflectContext* context)
    {
        IndirectCommandDescriptor::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IndirectBufferLayout>()
                ->Version(0)
                ->Field("m_commands", &IndirectBufferLayout::m_commands)
                ->Field("m_idReflectionForCommands", &IndirectBufferLayout::m_idReflectionForCommands)
                ->Field("m_type", &IndirectBufferLayout::m_type)
                ->Field("m_hash", &IndirectBufferLayout::m_hash)
                ;
        }
    }

    bool IndirectBufferLayout::IsFinalized() const
    {
        return m_hash != HashValue64{ 0 };
    }

    bool IndirectBufferLayout::Finalize()
    {
        if (!ValidateFinalizeState(ValidateFinalizeStateExpect::NotFinalized))
        {
            return false;
        }

        // Calculate the hash and get the main command type while
        // iterating through the commands.
        m_type = IndirectBufferLayoutType::Undefined;
        m_hash = HashValue64{ 0 };
        for (uint32_t i = 0; i < m_commands.size(); ++i)
        {
            const auto& commandDesc = m_commands[i];

            m_hash = TypeHash64(commandDesc.GetHash(), m_hash);
            m_idReflectionForCommands[static_cast<uint64_t>(commandDesc.GetHash())] = IndirectCommandIndex(i);
            bool result = true;
            switch (commandDesc.m_type)
            {
            case IndirectCommandType::Draw:
                result = SetType(IndirectBufferLayoutType::LinearDraw);
                break;
            case IndirectCommandType::DrawIndexed:
                result = SetType(IndirectBufferLayoutType::IndexedDraw);
                break;
            case IndirectCommandType::Dispatch:
                result = SetType(IndirectBufferLayoutType::Dispatch);
                break;
            case IndirectCommandType::DispatchRays:
                result = SetType(IndirectBufferLayoutType::DispatchRays);
                break;
                default:
                    // Skip command
                    break;
            }

            if (!result)
            {
                return false;
            }
        }

        m_hash = TypeHash64(m_type, m_hash);

        if (Validation::IsEnabled())
        {
            if (m_type == IndirectBufferLayoutType::Undefined)
            {
                AZ_Assert(false, "Missing Draw, DrawIndexed or Dispatch command in the layout.");
                return false;
            }
        }
        return true;
    }

    HashValue64 IndirectBufferLayout::GetHash([[maybe_unused]] HashValue64 seed) const
    {
        return m_hash;
    }

    bool IndirectBufferLayout::AddIndirectCommand(const IndirectCommandDescriptor& command)
    {
        if (!ValidateCommand(command))
        {
            return false;
        }

        m_commands.push_back(command);
        return true;
    }

    AZStd::span<const IndirectCommandDescriptor> IndirectBufferLayout::GetCommands() const
    {
        if (!ValidateFinalizeState(ValidateFinalizeStateExpect::Finalized))
        {
            return AZStd::span<const IndirectCommandDescriptor>();
        }
        return m_commands;
    }

    IndirectCommandIndex IndirectBufferLayout::FindCommandIndex(const IndirectCommandDescriptor& command) const
    {
        auto findIt = m_idReflectionForCommands.find(static_cast<uint64_t>(command.GetHash()));
        return findIt == m_idReflectionForCommands.end() ? IndirectCommandIndex::Null : findIt->second;
    }

    IndirectBufferLayoutType IndirectBufferLayout::GetType() const
    {
        return m_type;
    }

    bool IndirectBufferLayout::ValidateFinalizeState(ValidateFinalizeStateExpect expect) const
    {
        if (Validation::IsEnabled())
        {
            if (expect == ValidateFinalizeStateExpect::Finalized && !IsFinalized())
            {
                AZ_Assert(false, "IndirectBufferLayout must be finalized when calling this method.");
                return false;
            }
            else if (expect == ValidateFinalizeStateExpect::NotFinalized && IsFinalized())
            {
                AZ_Assert(false, "IndirectBufferLayout cannot be finalized when calling this method.");
                return false;
            }
        }
        return true;
    }

    bool IndirectBufferLayout::ValidateCommand(const IndirectCommandDescriptor& command) const
    {
        if (Validation::IsEnabled())
        {
            if (IsFinalized())
            {
                AZ_Assert(false, "Layout already finalized");
                return false;
            }

            switch (command.m_type)
            {
            case IndirectCommandType::Draw:
            case IndirectCommandType::DrawIndexed:
            case IndirectCommandType::Dispatch:
            case IndirectCommandType::DispatchRays:
            case IndirectCommandType::IndexBufferView:
            case IndirectCommandType::RootConstants:
            case IndirectCommandType::VertexBufferView:
                if (AZStd::find(m_commands.begin(), m_commands.end(), command) != m_commands.end())
                {
                    AZ_Assert(false, "Duplicated command %d.", command.m_type);
                    return false;
                }
                break;
            default:
                AZ_Assert(false, "Invalid command type %d.", command.m_type);
                return false;
            }
        }

        return true;
    }

    bool IndirectBufferLayout::SetType(IndirectBufferLayoutType type)
    {
        if (Validation::IsEnabled())
        {
            if (m_type != IndirectBufferLayoutType::Undefined)
            {
                AZ_Assert(false, "Trying to set a layout type (%d) when one is already set (%d)", type, m_type);
                return false;
            }
        }

        m_type = type;
        return true;
    }
}
