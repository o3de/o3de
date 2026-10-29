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
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    class TranslationManipulators;
    class RotationManipulators;
    class ScaleManipulators;
} // namespace AzToolsFramework

namespace WhiteBox
{
    //! Entity transform gizmo owned by the White Box pane; the editor's own is unavailable during component mode.
    class WhiteBoxEntityGizmo : private AZ::TransformNotificationBus::Handler
    {
    public:
        enum class Mode
        {
            None,
            Translate,
            Rotate,
            Scale
        };

        WhiteBoxEntityGizmo();
        ~WhiteBoxEntityGizmo();

        void SetTarget(AZ::EntityId entityId); //!< Rebuilds if a mode is active.
        void SetMode(Mode mode);
        Mode GetMode() const { return m_mode; }
        void Refresh(); //!< Re-read the entity transform (external edits, undo, etc.).
        //! Invoked when a gizmo drag finishes, so the owner (the pane) can refresh its controls.
        void SetChangedCallback(AZStd::function<void()> callback) { m_changedCallback = AZStd::move(callback); }

    private:
        // AZ::TransformNotificationBus overrides ... (keep the gizmo on the entity as it moves)
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        void Rebuild();
        void Destroy();

        void BeginBatch(const char* label);
        void EndBatch();

        AZ::EntityId m_entityId;
        Mode m_mode = Mode::None;

        AZStd::unique_ptr<AzToolsFramework::TranslationManipulators> m_translation;
        AZStd::unique_ptr<AzToolsFramework::RotationManipulators> m_rotation;
        AZStd::unique_ptr<AzToolsFramework::ScaleManipulators> m_scale;

        float m_startScale = 1.0f; //!< Entity uniform scale at scale-drag start.
        bool m_batchActive = false; //!< A drag is in progress. No engine undo batch is held open for it.
        AZStd::string m_batchLabel; //!< Undo label of the drag in progress.
        bool m_applying = false; //!< Guard: the gizmo itself is writing the transform.
        AZStd::function<void()> m_changedCallback;
    };
} // namespace WhiteBox
