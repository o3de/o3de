/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/View.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Rendering/ThumbnailRendererContext.h>
#include <Thumbnails/Rendering/ThumbnailRendererData.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/CaptureStep.h>
#include <AzCore/Math/MatrixUtils.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CaptureStep::CaptureStep(ThumbnailRendererContext* context)
                : ThumbnailRendererStep(context)
            {
            }

            void CaptureStep::Start()
            {
                if (!m_context->GetData()->m_materialAsset ||
                    !m_context->GetData()->m_modelAsset)
                {
                    AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                        m_context->GetData()->m_thumbnailKeyRendered,
                        &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                    m_context->SetStep(Step::FindThumbnailToRender);
                    return;
                }
                Render::MaterialComponentRequestBus::Event(
                    m_context->GetData()->m_modelEntity->GetId(),
                    &Render::MaterialComponentRequestBus::Events::SetDefaultMaterialOverride,
                    m_context->GetData()->m_materialAsset.GetId());
                Render::MeshComponentRequestBus::Event(
                    m_context->GetData()->m_modelEntity->GetId(),
                    &Render::MeshComponentRequestBus::Events::SetModelAsset,
                    m_context->GetData()->m_modelAsset);
                RepositionCamera();
                m_readyToCapture = true;
                m_ticksToCapture = 1;
                TickBus::Handler::BusConnect();
            }

            void CaptureStep::Stop()
            {
                m_context->GetData()->m_renderPipeline->RemoveFromRenderTick();
                TickBus::Handler::BusDisconnect();
                Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            }

            void CaptureStep::RepositionCamera() const
            {
                // Get bounding sphere of the model asset and estimate how far the camera needs to be see all of it
                const Aabb& aabb = m_context->GetData()->m_modelAsset->GetAabb();
                Vector3 modelCenter;
                float radius;
                aabb.GetAsSphere(modelCenter, radius);

                float distance = StartingDistanceMultiplier *
                    GetMax(GetMax(aabb.GetExtents().GetX(), aabb.GetExtents().GetY()), aabb.GetExtents().GetZ()) +
                    DepthNear;
                const Quaternion cameraRotation = Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), StartingRotationAngle);
                Vector3 cameraPosition(modelCenter.GetX(), modelCenter.GetY() - distance, modelCenter.GetZ());
                cameraPosition = cameraRotation.TransformVector(cameraPosition);
                auto cameraTransform = Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
                m_context->GetData()->m_view->SetCameraTransform(Matrix3x4::CreateFromTransform(cameraTransform));
            }

            void CaptureStep::OnTick(float deltaTime, ScriptTimePoint time)
            {
                m_context->GetData()->m_deltaTime = deltaTime;
                m_context->GetData()->m_simulateTime = time.GetSeconds();

                if (m_readyToCapture && m_ticksToCapture-- <= 0)
                {
                    m_context->GetData()->m_renderPipeline->AddToRenderTickOnce();

                    RPI::AttachmentReadback::CallbackFunction readbackCallback = [&](const RPI::AttachmentReadback::ReadbackResult& result)
                    {
                        if (!result.m_dataBuffer)
                        {
                            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                                m_context->GetData()->m_thumbnailKeyRendered,
                                &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
                            return;
                        }
                        uchar* data = result.m_dataBuffer.get()->data();
                        QImage image(
                            data, result.m_imageDescriptor.m_size.m_width, result.m_imageDescriptor.m_size.m_height, QImage::Format_RGBA8888);
                        QPixmap pixmap;
                        pixmap.convertFromImage(image);
                        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(
                            m_context->GetData()->m_thumbnailKeyRendered,
                            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered,
                            pixmap);
                    };

                    Render::FrameCaptureNotificationBus::Handler::BusConnect();
                    bool startedCapture = false;
                    Render::FrameCaptureRequestBus::BroadcastResult(
                        startedCapture,
                        &Render::FrameCaptureRequestBus::Events::CapturePassAttachmentWithCallback,
                        m_context->GetData()->m_passHierarchy, AZStd::string("Output"), readbackCallback);
                    // Reset the capture flag if the capture request was successful. Otherwise try capture it again next tick.
                    if (startedCapture)
                    {
                        m_readyToCapture = false;
                    }
                }
            }

            void CaptureStep::OnCaptureFinished([[maybe_unused]] Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
            {
                m_context->SetStep(Step::FindThumbnailToRender);
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
