/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OcclusionCullingPlane/OcclusionCullingPlaneFeatureProcessor.h>
#include <AzCore/Math/Random.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

namespace AZ
{
    namespace Render
    {
        OcclusionCullingPlane::~OcclusionCullingPlane()
        {
            Data::AssetBus::MultiHandler::BusDisconnect();
            m_meshFeatureProcessor->ReleaseMesh(m_visualizationMeshHandle);
        }

        void OcclusionCullingPlane::Init(RPI::Scene* scene)
        {
            AZ_Assert(scene, "OcclusionCullingPlane::Init called with a null Scene pointer");

            m_meshFeatureProcessor = scene->GetFeatureProcessor<Render::MeshFeatureProcessorInterface>();

            // load visualization plane model and material
            m_visualizationModelAsset = AZ::RPI::AssetUtils::LoadCriticalAsset<AZ::RPI::ModelAsset>(
                "Models/OcclusionCullingPlane.fbx.azmodel",
                AZ::RPI::AssetUtils::TraceLevel::Assert);

            m_visualizationMeshHandle = m_meshFeatureProcessor->AcquireMesh(MeshHandleDescriptor(m_visualizationModelAsset));
            m_meshFeatureProcessor->SetExcludeFromReflectionCubeMaps(m_visualizationMeshHandle, true);
            m_meshFeatureProcessor->SetRayTracingEnabled(m_visualizationMeshHandle, false);
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, AZ::Transform::CreateIdentity());

            SetVisualizationMaterial();
        }

        void OcclusionCullingPlane::SetVisualizationMaterial()
        {
            AZStd::string materialAssetPath;
            if (m_transparentVisualization)
            {
                materialAssetPath = "Materials/OcclusionCullingPlane/OcclusionCullingPlaneTransparentVisualization.azmaterial";
            }
            else
            {
                materialAssetPath = "Materials/OcclusionCullingPlane/OcclusionCullingPlaneVisualization.azmaterial";
            }

            RPI::AssetUtils::TraceLevel traceLevel = AZ::RPI::AssetUtils::TraceLevel::Assert;
            m_visualizationMaterialAsset = AZ::RPI::AssetUtils::LoadCriticalAsset<AZ::RPI::MaterialAsset>(materialAssetPath.c_str(), traceLevel);
            m_visualizationMaterialAsset.QueueLoad();
            Data::AssetBus::MultiHandler::BusConnect(m_visualizationMaterialAsset.GetId());
        }

        void OcclusionCullingPlane::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            if (m_visualizationMaterialAsset.GetId() == asset.GetId())
            {
                m_visualizationMaterialAsset = asset;
                Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

                m_visualizationMaterial = AZ::RPI::Material::FindOrCreate(m_visualizationMaterialAsset);
                m_meshFeatureProcessor->SetCustomMaterials(m_visualizationMeshHandle, m_visualizationMaterial);
            }
        }

        void OcclusionCullingPlane::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            AZ_Error("OcclusionCullingPlane", false, "Failed to load OcclusionCullingPlane visualization asset %s", asset.ToString<AZStd::string>().c_str());
            Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        }

        void OcclusionCullingPlane::SetTransform(const AZ::Transform& transform)
        {
            m_transform = transform;

            // update visualization plane transform
            m_meshFeatureProcessor->SetTransform(m_visualizationMeshHandle, transform);
        }

        void OcclusionCullingPlane::ShowVisualization(bool showVisualization)
        {
            if (m_showVisualization != showVisualization)
            {
                m_meshFeatureProcessor->SetVisible(m_visualizationMeshHandle, showVisualization);
                SetVisualizationMaterial();
            }
        }

        void OcclusionCullingPlane::SetTransparentVisualization(bool transparentVisualization)
        {
            if (m_transparentVisualization != transparentVisualization)
            {
                m_transparentVisualization = transparentVisualization;
                SetVisualizationMaterial();
            }
        }

    } // namespace Render
} // namespace AZ
