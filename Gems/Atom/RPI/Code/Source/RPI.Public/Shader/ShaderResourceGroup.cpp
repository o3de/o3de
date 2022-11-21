/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ShaderDataMappings.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        const char* ShaderResourceGroup::s_traceCategoryName = "ShaderResourceGroup";
        const Data::Instance<Image> ShaderResourceGroup::s_nullImage;
        const Data::Instance<Buffer> ShaderResourceGroup::s_nullBuffer;

        Data::InstanceId ShaderResourceGroup::MakeInstanceId(
            const Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            AZ_Assert(!srgName.IsEmpty(), "Invalid ShaderResourceGroup name");

            // Let's find the srg layout with the given name, because it contains the azsl file path of origin
            // which is essential to uniquely identify an SRG and avoid redundant copies in memory.
            auto srgLayout = shaderAsset->FindShaderResourceGroupLayout(srgName, supervariantIndex);
            AZ_Assert(srgLayout != nullptr, "Failed to find SRG with name %s, using supervariantIndex %u from shaderAsset %s", srgName.GetCStr(),
                supervariantIndex.GetIndex(), shaderAsset.GetHint().c_str());

            AZStd::string idString = AZStd::string::format("%s_%u_%s", srgLayout->GetUniqueId().c_str(), supervariantIndex.GetIndex(), srgName.GetCStr());
            return Data::InstanceId::CreateData(idString.data(), idString.size());
        }

        Data::Instance<ShaderResourceGroup> ShaderResourceGroup::Create(
            const Data::Asset<ShaderAsset>& shaderAsset, const AZ::Name& srgName)
        {
            // retrieve the supervariantIndex by searching for the default supervariant name, this will
            // allow the shader asset to properly handle the RPI::ShaderSystem supervariant
            SupervariantIndex supervariantIndex = shaderAsset->GetSupervariantIndex(AZ::Name(""));

            SrgInitParams initParams{ supervariantIndex, srgName };
            auto anyInitParams = AZStd::any(initParams);
            return Data::InstanceDatabase<ShaderResourceGroup>::Instance().Create(shaderAsset, &anyInitParams);
        }

        Data::Instance<ShaderResourceGroup> ShaderResourceGroup::Create(
            const Data::Asset<ShaderAsset>& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            SrgInitParams initParams{ supervariantIndex, srgName };
            auto anyInitParams = AZStd::any(initParams);
            return Data::InstanceDatabase<ShaderResourceGroup>::Instance().Create(shaderAsset, &anyInitParams);
        }

        Data::Instance<ShaderResourceGroup> ShaderResourceGroup::CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* anySrgInitParams)
        {
            AZ_Assert(anySrgInitParams, "Invalid SrgInitParams");
            auto srgInitParams = AZStd::any_cast<SrgInitParams>(*anySrgInitParams);

            Data::Instance<ShaderResourceGroup> srg = aznew ShaderResourceGroup();
            const RHI::ResultCode resultCode = srg->Init(shaderAsset, srgInitParams.m_supervariantIndex, srgInitParams.m_srgName);
            if (resultCode != RHI::ResultCode::Success)
            {
                return nullptr;
            }

            return srg;
        }

        RHI::ResultCode ShaderResourceGroup::Init(ShaderAsset& shaderAsset, const SupervariantIndex& supervariantIndex, const AZ::Name& srgName)
        {
            const auto& lay = shaderAsset.FindShaderResourceGroupLayout(srgName, supervariantIndex);
            m_layout = lay.get();

            if (!m_layout)
            {
                AZ_Assert(false, "ShaderResourceGroup cannot be initialized due to invalid ShaderResourceGroupLayout");
                return RHI::ResultCode::Fail;
            }

            m_pool = ShaderResourceGroupPool::FindOrCreate(
                AZ::Data::Asset<ShaderAsset>(&shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad), supervariantIndex, srgName);
            if (!m_pool)
            {
                return RHI::ResultCode::Fail;
            }

            m_shaderResourceGroup = m_pool->CreateRHIShaderResourceGroup();
            if (!m_shaderResourceGroup)
            {
                return RHI::ResultCode::Fail;
            }
            m_shaderResourceGroup->SetName(m_pool->GetRHIPool()->GetName());
            m_data = RHI::ShaderResourceGroupData(m_layout);
            m_asset = { &shaderAsset, AZ::Data::AssetLoadBehavior::PreLoad };

            // The RPI groups match the same dimensions as the RHI group.
            m_imageGroup.resize(m_layout->GetGroupSizeForImages());
            m_bufferGroup.resize(m_layout->GetGroupSizeForBuffers());

            m_isInitialized = true;

            return RHI::ResultCode::Success;
        }

        void ShaderResourceGroup::Compile()
        {
            m_shaderResourceGroup->Compile(m_data);

            //Mask is passed to RHI in the Compile call so we can reset it here
            m_data.ResetUpdateMask();
        }

        bool ShaderResourceGroup::IsQueuedForCompile() const
        {
            return m_shaderResourceGroup->IsQueuedForCompile();
        }

        RHI::ShaderInputBufferIndex ShaderResourceGroup::FindShaderInputBufferIndex(const Name& name) const
        {
            return m_layout->FindShaderInputBufferIndex(name);
        }

        RHI::ShaderInputImageIndex ShaderResourceGroup::FindShaderInputImageIndex(const Name& name) const
        {
            return m_layout->FindShaderInputImageIndex(name);
        }

        RHI::ShaderInputSamplerIndex ShaderResourceGroup::FindShaderInputSamplerIndex(const Name& name) const
        {
            return m_layout->FindShaderInputSamplerIndex(name);
        }

        RHI::ShaderInputConstantIndex ShaderResourceGroup::FindShaderInputConstantIndex(const Name& name) const
        {
            return m_layout->FindShaderInputConstantIndex(name);
        }

        RHI::ShaderInputBufferUnboundedArrayIndex ShaderResourceGroup::FindShaderInputBufferUnboundedArrayIndex(const Name& name) const
        {
            return m_layout->FindShaderInputBufferUnboundedArrayIndex(name);
        }

        RHI::ShaderInputImageUnboundedArrayIndex  ShaderResourceGroup::FindShaderInputImageUnboundedArrayIndex(const Name& name) const
        {
            return m_layout->FindShaderInputImageUnboundedArrayIndex(name);
        }

        //const Data::Asset<ShaderAsset>& ShaderResourceGroup::GetAsset() const
        //{
        //    return m_asset;
        //}

        const RHI::ShaderResourceGroupLayout* ShaderResourceGroup::GetLayout() const
        {
            return m_layout;
        }

        RHI::ShaderResourceGroup* ShaderResourceGroup::GetRHIShaderResourceGroup()
        {
            return m_shaderResourceGroup.get();
        }

        bool ShaderResourceGroup::SetShaderVariantKeyFallbackValue(const ShaderVariantKey& shaderKey)
        {
            uint32_t keySize = GetLayout()->GetShaderVariantKeyFallbackSize();
            if (keySize == 0)
            {
                return false;
            }

            auto shaderFallbackIndex = GetLayout()->GetShaderVariantKeyFallbackConstantIndex();
            if (!shaderFallbackIndex.IsValid())
            {
                return false;
            }

            return SetConstantRaw(shaderFallbackIndex, shaderKey.data(), 0, AZStd::min(keySize, (uint32_t) ShaderVariantKeyBitCount) / 8);
        }

        bool ShaderResourceGroup::HasShaderVariantKeyFallbackEntry() const
        {
            return GetLayout()->HasShaderVariantKeyFallbackEntry();
        }

        bool ShaderResourceGroup::SetImage(RHI::ShaderInputNameIndex& inputIndex, const Data::Instance<Image>& image, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImage(inputIndex.GetImageIndex(), image, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImage(RHI::ShaderInputImageIndex inputIndex, const Data::Instance<Image>& image, uint32_t arrayIndex)
        {
            const RHI::ImageView* imageView = image ? image->GetImageView() : nullptr;

            if (m_data.SetImageView(inputIndex, imageView, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Track the RPI image entry at the same slot.
                m_imageGroup[interval.m_min + arrayIndex] = image;

                return true;
            }

            return false;
        }

        bool ShaderResourceGroup::SetImageArray(RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const Data::Instance<Image>> images, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageArray(inputIndex.GetImageIndex(), images, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageArray(RHI::ShaderInputImageIndex inputIndex, AZStd::span<const Data::Instance<Image>> images, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(images.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < images.size(); ++i)
                {
                    isValidAll &= SetImage(inputIndex, images[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        const Data::Instance<Image>& ShaderResourceGroup::GetImage(RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return GetImage(inputIndex.GetImageIndex(), arrayIndex);
            }
            return s_nullImage;
        }

        const Data::Instance<Image>& ShaderResourceGroup::GetImage(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return m_imageGroup[interval.m_min + arrayIndex];
            }
            return s_nullImage;
        }

        AZStd::span<const Data::Instance<Image>> ShaderResourceGroup::GetImageArray(RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return GetImageArray(inputIndex.GetImageIndex());
            }
            return {};
        }

        AZStd::span<const Data::Instance<Image>> ShaderResourceGroup::GetImageArray(RHI::ShaderInputImageIndex inputIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, 0))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return AZStd::span<const Data::Instance<Image>>(&m_imageGroup[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        bool ShaderResourceGroup::SetImageView(RHI::ShaderInputNameIndex& inputIndex, const RHI::ImageView* imageView, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageView(inputIndex.GetImageIndex(), imageView, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageView(RHI::ShaderInputImageIndex inputIndex, const RHI::ImageView* imageView, uint32_t arrayIndex)
        {
            if (m_data.SetImageView(inputIndex, imageView, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Reset the RPI image entry, since an RHI version now takes precedence.
                m_imageGroup[interval.m_min + arrayIndex] = nullptr;

                return true;
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewArray(RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const RHI::ImageView* const> imageViews, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindImageIndex(GetLayout()))
            {
                return SetImageViewArray(inputIndex.GetImageIndex(), imageViews, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewArray(RHI::ShaderInputImageIndex inputIndex, AZStd::span<const RHI::ImageView* const> imageViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(imageViews.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < imageViews.size(); ++i)
                {
                    isValidAll &= SetImageView(inputIndex, imageViews[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroup::SetImageViewUnboundedArray(RHI::ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const RHI::ImageView* const> imageViews)
        {
            return m_data.SetImageViewUnboundedArray(inputIndex, imageViews);
        }

        bool ShaderResourceGroup::SetBufferView(RHI::ShaderInputNameIndex& inputIndex, const RHI::BufferView* bufferView, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferView(inputIndex.GetBufferIndex(), bufferView, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferView(RHI::ShaderInputBufferIndex inputIndex, const RHI::BufferView* bufferView, uint32_t arrayIndex)
        {
            if (m_data.SetBufferView(inputIndex, bufferView, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Reset the RPI image entry, since an RHI version now takes precedence.
                m_bufferGroup[interval.m_min + arrayIndex] = nullptr;

                return true;
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewArray(RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const RHI::BufferView* const> bufferViews, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferViewArray(inputIndex.GetBufferIndex(), bufferViews, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const RHI::BufferView* const> bufferViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(bufferViews.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < bufferViews.size(); ++i)
                {
                    isValidAll &= SetBufferView(inputIndex, bufferViews[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferViewUnboundedArray(RHI::ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const RHI::BufferView* const> bufferViews)
        {
            return m_data.SetBufferViewUnboundedArray(inputIndex, bufferViews);
        }

        bool ShaderResourceGroup::SetSampler(RHI::ShaderInputNameIndex& inputIndex, const RHI::SamplerState& sampler, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindSamplerIndex(GetLayout()))
            {
                return SetSampler(inputIndex.GetSamplerIndex(), sampler, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetSampler(RHI::ShaderInputSamplerIndex inputIndex, const RHI::SamplerState& sampler, uint32_t arrayIndex)
        {
            return m_data.SetSampler(inputIndex, sampler, arrayIndex);
        }

        bool ShaderResourceGroup::SetSamplerArray(RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const RHI::SamplerState> samplers, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindSamplerIndex(GetLayout()))
            {
                return SetSamplerArray(inputIndex.GetSamplerIndex(), samplers, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex, AZStd::span<const RHI::SamplerState> samplers, uint32_t arrayIndex)
        {
            return m_data.SetSamplerArray(inputIndex, samplers, arrayIndex);
        }

        bool ShaderResourceGroup::SetConstantRaw(RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteCount)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantRaw(inputIndex.GetConstantIndex(), bytes, byteCount);
            }
            return false;
        }

        bool ShaderResourceGroup::SetConstantRaw(RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
        {
            return m_data.SetConstantRaw(inputIndex, bytes, byteCount);
        }

        bool ShaderResourceGroup::SetConstantRaw(RHI::ShaderInputNameIndex& inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            if (inputIndex.ValidateOrFindConstantIndex(GetLayout()))
            {
                return SetConstantRaw(inputIndex.GetConstantIndex(), bytes, byteOffset, byteCount);
            }
            return false;
        }

        bool ShaderResourceGroup::SetConstantRaw(RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            return m_data.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        bool ShaderResourceGroup::ApplyDataMappings(const RHI::ShaderDataMappings& mappings)
        {
            bool success = true;

            success = success && ApplyDataMappingArray(mappings.m_colorMappings);
            success = success && ApplyDataMappingArray(mappings.m_uintMappings);
            success = success && ApplyDataMappingArray(mappings.m_floatMappings);
            success = success && ApplyDataMappingArray(mappings.m_float2Mappings);
            success = success && ApplyDataMappingArray(mappings.m_float3Mappings);
            success = success && ApplyDataMappingArray(mappings.m_float4Mappings);
            success = success && ApplyDataMappingArray(mappings.m_matrix3x3Mappings);
            success = success && ApplyDataMappingArray(mappings.m_matrix4x4Mappings);

            return success;
        }

        const RHI::ConstPtr<RHI::ImageView>& ShaderResourceGroup::GetImageView(RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindImageIndex(GetLayout());
            return GetImageView(inputIndex.GetImageIndex(), arrayIndex);
        }

        const RHI::ConstPtr<RHI::ImageView>& ShaderResourceGroup::GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetImageView(inputIndex, arrayIndex);
        }

        AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroup::GetImageViewArray(RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindImageIndex(GetLayout());
            return GetImageViewArray(inputIndex.GetImageIndex());
        }

        AZStd::span<const RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroup::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
        {
            return m_data.GetImageViewArray(inputIndex);
        }

        const RHI::ConstPtr<RHI::BufferView>& ShaderResourceGroup::GetBufferView(RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindBufferIndex(GetLayout());
            return GetBufferView(inputIndex.GetBufferIndex(), arrayIndex);
        }

        const RHI::ConstPtr<RHI::BufferView>& ShaderResourceGroup::GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetBufferView(inputIndex, arrayIndex);
        }

        AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroup::GetBufferViewArray(RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindBufferIndex(GetLayout());
            return GetBufferViewArray(inputIndex.GetBufferIndex());
        }

        AZStd::span<const RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroup::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
        {
            return m_data.GetBufferViewArray(inputIndex);
        }

        bool ShaderResourceGroup::SetBuffer(RHI::ShaderInputNameIndex& inputIndex, const Data::Instance<Buffer>& buffer, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBuffer(inputIndex.GetBufferIndex(), buffer, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBuffer(RHI::ShaderInputBufferIndex inputIndex, const Data::Instance<Buffer>& buffer, uint32_t arrayIndex)
        {
            const RHI::BufferView* bufferView = buffer ? buffer->GetBufferView() : nullptr;

            if (m_data.SetBufferView(inputIndex, bufferView, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);

                // Track the RPI buffer entry at the same slot.
                m_bufferGroup[interval.m_min + arrayIndex] = buffer;

                return true;
            }

            return false;
        }

        bool ShaderResourceGroup::SetBufferArray(RHI::ShaderInputNameIndex& inputIndex, AZStd::span<const Data::Instance<Buffer>> buffers, uint32_t arrayIndex)
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return SetBufferArray(inputIndex.GetBufferIndex(), buffers, arrayIndex);
            }
            return false;
        }

        bool ShaderResourceGroup::SetBufferArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::span<const Data::Instance<Buffer>> buffers, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex + static_cast<uint32_t>(buffers.size()) - 1))
            {
                bool isValidAll = true;
                for (size_t i = 0; i < buffers.size(); ++i)
                {
                    isValidAll &= SetBuffer(inputIndex, buffers[i], static_cast<uint32_t>(arrayIndex + i));
                }
                return isValidAll;
            }
            return false;
        }

        const Data::Instance<Buffer>& ShaderResourceGroup::GetBuffer(RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return GetBuffer(inputIndex.GetBufferIndex(), arrayIndex);
            }
            return s_nullBuffer;
        }

        const Data::Instance<Buffer>& ShaderResourceGroup::GetBuffer(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, arrayIndex))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return m_bufferGroup[interval.m_min + arrayIndex];
            }
            return s_nullBuffer;
        }

        AZStd::span<const Data::Instance<Buffer>> ShaderResourceGroup::GetBufferArray(RHI::ShaderInputNameIndex& inputIndex) const
        {
            if (inputIndex.ValidateOrFindBufferIndex(GetLayout()))
            {
                return GetBufferArray(inputIndex.GetBufferIndex());
            }
            return {};
        }

        AZStd::span<const Data::Instance<Buffer>> ShaderResourceGroup::GetBufferArray(RHI::ShaderInputBufferIndex inputIndex) const
        {
            if (m_layout->ValidateAccess(inputIndex, 0))
            {
                const RHI::Interval interval = m_layout->GetGroupInterval(inputIndex);
                return AZStd::span<const Data::Instance<Buffer>>(&m_bufferGroup[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        void ShaderResourceGroup::ResetViews()
        {
            m_data.ResetViews();
        }

        const RHI::SamplerState& ShaderResourceGroup::GetSampler(RHI::ShaderInputNameIndex& inputIndex, uint32_t arrayIndex) const
        {
            inputIndex.ValidateOrFindSamplerIndex(GetLayout());
            return GetSampler(inputIndex.GetSamplerIndex(), arrayIndex);
        }

        const RHI::SamplerState& ShaderResourceGroup::GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_data.GetSampler(inputIndex, arrayIndex);
        }

        AZStd::span<const RHI::SamplerState> ShaderResourceGroup::GetSamplerArray(RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindSamplerIndex(GetLayout());
            return GetSamplerArray(inputIndex.GetSamplerIndex());
        }

        AZStd::span<const RHI::SamplerState> ShaderResourceGroup::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
        {
            return m_data.GetSamplerArray(inputIndex);
        }

        AZStd::span<const uint8_t> ShaderResourceGroup::GetConstantRaw(RHI::ShaderInputNameIndex& inputIndex) const
        {
            inputIndex.ValidateOrFindConstantIndex(GetLayout());
            return GetConstantRaw(inputIndex.GetConstantIndex());
        }

        AZStd::span<const uint8_t> ShaderResourceGroup::GetConstantRaw(RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstantRaw(inputIndex);
        }

    } // namespace RPI
} // namespace AZ
