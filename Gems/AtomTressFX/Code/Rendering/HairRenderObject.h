/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/GeometryView.h>

// Hair specific
#include <TressFX/AMD_TressFX.h>
#include <TressFX/AMD_Types.h>
#include <TressFX/TressFXConstantBuffers.h>

#include <Rendering/HairCommon.h>
#include <Rendering/SharedBuffer.h>
#include <Rendering/HairDispatchItem.h>
#include <Rendering/HairBuffersSemantics.h>

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

        namespace Hair
        {
            class HairFeatureProcessor;

            //! TressFXStrandLevelData represents blended bone data per hair strand that once calculated
            //!  is passed between the skinning pass and the simulation shape constraints pass
            struct TressFXStrandLevelData
            {
                AMD::float4 skinningQuat;
                AMD::float4 vspQuat;
                AMD::float4 vspTranslation;
            };

            //!-----------------------------------------------------------------------------------------
            //!
            //!                                   DynamicHairData
            //!
            //!-----------------------------------------------------------------------------------------
            //! Contains the writable data that is passed and used by 3 modules:
            //!  simulation, signed distance field (collisions), and rendering.
            //! Rendering uses current position and tangent as SRVs in VS for computing creation and skinning. 
            //! Since this data is per object (hence per object dispatch) and requires sync point (barrier) between the
            //!  the passes, a single buffer is allocated and is shared by all hair objects and their 'streams'
            //!  where each have buffer view so that it points to its own portion of the original buffer's data.
            //! The shared buffer is therefore declared in the pass Srg to result in an execution dependency
            //!  so that a barrier will be created.  It also represents less overhead since we are using a single
            //!  coordinated / shared buffer sync point rather than many barriers (per object per buffer).
            //!-----------------------------------------------------------------------------------------
            class DynamicHairData
            {
                friend class HairRenderObject;

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
                //! Do not call this method before calling CreateAndBindGPUResources as it is already called
                //! from CreateAndBindGPUResources.
                //! This method can be called also for retrieving the descriptors table (SharedBuffer)
                static void PrepareSrgDescriptors(
                    AZStd::vector<SrgBufferDescriptor>& descriptorArray,
                    int32_t vertexCount, uint32_t strandsCount);

                void PrepareSrgDescriptors(int32_t vertexCount, uint32_t strandsCount)
                {
                    PrepareSrgDescriptors(m_dynamicBuffersDescriptors, vertexCount, strandsCount);
                }
 
                Data::Instance<RPI::ShaderResourceGroup> GetSimSrgForCompute()
                {
                    return m_initialized ? m_simSrgForCompute : nullptr;
                }

                Data::Instance<RPI::ShaderResourceGroup> GetSimSrgForRaster()
                {
                    return m_initialized ? m_simSrgForRaster : nullptr; }

                bool IsInitialized() { return m_initialized;  }


            private:
                //! Matching between the buffers Srg and its buffers descriptors, this method fills the Srg with
                //!  the views of the buffers to be used by the hair instance.
                bool BindPerObjectSrgForCompute();
                bool BindPerObjectSrgForRaster();

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
                //!  else they are cleared from the buffer via the reference mechanism.
                AZStd::vector<Data::Instance<HairSharedBufferAllocation>> m_dynamicViewAllocators;

                //------------------------------------------------------------------
                //! The following SRGs are the ones represented by this class' data.
                //! These Srgs are required for the changed dynamic data passed between the
                //!  skinning, simulation and rendering passes / shaders.
                //! It is the TressFX equivalent of the set:
                //!     - pSimPosTanLayout / m_pSimBindSets
                //------------------------------------------------------------------
                Data::Instance<RPI::ShaderResourceGroup> m_simSrgForCompute;    //! TressFX equivalent: pSimPosTanLayout / m_pSimBindSets
                Data::Instance<RPI::ShaderResourceGroup> m_simSrgForRaster;     //! Targeting only the Fill pass / shader

                bool m_initialized = false;
            };


            //!-----------------------------------------------------------------------------------------
            //!
            //!                                    HairRenderObject
            //!
            //!-----------------------------------------------------------------------------------------
            //! This class is equivalent to TressFXHairObject and HairStrands (the later is mainly a wrapper).
            //! This is the class that holds all the raw data used by all the hair passes and shaders.
            //!-----------------------------------------------------------------------------------------
            class HairRenderObject final
                : public Data::InstanceData
            {
                friend HairFeatureProcessor;

            public:
                AZ_RTTI(HairRenderObject, "{58F48A58-C5B9-4CAE-9AFD-9B3AF3A01C73}");
                HairRenderObject() = default;
                ~HairRenderObject();

                void Release();

                bool Init(
                    HairFeatureProcessor* featureProcessor, const char* assetName, AMD::TressFXAsset* asset,
                    AMD::TressFXSimulationSettings* simSettings, AMD::TressFXRenderingSettings* renderSettings
                );

                bool BuildDrawPacket(RPI::Shader* geometryShader, RHI::DrawPacketBuilder::DrawRequest& drawRequest);

                const RHI::DrawPacket* GetGeometrylDrawPacket(RPI::Shader* geometryShader);

                //! Creates and fill the dispatch item associated with the compute shader
                bool BuildDispatchItem(RPI::Shader* computeShader, DispatchLevel dispatchLevel);

                const RHI::DispatchItem* GetDispatchItem(RPI::Shader* computeShader);

                void PrepareHairGenerationSrgDescriptors(uint32_t vertexCount, uint32_t numStrands);

                // Based on SkinnedMeshInputLod::CreateStaticBuffer
                bool CreateAndBindHairGenerationBuffers(uint32_t vertexCount, uint32_t strandsCount);

                //! Updates the buffers data for the hair generation.
                //! Does NOT update the bone matrices - they will be updated every frame.
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
                //! Methods partially imported from TressFXHairObject
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
                //! If the image was not changed it should only bind without the retrieve operation.
                bool PopulateDrawStrandsBindSet(AMD::TressFXRenderingSettings* pRenderSettings/*=nullptr*/);

                // This function will be called when the image asset changed for the component.
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
                void InitBoneMatricesPlaceHolder(int numBoneMatrices);

                void SetFrameDeltaTime(float deltaTime);
                //! Updating the bone matrices for the skinning in the simulation constant buffer.
                //! pBoneMatricesInWS constraints array of column major bone matrices in world space.
                void UpdateRenderingParameters(
                    const AMD::TressFXRenderingSettings* parameters, const int nodePoolSize,
                    float distance, bool shadowUpdate /*= false*/);

                AMD::TressFXRenderParams* GetHairRenderParams() { return m_renderCB.get(); };

                //! Update of simulation constant buffer.
                //! Notice that the bone matrices are set elsewhere and should be updated before GPU submit.
                void UpdateSimulationParameters(const AMD::TressFXSimulationSettings* settings, float timeStep);

                void SetWind(const Vector3& windDir, float windMag, int frame);
                void SetRenderIndex(uint32_t renderIndex) { m_RenderIndex = renderIndex; }

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
                //----------------------- Private Methods --------------------------
                bool BindRenderSrgResources();
                void PrepareRenderSrgDescriptors();
                bool GetShaders();

                //------------------------------ Data ------------------------------
                static uint32_t s_objectCounter;

                //! The feature processor is the centralized class that gathers all render nodes and
                //!   responsible for the various stages and passes' updates
                HairFeatureProcessor* m_featureProcessor = nullptr;

                //! Skinning compute shader used for creation of the compute Srgs and dispatch item
                Data::Instance<RPI::Shader> m_skinningShader = nullptr;

                //! Compute dispatch items map per the existing passes
                AZStd::unordered_map<RPI::Shader*, Data::Instance<HairDispatchItem>> m_dispatchItems;

                //! Geometry raster shader used for creation of the raster Srgs.
                //! Since the Srgs for geometry raster are the same across the shaders we keep
                //! only a single shader - if this to change in the future, several shaders and sets
                //! of dynamic Srgs should be created. 
                Data::Instance<RPI::Shader> m_geometryRasterShader = nullptr;

                //! DrawPacket for the multi object geometry raster pass.
                AZStd::unordered_map<RPI::Shader*, RHI::ConstPtr<RHI::DrawPacket>>  m_geometryDrawPackets;

                float m_frameDeltaTime = 0.02;

                //! The following are the configuration settings that might be required during the update.
                AMD::TressFXSimulationSettings* m_simSettings = nullptr;
                AMD::TressFXRenderingSettings* m_renderSettings = nullptr;

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

                //! Controls reset / copy base hair state
                uint32_t m_SimulationFrame = 0;
                //! The index used as a look up into the material array during the resolve pass
                uint32_t m_RenderIndex = 0;

                //!-----------------------------------------------------------------
                //! The hair dynamic per instance buffers such as vertices, tangents, etc..
                //! The data of these buffers is read/write and will change between passes.
                DynamicHairData m_dynamicHairData;

                //!-----------------------------------------------------------------
                //! Static buffers & Srg: Initial position, bones transform skinning
                //!  data, physical hair properties..
                //!-----------------------------------------------------------------
                AZStd::vector<Data::Instance<RPI::Buffer>> m_hairGenerationBuffers;
                AZStd::vector<SrgBufferDescriptor> m_hairGenerationDescriptors;
                //! The simulation parameters constant buffer.
                HairUniformBuffer<AMD::TressFXSimulationParams> m_simCB; 
                Data::Instance<RPI::ShaderResourceGroup> m_hairGenerationSrg; 

                //!-----------------------------------------------------------------
                //!         TressFXRenderParams Srg buffers and declarations
                //! The rendering buffers and structures required for the render draw
                //!  calls and are sent to the GPU using TressFXRenderParams Srg.
                //!-----------------------------------------------------------------
                //! Vertex and UV buffers.
                //! Naming was not changed to preserve correlation to TressFXHairObject.h
                Data::Instance<RPI::Buffer> m_hairVertexRenderParams;
                Data::Instance<RPI::Buffer> m_hairTexCoords;
                
                //! Base color of the hair root and per strand texture.
                Data::Instance<RPI::Image> m_baseAlbedo;
                Data::Instance<RPI::Image> m_strandAlbedo;

                HairUniformBuffer<AMD::TressFXRenderParams> m_renderCB;
                HairUniformBuffer<AMD::TressFXStrandParams> m_strandCB;

                AZStd::vector<SrgBufferDescriptor> m_hairRenderDescriptors;
                // Equivalent to m_pRenderLayoutBindSet in TressFX.
                Data::Instance<RPI::ShaderResourceGroup> m_hairRenderSrg;   
 
                //! Index buffer for the render pass via draw calls - naming was kept
                Data::Instance<RHI::Buffer> m_indexBuffer;
                RHI::GeometryView m_geometryView;
                //-------------------------------------------------------------------

                AZStd::mutex m_mutex;
            };

        } // namespace Hair

    } // namespace Render

} // namespace AZ
