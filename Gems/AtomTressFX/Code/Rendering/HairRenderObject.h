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

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/DrawPacketBuilder.h>

// Hair specific
#include <TressFX/AMD_TressFX.h>
#include <TressFX/AMD_Types.h>
#include <TressFX/TressFXConstantBuffers.h>

#include <Rendering/HairCommon.h>
#include <Rendering/SharedBuffer.h>
#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairBuffersSemantics.h>
#include <Rendering/HairRenderObjectInterface.h>

#define TRESSFX_MIN_VERTS_PER_STRAND_FOR_GPU_ITERATION 64

namespace AMD
{
    struct float4x4;

    class TressFXAsset;
    class TressFXRenderingSettings;
    class TressFXSimulationSettings;
}

namespace AZ
{
    namespace RHI
    {
        class DrawPacket;
    }

    namespace RPI
    {
        class Model;
        class Scene;
        class Shader;
    }

    namespace Render
    {
        class HairFeatureProcessor;

        namespace Hair
        {
            class HairFeatureProcessor;

            //! TressFXStrandLevelData represents blended bone data per hair strand that once calculated
            //!  is passed between the skinning pass and the simulation shape constrains pass
            //! This is strictly used fir capsule collisions and can be removed if this is not required.
            //! The hair to mesh collision itself is done today with per object collision mesh and SDF.
            struct TressFXStrandLevelData
            {
                AMD::float4 skinningQuat;
                AMD::float4 vspQuat;
                AMD::float4 vspTranslation;
            };

            //------------------------------------------------------------------------------------------
            //
            //                                  DynamicHairData
            //
            //------------------------------------------------------------------------------------------
            //! Contains the dynamic data that is used by 3 modules: simulation, signed distance field, and rendering.
            //! Rendering uses current position and tangent as SRVs in VS.
            //! Equivalent to the original class TressFXDynamicHairData (or TressFXDynamicState as it was originally named).
            //! Since this data is per object (hence per object dispatch) and requires sync point (barrier) between the
            //!  the passes, we need to allocate a single buffer (per input stream) shared by all hair objects and
            //!  have buffer view held by each object so that it points to its own portion of the data.
            //! These buffers will need to be declared in the pass so that an execution dependency and a barrier
            //!  will be created.
            //!
            //! Reference:
            //!  - SkinnedMeshInputLod::CreateSkinningInputBuffer will create the views / sub buffers that will be bound
            //!     into the PerInstance SRG buffer entries and used for each dispatch
            //!  - SkinnedMeshComputePass::BuildAttachmentsInternal() will bind the single actual shared buffer that
            //!     contains all the sub-buffer associated with the per instance SRG of a dispatch. 
            class DynamicHairData
            {
            public:           
                //! Creates the GPU dynamic buffers of a single hair object
                //! Equivalent to TressFXDynamicHairData::CreateGPUResources
                bool CreateDynamicGPUResources(
                    Data::Instance<RPI::Shader> computeShader,
                    Data::Instance<RPI::Shader> rasterShader,
                    uint32_t vertexCount, uint32_t strandsCount);

                //! Data upload - copy the hair mesh asset data (positions and tangents) into the buffers.
                //! In the following line I assume that positions and tangents are of the same size.
                //! Equivalent to: TressFXDynamicHairData::UploadGPUData
                bool UploadGPUData(const char* name, void* positions, void* tangents);

                //! Preparation of the descriptors table of all the dynamic stream buffers within the class.
                //! Do not call this method manually as it is called from CreateAndBindGPUResources.
                //! Can be called from the outside to retrieve the descriptors table (SharedBuffer)
                static void PrepareSrgDescriptors(
                    AZStd::vector<SrgBufferDescriptor>& descriptorArray,
                    int32_t vertexCount, uint32_t strandsCount);

                void PrepareSrgDescriptors(int32_t vertexCount, uint32_t strandsCount)
                {
                    PrepareSrgDescriptors(m_dynamicBuffersDescriptors, vertexCount, strandsCount);
                }

                //! Matching between the buffers Srg and its buffers descriptors, this method fills the Srg with
                //!  the views of the buffers to be used by the hair instance.
                //! Do not call this method manually as it is called from CreateAndBindGPUResources.
                bool BindPerObjectSrgForCompute();
                bool BindPerObjectSrgForRaster();
 
                Data::Instance<RPI::ShaderResourceGroup> GetSimSrgForCompute() { return m_simSrgForCompute; }
                Data::Instance<RPI::ShaderResourceGroup> GetSimSrgForRaster() { return m_simSrgForRaster; }

                bool IsInitialized() { return m_initialized;  }

            private:
                //! The descriptors required to allocate and associate the dynamic buffers with the SRGs
                //! Each descriptor also contains the byte offsets of the sub-buffers in the global dynamic
                //!  array for the data copy.
                AZStd::vector<SrgBufferDescriptor> m_dynamicBuffersDescriptors;

                //! The following dynamic buffer views are views 'sub-buffers' located within a global large
                //!  dynamic buffer exposed and connected as an attachment between the passes and therefore
                //!  creates both dependency order between passes execution and sync point barrier.
                //! This indirectly forces the sync to be applied to all 'sub-buffers' used by each of the
                //!  HairObjects / HairDispatches and therefore allows us to change their data in the shader
                //!  between passes.
                AZStd::vector<Data::Instance<RHI::BufferView>> m_dynamicBuffersViews;   // RW used for the Compute
                AZStd::vector<Data::Instance<RHI::BufferView>> m_readBuffersViews;      // Read only used for the Raster fill

                //! The following vector is required in order to keep the allocators 'alive' or
                //!  else they are cleared from the buffer and can be re-allocated.
                AZStd::vector<Data::Instance<SharedBufferAllocation>> m_dynamicViewAllocators;


                // QUESTION: why do we want different Srgs? why not use the same Srgs of the dynamic data
                //  even if it means that we expose more data to some passes / shaders.
                // The other Srgs and descriptors array are commented out for now for this reason.

                //------------------------------------------------------------------
                //! The following SRGs are the ones represented by this class' data.
                //! These Srgs are required for the changed dynamic data passed between the
                //!  skinning, simulation and rendering passes / shaders.
                //! It is the TressFX equivalent of the set:
                //!     - pSimPosTanLayout / m_pSimBindSets
                //! In out approach we will utilize the same SRG for all passes and so this also
                //!  applies to the TressFX sets:
                //!     - pApplySDFLayout / m_pApplySDFBindSets
                //!     - pRenderPosTanLayout / m_pRenderBindSets.    
                //------------------------------------------------------------------
                Data::Instance<RPI::ShaderResourceGroup> m_simSrgForCompute;    //! TressFX equivalent: pSimPosTanLayout / m_pSimBindSets
                Data::Instance<RPI::ShaderResourceGroup> m_simSrgForRaster;     //! Targeting only the Fill pass / shader

                bool m_initialized = false;
            };


            //------------------------------------------------------------------------------------------
            //
            //                                  HairRenderObject
            //
            //------------------------------------------------------------------------------------------
            //! DynamicHairData holds all data required to skin, simulate and render a single hair batch object,
            //!  for example complete body fur or head hair coverage.
            //! It contains the simulation and rendering data, and the dispatch items required by the passes.
            //! 
            //! This class is equivalent to TressFXHairObject and HairStrands (the later is mainly a wrapper).
            class HairRenderObject final
                : public Data::InstanceData
                , public HairRenderObjectInterface
            {
                friend HairFeatureProcessor;

            public:
                AZ_RTTI(HairRenderObject, "{58F48A58-C5B9-4CAE-9AFD-9B3AF3A01C73}", HairRenderObjectInterface);
                HairRenderObject() = default;
                ~HairRenderObject();

                void Release();

                bool Init(
                    HairFeatureProcessor* featureProcessor, const char* assetName, AMD::TressFXAsset* asset,
                    AMD::TressFXSimulationSettings* simSettings, AMD::TressFXRenderingSettings* renderSettings
                );

                //! Creates and fill the draw item associated with the PPLL render of the
                //!  current hair object
                const RHI::DrawPacket* GetFillDrawPacket()
                {
                    return m_fillDrawPacket;
                }

                bool BuildPPLLDrawPacket(RHI::DrawPacketBuilder::DrawRequest& drawRequest);

                //! Creates and fill the dispatch item associated with the compute shader
                bool BuildDispatchItem(RPI::Shader* computeShader, DispatchLevel dispatchLevel);

                const RHI::DispatchItem* GetDispatchItem(RPI::Shader* computeShader);

                void PrepareHairGenerationSrgDescriptors(uint32_t vertexCount, uint32_t numStrands);

                // Based on SkinnedMeshInputLod::CreateStaticBuffer
                bool CreateAndBindHairGenerationBuffers(uint32_t vertexCount, uint32_t strandsCount);

                //! Updates the buffers data for the hair generation.
                //! Notice: does not update the bone matrices that will be updated every frame.
                bool UploadGPUData(const char* name, AMD::TressFXAsset* asset);

                Data::Instance<RPI::ShaderResourceGroup> GetHairGenerationSrg()
                {
                    return m_hairGenerationSrg;
                }

                bool BindPerObjectSrgForCompute()
                {
                    return m_dynamicHairData.IsInitialized() ? m_dynamicHairData.BindPerObjectSrgForCompute() : false;
                }

                bool BindPerObjectSrgForRaster()
                {
                    return m_dynamicHairData.IsInitialized() ? m_dynamicHairData.BindPerObjectSrgForRaster() : false;
                }

                //!-----------------------------------------------------------------
                //! Methods imported from TressFXHairObject
                //!-----------------------------------------------------------------
                int GetNumTotalHairVertices() const { return m_NumTotalVertices; }
                int GetNumTotalHairStrands() const { return m_NumTotalStrands; }
                int GetNumVerticesPerStrand() const { return m_NumVerticesPerStrand; }
                int GetCPULocalShapeIterations() const { return  m_CPULocalShapeIterations; }
                int GetNumFollowHairsPerGuideHair() const { return m_NumFollowHairsPerGuideHair; }
                int GetNumGuideHairs() const
                {
                    return GetNumTotalHairStrands() / (GetNumFollowHairsPerGuideHair() + 1);
                }
                //! This method is mainly a wrapper around BindRenderSrgResources to keep the
                //! connection in code to the TressFX method.
                //! Bind Render Srg (m_hairRenderSrg) resources. No resources data update should be doe here
                //! Notice that this also loads the images and is slower if a new asset is required.
                //!     If the image was not changed it should only bind without the retrieve operation.
                bool PopulateDrawStrandsBindSet(AMD::TressFXRenderingSettings* pRenderSettings/*=nullptr*/);

                // This function will called when the image asset changed from the component.
                bool LoadImageAsset(AMD::TressFXRenderingSettings* pRenderSettings);

                bool UploadRenderingGPUResources(AMD::TressFXAsset& asset);

                //! Creation of the render Srg m_hairRenderSrg, followed by creation and binding of the
                //! GPU render resources: vertex thickness, vertex UV, hair albedo maps and two constant buffers.
                bool CreateRenderingGPUResources(
                    Data::Instance<RPI::Shader> shader, AMD::TressFXAsset& asset, const char* assetName);

                bool Update();

                //! This method needs to be called in order to fill the bone matrices before the skinning
                void UpdateBoneMatrices(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices);
                //! update of the skinning matrices per frame.  The matrices are in model / local space
                //!  which is why the entity world matrix is also passed.
                void UpdateBoneMatrices(const AZ::Matrix3x4& entityWorldMatrix, const AZStd::vector<AZ::Matrix3x4>& boneMatrices);
                void InitBoneMatricesPlaceHolder(const AMD::float4x4* pBoneMatricesInWS, int numBoneMatrices);

                void SetFrameDeltaTime(float deltaTime);
                //! Updating the bone matrices for the skinning in the simulation constant buffer.
                //! pBoneMatricesInWS constrains array of column major bone matrices in world space.
                void UpdateRenderingParameters(
                    const AMD::TressFXRenderingSettings* parameters, const int nodePoolSize,
                    float distance, bool shadowUpdate /*= false*/);

                AMD::TressFXRenderParams* GetHairRenderParams() { return m_renderCB.get(); };

                //! Update of simulation constant buffer.
                //! Notice that the bone matrices are set elsewhere and should be updated before GPU submit.
                void UpdateSimulationParameters(const AMD::TressFXSimulationSettings* settings, float timeStep);

                void SetWind(const Vector3& windDir, float windMag, int frame);

                void ResetPositions() { m_simCB->g_ResetPositions = 1.0f; }
                void IncreaseSimulationFrame()
                {
                    m_simCB->g_ResetPositions = (m_SimulationFrame < 2) ? 1.0f : 0.0f;
                    m_SimulationFrame++;
                }

                bool IsEnabled()
                {
                    return m_enabled;
                }

                void SetEnabled(bool enable)
                {
                    m_enabled = enable;
                }
                //!-----------------------------------------------------------------

            private:
                bool BindRenderSrgResources();
                void PrepareRenderSrgDescriptors();
                bool GetShaders();


            private:
//                AZ_DISABLE_COPY_MOVE(HairRenderObject);

                static uint32_t s_objectCounter;

                //! The feature processor is the centralize entity that gathers all render nodes and
                //!   responsible for the various stages activation
                HairFeatureProcessor* m_featureProcessor = nullptr;

                //! The dispatch item used for the skinning
                HairDispatchItem m_skinningDispatchItem;    

                //! Array of pointers to the various dispatch items per the various passes
                //! CURRENTLY NOT USED
                AZStd::map<RPI::Shader*, Data::Instance<HairDispatchItem>> m_dispatchItems;

                //! Skinning compute shader used for creation of the compute Srgs and dispatch item
                Data::Instance<RPI::Shader> m_skinningShader = nullptr;

                //! PPLL fill shader used for creation of the raster Srgs and draw item
                Data::Instance<RPI::Shader> m_PPLLFillShader = nullptr;

                float m_frameDeltaTime = 0.02;

                //! Hair asset information
                uint32_t m_TotalIndices = 0;
                uint32_t m_NumTotalVertices = 0;
                uint32_t m_numGuideVertices = 0;
                uint32_t m_NumTotalStrands = 0;
                uint32_t m_NumVerticesPerStrand = 0;
                uint32_t m_CPULocalShapeIterations = 0;
                uint32_t m_NumFollowHairsPerGuideHair = 0;

                // LOD calculations factor
                float m_LODHairDensity = 1.0f;

                bool m_enabled = true;

                // Adi - check required: frame counter for wind effect
                uint32_t m_SimulationFrame = 0;
                // Adi: check if we need this: for parameter indexing of frame % 2
                uint32_t m_RenderIndex = 0;

                //!-----------------------------------------------------------------
                //! The hair dynamic per instance buffers such as vertices, tangents, etc..
                //! The data of these buffers is read/write and will change between passes.
                DynamicHairData m_dynamicHairData;

                //------------------------------------------------------------------
                //! Static buffers & Srg - initial position, bones transform skinning data, hair properties.. 
                AZStd::vector<Data::Instance<RPI::Buffer>> m_hairGenerationBuffers;
                AZStd::vector<SrgBufferDescriptor> m_hairGenerationDescriptors;
                HairUniformBuffer<AMD::TressFXSimulationParams> m_simCB; // The simulation parameters constant buffer.
                Data::Instance<RPI::ShaderResourceGroup> m_hairGenerationSrg; 
                //------------------------------------------------------------------


                //------------------------------------------------------------------
                //!         TressFXRenderParams Srg buffers and declarations
                //! The following section forms the rendering buffers and structures required
                //! for the render draw calls and sent to the GPU using TressFXRenderParams Srg.
                //! 
                //! Vertex and UV buffers.
                //! Naming was not changed to preserve correlation to TressFXHairObject.h
                Data::Instance<RPI::Buffer> m_hairVertexRenderParams;
                Data::Instance<RPI::Buffer> m_hairTexCoords;
                
                //! The textures are the color of the hair roots and can be skipped for now if desired.
                // Adi: what is the difference between Image and StreamingImage?
                Data::Instance<RPI::Image> m_baseAlbedo;
                Data::Instance<RPI::Image> m_strandAlbedo;

                HairUniformBuffer<AMD::TressFXRenderParams> m_renderCB;
                HairUniformBuffer<AMD::TressFXStrandParams> m_strandCB;

                AZStd::vector<SrgBufferDescriptor> m_hairRenerDescriptors;
                Data::Instance<RPI::ShaderResourceGroup> m_hairRenderSrg;   // equivalent to m_pRenderLayoutBindSet
                //-------------------------------------------------------------------

                //! Index buffer used for the render pass via draw calls - naming was not changed.
                Data::Instance<RHI::Buffer> m_indexBuffer;
                RHI::IndexBufferView m_indexBufferView;
                //-------------------------------------------------------------------

                const RHI::DrawPacket* m_fillDrawPacket = nullptr;

                AZStd::mutex m_mutex;
            };

        } // namespace Hair

    } // namespace Render

} // namespace AZ
