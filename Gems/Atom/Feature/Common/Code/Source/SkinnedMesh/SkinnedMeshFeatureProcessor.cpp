/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/SkinnedMesh/SkinnedMeshFeatureProcessorBus.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshStatsBus.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshRenderProxy.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <MorphTargets/MorphTargetComputePass.h>
#include <MorphTargets/MorphTargetDispatchItem.h>

#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/RenderPipeline.h>

#include <Atom/RHI/CommandList.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        const char* SkinnedMeshFeatureProcessor::s_featureProcessorName = "SkinnedMeshFeatureProcessor";

        void SkinnedMeshFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SkinnedMeshFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void SkinnedMeshFeatureProcessor::Activate()
        {
            m_statsCollector = AZStd::make_unique<SkinnedMeshStatsCollector>(this);

            EnableSceneNotification();
        }

        void SkinnedMeshFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();

            m_statsCollector = nullptr;

            AZ_Warning("SkinnedMeshFeatureProcessor", m_renderProxies.size() == 0,
                "Deactivaing the SkinnedMeshFeatureProcessor, but there are still outstanding render proxy handles. Components\n"
                "using SkinnedMeshRenderProxy handles should free them before the SkinnedMeshFeatureProcessor is deactivated.\n"
            );

        }

        void SkinnedMeshFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(AzRender, "SkinnedMeshFeatureProcessor: Render");

#if 0 //[GFX_TODO][ATOM-13564] Temporarily disable skinning culling until we figure out how to hook up visibility & lod selection with skinning:
            //Setup the culling workgroup (it will be re-used for each view)
            {
                AZ_PROFILE_SCOPE(AzRender, "set up skinned culling workgroup");
                azsnprintf(m_workgroup.m_name, AZ_ARRAY_SIZE(m_workgroup.m_name), "SkinnedMeshFP workgroup");
                m_workgroup.m_drawListMask.reset();
                m_workgroup.m_cullPackets.clear();
                m_lodPackets.clear();
                m_potentiallyVisibleProxies.clear();

                for (SkinnedMeshRenderProxy& renderProxy : m_renderProxies)
                {
                    renderProxy.m_isQueuedForCompile = false;

                    if (renderProxy.m_inputBuffers->IsUploadPending())
                    {
                        renderProxy.m_inputBuffers->WaitForUpload();
                    }

                    if (renderProxy.m_instance->m_model->IsUploadPending())
                    {
                        renderProxy.m_instance->m_model->WaitForUpload();
                    }

                    //Note: we are creating pointers to the meshDataInstance cullpacket and lod packet here,
                    //and holding them until the skinnedMeshDispatchItems are dispatched. There is an assumption that the underlying
                    //data will not move during this phase.
                    MeshDataInstance& meshDataInstance = **renderProxy.m_meshHandle;
                    m_workgroup.m_cullPackets.push_back(&meshDataInstance.GetCullPacket());
                    m_workgroup.m_drawListMask |= meshDataInstance.GetCullPacket().m_drawListMask;
                    m_lodPackets.push_back(&meshDataInstance.GetLodPacket());
                    m_potentiallyVisibleProxies.push_back(&renderProxy);
                }
            }

            if (m_workgroup.m_cullPackets.size() > 0)
            {
                RPI::CullingSystem* cullingSystem = packet.m_cullingSystem;
                Job* currentJob = JobContext::GetGlobalContext()->GetJobManager().GetCurrentJob();

                //Dispatch the workgroup to each view
                for (const RPI::ViewPtr& viewPtr : packet.m_views)
                {
                    Job* processWorkgroupJob = AZ::CreateJobFunction(
                        [this, cullingSystem, viewPtr](AZ::Job& thisJob)
                        {
                            AZ_PROFILE_SCOPE(AzRender, "skinningMeshFP processWorkgroupJob - View: %s", viewPtr->GetName().GetCStr());

                            auto dispatchSkinningComputeProgramsCallback = [this](AZStd::shared_ptr<RPI::CullingBatchResults> results) -> void
                            {
                                AZ_PROFILE_SCOPE(AzRender, "dispatchSkinningComputePrograms");

                                //the [1][1] element of a projection matrix stores cot(FovY/2) (equal to 2*nearPlaneDistance/nearPlaneHeight),
                                //which is used to determine the (vertical) projected size in screen space            
                                const float yScale = results->m_viewPtr->GetViewToClipMatrix().GetRow(1).GetY();
                                const Vector3 cameraPos = results->m_viewPtr->GetViewToWorldMatrix().GetTranslation();
                                const bool isPerspective = (results->m_viewPtr->GetViewToClipMatrix().GetElement(3, 3) == 0.f);

                                for (size_t v = 0, numVisibleItems = results->m_visibleItems.size(); v < numVisibleItems; ++v)
                                {
                                    uint8_t relativeIndex = results->m_visibleItems[v];
                                    uint32_t itemIndex = results->m_rangeFirst + relativeIndex;
                                    const RPI::LodPacket* lodPacket = m_lodPackets[itemIndex];
                                    Vector3 pos = m_workgroup.m_cullPackets[itemIndex]->m_boundingSphere.GetCenter();
                                    SkinnedMeshRenderProxy* renderProxy = m_potentiallyVisibleProxies[itemIndex];

                                    const float approxScreenPercentage = RPI::ModelLodUtils::ApproxScreenPercentage(
                                        pos, lodPacket->m_lodSelectionRadius, cameraPos, yScale, isPerspective);

                                    for (size_t lodIndex = 0, numLods = lodPacket->m_lods.size(); lodIndex < numLods; ++lodIndex)
                                    {
                                        const RPI::LodPacket::Lod& lod = lodPacket->m_lods[lodIndex];

                                        //Note that this supports overlapping lod ranges (to support cross-fading lods, for example)
                                        float minScreenPercentage(lod.m_range.m_min);
                                        float maxScreenPercentage(lod.m_range.m_max);
                                        if (approxScreenPercentage >= minScreenPercentage && approxScreenPercentage <= maxScreenPercentage)
                                        {
                                            AZStd::lock_guard lock(m_dispatchItemMutex);
                                            m_skinningDispatches.insert(&renderProxy->m_dispatchItemsByLod[lodIndex]->GetRHIDispatchItem());
                                            for (size_t morphTargetIndex = 0; morphTargetIndex < renderProxy->m_morphTargetDispatchItemsByLod[lodIndex].size(); morphTargetIndex++)
                                            {
                                                const MorphTargetDispatchItem* dispatchItem = renderProxy->m_morphTargetDispatchItemsByLod[lodIndex][morphTargetIndex].get();
                                                if (dispatchItem && dispatchItem->GetWeight() > AZ::Constants::FloatEpsilon)
                                                {
                                                    m_morphTargetDispatches.insert(&dispatchItem->GetRHIDispatchItem());
                                                }
                                            }
                                        }
                                    }
                                }
                            };

                            cullingSystem->DispatchCullingWorkgroup(viewPtr, m_workgroup, &thisJob, dispatchSkinningComputeProgramsCallback);
                        },
                        true, nullptr); //auto-deletes

                    currentJob->SetContinuation(processWorkgroupJob);
                    processWorkgroupJob->Start();
                }
            }
#else  //[GFX_TODO][ATOM-13564] This is a temporary implementation that submits all of the skinning compute shaders without any culling:
            for (SkinnedMeshRenderProxy& renderProxy : m_renderProxies)
            {
                renderProxy.m_isQueuedForCompile = false;

                if (renderProxy.m_inputBuffers->IsUploadPending())
                {
                    renderProxy.m_inputBuffers->WaitForUpload();
                }

                if (renderProxy.m_instance->m_model->IsUploadPending())
                {
                    renderProxy.m_instance->m_model->WaitForUpload();
                }

                MeshDataInstance& meshDataInstance = **renderProxy.m_meshHandle;
                const RPI::Cullable& cullable = meshDataInstance.GetCullable();

                for (const RPI::ViewPtr& viewPtr : packet.m_views)
                {
                    RPI::View* view = viewPtr.get();
                    const Matrix4x4& viewToClip = view->GetViewToClipMatrix();

                    //[GFX_TODO][ATOM-13564]:
                    // Option 1)
                    //  store the lastVisibleFrameIndex and lowestLodIndex (or a bitfield of the visible lods) on the Cullable,
                    //  ** run this code *after* culling is done **, use the cached info to decide what to dispatch here
                    // Option 2)
                    //  add a separate visibility entry for each skinned object to the IVisibilitySystem (with a different type flag),
                    //  ensure the entries are kept in sync with the corresponding mesh entry
                    //  do the enumeration for each view, keep track of the lowest lod for each entry,
                    //  and submit the appropriate dispatch item

                    //the [1][1] element of a perspective projection matrix stores cot(FovY/2) (equal to 2*nearPlaneDistance/nearPlaneHeight),
                    //which is used to determine the (vertical) projected size in screen space
                    const float yScale = viewToClip.GetElement(1, 1);
                    const bool isPerspective = viewToClip.GetElement(3, 3) == 0.f;
                    const Vector3 cameraPos = view->GetViewToWorldMatrix().GetTranslation();

                    const Vector3 pos = cullable.m_cullData.m_boundingSphere.GetCenter();

                    const float approxScreenPercentage = RPI::ModelLodUtils::ApproxScreenPercentage(
                        pos, cullable.m_lodData.m_lodSelectionRadius, cameraPos, yScale, isPerspective);

                    for (size_t lodIndex = 0; lodIndex < cullable.m_lodData.m_lods.size(); ++lodIndex)
                    {
                        const RPI::Cullable::LodData::Lod& lod = cullable.m_lodData.m_lods[lodIndex];

                        //Note that this supports overlapping lod ranges (to support cross-fading lods, for example)
                        if (approxScreenPercentage >= lod.m_screenCoverageMin && approxScreenPercentage <= lod.m_screenCoverageMax)
                        {
                            AZStd::lock_guard lock(m_dispatchItemMutex);
                            m_skinningDispatches.insert(&renderProxy.m_dispatchItemsByLod[lodIndex]->GetRHIDispatchItem());
                            for (size_t morphTargetIndex = 0; morphTargetIndex < renderProxy.m_morphTargetDispatchItemsByLod[lodIndex].size(); morphTargetIndex++)
                            {
                                const MorphTargetDispatchItem* dispatchItem = renderProxy.m_morphTargetDispatchItemsByLod[lodIndex][morphTargetIndex].get();
                                if (dispatchItem && dispatchItem->GetWeight() > AZ::Constants::FloatEpsilon)
                                {
                                    m_morphTargetDispatches.insert(&dispatchItem->GetRHIDispatchItem());
                                }
                            }
                        }
                    }
                }
            }
#endif
        }

        void SkinnedMeshFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            InitSkinningAndMorphPass(pipeline.get());
        }

        void SkinnedMeshFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            InitSkinningAndMorphPass(renderPipeline);
        }

        void SkinnedMeshFeatureProcessor::OnBeginPrepareRender()
        {
            m_renderProxiesChecker.soft_lock();

            SkinnedMeshFeatureProcessorNotificationBus::Broadcast(&SkinnedMeshFeatureProcessorNotificationBus::Events::OnUpdateSkinningMatrices);
        }

        void SkinnedMeshFeatureProcessor::OnRenderEnd()
        {
            m_renderProxiesChecker.soft_unlock();

            // Clear any dispatch items that were added but never submitted
            // in case there were no passes that submitted this frame
            // because they execute at a lower frequency
            m_skinningDispatches.clear();
            m_morphTargetDispatches.clear();
        }

        SkinnedMeshRenderProxyHandle SkinnedMeshFeatureProcessor::AcquireRenderProxy(const SkinnedMeshRenderProxyDesc& desc)
        {
            // don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during insertion
            SkinnedMeshRenderProxyHandle handle = m_renderProxies.emplace(desc);
            if (!handle->Init(*GetParentScene(), this))
            {
                m_renderProxies.erase(handle);
            }
            return handle;
        }

        bool SkinnedMeshFeatureProcessor::ReleaseRenderProxy(SkinnedMeshRenderProxyHandle& handle)
        {
            if (handle.IsValid())
            {
                AZStd::concurrency_check_scope scopeCheck(m_renderProxiesChecker);
                m_renderProxies.erase(handle);
                return true;
            }
            return false;
        }

        void SkinnedMeshFeatureProcessor::InitSkinningAndMorphPass(RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter skinPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name{ "SkinningPass" }, renderPipeline);
            RPI::Ptr<RPI::Pass> skinningPass = RPI::PassSystemInterface::Get()->FindFirstPass(skinPassFilter);
            if (skinningPass)
            {
                SkinnedMeshComputePass* skinnedMeshComputePass = azdynamic_cast<SkinnedMeshComputePass*>(skinningPass.get());
                skinnedMeshComputePass->SetFeatureProcessor(this);

                // There may be multiple skinning passes in the scene due to multiple pipelines, but there is only one skinning shader
                m_skinningShader = skinnedMeshComputePass->GetShader();

                if (!m_skinningShader)
                {
                    AZ_Error(s_featureProcessorName, false, "Failed to get skinning pass shader. It may need to finish processing.");
                }
                else
                {
                    m_cachedSkinningShaderOptions.SetShader(m_skinningShader);
                }
            }

            RPI::PassFilter morphPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name{ "MorphTargetPass" }, renderPipeline);
            RPI::Ptr<RPI::Pass> morphTargetPass = RPI::PassSystemInterface::Get()->FindFirstPass(morphPassFilter);
            if (morphTargetPass)
            {
                MorphTargetComputePass* morphTargetComputePass = azdynamic_cast<MorphTargetComputePass*>(morphTargetPass.get());
                morphTargetComputePass->SetFeatureProcessor(this);

                // There may be multiple morph target passes in the scene due to multiple pipelines, but there is only one morph target shader
                m_morphTargetShader = morphTargetComputePass->GetShader();

                if (!m_morphTargetShader)
                {
                    AZ_Error(s_featureProcessorName, false, "Failed to get morph target pass shader. It may need to finish processing.");
                }
            }
        }

        RPI::ShaderOptionGroup SkinnedMeshFeatureProcessor::CreateSkinningShaderOptionGroup(const SkinnedMeshShaderOptions shaderOptions, SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedHandler)
        {
            m_cachedSkinningShaderOptions.ConnectToShaderReinitializedEvent(shaderReinitializedHandler);
            return m_cachedSkinningShaderOptions.CreateShaderOptionGroup(shaderOptions);
        }

        void SkinnedMeshFeatureProcessor::OnSkinningShaderReinitialized(const Data::Instance<RPI::Shader> skinningShader)
        {
            m_skinningShader = skinningShader;
            m_cachedSkinningShaderOptions.SetShader(m_skinningShader);
        }

        void SkinnedMeshFeatureProcessor::SubmitSkinningDispatchItems(RHI::CommandList* commandList)
        {
            AZStd::lock_guard lock(m_dispatchItemMutex);
            for (const RHI::DispatchItem* dispatchItem : m_skinningDispatches)
            {
                commandList->Submit(*dispatchItem);
            }
            m_skinningDispatches.clear();
        }

        void SkinnedMeshFeatureProcessor::SubmitMorphTargetDispatchItems(RHI::CommandList* commandList)
        {
            AZStd::lock_guard lock(m_dispatchItemMutex);
            for (const RHI::DispatchItem* dispatchItem : m_morphTargetDispatches)
            {
                commandList->Submit(*dispatchItem);
            }
            m_morphTargetDispatches.clear();
        }

        SkinnedMeshRenderProxyInterfaceHandle SkinnedMeshFeatureProcessor::AcquireRenderProxyInterface(const SkinnedMeshRenderProxyDesc& desc)
        {
            return AcquireRenderProxy(desc);
        }

        bool SkinnedMeshFeatureProcessor::ReleaseRenderProxyInterface(SkinnedMeshRenderProxyInterfaceHandle& interfaceHandle)
        {
            SkinnedMeshRenderProxyHandle handle(AZStd::move(interfaceHandle));
            return ReleaseRenderProxy(handle);
        }

        Data::Instance<RPI::Shader> SkinnedMeshFeatureProcessor::GetSkinningShader() const
        {
            return m_skinningShader;
        }

        Data::Instance<RPI::Shader> SkinnedMeshFeatureProcessor::GetMorphTargetShader() const
        {
            return m_morphTargetShader;
        }
    } // namespace Render
} // namespace AZ
