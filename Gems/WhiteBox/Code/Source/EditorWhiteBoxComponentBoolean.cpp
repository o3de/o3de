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

        // Bring the source mesh from its local space into this entity's local space:
        // operandTransform = thisWorldFromLocal^-1 * sourceWorldFromLocal.
        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_boolean.m_sourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform operandTransform = thisWorldTM.GetInverse() * sourceWorldTM;

        AzToolsFramework::ScopedUndoBatch undoBatch("White Box Boolean");

        if (!Api::ApplyMeshBoolean(*targetMesh, *sourceMesh, operandTransform, m_boolean.m_operation, m_csgSolver))
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
        if (!m_boolean.m_sourceEntity.IsValid() || m_boolean.m_sourceEntity == GetEntityId())
        {
            return nullptr;
        }

        WhiteBoxMesh* baseMesh = GetWhiteBoxMesh();
        if (baseMesh == nullptr)
        {
            return nullptr;
        }

        AZ::Entity* sourceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            sourceEntity, &AZ::ComponentApplicationRequests::FindEntity, m_boolean.m_sourceEntity);
        if (sourceEntity == nullptr)
        {
            return nullptr;
        }
        const auto sourceComponents = sourceEntity->FindComponents<EditorWhiteBoxComponent>();
        if (sourceComponents.empty())
        {
            return nullptr;
        }
        WhiteBoxMesh* sourceMesh = sourceComponents[0]->GetEvaluatedWhiteBoxMesh();
        if (sourceMesh == nullptr)
        {
            return nullptr;
        }

        AZ::Transform thisWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(thisWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform sourceWorldTM = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(sourceWorldTM, m_boolean.m_sourceEntity, &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform operandTransform = thisWorldTM.GetInverse() * sourceWorldTM;

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
        if (Api::ApplyMeshBoolean(*evaluated, *sourceMesh, operandTransform, m_boolean.m_operation, m_csgSolver))
        {
            Api::CalculateNormals(*evaluated);
            Api::CalculatePlanarUVs(*evaluated);
            return evaluated;
        }
        // on failure (no overlap) return null -> callers fall back to the base mesh.
        return nullptr;
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
