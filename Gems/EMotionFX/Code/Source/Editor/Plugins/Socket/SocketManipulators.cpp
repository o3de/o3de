/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <EMotionFX/CommandSystem/Source/SocketCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/ViewportPluginBus.h>
#include <Editor/Plugins/Socket/SocketManipulators.h>

namespace EMotionFX
{
    namespace
    {
        AzToolsFramework::ViewportUi::ButtonId CreateClusterButton(
            AZ::s32 viewportId, AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName, const char* tooltip)
        {
            AzToolsFramework::ViewportUi::ButtonId buttonId;
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
                AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, clusterId, buttonId, tooltip);
            return buttonId;
        }
    }

    SocketManipulators::SocketManipulators()
        : m_translationManipulators(
              AzToolsFramework::TranslationManipulators::Dimensions::Three, AZ::Transform::CreateIdentity(), AZ::Vector3::CreateOne())
        , m_rotationManipulators(AZ::Transform::CreateIdentity())
    {
        m_rotationManipulators.SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
    }

    SocketManipulators::~SocketManipulators()
    {
        SetTarget(nullptr, AZStd::string());
    }

    void SocketManipulators::SetTarget(ActorInstance* actorInstance, const AZStd::string& socketName)
    {
        const bool hasTarget = actorInstance && !socketName.empty();
        if (hasTarget && actorInstance == m_actorInstance && socketName == m_socketName)
        {
            return;
        }

        Hide();
        m_dragging = false;

        if (hasTarget)
        {
            m_actorInstance = actorInstance;
            m_socketName = socketName;
            if (m_visible)
            {
                CreateCluster();
            }
            if (!AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusConnect();
            }
        }
        else
        {
            m_actorInstance = nullptr;
            m_socketName.clear();
            AZ::TickBus::Handler::BusDisconnect();
            DestroyCluster();
        }
    }

    void SocketManipulators::SetVisible(bool visible)
    {
        if (visible == m_visible)
        {
            return;
        }

        m_visible = visible;
        if (!m_visible)
        {
            Hide();
            m_dragging = false;
            DestroyCluster();
        }
        else if (m_actorInstance)
        {
            // The next tick shows the gizmo again.
            CreateCluster();
        }
    }

    bool SocketManipulators::GetPreview(const AZStd::string& socketName, AZ::Vector3& outPosition, AZ::Quaternion& outRotation) const
    {
        if (!m_dragging || socketName != m_socketName)
        {
            return false;
        }

        outPosition = m_previewPosition;
        outRotation = m_previewRotation;
        return true;
    }

    const ActorSocket* SocketManipulators::FindSocket() const
    {
        return m_actorInstance ? m_actorInstance->GetActor()->GetSocketSetup()->FindSocket(m_socketName) : nullptr;
    }

    bool SocketManipulators::GetJointFrame(const ActorSocket& socket, AZ::Transform& outFrame, float& outScale) const
    {
        if (!m_actorInstance || socket.GetJointIndex() >= m_actorInstance->GetActor()->GetNumNodes())
        {
            return false;
        }

        const AZ::Transform jointWorld =
            m_actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(socket.GetJointIndex()).ToAZTransform();
        outScale = AZ::GetMax(AZ::MinTransformScale, jointWorld.GetUniformScale());
        outFrame = AZ::Transform::CreateFromQuaternionAndTranslation(jointWorld.GetRotation(), jointWorld.GetTranslation());
        return true;
    }

    AZ::s32 SocketManipulators::GetViewportId() const
    {
        AZ::s32 viewportId = -1;
        EMStudio::ViewportPluginRequestBus::BroadcastResult(viewportId, &EMStudio::ViewportPluginRequestBus::Events::GetViewportId);
        return viewportId;
    }

    bool SocketManipulators::IsShown() const
    {
        return m_mode == Mode::Translate ? m_translationShown : m_rotationShown;
    }

    void SocketManipulators::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_visible)
        {
            return;
        }

        const ActorSocket* socket = FindSocket();
        AZ::Transform frame = AZ::Transform::CreateIdentity();
        float scale = 1.0f;
        if (!socket || !GetJointFrame(*socket, frame, scale))
        {
            Hide();
            return;
        }

        m_scale = scale;
        if (!IsShown())
        {
            Show(*socket, frame, scale);
        }

        // The joint moves while an animation plays, so the gizmo space follows it; the values only follow the data when not dragging.
        if (m_translationShown)
        {
            m_translationManipulators.SetSpace(frame);
            if (!m_dragging)
            {
                m_translationManipulators.SetLocalPosition(socket->GetLocalPosition() * scale);
            }
        }
        if (m_rotationShown)
        {
            m_rotationManipulators.SetSpace(frame);
            if (!m_dragging)
            {
                m_rotationManipulators.SetLocalPosition(socket->GetLocalPosition() * scale);
                m_rotationManipulators.SetLocalOrientation(socket->GetLocalRotation());
            }
            m_rotationManipulators.RefreshView(AzToolsFramework::GetCameraState(GetViewportId()).m_position);
        }
    }

    void SocketManipulators::Show(const ActorSocket& socket, const AZ::Transform& frame, float scale)
    {
        if (m_mode == Mode::Translate)
        {
            m_translationManipulators.SetSpace(frame);
            m_translationManipulators.SetLocalPosition(socket.GetLocalPosition() * scale);
            m_translationManipulators.Register(EMStudio::g_animManipulatorManagerId);
            AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(&m_translationManipulators);

            m_translationManipulators.InstallLinearManipulatorMouseDownCallback(
                [this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
                {
                    BeginDrag();
                });
            m_translationManipulators.InstallPlanarManipulatorMouseDownCallback(
                [this]([[maybe_unused]] const AzToolsFramework::PlanarManipulator::Action& action)
                {
                    BeginDrag();
                });
            m_translationManipulators.InstallSurfaceManipulatorMouseDownCallback(
                [this]([[maybe_unused]] const AzToolsFramework::SurfaceManipulator::Action& action)
                {
                    BeginDrag();
                });

            m_translationManipulators.InstallLinearManipulatorMouseMoveCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    OnTranslated(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
                });
            m_translationManipulators.InstallPlanarManipulatorMouseMoveCallback(
                [this](const AzToolsFramework::PlanarManipulator::Action& action)
                {
                    OnTranslated(action.m_start.m_localPosition, action.m_current.m_localOffset);
                });
            m_translationManipulators.InstallSurfaceManipulatorMouseMoveCallback(
                [this](const AzToolsFramework::SurfaceManipulator::Action& action)
                {
                    OnTranslated(action.m_start.m_localPosition, action.m_current.m_localOffset);
                });

            m_translationManipulators.InstallLinearManipulatorMouseUpCallback(
                [this](const AzToolsFramework::LinearManipulator::Action& action)
                {
                    EndTranslation(action.m_start.m_localPosition, action.m_current.m_localPositionOffset);
                });
            m_translationManipulators.InstallPlanarManipulatorMouseUpCallback(
                [this](const AzToolsFramework::PlanarManipulator::Action& action)
                {
                    EndTranslation(action.m_start.m_localPosition, action.m_current.m_localOffset);
                });
            m_translationManipulators.InstallSurfaceManipulatorMouseUpCallback(
                [this](const AzToolsFramework::SurfaceManipulator::Action& action)
                {
                    EndTranslation(action.m_start.m_localPosition, action.m_current.m_localOffset);
                });

            m_translationShown = true;
        }
        else
        {
            m_rotationManipulators.SetSpace(frame);
            m_rotationManipulators.SetLocalPosition(socket.GetLocalPosition() * scale);
            m_rotationManipulators.SetLocalOrientation(socket.GetLocalRotation());
            m_rotationManipulators.Register(EMStudio::g_animManipulatorManagerId);
            m_rotationManipulators.SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
            m_rotationManipulators.ConfigureView(
                2.0f, AzFramework::ViewportColors::XAxisColor, AzFramework::ViewportColors::YAxisColor,
                AzFramework::ViewportColors::ZAxisColor);

            m_rotationManipulators.InstallLeftMouseDownCallback(
                [this]([[maybe_unused]] const AzToolsFramework::AngularManipulator::Action& action)
                {
                    BeginDrag();
                });
            m_rotationManipulators.InstallMouseMoveCallback(
                [this](const AzToolsFramework::AngularManipulator::Action& action)
                {
                    OnRotated(action.LocalOrientation());
                });
            m_rotationManipulators.InstallLeftMouseUpCallback(
                [this](const AzToolsFramework::AngularManipulator::Action& action)
                {
                    EndRotation(action.LocalOrientation());
                });

            m_rotationShown = true;
        }
    }

    void SocketManipulators::Hide()
    {
        if (m_translationShown)
        {
            m_translationManipulators.Unregister();
            m_translationShown = false;
        }
        if (m_rotationShown)
        {
            m_rotationManipulators.Unregister();
            m_rotationShown = false;
        }
    }

    void SocketManipulators::SetMode(Mode mode)
    {
        if (mode == m_mode)
        {
            return;
        }

        // The next tick shows the gizmo of the new mode.
        Hide();
        m_dragging = false;
        m_mode = mode;
        UpdateActiveButton();
    }

    void SocketManipulators::CreateCluster()
    {
        if (m_clusterId != AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            return;
        }

        const AZ::s32 viewportId = GetViewportId();
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_clusterId, viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            AzToolsFramework::ViewportUi::Alignment::TopLeft);
        m_moveButtonId = CreateClusterButton(viewportId, m_clusterId, "Move", "Move the socket");
        m_rotateButtonId = CreateClusterButton(viewportId, m_clusterId, "Rotate", "Rotate the socket");

        m_clusterHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_moveButtonId)
                {
                    SetMode(Mode::Translate);
                }
                else if (buttonId == m_rotateButtonId)
                {
                    SetMode(Mode::Rotate);
                }
            });
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_clusterId,
            m_clusterHandler);

        UpdateActiveButton();
    }

    void SocketManipulators::DestroyCluster()
    {
        if (m_clusterId == AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            return;
        }

        m_clusterHandler.Disconnect();
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            GetViewportId(), &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_clusterId);
        m_clusterId = AzToolsFramework::ViewportUi::InvalidClusterId;
        m_moveButtonId = AzToolsFramework::ViewportUi::InvalidButtonId;
        m_rotateButtonId = AzToolsFramework::ViewportUi::InvalidButtonId;
    }

    void SocketManipulators::UpdateActiveButton()
    {
        if (m_clusterId == AzToolsFramework::ViewportUi::InvalidClusterId)
        {
            return;
        }

        const AZ::s32 viewportId = GetViewportId();
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_clusterId);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_clusterId,
            m_mode == Mode::Translate ? m_moveButtonId : m_rotateButtonId);
    }

    void SocketManipulators::BeginDrag()
    {
        const ActorSocket* socket = FindSocket();
        if (!socket)
        {
            return;
        }

        m_previewPosition = socket->GetLocalPosition();
        m_previewRotation = socket->GetLocalRotation();
        m_dragging = true;
    }

    void SocketManipulators::OnTranslated(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        if (!m_dragging)
        {
            return;
        }

        const AZ::Vector3 manipulatorPosition = startPosition + offset;
        m_previewPosition = manipulatorPosition / m_scale;
        m_translationManipulators.SetLocalPosition(manipulatorPosition);
    }

    void SocketManipulators::EndTranslation(const AZ::Vector3& startPosition, const AZ::Vector3& offset)
    {
        if (!m_dragging)
        {
            return;
        }

        CommitPosition((startPosition + offset) / m_scale);
        m_dragging = false;
    }

    void SocketManipulators::OnRotated(const AZ::Quaternion& rotation)
    {
        if (!m_dragging)
        {
            return;
        }

        m_previewRotation = rotation;
        m_rotationManipulators.SetLocalOrientation(rotation);
    }

    void SocketManipulators::EndRotation(const AZ::Quaternion& rotation)
    {
        if (!m_dragging)
        {
            return;
        }

        CommitRotation(rotation);
        m_dragging = false;
    }

    void SocketManipulators::CommitPosition(const AZ::Vector3& position)
    {
        if (m_actorInstance)
        {
            CommandSocketHelpers::AdjustSocket(
                m_actorInstance->GetActor()->GetID(), m_socketName, AZStd::nullopt, AZStd::nullopt, position, AZStd::nullopt);
        }
    }

    void SocketManipulators::CommitRotation(const AZ::Quaternion& rotation)
    {
        if (m_actorInstance)
        {
            CommandSocketHelpers::AdjustSocket(
                m_actorInstance->GetActor()->GetID(), m_socketName, AZStd::nullopt, AZStd::nullopt, AZStd::nullopt, rotation);
        }
    }
} // namespace EMotionFX
