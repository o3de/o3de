#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(ENABLED_GEMS
    ImGui
    ScriptEvents
    ExpressionEvaluation
    Gestures
    CertificateManager
    DebugDraw
    SceneProcessing
    GraphCanvas
    InAppPurchases
    AutomatedTesting
    EditorPythonBindings
    QtForPython
    PythonAssetBuilder
    Metastream
    AudioSystem
    Camera
    EMotionFX
    PhysX
    CameraFramework
    StartingPointMovement
    StartingPointCamera
    ScriptCanvas
    ScriptCanvasPhysics
    ScriptCanvasTesting
    LyShineExamples
    StartingPointInput
    PhysXDebug
    WhiteBox
    FastNoise
    SurfaceData
    GradientSignal
    Vegetation
    GraphModel
    LandscapeCanvas
    NvCloth
    Blast
    Maestro
    TextureAtlas
    LmbrCentral
    LyShine
    HttpRequestor
    Atom_AtomBridge
)

# TODO remove conditional add once AWSNativeSDK libs are fixed for Android and Linux Monolithic release.
set(aws_excluded_platforms Linux Android)
if (NOT (LY_MONOLITHIC_GAME AND ${PAL_PLATFORM_NAME} IN_LIST aws_excluded_platforms))
    list(APPEND ENABLED_GEMS
        AWSCore
        AWSClientAuth
        AWSMetrics
    )
endif()
