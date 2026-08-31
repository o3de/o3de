/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/MeshletPackRule.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ::Meshlets::Builders
{
    MeshletPackRule::MeshletPackRule() = default;

    void MeshletPackRule::SetDefaults()
    {
        m_maxVerticesPerCluster  = 128;
        m_maxTrianglesPerCluster = 256;
        m_coneWeight             = 0.5f;
        m_meshFilter             = AZStd::vector<AZStd::string>{ "*" };
    }

    void MeshletPackRule::Reflect(ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<MeshletPackRule, AZ::SceneAPI::DataTypes::IRule>()
                ->Version(1)
                ->Field("maxVerticesPerCluster",  &MeshletPackRule::m_maxVerticesPerCluster)
                ->Field("maxTrianglesPerCluster", &MeshletPackRule::m_maxTrianglesPerCluster)
                ->Field("coneWeight",             &MeshletPackRule::m_coneWeight)
                ->Field("meshFilter",             &MeshletPackRule::m_meshFilter);

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<MeshletPackRule>("Meshlet Pack Rule",
                    "Bake a .azmeshletpack alongside this model's .azmodel product. "
                    "Required for the model to render through the meshlet feature processor.")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ->DataElement(Edit::UIHandlers::SpinBox,
                        &MeshletPackRule::m_maxVerticesPerCluster, "Max Vertices/Cluster",
                        "Cluster vertex budget. Default 128. Range [32, 255] (meshopt hard "
                        "cap is 255). Larger = fewer, bigger clusters = fewer per-cluster "
                        "draws + better vertex-cache reuse when many instances are on screen.")
                        ->Attribute(Edit::Attributes::Min, 32)
                        ->Attribute(Edit::Attributes::Max, 255)
                    ->DataElement(Edit::UIHandlers::SpinBox,
                        &MeshletPackRule::m_maxTrianglesPerCluster, "Max Triangles/Cluster",
                        "Cluster triangle budget. Default 256. Range [16, 512] (meshopt hard "
                        "cap is 512; the builder rounds DOWN to a multiple of 4). Raise this "
                        "to make clusters bigger and cut the per-cluster draw-command count.")
                        ->Attribute(Edit::Attributes::Min, 16)
                        ->Attribute(Edit::Attributes::Max, 512)
                        ->Attribute(Edit::Attributes::Step, 4)
                    ->DataElement(Edit::UIHandlers::Slider,
                        &MeshletPackRule::m_coneWeight, "Cone Weight",
                        "Per-cluster cone tightness for cone-cull (used in SP2). [0, 1]; default 0.5.")
                        ->Attribute(Edit::Attributes::Min, 0.0f)
                        ->Attribute(Edit::Attributes::Max, 1.0f);
            }
        }
    }

} // namespace AZ::Meshlets::Builders
