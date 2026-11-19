/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxModelConvert.h"
#include "EditorWhiteBoxComponent.h"

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Name/Name.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    bool TrianglesFromModel(
        const AZ::RPI::ModelAsset& model, AZStd::vector<AZ::Vector3>& positions, AZStd::vector<AZ::u32>& indices,
        AZStd::vector<AZ::Data::AssetId>& triangleMaterials, AZStd::string& error)
    {
        positions.clear();
        indices.clear();
        triangleMaterials.clear();
        const auto lods = model.GetLodAssets();
        if (lods.empty() || !lods[0].IsReady())
        {
            error = "The model has no loaded level of detail.";
            return false;
        }
        const AZ::Name positionSemantic("POSITION");
        const auto& materialSlots = model.GetMaterialSlots();
        for (const auto& mesh : lods[0]->GetMeshes())
        {
            const auto points = mesh.GetSemanticBufferTyped<float>(positionSemantic);
            if (points.size() < 9 || points.size() % 3 != 0)
            {
                continue;
            }
            const auto base = static_cast<AZ::u32>(positions.size());
            for (size_t i = 0; i < points.size(); i += 3)
            {
                positions.emplace_back(points[i], points[i + 1], points[i + 2]);
            }
            const AZ::u32 elementSize = mesh.GetIndexBufferAssetView().GetBufferViewDescriptor().m_elementSize;
            const size_t before = indices.size();
            if (elementSize == sizeof(AZ::u16))
            {
                for (const AZ::u16 index : mesh.GetIndexBufferTyped<AZ::u16>())
                {
                    indices.push_back(base + index);
                }
            }
            else
            {
                for (const AZ::u32 index : mesh.GetIndexBufferTyped<AZ::u32>())
                {
                    indices.push_back(base + index);
                }
            }
            indices.resize(before + (indices.size() - before) / 3 * 3);
            const auto slot = materialSlots.find(mesh.GetMaterialSlotId());
            const AZ::Data::AssetId material = slot != materialSlots.end() ? slot->second.m_defaultMaterialAsset.GetId() : AZ::Data::AssetId{};
            triangleMaterials.resize(indices.size() / 3, material);
        }
        if (indices.empty())
        {
            error = "The model's first level of detail has no triangles White Box can read.";
            return false;
        }
        return true;
    }

    bool ConvertMeshEntityToWhiteBox(const AZ::EntityId entityId, AZStd::string& message)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        if (entity == nullptr)
        {
            message = "Select an entity with a Mesh component first.";
            return false;
        }
        AZ::Component* meshComponent = entity->FindComponent(AZ::Render::EditorMeshComponentTypeId);
        if (meshComponent == nullptr)
        {
            message = AZStd::string::format("%s has no Mesh component to convert.", entity->GetName().c_str());
            return false;
        }
        AZ::Data::Asset<const AZ::RPI::ModelAsset> model;
        AZ::Render::MeshComponentRequestBus::EventResult(model, entityId, &AZ::Render::MeshComponentRequests::GetModelAsset);
        if (!model.IsReady())
        {
            message = "The mesh's model has not finished loading. Try again in a moment.";
            return false;
        }

        AZStd::vector<AZ::Vector3> positions;
        AZStd::vector<AZ::u32> indices;
        AZStd::vector<AZ::Data::AssetId> materials;
        if (!TrianglesFromModel(*model.Get(), positions, indices, materials, message))
        {
            return false;
        }
        Api::WhiteBoxMeshPtr built = Api::CreateWhiteBoxMesh();
        if (!Api::BuildFromTriangles(*built, positions, indices, materials))
        {
            message = "Every triangle in the model was degenerate.";
            return false;
        }
        // Curved models import as many small faces; smoothing them keeps the look of the source.
        const Api::PolygonHandles polygons = Api::MeshPolygonHandles(*built);
        Api::AutoSmoothPolygons(*built, polygons, 40.0f);

        AzToolsFramework::ScopedUndoBatch undoBatch("Convert Mesh to White Box");
        AzToolsFramework::EntityCompositionRequests::RemoveComponentsOutcome removed = AZ::Failure(AZStd::string("uninitialized"));
        AZ::Component* const toRemove[] = { meshComponent };
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            removed, &AzToolsFramework::EntityCompositionRequests::RemoveComponents, AZStd::span<AZ::Component* const>(toRemove));
        if (!removed.IsSuccess())
        {
            message = AZStd::string::format("Could not remove the Mesh component: %s", removed.GetError().c_str());
            return false;
        }
        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome added = AZ::Failure(AZStd::string("uninitialized"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(
            added, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ entityId },
            AZ::ComponentTypeList{ azrtti_typeid<EditorWhiteBoxComponent>() });
        auto* whiteBox = entity->FindComponent<EditorWhiteBoxComponent>();
        if (!added.IsSuccess() || whiteBox == nullptr || whiteBox->GetWhiteBoxMesh() == nullptr)
        {
            message = "Could not add a White Box component. Undo restores the Mesh component.";
            return false;
        }
        // The fresh component starts as a parametric cube; freeze it so the imported mesh is not regenerated over.
        whiteBox->BakeParametricLayer(whiteBox->GetActiveLayerIndex());
        Api::AssignMesh(*whiteBox->GetWhiteBoxMesh(), *built);
        whiteBox->SerializeWhiteBox();
        whiteBox->RebuildWhiteBox();
        undoBatch.MarkEntityDirty(entityId);

        message = AZStd::string::format(
            "Converted %zu triangles into %zu polygons. A Material component on the entity no longer applies; per-slot materials were kept.",
            indices.size() / 3, polygons.size());
        return true;
    }
} // namespace WhiteBox
