/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Tools/WhiteBoxLayerGizmo.h"

#include "EditorWhiteBoxComponent.h"
#include "Util/WhiteBoxSnapUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Color.h>
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
    namespace
    {
        constexpr const char* GizmoUndoLabels[] = { "", "Move White Box Layer", "Rotate White Box Layer",
                                                    "Scale White Box Layer" };

        AZ::Vector3 ClampScale(const AZ::Vector3& scale)
        {
            return AZ::Vector3(
                AZStd::max(scale.GetX(), 0.001f), AZStd::max(scale.GetY(), 0.001f),
                AZStd::max(scale.GetZ(), 0.001f));
        }
    } // namespace

    // Define construction where the owned manipulator types are complete;
    // constructor cleanup can instantiate their unique_ptr destructors too.
    WhiteBoxLayerGizmo::WhiteBoxLayerGizmo() = default;

    WhiteBoxLayerGizmo::~WhiteBoxLayerGizmo()
    {
        Destroy();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    EditorWhiteBoxComponent* WhiteBoxLayerGizmo::Component() const
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
        return entity != nullptr ? entity->FindComponent<EditorWhiteBoxComponent>() : nullptr;
    }

    AZ::Transform WhiteBoxLayerGizmo::Space() const
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, m_entityId, &AZ::TransformBus::Events::GetWorldTM);
        return worldFromLocal;
    }

    void WhiteBoxLayerGizmo::SetTarget(const AZ::EntityId entityId, const int layerIndex)
    {
        if (entityId == m_entityId && layerIndex == m_layerIndex)
        {
            return;
        }
        m_entityId = entityId;
        m_layerIndex = layerIndex;

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

    void WhiteBoxLayerGizmo::SetMode(const Mode mode)
    {
        if (mode == m_mode)
        {
            return;
        }
        m_mode = mode;
        Rebuild();
    }

    void WhiteBoxLayerGizmo::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        if (m_translation)
        {
            m_translation->SetSpace(world);
        }
        if (m_rotation)
        {
            m_rotation->SetSpace(world);
        }
        if (m_scale)
        {
            m_scale->SetSpace(world);
        }
    }

    void WhiteBoxLayerGizmo::Refresh()
    {
        EditorWhiteBoxComponent* component = Component();
        if (component == nullptr || m_layerIndex < 0 || m_layerIndex >= component->GetLayerCount())
        {
            Destroy();
            return;
        }
        const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
        const AZ::Transform space = Space();
        if (m_translation)
        {
            m_translation->SetSpace(space);
            m_translation->SetLocalPosition(meta.m_position);
        }
        if (m_rotation)
        {
            m_rotation->SetSpace(space);
            m_rotation->SetLocalPosition(meta.m_position);
            m_rotation->SetLocalOrientation(AZ::Quaternion::CreateFromEulerAnglesDegrees(meta.m_rotation));
        }
        if (m_scale)
        {
            m_scale->SetSpace(space);
            m_scale->SetLocalPosition(meta.m_position);
        }
    }

    void WhiteBoxLayerGizmo::BeginBatch(const char* label)
    {
        if (!m_batchActive)
        {
            m_batchLabel = label;
            m_batchActive = true;
        }
    }

    void WhiteBoxLayerGizmo::EndBatch()
    {
        if (m_batchActive)
        {
            m_batchActive = false;
            // Open the undo batch here, not across the drag: something between mouse events could close it
            // and drop the dirty mark, leaving the prefab unsaved.
            {
                AzToolsFramework::ScopedUndoBatch undoBatch(m_batchLabel.c_str());
                undoBatch.MarkEntityDirty(m_entityId);
            }
            if (m_changedCallback)
            {
                m_changedCallback();
            }
        }
    }

    void WhiteBoxLayerGizmo::ApplyPositionSnapped(const AZ::Vector3& localPosition)
    {
        EditorWhiteBoxComponent* component = Component();
        if (component == nullptr || !SnapUtil::SnappingActive())
        {
            ApplyPosition(localPosition);
            return;
        }

        const int viewportId = SnapUtil::ActiveViewportId();

        // Resolve the snap anchor once, as an offset from the layer origin (valid for the whole translate).
        if (!m_snapAnchorResolved)
        {
            m_snapAnchorResolved = true;
            m_snapAnchorOffset.reset();

            const AZStd::vector<AZ::Vector3> layerVertices =
                component->GetLayerVertexPositionsEntityLocal(m_layerIndex);

            if (const auto anchorIndex = SnapUtil::FindAnchorIndex(m_entityId, viewportId, layerVertices))
            {
                const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
                m_snapAnchorOffset = layerVertices[anchorIndex.value()] - meta.m_position;
            }
        }

        if (!m_snapAnchorOffset.has_value())
        {
            ApplyPosition(localPosition);
            SnapUtil::SetActiveSnapTarget(AZStd::nullopt);
            return;
        }

        // Nothing is excluded: this layer's own vertices are legitimate targets for another part
        // of the same mesh, and the snapper works on the evaluated (all layers) geometry anyway.
        const auto snapTarget = SnapUtil::FindSnapTargetWorld(m_entityId, viewportId, {});
        SnapUtil::SetActiveSnapTarget(snapTarget);

        if (!snapTarget.has_value())
        {
            ApplyPosition(localPosition);
            return;
        }

        // Place the layer so the anchor vertex lands exactly on the target.
        const AZ::Vector3 targetEntityLocal = SnapUtil::MeshLocalFromWorld(m_entityId, snapTarget.value());
        ApplyPosition(targetEntityLocal - m_snapAnchorOffset.value());
    }

    void WhiteBoxLayerGizmo::ApplyPosition(const AZ::Vector3& localPosition)
    {
        if (EditorWhiteBoxComponent* component = Component())
        {
            EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
            meta.m_position = localPosition;
            component->SetLayerMeta(m_layerIndex, meta);
            if (m_translation)
            {
                m_translation->SetLocalPosition(localPosition);
            }
        }
    }

    void WhiteBoxLayerGizmo::ApplyRotation(const AZ::Quaternion& localOrientation)
    {
        if (EditorWhiteBoxComponent* component = Component())
        {
            EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
            meta.m_rotation = localOrientation.GetEulerDegrees();
            component->SetLayerMeta(m_layerIndex, meta);
            if (m_rotation)
            {
                m_rotation->SetLocalOrientation(localOrientation);
            }
        }
    }

    void WhiteBoxLayerGizmo::ApplyScale(const AZ::Vector3& scale)
    {
        if (EditorWhiteBoxComponent* component = Component())
        {
            EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
            meta.m_scale = ClampScale(scale);
            component->SetLayerMeta(m_layerIndex, meta);
        }
    }

    void WhiteBoxLayerGizmo::Rebuild()
    {
        Destroy();

        EditorWhiteBoxComponent* component = Component();
        if (m_mode == Mode::None || component == nullptr || m_layerIndex < 0 ||
            m_layerIndex >= component->GetLayerCount())
        {
            return;
        }

        namespace Aztf = AzToolsFramework;
        const EditorWhiteBoxComponent::LayerMeta meta = component->GetLayerMeta(m_layerIndex);
        const AZ::Transform space = Space();
        const char* undoLabel = GizmoUndoLabels[static_cast<int>(m_mode)];

        switch (m_mode)
        {
        case Mode::Translate:
            {
                m_translation = AZStd::make_unique<Aztf::TranslationManipulators>(
                    Aztf::TranslationManipulators::Dimensions::Three, space, AZ::Vector3::CreateOne());
                m_translation->SetLineBoundWidth(Aztf::ManipulatorLineBoundWidth());
                Aztf::ConfigureTranslationManipulatorAppearance3d(m_translation.get());
                m_translation->SetLocalPosition(meta.m_position);

                // The undo batch begins lazily on the first move and ends on mouse up.
                m_translation->InstallLinearManipulatorMouseMoveCallback(
                    [this, undoLabel](const Aztf::LinearManipulator::Action& action)
                    {
                        BeginBatch(undoLabel);
                        ApplyPositionSnapped(action.LocalPosition());
                    });
                m_translation->InstallLinearManipulatorMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::LinearManipulator::Action& action)
                    {
                        m_snapAnchorResolved = false;
                        m_snapAnchorOffset.reset();
                        SnapUtil::ClearActiveSnapTarget();
                        EndBatch();
                    });

                m_translation->InstallPlanarManipulatorMouseMoveCallback(
                    [this, undoLabel](const Aztf::PlanarManipulator::Action& action)
                    {
                        BeginBatch(undoLabel);
                        ApplyPositionSnapped(action.LocalPosition());
                    });
                m_translation->InstallPlanarManipulatorMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::PlanarManipulator::Action& action)
                    {
                        m_snapAnchorResolved = false;
                        m_snapAnchorOffset.reset();
                        SnapUtil::ClearActiveSnapTarget();
                        EndBatch();
                    });

                m_translation->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        case Mode::Rotate:
            {
                m_rotation = AZStd::make_unique<Aztf::RotationManipulators>(space);
                m_rotation->SetCircleBoundWidth(Aztf::ManipulatorCicleBoundWidth());
                m_rotation->SetLocalPosition(meta.m_position);
                m_rotation->SetLocalOrientation(AZ::Quaternion::CreateFromEulerAnglesDegrees(meta.m_rotation));
                m_rotation->SetLocalAxes(
                    AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
                m_rotation->ConfigureView(
                    Aztf::RotationManipulatorRadius(), AzFramework::ViewportColors::XAxisColor,
                    AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);

                m_rotation->InstallMouseMoveCallback(
                    [this, undoLabel](const Aztf::AngularManipulator::Action& action)
                    {
                        BeginBatch(undoLabel);
                        ApplyRotation(action.LocalOrientation());
                    });
                m_rotation->InstallLeftMouseUpCallback(
                    [this]([[maybe_unused]] const Aztf::AngularManipulator::Action& action) { EndBatch(); });

                m_rotation->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        case Mode::Scale:
            {
                m_scale = AZStd::make_unique<Aztf::ScaleManipulators>(space);
                m_scale->SetLineBoundWidth(Aztf::ManipulatorLineBoundWidth());
                m_scale->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
                m_scale->ConfigureView(
                    Aztf::LinearManipulatorAxisLength(), AzFramework::ViewportColors::XAxisColor,
                    AzFramework::ViewportColors::YAxisColor, AzFramework::ViewportColors::ZAxisColor);
                m_scale->SetLocalPosition(meta.m_position);

                // Multiplicative scaling as in TransformMode: startScale * (1 + sign * offset); the centre
                // handle applies Z to all axes.
                const auto scaleMove = [this, undoLabel](const Aztf::LinearManipulator::Action& action, bool uniform)
                {
                    if (!m_batchActive)
                    {
                        if (EditorWhiteBoxComponent* component = Component())
                        {
                            m_startScale = component->GetLayerMeta(m_layerIndex).m_scale;
                        }
                        BeginBatch(undoLabel);
                    }
                    const AZ::Vector3 offset = uniform
                        ? AZ::Vector3(action.LocalScaleOffset().GetZ())
                        : action.LocalScaleOffset();
                    const AZ::Vector3 factor = AZ::Vector3::CreateOne() + (action.m_start.m_sign * offset);
                    ApplyScale(m_startScale * factor);
                };
                const auto scaleEnd = [this]([[maybe_unused]] const Aztf::LinearManipulator::Action& action)
                { EndBatch(); };

                m_scale->InstallAxisMouseMoveCallback(
                    [scaleMove](const Aztf::LinearManipulator::Action& action) { scaleMove(action, false); });
                m_scale->InstallAxisLeftMouseUpCallback(scaleEnd);

                m_scale->InstallUniformMouseMoveCallback(
                    [scaleMove](const Aztf::LinearManipulator::Action& action) { scaleMove(action, true); });
                m_scale->InstallUniformLeftMouseUpCallback(scaleEnd);

                m_scale->Register(Aztf::GetMainManipulatorManagerId());
            }
            break;
        default:
            break;
        }
    }

    void WhiteBoxLayerGizmo::Destroy()
    {
        EndBatch(); // safety: never leave an undo batch open
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
