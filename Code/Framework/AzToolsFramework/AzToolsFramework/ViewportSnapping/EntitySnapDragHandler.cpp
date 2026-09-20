/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSnapping/EntitySnapDragHandler.h>
#include <AzToolsFramework/ViewportSnapping/ViewportDragCancelBus.h>
#include <AzToolsFramework/ViewportSnapping/ViewportSnapSourceBus.h>
#include <AzToolsFramework/ViewportSnapping/ViewportSnapping.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/limits.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

AZ_CVAR(
    AZ::Color, ed_snapTargetColor, AZ::Color::CreateFromRgba(0, 255, 160, 255), nullptr, AZ::ConsoleFunctorFlags::Null,
    "The colour of the highlight drawn on the position an entity drag is snapping to.");
AZ_CVAR(
    float, ed_snapTargetSize, 0.06f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "The radius of the highlight drawn on the position an entity drag is snapping to.");

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        namespace
        {
            //! Is the transform gizmo, or any manipulator, currently mid-interaction.
            bool ManipulatorInteracting()
            {
                bool interacting = false;
                ManipulatorManagerRequestBus::EventResult(
                    interacting, GetMainManipulatorManagerId(), &ManipulatorManagerRequests::Interacting);
                return interacting;
            }

            bool TranslateModeActive()
            {
                // Seeded with Translation: with no handler EventResult leaves this untouched, and
                // callers only reach here while a manipulator is interacting, which cannot happen
                // without a transform selection to own it.
                auto mode = EditorTransformComponentSelectionRequests::Mode::Translation;
                EditorTransformComponentSelectionRequestBus::EventResult(
                    mode, GetEntityContextId(), &EditorTransformComponentSelectionRequests::GetTransformMode);

                return mode == EditorTransformComponentSelectionRequests::Mode::Translation;
            }

            AZStd::optional<AZ::Vector3> ManipulatorPivot()
            {
                AZStd::optional<AZ::Transform> manipulatorTransform;
                EditorTransformComponentSelectionRequestBus::EventResult(
                    manipulatorTransform, GetEntityContextId(),
                    &EditorTransformComponentSelectionRequests::GetManipulatorTransform);

                if (manipulatorTransform.has_value())
                {
                    return manipulatorTransform->GetTranslation();
                }

                return AZStd::nullopt;
            }

            AZStd::vector<AZ::EntityId> SelectedEntities()
            {
                EntityIdList selected;
                ToolsApplicationRequestBus::BroadcastResult(
                    selected, &ToolsApplicationRequests::GetSelectedEntities);
                return selected;
            }

            //! Drop any entity that is a descendant of another entity in the same list.
            //!
            //! Moving a parent already moves its children, so applying the offset to both would
            //! double it - a child would end up twice as far from the snap target as it should be.
            AZStd::vector<AZ::EntityId> WithoutDescendantsOfSelection(const AZStd::vector<AZ::EntityId>& entities)
            {
                AZStd::vector<AZ::EntityId> topLevel;
                topLevel.reserve(entities.size());

                for (const AZ::EntityId entityId : entities)
                {
                    bool hasSelectedAncestor = false;

                    AZ::EntityId parentId;
                    AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

                    while (parentId.IsValid())
                    {
                        if (AZStd::find(entities.begin(), entities.end(), parentId) != entities.end())
                        {
                            hasSelectedAncestor = true;
                            break;
                        }

                        AZ::EntityId nextParentId;
                        AZ::TransformBus::EventResult(
                            nextParentId, parentId, &AZ::TransformBus::Events::GetParentId);
                        parentId = nextParentId;
                    }

                    if (!hasSelectedAncestor)
                    {
                        topLevel.push_back(entityId);
                    }
                }

                return topLevel;
            }

            //! A volume that accepts everything in front of the camera.
            //!
            //! Used to gather all of the dragged entities' candidates, so the one nearest the
            //! cursor can be picked as the anchor. This is the one query that deliberately does not
            //! want the cursor cone.
            SnapQueryVolume CreateAcceptAllVolume(const AzFramework::CameraState& cameraState)
            {
                // Large but finite - FLT_MAX would overflow to infinity in the radius multiply
                // inside Contains, which happens to still compare true but is not something worth
                // relying on.
                constexpr float VeryLarge = 1.0e9f;

                SnapQueryVolume volume;
                volume.m_rayOrigin = cameraState.m_position;
                volume.m_rayDirection = cameraState.m_forward;
                volume.m_maxDistance = VeryLarge;
                volume.m_radiusPerDistance = VeryLarge;
                volume.m_radiusConstant = VeryLarge;
                volume.m_bounds = AZ::Aabb::CreateNull();
                return volume;
            }

            //! The point of @p entities whose screen projection is nearest the cursor.
            //!
            //! This is what will be placed on the snap target, so it is resolved once at drag start
            //! and held: re-picking it every frame would let the anchor flip to a different vertex
            //! as the cursor moved, and the selection would appear to jump.
            AZStd::optional<AZ::Vector3> FindAnchorWorld(
                const ViewportSnappingInterface& snapping,
                const AZStd::vector<AZ::EntityId>& entities,
                const AzFramework::CameraState& cameraState,
                const AzFramework::ScreenPoint& screenPoint)
            {
                const SnapQueryVolume acceptAll = CreateAcceptAllVolume(cameraState);

                // Asked of the service rather than of ViewportSnapSourceRequestBus directly.
                // Going straight to the bus only finds entities that publish candidates
                // themselves, which quietly excludes ordinary meshes - they are served by
                // reconstruction inside the implementation. The visible symptom of getting this
                // wrong is subtle rather than obvious: nothing errors, the anchor silently falls
                // back to the pivot below, and a mesh appears to snap by its origin while a
                // publishing entity snaps by its corners.
                AZStd::vector<SnapVertex> vertices;
                snapping.CollectSnapCandidates(entities, acceptAll, vertices);

                if (vertices.empty())
                {
                    return AZStd::nullopt;
                }

                const float cursorX = aznumeric_cast<float>(screenPoint.m_x);
                const float cursorY = aznumeric_cast<float>(screenPoint.m_y);

                AZStd::optional<AZ::Vector3> nearest;
                float nearestDistanceSq = AZStd::numeric_limits<float>::max();

                for (const SnapVertex& vertex : vertices)
                {
                    const AzFramework::ScreenPoint projected =
                        AzFramework::WorldToScreen(vertex.m_worldPosition, cameraState);
                    const float deltaX = aznumeric_cast<float>(projected.m_x) - cursorX;
                    const float deltaY = aznumeric_cast<float>(projected.m_y) - cursorY;
                    const float distanceSq = deltaX * deltaX + deltaY * deltaY;

                    if (distanceSq < nearestDistanceSq)
                    {
                        nearestDistanceSq = distanceSq;
                        nearest = vertex.m_worldPosition;
                    }
                }

                return nearest;
            }
        } // namespace

        bool EntitySnapDragHandler::TryCancelDrag(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            if (mouseInteraction.m_mouseEvent != ViewportInteraction::MouseEvent::Down ||
                !mouseInteraction.m_mouseInteraction.m_mouseButtons.Right())
            {
                return false;
            }

            bool cancelled = false;
            ViewportDragCancelRequestBus::BroadcastResult(cancelled, &ViewportDragCancelRequests::CancelActiveDrag);

            if (cancelled)
            {
                // An entity drag is a drag too - abandon it on the same click.
                ResetEntityDrag();
            }

            return cancelled;
        }

        bool EntitySnapDragHandler::HandleMouseViewportInteraction(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            return TryCancelDrag(mouseInteraction);
        }

        bool EntitySnapDragHandler::HandleMouseManipulatorInteractionBefore(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            // Checked before the manipulator manager sees it: while a manipulator is interacting
            // the manager consumes the press, so this is the last point at which a right click is
            // still visible.
            return TryCancelDrag(mouseInteraction);
        }

        void EntitySnapDragHandler::ResetEntityDrag()
        {
            m_entityDrag = EntityDragState{};
            m_snapTargetWorld.reset();
        }

        void EntitySnapDragHandler::HandleMouseManipulatorInteractionAfter(
            const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
        {
            const auto* snapper = ViewportSnappingRequests::Get();
            if (snapper == nullptr || !snapper->IsSnappingEnabled())
            {
                ResetEntityDrag();
                return;
            }

            // Component modes register their manipulators with the same manager, so dragging one
            // vertex of a mesh in a component mode also reports "interacting". Without this guard
            // the whole entity would move while the user is editing one of its vertices.
            // Sub-element snapping is the component mode's own job.
            if (ComponentModeFramework::InComponentMode())
            {
                ResetEntityDrag();
                return;
            }

            if (!ManipulatorInteracting())
            {
                // The drag just ended. The manipulator manager clears its active manipulator inside
                // the call this runs after, so this - not the mouse up event - is where a finished
                // drag is observed. Resync the gizmo, which has been trailing the geometry by the
                // snap delta, now that it is safe to move it.
                if (m_entityDrag.m_active)
                {
                    EditorTransformComponentSelectionRequestBus::Event(
                        GetEntityContextId(), &EditorTransformComponentSelectionRequests::RefreshManipulators,
                        EditorTransformComponentSelectionRequests::RefreshType::Translation);
                }

                ResetEntityDrag();
                return;
            }

            // Only translation snaps. Rotating or scaling onto a vertex is not meaningful.
            if (!TranslateModeActive())
            {
                ResetEntityDrag();
                return;
            }

            // Ctrl belongs to the editor's own transform behaviour during a gizmo drag, so stand
            // down while it is held rather than have two snapping systems fight over one drag.
            //
            // Resetting rather than merely skipping is deliberate: the drag state caches the offset
            // from the gizmo pivot to the anchor, and anything Ctrl does to the pivot would leave
            // that stale. Clearing it means releasing Ctrl re-resolves both from wherever things
            // now are, so snapping resumes correctly rather than from a remembered position.
            if (mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
            {
                ResetEntityDrag();
                return;
            }

            const auto pivot = ManipulatorPivot();
            if (!pivot.has_value())
            {
                ResetEntityDrag();
                return;
            }

            const AzFramework::ViewportId viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
            const AzFramework::ScreenPoint screenPoint =
                mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates;
            const AzFramework::CameraState cameraState = GetCameraState(viewportId);

            if (!m_entityDrag.m_active)
            {
                const AZStd::vector<AZ::EntityId> selected = SelectedEntities();
                if (selected.empty())
                {
                    return;
                }

                m_entityDrag.m_entities = WithoutDescendantsOfSelection(selected);
                m_entityDrag.m_excludeEntities = selected;

                // Prefer a real vertex as the anchor, and fall back to the gizmo pivot for entities
                // with no usable geometry at all.
                const auto anchorWorld = FindAnchorWorld(*snapper, m_entityDrag.m_entities, cameraState, screenPoint);
                m_entityDrag.m_anchorOffsetFromPivot = anchorWorld.value_or(pivot.value()) - pivot.value();
                m_entityDrag.m_active = true;
            }

            // The gizmo rewrites entity transforms from its own mouse-down baseline every frame, so
            // whatever correction was applied last frame has already been undone: the pivot and
            // anchor read here are unsnapped, and the delta below is absolute rather than
            // incremental. That is also what makes the snap let go cleanly when the cursor leaves
            // range, instead of the selection drifting by an accumulated error.
            const AZ::Vector3 anchorWorld = pivot.value() + m_entityDrag.m_anchorOffsetFromPivot;

            SnapQueryConfig config;
            config.m_excludeEntities = m_entityDrag.m_excludeEntities;

            const auto snapResult = snapper->QuerySnapTarget(viewportId, screenPoint, config);
            m_snapTargetWorld =
                snapResult.has_value() ? AZStd::optional<AZ::Vector3>(snapResult->m_worldPosition) : AZStd::nullopt;

            if (!m_snapTargetWorld.has_value())
            {
                return;
            }

            const AZ::Vector3 delta = m_snapTargetWorld.value() - anchorWorld;
            if (delta.IsZero())
            {
                return;
            }

            for (const AZ::EntityId entityId : m_entityDrag.m_entities)
            {
                AZ::Vector3 worldTranslation = AZ::Vector3::CreateZero();
                AZ::TransformBus::EventResult(
                    worldTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);

                AZ::TransformBus::Event(
                    entityId, &AZ::TransformBus::Events::SetWorldTranslation, worldTranslation + delta);

                // The gizmo already opened an undo batch on mouse down; make sure the transforms
                // changed behind its back are part of it.
                ScopedUndoBatch::MarkEntityDirty(entityId);
            }

            // @note The gizmo is deliberately left where the mouse put it, so it trails the
            // geometry by the snap delta for the duration of the drag. Moving it mid-drag would
            // mean calling OverrideManipulatorTranslation - which sets a persistent pivot override
            // and regenerates the manipulators - on every mouse move, while that same manipulator
            // is mid-interaction. It is resynced when the drag ends, in the ManipulatorInteracting
            // branch above.
        }

        void EntitySnapDragHandler::DisplayViewportSelection(AzFramework::DebugDisplayRequests& debugDisplay) const
        {
            if (!m_snapTargetWorld.has_value())
            {
                return;
            }

            // Depth test off: the target is very often a corner on the far side of a solid, and a
            // marker the user cannot see is no use for telling them where the selection is about to
            // land.
            debugDisplay.DepthTestOff();
            debugDisplay.SetColor(static_cast<AZ::Color>(ed_snapTargetColor));
            debugDisplay.DrawBall(m_snapTargetWorld.value(), static_cast<float>(ed_snapTargetSize));
            debugDisplay.DepthTestOn();
        }
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
