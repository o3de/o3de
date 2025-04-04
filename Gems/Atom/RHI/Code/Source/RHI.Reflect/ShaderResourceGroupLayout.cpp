/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ShaderResourceGroupLayout::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderResourceGroupLayout>()
                ->Version(8) // ATOM-15472
                ->Field("m_name", &ShaderResourceGroupLayout::m_name)
                ->Field("m_azslFileOfOrigin", &ShaderResourceGroupLayout::m_uniqueId)
                ->Field("m_staticSamplers", &ShaderResourceGroupLayout::m_staticSamplers)
                ->Field("m_inputsForBuffers", &ShaderResourceGroupLayout::m_inputsForBuffers)
                ->Field("m_inputsForImages", &ShaderResourceGroupLayout::m_inputsForImages)
                ->Field("m_inputsForBufferUnboundedArrays", &ShaderResourceGroupLayout::m_inputsForBufferUnboundedArrays)
                ->Field("m_inputsForImageUnboundedArrays", &ShaderResourceGroupLayout::m_inputsForImageUnboundedArrays)
                ->Field("m_inputsForSamplers", &ShaderResourceGroupLayout::m_inputsForSamplers)
                ->Field("m_intervalsForBuffers", &ShaderResourceGroupLayout::m_intervalsForBuffers)
                ->Field("m_intervalsForImages", &ShaderResourceGroupLayout::m_intervalsForImages)
                ->Field("m_intervalsForSamplers", &ShaderResourceGroupLayout::m_intervalsForSamplers)
                ->Field("m_groupSizeForBuffers", &ShaderResourceGroupLayout::m_groupSizeForBuffers)
                ->Field("m_groupSizeForImages", &ShaderResourceGroupLayout::m_groupSizeForImages)
                ->Field("m_groupSizeForBufferUnboundedArrays", &ShaderResourceGroupLayout::m_groupSizeForBufferUnboundedArrays)
                ->Field("m_groupSizeForImageUnboundedArrays", &ShaderResourceGroupLayout::m_groupSizeForImageUnboundedArrays)
                ->Field("m_groupSizeForSamplers", &ShaderResourceGroupLayout::m_groupSizeForSamplers)
                ->Field("m_idReflectionForBuffers", &ShaderResourceGroupLayout::m_idReflectionForBuffers)
                ->Field("m_idReflectionForImages", &ShaderResourceGroupLayout::m_idReflectionForImages)
                ->Field("m_idReflectionForBufferUnboundedArrays", &ShaderResourceGroupLayout::m_idReflectionForBufferUnboundedArrays)
                ->Field("m_idReflectionForImageUnboundedArrays", &ShaderResourceGroupLayout::m_idReflectionForImageUnboundedArrays)
                ->Field("m_idReflectionForSamplers", &ShaderResourceGroupLayout::m_idReflectionForSamplers)
                ->Field("m_constantsDataLayout", &ShaderResourceGroupLayout::m_constantsDataLayout)
                ->Field("m_bindingSlot", &ShaderResourceGroupLayout::m_bindingSlot)
                ->Field("m_shaderVariantKeyFallbackSize", &ShaderResourceGroupLayout::m_shaderVariantKeyFallbackSize)
                ->Field("m_shaderVariantKeyFallbackConstantIndex", &ShaderResourceGroupLayout::m_shaderVariantKeyFallbackConstantIndex)
                ->Field("m_hash", &ShaderResourceGroupLayout::m_hash)
                ;
        }

        IdReflectionMapForBuffers::Reflect(context);
        IdReflectionMapForImages::Reflect(context);
        IdReflectionMapForBufferUnboundedArrays::Reflect(context);
        IdReflectionMapForImageUnboundedArrays::Reflect(context);
        IdReflectionMapForSamplers::Reflect(context);
    }

    RHI::Ptr<ShaderResourceGroupLayout> ShaderResourceGroupLayout::Create()
    {
        return aznew ShaderResourceGroupLayout();
    }

    bool ShaderResourceGroupLayout::IsFinalized() const
    {
        return m_hash != HashValue64{ 0 };
    }

    bool ShaderResourceGroupLayout::ValidateFinalizeState(ValidateFinalizeStateExpect expect) const
    {
        if (Validation::IsEnabled())
        {
            if (expect == ValidateFinalizeStateExpect::Finalized && !IsFinalized())
            {
                AZ_Assert(false, "ShaderResourceGroupLayout must be finalized when calling this method.");
                return false;
            }
            else if (expect == ValidateFinalizeStateExpect::NotFinalized && IsFinalized())
            {
                AZ_Assert(false, "ShaderResourceGroupLayout cannot be finalized when calling this method.");
                return false;
            }
        }
        return true;
    }       

    template<typename IndexType>
    bool ShaderResourceGroupLayout::ValidateAccess(IndexType inputIndex, size_t inputIndexLimit, [[maybe_unused]] const char* inputArrayTypeName) const
    {
        if (Validation::IsEnabled())
        {
            if (inputIndex.GetIndex() >= inputIndexLimit)
            {
                AZ_Assert(false, "%s Input index '%d' out of range [0,%d).", inputArrayTypeName, inputIndex.GetIndex(), inputIndexLimit);
                return false;
            }
        }
        return true;
    }

    template<typename IndexType>
    bool ShaderResourceGroupLayout::ValidateAccess(IndexType inputIndex, uint32_t arrayIndex, size_t inputIndexLimit,  const char* inputArrayTypeName) const
    {
        if (Validation::IsEnabled())
        {
            if (!ValidateAccess(inputIndex, inputIndexLimit, inputArrayTypeName))
            {
                return false;
            }

            auto& descriptor = GetShaderInput(inputIndex);
            if (arrayIndex >= descriptor.m_count)
            {
                AZ_Assert(false, "%s Input '%s[%d]': Array index '%d' out of range [0,%d).", inputArrayTypeName, descriptor.m_name.GetCStr(), descriptor.m_count, arrayIndex, descriptor.m_count);
                return false;
            }
        }
        return true;
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsDataLayout->ValidateAccess(inputIndex);
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
    {
        return ValidateAccess(inputIndex, arrayIndex, m_inputsForBuffers.size(), "Buffer");
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
    {
        return ValidateAccess(inputIndex, arrayIndex, m_inputsForImages.size(), "Image");
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
    {
        return ValidateAccess(inputIndex, arrayIndex, m_inputsForSamplers.size(), "Sampler");
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex) const
    {
        return ValidateAccess(inputIndex, m_inputsForBufferUnboundedArrays.size(), "BufferUnboundedArray");
    }

    bool ShaderResourceGroupLayout::ValidateAccess(RHI::ShaderInputImageUnboundedArrayIndex inputIndex) const
    {
        return ValidateAccess(inputIndex, m_inputsForImageUnboundedArrays.size(), "ImageUnboundedArray");
    }

    ShaderResourceGroupLayout::ShaderResourceGroupLayout()
        : m_constantsDataLayout(ConstantsLayout::Create())
    {
    }

    void ShaderResourceGroupLayout::Clear()
    {
        m_staticSamplers.clear();

        m_inputsForBuffers.clear();
        m_inputsForImages.clear();
        m_inputsForSamplers.clear();

        m_intervalsForBuffers.clear();
        m_intervalsForImages.clear();
        m_intervalsForSamplers.clear();

        m_groupSizeForBuffers = 0;
        m_groupSizeForImages = 0;
        m_groupSizeForBufferUnboundedArrays = 0;
        m_groupSizeForImageUnboundedArrays = 0;
        m_groupSizeForSamplers = 0;

        m_idReflectionForBuffers.Clear();
        m_idReflectionForImages.Clear();
        m_idReflectionForSamplers.Clear();

        if (m_constantsDataLayout)
        {
            m_constantsDataLayout->Clear();
        }           

        m_bindingSlot = {};
        m_hash = HashValue64{ 0 };
    }

    template<typename NameIdReflectionMapT, typename ShaderInputDescriptorT, typename ShaderInputIndexT>
    bool ShaderResourceGroupLayout::FinalizeShaderInputGroup(
        const AZStd::vector<ShaderInputDescriptorT>& shaderInputDescriptors,
        AZStd::vector<Interval>& intervals,
        NameIdReflectionMapT& nameIdReflectionMap,
        uint32_t& groupSize)
    {
        intervals.reserve(shaderInputDescriptors.size());
        nameIdReflectionMap.Reserve(shaderInputDescriptors.size());

        uint32_t currentGroupSize = 0;
        uint32_t shaderInputIndex = 0;
        for (const ShaderInputDescriptorT& shaderInput : shaderInputDescriptors)
        {
            const ShaderInputIndexT inputIndex(shaderInputIndex);
            if (!nameIdReflectionMap.Insert(shaderInput.m_name, inputIndex))
            {
                Clear();
                return false;
            }

            // Add the [min, max) interval for the input in the group.
            intervals.emplace_back(currentGroupSize, currentGroupSize + shaderInput.m_count);

            currentGroupSize += shaderInput.m_count;
            ++shaderInputIndex;
        }

        groupSize = currentGroupSize;

        return true;
    }

    template<typename NameIdReflectionMapT, typename ShaderInputDescriptorT, typename ShaderInputIndexT>
    bool ShaderResourceGroupLayout::FinalizeUnboundedArrayShaderInputGroup(
        const AZStd::vector<ShaderInputDescriptorT>& shaderInputDescriptors,
        NameIdReflectionMapT& nameIdReflectionMap,
        uint32_t& groupSize)
    {
        nameIdReflectionMap.Reserve(shaderInputDescriptors.size());

        uint32_t currentGroupSize = 0;
        uint32_t shaderInputIndex = 0;
        for (const ShaderInputDescriptorT& shaderInput : shaderInputDescriptors)
        {
            const ShaderInputIndexT inputIndex(shaderInputIndex);
            if (!nameIdReflectionMap.Insert(shaderInput.m_name, inputIndex))
            {
                Clear();
                return false;
            }

            ++currentGroupSize;
            ++shaderInputIndex;
        }

        groupSize = currentGroupSize;

        return true;
    }

    bool ShaderResourceGroupLayout::Finalize()
    {
        if (IsFinalized())
        {
            return true;
        }

        if (m_bindingSlot.IsNull())
        {
            AZ_Error("ShaderResourceGroupLayout", false, "You must supply a valid binding slot to ShaderResourceGroupLayoutDescriptor.");
            Clear();
            return false;
        }

        if (m_bindingSlot.GetIndex() >= Limits::Pipeline::ShaderResourceGroupCountMax)
        {
            AZ_Error("ShaderResourceGroupLayout", false,
                "Binding index (%d) must be less than the maximum number of allowed shader resource groups (%d)",
                m_bindingSlot.GetIndex(), Limits::Pipeline::ShaderResourceGroupCountMax);
            Clear();
            return false;
        }

        // Build buffer group
        if (!FinalizeShaderInputGroup<IdReflectionMapForBuffers, ShaderInputBufferDescriptor, ShaderInputBufferIndex>(
            m_inputsForBuffers, m_intervalsForBuffers, m_idReflectionForBuffers, m_groupSizeForBuffers))
        {
            return false;
        }

        // Build image group
        if (!FinalizeShaderInputGroup<IdReflectionMapForImages, ShaderInputImageDescriptor, ShaderInputImageIndex>(
            m_inputsForImages, m_intervalsForImages, m_idReflectionForImages, m_groupSizeForImages))
        {
            return false;
        }

        // Build buffer unbounded array group
        if (!FinalizeUnboundedArrayShaderInputGroup<IdReflectionMapForBufferUnboundedArrays, ShaderInputBufferUnboundedArrayDescriptor, ShaderInputBufferUnboundedArrayIndex>(
            m_inputsForBufferUnboundedArrays, m_idReflectionForBufferUnboundedArrays, m_groupSizeForBufferUnboundedArrays))
        {
            return false;
        }

        // Build image unbounded array group
        if (!FinalizeUnboundedArrayShaderInputGroup<IdReflectionMapForImageUnboundedArrays, ShaderInputImageUnboundedArrayDescriptor, ShaderInputImageUnboundedArrayIndex>(
            m_inputsForImageUnboundedArrays, m_idReflectionForImageUnboundedArrays, m_groupSizeForImageUnboundedArrays))
        {
            return false;
        }

        // Build sampler group
        if (!FinalizeShaderInputGroup<IdReflectionMapForSamplers, ShaderInputSamplerDescriptor, ShaderInputSamplerIndex>(
            m_inputsForSamplers, m_intervalsForSamplers, m_idReflectionForSamplers, m_groupSizeForSamplers))
        {
            return false;
        }

        // Finalize the constants data layout.
        if (!m_constantsDataLayout->Finalize())
        {
            Clear();
            return false;
        }

        if (!m_shaderVariantKeyFallbackConstantId.IsEmpty())
        {
            m_shaderVariantKeyFallbackConstantIndex = FindShaderInputConstantIndex(m_shaderVariantKeyFallbackConstantId);

            if (Validation::IsEnabled())
            {
                AZ_Assert(m_shaderVariantKeyFallbackConstantIndex.IsValid(), "Failed to find a valid ShaderVariantKey fallback constant index!");
            }
        }

        // Build the final hash based on the inputs.
        {
            HashValue64 hash = HashValue64{ 0 };

            for (const ShaderInputStaticSamplerDescriptor& staticSamplerDescriptor : m_staticSamplers)
            {
                hash = staticSamplerDescriptor.GetHash(hash);
            }

            for (const ShaderInputBufferDescriptor& shaderInputBuffer : m_inputsForBuffers)
            {
                hash = shaderInputBuffer.GetHash(hash);
            }

            for (const ShaderInputImageDescriptor& shaderInputImage : m_inputsForImages)
            {
                hash = shaderInputImage.GetHash(hash);
            }

            for (const ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray : m_inputsForBufferUnboundedArrays)
            {
                hash = shaderInputBufferUnboundedArray.GetHash(hash);
            }

            for (const ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray : m_inputsForImageUnboundedArrays)
            {
                hash = shaderInputImageUnboundedArray.GetHash(hash);
            }

            for (const ShaderInputSamplerDescriptor& shaderInputSampler : m_inputsForSamplers)
            {
                hash = shaderInputSampler.GetHash(hash);
            }

            hash = TypeHash64(m_constantsDataLayout->GetHash(), hash);

            hash = TypeHash64(m_bindingSlot.GetIndex(), hash);
            hash = TypeHash64(m_shaderVariantKeyFallbackSize, hash);
            hash = TypeHash64(m_shaderVariantKeyFallbackConstantIndex.GetIndex(), hash);

            m_hash = hash;
        }

        return true;
    }

    void ShaderResourceGroupLayout::SetShaderVariantKeyFallback(const Name& shaderConstantName, uint32_t bitSize)
    {
        if (Validation::IsEnabled())
        {
            AZ_Assert(bitSize > 0, "ShaderVariant fallback must have positive key size!");
            AZ_Assert(!shaderConstantName.IsEmpty(), "ShaderVariant fallback must have a valid attribute name!");
        }

        m_shaderVariantKeyFallbackSize = bitSize;
        m_shaderVariantKeyFallbackConstantId = shaderConstantName;
    }

    uint32_t ShaderResourceGroupLayout::GetShaderVariantKeyFallbackSize() const
    {
        return m_shaderVariantKeyFallbackSize;
    }

    bool ShaderResourceGroupLayout::HasShaderVariantKeyFallbackEntry() const
    {
        return m_shaderVariantKeyFallbackSize > 0;
    }

    const RHI::ShaderInputConstantIndex& ShaderResourceGroupLayout::GetShaderVariantKeyFallbackConstantIndex() const
    {
        return m_shaderVariantKeyFallbackConstantIndex;
    }

    void ShaderResourceGroupLayout::AddStaticSampler(const ShaderInputStaticSamplerDescriptor& sampler)
    {
        m_staticSamplers.push_back(sampler);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputBufferDescriptor& buffer)
    {
        m_inputsForBuffers.push_back(buffer);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputImageDescriptor& image)
    {
        m_inputsForImages.push_back(image);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputBufferUnboundedArrayDescriptor& bufferUnboundedArray)
    {
        m_inputsForBufferUnboundedArrays.push_back(bufferUnboundedArray);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputImageUnboundedArrayDescriptor& imageUnboundedArray)
    {
        m_inputsForImageUnboundedArrays.push_back(imageUnboundedArray);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputSamplerDescriptor& sampler)
    {
        m_inputsForSamplers.push_back(sampler);
    }

    void ShaderResourceGroupLayout::AddShaderInput(const ShaderInputConstantDescriptor& constant)
    {
        m_constantsDataLayout->AddShaderInput(constant);
    }

    void ShaderResourceGroupLayout::SetBindingSlot(uint32_t bindingSlot)
    {
        m_bindingSlot = Handle<uint32_t>(bindingSlot);
    }

    AZStd::span<const ShaderInputStaticSamplerDescriptor> ShaderResourceGroupLayout::GetStaticSamplers() const
    {
        return m_staticSamplers;
    }

    ShaderInputBufferIndex ShaderResourceGroupLayout::FindShaderInputBufferIndex(const Name& name) const
    {
        return m_idReflectionForBuffers.Find(name);
    }

    ShaderInputImageIndex ShaderResourceGroupLayout::FindShaderInputImageIndex(const Name& name) const
    {
        return m_idReflectionForImages.Find(name);
    }

    ShaderInputSamplerIndex ShaderResourceGroupLayout::FindShaderInputSamplerIndex(const Name& name) const
    {
        return m_idReflectionForSamplers.Find(name);
    }

    ShaderInputConstantIndex ShaderResourceGroupLayout::FindShaderInputConstantIndex(const Name& name) const
    {
        return m_constantsDataLayout->FindShaderInputIndex(name);
    }

    ShaderInputBufferUnboundedArrayIndex ShaderResourceGroupLayout::FindShaderInputBufferUnboundedArrayIndex(const Name& name) const
    {
        return m_idReflectionForBufferUnboundedArrays.Find(name);
    }

    ShaderInputImageUnboundedArrayIndex ShaderResourceGroupLayout::FindShaderInputImageUnboundedArrayIndex(const Name& name) const
    {
        return m_idReflectionForImageUnboundedArrays.Find(name);
    }

    const ShaderInputBufferDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputBufferIndex index) const
    {
        return m_inputsForBuffers[index.GetIndex()];
    }

    const ShaderInputImageDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputImageIndex index) const
    {
        return m_inputsForImages[index.GetIndex()];
    }

    const ShaderInputBufferUnboundedArrayDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputBufferUnboundedArrayIndex index) const
    {
        return m_inputsForBufferUnboundedArrays[index.GetIndex()];
    }

    const ShaderInputImageUnboundedArrayDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputImageUnboundedArrayIndex index) const
    {
        return m_inputsForImageUnboundedArrays[index.GetIndex()];
    }

    const ShaderInputSamplerDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputSamplerIndex index) const
    {
        return m_inputsForSamplers[index.GetIndex()];
    }

    const ShaderInputConstantDescriptor& ShaderResourceGroupLayout::GetShaderInput(ShaderInputConstantIndex index) const
    {
        return m_constantsDataLayout->GetShaderInput(index);
    }

    AZStd::span<const ShaderInputBufferDescriptor> ShaderResourceGroupLayout::GetShaderInputListForBuffers() const
    {
        return m_inputsForBuffers;
    }

    AZStd::span<const ShaderInputImageDescriptor> ShaderResourceGroupLayout::GetShaderInputListForImages() const
    {
        return m_inputsForImages;
    }

    AZStd::span<const ShaderInputSamplerDescriptor> ShaderResourceGroupLayout::GetShaderInputListForSamplers() const
    {
        return m_inputsForSamplers;
    }

    AZStd::span<const ShaderInputConstantDescriptor> ShaderResourceGroupLayout::GetShaderInputListForConstants() const
    {
        return m_constantsDataLayout->GetShaderInputList();
    }

    AZStd::span<const ShaderInputBufferUnboundedArrayDescriptor> ShaderResourceGroupLayout::GetShaderInputListForBufferUnboundedArrays() const
    {
        return m_inputsForBufferUnboundedArrays;
    }

    AZStd::span<const ShaderInputImageUnboundedArrayDescriptor> ShaderResourceGroupLayout::GetShaderInputListForImageUnboundedArrays() const
    {
        return m_inputsForImageUnboundedArrays;
    }

    Interval ShaderResourceGroupLayout::GetGroupInterval(ShaderInputBufferIndex inputIndex) const
    {
        return m_intervalsForBuffers[inputIndex.GetIndex()];
    }

    Interval ShaderResourceGroupLayout::GetGroupInterval(ShaderInputImageIndex inputIndex) const
    {
        return m_intervalsForImages[inputIndex.GetIndex()];
    }

    Interval ShaderResourceGroupLayout::GetGroupInterval(ShaderInputSamplerIndex inputIndex) const
    {
        return m_intervalsForSamplers[inputIndex.GetIndex()];
    }

    Interval ShaderResourceGroupLayout::GetConstantInterval(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsDataLayout->GetInterval(inputIndex);
    }

    uint32_t ShaderResourceGroupLayout::GetGroupSizeForBuffers() const
    {
        return m_groupSizeForBuffers;
    }

    uint32_t ShaderResourceGroupLayout::GetGroupSizeForImages() const
    {
        return m_groupSizeForImages;
    }

    uint32_t ShaderResourceGroupLayout::GetGroupSizeForBufferUnboundedArrays() const
    {
        return m_groupSizeForBufferUnboundedArrays;
    }

    uint32_t ShaderResourceGroupLayout::GetGroupSizeForImageUnboundedArrays() const
    {
        return m_groupSizeForImageUnboundedArrays;
    }

    uint32_t ShaderResourceGroupLayout::GetGroupSizeForSamplers() const
    {
        return m_groupSizeForSamplers;
    }

    uint32_t ShaderResourceGroupLayout::GetConstantDataSize() const
    {
        return m_constantsDataLayout->GetDataSize();
    }

    uint32_t ShaderResourceGroupLayout::GetBindingSlot() const
    {
        ValidateFinalizeState(ValidateFinalizeStateExpect::Finalized);

        return m_bindingSlot.GetIndex();
    }

    HashValue64 ShaderResourceGroupLayout::GetHash() const
    {
        ValidateFinalizeState(ValidateFinalizeStateExpect::Finalized);

        return m_hash;
    }

    const ConstantsLayout* ShaderResourceGroupLayout::GetConstantsLayout() const
    {
        return m_constantsDataLayout.get();
    }
}
