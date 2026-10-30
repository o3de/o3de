/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * EditorWhiteBoxComponent - ENTITY BOOLEAN translation unit: one-shot Apply Boolean, the
 * live (non-destructive) boolean evaluation, the debounced game-mode bake caches and the
 * boolean source listener.
 */

#include "EditorWhiteBoxComponent.h"

#include "Util/WhiteBoxMeshUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace WhiteBox
{
    namespace
    {
        //! Bake an operand into the target's unscaled local space, applying both non-uniform scales to the
        //! vertices: v' = S_target^-1 * (T_target^-1 * T_operand) * (S_operand * v).
        Api::WhiteBoxMeshPtr BakeOperandIntoTargetSpace(
            const WhiteBoxMesh& operandMesh, const AZ::Vector3& operandNonUniformScale,
            const AZ::Transform& operandWorldTM, const AZ::Transform& targetWorldTM,
            const AZ::Vector3& targetNonUniformScale)
        {
            Api::WhiteBoxMeshPtr baked = Api::CloneMesh(operandMesh);
            if (!baked)
            {
                return nullptr;
            }

            const AZ::Transform relative = targetWorldTM.GetInverse() * operandWorldTM; // rigid + uniform
            // Reciprocal of the target's non-uniform scale, guarded against zero components.
            const AZ::Vector3 invTargetScale(
                AZ::IsClose(targetNonUniformScale.GetX(), 0.0f) ? 1.0f : 1.0f / targetNonUniformScale.GetX(),
                AZ::IsClose(targetNonUniformScale.GetY(), 0.0f) ? 1.0f : 1.0f / targetNonUniformScale.GetY(),
                AZ::IsClose(targetNonUniformScale.GetZ(), 0.0f) ? 1.0f : 1.0f / targetNonUniformScale.GetZ());

            for (const Api::VertexHandle vertexHandle : Api::MeshVertexHandles(*baked))
            {
                AZ::Vector3 p = Api::VertexPosition(*baked, vertexHandle);
                p *= operandNonUniformScale;    // S_operand (component-wise)
                p = relative.TransformPoint(p); // T_target^-1 * T_operand (rotation + uniform scale + translation)
                p *= invTargetScale;            // S_target^-1 (component-wise)
                Api::SetVertexPosition(*baked, vertexHandle, p);
            }
            Api::CalculateNormals(*baked);
            Api::CalculatePlanarUVs(*baked);
            return baked;
        }

        //! The entity's non-uniform scale (identity when it has no Non-Uniform Scale component).
        AZ::Vector3 EntityNonUniformScaleFor(const AZ::EntityId entityId)
        {
            AZ::Vector3 scale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(scale, entityId, &AZ::NonUniformScaleRequests::GetScale);
            return scale;
        }
    } // namespace

    void EditorWhiteBoxComponent::ApplyBoolean()
    {
        if (!m_boolean.m_sourceEntity.IsValid() || m_boolean.m_sourceEntity == GetEntityId())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source is not set (or is this same entity).");
            return;
        }

        WhiteBoxMesh* freeform = GetWhiteBoxMesh();
        if (freeform == nullptr)
        {
            return;
        }

        AZ::Entity* sourceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_boolean.m_sourceEntity);
        if (sourceEntity == nullptr)
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source entity could not be found.");
            return;
        }

        const auto sourceComponents = sourceEntity->FindComponents<EditorWhiteBoxComponent>();
        if (sourceComponents.empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source entity has no White Box component.");
            return;
        }
        // Use the source's EVALUATED mesh so its stamped cubes participate too, not just its freeform.
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetEvaluatedWhiteBoxMesh();
        if (sourceMesh == nullptr || Api::MeshFaceHandles(*sourceMesh).empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source mesh is empty.");
            return;
        }

        // Boolean the combined mesh (freeform plus stamped cubes), not just the freeform.
        Api::WhiteBoxMeshPtr combined = CombinedWithGrid(freeform);
        WhiteBoxMesh* targetMesh = combined ? combined.get() : freeform;
        if (Api::MeshFaceHandles(*targetMesh).empty())
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "This White Box mesh is empty (nothing to boolean).");
            return;
        }

        // Bake the source into local space; a transform alone would lose the non-uniform scales.
        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_boolean.m_sourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        Api::WhiteBoxMeshPtr sourceOperand = BakeOperandIntoTargetSpace(
            *sourceMesh, EntityNonUniformScaleFor(m_boolean.m_sourceEntity), sourceWorldTM, thisWorldTM,
            EntityNonUniformScaleFor(GetEntityId()));
        if (!sourceOperand)
        {
            AZ_Warning("EditorWhiteBoxComponent", false, "Boolean Source mesh could not be cloned.");
            return;
        }

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Boolean");

        if (!Api::ApplyMeshBoolean(
                *targetMesh, *sourceOperand, AZ::Transform::CreateIdentity(), m_boolean.m_operation, m_csgSolver))
        {
            AZ_Warning(
                "EditorWhiteBoxComponent", false,
                "White Box boolean produced no result (the meshes may not overlap, or are not closed).");
            return;
        }

        Api::CalculateNormals(*targetMesh);
        Api::CalculatePlanarUVs(*targetMesh);

        // If we booleaned the combined solid, bake the result back into the freeform mesh and clear the
        // now-consumed stamp/grid layers (their geometry is folded into the boolean result).
        if (combined)
        {
            Api::WhiteBoxMeshStream stream;
            Api::WriteMesh(*targetMesh, stream);
            Api::ReadMesh(*freeform, stream);

            m_gridMesh = Api::CreateWhiteBoxMesh();
            Api::WriteMesh(*m_gridMesh, m_voxel.m_legacyGridData);
            m_voxel.m_legacyGridMergedData.clear();
            m_voxel.m_cells.clear();
            m_voxel.m_sizes.clear();
            m_voxel.m_legacyMerged.clear();
        }

        SerializeWhiteBox();
        RebuildWhiteBox();
        undoBatch.MarkEntityDirty(GetEntityId());

        // Optionally tidy up the source entity once it has been consumed.
        if (m_boolean.m_sourceAfterApply == SourceAfterApply::Delete)
        {
            const AZ::EntityId sourceId = m_boolean.m_sourceEntity;
            m_boolean.m_sourceEntity = AZ::EntityId{}; // clear the now-dangling reference
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::DeleteEntityById, sourceId);
        }
        else if (m_boolean.m_sourceAfterApply == SourceAfterApply::Hide)
        {
            AzToolsFramework::SetEntityVisibility(m_boolean.m_sourceEntity, false);
        }
    }

    WhiteBoxMesh* EditorWhiteBoxComponent::GetLiveBooleanDisplayMesh()
    {
        return m_displayMesh.get();
    }

    Api::WhiteBoxMeshPtr EditorWhiteBoxComponent::EvaluateBooleanMesh(bool physicsPass)
    {
        const bool hasSingleSource =
            m_boolean.m_sourceEntity.IsValid() && m_boolean.m_sourceEntity != GetEntityId();

        // Global cutters that overlap this target (only while this entity is an active global-boolean
        // target - RefreshGlobalBooleans sets m_globalBooleanActive and this entity is not excluded).
        AZStd::vector<ResolvedCutter> cutters;
        if (m_globalBooleanActive && !m_boolean.m_excludeFromBoolean)
        {
            cutters = CollectOverlappingCutters(physicsPass);
        }

        // Nothing to evaluate: no single source and no overlapping cutter.
        if (!hasSingleSource && cutters.empty())
        {
            return nullptr;
        }

        WhiteBoxMesh* baseMesh = GetWhiteBoxMesh();
        if (baseMesh == nullptr)
        {
            return nullptr;
        }

        // This target's world transform and non-uniform scale (operands are baked into this
        // entity's UNSCALED local space; see BakeOperandIntoTargetSpace).
        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Vector3 thisNonUniformScale = EntityNonUniformScaleFor(GetEntityId());

        // Resolve the single-source operand (if any), baked into this target's local space. A
        // missing/invalid single source is not fatal when global cutters still apply.
        Api::WhiteBoxMeshPtr sourceOperand;
        if (hasSingleSource)
        {
            AZ::Entity* sourceEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_boolean.m_sourceEntity);
            const auto sourceComponents =
                sourceEntity ? sourceEntity->FindComponents<EditorWhiteBoxComponent>()
                             : AZStd::vector<EditorWhiteBoxComponent*>{};
            WhiteBoxMesh* sourceMesh = sourceComponents.empty() ? nullptr : sourceComponents[0]->GetEvaluatedWhiteBoxMesh();
            if (sourceMesh != nullptr)
            {
                AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
                AZ::TransformBus::EventResult(
                    sourceWorldTM, m_boolean.m_sourceEntity, &AZ::TransformBus::Events::GetWorldTM);
                sourceOperand = BakeOperandIntoTargetSpace(
                    *sourceMesh, EntityNonUniformScaleFor(m_boolean.m_sourceEntity), sourceWorldTM, thisWorldTM,
                    thisNonUniformScale);
            }
        }

        if (sourceOperand == nullptr && cutters.empty())
        {
            return nullptr; // single source failed to resolve and no cutters -> nothing to do
        }

        // Evaluate into a fresh mesh; the cut applies to all layers unless "Affect only the active layer".
        Api::WhiteBoxMeshPtr evaluated;
        if (m_boolean.m_affectActiveOnly)
        {
            evaluated = Api::CloneMesh(*baseMesh);
        }
        else
        {
            // Pass the physicsPass flag down to BuildCombined!
            evaluated = BuildCombined(baseMesh, physicsPass); // full base combined (no live boolean -> no recursion)
            if (!evaluated)
            {
                evaluated = Api::CloneMesh(*baseMesh);
            }
        }
        if (!evaluated)
        {
            return nullptr;
        }

        // The single source first, then each overlapping global cutter; cutters with no result are skipped.
        bool anyApplied = false;
        if (sourceOperand != nullptr &&
            Api::ApplyMeshBoolean(
                *evaluated, *sourceOperand, AZ::Transform::CreateIdentity(), m_boolean.m_operation, m_csgSolver))
        {
            anyApplied = true;
        }
        for (const ResolvedCutter& cutter : cutters)
        {
            if (cutter.m_mesh != nullptr &&
                Api::ApplyMeshBoolean(
                    *evaluated, *cutter.m_mesh, AZ::Transform::CreateIdentity(), cutter.m_operation, m_csgSolver))
            {
                anyApplied = true;
            }
        }

        if (anyApplied)
        {
            Api::CalculateNormals(*evaluated);
            Api::CalculatePlanarUVs(*evaluated);
            return evaluated;
        }
        // nothing applied (no overlap) -> null so callers fall back to the base mesh.
        return nullptr;
    }

    AZStd::vector<EditorWhiteBoxComponent::ResolvedCutter> EditorWhiteBoxComponent::CollectOverlappingCutters(
        bool physicsPass)
    {
        AZStd::vector<ResolvedCutter> cutters;

        // This entity's world AABB and transform (targets are cut in their own local space).
        const AZ::Aabb thisBounds = GetWorldBounds();
        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Vector3 thisNonUniformScale = EntityNonUniformScaleFor(GetEntityId());

        auto callback =
            [this, &cutters, &thisBounds, &thisWorldTM, &thisNonUniformScale, physicsPass](AZ::Entity* entity)
        {
            if (entity == nullptr || entity->GetId() == GetEntityId())
            {
                return; // never cut with self
            }
            auto* cutter = entity->FindComponent<EditorWhiteBoxComponent>();
            if (cutter == nullptr || !cutter->m_boolean.m_booleanOthers)
            {
                return; // only entities flagged as cutters participate
            }
            if (!thisBounds.Overlaps(cutter->GetWorldBounds()))
            {
                return; // AABB cull: a cutter that cannot touch this target does no work
            }
            // The strict physics mesh already contains the cutter's non-uniform scale; only the visual one needs it.
            WhiteBoxMesh* cutterMesh =
                physicsPass ? cutter->GetPhysicsCombinedMeshStrict() : cutter->GetEvaluatedWhiteBoxMesh();
            if (cutterMesh == nullptr || Api::MeshFaceHandles(*cutterMesh).empty())
            {
                return;
            }
            AZ::Transform cutterWorldTM = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                cutterWorldTM, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);

            ResolvedCutter resolved;
            const AZ::Vector3 cutterScale =
                physicsPass ? AZ::Vector3::CreateOne() : EntityNonUniformScaleFor(entity->GetId());
            resolved.m_mesh = BakeOperandIntoTargetSpace(
                *cutterMesh, cutterScale, cutterWorldTM, thisWorldTM, thisNonUniformScale);
            if (!resolved.m_mesh)
            {
                return;
            }
            resolved.m_operation = cutter->m_boolean.m_cutterOperation;
            cutters.push_back(AZStd::move(resolved));
        };
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, callback);

        return cutters;
    }

    bool EditorWhiteBoxComponent::HasOverlappingCutter() const
    {
        if (m_boolean.m_excludeFromBoolean)
        {
            return false;
        }
        const AZ::Aabb thisBounds = GetWorldBounds();
        bool found = false;
        auto callback = [this, &thisBounds, &found](AZ::Entity* entity)
        {
            if (found || entity == nullptr || entity->GetId() == GetEntityId())
            {
                return;
            }
            auto* cutter = entity->FindComponent<EditorWhiteBoxComponent>();
            if (cutter != nullptr && cutter->m_boolean.m_booleanOthers &&
                thisBounds.Overlaps(cutter->GetWorldBounds()))
            {
                found = true;
            }
        };
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, callback);
        return found;
    }

    void EditorWhiteBoxComponent::RefreshGlobalBooleans()
    {
        // Recompute every non-excluded target in the level. On demand only (N x M cost).
        AZStd::vector<EditorWhiteBoxComponent*> components;
        auto collect = [&components](AZ::Entity* entity)
        {
            if (entity != nullptr)
            {
                if (auto* component = entity->FindComponent<EditorWhiteBoxComponent>())
                {
                    components.push_back(component);
                }
            }
        };
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::EnumerateEntities, collect);

        for (EditorWhiteBoxComponent* component : components)
        {
            // Recompute the active flag from scratch and rebuild display, physics and bake.
            const bool wasActive = component->m_globalBooleanActive;
            component->m_globalBooleanActive = component->HasOverlappingCutter();
            component->RebuildWhiteBox();

            // Mark only targets whose baked data changed, so a refresh does not dirty the whole scene.
            if (component->m_globalBooleanActive || wasActive)
            {
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::AddDirtyEntity, component->GetEntityId());
            }
        }
    }

    AZ::Crc32 EditorWhiteBoxComponent::OnRefreshGlobalBooleans()
    {
        // One undo step for the whole refresh (it touches many entities). The batch must be open when
        // RefreshGlobalBooleans marks the changed entities dirty, or the engine drops the marks.
        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Refresh Global Booleans");
        RefreshGlobalBooleans();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorWhiteBoxComponent::EvaluateLiveBoolean()
    {
        // Evaluate whenever a source is set, so the game entity gets both variants; the live flag only
        // decides what is displayed.
        m_displayMesh = EvaluateBooleanMesh();
        m_physicsDisplayMesh = EvaluateBooleanMesh(true); // Physics pass
        
        // The bake caches cost a full recombine, so recompute them debounced on tick (and in BuildGameEntity).
        m_rebuild.m_bakedDataDirty = true;
        m_rebuild.m_bakedDataDelay = 0.0f;
    }

    void EditorWhiteBoxComponent::RebuildBakedRenderData()
    {
        // Cache the full uncut base render data for the game-mode bake (serialized for clones).
        if (WhiteBoxMesh* baseMesh = GetWhiteBoxMesh())
        {
            // --- VISUAL BAKE ---
            if (PerLayerRenderActive()) {
                m_bakedBaseRenderData = BuildColoredRenderData(baseMesh);
            } else {
                const Api::WhiteBoxMeshPtr combined = BuildCombined(baseMesh, false);
                m_bakedBaseRenderData = CreateWhiteBoxRenderData(combined ? *combined : *baseMesh, m_material);
            }

            // Physics geometry without Collision-off layers; empty means nothing collides.
            const Api::WhiteBoxMeshPtr physicsCombined = BuildCombined(baseMesh, true);
            // Bake the non-uniform scale into the physics geometry, as the edit-time mesh does.
            if (const AZ::Vector3 nus = EntityNonUniformScale();
                physicsCombined && !nus.IsClose(AZ::Vector3::CreateOne()))
            {
                ApplyTransformToMesh(*physicsCombined, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), nus);
                Api::CalculateNormals(*physicsCombined);
            }
            m_bakedPhysicsBaseRenderData =
                physicsCombined ? CreateWhiteBoxRenderData(*physicsCombined, m_material) : WhiteBoxRenderData{};
            // Mark that the physics geometry has been baked (even when empty) so the game-mode build can
            // trust an empty result as "collision off" instead of falling back to the full visual mesh.
            m_physicsBaked = true;
        }

        // Cache the boolean render variant (serialized for clones); in active-only mode fold the other layers around it.
        if (m_displayMesh)
        {
            if (PerLayerRenderActive())
            {
                m_bakedBooleanRenderData = BuildColoredRenderData(m_displayMesh.get());
            }
            else
            {
                const Api::WhiteBoxMeshPtr combined =
                    m_boolean.m_affectActiveOnly ? BuildCombined(m_displayMesh.get()) : nullptr;
                m_bakedBooleanRenderData =
                    CreateWhiteBoxRenderData(combined ? *combined : *m_displayMesh, m_material);
            }
            // --- PHYSICS BAKE (ADD THIS) ---
            if (m_physicsDisplayMesh)
            {
                Api::WhiteBoxMeshPtr physicsCombined =
                    m_boolean.m_affectActiveOnly ? BuildCombined(m_physicsDisplayMesh.get(), true) : nullptr;
                // Bake the entity non-uniform scale in (clone the shared display mesh first if needed).
                if (const AZ::Vector3 nus = EntityNonUniformScale(); !nus.IsClose(AZ::Vector3::CreateOne()))
                {
                    if (!physicsCombined)
                    {
                        physicsCombined = Api::CloneMesh(*m_physicsDisplayMesh);
                    }
                    if (physicsCombined)
                    {
                        ApplyTransformToMesh(
                            *physicsCombined, AZ::Vector3::CreateZero(), AZ::Vector3::CreateZero(), nus);
                        Api::CalculateNormals(*physicsCombined);
                    }
                }
                m_bakedPhysicsBooleanRenderData =
                    CreateWhiteBoxRenderData(physicsCombined ? *physicsCombined : *m_physicsDisplayMesh, m_material);
            }
        }
        else if (!m_boolean.m_sourceEntity.IsValid() || m_boolean.m_sourceEntity == GetEntityId())
        {
            // no boolean source -> there is no boolean variant; clear the cache
            m_bakedBooleanRenderData = WhiteBoxRenderData{};
            m_bakedPhysicsBooleanRenderData = WhiteBoxRenderData{};
        }
        // else: a source is set but evaluation transiently failed (e.g. the source entity is
        // not active yet at load time). Keep any previously cached boolean render data.

        // Refresh the serialized byte stream now that every bake above is current.
        PackBakedDataBlob();
    }

    void EditorWhiteBoxComponent::UpdateBooleanSourceListener()
    {
        m_booleanSourceListener.BusDisconnect();
        // listen whenever a source is set (not just while live) so the baked boolean stays
        // current when the source entity moves, even with the live boolean off.
        if (m_boolean.m_sourceEntity.IsValid() && m_boolean.m_sourceEntity != GetEntityId())
        {
            m_booleanSourceListener.m_owner = this;
            m_booleanSourceListener.BusConnect(m_boolean.m_sourceEntity);
        }
    }

    AZ::u32 EditorWhiteBoxComponent::OnCsgSolverChange()
    {
        m_layerRuntime.m_meshCache.clear();
        RebuildWhiteBox(); // re-evaluate every layer combine and the entity boolean with the new solver
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    AZ::u32 EditorWhiteBoxComponent::OnLiveBooleanChange()
    {
        UpdateBooleanSourceListener();
        RebuildWhiteBox(); // evaluates the live boolean (or reverts to base) + rebuilds render/physics
        return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
    }

    AZ::u32 EditorWhiteBoxComponent::OnBooleanSourceChange()
    {
        OnLiveBooleanChange();
        // EntityId fields ignore the ChangeNotify refresh level, so rebuild the tree to show/hide the group.
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
            AzToolsFramework::Refresh_EntireTree);
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    AZ::Crc32 EditorWhiteBoxComponent::BooleanGroupVisibility() const
    {
        return m_boolean.m_sourceEntity.IsValid() ? AZ::Edit::PropertyVisibility::Show
                                               : AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorWhiteBoxComponent::BooleanSourceListener::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        if (m_owner != nullptr)
        {
            // Rebuild once on the next tick: a dragged source sends many notifications per frame.
            m_owner->m_rebuild.m_liveBooleanPending = true;
        }
    }
} // namespace WhiteBox
