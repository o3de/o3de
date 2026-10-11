/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace EMotionFX
{
    class ActorInstance;
    class ActorSocket;

    // Move and rotate gizmos for one socket in the Animation Editor viewport, with a Move/Rotate cluster to switch between them.
    // The gizmos follow the socket's data every tick; a drag only previews the new value and is committed as one undoable command on mouse up.
    class SocketManipulators
        : private AZ::TickBus::Handler
    {
    public:
        enum class Mode
        {
            Translate,
            Rotate
        };

        SocketManipulators();
        ~SocketManipulators();

        // Shows the gizmos for the named socket of the actor instance; a null instance or an empty name hides them.
        void SetTarget(ActorInstance* actorInstance, const AZStd::string& socketName);

        // Hides or shows the gizmos and the Move/Rotate cluster, e.g. when the viewport's Sockets render option is turned off.
        void SetVisible(bool visible);

        // The in-progress local position and rotation while a gizmo is being dragged; false when nothing is being dragged.
        bool GetPreview(const AZStd::string& socketName, AZ::Vector3& outPosition, AZ::Quaternion& outRotation) const;

    private:
        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        const ActorSocket* FindSocket() const;
        // The joint's world frame without scale, and the joint's uniform world scale.
        bool GetJointFrame(const ActorSocket& socket, AZ::Transform& outFrame, float& outScale) const;
        AZ::s32 GetViewportId() const;

        bool IsShown() const;
        void Show(const ActorSocket& socket, const AZ::Transform& frame, float scale);
        void Hide();
        void SetMode(Mode mode);

        void CreateCluster();
        void DestroyCluster();
        void UpdateActiveButton();

        void BeginDrag();
        void OnTranslated(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void EndTranslation(const AZ::Vector3& startPosition, const AZ::Vector3& offset);
        void OnRotated(const AZ::Quaternion& rotation);
        void EndRotation(const AZ::Quaternion& rotation);
        void CommitPosition(const AZ::Vector3& position);
        void CommitRotation(const AZ::Quaternion& rotation);

        ActorInstance* m_actorInstance = nullptr;
        AZStd::string m_socketName;
        Mode m_mode = Mode::Translate;
        bool m_visible = true;

        AzToolsFramework::TranslationManipulators m_translationManipulators;
        AzToolsFramework::RotationManipulators m_rotationManipulators;
        bool m_translationShown = false;
        bool m_rotationShown = false;

        // The joint's uniform world scale, refreshed every tick; manipulator units are the socket's local units times this.
        float m_scale = 1.0f;
        bool m_dragging = false;
        AZ::Vector3 m_previewPosition = AZ::Vector3::CreateZero();
        AZ::Quaternion m_previewRotation = AZ::Quaternion::CreateIdentity();

        AzToolsFramework::ViewportUi::ClusterId m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        AzToolsFramework::ViewportUi::ButtonId m_moveButtonId = AzToolsFramework::ViewportUi::InvalidButtonId;
        AzToolsFramework::ViewportUi::ButtonId m_rotateButtonId = AzToolsFramework::ViewportUi::InvalidButtonId;
        AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler m_clusterHandler;
    };
} // namespace EMotionFX
