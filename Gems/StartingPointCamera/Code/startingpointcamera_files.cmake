#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    Source/StartingPointCamera_precompiled.cpp
    Source/StartingPointCamera_precompiled.h
)
