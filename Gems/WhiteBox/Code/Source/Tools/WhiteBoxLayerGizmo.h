/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    class TranslationManipulators;
    class RotationManipulators;
    class ScaleManipulators;
} // namespace AzToolsFramework

namespace WhiteBox
{
    class EditorWhiteBoxComponent;

    //! Translate / rotate / scale gizmo for a layer's Position / Rotation / Scale, driven by the pane.
    class WhiteBoxLayerGizmo : private AZ::TransformNotificationBus::Handler
    {
    public:
        enum class Mode
        {
            None,
            Translate,
            Rotate,
            Scale
        };

        WhiteBoxLayerGizmo();
        ~WhiteBoxLayerGizmo();

        //! Which entity/layer the gizmo edits. Rebuilds the manipulators if a mode is active.
        void SetTarget(AZ::EntityId entityId, int layerIndex);
        //! Show the given manipulator type (None hides the gizmo).
        void SetMode(Mode mode);
        Mode GetMode() const { return m_mode; }
        //! Re-read the layer meta / entity transform (call after external edits, undo, etc.).
        void Refresh();
        //! Invoked when a gizmo drag finishes, so the owner (the pane) can refresh its controls.
        void SetChangedCallback(AZStd::function<void()> callback) { m_changedCallback = AZStd::move(callback); }

    private:
        // AZ::TransformNotificationBus overrides ... (follow the entity as it moves)
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void Rebuild();
        void Destroy();

        EditorWhiteBoxComponent* Component() const;
        AZ::Transform Space() const; //!< The entity's world transform (manipulator space).

        void BeginBatch(const char* label);
        void EndBatch();

        //! Apply a translate drag, snapping the layer onto a nearby vertex when Snap to Vertex is
        //! on. @p localPosition is the manipulator's unsnapped position (entity-local).
        void ApplyPositionSnapped(const AZ::Vector3& localPosition);

        void ApplyPosition(const AZ::Vector3& localPosition);
        void ApplyRotation(const AZ::Quaternion& localOrientation);
        void ApplyScale(const AZ::Vector3& scale);

        AZ::EntityId m_entityId;
        int m_layerIndex = -1;
        Mode m_mode = Mode::None;

        AZStd::unique_ptr<AzToolsFramework::TranslationManipulators> m_translation;
        AZStd::unique_ptr<AzToolsFramework::RotationManipulators> m_rotation;
        AZStd::unique_ptr<AzToolsFramework::ScaleManipulators> m_scale;

        //! Snap anchor (translate only): offset from the layer origin to the anchor vertex, held for the drag.
        AZStd::optional<AZ::Vector3> m_snapAnchorOffset;
        bool m_snapAnchorResolved = false;

        AZ::Vector3 m_startScale = AZ::Vector3::CreateOne(); //!< Layer scale at scale-drag start.
        //! A drag is in progress. No engine undo batch is held open for it (see EndBatch).
        bool m_batchActive = false;
        AZStd::string m_batchLabel; //!< Undo label of the drag in progress.
        AZStd::function<void()> m_changedCallback; //!< Owner notification after a finished drag.
    };
} // namespace WhiteBox
