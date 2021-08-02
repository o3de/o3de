#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/StartingPointCamera/StartingPointCameraConstants.h
    Include/StartingPointCamera/StartingPointCameraUtilities.h
    Source/StartingPointCamera/StartingPointCameraUtilities.cpp
    Source/CameraTargetAcquirers/AcquireByEntityId.h
    Source/CameraTargetAcquirers/AcquireByEntityId.cpp
    Source/CameraTargetAcquirers/AcquireByTag.h
    Source/CameraTargetAcquirers/AcquireByTag.cpp
    Source/CameraLookAtBehaviors/OffsetPosition.h
    Source/CameraLookAtBehaviors/OffsetPosition.cpp
    Source/CameraLookAtBehaviors/SlideAlongAxisBasedOnAngle.h
    Source/CameraLookAtBehaviors/SlideAlongAxisBasedOnAngle.cpp
    Source/CameraLookAtBehaviors/RotateCameraLookAt.h
    Source/CameraLookAtBehaviors/RotateCameraLookAt.cpp
    Source/CameraTransformBehaviors/FaceTarget.h
    Source/CameraTransformBehaviors/FaceTarget.cpp
    Source/CameraTransformBehaviors/FollowTargetFromDistance.h
    Source/CameraTransformBehaviors/FollowTargetFromDistance.cpp
    Source/CameraTransformBehaviors/FollowTargetFromAngle.h
    Source/CameraTransformBehaviors/FollowTargetFromAngle.cpp
    Source/CameraTransformBehaviors/OffsetCameraPosition.h
    Source/CameraTransformBehaviors/OffsetCameraPosition.cpp
    Source/CameraTransformBehaviors/Rotate.h
    Source/CameraTransformBehaviors/Rotate.cpp
)
