/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <AzCore/Debug/EventTrace.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>

#include <Atom/Utils/Utils.h>

// Hair Specific
#include <TressFX/TressFXAsset.h>
#include <TressFX/TressFXSettings.h>

#include <Rendering/HairCommon.h>
#include <Rendering/HairRenderObject.h>
#include <Rendering/HairFeatureProcessor.h>

#pragma optimize("", off)
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
                f4.w = vec4.GetZ();
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
                    RHI::Format::Unknown, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositions"}, Name{"m_hairVertexPositions"}, 0, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::PositionsPrev)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::Unknown, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositionsPrev"}, Name{"m_hairVertexPositionsPrev"}, 1, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::PositionsPrevPrev)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::Unknown, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexPositionsPrevPrev"}, Name{"m_hairVertexPositionsPrevPrev"}, 2, 0
                );
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::Tangent)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::Unknown, sizeof(AZ::Vector4), vertexCount,
                    Name{"HairVertexTangents"}, Name{"m_hairVertexTangents"}, 3, 0
                );
                // Notice the following format - set to Format::Unknown that indicates StructuredBuffer
                // For more info review BufferViewDescriptor.h
                descriptorArray[uint8_t(HairDynamicBuffersSemantics::StrandLevelData)] =
                    SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadWrite,
                    RHI::Format::Unknown, sizeof(TressFXStrandLevelData), strandsCount,
                    Name{"StrandLevelData"}, Name{"m_strandLevelData"}, 4, 0
                );
            }

            //! Matching between the buffers Srg and its buffers descriptors, this method fills the Srg with
            //!  the views of the buffers to be used by the hair instance.
            //! Do not call this method manually as it is called from CreateAndBindGPUResources.
            bool DynamicHairData::BindSrgBufferViewsAndOffsets()
            {
                //! Get the SRG indices for each input stream and set it in the Srg.
                //! Currently the dynamic streams are not used (not clear if RWBuffer is supported properly
                //!  using this way), but the offsets within the global shared buffer is used instead.
                
                //  Skip this for now - currently it doesn't seem to be a valid choice with Atom
                for (uint8_t buffer = 0; buffer < uint8_t(HairDynamicBuffersSemantics::NumBufferStreams); ++buffer)
                {
                    SrgBufferDescriptor& streamDesc = m_dynamicBuffersDescriptors[buffer];
                    RHI::ShaderInputBufferIndex indexHandle = m_simSrg->FindShaderInputBufferIndex(streamDesc.m_paramNameInSrg);
                    streamDesc.m_resourceShaderIndex = indexHandle.GetIndex();

                    if (!m_simSrg->SetBufferView(indexHandle, m_dynamicBuffersViews[buffer].get()))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind buffer view for %s", streamDesc.m_bufferName.GetCStr());
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
                    RHI::ShaderInputConstantIndex indexHandle = m_simSrg->FindShaderInputConstantIndex(offsetNames[buffer]);
                    if (!m_simSrg->SetConstant(indexHandle, viewOffsetInBytes))
                    {
                        AZ_Error("Hair Gem", false, "Failed to bind Constant [%s]", offsetNames[buffer].GetCStr());
                        return false;
                    }
                }

                return true;
            }

            //! Creates the GPU dynamic buffers of a single hair object
            //! Equivalent to TressFXDynamicHairData::CreateGPUResources
            bool DynamicHairData::CreateAndBindGPUResources(
                Data::Instance<RPI::Shader> shader, uint32_t vertexCount, uint32_t strandsCount )
            {
                AZ_Assert(vertexCount <= std::numeric_limits<uint32_t>().max(), "Hair vertex count exceeds uint32_t size.");

                // Create the dynamic shared buffers Srg .
                // Notice that Atom Hair will be using a single Srg instead of the three used by TressFX
                // to represent Sim, SDF collision and Render since the one we use (equivalent to the
                // TressFX sim one) is the superset of the other two that can simply use it.
                m_simSrg = UtilityClass::CreateShaderResourceGroup(shader, "HairDynamicDataSrg", "Hair Gem");
                if (!m_simSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to create the hair shared simulation shader resource group [m_dynamicBuffersSrg]");
                    return false;
                }

                // Buffers preparation and creation.
                // The shared buffer must already be created and initialized at this point.
                PrepareSrgDescriptors(vertexCount, strandsCount );

                m_dynamicBuffersViews.resize(uint8_t(HairDynamicBuffersSemantics::NumBufferStreams));
                m_dynamicViewAllocators.resize(uint8_t(HairDynamicBuffersSemantics::NumBufferStreams));

                for (int stream=0; stream< uint8_t(HairDynamicBuffersSemantics::NumBufferStreams) ; ++stream)
                {
                    SrgBufferDescriptor& streamDesc = m_dynamicBuffersDescriptors[stream];
                    size_t requiredSize = (uint64_t)streamDesc.m_elementCount * streamDesc.m_elementSize;
                    m_dynamicViewAllocators[stream] = SharedBufferInterface::Get()->Allocate(requiredSize);
                    if (!m_dynamicViewAllocators[stream])
                    {
                        // [To Do] - remove the already allocated memory!  It will be released on shut down but
                        // in the meantime it takes space on the shared buffer.
                        AZ_Error("Hair Gem", false, "Dynamic Buffer out of memory");
                        return false;
                    }

                  /*
//                    [To Do] Adi: This should be put back in use if dynamic buffers are to be used.
//                    Currently indications are that this approach cannot work in Atom or proper care
//                    should be to match the buffer type - not clear at this point but it doesn't work.
//                    Due to that I am employing general shared buffer with offsets into the locations of
//                    each desired stream and per object
*/
                    // Create the buffer view into the shared buffer and store it. It will be used by the shader Srg.
                    streamDesc.m_viewOffsetInBytes = uint32_t(m_dynamicViewAllocators[stream]->GetVirtualAddress().m_ptr);
                    AZ_Assert(streamDesc.m_viewOffsetInBytes % streamDesc.m_elementSize == 0, "Offset of buffer within The SharedBuffer is NOT aligned.");
                    const RHI::BufferViewDescriptor viewDescriptor = SharedBuffer::CreateResourceViewWithDifferentFormat(
                        streamDesc.m_viewOffsetInBytes, streamDesc.m_elementCount, streamDesc.m_elementSize,
                        streamDesc.m_elementFormat, RHI::BufferBindFlags::ShaderReadWrite
                    );
                    RHI::Buffer* rhiBuffer = SharedBuffer::Get()->GetBuffer()->GetRHIBuffer();
                    m_dynamicBuffersViews[stream] = RHI::Factory::Get().CreateBufferView();
                    RHI::ResultCode resultCode = m_dynamicBuffersViews[stream]->Init(*rhiBuffer, viewDescriptor);
                    if (resultCode != RHI::ResultCode::Success)
                    {
                        AZ_Error("Hair Gem", false, "Dynamic BufferView could not be retrieved for [%s]", streamDesc.m_bufferName.GetCStr());
                        return false;
                    }
                } 
                m_initialized = BindSrgBufferViewsAndOffsets();

                return m_initialized;
            }

            //! Data upload - copy the hair mesh asset data (positions and tangents) into the buffers.
            //! In the following line I assume that positions and tangents are of the same size.
            //! This is equivalent to: TressFXDynamicHairData::UploadGPUData
            bool DynamicHairData::UploadGPUData(
                [[maybe_unused]] const char* name, [[maybe_unused]] void* positions, [[maybe_unused]] void* tangents)
            {
                AZ_Error("Hair Gem", m_initialized, "Attempt to load Hair dynamic data for [%s] without views being properly initilized", name);

                SrgBufferDescriptor* streamDesc = &m_dynamicBuffersDescriptors[uint8_t(HairDynamicBuffersSemantics::Position)];
                uint32_t requiredSize = streamDesc->m_elementSize * streamDesc->m_elementCount;
                Data::Instance<RPI::Buffer> sharedBuffer = SharedBufferInterface::Get()->GetBuffer();
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

                // static StructuredBuffers for the various hair strands and bones static data.
                // [To Do] Adi: verify that when setting RHI::Format we can use StructuredBuffer on the shader side.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::InitialHairPositions)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), vertexCount,
                    Name{"InitialHairPositions" + s_objectCounter }, Name{"m_initialHairPositions"}, 0, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::HairRestLengthSRV)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT, sizeof(float), vertexCount,
                    Name{"HairRestLengthSRV" + s_objectCounter }, Name{"m_hairRestLengthSRV"}, 1, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::HairStrandType)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_UINT, sizeof(uint32_t), strandsCount,
                    Name{"HairStrandType" + s_objectCounter }, Name{"m_hairStrandType"}, 2, 0
                };
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::FollowHairRootOffset)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32B32A32_FLOAT, sizeof(AZ::Vector4), strandsCount,
                    Name{"FollowHairRootOffset" + s_objectCounter }, Name{"m_followHairRootOffset"}, 3, 0
                };
                // StructuredBuffer with strandsCount elements specifying hair blend bones and their weight
                // Format set to Format::Unknown to avoid set size by type but follow the specified size.
                // This is specifically required fro StructuredBuffers.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::BoneSkinningData)] = {
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown, sizeof(AMD::TressFXBoneSkinningData), strandsCount,
                    Name{"BoneSkinningData" + s_objectCounter }, Name{"m_boneSkinningData"}, 4, 0
                };

                // Constant Buffer.  RHI::Format::Unknown will create is as structured buffer per
                // 'BufferSystemInterface' and the pool type will set it as constant buffer.
                m_hairGenerationDescriptors[uint8_t(HairGenerationBuffersSemantics::TressFXSimulationConstantBuffer)] = {
                    RPI::CommonBufferPoolType::Constant,
                    RHI::Format::Unknown, sizeof(AMD::TressFXSimulationParams), 1,
                    Name{"TressFXSimConstantBuffer" + s_objectCounter }, Name{"m_tressfxSimParameters"}, 5, 0
                };
            }

            // Based on SkinnedMeshInputLod::CreateStaticBuffer
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
            bool HairRenderObject::UploadGPUData(const char* name, AMD::TressFXAsset* asset)
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

                // [To Do] Adi: notice that the data update of the constant buffer is NOT done here
                for (uint8_t buffer = 0; buffer < uint8_t(HairGenerationBuffersSemantics::NumBufferStreams) ; ++buffer)
                {
                    SrgBufferDescriptor& streamDesc = m_hairGenerationDescriptors[buffer];
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

                m_hairRenerDescriptors.resize(uint8_t(HairRenderBuffersSemantics::NumBufferStreams));

                // Rendering constant buffers creation
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::RenderCB)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXRenderParams), 1,
                    Name{ "TressFXRenderConstantBuffer" + s_objectCounter  }, Name{ "m_tressFXRenderParameters" }, 0, 0
                );

                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandCB)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Constant, RHI::Format::Unknown,
                    sizeof(AMD::TressFXStrandParams), 1,
                    Name{ "TressFXStrandConstantBuffer" + s_objectCounter}, Name{ "m_tressFXStrandParameters" }, 0, 0
                );

                // Albedo texture Srg binding indices
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Invalid, RHI::Format::R32_UINT, sizeof(uint32_t), 1,
                    Name{"HairBaseAlbedo" + s_objectCounter}, Name{"m_baseAlbedoTexture"}, 0, 0
                );
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputImageIndex(m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)].m_paramNameInSrg).GetIndex();

                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::Invalid, RHI::Format::R32_UINT, sizeof(uint32_t), 1,
                    Name{"HairStrandAlbedo" + s_objectCounter}, Name{"m_strandAlbedoTexture"}, 0, 0
                );
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputImageIndex(m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)].m_paramNameInSrg).GetIndex();

                // Vertices Data creation and bind: vertex thickness and texture coordinates.
                // Vertex thickness
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32_FLOAT, sizeof(float), m_NumTotalVertices,
                    Name{ "HairVertRenderParams" + s_objectCounter}, Name{ "m_hairThicknessCoeffs" }, 0, 0
                );
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputBufferIndex(m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)].m_paramNameInSrg).GetIndex();

                // Texture coordinates
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)] = SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::R32G32_FLOAT, 2.0 * sizeof(float), m_NumTotalStrands,
                    Name{"HairTexCoords" + s_objectCounter}, Name{"m_hairStrandTexCd"}, 0, 0
                );
                m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)].m_resourceShaderIndex =
                    m_hairRenderSrg->FindShaderInputBufferIndex(m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)].m_paramNameInSrg).GetIndex();
            }


            //! This is the binding method - not the data set that will happen every frame update.
            bool HairRenderObject::BindRenderSrgResources()
            {
                // Protect Update and Render if on async threads
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                // Constant buffer structures - the bind and update come together.
                bool bindSuccess = true;

                bindSuccess &= m_renderCB.UpdateGPUData();
                bindSuccess &= m_strandCB.UpdateGPUData();

                // Albedo textures 
                SrgBufferDescriptor* desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::BaseAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_baseAlbedo))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }
                desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandAlbedo)];
                if (!m_hairRenderSrg->SetImage(RHI::ShaderInputImageIndex(desc->m_resourceShaderIndex), m_strandAlbedo))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind SRG image for [%s]", desc->m_paramNameInSrg.GetCStr());
                }

                // Vertex streams: thickness and texture coordinates
                desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)];
                if (!m_hairRenderSrg->SetBufferView(RHI::ShaderInputBufferIndex(desc->m_resourceShaderIndex), m_hairVertexRenderParams->GetBufferView()))
                {
                    bindSuccess = false;
                    AZ_Error("Hair Gem", false, "Failed to bind buffer view for [%s]", desc->m_bufferName.GetCStr());
                }
                desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)];
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
                Data::Instance<RPI::Shader> shader, AMD::TressFXAsset& asset, const char* assetName)
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
                    m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::RenderCB)]);
                bindSuccess &= m_strandCB.InitForUniqueSrg(m_hairRenderSrg,
                    m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::StrandCB)]);

                if (!bindSuccess)
                {
                    AZ_Error("Hair Gem", false, "Failed to InitForUniqueSrg hair render for model [%s]", assetName);
                    return false;
                }

                // Vertices Data creation and bind: vertex thickness and texture coordinates.
                m_hairVertexRenderParams = UtilityClass::CreateBuffer("Hair Gem",
                    m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)], nullptr);
                if (!m_hairVertexRenderParams.get())
                {
                    AZ_Error("Hair Gem", false, "Failed to create hair vertex buffer for model [%s]", assetName);
                    return false;
                }

                if (asset.m_strandUV.data())
                {
                    m_hairTexCoords = UtilityClass::CreateBuffer("Hair Gem", m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)], nullptr);
                }

                //------------ Index Buffer  ------------
                m_TotalIndices = asset.GetNumHairTriangleIndices();

                RHI::BufferInitRequest request;
                uint32_t indexBufferSize = m_TotalIndices * sizeof(uint32_t);
                m_indexBuffer = RHI::Factory::Get().CreateBuffer();
                request.m_buffer = m_indexBuffer.get();
                request.m_descriptor = RHI::BufferDescriptor{ RHI::BufferBindFlags::InputAssembly, indexBufferSize };
                request.m_initialData = (void*)asset.m_triangleIndices.data();
                
                RHI::Ptr<RHI::BufferPool> bufferPool = RPI::BufferSystemInterface::Get()->GetCommonBufferPool(RPI::CommonBufferPoolType::StaticInputAssembly);
                if (!bufferPool)
                {
                    AZ_Error("Hair Gem", false, "Common buffer pool for index buffer could not be created");
                    return false;
                }

                RHI::ResultCode result = bufferPool->InitBuffer(request);
                AZ_Error("Hair Gem", result == RHI::ResultCode::Success, "Failed to initialize index buffer - error [%d]", result);

                // create index buffer view
                m_indexBufferView = RHI::IndexBufferView(*m_indexBuffer.get(), 0, indexBufferSize, RHI::IndexFormat::Uint32 );
 
                return true;
            }


            //! Bind Render Srg (m_hairRenderSrg) resources. No resources data update should be doe here
            //! Notice that this also loads the images and is slower if a new asset is required.
            //!     If the image was not changed it should only bind without the retrieve operation.
            bool HairRenderObject::PopulateDrawStrandsBindSet(AMD::TressFXRenderingSettings* pRenderSettings)
            {
                // Load the two albedo maps required for coloring the hair base.
                // [To Do] Adi: make sure this method is only called when there is an update in the parameters
                // and / or reload textures only when it is specifically required!
                AZStd::string baseAlbedoName = "white.tif.streamingimage";
                AZStd::string strandAlbedoName = "white.tif.streamingimage";
                if (pRenderSettings)
                {
                    // Hair base albedo color
                    if (pRenderSettings->m_BaseAlbedoName != "<none>")
                    {
                        baseAlbedoName = pRenderSettings->m_BaseAlbedoName;
                    }

                    // Strand base albedo color
                    if (pRenderSettings->m_StrandAlbedoName != "<none>")
                    {
                        strandAlbedoName = pRenderSettings->m_StrandAlbedoName;
                    }
                    else
                    {
                        strandAlbedoName = baseAlbedoName;
                    }
                }

                // Loading the images from file or from the asset catalog if loaded already.
                // Can be improved by skipping that and only binding.
                m_baseAlbedo = UtilityClass::LoadStreamingImage(baseAlbedoName.c_str(), "Hair Gem");
                m_strandAlbedo = UtilityClass::LoadStreamingImage(strandAlbedoName.c_str(), "Hair Gem");

                // Bind the Srg resources
                return BindRenderSrgResources();
            }


            bool HairRenderObject::UploadRenderingGPUResources(AMD::TressFXAsset& asset)
            {
                bool updateSuccess;
//                AZ_Assert(asset.m_numTotalStrands == m_NumTotalStrands);
//               AZ_Assert(asset.m_numTotalVertices == m_NumTotalVertices);
//                AZ_Assert(m_TotalIndices == asset.GetNumHairTriangleIndices());

                // If the CBs data was changed, it is not being uploaded to the GPU.
                updateSuccess = m_renderCB.UpdateGPUData();
                updateSuccess &= m_strandCB.UpdateGPUData();

                // Adi: this should be done only once and separate method should apply the CBs update.
                // Vertex streams data update
                if (asset.m_strandUV.data())
                {
                    SrgBufferDescriptor* desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairTexCoords)];
                    updateSuccess &= m_hairTexCoords->UpdateData( (void*)asset.m_strandUV.data(), desc->m_elementCount * desc->m_elementSize, 0);
                }

                SrgBufferDescriptor* desc = &m_hairRenerDescriptors[uint8_t(HairRenderBuffersSemantics::HairVertexRenderParams)];
                updateSuccess &= m_hairVertexRenderParams->UpdateData((void*)asset.m_thicknessCoeffs.data(), desc->m_elementCount * desc->m_elementSize, 0);

                // [To Do] Adi: no need to update index buffer data unless we go to dynamic reduction
                return updateSuccess;
            }

            //!=====================================================================================
            //!
            //!                                    Update Methods
            //!
            //!-------------------------------------------------------------------------------------
            // TODO Move wind settings to Simulation Parameters or alike.

            // Wind is in a pyramid around the main wind direction.
            // To add a random appearance, the shader will sample some direction
            // within this cone based on the strand index.
            // This function computes the vector for each edge of the pyramid.
            static void SetWindCorner(
                Quaternion rotFromXAxisToWindDir, Vector3 rotAxis,
                float angleToWideWindCone, float wM, AMD::float4& outVec)
            {
                static const Vector3 XAxis(1.0f, 0, 0);
                Quaternion rot(rotAxis, angleToWideWindCone);
// org               Vector3 newWindDir = rotFromXAxisToWindDir * rot * XAxis;
                Vector3 newWindDir = (rotFromXAxisToWindDir * rot).TransformVector(XAxis);  // [To Do] Adi: test this
                outVec.x = newWindDir.GetX() * wM;
                outVec.y = newWindDir.GetY() * wM;
                outVec.z = newWindDir.GetZ() * wM;
                outVec.w = 0;  // unused.
            }

            void HairRenderObject::SetWind(const Vector3& windDir, float windMag, int frame)
            {
                float wM = windMag * (pow(sin(frame * 0.01f), 2.0f) + 0.5f);

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
// org                    rotFromXAxisToWindDir.SetRotation(xCrossW.Normalize(), angle);
                    rotFromXAxisToWindDir.CreateFromVector3AndValue(xCrossW.GetNormalized(), angle);
                }

                const float angleToWideWindCone = AZ::DegToRad(40.f);



                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 1.0, 0),  angleToWideWindCone, wM, m_simCB->m_Wind);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, -1.0, 0), angleToWideWindCone, wM, m_simCB->m_Wind1);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 0, 1.0),  angleToWideWindCone, wM, m_simCB->m_Wind2);
                SetWindCorner(rotFromXAxisToWindDir, Vector3(0, 0, -1.0), angleToWideWindCone, wM, m_simCB->m_Wind3);
                // fourth component unused. (used to store frame number, but no longer used).
            }

            void HairRenderObject::InitBoneMatricesPlaceHolder(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices)
            {
                pBoneMatricesInWS;

                float identityValues[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
                int numMatrices = AZStd::min(numBoneMatrices, AMD_TRESSFX_MAX_NUM_BONES);
                for (int i = 0; i < numMatrices; ++i)
                {
                    memcpy(m_simCB->m_BoneSkinningMatrix[i].m, identityValues, 16 * sizeof(float));
                }
            }

            // [To do] Adi: move all CB update methods to be constant buffer structure internal methods
            // and only expose via HairObject.
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

            // [To Do] Adi: Change only parameters that require per frame update such as time.
            // The rest should be changed only when changed (per editor or script)
            //! Update of simulation constant buffer.
            //! Notice that the bone matrices are set elsewhere and should be updated before GPU submit.
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

            // [To Do] Adi: Change only parameters that require per frame update such as distance.
            // The rest should be changed only when changed (per editor or script)
            void HairRenderObject::UpdateRenderingParameters(
                const AMD::TressFXRenderingSettings* parameters, const int nodePoolSize,
                float distance, bool shadowUpdate /*= false*/)
            {
                // Update Render Parameters
                m_renderCB->FiberRadius = parameters->m_FiberRadius;	// Don't modify radius by LOD multiplier as this one is used to calculate shadowing and that calculation should remain unaffected

                m_renderCB->ShadowAlpha = parameters->m_HairShadowAlpha;
                m_renderCB->FiberSpacing = parameters->m_HairFiberSpacing;

                m_renderCB->HairKs2 = parameters->m_HairKSpec2;
                m_renderCB->HairEx2 = parameters->m_HairSpecExp2;

                m_renderCB->MatKValue = { 0.f, parameters->m_HairKDiffuse, parameters->m_HairKSpec1, parameters->m_HairSpecExp1 }; // no ambient

                m_renderCB->MaxShadowFibers = parameters->m_HairMaxShadowFibers;

                // Update Strand Parameters (per hair object)
                m_strandCB->MatBaseColor = ToAMDFloat4(parameters->m_HairMatBaseColor);
                m_strandCB->MatTipColor = ToAMDFloat4(parameters->m_HairMatTipColor);
                m_strandCB->TipPercentage = parameters->m_TipPercentage;
                m_strandCB->StrandUVTilingFactor = parameters->m_StrandUVTilingFactor;
                m_strandCB->FiberRatio = parameters->m_FiberRatio;

                // Reset LOD hair density for the frame
                m_LODHairDensity = 1.f;

                float FiberRadius = parameters->m_FiberRadius;
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
                        float MaxLODFiberRadius = FiberRadius * (shadowUpdate ? parameters->m_ShadowLODWidthMultiplier : parameters->m_LODWidthMultiplier);
                        FiberRadius = FiberRadius + (DistanceRatio * (MaxLODFiberRadius - FiberRadius));

                        // Lerp: x + s(y-x)
                        m_LODHairDensity = 1.f + (DistanceRatio * ((shadowUpdate ? parameters->m_ShadowLODPercent : parameters->m_LODPercent) - 1.f));
                    }
                }

                m_strandCB->FiberRadius = FiberRadius;

                m_strandCB->NumVerticesPerStrand = m_NumVerticesPerStrand;  // Always constant
                m_strandCB->EnableThinTip = parameters->m_EnableThinTip;
                m_strandCB->NodePoolSize = nodePoolSize;
                m_strandCB->RenderParamsIndex = m_RenderIndex;				// Always constant

                m_strandCB->EnableStrandUV = parameters->m_EnableStrandUV;
                m_strandCB->EnableStrandTangent = parameters->m_EnableStrandTangent;
            }

            //!=====================================================================================
            //!
            //!                     Generic non StressFX Specific Methods
            //!
            //!-------------------------------------------------------------------------------------
            //! This is the equivalent method to the TressFXObject constructor.
            //! It prepare all dynamic and static buffers and load the data into them, then
            //!  creates all the Srgs associated with the buffers and the remaining structures
            //!  that drive skinning, simulation and rendering of the hair.
            //!---------------------------------------------------------------------
            bool HairRenderObject::Init(
                HairFeatureProcessor* featureProcessor,
                const char* assetName, AMD::TressFXAsset* asset,
                AMD::TressFXSimulationSettings* simSettings, AMD::TressFXRenderingSettings* renderSettings)
            {
                AZ_TRACE_METHOD();

                ++s_objectCounter;

                AZ_Error("Hair Gem", featureProcessor, "Feature processor initialized as null hence preventing proper creation of hair");
                m_featureProcessor = featureProcessor;

                m_NumTotalVertices = asset->m_numTotalVertices;
                m_NumTotalStrands = asset->m_numTotalStrands;
                m_numGuideVertices = asset->m_numGuideVertices;
                m_NumVerticesPerStrand = asset->m_numVerticesPerStrand;
                m_NumFollowHairsPerGuideHair = asset->m_numFollowStrandsPerGuide;

                 // dummy method - replace with the bone matrices at init pose and the real bones amount
                InitBoneMatricesPlaceHolder(nullptr, AMD_TRESSFX_MAX_NUM_BONES);
                // First time around, make sure all parameters are properly filled
                UpdateSimulationParameters(simSettings, 0.035f); // large fast step for faster simulation, but..

                // [To Do] Adi: change to be dynamically calculated before the final submit.
                const float distanceFromCamera = 1.0; 
                const float updateShadows = false;
                UpdateRenderingParameters(renderSettings, RESERVED_PIXELS_FOR_OIT, distanceFromCamera, updateShadows);

                if (!GetShaders())
                {
                    return false;
                }

                //-------------------------------------
                // Dynamic buffers, data and Srg creation - shared between passes and changed on the GPU
                if (!m_dynamicHairData.CreateAndBindGPUResources(m_skinningShader, m_NumTotalVertices, m_NumTotalStrands))
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
                renderResourcesSuccess = CreateRenderingGPUResources(m_PPLLFillShader, *asset, assetName);
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
                    Data::Instance<HairPPLLRasterPass> rasterPass = m_featureProcessor->GetHairPPLLRasterPass();
                    if (!rasterPass.get())
                    {
                        AZ_Error("Hair Gem", false, "Failed to get PPLL raster fill Pass.");
                        return false;
                    }

                    m_PPLLFillShader = rasterPass->GetShader();
                    if (!m_PPLLFillShader)
                    {
                        AZ_Error("Hair Gem", false, "Failed to get hair raster fill shader from raster pass");
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
                // [To Do] Adi: can the CB update be skipped if no change was done?
                {
                    // Protect Update and Render if on async threads
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

                    updatedCB = m_simCB.UpdateGPUData();
                    updatedCB &= m_renderCB.UpdateGPUData();
                    updatedCB &= m_strandCB.UpdateGPUData();
                }

                RPI::ShaderResourceGroup* simSrg = m_dynamicHairData.GetSimSRG().get();
                RPI::ShaderResourceGroup* generationSrg = m_hairGenerationSrg.get();
                RPI::ShaderResourceGroup* renderMaterialSrg = m_hairRenderSrg.get();
                if (!simSrg || !generationSrg || !renderMaterialSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to get one of the Hair Object Srgs.");
                    return false;
                }

                // Single compilation per frame
                simSrg->Compile();
                generationSrg->Compile();
                renderMaterialSrg->Compile();

                IncreaseSimulationFrame();

                return updatedCB;
            }

            // Move this to be built by the raster fill pass per object in the list during the Simulate?
            bool HairRenderObject::BuildPPLLDrawPacket(RPI::ShaderResourceGroup* perPassSrg, RHI::DrawPacketBuilder::DrawRequest& drawRequest)
            {
                RHI::DrawPacketBuilder drawPacketBuilder;

                RHI::DrawIndexed drawIndexed;
                drawIndexed.m_indexCount = m_TotalIndices;
                drawIndexed.m_indexOffset = 0;
                drawIndexed.m_vertexOffset = 0;

                drawPacketBuilder.Begin(nullptr);
                drawPacketBuilder.SetDrawArguments(drawIndexed);
                drawPacketBuilder.SetIndexBufferView(m_indexBufferView);

                // Adi: m_hairRenderSrg is per material srg - should it be avoided from adding it as the per draw srg?
                RPI::ShaderResourceGroup* renderMaterialSrg = m_hairRenderSrg.get();
                RPI::ShaderResourceGroup* simSrg = m_dynamicHairData.GetSimSRG().get();

                if (!renderMaterialSrg || !simSrg || !perPassSrg)
                {
                    AZ_Error("Hair Gem", false, "Failed to get per pass or hair material Srg for the raster pass.");
                    return false;
                }
                // No need to compile the simSrg since it was compiled already by the Compute pass this frame
                drawPacketBuilder.AddShaderResourceGroup(renderMaterialSrg->GetRHIShaderResourceGroup());
                drawPacketBuilder.AddShaderResourceGroup(simSrg->GetRHIShaderResourceGroup());

                drawPacketBuilder.AddDrawItem(drawRequest);

                // [To Do] Adi: avoid doing the following every frame. 
                if (m_fillDrawPacket)
                {
                    delete m_fillDrawPacket;
                }
                m_fillDrawPacket = drawPacketBuilder.End();

                if (!m_fillDrawPacket)
                {
                    AZ_Error("Hair Gem", false, "Failed to build the hair DrawPacket.");
                    return false;
                }

                return true;
            }

            const RHI::DispatchItem* HairRenderObject::GetDispatchItem(RPI::ShaderResourceGroup* perPassSrg)
            {
                auto dispatchIter = m_dispatchItems.find(perPassSrg);
                if (dispatchIter == m_dispatchItems.end())
                {
                    AZ_Error("Hair Gem", false, "GetDispatchItem could not find the dispatch item based on the given shader resource group");
                    return nullptr;
                }
                return dispatchIter->second->GetDispatchItem();
            }

            bool HairRenderObject::BuildDispatchItem(
                RPI::Shader* computeShader,
                RPI::ShaderResourceGroup* perPassSrg,
                DispatchLevel dispatchLevel)
            {
                RPI::ShaderResourceGroup* simSrg = m_dynamicHairData.GetSimSRG().get();
                RPI::ShaderResourceGroup* hairGenerationSrg = m_hairGenerationSrg.get();
                if (!simSrg || !hairGenerationSrg || !computeShader)
                {
                    AZ_Error("Hair Gem", false, "Failed to get Skinning Pass or one of the Srgs.");
                    return false;
                }

                uint32_t elementsAmount =
                    (dispatchLevel == DispatchLevel::DISPATCHLEVEL_VERTEX) ?
                    m_numGuideVertices : //, m_NumTotalVertices
                    m_NumTotalStrands;

                m_dispatchItems[perPassSrg] = aznew HairDispatchItem;
                m_dispatchItems[perPassSrg]->InitSkinningDispatch(
                    computeShader, hairGenerationSrg, simSrg, elementsAmount );

                return true;
            }

        } // namespace Hair
    } // namespace Render
} // namespace AZ
#pragma optimize("", on)
