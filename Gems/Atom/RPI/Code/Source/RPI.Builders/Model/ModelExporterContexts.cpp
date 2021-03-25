/*
 * All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution(the "License").All use of this software is governed by the License,
 *or, if provided, by the license below or the license accompanying this file.Do not
 * remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Model/ModelExporterContexts.h>

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
