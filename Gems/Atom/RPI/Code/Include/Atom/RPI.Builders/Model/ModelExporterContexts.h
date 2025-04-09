/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAsset.h>

#include <AzCore/Asset/AssetCommon.h>

#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Utilities/CoordinateSystemConverter.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshGroup;
        }
        namespace Containers
        {
            class Scene;
        }
    } // namespace SceneAPI

    namespace RPI
    {
        struct NamedMaterialAsset
        {
            Data::Asset<MaterialAsset> m_asset;
            AZStd::string m_name;
        };
        using MaterialAssetsByUid = AZStd::unordered_map<uint64_t, NamedMaterialAsset>;

        struct ModelAssetBuilderContext : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ModelAssetBuilderContext, "{63FEFB4B-25DC-48DD-AC72-D27DA9A6D94A}", AZ::SceneAPI::Events::ICallContext);

            ModelAssetBuilderContext(
                const AZ::SceneAPI::Containers::Scene& scene,
                const AZ::SceneAPI::DataTypes::IMeshGroup& group,
                const AZ::SceneAPI::CoordinateSystemConverter coordSysConverter,
                const MaterialAssetsByUid& materialsByUid,
                Data::Asset<ModelAsset>& outputModelAsset,
                Data::Asset<SkinMetaAsset>& outputSkinMetaAsset,
                Data::Asset<MorphTargetMetaAsset>& outputMorphTargetMetaAsset);
            ~ModelAssetBuilderContext() override = default;

            ModelAssetBuilderContext& operator=(const ModelAssetBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            const AZ::SceneAPI::DataTypes::IMeshGroup& m_group;
            const MaterialAssetsByUid& m_materialsByUid;
            AZ::SceneAPI::CoordinateSystemConverter m_coordSysConverter;
            Data::Asset<ModelAsset>& m_outputModelAsset;
            Data::Asset<SkinMetaAsset>& m_outputSkinMetaAsset;
            Data::Asset<MorphTargetMetaAsset>& m_outputMorphTargetMetaAsset;
        };

        struct ModelAssetPostBuildContext : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ModelAssetPostBuildContext, "{E0AA70B6-FA06-41E9-A137-60D7DCB85115}", AZ::SceneAPI::Events::ICallContext);

            ModelAssetPostBuildContext(
                const AZ::SceneAPI::Containers::Scene& scene,
                AZStd::string outputDirectory,
                SceneAPI::Events::ExportProductList& productList,
                const AZ::SceneAPI::DataTypes::IMeshGroup& group,
                const Data::Asset<ModelAsset>& modelAsset)
                : m_scene(scene)
                , m_outputDirectory(outputDirectory)
                , m_productList(productList)
                , m_group(group)
                , m_modelAsset(modelAsset)
            {
            }
            ~ModelAssetPostBuildContext() override = default;

            ModelAssetPostBuildContext& operator=(const ModelAssetPostBuildContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            AZStd::string m_outputDirectory;
            SceneAPI::Events::ExportProductList& m_productList;
            const AZ::SceneAPI::DataTypes::IMeshGroup& m_group;
            const Data::Asset<ModelAsset>& m_modelAsset;
        };

        struct MaterialAssetBuilderContext : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(MaterialAssetBuilderContext, "{6451418A-453B-4646-A5B2-A5687FA2E97F}", AZ::SceneAPI::Events::ICallContext);

            MaterialAssetBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, MaterialAssetsByUid& outputMaterialsByUid);
            ~MaterialAssetBuilderContext() override = default;

            MaterialAssetBuilderContext& operator=(const MaterialAssetBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            MaterialAssetsByUid& m_outputMaterialsByUid;
        };
    } // namespace RPI
} // namespace AZ
