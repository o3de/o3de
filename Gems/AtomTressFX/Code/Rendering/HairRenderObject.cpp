/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

#include <Atom/Utils/Utils.h>

// Hair Specific
#include <TressFX/TressFXAsset.h>
#include <TressFX/TressFXSettings.h>

#include <Rendering/HairCommon.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            uint32_t HairRenderObject::s_objectCounter = 0;

            AMD::float4 ToAMDFloat4(const AZ::Vector4& vec4)
            {
                AMD::float4 f4;
                f4.x = vec4.GetX();
                f4.y = vec4.GetY();
                f4.z = vec4.GetZ();
                f4.w = vec4.GetW();
                return f4;
            }

            AMD::float4 ToAMDFloat4(const AZ::Color& color)
            {
                AMD::float4 f4;
                f4.x = color.GetR();
                f4.y = color.GetG();
                f4.z = color.GetB();
                f4.w = color.GetA();
                return f4;
            }

            //!=====================================================================================
            //!
            //!                                 DynamicHairData 
            //!
            //!=====================================================================================
            //! Preparation of the descriptors table of all the dynamic stream buffers within the class.
            //! Do not call this method manually as it is called from CreateAndBindGPUResources.
            void DynamicHairData::PrepareSrgDescriptors(
                AZStd::vector<SrgBufferDescriptor>& descriptorArray,
                int32_t vertexCount, uint32_t strandsCount)
            {
                descriptorArray.resize(uint8_t(HairDynamicBuffersSemantics::NumBufferStreams));

                descriptorArray[uint8_t(HairDynamicBuffersSemantics::Position)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositions"}, Name{"m_hairVertexPositions"}, 0, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::PositionsPrev)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositionsPrev"}, Name{"m_hairVertexPositionsPrev"}, 1, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::PositionsPrevPrev)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositionsPrevPrev"}, Name{"m_hairVertexPositionsPrevPrev"}, 2, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::Tangent)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexTangents"}, Name{"m_hairVertexTangents"}, 3, 0
                );

                // Notice the following Format::Unknown that indicates StructuredBuffer
                // For more info review BufferViewDescriptor.h
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::StrandLevelData)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::Unknown, sizeof(TressFXStrandLevelData), strandsCount,
                    Name{"StrandLevelData"}, Name{"m_strandLevelData"}, 4, 0
                );
            }

            bool DynamicHairData::BindPerObjectSrgForRaster()
            {
                uint8_t streams[2] = {
                    uint8_t(HairDynamicBuffersSemantics::Position),
                    uint8_t(HairDynamicBuffersSemantics::Tangent)
                };
                Name offsetNames[2] = {
                    Name("m_positionBufferOffset"),
                    Name("m_tangentBufferOffset")
                };

                m_readBuffersViews.resize(2);
           
                RHI::Buffer* rhiBuffer = SharedBuffer::Get()->GetBuffer()->GetRHIBuffer();
                for (uint8_t index = 0; index < 2 ; ++index)
                {
                    // Buffer view creation from the shared buffer
                    uint8_t stream = streams[index];
                    SrgBufferDescriptor streamDesc = m_dynamicBuffersDescriptors[stream];

                    streamDesc.m_viewOffsetInBytes = uint32_t(m_dynamicViewAllocators[stream]->GetVirtualAddress().m_ptr);
                    AZ_Assert(streamDesc.m_viewOffsetInBytes % streamDesc.m_elementSize == 0, "Offset of buffer within The SharedBuffer is NOT aligned.");
                    const RHI::BufferViewDescriptor viewDescriptor = SharedBuffer::CreateResourceViewWithDifferentFormat(
                        streamDesc.m_viewOffsetInBytes, streamDesc.m_elementCount, streamDesc.m_elementSize,
                        streamDesc.m_elementFormat, RHI::BufferBindFlags::ShaderRead    // No need for ReadWrite in the raster fill
                    );

                    m_readBuffersViews[index] = rhiBuffer->BuildBufferView(viewDescriptor);

                    // Buffer binding into the raster srg
                    RHI::ShaderInputBufferIndex indexHandle = m_simSrgForRaster->FindShaderInputBufferIndex(streamDesc.m_paramNameInSrg);
                    if (!m_simSrgForRaster->SetBufferView(indexHandle, m_readBuffersViews[index].get()))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind raster buffer view for %s", streamDesc.m_bufferName.GetCStr());
                        return false;
                    }

                    // And now for the offsets (if using offsets rather than BufferView)
                    RHI::ShaderInputConstantIndex indexConstHandle = m_simSrgForRaster->FindShaderInputConstantIndex(offsetNames[index]);
                    if (!m_simSrgForRaster->SetConstant(indexConstHandle, streamDesc.m_viewOffsetInBytes))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind Raster Constant [%s]", offsetNames[index].GetCStr());
                        return false;
                    }
                }

                return true;
            }

            //! Matching between the buffers Srg and its buffers descriptors, this method fills the Srg with
            //!  the views of the buffers to be used by the hair instance.
            //! Do not call this method manually as it is called from CreateAndBindGPUResources.
            bool DynamicHairData::BindPerObjectSrgForCompute()
            {
                //! Get the SRG indices for each input stream and set it in the Srg.
                //! There are two methods to use the shared buffer:
                //! 1. Use a the same buffer with pass sync point and use offset to the data
                //!     structures inside. The problem there is offset overhead + complex conversions
                //! 2. Use buffer views into the original shared buffer and treat them as buffers
                //!     with the desired data type. It seems that Atom still requires single shared buffer
                //!     usage within the shader in order to support the sync point. 
                
                // In Atom the usage of BufferView is what permits the usage of different 'buffers'
                //  allocated from within the originally bound single buffer.
                // This allows us to have a single sync point (barrier) between passes only for this
                //  buffer, while indirectly it is used as multiple buffers used by multiple objects in
                //  this pass. 
                for (uint8_t buffer = 0; buffer < uint8_t(HairDynamicBuffersSemantics::NumBufferStreams); ++buffer)
                {
                    SrgBufferDescriptor& streamDesc = m_dynamicBuffersDescriptors[buffer];
                    RHI::ShaderInputBufferIndex indexHandle = m_simSrgForCompute->FindShaderInputBufferIndex(streamDesc.m_paramNameInSrg);
                    streamDesc.m_resourceShaderIndex = indexHandle.GetIndex();

                    if (!m_simSrgForCompute->SetBufferView(indexHandle, m_dynamicBuffersViews[buffer].get()))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind compute buffer view for %s", streamDesc.m_bufferName.GetCStr());
                        return false;
                    }
                }

                // Setting the specific per object buffer offsets within the global shared buffer
                Name offsetNames[5] = {
                    // Notice: order must match HairDynamicBuffersSemantics order
                    Name("m_positionBufferOffset"),
                    Name("m_positionPrevBufferOffset"),
                    Name("m_positionPrevPrevBufferOffset"),
                    Name("m_tangentBufferOffset"),
                    Name("m_strandLevelDataOffset")
                };

                for (uint8_t buffer=0 ; buffer < uint8_t(HairDynamicBuffersSemantics::NumBufferStreams) ; ++buffer)
                {
                    uint32_t viewOffsetInBytes = uint32_t(m_dynamicViewAllocators[buffer]->GetVirtualAddress().m_ptr);
                    RHI::ShaderInputConstantIndex indexHandle = m_simSrgForCompute->FindShaderInputConstantIndex(offsetNames[buffer]);
                    if (!m_simSrgForCompute->SetConstant(indexHandle, viewOffsetInBytes))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind Compute Constant [%s]", offsetNames[buffer].GetCStr());
                        return false;
                    }
                }

                return true;
            }

            //! Creates the GPU dynamic buffers of a single hair object
            //! Equivalent to TressFXDynamicHairData::CreateGPUResources
            bool DynamicHairData::CreateDynamicGPUResources(
                Data::Instance<RPI::Shader> computeShader,
                Data::Instance<RPI::Shader> rasterShader,
                uint32_t vertexCount, uint32_t strandsCount )
            {
                m_initialized = false;

                AZ_Assert(vertexCount <= std::numeric_limits<uint32_t>().max(), "Hair vertex count exceeds uint32_t size.");

                // Create the dynamic shared buffers Srg.
                m_simSrgForCompute = UtilityClass::CreateShaderResourceGroup(computeShader, "HairDynamicDataSrg", "Hair Gem");
                m_simSrgForRaster = UtilityClass::CreateShaderResourceGroup(rasterShader, "HairDynamicDataSrg", "Hair Gem");
                if (!m_simSrgForCompute || !m_simSrgForRaster)
                {
                    AZ_Error("Hair Gem", false, "Failed to create the Per Object shader resource group [HairDynamicDataSrg]");
                    return false;
                }

                // Buffers preparation and creation.
                // The shared buffer must already be created and initialized at this point.
                PrepareSrgDescriptors(vertexCount, strandsCount);

                m_dynamicBuffersViews.resize(uint8_t(HairDynamicBuffersSemantics::NumBufferStreams));
                m_dynamicViewAllocators.resize(uint8_t(HairDynamicBuffersSemantics::NumBufferStreams));

                RHI::Buffer* rhiBuffer = SharedBuffer::Get()->GetBuffer()->GetRHIBuffer();
                for (int stream=0; stream< uint8_t(HairDynamicBuffersSemantics::NumBufferStreams) ; ++stream)
                {
                    SrgBufferDescriptor& streamDesc = m_dynamicBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)streamDesc.m_elementCount * streamDesc.m_elementSize;
                    m_dynamicViewAllocators[stream] = HairSharedBufferInterface::Get()->Allocate(requiredSize);
                    if (!m_dynamicViewAllocators[stream])
                    {
                        // Allocated memory will be cleared using the underlying allocator system and
                        //  indirectly the garbage collection.
                        // Since the garbage collection is ran with delay of 3 frames due to CPU-GPU
                        //  latency, this might result in over allocation at reset / back from game mode.
                        AZ_Error("Hair Gem", false, "Dynamic Buffer out of memory");
                        return false;
                    }

                    // Create the buffer view into the shared buffer - it will be used as a separate buffer
                    // by the PerObject Srg.
                    streamDesc.m_viewOffsetInBytes = uint32_t(m_dynamicViewAllocators[stream]->GetVirtualAddress().m_ptr);
                    AZ_Assert(streamDesc.m_viewOffsetInBytes % streamDesc.m_elementSize == 0, "Offset of buffer within The SharedBuffer is NOT aligned.");
                    const RHI::BufferViewDescriptor viewDescriptor = SharedBuffer::CreateResourceViewWithDifferentFormat(
                        streamDesc.m_viewOffsetInBytes, streamDesc.m_elementCount, streamDesc.m_elementSize,
                        streamDesc.m_elementFormat, RHI::BufferBindFlags::ShaderReadWrite
                    );

                    m_dynamicBuffersViews[stream] = rhiBuffer->BuildBufferView(viewDescriptor);
                }

                m_initialized = true;
                return true;
            }

            //! Data upload - copy the hair mesh asset data (positions and tangents) into the buffers.
            //! This is equivalent to: TressFXDynamicHairData::UploadGPUData
            bool DynamicHairData::UploadGPUData(
                [[maybe_unused]] const char* name, [[maybe_unused]] void* positions, [[maybe_unused]] void* tangents)
            {
                AZ_Error("Hair Gem", m_initialized, "Attempt to load Hair dynamic data for [%s] without views being properly initilized", name);

                const SrgBufferDescriptor* streamDesc = &m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::Position)];
                uint32_t requiredSize = streamDesc->m_elementSize * streamDesc->m_elementCount;
                Data::Instance<RPI::Buffer> sharedBuffer = HairSharedBufferInterface::Get()->GetBuffer();
                AZ_Error("Hair Gem", sharedBuffer, "Attempt to load Hair dynamic data for [%s] without initialize shared buffer", name);

                bool uploadSuccess = true;
                uploadSuccess &= sharedBuffer->UpdateData(positions, requiredSize,
                    m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::Position)].m_viewOffsetInBytes);
                uploadSuccess &= sharedBuffer->UpdateData(positions, requiredSize,
                    m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::PositionsPrev)].m_viewOffsetInBytes);
                uploadSuccess &= sharedBuffer->UpdateData(positions, requiredSize,
                    m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::PositionsPrevPrev)].m_viewOffsetInBytes);

                streamDesc = &m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::Tangent)];
                requiredSize = streamDesc->m_elementSize * streamDesc->m_elementCount;
                uploadSuccess &= sharedBuffer->UpdateData(tangents, requiredSize,
                    m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::Tangent)].m_viewOffsetInBytes);

                return uploadSuccess;
            }


            //!=====================================================================================
            //!
            //!                  HairRenderObject - Equivalent to TressFXHairObject
            //!
            //!=====================================================================================
            HairRenderObject::~HairRenderObject()
            {
                Release();
            }

            void HairRenderObject::Release()
            {

            }

            void HairRenderObject::PrepareHairGenerationSrgDescriptors(uint32_t vertexCount, uint32_t strandsCount)
            {
                m_hairGenerationDescriptors.resize(uint8_t(HairGenerationBuffersSemantics::NumBufferStreams));
                AZStd::string objectNumber = AZStd::to_string(s_objectCounter);
                // static StructuredBuffers for the various hair strands and bones static data.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::InitialHairPositions)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"InitialHairPositions" + objectNumber }, Name{"m_initialHairPositions"}, 0, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::HairRestLengthSRV)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT, sizeof(float), vertexCount,
                    Name{"HairRestLengthSRV" + objectNumber }, Name{"m_hairRestLengthSRV"}, 1, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::HairStrandType)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_UINT, sizeof(uint32_t), strandsCount,
                    Name{"HairStrandType" + objectNumber }, Name{"m_hairStrandType"}, 2, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::FollowHairRootOffset)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), strandsCount,
                    Name{"FollowHairRootOffset" + objectNumber }, Name{"m_followHairRootOffset"}, 3, 0
                };
                // StructuredBuffer with strandsCount elements specifying hair blend bones and their weight
                // Format set to Format::Unknown to avoid set size by type but follow the specified size.
                // This is specifically required for StructuredBuffers.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::BoneSkinningData)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown, sizeof(AMD::TressFXBoneSkinningData), strandsCount,
                    Name{"BoneSkinningData" + objectNumber }, Name{"m_boneSkinningData"}, 4, 0
                };

                // Constant Buffer.  RHI::Format::Unknown will create is as structured buffer per
                // 'BufferSystemInterface' and the pool type will set it as constant buffer.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::TressFXSimulationConstantBuffer)] = {
                    RPI::CommonBufferPoolType::Constant,
                    RHI::Format::Unknown, sizeof(AMD::TressFXSimulationParams), 1,
                    Name{"TressFXSimConstantBuffer" + objectNumber }, Name{"m_tressfxSimParameters"}, 5, 0
                };
            }

            bool HairRenderObject::CreateAndBindHairGenerationBuffers(uint32_t vertexCount, uint32_t strandsCount)
            {
                PrepareHairGenerationSrgDescriptors(vertexCount, strandsCount);

                m_hairGenerationBuffers.resize(uint8_t(HairGenerationBuffersSemantics::NumBufferStreams));
                for (uint8_t buffer = 0; buffer < uint8_t(HairGenerationBuffersSemantics::NumBufferStreams); ++buffer)
                {
                    SrgBufferDescriptor& bufferDesc = m_hairGenerationDescriptors[buffer];
                    if (buffer == uint8_t(HairGenerationBuffersSemantics::TressFXSimulationConstantBuffer))
                    {
                        if (!m_simCB.InitForUniqueSrg(m_hairGenerationSrg, bufferDesc))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        m_hairGenerationBuffers[buffer] = UtilityClass::CreateBufferAndBindToSrg("Hair Gem", bufferDesc, m_hairGenerationSrg);
                        if (!m_hairGenerationBuffers[buffer])
                        {
                            return false;   // No need for error message as it was done already.
                        }
                    }
                }
                return true;
            }

            //! Updates the buffers data for the hair generation.
            //! Notice: does not update the bone matrices that will be updated every frame.
            bool HairRenderObject::UploadGPUData([[maybe_unused]] const char* name, AMD::TressFXAsset* asset)
            {
                // The following must correlate the order in HairGenerationBuffersSemantics
                void* buffersData[uint8_t(HairGenerationBuffersSemantics::NumBufferStreams)] = {
                    (void*)asset->m_positions.data(),
                    (void*)asset->m_restLengths.data(),
                    (void*)asset->m_strandTypes.data(),
                    (void*)asset->m_followRootOffsets.data(),
                    (void*)asset->m_boneSkinningData.data(),
                    nullptr,    // updated by the HairUniformBuffer class
                };

                // The data update of the constant buffer is NOT done here but via the class update
                for (uint8_t buffer = 0; buffer < uint8_t(HairGenerationBuffersSemantics::NumBufferStreams) ; ++buffer)
                {
                    const SrgBufferDescriptor& streamDesc = m_hairGenerationDescriptors[buffer];
                    uint32_t requiredSize = streamDesc.m_elementSize * streamDesc.m_elementCount;

                    if (buffer == uint8_t(HairGenerationBuffersSemantics::TressFXSimulationConstantBuffer))
                    {
                        if (!m_simCB.UpdateGPUData())
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!m_hairGenerationBuffers[buffer]->UpdateData(buffersData[buffer], requiredSize, 0))
                        {
                            AZ_Error("Hair Gem", false, "[%s] Failed to upload data to GPU buffer [%s]", name, streamDesc.m_bufferName.GetCStr());
                            return false;
                        }
                    }
                }
                return true;
            }

            void HairRenderObject::PrepareRenderSrgDescriptors()
            {
                AZ_Error("Hair Gem", m_hairRenderSrg, "Error - m_hairRenderSrg was not created yet");

                m_hairRenderDescriptors.resize(uint8_t(HairRenderBuffersSemantics::NumBufferStreams));
                AZStd::string objectNumber = AZStd::to_string(s_objectCounter);

                // Rendering constant buffers creation
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::RenderCB)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXRenderParams), 1,
                    Name{ "TressFXRenderConstantBuffer" + objectNumber  }, Name{ "m_tressFXRenderParameters" }, 0, 0
                );

                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandCB)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXStrandParams), 1,
                    Name{ "TressFXStrandConstantBuffer" + objectNumber}, Name{ "m_tressFXStrandParameters" }, 0, 0
                );

                // Albedo texture Srg binding indices
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Invalid, RHI::Format::R32_UINT, sizeof(uint32_t), 1,
                    Name{"HairBaseAlbedo" + objectNumber}, Name{"m_baseAlbedoTexture"}, 0, 0
                );
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputImageIndex(m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)].m_paramNameInSrg).GetIndex();

                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Invalid, RHI::Format::R32_UINT, sizeof(uint32_t), 1,
                    Name{"HairStrandAlbedo" + objectNumber}, Name{"m_strandAlbedoTexture"}, 0, 0
                );
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputImageIndex(m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)].m_paramNameInSrg).GetIndex();

                // Vertices Data creation and bind: vertex thickness and texture coordinates.
                // Vertex thickness
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT, sizeof(float), m_NumTotalVertices,
                    Name{ "HairVertRenderParams" + objectNumber}, Name{ "m_hairThicknessCoeffs" }, 0, 0
                );
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputBufferIndex(m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)].m_paramNameInSrg).GetIndex();

                // Texture coordinates
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32_FLOAT, 2 * sizeof(float), m_NumTotalStrands,
                    Name{"HairTexCoords" + objectNumber}, Name{"m_hairStrandTexCd"}, 0, 0
                );
                m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputBufferIndex(m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)].m_paramNameInSrg).GetIndex();
            }


            //! This is the binding method - not the actual content update that will happen every frame update.
            bool HairRenderObject::BindRenderSrgResources()
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                // Constant buffer structures - the bind and update come together.
                bool bindSuccess = true;

                bindSuccess &= m_renderCB.UpdateGPUData();
                bindSuccess &= m_strandCB.UpdateGPUData();

                // Albedo textures 
                const SrgBufferDescriptor* desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_baseAlbedo))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }
                desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_strandAlbedo))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }

                // Vertex streams: thickness and texture coordinates
                desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)];
                if (!m_hairRenderSrg->SetBufferView(RHI::ShaderInputBufferIndex(desc->m_resourceShaderIndex), m_hairVertexRenderParams->GetBufferView()))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", desc->m_bufferName.GetCStr());
                }
                desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)];
                if (!m_hairRenderSrg->SetBufferView(RHI::ShaderInputBufferIndex(desc->m_resourceShaderIndex), m_hairTexCoords->GetBufferView()))
                {
                    AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", desc->m_bufferName.GetCStr());
                    bindSuccess = false;
                }
                return bindSuccess;
            }


            //! Creation of the render Srg m_hairRenderSrg, followed by creation and binding of the
            //! GPU render resources: vertex thickness, vertex UV, hair albedo maps and two constant buffers.
            bool HairRenderObject::CreateRenderingGPUResources(
                Data::Instance<RPI::Shader> shader, AMD::TressFXAsset& asset, [[maybe_unused]] const char* assetName)
            {
                //-------------------- Render Srg Creation ---------------------
                m_hairRenderSrg = UtilityClass::CreateShaderResourceGroup(shader, "HairRenderingMaterialSrg", "Hair Gem");
                if (!m_hairRenderSrg)
                {
                    AZ_Error("Hair Gem", false,
                        "Failed to create the hair render resource group [m_hairRenderSrg] for model [%s]", assetName );
                    return false;
                }

                //------------------- Resource Descriptors ---------------------
                // Prepare descriptors for the various data creation including Srg index.
                // This method should not bind the descriptors as binding will be done after we
                // update the data (before the pass dispatch)
                PrepareRenderSrgDescriptors();

                //-------------------- Constant Buffers Creation -------------------
                // Remark: the albedo images will not be created here but during asset load.
                // Constant buffer structures
                bool bindSuccess = true;
                bindSuccess &= m_renderCB.InitForUniqueSrg(m_hairRenderSrg,
                    m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::RenderCB)]);
                bindSuccess &= m_strandCB.InitForUniqueSrg(m_hairRenderSrg,
                    m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandCB)]);

                if (!bindSuccess)
                {
                    AZ_Error("Hair Gem", false, "Failed to InitForUniqueSrg hair render for model [%s]", assetName);
                    return false;
                }

                // Vertices Data creation and bind: vertex thickness and texture coordinates.
                m_hairVertexRenderParams = UtilityClass::CreateBuffer("Hair Gem",
                    m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)], nullptr);
                if (!m_hairVertexRenderParams.get())
                {
                    AZ_Error("Hair Gem", false, "Failed to create hair vertex buffer for model [%s]", assetName);
                    return false;
                }

                if (asset.m_strandUV.data())
                {
                    m_hairTexCoords = UtilityClass::CreateBuffer("Hair Gem", m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)], nullptr);
                }

                //------------ Index Buffer  ------------
                m_TotalIndices = asset.GetNumHairTriangleIndices();

                RHI::BufferInitRequest request;
                uint32_t indexBufferSize = m_TotalIndices * sizeof(uint32_t);
                m_indexBuffer = aznew RHI::Buffer;
                request.m_buffer = m_indexBuffer.get();
                request.m_descriptor = RHI::BufferDescriptor{
                    RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::InputAssembly,
                    indexBufferSize
                };
                request.m_initialData = (void*)asset.m_triangleIndices.data();
                
                RHI::Ptr<RHI::BufferPool> bufferPool = RPI::BufferSystemInterface::Get()->GetCommonBufferPool(RPI::CommonBufferPoolType::StaticInputAssembly);
                if (!bufferPool)
                {
                    AZ_Error("Hair Gem", false, "Common buffer pool for index buffer could not be created");
                    return false;
                }

                [[maybe_unused]] RHI::ResultCode result = bufferPool->InitBuffer(request);
                AZ_Error("Hair Gem", result == RHI::ResultCode::Success, "Failed to initialize index buffer - error [%d]", result);

                // create index buffer view
                m_geometryView.SetIndexBufferView(RHI::IndexBufferView(*m_indexBuffer.get(), 0, indexBufferSize, RHI::IndexFormat::Uint32));
 
                return true;
            }


            //! Bind Render Srg (m_hairRenderSrg) resources. No resources data update should be doe here
            //! Notice that this also loads the images and is slower if a new asset is required.
            //! If the image was not changed it should only bind without the retrieve operation.
            bool HairRenderObject::PopulateDrawStrandsBindSet(AMD::TressFXRenderingSettings* pRenderSettings)
            {
                // First, Directly loading from the asset stored in the render settings.
                if (pRenderSettings)
                {
                    if (pRenderSettings->m_baseAlbedoAsset)
                    {
                        pRenderSettings->m_baseAlbedoAsset.BlockUntilLoadComplete();
                        m_baseAlbedo = RPI::StreamingImage::FindOrCreate(pRenderSettings->m_baseAlbedoAsset);
                    }
                    if (pRenderSettings->m_strandAlbedoAsset)
                    {
                        pRenderSettings->m_strandAlbedoAsset.BlockUntilLoadComplete();
                        m_strandAlbedo = RPI::StreamingImage::FindOrCreate(pRenderSettings->m_strandAlbedoAsset);
                    }
                }

                // Fallback using the texture name stored in the render settings. 
                // This method should only called when there is an update in the parameters
                // and / or reload textures only when it is specifically required!
                if (!m_baseAlbedo)
                {
                    AZStd::string baseAlbedoName = "defaultwhite.png.streamingimage";
                    if (pRenderSettings && pRenderSettings->m_BaseAlbedoName != "<none>")
                    {
                        baseAlbedoName = pRenderSettings->m_BaseAlbedoName;
                    }
                    m_baseAlbedo = AZ::RPI::LoadStreamingTexture(baseAlbedoName.c_str());
                }
                if (!m_strandAlbedo)
                {
                    AZStd::string strandAlbedoName = "defaultwhite.png.streamingimage";
                    if (pRenderSettings && pRenderSettings->m_StrandAlbedoName != "<none>")
                    {
                        strandAlbedoName = pRenderSettings->m_StrandAlbedoName;
                    }
                    m_strandAlbedo = AZ::RPI::LoadStreamingTexture(strandAlbedoName.c_str());
                }

                // Bind the Srg resources
                return BindRenderSrgResources();
            }

            bool HairRenderObject::LoadImageAsset(AMD::TressFXRenderingSettings* pRenderSettings)
            {
                Data::Instance<RPI::StreamingImage> baseAlbedo = RPI::StreamingImage::FindOrCreate(pRenderSettings->m_baseAlbedoAsset);
                Data::Instance<RPI::StreamingImage> strandAlbedo = RPI::StreamingImage::FindOrCreate(pRenderSettings->m_strandAlbedoAsset);

                // Protect Update and Render if on async threads.
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                // Set albedo textures on shader resources.
                m_baseAlbedo = baseAlbedo;
                m_strandAlbedo = strandAlbedo;

                bool success = true;
                const SrgBufferDescriptor* desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_baseAlbedo))
                {
                    success = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }
                desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_strandAlbedo))
                {
                    success = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }
                return success;
            }

            bool HairRenderObject::UploadRenderingGPUResources(AMD::TressFXAsset& asset)
            {
                bool updateSuccess = true;

                // When the CBs data is changed, this is updating the CPU memory - it will be reflected to the GPU
                // only after binding and compiling stage in the pass.
                updateSuccess &= m_renderCB.UpdateGPUData();
                updateSuccess &= m_strandCB.UpdateGPUData();

                // This should be called once on creation and separate method should apply the CBs update.
                // Vertex streams data update
                if (asset.m_strandUV.data())
                {
                    const SrgBufferDescriptor* desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)];
                    updateSuccess &= m_hairTexCoords->UpdateData( (void*)asset.m_strandUV.data(), desc->m_elementCount * desc->m_elementSize, 0);
                }

                const SrgBufferDescriptor* desc = &m_hairRenderDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)];
                updateSuccess &= m_hairVertexRenderParams->UpdateData((void*)asset.m_thicknessCoeffs.data(), desc->m_elementCount * desc->m_elementSize, 0);

                // No need to update index buffer data unless we go to dynamic reduction
                return updateSuccess;
            }

            //!=====================================================================================
            //!
            //!                                    Update Methods
            //!
            //!-------------------------------------------------------------------------------------
            
            // [To Do] Possibly move wind settings to Simulation Parameters or alike.

            // Wind is in a pyramid around the main wind direction.
            // To add a random appearance, the shader will sample some direction
            // within this cone based on the strand index.
            // This function computes the vector for each edge of the pyramid.
            // [To Do] Requires more testing
            static void SetWindCorner(
                Quaternion rotFromXAxisToWindDir, Vector3 rotAxis,
                float angleToWideWindCone, float windMagnitude, AMD::float4& outVec)
            {
                static const Vector3 XAxis(1.0f, 0, 0);
                Quaternion rot(rotAxis, angleToWideWindCone);
                // original code: Vector3 newWindDir = rotFromXAxisToWindDir * rot * XAxis;
                Vector3 newWindDir = (rotFromXAxisToWindDir * rot).TransformVector(XAxis);
                outVec.x = newWindDir.GetX() * windMagnitude;
                outVec.y = newWindDir.GetY() * windMagnitude;
                outVec.z = newWindDir.GetZ() * windMagnitude;
                outVec.w = 0;  // unused.
            }

            void HairRenderObject::SetWind(const Vector3& windDir, float windMag, int frame)
            {
                // Based on the original AMD code for pleasing wind rate simulation.
                // [To Do] - production can add user control over velocity if required.
                float windMagnitude = windMag * (pow(sin(frame * 0.01f), 2.0f) + 0.5f);

                Vector3 windDirN(windDir);
                windDirN.Normalize();

                Vector3 XAxis(1.0f, 0, 0);
                Vector3 xCrossW = XAxis.Cross(windDirN);

                // Adi: this part is now using AZ::Quaternion - verify correctness
                Quaternion rotFromXAxisToWindDir;
                rotFromXAxisToWindDir.CreateIdentity();

                float angle = asin(xCrossW.GetLength());

                if (angle > 0.001)
                {
                    // original code: rotFromXAxisToWindDir.SetRotation(xCrossW.Normalize(), angle);
                    rotFromXAxisToWindDir.CreateFromVector3AndValue(xCrossW.GetNormalized(), angle);
                }

                const float angleToWideWindCone = AZ::DegToRad(40.f);

                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 1.0, 0),  angleToWideWindCone, windMagnitude, m_simCB->m_Wind);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, -1.0, 0), angleToWideWindCone, windMagnitude, m_simCB->m_Wind1);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 0, 1.0),  angleToWideWindCone, windMagnitude, m_simCB->m_Wind2);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 0, -1.0), angleToWideWindCone, windMagnitude, m_simCB->m_Wind3);
                // fourth component unused. (used to store frame number, but no longer used).
            }

            void HairRenderObject::InitBoneMatricesPlaceHolder(int numBoneMatrices)
            {
                float identityValues[16] = {
                    1, 0, 0, 0,
                    0, 1, 0, 0,
                    0, 0, 1, 0,
                    0, 0, 0, 1
                };
                int numMatrices = AZStd::min(numBoneMatrices, AMD_TRESSFX_MAX_NUM_BONES);
                for (int i = 0; i < numMatrices; ++i)
                {
                    memcpy(m_simCB->m_BoneSkinningMatrix[i].m, identityValues, 16 * sizeof(float));
                }
            }

            //! Updating the bone matrices in the simulation constant buffer.
            void HairRenderObject::UpdateBoneMatrices(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices)
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                int numMatrices = AZStd::min(numBoneMatrices, AMD_TRESSFX_MAX_NUM_BONES);
                for (int i = 0; i < numMatrices; ++i)
                {
                    m_simCB->m_BoneSkinningMatrix[i] = pBoneMatricesInWS[i];
                }
            }

            void HairRenderObject::UpdateBoneMatrices(
                const AZ::Matrix3x4& entityWorldMatrix,
                const AZStd::vector<AZ::Matrix3x4>& boneMatrices
            )
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                const int numMatrices = AZStd::min(static_cast<int>(boneMatrices.size()), AMD_TRESSFX_MAX_NUM_BONES);
                for (int i = 0; i < numMatrices; ++i)
                {
                    AZ::Matrix3x4 boneMatrixWS = entityWorldMatrix * boneMatrices[i];

                    // [rhhong-TODO] - switch to AZ::Matrix4x4 later.
                    m_simCB->m_BoneSkinningMatrix[i].m[0]  = boneMatrixWS(0, 0);
                    m_simCB->m_BoneSkinningMatrix[i].m[1]  = boneMatrixWS(1, 0);
                    m_simCB->m_BoneSkinningMatrix[i].m[2]  = boneMatrixWS(2, 0);
                    m_simCB->m_BoneSkinningMatrix[i].m[3]  = 0.0f;
                    m_simCB->m_BoneSkinningMatrix[i].m[4]  = boneMatrixWS(0, 1);
                    m_simCB->m_BoneSkinningMatrix[i].m[5]  = boneMatrixWS(1, 1);
                    m_simCB->m_BoneSkinningMatrix[i].m[6]  = boneMatrixWS(2, 1);
                    m_simCB->m_BoneSkinningMatrix[i].m[7]  = 0.0f;
                    m_simCB->m_BoneSkinningMatrix[i].m[8]  = boneMatrixWS(0, 2);
                    m_simCB->m_BoneSkinningMatrix[i].m[9]  = boneMatrixWS(1, 2);
                    m_simCB->m_BoneSkinningMatrix[i].m[10] = boneMatrixWS(2, 2);
                    m_simCB->m_BoneSkinningMatrix[i].m[11] = 0.0f;
                    m_simCB->m_BoneSkinningMatrix[i].m[12] = boneMatrixWS(0, 3);
                    m_simCB->m_BoneSkinningMatrix[i].m[13] = boneMatrixWS(1, 3);
                    m_simCB->m_BoneSkinningMatrix[i].m[14] = boneMatrixWS(2, 3);
                    m_simCB->m_BoneSkinningMatrix[i].m[15] = 1.0f;
                }
            }

            // [To Do] Change only parameters that require per frame update such as time.
            // The rest should be changed only when changed (per editor or script)
            //! Update of simulation constant buffer.
            //! The bone matrices are set elsewhere and should be updated before GPU submit.
            void HairRenderObject::UpdateSimulationParameters(const AMD::TressFXSimulationSettings* settings, float timeStep)
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                m_simCB->SetVelocityShockPropogation(settings->m_vspCoeff);
                m_simCB->SetVSPAccelThreshold(settings->m_vspAccelThreshold);
                m_simCB->SetDamping(settings->m_damping);
                m_simCB->SetLocalStiffness(settings->m_localConstraintStiffness);
                m_simCB->SetGlobalStiffness(settings->m_globalConstraintStiffness);
                m_simCB->SetGlobalRange(settings->m_globalConstraintsRange);
                m_simCB->SetGravity(settings->m_gravityMagnitude);
                m_simCB->SetTimeStep(timeStep);
                m_simCB->SetCollision(false);
                m_simCB->SetVerticesPerStrand(m_NumVerticesPerStrand);
                m_simCB->SetFollowHairsPerGuidHair(m_NumFollowHairsPerGuideHair);
                m_simCB->SetTipSeperation(settings->m_tipSeparation);

                // use 1.0 for now, this needs to be maxVelocity * timestep
                m_simCB->g_ClampPositionDelta = 20.0f;

                // Right now, we do all local constraint iterations on the CPU.
                if (m_NumVerticesPerStrand >= TRESSFX_MIN_VERTS_PER_STRAND_FOR_GPU_ITERATION)
                {
                    m_simCB->SetLocalIterations((int)settings->m_localConstraintsIterations);
                    m_CPULocalShapeIterations = 1;
                }
                else
                {
                    m_simCB->SetLocalIterations(1);
                    m_CPULocalShapeIterations = (int)settings->m_localConstraintsIterations;
                }

                m_simCB->SetLengthIterations((int)settings->m_lengthConstraintsIterations);

                // Set wind parameters
                const Vector3& windDir = settings->m_windDirection;
                float windMag = settings->m_windMagnitude;
                SetWind(windDir, windMag, m_SimulationFrame);

#if TRESSFX_COLLISION_CAPSULES
                m_simCB->m_numCollisionCapsules.x = 0;

                // Below is an example showing how to pass capsule collision objects. 
                /*
                mSimCB.m_numCollisionCapsules.x = 1;
                mSimCB.m_centerAndRadius0[0] = { 0, 0.f, 0.f, 50.f };
                mSimCB.m_centerAndRadius1[0] = { 0, 100.f, 0, 10.f };
                */
#endif
                // make sure we start of with a correct pose
                if (m_SimulationFrame < 2)
                    ResetPositions();
            }

            // [To Do] - Change only parameters that require per frame update such as distance.
            // The rest should be changed only when changed (per editor or script)
            void HairRenderObject::UpdateRenderingParameters(
                const AMD::TressFXRenderingSettings* parameters, const int nodePoolSize,
                float distance, bool shadowUpdate /*= false*/)
            {
                if (!parameters)
                {
                    parameters = m_renderSettings;
                }

                // Update Render Parameters
                // If you alter FiberRadius make sure to change it also in the material properties
                // passed by the Feature Processor for the shading. 
                m_renderCB->FiberRadius = parameters->m_FiberRadius;

                m_renderCB->ShadowAlpha = parameters->m_HairShadowAlpha;
                m_renderCB->FiberSpacing = parameters->m_HairFiberSpacing;

                // original TressFX lighting parameters - two specular lobes approximating
                // the Marschner R and and TRT lobes + diffuse component. 
                m_renderCB->MatKValue = { {{0.f, parameters->m_HairKDiffuse, parameters->m_HairKSpec1, parameters->m_HairSpecExp1}} };
                m_renderCB->HairKs2 = parameters->m_HairKSpec2;
                m_renderCB->HairEx2 = parameters->m_HairSpecExp2;

                m_renderCB->CuticleTilt = parameters->m_HairCuticleTilt;
                m_renderCB->Roughness = parameters->m_HairRoughness;

                m_renderCB->MaxShadowFibers = parameters->m_HairMaxShadowFibers;

                // Update Strand Parameters (per hair object)
                m_strandCB->MatBaseColor = ToAMDFloat4(parameters->m_HairMatBaseColor);
                m_strandCB->MatTipColor = ToAMDFloat4(parameters->m_HairMatTipColor);
                m_strandCB->TipPercentage = parameters->m_TipPercentage;
                m_strandCB->StrandUVTilingFactor = parameters->m_StrandUVTilingFactor;
                m_strandCB->FiberRatio = parameters->m_FiberRatio;
                m_strandCB->EnableThinTip = parameters->m_EnableThinTip;
                m_strandCB->EnableStrandUV = parameters->m_EnableStrandUV;

                // Reset LOD hair density for the frame
                m_LODHairDensity = 1.f;
                float fiberRadius = parameters->m_FiberRadius;

                if (parameters->m_EnableHairLOD)
                {
                    float MinLODDist = shadowUpdate ?
                        AZStd::min(parameters->m_ShadowLODStartDistance, parameters->m_ShadowLODEndDistance) :
                        AZStd::min(parameters->m_LODStartDistance, parameters->m_LODEndDistance);
                    float MaxLODDist = shadowUpdate ?
                        AZStd::max(parameters->m_ShadowLODStartDistance, parameters->m_ShadowLODEndDistance) :
                        AZStd::max(parameters->m_LODStartDistance, parameters->m_LODEndDistance);

                    if (distance > MinLODDist)
                    {
                        float DistanceRatio = AZStd::min((distance - MinLODDist) / AZStd::max(MaxLODDist - MinLODDist, 0.00001f), 1.f);

                        // Lerp: x + s(y-x)
                        float MaxLODFiberRadius = fiberRadius * (shadowUpdate ? parameters->m_ShadowLODWidthMultiplier : parameters->m_LODWidthMultiplier);
                        fiberRadius = fiberRadius + (DistanceRatio * (MaxLODFiberRadius - fiberRadius));

                        // Lerp: x + s(y-x)
                        m_LODHairDensity = 1.f + (DistanceRatio * ((shadowUpdate ? parameters->m_ShadowLODPercent : parameters->m_LODPercent) - 1.f));
                    }
                }

                m_strandCB->FiberRadius = fiberRadius;
                m_strandCB->NumVerticesPerStrand = m_NumVerticesPerStrand;  // Constant through the run per object
                m_strandCB->NodePoolSize = nodePoolSize;
                m_strandCB->RenderParamsIndex = m_RenderIndex;      // Per Objects specific according to its index in the FP
            }

            //!=====================================================================================
            //!
            //!                     Generic non tressFX Specific Methods
            //!
            //!-------------------------------------------------------------------------------------
            //! This is the equivalent method to the TressFXObject constructor.
            //! It prepares all dynamic and static buffers and load the data into them, then
            //!  creates all the Srgs associated with the buffers and the remaining structures
            //!  that drive skinning, simulation and rendering of the hair.
            //!---------------------------------------------------------------------
            bool HairRenderObject::Init(
                HairFeatureProcessor* featureProcessor,
                const char* assetName, AMD::TressFXAsset* asset,
                AMD::TressFXSimulationSettings* simSettings, AMD::TressFXRenderingSettings* renderSettings)
            {
                AZ_PROFILE_FUNCTION(AzRender);

                ++s_objectCounter;

                AZ_Error("Hair Gem", featureProcessor, "Feature processor initialized as null hence preventing proper creation of hair");
                m_featureProcessor = featureProcessor;

                m_NumTotalVertices = asset->m_numTotalVertices;
                m_NumTotalStrands = asset->m_numTotalStrands;
                m_numGuideVertices = asset->m_numGuideVertices;
                m_NumVerticesPerStrand = asset->m_numVerticesPerStrand;
                m_NumFollowHairsPerGuideHair = asset->m_numFollowStrandsPerGuide;

                 // dummy method - replace with the bone matrices at init pose and the real bones amount
                InitBoneMatricesPlaceHolder(AMD_TRESSFX_MAX_NUM_BONES);

                // First time around, make sure all parameters are properly filled
                const float SIMULATION_TIME_STEP = 0.0166667f;  // 60 fps to start with nominal step
                UpdateSimulationParameters(simSettings, SIMULATION_TIME_STEP); 

                // [To Do] Hair - change to be dynamically calculated
                const float distanceFromCamera = 1.0f; 
                const float updateShadows = false;
                m_renderSettings = renderSettings;
                m_simSettings = simSettings;
                UpdateRenderingParameters(renderSettings, RESERVED_PIXELS_FOR_OIT, distanceFromCamera, updateShadows);

                if (!GetShaders())
                {
                    return false;
                }

                //-------------------------------------
                // Dynamic buffers, data and Srg creation - shared between passes and changed on the GPU
                if (!m_dynamicHairData.CreateDynamicGPUResources(
                    m_skinningShader, m_geometryRasterShader,
                    m_NumTotalVertices, m_NumTotalStrands))
                {
                    AZ_Error("Hair Gem", false, "Hair - Error creating dynamic resources [%s]", assetName );
                    return false;
                }
                m_dynamicHairData.UploadGPUData(assetName, asset->m_positions.data(), asset->m_tangents.data());

                //-------------------------------------
                // Static buffer, data and Srg creation

                m_hairGenerationSrg = UtilityClass::CreateShaderResourceGroup(m_skinningShader, "HairGenerationSrg", "Hair Gem");
                if (!m_hairGenerationSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to create the hair generation resource group [m_hairGenerationSrg]");
                    return false;
                }

                if (!CreateAndBindHairGenerationBuffers(m_NumTotalVertices, m_NumTotalStrands))
                {
                    AZ_Error("Hair Gem", false, "Hair - Error creating static resources for asset [%s]", assetName);
                    return false;
                }

                if (!UploadGPUData(assetName, asset))
                {
                    AZ_Error("Hair Gem", false, "Hair - Error copying hair generation static buffers [%s]", assetName);
                    return false;
                }

                // Set up with defaults.
                ResetPositions();

                // Rendering setup
                bool renderResourcesSuccess;
                renderResourcesSuccess = CreateRenderingGPUResources(m_geometryRasterShader, *asset, assetName);
                renderResourcesSuccess &= PopulateDrawStrandsBindSet(renderSettings);   
                renderResourcesSuccess &= UploadRenderingGPUResources(*asset);

                return renderResourcesSuccess;
            }

            bool HairRenderObject::GetShaders()
            {
                {
                    // The skinning shader is used for generating the shared per Object srg
                    // Unlike per pass Srg that is uniquely bound to its shader, the other srgs can be
                    // used by multiple shaders - for example PerView, PerMaterial and PerScene
                    Data::Instance<HairSkinningComputePass> skinningPass = m_featureProcessor->GetHairSkinningComputegPass();
                    if (!skinningPass.get())
                    {
                        AZ_Error("Hair Gem", false, "Failed to get Skinning Pass.");
                        return false;
                    }

                    m_skinningShader = skinningPass->GetShader();
                    if (!m_skinningShader)
                    {
                        AZ_Error("Hair Gem", false, "Failed to get hair skinning shader from skinning pass");
                        return false;
                    }
                }

                {
                    m_geometryRasterShader = m_featureProcessor->GetGeometryRasterShader();
                    if (!m_geometryRasterShader)
                    {
                        AZ_Error("Hair Gem", false, "Failed to get hair geometry raster shader");
                        return false;
                    }
                }

                return true;
            }

            void HairRenderObject::SetFrameDeltaTime(float deltaTime)
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                m_frameDeltaTime = deltaTime;
                m_simCB->SetTimeStep(deltaTime);
            }

            bool HairRenderObject::Update()
            {
                bool updatedCB;
                {
                    // Protect Update and Render if on async threads
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                    updatedCB = m_simCB.UpdateGPUData();
                    updatedCB &= m_renderCB.UpdateGPUData();
                    updatedCB &= m_strandCB.UpdateGPUData();
                }

                RPI::ShaderResourceGroup* simSrgForCompute = m_dynamicHairData.GetSimSrgForCompute().get();
                RPI::ShaderResourceGroup* simSrgForRaster= m_dynamicHairData.GetSimSrgForRaster().get();
                RPI::ShaderResourceGroup* generationSrg = m_hairGenerationSrg.get();
                RPI::ShaderResourceGroup* renderMaterialSrg = m_hairRenderSrg.get();
                if (!simSrgForCompute || !simSrgForRaster || !generationSrg || !renderMaterialSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to get one of the Hair Object Srgs.");
                    return false;
                }

                // Single compilation per frame
                simSrgForCompute->Compile();
                simSrgForRaster->Compile();
                generationSrg->Compile();
                renderMaterialSrg->Compile();

                IncreaseSimulationFrame();

                return updatedCB;
            }

            bool HairRenderObject::BuildDrawPacket(RPI::Shader* geometryShader, RHI::DrawPacketBuilder::DrawRequest& drawRequest)
            {
                RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};

                uint32_t numPrimsToRender = m_TotalIndices;
                if (m_LODHairDensity < 1.0f)
                {
                    numPrimsToRender /= 3;
                    numPrimsToRender = uint32_t(float(numPrimsToRender) * m_LODHairDensity);

                    // Calculate a new number of Primitives to draw. Keep it aligned to number of primitives per strand (i.e. don't cut strands in half or anything)
                    uint32_t numPrimsPerStrand = (m_NumVerticesPerStrand - 1) * 2;
                    uint32_t remainderPrims = numPrimsToRender % numPrimsPerStrand;

                    numPrimsToRender = (remainderPrims > 0) ? numPrimsToRender + numPrimsPerStrand - remainderPrims : numPrimsToRender;

                    // Force prims to be on (guide hair + its follow hairs boundary... no partial groupings)
                    numPrimsToRender = numPrimsToRender - (numPrimsToRender % (numPrimsPerStrand * (m_NumFollowHairsPerGuideHair + 1)));
                    numPrimsToRender *= 3;
                }

                m_geometryView.SetDrawArguments(RHI::DrawIndexed{ 0, numPrimsToRender, 0 });

                drawPacketBuilder.Begin(nullptr);
                drawPacketBuilder.SetGeometryView(&m_geometryView);

                RPI::ShaderResourceGroup* renderMaterialSrg = m_hairRenderSrg.get();
                RPI::ShaderResourceGroup* simSrg = m_dynamicHairData.GetSimSrgForRaster().get();

                if (!renderMaterialSrg || !simSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to get the hair material Srg for the raster pass.");
                    return false;
                }
                // No need to compile the simSrg since it was compiled already by the Compute pass this frame
                drawPacketBuilder.AddShaderResourceGroup(renderMaterialSrg->GetRHIShaderResourceGroup());
                drawPacketBuilder.AddShaderResourceGroup(simSrg->GetRHIShaderResourceGroup());
                drawPacketBuilder.AddDrawItem(drawRequest);

                auto drawPacket = drawPacketBuilder.End();
                if (!drawPacket)
                {
                    AZ_Error("Hair Gem", false, "Failed to build the hair DrawPacket.");
                    return false;
                }

                // Insert the newly created draw packet to the map based on its shader
                auto iter = m_geometryDrawPackets.find(geometryShader);
                if (iter != m_geometryDrawPackets.end())
                {
                    iter->second = drawPacket;
                }
                else
                {
                    m_geometryDrawPackets[geometryShader] = drawPacket;
                }

                return true;
            }

            const RHI::DrawPacket* HairRenderObject::GetGeometrylDrawPacket(RPI::Shader* geometryShader)
            {
                auto iter = m_geometryDrawPackets.find(geometryShader);
                if (iter == m_geometryDrawPackets.end())
                {
                    return nullptr;
                }
                return iter->second.get();
            }

            const RHI::DispatchItem* HairRenderObject::GetDispatchItem(RPI::Shader* computeShader)
            {
                auto dispatchIter = m_dispatchItems.find(computeShader);
                if (dispatchIter == m_dispatchItems.end())
                {
                    AZ_Error("Hair Gem", false, "GetDispatchItem could not find the dispatch item based on the given shader resource group");
                    return nullptr;
                }
                return dispatchIter->second->GetDispatchItem();
            }

            bool HairRenderObject::BuildDispatchItem(RPI::Shader* computeShader, DispatchLevel dispatchLevel)
            {
                RPI::ShaderResourceGroup* simSrg = m_dynamicHairData.GetSimSrgForCompute().get();
                RPI::ShaderResourceGroup* hairGenerationSrg = m_hairGenerationSrg.get();
                if (!simSrg || !hairGenerationSrg || !computeShader)
                {
                    AZ_Error("Hair Gem", false, "Failed to get Skinning Pass or one of the Srgs.");
                    return false;
                }

                uint32_t elementsAmount =
                    (dispatchLevel == DispatchLevel::DISPATCHLEVEL_VERTEX) ?
                    m_numGuideVertices : 
                    m_NumTotalStrands;

                m_dispatchItems[computeShader] = aznew HairDispatchItem;
                m_dispatchItems[computeShader]->InitSkinningDispatch(
                    computeShader, hairGenerationSrg, simSrg, elementsAmount );

                return true;
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
