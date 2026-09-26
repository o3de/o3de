/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */
#pragma once

#include "EditorWhiteBoxComponentModeTypes.h"
#include "SubComponentModes/WhiteBoxPaintSettings.h"
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    class ScopedUndoBatch;
}

namespace AzFramework
{
    struct ViewportInfo;
    class DebugDisplayRequests;
}

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! Paint individual mesh triangles without changing polygon groupings or topology.
    class PaintMode
    {
    public:
        explicit PaintMode(const AZ::EntityComponentIdPair& entityComponentIdPair);
        ~PaintMode();
        void Refresh();
        bool HandleMouseInteraction(const ModeMouseInteraction& mouse);
        void Display(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            const AZ::Transform& worldFromLocal,
            const IntersectionAndRenderData& renderData,
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay);
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActions(const AZ::EntityComponentIdPair&);
        bool CancelActiveDrag();
        bool HandleEscape() { return CancelActiveDrag(); }

    private:
        EditorWhiteBoxComponent* Component() const;
        AZ::Vector3 WorldPoint(EditorWhiteBoxComponent& component, const AZ::Vector3& point) const;
        //! The face under the cursor; the hit point and face normal (world space) go to the optional outputs.
        Api::FaceHandle PickFace(
            EditorWhiteBoxComponent& component, const ModeMouseInteraction& mouse, AZ::Vector3* hitPoint = nullptr,
            AZ::Vector3* hitNormal = nullptr) const;
        //! One dab of the blend brush at a world point: every vertex in reach moves towards the chosen layer.
        void PaintBlend(EditorWhiteBoxComponent& component, const AZ::Vector3& worldPoint);
        void Paint(EditorWhiteBoxComponent& component, Api::FaceHandle face);
        void FinishStroke(bool commit);
        void NotifyMeshChanged() const;

        AZ::EntityComponentIdPair m_entityComponentIdPair;
        Api::FaceHandle m_hover;
        AZ::Vector3 m_hoverPoint = AZ::Vector3::CreateZero();  //!< World hit under the cursor, for the brush circle.
        AZ::Vector3 m_hoverNormal = AZ::Vector3::CreateAxisZ();
        FacePaintSettings m_settings;
        Api::WhiteBoxMeshPtr m_snapshot;
        WhiteBoxMesh* m_strokeMesh = nullptr;
        AZStd::unique_ptr<AzToolsFramework::ScopedUndoBatch> m_undo;
        Api::FaceHandles m_paintedFaces;
        bool m_changed = false;
    };
}
