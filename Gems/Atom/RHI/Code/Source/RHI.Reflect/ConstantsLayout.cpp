/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ConstantsLayout::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ConstantsLayout>()
                ->Version(1)    // Version 1: Adding debug helper functions to Shader Resource Groups
                ->Field("m_inputs", &ConstantsLayout::m_inputs)
                ->Field("m_idReflection", &ConstantsLayout::m_idReflection)
                ->Field("m_sizeInBytes", &ConstantsLayout::m_sizeInBytes)
                ->Field("m_hash", &ConstantsLayout::m_hash);
        }

        IdReflectionMapForConstants::Reflect(context);
    }

    RHI::Ptr<ConstantsLayout> ConstantsLayout::Create()
    {
        return aznew ConstantsLayout;
    }

    void ConstantsLayout::AddShaderInput(const ShaderInputConstantDescriptor& descriptor)
    {
        m_inputs.push_back(descriptor);
    }

    void ConstantsLayout::Clear()
    {
        m_inputs.clear();
        m_idReflection.Clear();
        m_sizeInBytes = 0;
        m_hash = InvalidHash;
    }

    bool ConstantsLayout::IsFinalized() const
    {
        return m_hash != InvalidHash;
    }

    bool ConstantsLayout::Finalize()
    {
        m_hash = HashValue64{ 0 };
        uint32_t constantInputIndex = 0;
        uint32_t constantDataSize = 0;
        for (const auto& constantDescriptor : m_inputs)
        {
            const ShaderInputConstantIndex inputIndex(constantInputIndex);
            if (!m_idReflection.Insert(constantDescriptor.m_name, inputIndex))
            {
                Clear();
                return false;
            }

            uint32_t end = constantDescriptor.m_constantByteOffset + constantDescriptor.m_constantByteCount;
            constantDataSize = AZStd::max(constantDataSize, end);

            ++constantInputIndex;
            m_hash = TypeHash64(constantDescriptor.GetHash(), m_hash);
        }

        m_sizeInBytes = constantDataSize;

        if (!ValidateConstantInputs())
        {
            Clear();
            return false;
        }

        return true;
    }

    HashValue64 ConstantsLayout::GetHash() const
    {
        return m_hash;
    }

    ShaderInputConstantIndex ConstantsLayout::FindShaderInputIndex(const Name& name) const
    {
        return m_idReflection.Find(name);
    }

    Interval ConstantsLayout::GetInterval(ShaderInputConstantIndex inputIndex) const
    {
        const ShaderInputConstantDescriptor& desc = GetShaderInput(inputIndex);
        uint32_t start = desc.m_constantByteOffset;
        uint32_t end = start + desc.m_constantByteCount;
        return Interval(start, end);
    }

    const ShaderInputConstantDescriptor& ConstantsLayout::GetShaderInput(ShaderInputConstantIndex inputIndex) const
    {
        const auto index = inputIndex.GetIndex();
        static const ShaderInputConstantDescriptor invalidDescriptor;
        return index < m_inputs.size() ? m_inputs[index] : invalidDescriptor;
    }

    AZStd::span<const ShaderInputConstantDescriptor> ConstantsLayout::GetShaderInputList() const
    {
        return m_inputs;
    }

    uint32_t ConstantsLayout::GetDataSize() const
    {
        return m_sizeInBytes;
    }

    bool ConstantsLayout::ValidateAccess(ShaderInputConstantIndex inputIndex) const
    {
        if (Validation::IsEnabled())
        {
            uint32_t count = static_cast<uint32_t>(m_inputs.size());
            if (inputIndex.GetIndex() >= count)
            {
                AZ_Assert(false, "Inline Constant Input index '%d' out of range [0,%d).", inputIndex.GetIndex(), count);
                return false;
            }
        }

        return true;
    }

    bool ConstantsLayout::ValidateConstantInputs() const
    {
        if (Validation::IsEnabled())
        {
            if (!m_sizeInBytes)
            {
                AZ_Assert(m_idReflection.IsEmpty(), "Constants size is not valid.");
                return m_idReflection.IsEmpty();
            }
        }

        return true;
    }

    void ConstantsLayout::DebugPrintNames(AZStd::span<const ShaderInputConstantIndex> constantList) const
    {
        AZStd::string output;
        for (const ShaderInputConstantIndex& constantIdx : constantList)
        {
            if (constantIdx.GetIndex() < m_inputs.size())
            {
                output += m_inputs[constantIdx.GetIndex()].m_name.GetCStr();
                output += " - ";
            }
        }
        AZ_Printf("RHI", output.c_str());
    }
}
