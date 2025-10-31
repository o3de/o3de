/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>

namespace AZ::RHI
{
    // --- Constructors, assignment and reflection ---

    ShaderInputNameIndex::ShaderInputNameIndex(Name name)
    {
        *this = name;
    }

    ShaderInputNameIndex::ShaderInputNameIndex(const char* name)
    {
        *this = name;
    }

    void ShaderInputNameIndex::operator=(Name name)
    {
        Reset();
        m_name = name;
    }

    void ShaderInputNameIndex::operator=(const char* name)
    {
        Reset();
        m_name = Name(name);
    }

    void ShaderInputNameIndex::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Only serialize the Name field since the rest of the members are derived from the name at runtime
            serializeContext->Class<ShaderInputNameIndex>()
                ->Version(0)
                ->Field("Name", &ShaderInputNameIndex::m_name);
        }
    }

    // --- Functions for initializing the index ---

    void ShaderInputNameIndex::Initialize(IndexType indexType)
    {
        AssertHasName();
        m_initialized = true;
        m_inputType = indexType;
    }

    void ShaderInputNameIndex::FindBufferIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        Initialize(IndexType::ShaderBuffer);
        m_index = Handle<>(srgLayout->FindShaderInputBufferIndex(m_name).GetIndex());
    }

    void ShaderInputNameIndex::FindImageIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        Initialize(IndexType::ShaderImage);
        m_index = Handle<>(srgLayout->FindShaderInputImageIndex(m_name).GetIndex());
    }

    void ShaderInputNameIndex::FindSamplerIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        Initialize(IndexType::ShaderSampler);
        m_index = Handle<>(srgLayout->FindShaderInputSamplerIndex(m_name).GetIndex());
    }

    void ShaderInputNameIndex::FindConstantIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        Initialize(IndexType::ShaderConstant);
        m_index = Handle<>(srgLayout->FindShaderInputConstantIndex(m_name).GetIndex());
    }

    // --- Functions for checking if the index is initialized and retrieving it if not

    bool ShaderInputNameIndex::ValidateOrFindBufferIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        if (IsValid())      // 99% use case, check this first for a quick early out
        {
            return true;
        }
        if (!m_initialized)
        {
            FindBufferIndex(srgLayout);
            return IsValid();
        }
        return false;
    }

    bool ShaderInputNameIndex::ValidateOrFindImageIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        if (IsValid())      // 99% use case, check this first for a quick early out
        {
            return true;
        }
        if (!m_initialized)
        {
            FindImageIndex(srgLayout);
            return IsValid();
        }
        return false;
    }

    bool ShaderInputNameIndex::ValidateOrFindSamplerIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        if (IsValid())      // 99% use case, check this first for a quick early out
        {
            return true;
        }
        if (!m_initialized)
        {
            FindSamplerIndex(srgLayout);
            return IsValid();
        }
        return false;
    }

    bool ShaderInputNameIndex::ValidateOrFindConstantIndex(const ShaderResourceGroupLayout* srgLayout)
    {
        if (IsValid())      // 99% use case, check this first for a quick early out
        {
            return true;
        }
        if (!m_initialized)
        {
            FindConstantIndex(srgLayout);
            return IsValid();
        }
        return false;
    }

    // --- Index getters with assertions ---

    u32 ShaderInputNameIndex::GetIndex() const
    {
        AssertValid();
        return m_index.GetIndex();
    }

    template<typename T>
    T ShaderInputNameIndex::GetIndexAs() const
    {
        return static_cast<T>(GetIndex());
    }

    ShaderInputBufferIndex ShaderInputNameIndex::GetBufferIndex() const
    {
        AZ_Assert(m_inputType == IndexType::ShaderBuffer, "ShaderInputNameIndex [%s] being cast as BufferIndex but is not of Buffer type!", m_name.GetCStr());
        return GetIndexAs<ShaderInputBufferIndex>();
    }

    ShaderInputImageIndex ShaderInputNameIndex::GetImageIndex() const
    {
        AZ_Assert(m_inputType == IndexType::ShaderImage, "ShaderInputNameIndex [%s] being cast as ImageIndex but is not of Image type!", m_name.GetCStr());
        return GetIndexAs<ShaderInputImageIndex>();
    }

    ShaderInputSamplerIndex ShaderInputNameIndex::GetSamplerIndex() const
    {
        AZ_Assert(m_inputType == IndexType::ShaderSampler, "ShaderInputNameIndex [%s] being cast as SamplerIndex but is not of Sampler type!", m_name.GetCStr());
        return GetIndexAs<ShaderInputSamplerIndex>();
    }

    ShaderInputConstantIndex ShaderInputNameIndex::GetConstantIndex() const
    {
        AZ_Assert(m_inputType == IndexType::ShaderConstant, "ShaderInputNameIndex [%s] being cast as ConstantIndex but is not of Constant type!", m_name.GetCStr());
        return GetIndexAs<ShaderInputConstantIndex>();
    }

    ShaderInputStaticSamplerIndex ShaderInputNameIndex::GetStaticSamplerIndex() const
    {
        AZ_Assert(m_inputType == IndexType::ShaderSampler, "ShaderInputNameIndex [%s] being cast as SamplerIndex but is not of Sampler type!", m_name.GetCStr());
        return GetIndexAs<ShaderInputStaticSamplerIndex>();
    }

    // --- Reset & Clear ---

    void ShaderInputNameIndex::Reset()
    {
        m_index.Reset();
        m_initialized = false;
        m_inputType = IndexType::InvalidIndex;
    }

    // --- Checks and asserts ---

    bool ShaderInputNameIndex::HasName() const
    {
        return !m_name.IsEmpty();
    }

    void ShaderInputNameIndex::AssertHasName() const
    {
        AZ_Assert(HasName(), "ShaderInputNameIndex does not have a valid Name. Please initialize with a valid Name.");
    }

    bool ShaderInputNameIndex::IsValid() const
    {
        return m_index.IsValid();
    }

    void ShaderInputNameIndex::AssertValid() const
    {
        AZ_Assert(IsValid(), "ShaderInputNameIndex [%s] does not have a valid index. Please initialize with the Shader Resource Group.", m_name.GetCStr());
    }

    bool ShaderInputNameIndex::IsInitialized() const
    {
        return m_initialized;
    }

    void ShaderInputNameIndex::AssetInialized() const
    {
        AZ_Assert(IsInitialized(), "ShaderInputNameIndex [%s] has not been initialized. Please initialize with the Shader Resource Group.", m_name.GetCStr());

    }

    const Name& ShaderInputNameIndex::GetNameForDebug() const
    {
        AZ_Assert(HasName(), "GetNameForDebug() called on ShaderInputNameIndex that doesn't have a name set. Please initialize it with a name.", m_name.GetCStr());
        return m_name;
    }
}
