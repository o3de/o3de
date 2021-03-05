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
    Source/Gestures_precompiled.cpp
    Source/Gestures_precompiled.h
    Include/Gestures/GestureRecognizerClickOrTap.h
    Include/Gestures/GestureRecognizerClickOrTap.inl
    Include/Gestures/GestureRecognizerDrag.h
    Include/Gestures/GestureRecognizerDrag.inl
    Include/Gestures/GestureRecognizerHold.h
    Include/Gestures/GestureRecognizerHold.inl
    Include/Gestures/GestureRecognizerPinch.h
    Include/Gestures/GestureRecognizerPinch.inl
    Include/Gestures/GestureRecognizerRotate.h
    Include/Gestures/GestureRecognizerRotate.inl
    Include/Gestures/GestureRecognizerSwipe.h
    Include/Gestures/GestureRecognizerSwipe.inl
    Include/Gestures/IGestureRecognizer.h
    Source/GesturesModule.cpp
    Source/GesturesSystemComponent.cpp
    Source/GesturesSystemComponent.h
    Source/InputChannelGesture.h
    Source/InputChannelGesture.cpp
    Source/InputChannelGestureClickOrTap.cpp
    Source/InputChannelGestureClickOrTap.h
    Source/InputChannelGestureDrag.cpp
    Source/InputChannelGestureDrag.h
    Source/InputChannelGestureHold.cpp
    Source/InputChannelGestureHold.h
    Source/InputChannelGesturePinch.cpp
    Source/InputChannelGesturePinch.h
    Source/InputChannelGestureRotate.cpp
    Source/InputChannelGestureRotate.h
    Source/InputChannelGestureSwipe.cpp
    Source/InputChannelGestureSwipe.h
    Source/InputDeviceGestures.cpp
    Source/InputDeviceGestures.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    Source/GesturesModule.cpp
)
