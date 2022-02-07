/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/StandardHeaders.h>
#include "../EMStudioConfig.h"
#include "../PluginOptions.h"
#include "../PluginOptionsBus.h"

QT_FORWARD_DECLARE_CLASS(QSettings);

namespace EMStudio
{
    class EMSTUDIO_API RenderOptions 
        : public PluginOptions
    {
    public:
        AZ_RTTI(RenderOptions, "{D661AA14-F61D-4917-9F19-2B32494556B1}", PluginOptions);
        
        static const char* s_gridUnitSizeOptionName;
        static const char* s_vertexNormalsScaleOptionName;
        static const char* s_faceNormalsScaleOptionName;
        static const char* s_tangentsScaleOptionName;
        static const char* s_nodeOrientationScaleOptionName;
        static const char* s_scaleBonesOnLengthOptionName;
        static const char* s_nearClipPlaneDistanceOptionName;
        static const char* s_farClipPlaneDistanceOptionName;
        static const char* s_FOVOptionName;
        static const char* s_mainLightIntensityOptionName;
        static const char* s_mainLightAngleAOptionName;
        static const char* s_mainLightAngleBOptionName;
        static const char* s_specularIntensityOptionName;
        static const char* s_rimIntensityOptionName;
        static const char* s_rimWidthOptionName;
        static const char* s_rimAngleOptionName;
        static const char* s_showFPSOptionName;
        static const char* s_lightGroundColorOptionName;
        static const char* s_lightSkyColorOptionName;
        static const char* s_rimColorOptionName;
        static const char* s_backgroundColorOptionName;
        static const char* s_gradientSourceColorOptionName;
        static const char* s_gradientTargetColorOptionName;
        static const char* s_wireframeColorOptionName;
        static const char* s_collisionMeshColorOptionName;
        static const char* s_vertexNormalsColorOptionName;
        static const char* s_faceNormalsColorOptionName;
        static const char* s_tangentsColorOptionName;
        static const char* s_mirroredBitangentsColorOptionName;
        static const char* s_bitangentsColorOptionName;
        static const char* s_nodeAABBColorOptionName;
        static const char* s_staticAABBColorOptionName;
        static const char* s_meshAABBColorOptionName;
        static const char* s_lineSkeletonColorOptionName;
        static const char* s_skeletonColorOptionName;
        static const char* s_selectionColorOptionName;
        static const char* s_selectedObjectColorOptionName;
        static const char* s_nodeNameColorOptionName;
        static const char* s_gridColorOptionName;
        static const char* s_mainAxisColorOptionName;
        static const char* s_subStepColorOptionName;
        static const char* s_trajectoryArrowInnerColorOptionName;
        static const char* s_hitDetectionColliderColorOptionName;
        static const char* s_selectedHitDetectionColliderColorOptionName;
        static const char* s_ragdollColliderColorOptionName;
        static const char* s_selectedRagdollColliderColorOptionName;
        static const char* s_violatedJointLimitColorOptionName;
        static const char* s_clothColliderColorOptionName;
        static const char* s_selectedClothColliderColorOptionName;
        static const char* s_lastUsedLayoutOptionName;
        static const char* s_renderSelectionBoxOptionName;

        RenderOptions();
        ~RenderOptions();
        RenderOptions& operator=(const RenderOptions& other);

        void Save(QSettings* settings);
        static RenderOptions Load(QSettings* settings);

        static AZ::Color StringToColor(const QString& text);
        static QString ColorToString(const AZ::Color& color);
    
        static void Reflect(AZ::ReflectContext* context);

        float GetGridUnitSize() const { return m_gridUnitSize; }
        void SetGridUnitSize(float gridUnitSize);

        float GetVertexNormalsScale() const { return m_vertexNormalsScale; }
        void SetVertexNormalsScale(float vertexNormalsScale);

        float GetFaceNormalsScale() const { return m_faceNormalsScale; }
        void SetFaceNormalsScale(float faceNormalsScale);

        float GetTangentsScale() const { return m_tangentsScale; }
        void SetTangentsScale(float tangentsScale);

        float GetNodeOrientationScale() const { return m_nodeOrientationScale; }
        void SetNodeOrientationScale(float nodeOrientationScale);

        bool GetScaleBonesOnLength() const { return m_scaleBonesOnLength; }
        void SetScaleBonesOnLenght(bool scaleBonesOnLenght);

        float GetNearClipPlaneDistance() const { return m_nearClipPlaneDistance; }
        void SetNearClipPlaneDistance(float nearClipPlaneDistance);

        float GetFarClipPlaneDistance() const { return m_farClipPlaneDistance; }
        void SetFarClipPlaneDistance(float farClipPlaneDistance);

        float GetFOV() const { return m_fov; }
        void SetFOV(float FOV);

        float GetMainLightIntensity() const { return m_mainLightIntensity; }
        void SetMainLightIntensity(float mainLightIntensity);

        float GetMainLightAngleA() const { return m_mainLightAngleA; }
        void SetMainLightAngleA(float mainLightAngleA);

        float GetMainLightAngleB() const { return m_mainLightAngleB; }
        void SetMainLightAngleB(float mainLightAngleB);

        float GetSpecularIntensity() const { return m_specularIntensity; }
        void SetSpecularIntensity(float specularIntensity);

        float GetRimIntensity() const { return m_rimIntensity; }
        void SetRimIntensity(float rimIntensity);

        float GetRimWidth() const { return m_rimWidth; }
        void SetRimWidth(float rimWidth);

        float GetRimAngle() const { return m_rimAngle; }
        void SetRimAngle(float rimAngle);

        bool GetShowFPS() const { return m_showFPS; }
        void SetShowFPS(bool showFPS);

        AZ::Color GetLightGroundColor() const { return m_lightGroundColor; }
        void SetLightGroundColor(const AZ::Color& lightGroundColor);

        AZ::Color GetLightSkyColor() const { return m_lightSkyColor; }
        void SetLightSkyColor(const AZ::Color& lightSkyColor);
        
        AZ::Color GetRimColor() const { return m_rimColor; }
        void SetRimColor(const AZ::Color& rimColor);

        AZ::Color GetBackgroundColor() const { return m_backgroundColor; }
        void SetBackgroundColor(const AZ::Color& backgroundColor);

        AZ::Color GetGradientSourceColor() const { return m_gradientSourceColor; }
        void SetGradientSourceColor(const AZ::Color& gradientSourceColor);
        
        AZ::Color GetGradientTargetColor() const { return m_gradientTargetColor; }
        void SetGradientTargetColor(const AZ::Color& gradientTargetColor);

        AZ::Color GetWireframeColor() const { return m_wireframeColor; }
        void SetWireframeColor(const AZ::Color& wireframeColor);

        AZ::Color GetCollisionMeshColor() const { return m_collisionMeshColor; }
        void SetCollisionMeshColor(const AZ::Color& collisionMeshColor);

        AZ::Color GetVertexNormalsColor() const { return m_vertexNormalsColor; }
        void SetVertexNormalsColor(const AZ::Color& vertexNormalsColor);

        AZ::Color GetFaceNormalsColor() const { return m_faceNormalsColor; }
        void SetFaceNormalsColor(const AZ::Color& faceNormalsColor);

        AZ::Color GetTangentsColor() const { return m_tangentsColor; }
        void SetTangentsColor(const AZ::Color& tangentsColor);

        AZ::Color GetMirroredBitangentsColor() const { return m_mirroredBitangentsColor; }
        void SetMirroredBitangentsColor(const AZ::Color& mirroredBitangentsColor);

        AZ::Color GetBitangentsColor() const { return m_bitangentsColor; }
        void SetBitangentsColor(const AZ::Color& bitangentsColor);

        AZ::Color GetNodeAABBColor() const { return m_nodeAABBColor; }
        void SetNodeAABBColor(const AZ::Color& nodeAABBColor);

        AZ::Color GetStaticAABBColor() const { return m_staticAABBColor; }
        void SetStaticAABBColor(const AZ::Color& staticAABBColor);

        AZ::Color GetMeshAABBColor() const { return m_meshAABBColor; }
        void SetMeshAABBColor(const AZ::Color& meshAABBColor);

        AZ::Color GetLineSkeletonColor() const { return m_lineSkeletonColor; }
        void SetLineSkeletonColor(const AZ::Color& lineSkeletonColor);

        AZ::Color GetSkeletonColor() const { return m_skeletonColor; }
        void SetSkeletonColor(const AZ::Color& skeletonColor);

        AZ::Color GetSelectionColor() const { return m_selectionColor; }
        void SetSelectionColor(const AZ::Color& selectionColor);

        AZ::Color GetSelectedObjectColor() const { return m_selectedObjectColor; }
        void SetSelectedObjectColor(const AZ::Color& selectedObjectColor);

        AZ::Color GetNodeNameColor() const { return m_nodeNameColor; }
        void SetNodeNameColor(const AZ::Color& nodeNameColor);

        AZ::Color GetGridColor() const { return m_gridColor; }
        void SetGridColor(const AZ::Color& gridColor);

        AZ::Color GetMainAxisColor() const { return m_mainAxisColor; }
        void SetMainAxisColor(const AZ::Color& mainAxisColor);

        AZ::Color GetSubStepColor() const { return m_subStepColor; }
        void SetSubStepColor(const AZ::Color& subStepColor);

        AZ::Color GetHitDetectionColliderColor() const { return m_hitDetectionColliderColor; }
        void SetHitDetectionColliderColor(const AZ::Color& colliderColor);

        AZ::Color GetSelectedHitDetectionColliderColor() const { return m_selectedHitDetectionColliderColor; }
        void SetSelectedHitDetectionColliderColor(const AZ::Color& colliderColor);

        AZ::Color GetRagdollColliderColor() const { return m_ragdollColliderColor; }
        void SetRagdollColliderColor(const AZ::Color& color);

        AZ::Color GetSelectedRagdollColliderColor() const { return m_selectedRagdollColliderColor; }
        void SetSelectedRagdollColliderColor(const AZ::Color& color);

        AZ::Color GetViolatedJointLimitColor() const { return m_violatedJointLimitColor; }
        void SetViolatedJointLimitColor(const AZ::Color& color);

        AZ::Color GetClothColliderColor() const { return m_clothColliderColor; }
        void SetClothColliderColor(const AZ::Color& colliderColor);

        AZ::Color GetSelectedClothColliderColor() const { return m_selectedClothColliderColor; }
        void SetSelectedClothColliderColor(const AZ::Color& colliderColor);

        AZ::Color GetSimulatedObjectColliderColor() const { return m_simulatedObjectColliderColor; }
        void SetSimulatedObjectColliderColor(const AZ::Color& colliderColor);

        AZ::Color GetSelectedSimulatedObjectColliderColor() const { return m_selectedSimulatedObjectColliderColor; }
        void SetSelectedSimulatedColliderObjectColor(const AZ::Color& colliderColor);

        AZ::Color GetTrajectoryArrowInnerColor() const { return m_trajectoryArrowInnerColor; }
        void SetTrajectoryArrowInnerColor(const AZ::Color& trajectoryArrowInnerColor);
        
        AZStd::string GetLastUsedLayout() const { return m_lastUsedLayout; }
        void SetLastUsedLayout(const AZStd::string& lastUsedLayout);

        bool GetRenderSelectionBox() const { return m_renderSelectionBox; }
        void SetRenderSelectionBox(bool renderSelectionBox);

        enum ManipulatorMode
        {
            SELECT = 0,
            TRANSLATE = 1,
            ROTATE = 2,
            SCALE = 3,
            NUM_MODES = 4
        };

        void SetManipulatorMode(ManipulatorMode mode);
        ManipulatorMode GetManipulatorMode() const;

    private:
        void OnGridUnitSizeChangedCallback() const;
        void OnVertexNormalsScaleChangedCallback() const;
        void OnFaceNormalsScaleChangedCallback() const;
        void OnTangentsScaleChangedCallback() const;
        void OnNodeOrientationScaleChangedCallback() const;
        void OnScaleBonesOnLengthChangedCallback() const;
        void OnNearClipPlaneDistanceChangedCallback() const;
        void OnFarClipPlaneDistanceChangedCallback() const;
        void OnFOVChangedCallback() const;
        void OnMainLightIntensityChangedCallback() const;
        void OnMainLightAngleAChangedCallback() const;
        void OnMainLightAngleBChangedCallback() const;
        void OnSpecularIntensityChangedCallback() const;
        void OnRimIntensityChangedCallback() const;
        void OnRimWidthChangedCallback() const;
        void OnRimAngleChangedCallback() const;
        void OnShowFPSChangedCallback() const;
        void OnLightGroundColorChangedCallback() const;
        void OnLightSkyColorChangedCallback() const;
        void OnRimColorChangedCallback() const;
        void OnBackgroundColorChangedCallback() const;
        void OnGradientSourceColorChangedCallback() const;
        void OnGradientTargetColorChangedCallback() const;
        void OnWireframeColorChangedCallback() const;
        void OnCollisionMeshColorChangedCallback() const;
        void OnVertexNormalsColorChangedCallback() const;
        void OnFaceNormalsColorChangedCallback() const;
        void OnTangentsColorChangedCallback() const;
        void OnMirroredBitangentsColorChangedCallback() const;
        void OnBitangentsColorChangedCallback() const;
        void OnNodeAABBColorChangedCallback() const;
        void OnStaticAABBColorChangedCallback() const;
        void OnMeshAABBColorChangedCallback() const;
        void OnLineSkeletonColorChangedCallback() const;
        void OnSkeletonColorChangedCallback() const;
        void OnSelectionColorChangedCallback() const;
        void OnSelectedObjectColorChangedCallback() const;
        void OnNodeNameColorChangedCallback() const;
        void OnGridColorChangedCallback() const;
        void OnMainAxisColorChangedCallback() const;
        void OnSubStepColorChangedCallback() const;
        void OnTrajectoryArrowInnerColorChangedCallback() const;
        void OnHitDetectionColliderColorChangedCallback() const;
        void OnSelectedHitDetectionColliderColorChangedCallback() const;
        void OnRagdollColliderColorChangedCallback() const;
        void OnSelectedRagdollColliderColorChangedCallback() const;
        void OnViolatedJointLimitColorChangedCallback() const;
        void OnClothColliderColorChangedCallback() const;
        void OnSelectedClothColliderColorChangedCallback() const;
        void OnLastUsedLayoutChangedCallback() const;
        void OnRenderSelectionBoxChangedCallback() const;

        // Maintain the order between here and the reflect method.
        // The order in the SerializeContext defines the order it is shown in the UI
        float            m_gridUnitSize;
        float            m_vertexNormalsScale;
        float            m_faceNormalsScale;
        float            m_tangentsScale;
        float            m_nodeOrientationScale;
        bool             m_scaleBonesOnLength;
        float            m_nearClipPlaneDistance;
        float            m_farClipPlaneDistance;
        float            m_fov;
        float            m_mainLightIntensity;
        float            m_mainLightAngleA;
        float            m_mainLightAngleB;
        float            m_specularIntensity;
        float            m_rimIntensity;
        float            m_rimWidth;
        float            m_rimAngle;
        bool             m_showFPS;

        // Colors
        AZ::Color        m_lightGroundColor;
        AZ::Color        m_lightSkyColor;
        AZ::Color        m_rimColor;
        AZ::Color        m_backgroundColor;
        AZ::Color        m_gradientSourceColor;
        AZ::Color        m_gradientTargetColor;
        AZ::Color        m_wireframeColor;
        AZ::Color        m_collisionMeshColor;
        AZ::Color        m_vertexNormalsColor;
        AZ::Color        m_faceNormalsColor;
        AZ::Color        m_tangentsColor;
        AZ::Color        m_mirroredBitangentsColor;
        AZ::Color        m_bitangentsColor;
        AZ::Color        m_nodeAABBColor;
        AZ::Color        m_staticAABBColor;
        AZ::Color        m_meshAABBColor;
        AZ::Color        m_lineSkeletonColor;
        AZ::Color        m_skeletonColor;
        AZ::Color        m_selectionColor;
        AZ::Color        m_selectedObjectColor;
        AZ::Color        m_nodeNameColor;
        AZ::Color        m_gridColor;
        AZ::Color        m_mainAxisColor;
        AZ::Color        m_subStepColor;
        AZ::Color        m_trajectoryArrowInnerColor;
        AZ::Color        m_hitDetectionColliderColor;
        AZ::Color        m_selectedHitDetectionColliderColor;
        AZ::Color        m_ragdollColliderColor;
        AZ::Color        m_selectedRagdollColliderColor;
        AZ::Color        m_violatedJointLimitColor;
        AZ::Color        m_clothColliderColor;
        AZ::Color        m_selectedClothColliderColor;
        AZ::Color        m_simulatedObjectColliderColor;
        AZ::Color        m_selectedSimulatedObjectColliderColor;

        // The following are not in the  UI
        AZStd::string    m_lastUsedLayout;
        bool             m_renderSelectionBox;
        ManipulatorMode  m_manipulatorMode = SELECT;
    };

} // namespace EMStudio
