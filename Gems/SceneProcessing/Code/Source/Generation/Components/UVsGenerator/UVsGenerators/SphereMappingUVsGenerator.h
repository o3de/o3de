/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneData::GraphData { class MeshVertexUVData; } 

namespace AZ::UVsGeneration::Mesh::SphericalMapping
{
    //! A simple positional sphere mapping UV generator.
    //! The vertices from the mesh are projected onto a unit sphere and then that is used
    //! to generate UV coordinates.
    bool GenerateUVsSphericalMapping(
        const AZ::SceneAPI::DataTypes::IMeshData* meshData, // in
        AZ::SceneData::GraphData::MeshVertexUVData* uvData); // out
}
