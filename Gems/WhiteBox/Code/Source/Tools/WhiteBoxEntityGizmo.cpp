/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxEntityGizmo.h"

#include <AzCore/Math/Quaternion.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/RotationManipulators.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace WhiteBox
{
    WhiteBoxEntityGizmo::~WhiteBoxEntityGizmo()
    {
        Destroy();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void WhiteBoxEntityGizmo::SetTarget(const AZ::EntityId entityId)
    {
        if (entityId == m_entityId)
        {
            return;
        }
        m_entityId = entityId;

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        if (m_entityId.IsValid())
        {
            AZ::TransformNotificationBus::Handler::BusConnect(m_entityId);
        }

        if (m_mode != Mode::None)
        {
            Rebuild();
        }
    }

    void WhiteBoxEntityGizmo::SetMode(const Mode mode)
    {
        if (mode == m_mode)
        {
            return;
        }
        m_mode = mode;
        Rebuild();
    }

    void WhiteBoxEntityGizmo::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        if (!m_applying)
        {
            Refresh(); // entity moved by something else (spin boxes, undo, another tool)
        }
    }

    void WhiteBoxEntityGizmo::Refresh()
    {
        if (!m_entityId.IsValid())
        {
            Destroy();
            return;
        }
        AZ::Vector3 worldTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        AZ::Quaternion worldRotation = AZ::Quaternion::CreateIdentity();
        AZ::TransformBus::EventResult(
            worldRotation, m_entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);

        // The manipulators live in identity space; the entity's world transform is expressed
        // through their local position/orientation.
        if (m_translation)
        {
            m_translation->SetLocalPosition(worldTranslation);
        }
        if (m_rotation)
        {
            m_rotation->SetLocalPosition(worldTranslation);
            m_rotation->SetLocalOrientation(worldRotation);
        }
        if (m_scale)
        {
            m_scale->SetLocalPosition(worldTranslation);
        }
    }

    void WhiteBoxEntityGizmo::BeginBatch(const char* label)
    {
        if (!m_batchActive)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, label);
            m_batchActive = true;
        }
    }

    void WhiteBoxEntityGizmo::EndBatch()
    {
        if (m_batchActive)
        {
            AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(m_entityId);
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
            m_batchActive = false;
            if (m_changedCallback)
            {
                m_changedCallback();
            }
        }
    }

    void WhiteBoxEntityGizmo::Rebuild()
    {
        Destroy();
        if (m_mode == Mode::None || !m_entityId.IsValid())
        {
            return;
        }

        namespace Aztf = AzToolsFramework;
        AZ::Vector3 worldTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            worldTranslation, m_entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        AZ::Quaternion worldRotation = AZ::Quaternion::CreateIdentity();
        AZ::TransformBus::EventResult(
            worldRotation, m_entityId, &AZ::TransformBus::Events::GetWorldRotationQuaternion);
        const AZ::Transform identity = AZ::Transform::CreateIdentity();

        switch (m_mode)
        {
        case Mode::Translate:
            {
                m_translation = AZStd::make_unique<Aztf::TranslationManipulators>(
                    Aztf::TranslationManipulators::Dimensions::Three, identity, AZ::Vector3::CreateOne());
                m_translation->SetLineBoundWidth(Aztf::ManipulatorLineBoundWidth());
                Aztf::ConfigureTranslationManipulatorAppearance3d(m_translation.get());
                m_translation->SetLocalPosition(worldTranslation);

                const auto applyPosition = [this](const AZ::Vector3& worldPosition)
                {
                    m_applying = true;
                    AZ::TransformBus::Event(
                        m_entityId, &AZ::TransformBus::Events::SetWorldTranslation, worldPosition);
                    m_applying = false;
                    if (m_translation)
                    {
                        m_translation->SetLocalPosition(worldPosition);
                    }
                };
                m_translation->InstallLinearManipulatorMouseMoveCallback(
                    [this, applyPosition](const Aztf::LinearManipulator::Action& action)
                    {
                        BeginBatch("Move White Box Entity");
                        applyPosition(action.LocalPosition());
                    });
                m_translation->InstallLinearManipulatorMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::LinearManipulator::Action& action) { EndBatch(); });
                m_translation->InstallPlanarManipulatorMouseMoveCallback(
                    [this, applyPosition](const Aztf::PlanarManipulator::Action& action)
                    {
                        BeginBatch("Move White Box Entity");
                        applyPosition(action.LocalPosition());
                    });
                m_translation->InstallPlanarManipulatorMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::PlanarManipulator::Action& action) { EndBatch(); });

                m_translation->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        case Mode::Rotate:
            {
                m_rotation = AZStd::make_unique<Aztf::RotationManipulators>(identity);
                m_rotation->SetCircleBoundWidth(Aztf::ManipulatorCicleBoundWidth());
                m_rotation->SetLocalPosition(worldTranslation);
                m_rotation->SetLocalOrientation(worldRotation);
                m_rotation->SetLocalAxes(
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
                m_rotation->ConfigureView(
                    Aztf::RotationManipulatorRadius(), AzFramework::ViewportColors::XAxisColor,
                    AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);

                m_rotation->InstallMouseMoveCallback(
                    [this](const Aztf::AngularManipulator::Action& action)
                    {
                        BeginBatch("Rotate White Box Entity");
                        m_applying = true;
                        AZ::TransformBus::Event(
                            m_entityId, &AZ::TransformBus::Events::SetWorldRotationQuaternion,
                            action.LocalOrientation());
                        m_applying = false;
                        if (m_rotation)
                        {
                            m_rotation->SetLocalOrientation(action.LocalOrientation());
                        }
                    });
                m_rotation->InstallLeftMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::AngularManipulator::Action& action) { EndBatch(); });

                m_rotation->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        case Mode::Scale:
            {
                m_scale = AZStd::make_unique<Aztf::ScaleManipulators>(identity);
                m_scale->SetLineBoundWidth(Aztf::ManipulatorLineBoundWidth());
                m_scale->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
                m_scale->ConfigureView(
                    Aztf::LinearManipulatorAxisLength(), AzFramework::ViewportColors::XAxisColor,
                    AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);
                m_scale->SetLocalPosition(worldTranslation);

                // The entity scale is UNIFORM: any handle applies the same multiplicative
                // factor, derived from the largest deviation of the drag's scale offset.
                const auto scaleMove = [this](const Aztf::LinearManipulator::Action& action)
                {
                    if (!m_batchActive)
                    {
                        AZ::TransformBus::EventResult(
                            m_startScale, m_entityId, &AZ::TransformBus::Events::GetLocalUniformScale);
                        BeginBatch("Scale White Box Entity");
                    }
                    const AZ::Vector3 offset = action.LocalScaleOffset();
                    float delta = offset.GetX();
                    if (AZStd::abs(offset.GetY()) > AZStd::abs(delta))
                    {
                        delta = offset.GetY();
                    }
                    if (AZStd::abs(offset.GetZ()) > AZStd::abs(delta))
                    {
                        delta = offset.GetZ();
                    }
                    const float factor = 1.0f + (action.m_start.m_sign * delta);
                    const float scale = AZStd::max(m_startScale * factor, 0.001f);
                    m_applying = true;
                    AZ::TransformBus::Event(m_entityId, &AZ::TransformBus::Events::SetLocalUniformScale, scale);
                    m_applying = false;
                };
                const auto scaleEnd = [this]([[maybe_unused]] const Aztf::LinearManipulator::Action& action)
                { EndBatch(); };

                m_scale->InstallAxisMouseMoveCallback(scaleMove);
                m_scale->InstallAxisLeftMouseUpCallback(scaleEnd);
                m_scale->InstallUniformMouseMoveCallback(scaleMove);
                m_scale->InstallUniformLeftMouseUpCallback(scaleEnd);

                m_scale->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        default:
            break;
        }
    }

    void WhiteBoxEntityGizmo::Destroy()
    {
        EndBatch();
        if (m_translation)
        {
            m_translation->Unregister();
            m_translation.reset();
        }
        if (m_rotation)
        {
            m_rotation->Unregister();
            m_rotation.reset();
        }
        if (m_scale)
        {
            m_scale->Unregister();
            m_scale.reset();
        }
    }
} // namespace WhiteBox
