/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/Feature/RenderCommon.h>

#include <MeshletsRenderObject.h>
#include <MeshletsFeatureProcessor.h>

#ifndef SAFE_DELETE
    #define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
#endif

namespace AZ
{
    namespace Meshlets
    {
        MeshletsFeatureProcessor::MeshletsFeatureProcessor()
        {
            MeshletsComputePassName = Name("MeshletsComputePass");
            MeshletsRenderPassName = Name("MeshletsRenderPass");
            CreateResources();
        }

        MeshletsFeatureProcessor::~MeshletsFeatureProcessor()
        {
            CleanResources();
        }

        void MeshletsFeatureProcessor::CreateResources()
        {
            if (!Meshlets::SharedBufferInterface::Get())
            {   // Since there can be several pipelines, allocate the shared buffer only for the
                // first one and from that moment on it will be used through its interface
                AZStd::string sharedBufferName = "MeshletsSharedBuffer";
                uint32_t bufferSize = 256 * 1024 * 1024;

                // Prepare Render Srg descriptors for calculating the required alignment for the shared buffer
                MeshRenderData tempRenderData;
                MeshletsRenderObject::PrepareRenderSrgDescriptors(tempRenderData, 1, 1);

                m_sharedBuffer = AZStd::make_unique<Meshlets::SharedBuffer>(sharedBufferName, bufferSize, tempRenderData.RenderBuffersDescriptors);
            }

            m_renderObjectsMarkedForDeletion = m_meshletsRenderObjects;
            m_renderObjectsMarkedForDeletion.clear();
            DeletePendingMeshletsRenderObjects();
        }

        void MeshletsFeatureProcessor::CleanResources()
        {
            m_sharedBuffer.reset();
        }

        void MeshletsFeatureProcessor::CleanPasses()
        {
            m_computePass = nullptr;
            m_renderPass = nullptr;
        }

        void MeshletsFeatureProcessor::Init([[maybe_unused]]RPI::RenderPipeline* pipeline)
        {
            InitComputePass(MeshletsComputePassName);
            InitRenderPass(MeshletsRenderPassName);
        }

        void MeshletsFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshletsFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshletsFeatureProcessor::Activate()
        {
            m_transformServiceFeatureProcessor = GetParentScene()->GetFeatureProcessor<Render::TransformServiceFeatureProcessor>();
            AZ_Assert(m_transformServiceFeatureProcessor, "MeshFeatureProcessor requires a TransformServiceFeatureProcessor on its parent scene.");

            EnableSceneNotification();
            TickBus::Handler::BusConnect();
        }

        void MeshletsFeatureProcessor::Deactivate()
        {
            m_passRequestAsset.Reset();
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        int MeshletsFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        bool MeshletsFeatureProcessor::HasMeshletPasses(RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(MeshletsComputePassName, renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            return desiredPass ? true : false;
        }

        bool MeshletsFeatureProcessor::InitComputePass(const Name& passName)
        {
            m_computePass = Data::Instance<MultiDispatchComputePass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_computePass = static_cast<MultiDispatchComputePass*>(desiredPass.get());
                m_computeShader = m_computePass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }
            return true;
        }

        bool MeshletsFeatureProcessor::InitRenderPass(const Name& passName)
        {
            m_renderPass = Data::Instance<MeshletsRenderPass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_renderPass = static_cast<MeshletsRenderPass*>(desiredPass.get());
                m_renderShader = m_renderPass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }
            return true;
        }

        // This hook to the pipeline is still not connected and requires testing.
        // Current connection is by altering the two pipeline manually. Since the hook is
        // not the same for both pipelines, special care should be taken (on MainPipeline
        // it comes after OpaquePass while on the LowEndPipeline after MSAAResolvePass). It
        // is possible to simplify the logic and only hook to the active pipeline.
        bool MeshletsFeatureProcessor::AddMeshletsPassesToPipeline(RPI::RenderPipeline* renderPipeline)
        {
            if (HasMeshletPasses(renderPipeline))
            {   // This pipeline is not relevant - exist without changing anything.
                return true;
            }

            const char* passRequestAssetFilePath = "Passes/MeshletsPassRequest.azasset";
            m_passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            const AZ::RPI::PassRequest* passRequest = nullptr;
            if (m_passRequestAsset->IsReady())
            {
                passRequest = m_passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            }
            if (!passRequest)
            {
                AZ_Error("Meshlets", false, "Failed to add meshlets pass. Can't load PassRequest from [%s]", passRequestAssetFilePath);
                return false;
            }

            // Create the pass
            RPI::Ptr<RPI::Pass> pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!pass)
            {
                AZ_Error("Meshlets", false, "Failed creating meshlets pass from pass request for pipeline [%s]",
                    renderPipeline->GetId().GetCStr());
                return false;
            }

            // Add the pass to render pipeline
            bool success = renderPipeline->AddPassAfter(pass, Name("OpaquePass"));

            AZ_Error("Meshlets", success, "Meshlets pass injection to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());

            return success;
        }

        // This method will be called by the scene to establish all required injections
        // to the pass pipeline
        void MeshletsFeatureProcessor::AddRenderPasses(RPI::RenderPipeline* renderPipeline)
        {
            AddMeshletsPassesToPipeline(renderPipeline);
        }

        void MeshletsFeatureProcessor::SetTransform(
            const Render::TransformServiceFeatureProcessorInterface::ObjectId objectId, 
            const AZ::Transform& transform)
        {
            m_transformServiceFeatureProcessor->SetTransformForId(objectId, transform);
        }

        //==============================================================================
        // This method is called the first time that a render object is constructed and
        // does not need to be called again.
        // At each frame the MeshletsFeatureProcessor will call the AddDrawPackets per each
        // visible (multi meshlet) mesh and add its draw packets to the view.
        // The buffers for the render are passed and set via Srg and not as assembly buffers
        // which means that the instance Id must be set (for matrices) and vertex Id must
        // be used in the shader to address the buffers
        // Notice that because the object Id is mapped 1:1 with the DrawPacket, it currently
        // does not support instancing. For instancing support, a separate per Instance Srg
        // per call is required and the DrawPacket as well as the dispatch should be move to
        // become part of an object instance structure and not the render object (that is
        // shared between instances)
        bool MeshletsFeatureProcessor::BuildDrawPacket(
            ModelLodDataArray& lodRenderDataArray,
            Render::TransformServiceFeatureProcessorInterface::ObjectId objectId)
        {
            if (!m_renderPass)
            {
                return false;
            }

            for (uint32_t lod = 0; lod < lodRenderDataArray.size(); ++lod)
            {
                MeshRenderData* lodRenderData = lodRenderDataArray[lod];
                if (!lodRenderData || !lodRenderData->RenderObjectSrg)
                {
                    AZ_Error("Meshlets", false,
                        "Failed to build draw packet - missing LOD[%d] render data Render Srg.", lod);
                    return false;
                }

                // ObjectId belongs to the instance and not the object - to be moved
                lodRenderData->ObjectId = objectId;

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                m_renderPass->FillDrawRequestData(drawRequest);
                drawRequest.m_stencilRef = 0;
                drawRequest.m_sortKey = 0;
// Leave the following empty if using buffers rather than vertex streams.
//                drawRequest.m_streamBufferViews = lodRenderData->m_renderStreamBuffersViews; 

                RHI::DrawPacketBuilder drawPacketBuilder;
                RHI::DrawIndexed drawIndexed;

                drawIndexed.m_indexCount = lodRenderData->IndexCount;
                drawIndexed.m_indexOffset = 0;
                drawIndexed.m_vertexOffset = 0;

                drawPacketBuilder.Begin(nullptr);
                drawPacketBuilder.SetDrawArguments(drawIndexed);
                drawPacketBuilder.SetIndexBufferView(lodRenderData->IndexBufferView);

                // Add the object Id to the Srg - once instancing is supported, the ObjectId and the
                // render Srg should be per instance / draw and not per object.
                RHI::ShaderInputConstantIndex indexConstHandle = lodRenderData->RenderObjectSrg->FindShaderInputConstantIndex(Name("m_objectId"));
                if (!lodRenderData->RenderObjectSrg->SetConstant(indexConstHandle, objectId.GetIndex()))
                {
                    AZ_Error("Meshlets", false, "Failed to bind Render Constant [m_ObjectId]");
                }
                lodRenderData->RenderObjectSrg->Compile();

                // Add the per object render Srg - buffers required for the geometry render
                drawPacketBuilder.AddShaderResourceGroup(lodRenderData->RenderObjectSrg->GetRHIShaderResourceGroup());

                drawPacketBuilder.AddDrawItem(drawRequest);

                // Change the following line in order to support instancing.
                // For instancing the data cannot be associated 1:1 with the object
                lodRenderData->MeshDrawPacket = drawPacketBuilder.End();

                if (!lodRenderData->MeshDrawPacket)
                {
                    AZ_Error("Meshlets", false, "Failed to build the Meshlet DrawPacket.");
                    return false;
                }
            }

            return true;
        }

        // Notice that currently this does not support object instancing.  Each object is assumed to
        // have a single ObjectId and PerObject srg.
        // To enhance this, create an ObjectInstanceData and per instance srg rather than per object
        // and create a new instance every time this method is invoked.
        // This will also require to split the srg from the srg with the meshlets buffers.
        Render::TransformServiceFeatureProcessorInterface::ObjectId
            MeshletsFeatureProcessor::AddMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            Render::TransformServiceFeatureProcessorInterface::ObjectId objectId = m_transformServiceFeatureProcessor->ReserveObjectId();

            if (m_renderPass)
            {
                BuildDrawPacket(meshletsRenderObject->GetMeshletsRenderData(), objectId);
            }

            m_meshletsRenderObjects.push_back(meshletsRenderObject);

            AZ_Error("Meshlets", m_renderPass, "Meshlets object did not build DrawItem due to missing render pass");

            return objectId;
        }

        void MeshletsFeatureProcessor::DeletePendingMeshletsRenderObjects()
        {
            if (m_renderObjectsMarkedForDeletion.empty())
            {
                return;
            }

            for (auto renderObject : m_renderObjectsMarkedForDeletion)
            {
                ModelLodDataArray& modelLodArray = renderObject->GetMeshletsRenderData(0);
                for (auto& renderData : modelLodArray)
                {
                    if (m_transformServiceFeatureProcessor && renderData)
                    {
                        m_transformServiceFeatureProcessor->ReleaseObjectId(renderData->ObjectId);
                        break;  //same instance / object Id
                    }
                }
                m_meshletsRenderObjects.remove(renderObject);
                SAFE_DELETE(renderObject);
            }
            m_renderObjectsMarkedForDeletion.clear();
        }

        void MeshletsFeatureProcessor::RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            m_renderObjectsMarkedForDeletion.emplace_back(meshletsRenderObject);
        }

        void MeshletsFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            // OnTick can be used instead of the ::Simulate since it is set to be before the render
        }

        void MeshletsFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            AZ_UNUSED(packet);
        }

        void MeshletsFeatureProcessor::Render([[maybe_unused]] const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            // Skip adding draw or dispatch items if there it no hair render objects
            if (m_meshletsRenderObjects.size() == 0)
            {
                return;
            }

            // Remove any dangling leftovers 
            DeletePendingMeshletsRenderObjects();

            AZStd::list<RHI::DispatchItem*> dispatchItems;
            AZStd::list<const RHI::DrawPacket*> drawPackets;
            for (auto renderObject : m_meshletsRenderObjects)
            {
                // For demo purposed the model lod index is set for 0.
                // This entire control scheme will be removed to be replaced with GPU
                // driven pipeline control.
                ModelLodDataArray& modelLodArray = renderObject->GetMeshletsRenderData(0);

                for (auto& renderData : modelLodArray)
                {
                    if (renderData)
                    {
                        // the following is for testing only
                        Render::TransformServiceFeatureProcessorInterface::ObjectId objectId = renderData->ObjectId;
                        [[maybe_unused]] Transform transform = m_transformServiceFeatureProcessor->GetTransformForId(objectId);

                        if (MeshletsRenderObject::CreateAndBindComputeSrgAndDispatch(m_computeShader, *renderData))
                        {
                            dispatchItems.push_back(renderData->MeshDispatchItem.GetDispatchItem());
                        }
                        drawPackets.push_back(renderData->MeshDrawPacket);
                    }
                    AZ_Error("Meshlets", renderData, "Render data is NULL");
                }
            }
            m_computePass->AddDispatchItems(dispatchItems);
            m_renderPass->AddDrawPackets(drawPackets);
        }

        void MeshletsFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline, RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (!HasMeshletPasses(renderPipeline))
            {   // This pipeline is not relevant - exist without changing anything.
                return;
            }

            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added
                || changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                m_renderPipeline = renderPipeline;
                CreateResources();
                Init(m_renderPipeline);
            }
            else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                m_renderPipeline = nullptr;
            }
        }
    } // namespace AtomSceneStream
} // namespace AZ
