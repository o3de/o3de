/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/AuxGeom/AuxGeomFeatureProcessor.h>

#include "AuxGeomDrawQueue.h"
#include "DynamicPrimitiveProcessor.h"
#include "FixedShapeProcessor.h"

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Public/View.h>

#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace Render
    {
        const char* AuxGeomFeatureProcessor::s_featureProcessorName = "AuxGeomFeatureProcessor";

        void AuxGeomFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<AuxGeomFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void AuxGeomFeatureProcessor::Activate()
        {
            AZ::RPI::Scene* scene = GetParentScene();
            // create an AuxGeomDrawQueue object for this scene and register it with the AuxGeomSystemComponent
            m_sceneDrawQueue = RPI::AuxGeomDrawPtr(aznew AuxGeomDrawQueue);

            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();

            // initialize the dynamic primitive processor
            m_dynamicPrimitiveProcessor = AZStd::make_unique<DynamicPrimitiveProcessor>();
            if (!m_dynamicPrimitiveProcessor->Initialize(scene))
            {
                AZ_Error(s_featureProcessorName, false, "Failed to init AuxGeom DynamicPrimitiveProcessor");
                return;
            }

            // initialize the fixed shape processor
            m_fixedShapeProcessor = AZStd::make_unique<FixedShapeProcessor>();
            if (!m_fixedShapeProcessor->Initialize(*rhiSystem->GetDevice(), scene))
            {
                AZ_Error(s_featureProcessorName, false, "Failed to init AuxGeom FixedShapeProcessor");
                return;
            }

            EnableSceneNotification();
        }

        void AuxGeomFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();

            m_viewDrawDataMap.clear();

            m_dynamicPrimitiveProcessor->Release();
            m_dynamicPrimitiveProcessor = nullptr;

            m_fixedShapeProcessor->Release();
            m_fixedShapeProcessor = nullptr;

            // release the AuxGeomDrawQueue object for this scene
            m_sceneDrawQueue = nullptr;
        }

        void AuxGeomFeatureProcessor::Render(const FeatureProcessor::RenderPacket& fpPacket)
        {
            AZ_PROFILE_SCOPE(AzRender, "AuxGeomFeatureProcessor: Render");

            // Get the scene data and switch buffers so that other threads can continue to queue requests
            AuxGeomBufferData* bufferData = static_cast<AuxGeomDrawQueue*>(m_sceneDrawQueue.get())->Commit();

            // Process the dynamic primitives
            m_dynamicPrimitiveProcessor->PrepareFrame();
            m_dynamicPrimitiveProcessor->ProcessDynamicPrimitives(bufferData, fpPacket);

            // Process the objects (draw requests using fixed shape buffers)
            m_fixedShapeProcessor->PrepareFrame();
            m_fixedShapeProcessor->ProcessObjects(bufferData, fpPacket);

            if (m_viewDrawDataMap.size())
            {
                FeatureProcessor::RenderPacket perViewRP;
                perViewRP.m_drawListMask = fpPacket.m_drawListMask;
                for (auto& view : fpPacket.m_views)
                {
                    auto it = m_viewDrawDataMap.find(view.get());
                    if (it != m_viewDrawDataMap.end())
                    {
                        bufferData = static_cast<AuxGeomDrawQueue*>(it->second.get())->Commit();
                        perViewRP.m_views.push_back(view);

                        // Process the dynamic primitives
                        m_dynamicPrimitiveProcessor->ProcessDynamicPrimitives(bufferData, perViewRP);

                        // Process the objects (draw requests using fixed shape buffers)
                        m_fixedShapeProcessor->ProcessObjects(bufferData, perViewRP);

                        perViewRP.m_views.clear();
                    }
                }
            }
        }

        RPI::AuxGeomDrawPtr AuxGeomFeatureProcessor::GetDrawQueueForView(const RPI::View* view)
        {
            if (view)
            {
                auto drawDataIterator = m_viewDrawDataMap.find(view);
                if (drawDataIterator != m_viewDrawDataMap.end())
                {
                    return drawDataIterator->second;
                }
            }
            AZ_Warning("AuxGeomFeatureProcessor", false, "Draw Queue requested for unknown view");
            return RPI::AuxGeomDrawPtr(nullptr);
        }

        RPI::AuxGeomDrawPtr AuxGeomFeatureProcessor::GetOrCreateDrawQueueForView(const RPI::View* view)
        {
            if (!view)
            {
                return RPI::AuxGeomDrawPtr(nullptr);
            }
            auto drawQueueIterator = m_viewDrawDataMap.find(view);

            if (drawQueueIterator == m_viewDrawDataMap.end())
            {
                RPI::AuxGeomDrawPtr drawQueue = RPI::AuxGeomDrawPtr(aznew AuxGeomDrawQueue());
                m_viewDrawDataMap.emplace(view, AZStd::move(drawQueue));
                return m_viewDrawDataMap[view];
            }

            return drawQueueIterator->second;
        }

        void AuxGeomFeatureProcessor::ReleaseDrawQueueForView(const RPI::View* view)
        {
            m_viewDrawDataMap.erase(view);
        }

        void AuxGeomFeatureProcessor::OnSceneRenderPipelinesChanged()
        {
            m_dynamicPrimitiveProcessor->SetUpdatePipelineStates();
            m_fixedShapeProcessor->SetUpdatePipelineStates();
        }

        void AuxGeomFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            OnSceneRenderPipelinesChanged();
        }

        void AuxGeomFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            OnSceneRenderPipelinesChanged();
        }

    } // namespace Render
} // namespace AZ
