/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::RPI
{
    class ModelAsset;
}

namespace WhiteBox
{
    //! LOD 0 of a loaded model as a triangle soup, with each triangle's slot default material.
    bool TrianglesFromModel(
        const AZ::RPI::ModelAsset& model, AZStd::vector<AZ::Vector3>& positions, AZStd::vector<AZ::u32>& indices,
        AZStd::vector<AZ::Data::AssetId>& triangleMaterials, AZStd::string& error);

    //! Replace an entity's Mesh component with a White Box holding the same geometry, auto smoothed. One undo step.
    bool ConvertMeshEntityToWhiteBox(AZ::EntityId entityId, AZStd::string& message);
} // namespace WhiteBox
