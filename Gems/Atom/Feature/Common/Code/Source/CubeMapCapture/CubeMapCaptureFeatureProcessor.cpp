/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CubeMapCapture/CubeMapCaptureFeatureProcessor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        void CubeMapCaptureFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<CubeMapCaptureFeatureProcessor, FeatureProcessor>()
                    ->Version(1);
            }
        }

        void CubeMapCaptureFeatureProcessor::Activate()
        {
            m_cubeMapCaptures.reserve(InitialProbeAllocationSize);

            EnableSceneNotification();
        }

        void CubeMapCaptureFeatureProcessor::Deactivate()
        {
            AZ_Warning("CubeMapCaptureFeatureProcessor", m_cubeMapCaptures.size() == 0, 
                "Deactivating the CubeMapCaptureFeatureProcessor but there are still outstanding CubeMapCaptures. Components\n"
                "using CubeMapCaptureHandles should free them before the CubeMapCaptureFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();
        }

        void CubeMapCaptureFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(AzRender, "CubeMapCaptureFeatureProcessor: Simulate");

            // call Simulate on all cubeMapCaptures
            for (auto& cubeMapCapture : m_cubeMapCaptures)
            {
                AZ_Assert(cubeMapCapture.use_count() > 1, "CubeMapCapture found with no corresponding owner, ensure that RemoveCubeMapCapture() is called before releasing CubeMapCapture handles");
                cubeMapCapture->Simulate();
            }
        }

        void CubeMapCaptureFeatureProcessor::OnRenderEnd()
        {
            // call OnRenderEnd on all cubeMapCaptures
            for (auto& cubeMapCapture : m_cubeMapCaptures)
            {
                AZ_Assert(cubeMapCapture.use_count() > 1, "CubeMapCapture found with no corresponding owner, ensure that RemoveCubeMapCapture() is called before releasing CubeMapCapture handles");
                cubeMapCapture->OnRenderEnd();
            }
        }

        CubeMapCaptureHandle CubeMapCaptureFeatureProcessor::AddCubeMapCapture(const AZ::Transform& transform)
        {
            AZStd::shared_ptr<CubeMapCapture> cubeMapCapture = AZStd::make_shared<CubeMapCapture>();
            cubeMapCapture->Init(GetParentScene());
            cubeMapCapture->SetTransform(transform);
            m_cubeMapCaptures.push_back(cubeMapCapture);
            
            return cubeMapCapture;
        }

        void CubeMapCaptureFeatureProcessor::RemoveCubeMapCapture(CubeMapCaptureHandle& cubeMapCapture)
        {
            AZ_Assert(cubeMapCapture.get(), "RemoveCubeMapCapture called with an invalid handle");

            auto itEntry = AZStd::find_if(m_cubeMapCaptures.begin(), m_cubeMapCaptures.end(), [&](AZStd::shared_ptr<CubeMapCapture> const& entry)
            {
                return (entry == cubeMapCapture);
            });

            AZ_Assert(itEntry != m_cubeMapCaptures.end(), "RemoveCubeMapCapture called with a CubeMapCapture that is not in the CubeMapCapture list");
            m_cubeMapCaptures.erase(itEntry);
        }

        void CubeMapCaptureFeatureProcessor::SetTransform(const CubeMapCaptureHandle& cubeMapCapture, const AZ::Transform& transform)
        {
            AZ_Assert(cubeMapCapture.get(), "SetTransform called with an invalid handle");
            cubeMapCapture->SetTransform(transform);
        }

        void CubeMapCaptureFeatureProcessor::RenderCubeMap(const CubeMapCaptureHandle& cubeMapCapture, RenderCubeMapCallback callback, const AZStd::string& relativePath)
        {
            AZ_Assert(cubeMapCapture.get(), "RenderCubeMap called with an invalid handle");
            cubeMapCapture->RenderCubeMap(callback, relativePath);
        }

        bool CubeMapCaptureFeatureProcessor::IsCubeMapReferenced(const AZStd::string& relativePath)
        {
            for (auto& cubeMapCapture : m_cubeMapCaptures)
            {
                if (cubeMapCapture->GetRelativePath() == relativePath)
                {
                    return true;
                }
            }

            return false;
        }

        void CubeMapCaptureFeatureProcessor::SetExposure(const CubeMapCaptureHandle& cubeMapCapture, float exposure)
        {
            AZ_Assert(cubeMapCapture.get(), "SetExposure called with an invalid handle");
            cubeMapCapture->SetExposure(exposure);
        }

        void CubeMapCaptureFeatureProcessor::SetRelativePath(const CubeMapCaptureHandle& cubeMapCapture, const AZStd::string& relativePath)
        {
            AZ_Assert(cubeMapCapture.get(), "SetRelativePath called with an invalid handle");
            cubeMapCapture->SetRelativePath(relativePath);
        }

        void CubeMapCaptureFeatureProcessor::OnRenderPipelineChanged(RPI::RenderPipeline* renderPipeline,
                RPI::SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::PassChanged)
            {
                for (auto& cubeMapCapture : m_cubeMapCaptures)
                {
                    cubeMapCapture->OnRenderPipelinePassesChanged(renderPipeline);
                }
            }
        }
    } // namespace Render
} // namespace AZ
