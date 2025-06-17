/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Builders/Model/ModelExporterContexts.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>

namespace AZ
{
    namespace RPI
    {
        ModelAssetBuilderContext::ModelAssetBuilderContext(
            const AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::DataTypes::IMeshGroup& group,
            const AZ::SceneAPI::CoordinateSystemConverter coordSysConverter,
            const MaterialAssetsByUid& materialsByUid,
            Data::Asset<ModelAsset>& outputModelAsset,
            Data::Asset<SkinMetaAsset>& outputSkinMetaAsset,
            Data::Asset<MorphTargetMetaAsset>& outputMorphTargetMetaAsset)
            : m_scene(scene)
            , m_group(group)
            , m_coordSysConverter(coordSysConverter)
            , m_materialsByUid(materialsByUid)
            , m_outputModelAsset(outputModelAsset)
            , m_outputSkinMetaAsset(outputSkinMetaAsset)
            , m_outputMorphTargetMetaAsset(outputMorphTargetMetaAsset)
        {}

        MaterialAssetBuilderContext::MaterialAssetBuilderContext(
            const AZ::SceneAPI::Containers::Scene& scene, 
            MaterialAssetsByUid& outputMaterialsByUid)
            : m_scene(scene)
            , m_outputMaterialsByUid(outputMaterialsByUid)
        {}

    } // namespace RPI
} // namespace AZ
