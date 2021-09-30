/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Shader/Shader.h>

// Hair specific
#include <Rendering/SharedBuffer.h>

namespace AZ
{
    namespace RHI
    {
        class BufferAssetView;
    }

    namespace RPI
    {
        class Buffer;
    }
    
    namespace Data
    {
        /**
        * This is an alias of intrusive_ptr designed for any class which inherits from
        * InstanceData. You're not required to use Instance<> over AZStd::intrusive_ptr<>,
        * but it provides symmetry with Asset<>.
        */
        template <typename T>
        using Instance = AZStd::intrusive_ptr<T>;
    }

    namespace Render
    {
        namespace Hair
        {
            class UtilityClass
            {
            public:
                UtilityClass() = default;

                static Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                    Data::Instance<RPI::Shader> shader,
                    const char* shaderResourceGroupId,
                    const char* moduleName
                );

                static Data::Instance<RPI::Buffer> CreateBuffer(
                    const char* warningHeader,
                    SrgBufferDescriptor& bufferDesc,
                    Data::Instance<RPI::ShaderResourceGroup> srg
                );

                static Data::Instance<RPI::Buffer> CreateBufferAndBindToSrg(
                    const char* warningHeader,
                    SrgBufferDescriptor& bufferDesc,
                    Data::Instance<RPI::ShaderResourceGroup> srg
                );

                static bool BindBufferToSrg(
                    const char* warningHeader,
                    Data::Instance<RPI::Buffer> buffer,
                    SrgBufferDescriptor& bufferDesc,
                    Data::Instance<RPI::ShaderResourceGroup> srg=nullptr
                );

                static Data::Instance<RPI::StreamingImage> LoadStreamingImage(
                    const char* textureFilePath, [[maybe_unused]] const char* sampleName
                );

                static Data::Instance<RHI::ImagePool> CreateImagePool(
                    RHI::ImagePoolDescriptor& imagePoolDesc);

                static Data::Instance<RHI::Image> CreateImage2D(
                    RHI::ImagePool* imagePool, RHI::ImageDescriptor& imageDesc);
            };

            //! The following class matches between a constant buffer structure in CPU and its counter
            //!  part on the GPU. It is the equivalent Atom class for TressFXUniformBuffer.
            template<class TYPE>
            class HairUniformBuffer
            {
            public:
                TYPE* operator->()
                {
                    return &m_cpuBuffer;
                }

                TYPE* get()
                {
                    return &m_cpuBuffer;
                }

                Render::SrgBufferDescriptor& GetBufferDescriptor() { return m_bufferDesc;  }

                //! Use this method only if the buffer will be attached to a single Srg.
                //! If this is not the case use InitForUndefinedSrg
                bool InitForUniqueSrg(
                    Data::Instance<RPI::ShaderResourceGroup> srg,
                    Render::SrgBufferDescriptor& srgDesc)
                {
                    m_srg = srg;
                    m_bufferDesc = srgDesc;

                    AZ_Error("HairUniformBuffer", m_srg, "Critical Error - no Srg was provided to bind buffer to [%s]",
                        m_bufferDesc.m_bufferName.GetCStr());

                    RHI::ShaderInputConstantIndex indexHandle = m_srg->FindShaderInputConstantIndex(m_bufferDesc.m_paramNameInSrg);
                    if (indexHandle.IsNull())
                    {
                        AZ_Error("HairUniformBuffer", false,
                            "Failed to find shader constant buffer index for [%s] in the SRG.",
                            m_bufferDesc.m_paramNameInSrg.GetCStr());
                        return false;
                    }

                    m_bufferDesc.m_resourceShaderIndex = indexHandle.GetIndex();
                    return true;
                }

                //! Updates and binds the data to the Srg and copies it to the GPU side.
                //! Assumes that the buffer is uniquely attached to the saved srg.
                bool UpdateGPUData()
                {
                    if (!m_srg.get())
                    {
                        AZ_Error("HairUniformBuffer", false, "Critical Error - no Srg was provided to bind buffer to [%s]",
                            m_bufferDesc.m_bufferName.GetCStr());
                        return false;
                    }

                    RHI::ShaderInputConstantIndex constantIndex = RHI::ShaderInputConstantIndex(m_bufferDesc.m_resourceShaderIndex);
                    if (constantIndex.IsNull())
                    {
                        AZ_Error("HairUniformBuffer", false, "Critical Error - Srg index is not valide for [%s]",
                            m_bufferDesc.m_paramNameInSrg.GetCStr());
                        return false;
                    }

                    if (!m_srg->SetConstantRaw(constantIndex, &m_cpuBuffer, m_bufferDesc.m_elementSize))
                    {
                        AZ_Error("HairUniformBuffer", false, "Failed to bind Constant Buffer for [%s]", m_bufferDesc.m_bufferName.GetCStr());
                        return false;
                    }
                    return true;
                }

                //! Updates Binds the data to the Srg and copies it to the GPU side.
                //! Assumes that the buffer can be associated with various srgs.
                bool UpdateGPUData(
                    Data::Instance<RPI::ShaderResourceGroup> srg,
                    Render::SrgBufferDescriptor& srgDesc)
                {
                    if (!srg)
                    {
                        AZ_Error("HairUniformBuffer", srg, "Critical Error - no Srg was provided to bind buffer to [%s]",
                            srgDesc.m_bufferName.GetCStr());
                        return false;
                    }

                    RHI::ShaderInputConstantIndex indexHandle = srg->FindShaderInputConstantIndex(srgDesc.m_paramNameInSrg);
                    if (indexHandle.IsNull())
                    {
                        AZ_Error("HairUniformBuffer", false,
                            "Failed to find shader constant buffer index for [%s] in the SRG.",
                            srgDesc.m_paramNameInSrg.GetCStr());
                        return false;
                    }

                    if (!srg->SetConstantRaw(indexHandle, &m_cpuBuffer, srgDesc.m_elementSize))
                    {
                        AZ_Error("HairUniformBuffer", false, "Failed to bind Constant Buffer for [%s]", srgDesc.m_bufferName.GetCStr());
                        return false;
                    }
                    return true;
                }


            private:
                TYPE m_cpuBuffer;

                //! When this srg is set as nullptr, we assume that the buffer can be shared
                //!  between several passes (as done for PerView and PerScene).
                Data::Instance<RPI::ShaderResourceGroup> m_srg = nullptr;

                Render::SrgBufferDescriptor  m_bufferDesc = Render::SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant,
                    RHI::Format::Unknown, sizeof(TYPE), 1,
                    Name{"BufferNameUndefined"}, Name{"BufferNameUndefined"}, 0, 0
                );
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
