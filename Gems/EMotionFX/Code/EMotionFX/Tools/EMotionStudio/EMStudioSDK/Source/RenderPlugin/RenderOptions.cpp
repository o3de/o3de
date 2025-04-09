/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Integration/Rendering/RenderActorSettings.h>
#include <MysticQt/Source/MysticQtConfig.h>

#include <QColor>
#include <QSettings>

namespace EMStudio
{
    const char* RenderOptions::s_gridUnitSizeOptionName = "gridUnitSize";
    const char* RenderOptions::s_vertexNormalsScaleOptionName = "vertexNormalsScale";
    const char* RenderOptions::s_faceNormalsScaleOptionName = "faceNormalsScale";
    const char* RenderOptions::s_tangentsScaleOptionName = "tangentsScale";
    const char* RenderOptions::s_nodeOrientationScaleOptionName = "nodeOrientationScale";
    const char* RenderOptions::s_scaleBonesOnLengthOptionName = "scaleBonesOnLength";
    const char* RenderOptions::s_nearClipPlaneDistanceOptionName = "nearClipPlaneDistance";
    const char* RenderOptions::s_farClipPlaneDistanceOptionName = "farClipPlaneDistance";
    const char* RenderOptions::s_FOVOptionName = "fieldOfView";
    const char* RenderOptions::s_mainLightIntensityOptionName = "mainLightIntensity";
    const char* RenderOptions::s_mainLightAngleAOptionName = "mainLightAngleA";
    const char* RenderOptions::s_mainLightAngleBOptionName = "mainLightAngleB";
    const char* RenderOptions::s_specularIntensityOptionName = "specularIntensity";
    const char* RenderOptions::s_rimIntensityOptionName = "rimIntensity";
    const char* RenderOptions::s_rimWidthOptionName = "rimWidth";
    const char* RenderOptions::s_rimAngleOptionName = "rimAngle";
    const char* RenderOptions::s_showFPSOptionName = "showFPS";
    const char* RenderOptions::s_lightGroundColorOptionName = "lightGroundColor";
    const char* RenderOptions::s_lightSkyColorOptionName = "lightSkyColor_v2";
    const char* RenderOptions::s_rimColorOptionName = "rimColor_v2";
    const char* RenderOptions::s_backgroundColorOptionName = "backgroundColor";
    const char* RenderOptions::s_gradientSourceColorOptionName = "gradientSourceColor_v2";
    const char* RenderOptions::s_gradientTargetColorOptionName = "gradientTargetColor";
    const char* RenderOptions::s_wireframeColorOptionName = "wireframeColor";
    const char* RenderOptions::s_collisionMeshColorOptionName = "collisionMeshColor";
    const char* RenderOptions::s_vertexNormalsColorOptionName = "vertexNormalsColor";
    const char* RenderOptions::s_faceNormalsColorOptionName = "faceNormalsColor";
    const char* RenderOptions::s_tangentsColorOptionName = "tangentsColor";
    const char* RenderOptions::s_mirroredBitangentsColorOptionName = "mirroredBitangentsColor";
    const char* RenderOptions::s_bitangentsColorOptionName = "bitangentsColor";
    const char* RenderOptions::s_nodeAABBColorOptionName = "nodeAABBColor";
    const char* RenderOptions::s_staticAABBColorOptionName = "staticAABBColor";
    const char* RenderOptions::s_meshAABBColorOptionName = "meshAABBColor";
    const char* RenderOptions::s_lineSkeletonColorOptionName = "lineSkeletonColor_v2";
    const char* RenderOptions::s_skeletonColorOptionName = "skeletonColor";
    const char* RenderOptions::s_selectionColorOptionName = "selectionColor";
    const char* RenderOptions::s_selectedObjectColorOptionName = "selectedObjectColor";
    const char* RenderOptions::s_nodeNameColorOptionName = "nodeNameColor";
    const char* RenderOptions::s_gridColorOptionName = "gridColor";
    const char* RenderOptions::s_mainAxisColorOptionName = "gridMainAxisColor";
    const char* RenderOptions::s_subStepColorOptionName = "gridSubStepColor";
    const char* RenderOptions::s_trajectoryArrowInnerColorOptionName = "trajectoryArrowInnerColor";
    const char* RenderOptions::s_hitDetectionColliderColorOptionName = "hitDetectionColliderColor_v2";
    const char* RenderOptions::s_selectedHitDetectionColliderColorOptionName = "selectedHitDetectionColliderColor_v2";
    const char* RenderOptions::s_ragdollColliderColorOptionName = "ragdollColliderColor_v2";
    const char* RenderOptions::s_selectedRagdollColliderColorOptionName = "selectedRagdollColliderColor_v2";
    const char* RenderOptions::s_violatedJointLimitColorOptionName = "violatedJointLimitColor";
    const char* RenderOptions::s_clothColliderColorOptionName = "clothColliderColor";
    const char* RenderOptions::s_selectedClothColliderColorOptionName = "selectedClothColliderColor_v2";
    const char* RenderOptions::s_lastUsedLayoutOptionName = "lastUsedLayout";
    const char* RenderOptions::s_renderSelectionBoxOptionName = "renderSelectionBox";

    // constructor
    RenderOptions::RenderOptions()
        : m_gridUnitSize(0.2f)
        , m_vertexNormalsScale(1.0f)
        , m_faceNormalsScale(1.0f)
        , m_tangentsScale(1.0f)
        , m_nodeOrientationScale(1.0f)
        , m_scaleBonesOnLength(true)
        , m_nearClipPlaneDistance(0.1f)
        , m_farClipPlaneDistance(200.0f)
        , m_fov(55.0f)
        , m_mainLightIntensity(1.0f)
        , m_mainLightAngleA(0.0f)
        , m_mainLightAngleB(0.0f)
        , m_specularIntensity(1.0f)
        , m_rimIntensity(1.5f)
        , m_rimWidth(0.65f)
        , m_rimAngle(60.0f)
        , m_showFPS(false)
        , m_lightGroundColor(0.117f, 0.015f, 0.07f, 1.0f)
        , m_lightSkyColor(AZ::Color::CreateFromRgba(127, 127, 127, 255))
        , m_rimColor(AZ::Color::CreateFromRgba(208, 208, 208, 255))
        , m_backgroundColor(0.359f, 0.3984f, 0.4492f, 1.0f)
        , m_gradientSourceColor(AZ::Color::CreateFromRgba(64, 71, 75, 255))
        , m_gradientTargetColor(0.0941f, 0.1019f, 0.1098f, 1.0f)
        , m_wireframeColor(0.0f, 0.0f, 0.0f, 1.0f)
        , m_collisionMeshColor(0.0f, 1.0f, 1.0f, 1.0f)
        , m_vertexNormalsColor(0.0f, 1.0f, 0.0f, 1.0f)
        , m_faceNormalsColor(0.5f, 0.5f, 1.0f, 1.0f)
        , m_tangentsColor(1.0f, 0.0f, 0.0f, 1.0f)
        , m_mirroredBitangentsColor(1.0f, 1.0f, 0.0f, 1.0f)
        , m_bitangentsColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_nodeAABBColor(1.0f, 0.0f, 0.0f, 1.0f)
        , m_staticAABBColor(0.0f, 0.7f, 0.7f, 1.0f)
        , m_meshAABBColor(0.0f, 0.0f, 0.7f, 1.0f)
        , m_lineSkeletonColor(0.33333f, 1.0f, 0.0f, 1.0f)
        , m_skeletonColor(0.19f, 0.58f, 0.19f, 1.0f)
        , m_selectionColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_selectedObjectColor(1.0f, 0.647f, 0.0f, 1.0f)
        , m_nodeNameColor(1.0f, 1.0f, 1.0f, 1.0f)
        , m_gridColor(0.3242f, 0.3593f, 0.40625f, 1.0f)
        , m_mainAxisColor(0.0f, 0.01f, 0.04f, 1.0f)
        , m_subStepColor(0.2460f, 0.2851f, 0.3320f, 1.0f)
        , m_trajectoryArrowInnerColor(0.184f, 0.494f, 0.866f, 1.0f)
        , m_hitDetectionColliderColor(AZ::Color::CreateFromRgba(112, 112, 112, 255))
        , m_selectedHitDetectionColliderColor(AZ::Color::CreateFromRgba(74, 144, 226, 255))
        , m_ragdollColliderColor(AZ::Color::CreateFromRgba(112, 112, 112, 255))
        , m_selectedRagdollColliderColor(AZ::Color::CreateFromRgba(245, 166, 35, 255))
        , m_violatedJointLimitColor(AZ::Color::CreateFromRgba(255, 0, 0, 255))
        , m_clothColliderColor(AZ::Color::CreateFromRgba(112, 112, 112, 255))
        , m_selectedClothColliderColor(AZ::Color::CreateFromRgba(155, 117, 255, 255))
        , m_simulatedObjectColliderColor(AZ::Color::CreateFromRgba(112, 112, 112, 255))
        , m_selectedSimulatedObjectColliderColor(AZ::Color::CreateFromRgba(255, 86, 222, 255))
        , m_lastUsedLayout("Single")
        , m_renderSelectionBox(true)
    {
    }

    RenderOptions& RenderOptions::operator=(const RenderOptions& other)
    {
        SetGridUnitSize(other.GetGridUnitSize());
        SetVertexNormalsScale(other.GetVertexNormalsScale());
        SetFaceNormalsScale(other.GetFaceNormalsScale());
        SetTangentsScale(other.GetTangentsScale());
        SetNodeOrientationScale(other.GetNodeOrientationScale());
        SetScaleBonesOnLenght(other.GetScaleBonesOnLength());
        SetMainLightIntensity(other.GetMainLightIntensity());
        SetMainLightAngleA(other.GetMainLightAngleA());
        SetMainLightAngleB(other.GetMainLightAngleB());
        SetSpecularIntensity(other.GetSpecularIntensity());
        SetRimIntensity(other.GetRimIntensity());
        SetRimWidth(other.GetRimWidth());
        SetRimAngle(other.GetRimAngle());
        SetShowFPS(other.GetShowFPS());
        SetLightGroundColor(other.GetLightGroundColor());
        SetLightSkyColor(other.GetLightSkyColor());
        SetRimColor(other.GetRimColor());
        SetBackgroundColor(other.GetBackgroundColor());
        SetGradientSourceColor(other.GetGradientSourceColor());
        SetGradientTargetColor(other.GetGradientTargetColor());
        SetWireframeColor(other.GetWireframeColor());
        SetCollisionMeshColor(other.GetCollisionMeshColor());
        SetVertexNormalsColor(other.GetVertexNormalsColor());
        SetFaceNormalsColor(other.GetFaceNormalsColor());
        SetTangentsColor(other.GetTangentsColor());
        SetMirroredBitangentsColor(other.GetMirroredBitangentsColor());
        SetBitangentsColor(other.GetBitangentsColor());
        SetNodeAABBColor(other.GetNodeAABBColor());
        SetStaticAABBColor(other.GetStaticAABBColor());
        SetMeshAABBColor(other.GetMeshAABBColor());
        SetLineSkeletonColor(other.GetLineSkeletonColor());
        SetSkeletonColor(other.GetSkeletonColor());
        SetSelectionColor(other.GetSelectionColor());
        SetSelectedObjectColor(other.GetSelectedObjectColor());
        SetNodeNameColor(other.GetNodeNameColor());
        SetGridColor(other.GetGridColor());
        SetMainAxisColor(other.GetMainAxisColor());
        SetSubStepColor(other.GetSubStepColor());
        SetTrajectoryArrowInnerColor(other.GetTrajectoryArrowInnerColor());
        SetLastUsedLayout(other.GetLastUsedLayout());
        SetRenderSelectionBox(other.GetRenderSelectionBox());
        SetNearClipPlaneDistance(other.GetNearClipPlaneDistance());
        SetFarClipPlaneDistance(other.GetFarClipPlaneDistance());
        SetFOV(other.GetFOV());
        SetRenderFlags(other.GetRenderFlags());
        SetManipulatorMode(other.GetManipulatorMode());
        SetCameraViewMode(other.GetCameraViewMode());
        SetCameraFollowUp(other.GetCameraFollowUp());
        return *this;
    }

    RenderOptions::~RenderOptions()
    {
    }

    void RenderOptions::Save(QSettings* settings)
    {
        settings->setValue(s_backgroundColorOptionName, ColorToString(m_backgroundColor));
        settings->setValue(s_gradientSourceColorOptionName, ColorToString(m_gradientSourceColor));
        settings->setValue(s_gradientTargetColorOptionName, ColorToString(m_gradientTargetColor));
        settings->setValue(s_wireframeColorOptionName, ColorToString(m_wireframeColor));
        settings->setValue(s_vertexNormalsColorOptionName, ColorToString(m_vertexNormalsColor));
        settings->setValue(s_faceNormalsColorOptionName, ColorToString(m_faceNormalsColor));
        settings->setValue(s_tangentsColorOptionName, ColorToString(m_tangentsColor));
        settings->setValue(s_mirroredBitangentsColorOptionName, ColorToString(m_mirroredBitangentsColor));
        settings->setValue(s_bitangentsColorOptionName, ColorToString(m_bitangentsColor));
        settings->setValue(s_nodeAABBColorOptionName, ColorToString(m_nodeAABBColor));
        settings->setValue(s_staticAABBColorOptionName, ColorToString(m_staticAABBColor));
        settings->setValue(s_meshAABBColorOptionName, ColorToString(m_meshAABBColor));
        settings->setValue(s_collisionMeshColorOptionName, ColorToString(m_collisionMeshColor));
        settings->setValue(s_lineSkeletonColorOptionName, ColorToString(m_lineSkeletonColor));
        settings->setValue(s_skeletonColorOptionName, ColorToString(m_skeletonColor));
        settings->setValue(s_selectionColorOptionName, ColorToString(m_selectionColor));
        settings->setValue(s_selectedObjectColorOptionName, ColorToString(m_selectedObjectColor));
        settings->setValue(s_nodeNameColorOptionName, ColorToString(m_nodeNameColor));

        settings->setValue(s_gridColorOptionName, ColorToString(m_gridColor));
        settings->setValue(s_mainAxisColorOptionName, ColorToString(m_mainAxisColor));
        settings->setValue(s_subStepColorOptionName, ColorToString(m_subStepColor));
        settings->setValue(s_hitDetectionColliderColorOptionName, ColorToString(m_hitDetectionColliderColor));
        settings->setValue(s_selectedHitDetectionColliderColorOptionName, ColorToString(m_selectedHitDetectionColliderColor));
        settings->setValue(s_ragdollColliderColorOptionName, ColorToString(m_ragdollColliderColor));
        settings->setValue(s_selectedRagdollColliderColorOptionName, ColorToString(m_selectedRagdollColliderColor));
        settings->setValue(s_clothColliderColorOptionName, ColorToString(m_clothColliderColor));
        settings->setValue(s_selectedClothColliderColorOptionName, ColorToString(m_selectedClothColliderColor));

        settings->setValue(s_lightSkyColorOptionName, ColorToString(m_lightSkyColor));
        settings->setValue(s_lightGroundColorOptionName, ColorToString(m_lightGroundColor));
        settings->setValue(s_rimColorOptionName, ColorToString(m_rimColor));

        settings->setValue(s_trajectoryArrowInnerColorOptionName, ColorToString(m_trajectoryArrowInnerColor));

        settings->setValue(s_gridUnitSizeOptionName, (double)m_gridUnitSize);
        settings->setValue(s_faceNormalsScaleOptionName, (double)m_faceNormalsScale);
        settings->setValue(s_vertexNormalsScaleOptionName, (double)m_vertexNormalsScale);
        settings->setValue(s_tangentsScaleOptionName, (double)m_tangentsScale);
        settings->setValue(s_nearClipPlaneDistanceOptionName, (double)m_nearClipPlaneDistance);
        settings->setValue(s_farClipPlaneDistanceOptionName, (double)m_farClipPlaneDistance);
        settings->setValue(s_FOVOptionName, (double)m_fov);
        settings->setValue(s_showFPSOptionName, m_showFPS);

        settings->setValue(s_lastUsedLayoutOptionName, m_lastUsedLayout.c_str());

        settings->setValue(s_nodeOrientationScaleOptionName, (double)m_nodeOrientationScale);
        settings->setValue(s_scaleBonesOnLengthOptionName, m_scaleBonesOnLength);

        settings->setValue(s_mainLightIntensityOptionName, (double)m_mainLightIntensity);
        settings->setValue(s_mainLightAngleAOptionName, (double)m_mainLightAngleA);
        settings->setValue(s_mainLightAngleBOptionName, (double)m_mainLightAngleB);
        settings->setValue(s_specularIntensityOptionName, (double)m_specularIntensity);

        settings->setValue(s_rimIntensityOptionName, (double)m_rimIntensity);
        settings->setValue(s_rimAngleOptionName, (double)m_rimAngle);
        settings->setValue(s_rimWidthOptionName, (double)m_rimWidth);

        settings->setValue(s_renderSelectionBoxOptionName, m_renderSelectionBox);

        settings->setValue("manipulatorMode", static_cast<int>(m_manipulatorMode));
        settings->setValue("cameraViewMode", static_cast<int>(m_cameraViewMode));
        settings->setValue("cameraFollowUp", m_cameraFollowUp);

        // Save render flags
        settings->setValue("renderFlags", static_cast<AZ::u32>(m_renderFlags));
    }

    RenderOptions RenderOptions::Load(QSettings* settings)
    {
        RenderOptions options;
        options.m_lastUsedLayout = FromQtString(settings->value(s_lastUsedLayoutOptionName, options.m_lastUsedLayout.c_str()).toString());
        options.m_backgroundColor = StringToColor(settings->value(s_backgroundColorOptionName, ColorToString(options.m_backgroundColor)).toString());
        options.m_gradientSourceColor = StringToColor(settings->value(s_gradientSourceColorOptionName, ColorToString(options.m_gradientSourceColor)).toString());
        options.m_gradientTargetColor = StringToColor(settings->value(s_gradientTargetColorOptionName, ColorToString(options.m_gradientTargetColor)).toString());
        options.m_wireframeColor = StringToColor(settings->value(s_wireframeColorOptionName, ColorToString(options.m_wireframeColor)).toString());
        options.m_vertexNormalsColor = StringToColor(settings->value(s_vertexNormalsColorOptionName, ColorToString(options.m_vertexNormalsColor)).toString());
        options.m_faceNormalsColor = StringToColor(settings->value(s_faceNormalsColorOptionName, ColorToString(options.m_faceNormalsColor)).toString());
        options.m_tangentsColor = StringToColor(settings->value(s_tangentsColorOptionName, ColorToString(options.m_tangentsColor)).toString());
        options.m_mirroredBitangentsColor = StringToColor(settings->value(s_mirroredBitangentsColorOptionName, ColorToString(options.m_mirroredBitangentsColor)).toString());
        options.m_bitangentsColor = StringToColor(settings->value(s_bitangentsColorOptionName, ColorToString(options.m_bitangentsColor)).toString());
        options.m_nodeAABBColor = StringToColor(settings->value(s_nodeAABBColorOptionName, ColorToString(options.m_nodeAABBColor)).toString());
        options.m_staticAABBColor = StringToColor(settings->value(s_staticAABBColorOptionName, ColorToString(options.m_staticAABBColor)).toString());
        options.m_meshAABBColor = StringToColor(settings->value(s_meshAABBColorOptionName, ColorToString(options.m_meshAABBColor)).toString());
        options.m_collisionMeshColor = StringToColor(settings->value(s_collisionMeshColorOptionName, ColorToString(options.m_collisionMeshColor)).toString());
        options.m_lineSkeletonColor = StringToColor(settings->value(s_lineSkeletonColorOptionName, ColorToString(options.m_lineSkeletonColor)).toString());
        options.m_skeletonColor = StringToColor(settings->value(s_skeletonColorOptionName, ColorToString(options.m_skeletonColor)).toString());
        options.m_selectionColor = StringToColor(settings->value(s_selectionColorOptionName, ColorToString(options.m_selectionColor)).toString());
        options.m_selectedObjectColor = StringToColor(settings->value(s_selectedObjectColorOptionName, ColorToString(options.m_selectedObjectColor)).toString());
        options.m_nodeNameColor = StringToColor(settings->value(s_nodeNameColorOptionName, ColorToString(options.m_nodeNameColor)).toString());
        options.m_rimColor = StringToColor(settings->value(s_rimColorOptionName, ColorToString(options.m_rimColor)).toString());

        options.m_trajectoryArrowInnerColor = StringToColor(settings->value(s_trajectoryArrowInnerColorOptionName, ColorToString(options.m_trajectoryArrowInnerColor)).toString());

        options.m_gridColor = StringToColor(settings->value(s_gridColorOptionName, ColorToString(options.m_gridColor)).toString());
        options.m_mainAxisColor = StringToColor(settings->value(s_mainAxisColorOptionName, ColorToString(options.m_mainAxisColor)).toString());
        options.m_subStepColor = StringToColor(settings->value(s_subStepColorOptionName, ColorToString(options.m_subStepColor)).toString());

        options.m_lightSkyColor = StringToColor(settings->value(s_lightSkyColorOptionName, ColorToString(options.m_lightSkyColor)).toString());
        options.m_lightGroundColor = StringToColor(settings->value(s_lightGroundColorOptionName, ColorToString(options.m_lightGroundColor)).toString());

        options.m_hitDetectionColliderColor = StringToColor(settings->value(s_hitDetectionColliderColorOptionName, ColorToString(options.m_hitDetectionColliderColor)).toString());
        options.m_selectedHitDetectionColliderColor = StringToColor(settings->value(s_selectedHitDetectionColliderColorOptionName, ColorToString(options.m_selectedHitDetectionColliderColor)).toString());
        options.m_ragdollColliderColor = StringToColor(settings->value(s_ragdollColliderColorOptionName, ColorToString(options.m_ragdollColliderColor)).toString());
        options.m_selectedRagdollColliderColor = StringToColor(settings->value(s_selectedRagdollColliderColorOptionName, ColorToString(options.m_selectedRagdollColliderColor)).toString());
        options.m_clothColliderColor = StringToColor(settings->value(s_clothColliderColorOptionName, ColorToString(options.m_clothColliderColor)).toString());
        options.m_selectedClothColliderColor = StringToColor(settings->value(s_selectedClothColliderColorOptionName, ColorToString(options.m_selectedClothColliderColor)).toString());

        options.m_showFPS = settings->value(s_showFPSOptionName, options.m_showFPS).toBool();

        options.m_gridUnitSize = (float)settings->value(s_gridUnitSizeOptionName, (double)options.m_gridUnitSize).toDouble();
        options.m_faceNormalsScale = (float)settings->value(s_faceNormalsScaleOptionName, (double)options.m_faceNormalsScale).toDouble();
        options.m_vertexNormalsScale = (float)settings->value(s_vertexNormalsScaleOptionName, (double)options.m_vertexNormalsScale).toDouble();
        options.m_tangentsScale = (float)settings->value(s_tangentsScaleOptionName, (double)options.m_tangentsScale).toDouble();

        options.m_nearClipPlaneDistance = (float)settings->value(s_nearClipPlaneDistanceOptionName, (double)options.m_nearClipPlaneDistance).toDouble();
        options.m_farClipPlaneDistance = (float)settings->value(s_farClipPlaneDistanceOptionName, (double)options.m_farClipPlaneDistance).toDouble();
        options.m_fov = (float)settings->value(s_FOVOptionName, (double)options.m_fov).toDouble();

        options.m_mainLightIntensity = (float)settings->value(s_mainLightIntensityOptionName, (double)options.m_mainLightIntensity).toDouble();
        options.m_mainLightAngleA = (float)settings->value(s_mainLightAngleAOptionName, (double)options.m_mainLightAngleA).toDouble();
        options.m_mainLightAngleB = (float)settings->value(s_mainLightAngleBOptionName, (double)options.m_mainLightAngleB).toDouble();
        options.m_specularIntensity = (float)settings->value(s_specularIntensityOptionName, (double)options.m_specularIntensity).toDouble();

        options.m_nodeOrientationScale = (float)settings->value(s_nodeOrientationScaleOptionName, (double)options.m_nodeOrientationScale).toDouble();
        options.m_scaleBonesOnLength = settings->value(s_scaleBonesOnLengthOptionName, options.m_scaleBonesOnLength).toBool();

        options.m_rimIntensity = (float)settings->value(s_rimIntensityOptionName, (double)options.m_rimIntensity).toDouble();
        options.m_rimAngle = (float)settings->value(s_rimAngleOptionName, (double)options.m_rimAngle).toDouble();
        options.m_rimWidth = (float)settings->value(s_rimWidthOptionName, (double)options.m_rimWidth).toDouble();

        options.m_renderSelectionBox = settings->value(s_renderSelectionBoxOptionName, options.m_renderSelectionBox).toBool();

        options.m_manipulatorMode = static_cast<ManipulatorMode>(settings->value("manipulatorMode", options.m_manipulatorMode).toInt());
        options.m_cameraViewMode = static_cast<CameraViewMode>(settings->value("cameraViewMode", options.m_cameraViewMode).toInt());
        options.m_cameraFollowUp = settings->value("CameraFollowUp", options.m_cameraFollowUp).toBool();

        // Read render flags
        options.m_renderFlags =
            EMotionFX::ActorRenderFlags(settings->value("RenderFlags", static_cast<int>(EMotionFX::ActorRenderFlags::Default)).toInt());

        options.CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());

        return options;
    }

    AZ::Color RenderOptions::StringToColor(const QString& text)
    {
        QColor color = QColor(text);
        return AZ::Color(static_cast<float>(color.redF()),
            static_cast<float>(color.greenF()),
            static_cast<float>(color.blueF()),
            static_cast<float>(color.alphaF()));
    }

    QString RenderOptions::ColorToString(const AZ::Color& color)
    {
        QColor qColor;
        qColor.setRedF(color.GetR());
        qColor.setGreenF(color.GetG());
        qColor.setBlueF(color.GetB());
        qColor.setAlphaF(color.GetA());
        return qColor.name();
    }

    void RenderOptions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<RenderOptions>()
            ->Version(1)
            ->Field(s_gridUnitSizeOptionName, &RenderOptions::m_gridUnitSize)
            ->Field(s_vertexNormalsScaleOptionName, &RenderOptions::m_vertexNormalsScale)
            ->Field(s_faceNormalsScaleOptionName, &RenderOptions::m_faceNormalsScale)
            ->Field(s_tangentsScaleOptionName, &RenderOptions::m_tangentsScale)
            ->Field(s_nodeOrientationScaleOptionName, &RenderOptions::m_nodeOrientationScale)
            ->Field(s_scaleBonesOnLengthOptionName, &RenderOptions::m_scaleBonesOnLength)
            ->Field(s_nearClipPlaneDistanceOptionName, &RenderOptions::m_nearClipPlaneDistance)
            ->Field(s_farClipPlaneDistanceOptionName, &RenderOptions::m_farClipPlaneDistance)
            ->Field(s_FOVOptionName, &RenderOptions::m_fov)
            ->Field(s_mainLightIntensityOptionName, &RenderOptions::m_mainLightIntensity)
            ->Field(s_mainLightAngleAOptionName, &RenderOptions::m_mainLightAngleA)
            ->Field(s_mainLightAngleBOptionName, &RenderOptions::m_mainLightAngleB)
            ->Field(s_specularIntensityOptionName, &RenderOptions::m_specularIntensity)
            ->Field(s_rimIntensityOptionName, &RenderOptions::m_rimIntensity)
            ->Field(s_rimWidthOptionName, &RenderOptions::m_rimWidth)
            ->Field(s_rimAngleOptionName, &RenderOptions::m_rimAngle)
            ->Field(s_showFPSOptionName, &RenderOptions::m_showFPS)
            ->Field(s_lightGroundColorOptionName, &RenderOptions::m_lightGroundColor)
            ->Field(s_lightSkyColorOptionName, &RenderOptions::m_lightSkyColor)
            ->Field(s_rimColorOptionName, &RenderOptions::m_rimColor)
            ->Field(s_backgroundColorOptionName, &RenderOptions::m_backgroundColor)
            ->Field(s_gradientSourceColorOptionName, &RenderOptions::m_gradientSourceColor)
            ->Field(s_gradientTargetColorOptionName, &RenderOptions::m_gradientTargetColor)
            ->Field(s_wireframeColorOptionName, &RenderOptions::m_wireframeColor)
            ->Field(s_collisionMeshColorOptionName, &RenderOptions::m_collisionMeshColor)
            ->Field(s_vertexNormalsColorOptionName, &RenderOptions::m_vertexNormalsColor)
            ->Field(s_faceNormalsColorOptionName, &RenderOptions::m_faceNormalsColor)
            ->Field(s_tangentsColorOptionName, &RenderOptions::m_tangentsColor)
            ->Field(s_mirroredBitangentsColorOptionName, &RenderOptions::m_mirroredBitangentsColor)
            ->Field(s_bitangentsColorOptionName, &RenderOptions::m_bitangentsColor)
            ->Field(s_nodeAABBColorOptionName, &RenderOptions::m_nodeAABBColor)
            ->Field(s_staticAABBColorOptionName, &RenderOptions::m_staticAABBColor)
            ->Field(s_meshAABBColorOptionName, &RenderOptions::m_meshAABBColor)
            ->Field(s_lineSkeletonColorOptionName, &RenderOptions::m_lineSkeletonColor)
            ->Field(s_skeletonColorOptionName, &RenderOptions::m_skeletonColor)
            ->Field(s_selectionColorOptionName, &RenderOptions::m_selectionColor)
            ->Field(s_selectedObjectColorOptionName, &RenderOptions::m_selectedObjectColor)
            ->Field(s_nodeNameColorOptionName, &RenderOptions::m_nodeNameColor)
            ->Field(s_gridColorOptionName, &RenderOptions::m_gridColor)
            ->Field(s_mainAxisColorOptionName, &RenderOptions::m_mainAxisColor)
            ->Field(s_subStepColorOptionName, &RenderOptions::m_subStepColor)
            ->Field(s_trajectoryArrowInnerColorOptionName, &RenderOptions::m_trajectoryArrowInnerColor)
            ->Field(s_lastUsedLayoutOptionName, &RenderOptions::m_lastUsedLayout)
            ->Field(s_renderSelectionBoxOptionName, &RenderOptions::m_renderSelectionBox)
            ->Field(s_hitDetectionColliderColorOptionName, &RenderOptions::m_hitDetectionColliderColor)
            ->Field(s_selectedHitDetectionColliderColorOptionName, &RenderOptions::m_selectedHitDetectionColliderColor)
            ->Field(s_ragdollColliderColorOptionName, &RenderOptions::m_ragdollColliderColor)
            ->Field(s_selectedRagdollColliderColorOptionName, &RenderOptions::m_selectedRagdollColliderColor)
            ->Field(s_clothColliderColorOptionName, &RenderOptions::m_clothColliderColor)
            ->Field(s_selectedClothColliderColorOptionName, &RenderOptions::m_selectedClothColliderColor);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<RenderOptions>("Render plugin properties", "Render window properties")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_gridUnitSize, "Grid unit size",
                "Render a grid line every X units.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnGridUnitSizeChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_vertexNormalsScale, "Vertex normals scale",
                "Scale factor for vertex normals.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnVertexNormalsScaleChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_faceNormalsScale, "Face normals scale",
                "Scale factor for face normals.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnFaceNormalsScaleChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_tangentsScale, "Tangents & bitangents scale",
                "Scale factor for tangents and bitangents.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnTangentsScaleChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_nodeOrientationScale, "Joint transform scale",
                "Scale factor for joint transform visualizations.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnNodeOrientationScaleChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_scaleBonesOnLength, "Scale joint transforms on length",
                "Scale joint transforms based on the length of the bone. The longer the bone, the bigger the joint transform visualization.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnScaleBonesOnLengthChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_nearClipPlaneDistance, "Near clip plane distance",
                "Polygons closer to the camera will not be shown.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnNearClipPlaneDistanceChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.001f)
            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_farClipPlaneDistance, "Far clip plane distance",
                "Polygons further away will not be shown.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnFarClipPlaneDistanceChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_fov, "Field of view",
                "Angle in degrees of the field of view.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnFOVChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 170.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_mainLightIntensity, "Main light intensity",
                "Intensity of the main light.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMainLightIntensityChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_mainLightAngleA, "Main light angle A",
                "Angle in degrees of the main light.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMainLightAngleAChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_mainLightAngleB, "Main light angle B",
                "Angle in degrees of the main light.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMainLightAngleBChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_specularIntensity, "Specular intensity",
                "Specular intensity.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSpecularIntensityChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 3.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_rimIntensity, "Rim intensity",
                "Rim light intensity.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnRimIntensityChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 3.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_rimWidth, "Rim width",
                "Rim light width.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnRimWidthChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_rimAngle, "Rim angle",
                "Rim light angle.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnRimAngleChangedCallback)
            ->Attribute(AZ::Edit::Attributes::Min, -360.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 360.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_showFPS, "Show FPS",
                "Show anim graph rendering statistics like render time and average frames per second.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnShowFPSChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_lightGroundColor, "Ground light color",
                "Ground light color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnLightGroundColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_lightSkyColor, "Sky light color",
                "Sky light color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnLightSkyColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_rimColor, "Rim light color",
                "Rim light color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnRimColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_backgroundColor, "Background color",
                "Background color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnBackgroundColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_gradientSourceColor, "Gradient background top color",
                "Gradient background top color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnGradientSourceColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_gradientTargetColor, "Gradient background bottom color",
                "Gradient background bottom color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnGradientTargetColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_wireframeColor, "Wireframe color",
                "Color for rendering the character mesh in wireframe mode.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnWireframeColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_collisionMeshColor, "Collision mesh color",
                "Collision mesh color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnCollisionMeshColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_vertexNormalsColor, "Vertex normals color",
                "Vertex normals color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnVertexNormalsColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_faceNormalsColor, "Face normals color",
                "Face normals color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnFaceNormalsColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_tangentsColor, "Tangents color",
                "Tangents color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnTangentsColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_mirroredBitangentsColor, "Mirrored bitangents color",
                "Mirrored bitangents color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMirroredBitangentsColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_bitangentsColor, "Bitangents color",
                "Bitangents color")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnBitangentsColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_nodeAABBColor, "Joint based AABB color",
                "Color for the runtime-updated AABB calculated based on the skeletal pose.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnNodeAABBColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_staticAABBColor, "Static based AABB color",
                "Color for the pre-calculated, static AABB.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnStaticAABBColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_meshAABBColor, "Mesh based AABB color",
                "Color for the runtime-updated AABB calculated based on the deformed meshes.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMeshAABBColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_lineSkeletonColor, "Line based skeleton color",
                "Line-based skeleton color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnLineSkeletonColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_skeletonColor, "Solid skeleton color",
                "Solid skeleton color.")
            ->Attribute(AZ_CRC_CE("AlphaChannel"), true)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSkeletonColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_selectionColor, "Selection gizmo color",
                "Selection gizmo color")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSelectionColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_selectedObjectColor, "Selected object color",
                "Selection gizmo color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSelectedObjectColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_nodeNameColor, "Joint name color",
                "Joint name text color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnNodeNameColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_gridColor, "Grid color",
                "Grid color. The grid is tiled and every fifth line uses this color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnGridColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_mainAxisColor, "Grid main axis color",
                "Grid main axis color. (Lines going through the origin)")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnMainAxisColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_subStepColor, "Grid substep color",
                "Grid substep color. The inner four lines within a tile use this color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSubStepColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_trajectoryArrowInnerColor, "Trajectory path color",
                "Color of the trajectory path the characters creates when using motion extraction.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnTrajectoryArrowInnerColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_hitDetectionColliderColor, "Hit detection collider color",
                "Hit detection collider color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnHitDetectionColliderColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_selectedHitDetectionColliderColor, "Selected hit detection collider color",
                "Selected hit detection collider color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSelectedHitDetectionColliderColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_ragdollColliderColor, "Ragdoll collider color",
                "Ragdoll collider color.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnRagdollColliderColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_selectedRagdollColliderColor, "Selected ragdoll collider color",
                "Selected ragdoll collider color")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSelectedRagdollColliderColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_clothColliderColor, "Cloth collider color",
                "Cloth collider color")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnClothColliderColorChangedCallback)
            ->DataElement(AZ::Edit::UIHandlers::Default, &RenderOptions::m_selectedClothColliderColor, "Selected cloth collider color",
                "Selected cloth collider color")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &RenderOptions::OnSelectedClothColliderColorChangedCallback)
            ;
    }

    void RenderOptions::SetGridUnitSize(float gridUnitSize)
    {
        if (!AZ::IsClose(gridUnitSize, m_gridUnitSize, std::numeric_limits<float>::epsilon()))
        {
            m_gridUnitSize = gridUnitSize;
            OnGridUnitSizeChangedCallback();
        }
    }

    void RenderOptions::SetVertexNormalsScale(float vertexNormalsScale)
    {
        if (!AZ::IsClose(vertexNormalsScale, m_vertexNormalsScale, std::numeric_limits<float>::epsilon()))
        {
            m_vertexNormalsScale = vertexNormalsScale;
            OnVertexNormalsScaleChangedCallback();
        }
    }

    void RenderOptions::SetFaceNormalsScale(float faceNormalsScale)
    {
        if (!AZ::IsClose(faceNormalsScale, m_faceNormalsScale, std::numeric_limits<float>::epsilon()))
        {
            m_faceNormalsScale = faceNormalsScale;
            OnFaceNormalsScaleChangedCallback();
        }
    }

    void RenderOptions::SetTangentsScale(float tangentsScale)
    {
        if (!AZ::IsClose(tangentsScale, m_tangentsScale, std::numeric_limits<float>::epsilon()))
        {
            m_tangentsScale = tangentsScale;
            OnTangentsScaleChangedCallback();
        }
    }

    void RenderOptions::SetNodeOrientationScale(float nodeOrientationScale)
    {
        if (!AZ::IsClose(nodeOrientationScale, m_nodeOrientationScale, std::numeric_limits<float>::epsilon()))
        {
            m_nodeOrientationScale = nodeOrientationScale;
            OnNodeOrientationScaleChangedCallback();
        }
    }

    void RenderOptions::SetScaleBonesOnLenght(bool scaleBonesOnLenght)
    {
        if (scaleBonesOnLenght != m_scaleBonesOnLength)
        {
            m_scaleBonesOnLength = scaleBonesOnLenght;
            OnScaleBonesOnLengthChangedCallback();
        }
    }

    void RenderOptions::SetNearClipPlaneDistance(float nearClipPlaneDistance)
    {
        if (!AZ::IsClose(nearClipPlaneDistance, m_nearClipPlaneDistance, std::numeric_limits<float>::epsilon()))
        {
            m_nearClipPlaneDistance = nearClipPlaneDistance;
            OnNearClipPlaneDistanceChangedCallback();
        }
    }

    void RenderOptions::SetFarClipPlaneDistance(float farClipPlaneDistance)
    {
        if (!AZ::IsClose(farClipPlaneDistance, m_farClipPlaneDistance, std::numeric_limits<float>::epsilon()))
        {
            m_farClipPlaneDistance = farClipPlaneDistance;
            OnFarClipPlaneDistanceChangedCallback();
        }
    }

    void RenderOptions::SetFOV(float FOV)
    {
        if (!AZ::IsClose(FOV, m_fov, std::numeric_limits<float>::epsilon()))
        {
            m_fov = FOV;
            OnFOVChangedCallback();
        }
    }

    void RenderOptions::SetMainLightIntensity(float mainLightIntensity)
    {
        if (!AZ::IsClose(mainLightIntensity, m_mainLightIntensity, std::numeric_limits<float>::epsilon()))
        {
            m_mainLightIntensity = mainLightIntensity;
            OnMainLightIntensityChangedCallback();
        }
    }

    void RenderOptions::SetMainLightAngleA(float mainLightAngleA)
    {
        if (!AZ::IsClose(mainLightAngleA, m_mainLightAngleA, std::numeric_limits<float>::epsilon()))
        {
            m_mainLightAngleA = mainLightAngleA;
            OnMainLightAngleAChangedCallback();
        }
    }

    void RenderOptions::SetMainLightAngleB(float mainLightAngleB)
    {
        if (!AZ::IsClose(mainLightAngleB, m_mainLightAngleB, std::numeric_limits<float>::epsilon()))
        {
            m_mainLightAngleB = mainLightAngleB;
            OnMainLightAngleBChangedCallback();
        }
    }

    void RenderOptions::SetSpecularIntensity(float specularIntensity)
    {
        if (!AZ::IsClose(specularIntensity, m_specularIntensity, std::numeric_limits<float>::epsilon()))
        {
            m_specularIntensity = specularIntensity;
            OnSpecularIntensityChangedCallback();
        }
    }

    void RenderOptions::SetRimIntensity(float rimIntensity)
    {
        if (!AZ::IsClose(rimIntensity, m_rimIntensity, std::numeric_limits<float>::epsilon()))
        {
            m_rimIntensity = rimIntensity;
            OnRimIntensityChangedCallback();
        }
    }

    void RenderOptions::SetRimWidth(float rimWidth)
    {
        if (!AZ::IsClose(rimWidth, m_rimWidth, std::numeric_limits<float>::epsilon()))
        {
            m_rimWidth = rimWidth;
            OnRimWidthChangedCallback();
        }
    }

    void RenderOptions::SetRimAngle(float rimAngle)
    {
        if (!AZ::IsClose(rimAngle, m_rimAngle, std::numeric_limits<float>::epsilon()))
        {
            m_rimAngle = rimAngle;
            OnRimAngleChangedCallback();
        }
    }

    void RenderOptions::SetShowFPS(bool showFPS)
    {
        if (showFPS != m_showFPS)
        {
            m_showFPS = showFPS;
            OnShowFPSChangedCallback();
        }
    }

    void RenderOptions::SetLightGroundColor(const AZ::Color& lightGroundColor)
    {
        if (!lightGroundColor.IsClose(m_lightGroundColor))
        {
            m_lightGroundColor = lightGroundColor;
            OnLightGroundColorChangedCallback();
        }
    }

    void RenderOptions::SetLightSkyColor(const AZ::Color& lightSkyColor)
    {
        if (!lightSkyColor.IsClose(m_lightSkyColor))
        {
            m_lightSkyColor = lightSkyColor;
            OnLightSkyColorChangedCallback();
        }
    }

    void RenderOptions::SetRimColor(const AZ::Color& rimColor)
    {
        if (!rimColor.IsClose(m_rimColor))
        {
            m_rimColor = rimColor;
            OnRimColorChangedCallback();
        }
    }

    void RenderOptions::SetBackgroundColor(const AZ::Color& backgroundColor)
    {
        if (!backgroundColor.IsClose(m_backgroundColor))
        {
            m_backgroundColor = backgroundColor;
            OnBackgroundColorChangedCallback();
        }
    }

    void RenderOptions::SetGradientSourceColor(const AZ::Color& gradientSourceColor)
    {
        if (!gradientSourceColor.IsClose(m_gradientSourceColor))
        {
            m_gradientSourceColor = gradientSourceColor;
            OnGradientSourceColorChangedCallback();
        }
    }

    void RenderOptions::SetGradientTargetColor(const AZ::Color& gradientTargetColor)
    {
        if (!gradientTargetColor.IsClose(m_gradientTargetColor))
        {
            m_gradientTargetColor = gradientTargetColor;
            OnGradientTargetColorChangedCallback();
        }
    }

    void RenderOptions::SetWireframeColor(const AZ::Color& wireframeColor)
    {
        if (!wireframeColor.IsClose(m_wireframeColor))
        {
            m_wireframeColor = wireframeColor;
            OnWireframeColorChangedCallback();
        }
    }

    void RenderOptions::SetCollisionMeshColor(const AZ::Color& collisionMeshColor)
    {
        if (!collisionMeshColor.IsClose(m_collisionMeshColor))
        {
            m_collisionMeshColor = collisionMeshColor;
            OnCollisionMeshColorChangedCallback();
        }
    }

    void RenderOptions::SetVertexNormalsColor(const AZ::Color& vertexNormalsColor)
    {
        if (!vertexNormalsColor.IsClose(m_vertexNormalsColor))
        {
            m_vertexNormalsColor = vertexNormalsColor;
            OnVertexNormalsColorChangedCallback();
        }
    }

    void RenderOptions::SetFaceNormalsColor(const AZ::Color& faceNormalsColor)
    {
        if (!faceNormalsColor.IsClose(m_faceNormalsColor))
        {
            m_faceNormalsColor = faceNormalsColor;
            OnFaceNormalsColorChangedCallback();
        }
    }

    void RenderOptions::SetTangentsColor(const AZ::Color& tangentsColor)
    {
        if (!tangentsColor.IsClose(m_tangentsColor))
        {
            m_tangentsColor = tangentsColor;
            OnTangentsColorChangedCallback();
        }
    }

    void RenderOptions::SetMirroredBitangentsColor(const AZ::Color& mirroredBitangentsColor)
    {
        if (!mirroredBitangentsColor.IsClose(m_mirroredBitangentsColor))
        {
            m_mirroredBitangentsColor = mirroredBitangentsColor;
            OnMirroredBitangentsColorChangedCallback();
        }
    }

    void RenderOptions::SetBitangentsColor(const AZ::Color& bitangentsColor)
    {
        if (!bitangentsColor.IsClose(m_bitangentsColor))
        {
            m_bitangentsColor = bitangentsColor;
            OnBitangentsColorChangedCallback();
        }
    }

    void RenderOptions::SetNodeAABBColor(const AZ::Color& nodeAABBColor)
    {
        if (!nodeAABBColor.IsClose(m_nodeAABBColor))
        {
            m_nodeAABBColor = nodeAABBColor;
            OnNodeAABBColorChangedCallback();
        }
    }

    void RenderOptions::SetStaticAABBColor(const AZ::Color& staticAABBColor)
    {
        if (!staticAABBColor.IsClose(m_staticAABBColor))
        {
            m_staticAABBColor = staticAABBColor;
            OnStaticAABBColorChangedCallback();
        }
    }

    void RenderOptions::SetMeshAABBColor(const AZ::Color& meshAABBColor)
    {
        if (!meshAABBColor.IsClose(m_meshAABBColor))
        {
            m_meshAABBColor = meshAABBColor;
            OnMeshAABBColorChangedCallback();
        }
    }

    void RenderOptions::SetLineSkeletonColor(const AZ::Color& lineSkeletonColor)
    {
        if (!lineSkeletonColor.IsClose(m_lineSkeletonColor))
        {
            m_lineSkeletonColor = lineSkeletonColor;
            OnLineSkeletonColorChangedCallback();
        }
    }

    void RenderOptions::SetSkeletonColor(const AZ::Color& skeletonColor)
    {
        if (!skeletonColor.IsClose(m_skeletonColor))
        {
            m_skeletonColor = skeletonColor;
            OnSkeletonColorChangedCallback();
        }
    }

    void RenderOptions::SetSelectionColor(const AZ::Color& selectionColor)
    {
        if (!selectionColor.IsClose(m_selectionColor))
        {
            m_selectionColor = selectionColor;
            OnSelectionColorChangedCallback();
        }
    }

    void RenderOptions::SetSelectedObjectColor(const AZ::Color& selectedObjectColor)
    {
        if (!selectedObjectColor.IsClose(m_selectedObjectColor))
        {
            m_selectedObjectColor = selectedObjectColor;
            OnSelectedObjectColorChangedCallback();
        }
    }

    void RenderOptions::SetNodeNameColor(const AZ::Color& nodeNameColor)
    {
        if (!nodeNameColor.IsClose(m_nodeNameColor))
        {
            m_nodeNameColor = nodeNameColor;
            OnNodeNameColorChangedCallback();
        }
    }

    void RenderOptions::SetGridColor(const AZ::Color& gridColor)
    {
        if (!gridColor.IsClose(m_gridColor))
        {
            m_gridColor = gridColor;
            OnGridColorChangedCallback();
        }
    }

    void RenderOptions::SetMainAxisColor(const AZ::Color& mainAxisColor)
    {
        if (!mainAxisColor.IsClose(m_mainAxisColor))
        {
            m_mainAxisColor = mainAxisColor;
            OnMainAxisColorChangedCallback();
        }
    }

    void RenderOptions::SetSubStepColor(const AZ::Color& subStepColor)
    {
        if (!subStepColor.IsClose(m_subStepColor))
        {
            m_subStepColor = subStepColor;
            OnSubStepColorChangedCallback();
        }
    }

    void RenderOptions::SetHitDetectionColliderColor(const AZ::Color& colliderColor)
    {
        if (!colliderColor.IsClose(m_hitDetectionColliderColor))
        {
            m_hitDetectionColliderColor = colliderColor;
            OnHitDetectionColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetSelectedHitDetectionColliderColor(const AZ::Color& colliderColor)
    {
        if (!colliderColor.IsClose(m_selectedHitDetectionColliderColor))
        {
            m_selectedHitDetectionColliderColor = colliderColor;
            OnSelectedHitDetectionColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetRagdollColliderColor(const AZ::Color& color)
    {
        if (!color.IsClose(m_ragdollColliderColor))
        {
            m_ragdollColliderColor = color;
            OnRagdollColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetSelectedRagdollColliderColor(const AZ::Color& color)
    {
        if (!color.IsClose(m_selectedRagdollColliderColor))
        {
            m_selectedRagdollColliderColor = color;
            OnSelectedRagdollColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetViolatedJointLimitColor(const AZ::Color& color)
    {
        if (!color.IsClose(m_violatedJointLimitColor))
        {
            m_violatedJointLimitColor = color;
            OnViolatedJointLimitColorChangedCallback();
        }
    }

    void RenderOptions::SetClothColliderColor(const AZ::Color& colliderColor)
    {
        if (!colliderColor.IsClose(m_clothColliderColor))
        {
            m_clothColliderColor = colliderColor;
            OnClothColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetSelectedClothColliderColor(const AZ::Color& colliderColor)
    {
        if (!colliderColor.IsClose(m_selectedClothColliderColor))
        {
            m_selectedClothColliderColor = colliderColor;
            OnSelectedClothColliderColorChangedCallback();
        }
    }

    void RenderOptions::SetTrajectoryArrowInnerColor(const AZ::Color& trajectoryArrowInnerColor)
    {
        if (!trajectoryArrowInnerColor.IsClose(m_trajectoryArrowInnerColor))
        {
            m_trajectoryArrowInnerColor = trajectoryArrowInnerColor;
            OnTrajectoryArrowInnerColorChangedCallback();
        }
    }

    void RenderOptions::SetLastUsedLayout(const AZStd::string& lastUsedLayout)
    {
        if (lastUsedLayout != m_lastUsedLayout)
        {
            m_lastUsedLayout = lastUsedLayout;
            OnLastUsedLayoutChangedCallback();
        }
    }

    void RenderOptions::SetRenderSelectionBox(bool renderSelectionBox)
    {
        if (renderSelectionBox != m_renderSelectionBox)
        {
            m_renderSelectionBox = renderSelectionBox;
            OnRenderSelectionBoxChangedCallback();
        }
    }

    void RenderOptions::SetManipulatorMode(ManipulatorMode mode)
    {
        m_manipulatorMode = mode;
    }

    RenderOptions::ManipulatorMode RenderOptions::GetManipulatorMode() const
    {
        return m_manipulatorMode;
    }

    void RenderOptions::SetCameraViewMode(CameraViewMode mode)
    {
        m_cameraViewMode = mode;
    }

    RenderOptions::CameraViewMode RenderOptions::GetCameraViewMode() const
    {
        return m_cameraViewMode;
    }

    void RenderOptions::SetCameraFollowUp(bool followUp)
    {
        m_cameraFollowUp = followUp;
    }

    bool RenderOptions::GetCameraFollowUp() const
    {
        return m_cameraFollowUp;
    }

    void RenderOptions::ToggerRenderFlag(uint8 index)
    {
        m_renderFlags ^= EMotionFX::ActorRenderFlags(AZ_BIT(index));
    }

    void RenderOptions::SetRenderFlags(EMotionFX::ActorRenderFlags renderFlags)
    {
        m_renderFlags = renderFlags;
    }

    EMotionFX::ActorRenderFlags RenderOptions::GetRenderFlags() const
    {
        return m_renderFlags;
    }

    void RenderOptions::CopyToRenderActorSettings(AZ::Render::RenderActorSettings& settings) const
    {
        settings.m_vertexNormalsScale = m_vertexNormalsScale;
        settings.m_faceNormalsScale = m_faceNormalsScale;
        settings.m_tangentsScale = m_tangentsScale;
        settings.m_nodeOrientationScale = m_nodeOrientationScale;

        settings.m_vertexNormalsColor = m_vertexNormalsColor;
        settings.m_faceNormalsColor = m_faceNormalsColor;
        settings.m_tangentsColor = m_tangentsColor;
        settings.m_mirroredBitangentsColor = m_mirroredBitangentsColor;
        settings.m_bitangentsColor = m_bitangentsColor;
        settings.m_wireframeColor = m_wireframeColor;
        settings.m_nodeAABBColor = m_nodeAABBColor;
        settings.m_meshAABBColor = m_meshAABBColor;
        settings.m_staticAABBColor = m_staticAABBColor;
        settings.m_skeletonColor = m_skeletonColor;
        settings.m_lineSkeletonColor = m_lineSkeletonColor;

        settings.m_hitDetectionColliderColor = m_hitDetectionColliderColor;
        settings.m_selectedHitDetectionColliderColor = m_selectedHitDetectionColliderColor;
        settings.m_ragdollColliderColor = m_ragdollColliderColor;
        settings.m_selectedRagdollColliderColor = m_selectedRagdollColliderColor;
        settings.m_violatedJointLimitColor = m_violatedJointLimitColor;
        settings.m_clothColliderColor = m_clothColliderColor;
        settings.m_selectedClothColliderColor = m_selectedClothColliderColor;
        settings.m_simulatedObjectColliderColor = m_simulatedObjectColliderColor;
        settings.m_selectedSimulatedObjectColliderColor = m_selectedSimulatedObjectColliderColor;
        settings.m_jointNameColor = m_nodeNameColor;
        settings.m_trajectoryPathColor = m_trajectoryArrowInnerColor;
    }

    void RenderOptions::OnGridUnitSizeChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_gridUnitSizeOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_gridUnitSizeOptionName);
    }

    void RenderOptions::OnVertexNormalsScaleChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_vertexNormalsScaleOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_vertexNormalsScaleOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnFaceNormalsScaleChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_faceNormalsScaleOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_faceNormalsScaleOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnTangentsScaleChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_tangentsScaleOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_tangentsScaleOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnNodeOrientationScaleChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_nodeOrientationScaleOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_nodeOrientationScaleOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnScaleBonesOnLengthChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_scaleBonesOnLengthOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_scaleBonesOnLengthOptionName);
    }

    void RenderOptions::OnNearClipPlaneDistanceChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_nearClipPlaneDistanceOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_nearClipPlaneDistanceOptionName);
    }

    void RenderOptions::OnFarClipPlaneDistanceChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_farClipPlaneDistanceOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_farClipPlaneDistanceOptionName);
    }

    void RenderOptions::OnFOVChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_FOVOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_FOVOptionName);
    }

    void RenderOptions::OnMainLightIntensityChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_mainLightIntensityOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_mainLightIntensityOptionName);
    }

    void RenderOptions::OnMainLightAngleAChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_mainLightAngleAOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_mainLightAngleAOptionName);
    }

    void RenderOptions::OnMainLightAngleBChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_mainLightAngleBOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_mainLightAngleBOptionName);
    }

    void RenderOptions::OnSpecularIntensityChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_specularIntensityOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_specularIntensityOptionName);
    }

    void RenderOptions::OnRimIntensityChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_rimIntensityOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_rimIntensityOptionName);
    }

    void RenderOptions::OnRimWidthChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_rimWidthOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_rimWidthOptionName);
    }

    void RenderOptions::OnRimAngleChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_rimAngleOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_rimAngleOptionName);
    }

    void RenderOptions::OnShowFPSChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_showFPSOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_showFPSOptionName);
    }

    void RenderOptions::OnLightGroundColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_lightGroundColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_lightGroundColorOptionName);
    }

    void RenderOptions::OnLightSkyColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_lightSkyColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_lightSkyColorOptionName);
    }

    void RenderOptions::OnRimColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_rimColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_rimColorOptionName);
    }

    void RenderOptions::OnBackgroundColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_backgroundColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_backgroundColorOptionName);
    }

    void RenderOptions::OnGradientSourceColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_gradientSourceColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_gradientSourceColorOptionName);
    }

    void RenderOptions::OnGradientTargetColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_gradientTargetColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_gradientTargetColorOptionName);
    }

    void RenderOptions::OnWireframeColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_wireframeColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_wireframeColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnCollisionMeshColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_collisionMeshColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_collisionMeshColorOptionName);
    }

    void RenderOptions::OnVertexNormalsColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_vertexNormalsColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_vertexNormalsColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnFaceNormalsColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_faceNormalsColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_faceNormalsColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnTangentsColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_tangentsColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_tangentsColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnMirroredBitangentsColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_mirroredBitangentsColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_mirroredBitangentsColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnBitangentsColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_bitangentsColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_bitangentsColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnNodeAABBColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_nodeAABBColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_nodeAABBColorOptionName);
    }

    void RenderOptions::OnStaticAABBColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_staticAABBColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_staticAABBColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnMeshAABBColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_meshAABBColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_meshAABBColorOptionName);
    }

    void RenderOptions::OnLineSkeletonColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_lineSkeletonColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_lineSkeletonColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnSkeletonColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_skeletonColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_skeletonColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnSelectionColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_selectionColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_selectionColorOptionName);
    }

    void RenderOptions::OnSelectedObjectColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_selectedObjectColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_selectedObjectColorOptionName);
    }

    void RenderOptions::OnNodeNameColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_nodeNameColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_nodeNameColorOptionName);
        CopyToRenderActorSettings(EMotionFX::GetRenderActorSettings());
    }

    void RenderOptions::OnGridColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_gridColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_gridColorOptionName);
    }

    void RenderOptions::OnMainAxisColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_mainAxisColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_mainAxisColorOptionName);
    }

    void RenderOptions::OnSubStepColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_subStepColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_subStepColorOptionName);
    }

    void RenderOptions::OnTrajectoryArrowInnerColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_trajectoryArrowInnerColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_trajectoryArrowInnerColorOptionName);
    }

    void RenderOptions::OnHitDetectionColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_hitDetectionColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_hitDetectionColliderColorOptionName);
    }

    void RenderOptions::OnSelectedHitDetectionColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_selectedHitDetectionColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_selectedHitDetectionColliderColorOptionName);
    }

    void RenderOptions::OnRagdollColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_ragdollColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_ragdollColliderColorOptionName);
    }

    void RenderOptions::OnSelectedRagdollColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_selectedRagdollColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_selectedRagdollColliderColorOptionName);
    }

    void RenderOptions::OnViolatedJointLimitColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_violatedJointLimitColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_violatedJointLimitColorOptionName);
    }

    void RenderOptions::OnClothColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_clothColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_clothColliderColorOptionName);
    }

    void RenderOptions::OnSelectedClothColliderColorChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_selectedClothColliderColorOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_selectedClothColliderColorOptionName);
    }

    void RenderOptions::OnLastUsedLayoutChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_lastUsedLayoutOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_lastUsedLayoutOptionName);
    }

    void RenderOptions::OnRenderSelectionBoxChangedCallback() const
    {
        PluginOptionsNotificationsBus::Event(s_renderSelectionBoxOptionName, &PluginOptionsNotificationsBus::Events::OnOptionChanged, s_renderSelectionBoxOptionName);
    }
} // namespace EMStudio
