/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/JsonSidecarDescriptor.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>

namespace AZ::Meshlets::Builders
{
    void JsonSidecarDescriptor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JsonSidecarDescriptor>()
                ->Version(4)
                ->Field("source_model_asset_id",   &JsonSidecarDescriptor::m_sourceModelAssetIdStr)
                ->Field("max_vertices",            &JsonSidecarDescriptor::m_maxVerticesPerCluster)
                ->Field("max_triangles",           &JsonSidecarDescriptor::m_maxTrianglesPerCluster)
                ->Field("cone_weight",             &JsonSidecarDescriptor::m_coneWeight)
                ->Field("generate_cluster_dag",    &JsonSidecarDescriptor::m_generateClusterDag)
                ->Field("generate_pages",          &JsonSidecarDescriptor::m_generatePages)
                ->Field("mesh_filter",             &JsonSidecarDescriptor::m_meshFilter);
        }
    }

} // namespace AZ::Meshlets::Builders
