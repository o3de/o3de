/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ViewportScreen.h"

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AzFramework
{
    // a transform that maps from:
    // z-up, y into the screen, x-right
    // y-up, z into the screen, x-left
    //
    // x -> -x
    // y ->  z
    // z ->  y
    //
    // the same transform can be used to go to/from z-up - the only difference is the order of
    // multiplication which must be used (see CameraTransformFromCameraView and CameraViewFromCameraTransform)
    // note: coordinate system convention is right handed
    //       see Matrix4x4::CreateProjection for more details
    static AZ::Matrix3x4 ZYCoordinateSystemConversion()
    {
        // note: the below matrix is the result of these combined transformations
        //      pitch = AZ::Matrix4x4::CreateRotationX(AZ::DegToRad(-90.0f));
        //      yaw = AZ::Matrix4x4::CreateRotationZ(AZ::DegToRad(180.0f));
        //      conversion = pitch * yaw
        return AZ::Matrix3x4::CreateFromColumns(
            AZ::Vector3(-1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 0.0f));
    }

    AZ::Matrix3x4 CameraTransform(const CameraState& cameraState)
    {
        return AZ::Matrix3x4::CreateFromColumns(cameraState.m_side, cameraState.m_forward, cameraState.m_up, cameraState.m_position);
    }

    AZ::Matrix3x4 CameraView(const CameraState& cameraState)
    {
        // ensure the camera is looking down positive z with the x axis pointing left
        return ZYCoordinateSystemConversion() * CameraTransform(cameraState).GetInverseFast();
    }

    AZ::Matrix3x4 InverseCameraView(const CameraState& cameraState)
    {
        // ensure the camera is looking down positive z with the x axis pointing left
        return CameraView(cameraState).GetInverseFast();
    }

    AZ::Matrix4x4 CameraProjection(const CameraState& cameraState)
    {
        return AZ::Matrix4x4::CreateProjection(
            cameraState.VerticalFovRadian(), AspectRatio(cameraState.m_viewportSize), cameraState.m_nearClip, cameraState.m_farClip);
    }

    AZ::Matrix4x4 InverseCameraProjection(const CameraState& cameraState)
    {
        return CameraProjection(cameraState).GetInverseFull();
    }

    AZ::Matrix3x4 CameraTransformFromCameraView(const AZ::Matrix3x4& cameraView)
    {
        return (ZYCoordinateSystemConversion() * cameraView).GetInverseFast();
    }

    AZ::Matrix3x4 CameraViewFromCameraTransform(const AZ::Matrix3x4& cameraTransform)
    {
        return ZYCoordinateSystemConversion() * cameraTransform.GetInverseFast();
    }

    AZ::Frustum FrustumFromCameraState(const CameraState& cameraState)
    {
        return AZ::Frustum(ViewFrustumAttributesFromCameraState(cameraState));
    }

    AZ::ViewFrustumAttributes ViewFrustumAttributesFromCameraState(const CameraState& cameraState)
    {
        const auto worldFromView = AzFramework::CameraTransform(cameraState);
        const auto cameraWorldTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateFromMatrix3x4(worldFromView), worldFromView.GetTranslation());
        return AZ::ViewFrustumAttributes(
            cameraWorldTransform, AspectRatio(cameraState.m_viewportSize), cameraState.m_fovOrZoom, cameraState.m_nearClip,
            cameraState.m_farClip);
    }

    AZ::Vector3 WorldToScreenNdc(const AZ::Vector3& worldPosition, const AZ::Matrix3x4& cameraView, const AZ::Matrix4x4& cameraProjection)
    {
        // transform the world space position to clip space
        const auto clipSpacePosition =
            cameraProjection * AZ::Vector3ToVector4(cameraView.TransformPoint(worldPosition), 1.0f);
        // transform the clip space position to ndc space (perspective divide)
        const auto ndcPosition = clipSpacePosition / clipSpacePosition.GetW();
        // transform ndc space from <-1,1> to <0, 1> range
        return (AZ::Vector4ToVector3(ndcPosition) + AZ::Vector3::CreateOne()) * 0.5f;
    }

    ScreenPoint WorldToScreen(
        const AZ::Vector3& worldPosition,
        const AZ::Matrix3x4& cameraView,
        const AZ::Matrix4x4& cameraProjection,
        const AZ::Vector2& viewportSize)
    {
        // scale ndc position by screen dimensions to return screen position
        return ScreenPointFromNdc(AZ::Vector3ToVector2(WorldToScreenNdc(worldPosition, cameraView, cameraProjection)), viewportSize);
    }

    ScreenPoint WorldToScreen(const AZ::Vector3& worldPosition, const CameraState& cameraState)
    {
        return WorldToScreen(worldPosition, CameraView(cameraState), CameraProjection(cameraState), cameraState.m_viewportSize);
    }

    AZ::Vector3 ScreenNdcToWorld(
        const AZ::Vector2& normalizedScreenPosition, const AZ::Matrix3x4& inverseCameraView, const AZ::Matrix4x4& inverseCameraProjection)
    {
        // convert screen space coordinates from <0, 1> to <-1,1> range
        const auto ndcPosition = normalizedScreenPosition * 2.0f - AZ::Vector2::CreateOne();

        // transform ndc space position to clip space
        const auto clipSpacePosition = inverseCameraProjection * Vector2ToVector4(ndcPosition, -1.0f, 1.0f);
        // transform clip space to camera space
        const auto cameraSpacePosition = AZ::Vector4ToVector3(clipSpacePosition) / clipSpacePosition.GetW();
        // transform camera space to world space
        const auto worldPosition = inverseCameraView * cameraSpacePosition;

        return worldPosition;
    }

    AZ::Vector3 ScreenToWorld(
        const ScreenPoint& screenPosition,
        const AZ::Matrix3x4& inverseCameraView,
        const AZ::Matrix4x4& inverseCameraProjection,
        const AZ::Vector2& viewportSize)
    {
        return ScreenNdcToWorld(NdcFromScreenPoint(screenPosition, viewportSize), inverseCameraView, inverseCameraProjection);
    }

    AZ::Vector3 ScreenToWorld(const ScreenPoint& screenPosition, const CameraState& cameraState)
    {
        return ScreenToWorld(
            screenPosition, InverseCameraView(cameraState), InverseCameraProjection(cameraState), cameraState.m_viewportSize);
    }
} // namespace AzFramework
