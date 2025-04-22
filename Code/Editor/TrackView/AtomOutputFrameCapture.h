/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <AzFramework/Components/CameraBus.h>

namespace TrackView
{
    //! Provides functionality to capture frames from the "MainCamera".
    //! A new pipeline is created (and associated with the scene provided), a callback can be
    //! provided to handle the attachment readback (what to do with the captured frame) and also
    //! what to do after an individual capture fully completes (called in OnCaptureFinished).
    class AtomOutputFrameCapture : private AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        AtomOutputFrameCapture() = default;

        using CaptureFinishedCallback = AZStd::function<void()>;

        //! Create a new pipeline associated with a given scene.
        //! @note "MainCamera" is the view that is captured.
        void CreatePipeline(AZ::RPI::Scene& scene, const AZStd::string& pipelineName, uint32_t width, uint32_t height);
        //! Removes the pipeline from the scene provided and then destroys it.
        //! @note scene must be the same scene used to create the pipeline.
        void DestroyPipeline(AZ::RPI::Scene& scene);

        bool IsCreated() const { return m_pipelineCreated; }

        //! Request a capture to start.
        //! @param attachmentReadbackCallback Handles the returned attachment (image data returned by the renderer).
        //! @param captureFinishedCallback Logic to run once the capture has completed fully.
        bool BeginCapture(
            const AZ::RPI::AttachmentReadback::CallbackFunction& attachmentReadbackCallback,
            CaptureFinishedCallback captureFinishedCallback);

        //! Update the internal view that is associated with the created pipeline.
        void UpdateView(const AZ::Matrix3x4& cameraTransform, const AZ::Matrix4x4& cameraProjection, const AZ::RPI::ViewPtr targetView = nullptr);

    private:
        AZ::RPI::RenderPipelinePtr m_renderPipeline; //!< The internal render pipeline.
        AZ::RPI::ViewPtr m_view; //!< The view associated with the render pipeline.
        AZ::RPI::ViewPtr m_targetView; //!< The view that this render pipeline will mimic.
        AZStd::vector<AZStd::string> m_passHierarchy; //!< Pass hierarchy (includes pipelineName and CopyToSwapChain).
        CaptureFinishedCallback m_captureFinishedCallback; //!< Stored callback called from OnCaptureFinished.

        bool m_pipelineCreated{};

        // FrameCaptureNotificationBus overrides ...
        void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;
    };

    inline AZ::EntityId ActiveCameraEntityId()
    {
        AZ::EntityId activeCameraId;
        Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequests::GetActiveCamera);
        return activeCameraId;
    }

    //! Returns the transform for the given EntityId.
    AZ::Matrix3x4 TransformFromEntityId(AZ::EntityId entityId);

    //! Returns the projection matrix for the given camera EntityId.
    //! @note Must provide a valid camera entity.
    AZ::Matrix4x4 ProjectionFromCameraEntityId(AZ::EntityId entityId, float outputWidth, float outputHeight);

    //! Helper to return the GameEntityContext scene.
    AZ::RPI::Scene* SceneFromGameEntityContext();
} // namespace TrackView
