/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
        void ConstantsLayout::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ConstantsLayout>()
                    ->Version(0)
                    ->Field("m_inputs", &ConstantsLayout::m_inputs)
                    ->Field("m_idReflection", &ConstantsLayout::m_idReflection)
                    ->Field("m_intervals", &ConstantsLayout::m_intervals)
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
            m_intervals.clear();
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

                // The constant data size is the maximum offset + size from the start of the struct.
                constantDataSize = AZStd::max(constantDataSize, constantDescriptor.m_constantByteOffset + constantDescriptor.m_constantByteCount);

                // Add the [min, max) interval for the inline constant.
                m_intervals.emplace_back(constantDescriptor.m_constantByteOffset, constantDescriptor.m_constantByteOffset + constantDescriptor.m_constantByteCount);

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
            return m_intervals[inputIndex.GetIndex()];
        }

        const ShaderInputConstantDescriptor& ConstantsLayout::GetShaderInput(ShaderInputConstantIndex inputIndex) const
        {
            return m_inputs[inputIndex.GetIndex()];
        }

        AZStd::array_view<ShaderInputConstantDescriptor> ConstantsLayout::GetShaderInputList() const
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
                    AZ_Assert(m_intervals.empty(), "Constants size is not valid.");
                    return m_intervals.empty();
                }
            }

            // [GFX TODO][ATOM-1669]: Review if it's needed to validate
            // overlapping of ranges.

            return true;
        }
    }
}
